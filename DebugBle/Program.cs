using System;
using System.Collections.Generic;


namespace DebugBle
{
    class Program
    {
        static BleWinrt ble = new BleWinrt();

		static void Main(string[] args)
        {
			Console.WriteLine($"scan started");

            ble.DeviceAdded += OnAdded;
            ble.DeviceUpdated += OnUpdated;
            ble.DeviceRemoved += OnRemoved;
            ble.StartScan();

			//string deviceId = null;

			//BLE.BLEScan scan = BLE.ScanDevices();
			/*
            scan.Found = (mac, deviceName) =>
            {
                Console.WriteLine($"found device {mac}: {deviceName}");
            };
            scan.Finished = () =>
            {
                Console.WriteLine("scan finished");
                if (deviceId == null)
                    deviceId = "-1";
            };
            while (deviceId == null)
                Thread.Sleep(500);

            scan.Cancel();
            if (deviceId == "-1")
            {
                Console.WriteLine("no device found!");
                return;
            }

            ble.Connect(deviceId,
                "{f6f04ffa-9a61-11e9-a2a3-2a2ae2dbcce4}", 
                new string[] { "{f6f07c3c-9a61-11e9-a2a3-2a2ae2dbcce4}",
                    "{f6f07da4-9a61-11e9-a2a3-2a2ae2dbcce4}",
                    "{f6f07ed0-9a61-11e9-a2a3-2a2ae2dbcce4}" });

            for(int guard = 0; guard < 2000; guard++)
            {
                BLE.ReadPackage();
                BLE.WritePackage(deviceId,
                    "{f6f04ffa-9a61-11e9-a2a3-2a2ae2dbcce4}",
                    "{f6f07ffc-9a61-11e9-a2a3-2a2ae2dbcce4}",
                    new byte[] { 0, 1, 2 });
                Console.WriteLine(BLE.GetError());
                Thread.Sleep(5);
            }
*/
			Console.WriteLine("Press enter to exit the program...");
            Console.ReadLine();
        }

        static async void OnAdded(BleWinrt.DeviceInfo deviceInfo)
        {
            Console.WriteLine($"+ {deviceInfo}");

            string id = deviceInfo.id;

            BleWinrt.BleServiceArray carrier = await ble.GetServices(id);

            Console.WriteLine(id + " // " + carrier.count);

//                foreach (var service in carrier.services)
//                    Console.WriteLine($"\t{service.serviceUuid}");
		}

		static void OnRemoved(BleWinrt.DeviceInfoUpdate deviceInfoUpdate)
		{
			Console.WriteLine($"- {deviceInfoUpdate}");
		}

		static void OnUpdated(BleWinrt.DeviceInfoUpdate deviceInfoUpdate)
		{
			Console.WriteLine($". {deviceInfoUpdate}");
		}

		static void OnCompleted()
		{
			Console.WriteLine($"scan completed");
		}

		static void OnStopped()
        {
			Console.WriteLine($"scan stopped");
		}
	}
}
