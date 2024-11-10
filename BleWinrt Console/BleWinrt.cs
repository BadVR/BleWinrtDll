using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Threading.Tasks;


public class BleWinrt
{
	public delegate void LogCallback(string log);
	public delegate void ErrorCallback(string err);

	[DllImport("BleWinrt.dll", EntryPoint = "RegisterLogCallback")]
	public static extern void RegisterLogCallback(LogCallback logCb);

	[DllImport("BleWinrt.dll", EntryPoint = "RegisterErrorCallback")]
	public static extern void RegisterErrorCallback(ErrorCallback logCb);


	public delegate void AdvertCallback(BleAdvert ad);
	public delegate void StoppedCallback();
	public delegate void DisconnectedCallback();

	public delegate void ServicesFoundCallback(BleServiceArray services);
	public delegate void CharacteristicsFoundCallback(BleCharacteristicArray characteristics);

	public delegate void SubscribeCallback();
	public delegate void ReadBytesCallback();
	public delegate void WriteBytesCallback();

	//static events for device scanning
	public AdvertCallback Advertisement;
	public StoppedCallback ScanStopped;


	[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
	public struct BleAdvert
	{
		public ulong mac;

		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
		public string name;

		public int signalStrength;

		public override string ToString()
		{
            string str = MacHex(mac);

            if (!string.IsNullOrEmpty(name))
                str += " / " + name;

            str += $" // {signalStrength}";

            return str;
	    }
	}

	public struct BleService
	{
		public Guid serviceUuid;
	};

	[StructLayout(LayoutKind.Sequential)]
	public struct BleServiceArray
	{
		public IntPtr services;  // Pointer to the array of structs
		public int count;        // Number of elements in the array
	}

	[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
	public struct BleCharacteristic
	{
		public Guid serviceUuid;

		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
		public string userDescription;
	};

	[StructLayout(LayoutKind.Sequential)]
	public struct BleCharacteristicArray
	{
		public IntPtr characteristics;  // Pointer to the array of structs
		public int count;        // Number of elements in the array
	}

	[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
	public struct BleData
	{
		public Guid serviceUuid;
		public Guid characteristicUuid;

		[MarshalAs(UnmanagedType.ByValArray, SizeConst = 512)]
		public byte[] buf;

		public short size;
	};


	bool is_scanning = false;

	public void StartScan()
	{
		if (is_scanning)
			return;

		//flag as scanning
		is_scanning = true;

		StartDeviceScan(Advertisement, InternalScanStopped);
	}

	public void StopScan()
	{
		if (!is_scanning)
			return;

		StopDeviceScan();
	}

	public Task<List<BleService>> GetServices(ulong addr)
	{
		var tcs = new TaskCompletionSource<List<BleService>>();

		ScanServices(addr, (arr) =>
		{
			List<BleService> services = new List<BleService>();

			for (int i = 0; i < arr.count; i++)
			{
				//read the record
				IntPtr servicePtr = IntPtr.Add(arr.services, i * Marshal.SizeOf(typeof(BleService)));
				BleService service = Marshal.PtrToStructure<BleService>(servicePtr);

				services.Add(service);
			}

			tcs.SetResult(services);
		});

		return tcs.Task;
	}

	public Task<List<BleCharacteristic>> GetCharacteristics(ulong addr, Guid serviceUuid)
	{
		var tcs = new TaskCompletionSource<List<BleCharacteristic>>();

		ScanCharacteristics(addr, serviceUuid, (arr) =>
		{
			List<BleCharacteristic> characteristics = new List<BleCharacteristic>();

			for (int i = 0; i < arr.count; i++)
			{
				//read the record
				IntPtr servicePtr = IntPtr.Add(arr.characteristics, i * Marshal.SizeOf(typeof(BleCharacteristic)));
				BleCharacteristic @char = Marshal.PtrToStructure<BleCharacteristic>(servicePtr);

				characteristics.Add(@char);
			}

			tcs.SetResult(characteristics);
		});

		return tcs.Task;
	}

	public void Disconnect(ulong addr, DisconnectedCallback disconnectedCb)
	{
		DisconnectDevice(addr, disconnectedCb);
	}

	void InternalScanStopped()
	{
		//reset flag
		is_scanning = false;

		//relay the completion
		ScanStopped?.Invoke();
	}
	

	[DllImport("BleWinrt.dll", EntryPoint = "StartDeviceScan", CharSet = CharSet.Unicode)]
    public static extern void StartDeviceScan(AdvertCallback addedCallback, StoppedCallback stoppedCallback);

    [DllImport("BleWinrt.dll", EntryPoint = "StopDeviceScan")]
    public static extern void StopDeviceScan();


	[DllImport("BleWinrt.dll", EntryPoint = "DisconnectDevice", CharSet = CharSet.Unicode)]
	public static extern void DisconnectDevice(ulong addr, DisconnectedCallback disconnectedCb);


	[DllImport("BleWinrt.dll", EntryPoint = "ScanServices", CharSet = CharSet.Unicode)]
	public static extern void ScanServices(ulong addr, ServicesFoundCallback serviceFoundCb);

	[DllImport("BleWinrt.dll", EntryPoint = "ScanCharacteristics", CharSet = CharSet.Unicode)]
    public static extern void ScanCharacteristics(ulong addr, Guid serviceUuid, CharacteristicsFoundCallback characteristicFoundCb);


    [DllImport("BleWinrt.dll", EntryPoint = "SubscribeCharacteristic", CharSet = CharSet.Unicode)]
    public static extern void SubscribeCharacteristic(ulong addr, Guid serviceUuid, Guid characteristicUuid, SubscribeCallback subscribeCallback);


	[DllImport("BleWinrt.dll", EntryPoint = "ReadData", CharSet = CharSet.Unicode)]
	public static extern void ReadBytes(ulong addr, Guid serviceUuid, Guid characteristicUuid, ReadBytesCallback readBufferCb);

	[DllImport("BleWinrt.dll", EntryPoint = "WriteData", CharSet = CharSet.Unicode)]
    public static extern void WriteBytes(ulong addr, Guid serviceUuid, Guid characteristicUuid, byte[] buf, int size, WriteBytesCallback writeBytesCb);

	/// <summary>
	/// close everything and clean up
	/// </summary>
    [DllImport("BleWinrt.dll", EntryPoint = "Quit")]
    public static extern void Quit();

	public static string MacHex(ulong mac)
	{
		// Extract each byte and format as hex with two digits and colons in between
		return string.Format("{0:X2}:{1:X2}:{2:X2}:{3:X2}:{4:X2}:{5:X2}",
			(mac >> 40) & 0xFF,
			(mac >> 32) & 0xFF,
			(mac >> 24) & 0xFF,
			(mac >> 16) & 0xFF,
			(mac >> 8) & 0xFF,
			mac & 0xFF);
	}
}