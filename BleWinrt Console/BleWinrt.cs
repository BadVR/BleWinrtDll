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

	public delegate void SubscribeCallback(ulong deviceAddress, Guid serviceUuid, Guid characteristicUuid, byte[] data, ulong size);
	public delegate void ReadBytesCallback(byte[] data, ulong size);
	public delegate void WriteBytesCallback(bool success);


	[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
	public struct BleAdvert
	{
		public ulong mac;

		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
		public string name;

		public long timestamp;

		public int signalStrength;
		public int powerLevel;

		//pointer to array of Guid service uuids
		public IntPtr serviceUuids;
		public int numServiceUuids;

		public override readonly string ToString()
		{
			string str = MacHex(mac);

			if (!string.IsNullOrEmpty(name))
				str += " / " + name;

			str += $" // {signalStrength}";

			return str;
		}

		public readonly string ToServicesString()
		{
			//TODO
			return "";
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


	public void Initialize(AdvertCallback advertCb, StoppedCallback stoppedCb = null)
	{
		InitializeScan(null, Guid.Empty, advertCb, stoppedCb);
	}

	public void Initialize(Guid serviceFilter, AdvertCallback advertCb, StoppedCallback stoppedCb = null)
	{
		InitializeScan(null, serviceFilter, advertCb, stoppedCb);
	}

	public void Initialize(string nameFilter, AdvertCallback advertCb, StoppedCallback stoppedCb = null)
	{
		InitializeScan(nameFilter, Guid.Empty, advertCb, stoppedCb);
	}

	public void Initialize(string nameFilter, Guid serviceFilter, AdvertCallback advertCb, StoppedCallback stoppedCb = null)
	{
		InitializeScan(nameFilter, serviceFilter, advertCb, stoppedCb);
	}

	public void Start()
	{
		StartScan();
	}

	public void Stop()
	{
		StopScan();
	}

	public void ConnectDevice(ulong addr, ServicesFoundCallback serviceFoundCb, CharacteristicsFoundCallback characteristicFoundCb)
	{
		//scan services and characteristics for specific device
	}

	public Task<List<BleService>> GetServices(ulong addr)
	{
		var tcs = new TaskCompletionSource<List<BleService>>();

		ScanServices(addr, (arr) =>
		{
			List<BleService> services = new();

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
			List<BleCharacteristic> characteristics = new();

			for (int i = 0; i < arr.count; i++)
			{
				//read the record
				IntPtr servicePtr = IntPtr.Add(arr.characteristics, i * Marshal.SizeOf(typeof(BleCharacteristic)));
				BleCharacteristic characteristic = Marshal.PtrToStructure<BleCharacteristic>(servicePtr);

				characteristics.Add(characteristic);
			}

			tcs.SetResult(characteristics);
		});

		return tcs.Task;
	}

	public void Disconnect(ulong addr, DisconnectedCallback disconnectedCb)
	{
		DisconnectDevice(addr, disconnectedCb);
	}


	[DllImport("BleWinrt.dll", EntryPoint = "InitializeScan", CharSet = CharSet.Unicode)]
	static extern void InitializeScan(string nameFilter, Guid serviceFilter, AdvertCallback addedCallback, StoppedCallback stoppedCallback);

	[DllImport("BleWinrt.dll", EntryPoint = "StartScan")]
	static extern void StartScan();

	[DllImport("BleWinrt.dll", EntryPoint = "StopScan")]
	static extern void StopScan();

	[DllImport("BleWinrt.dll", EntryPoint = "DisconnectDevice", CharSet = CharSet.Unicode)]
	static extern void DisconnectDevice(ulong addr, DisconnectedCallback disconnectedCb);


	[DllImport("BleWinrt.dll", EntryPoint = "ScanServices", CharSet = CharSet.Unicode)]
	static extern void ScanServices(ulong addr, ServicesFoundCallback serviceFoundCb);

	[DllImport("BleWinrt.dll", EntryPoint = "ScanCharacteristics", CharSet = CharSet.Unicode)]
	static extern void ScanCharacteristics(ulong addr, Guid serviceUuid, CharacteristicsFoundCallback characteristicFoundCb);


	[DllImport("BleWinrt.dll", EntryPoint = "SubscribeCharacteristic", CharSet = CharSet.Unicode)]
	static extern void SubscribeCharacteristic(ulong addr, Guid serviceUuid, Guid characteristicUuid, SubscribeCallback subscribeCallback);

	[DllImport("BleWinrt.dll", EntryPoint = "UnsubscribeCharacteristic", CharSet = CharSet.Unicode)]
	static extern void UnsubscribeCharacteristic(ulong addr, Guid serviceUuid, Guid characteristicUuid);


	[DllImport("BleWinrt.dll", EntryPoint = "ReadData", CharSet = CharSet.Unicode)]
	static extern void ReadBytes(ulong addr, Guid serviceUuid, Guid characteristicUuid, ReadBytesCallback readBufferCb);

	[DllImport("BleWinrt.dll", EntryPoint = "WriteData", CharSet = CharSet.Unicode)]
	static extern void WriteBytes(ulong addr, Guid serviceUuid, Guid characteristicUuid, byte[] buf, int size, WriteBytesCallback writeBytesCb);

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