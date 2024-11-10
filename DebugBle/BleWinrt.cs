using System.Runtime.InteropServices;


public class BleWinrt
{
	const int ID_SIZE = 128;
	const int UUID_SIZE = 100;

	public delegate void LogCallback(string log);
	public delegate void ErrorCallback(string err);

	[DllImport("BleWinrtDll.dll", EntryPoint = "RegisterLogCallback")]
	public static extern void RegisterLogCallback(LogCallback logCb);

	[DllImport("BleWinrtDll.dll", EntryPoint = "RegisterErrorCallback")]
	public static extern void RegisterErrorCallback(ErrorCallback logCb);


	public delegate void DeviceInfoCallback(DeviceInfo deviceInfo);
	public delegate void DeviceUpdateCallback(DeviceInfoUpdate deviceInfoUpdate);
	public delegate void CompletedCallback();
	public delegate void StoppedCallback();

	public delegate void ConnectedCallback(string id);
	public delegate void ServiceFoundCallback(BleService service);
	public delegate void CharacteristicFoundCallback(BleCharacteristic characteristic);

	public delegate void SubscribeCallback();
	public delegate void ReadBytesCallback();
	public delegate void WriteBytesCallback();

	//static events for device scanning
	public DeviceInfoCallback DeviceAdded;
	public DeviceUpdateCallback DeviceUpdated;
	public DeviceUpdateCallback DeviceRemoved;

	public CompletedCallback ScanCompleted;
	public StoppedCallback ScanStopped;


	[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
	public struct DeviceInfo
	{
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = ID_SIZE)]
		public string id;

		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = ID_SIZE)]
		public string name;

		[MarshalAs(UnmanagedType.I8)]
		public ulong mac;

		[MarshalAs(UnmanagedType.I4)]
		public int signalStrength;

		[MarshalAs(UnmanagedType.I1)]
		public bool signalStrengthPresent;

		[MarshalAs(UnmanagedType.I1)]
		public bool isConnected;

		[MarshalAs(UnmanagedType.I1)]
		public bool isConnectedPresent;

		[MarshalAs(UnmanagedType.I1)]
		public bool isConnectable;

		[MarshalAs(UnmanagedType.I1)]
		public bool isConnectablePresented;

		public override string ToString()
		{
            string str = id;

            if (!string.IsNullOrEmpty(name))
                str += " / " + name;

            if (signalStrengthPresent)
                str += $" // {signalStrength}";

            return str;
	    }
	}

	[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public struct DeviceInfoUpdate
    {
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = ID_SIZE)]
		public string id;

		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = ID_SIZE)]
		public string name;

		[MarshalAs(UnmanagedType.I1)]
		public bool namePresent;

		[MarshalAs(UnmanagedType.I4)]
		public int signalStrength;

		[MarshalAs(UnmanagedType.I1)]
		public bool signalStrengthPresent;

		[MarshalAs(UnmanagedType.I1)]
		public bool isConnected;

		[MarshalAs(UnmanagedType.I1)]
		public bool isConnectedUpdated;

		[MarshalAs(UnmanagedType.I1)]
		public bool isConnectable;

		[MarshalAs(UnmanagedType.I1)]
		public bool isConnectableUpdated;

		public override string ToString()
		{
			string str = id;

			if (namePresent && !string.IsNullOrEmpty(name))
				str += " / " + name;

			if (signalStrengthPresent)
				str += $" // {signalStrength}";

			return str;
		}
	}

	[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
	public struct BleService
	{
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 100)]
		public string serviceUuid;
	};

	[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
	public struct BleCharacteristic
	{
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = UUID_SIZE)]
		public string serviceUuid;

		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = ID_SIZE)]
		public string userDescription;
	};

	[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
	public struct BleData
	{
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = ID_SIZE)]
		public string id;

		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = UUID_SIZE)]
		public string serviceUuid;

		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = UUID_SIZE)]
		public string characteristicUuid;


		[MarshalAs(UnmanagedType.ByValArray, SizeConst = 512)]
		public byte[] buf;

		[MarshalAs(UnmanagedType.I2)]
		public short size;
	};


	bool is_scanning = false;

	public void StartScan()
	{
		if (is_scanning)
			return;

		//flag as scanning
		is_scanning = true;

		StartDeviceScan(DeviceAdded, DeviceUpdated, DeviceRemoved, InternalScanCompleted, InternalScanStopped);
	}

	public void Stop()
	{
		if (!is_scanning)
			return;

		StopDeviceScan();
	}

	void InternalScanCompleted()
	{
		//reset flag
		is_scanning = false;

		//relay the completion
		ScanCompleted?.Invoke();
	}

	void InternalScanStopped()
	{
		//reset flag
		is_scanning = false;

		//relay the completion
		ScanStopped?.Invoke();
	}

	[DllImport("BleWinrtDll.dll", EntryPoint = "StartDeviceScan", CharSet = CharSet.Unicode)]
    public static extern void StartDeviceScan(DeviceInfoCallback addedCallback, DeviceUpdateCallback updatedCallback, DeviceUpdateCallback removedCallback, CompletedCallback completedCallback, StoppedCallback stoppedCallback);

    [DllImport("BleWinrtDll.dll", EntryPoint = "StopDeviceScan")]
    public static extern void StopDeviceScan();


    [DllImport("BleWinrtDll.dll", EntryPoint = "Connect")]
    public static extern void Connect(string id, ConnectedCallback connectedCb);

	[DllImport("BleWinrtDll.dll", EntryPoint = "Disconnect")]
	public static extern void Disconnect(string id, ConnectedCallback disconnectedCb);


	[DllImport("BleWinrtDll.dll", EntryPoint = "ScanServices")]
	public static extern void ScanServices(string id, ServiceFoundCallback serviceFoundCb);

	[DllImport("BleWinrtDll.dll", EntryPoint = "ScanCharacteristics", CharSet = CharSet.Unicode)]
    public static extern void ScanCharacteristics(string id, string serviceUuid, CharacteristicFoundCallback characteristicFoundCb);

    [DllImport("BleWinrtDll.dll", EntryPoint = "SubscribeCharacteristic", CharSet = CharSet.Unicode)]
    public static extern void SubscribeCharacteristic(string id, string serviceUuid, string characteristicUuid, SubscribeCallback subscribeCallback);


	[DllImport("BleWinrtDll.dll", EntryPoint = "ReadData", CharSet = CharSet.Unicode)]
	public static extern void ReadBytes(string id, string serviceUuid, string characteristicUuid, ReadBytesCallback readBufferCb);

	[DllImport("BleWinrtDll.dll", EntryPoint = "WriteData", CharSet = CharSet.Unicode)]
    public static extern void WriteBytes(string id, string serviceUuid, string characteristicUuid, byte[] buf, int size, WriteBytesCallback writeBytesCb);

	/// <summary>
	/// close everything and clean up
	/// </summary>
    [DllImport("BleWinrtDll.dll", EntryPoint = "Quit")]
    public static extern void Quit();
}