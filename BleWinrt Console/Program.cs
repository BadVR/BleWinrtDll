using System;
using System.Collections.Generic;
using static BleWinrt;


namespace BleWinrtConsole
{
    class Program
    {
        static readonly BleWinrt ble = new();
        static readonly HashSet<ulong> set = new();

		static void Main(string[] args)
        {
			Console.WriteLine($"scan started");

            ble.StartScan(OnAdvertisement, OnStopped);

			Console.WriteLine("Press enter to exit the program...");
            Console.ReadLine();

			ble.StopScan();
        }

        static async void OnAdvertisement(BleAdvert ad)
        {
            Console.WriteLine(ad);

            //check if added it
            if (set.Add(ad.mac))
            {
				try
				{
					List<BleService> services = await ble.GetServices(ad.mac);

					string str = ad + " >>> " + services.Count + " service(s)";

					if (services.Count > 0)
						str += "\n";

					for (int i = 0; i < services.Count; i++)
					{
						Guid serviceUuid = services[i].serviceUuid;

						str += $"- {serviceUuid}\n";

						List<BleCharacteristic> characteristics = await ble.GetCharacteristics(ad.mac, serviceUuid);

						for (int j = 0; j < characteristics.Count; j++)
						{
							Guid charUuid = characteristics[j].serviceUuid;

							str += $"  {charUuid}\n";
						}

						if (i < services.Count - 1)
							str += "\n";
					}

					Console.WriteLine(str);
				}
				catch (Exception ex)
				{
					Console.WriteLine(ex);
				}
			}
		}

		static void OnStopped()
        {
			Console.WriteLine($"scan stopped");
		}
	}
}
