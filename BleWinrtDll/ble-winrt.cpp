#include "stdafx.h"
#include "carriers.h"
#include "ble-winrt.h"
#include "serialization.h"
#include "logging.h"
#include "cache.h"

#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>

#define __WFILE__ L"ble-winrt.cpp"

using namespace winrt;
using namespace Windows::Devices::Bluetooth;
using namespace Windows::Devices::Bluetooth::Advertisement;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;


ReceivedCallback* receivedCallback = nullptr;
StoppedCallback* stoppedCallback = nullptr;

//TODO: move to this instead
BluetoothLEAdvertisementWatcher advertisementWatcher{ nullptr };

// global flag to release calling thread
mutex quitLock;
bool quitFlag = false;

list<Subscription*> subscriptions;


void StartDeviceScan(ReceivedCallback addedCb, StoppedCallback stoppedCb)
{
	{
		std::lock_guard lock(quitLock);
		quitFlag = false;
	}

	receivedCallback = addedCb;
	stoppedCallback = stoppedCb;

	// Create BluetoothLEAdvertisementWatcher and set scanning mode
	advertisementWatcher = BluetoothLEAdvertisementWatcher();
	advertisementWatcher.ScanningMode(BluetoothLEScanningMode::Active);

	// Handle received advertisements
	advertisementWatcher.Received([](BluetoothLEAdvertisementWatcher const&, BluetoothLEAdvertisementReceivedEventArgs const& args)
		{
			BleAdvert di;

			di.mac = args.BluetoothAddress();
			di.signalStrength = args.RawSignalStrengthInDBm();

			// Check if TransmitPowerLevelInDBm has a value and assign it if available
			if (args.TransmitPowerLevelInDBm())
				di.powerLevel = args.TransmitPowerLevelInDBm().Value();
			else
				di.powerLevel = 0; // or set to a default if no power level is provided

			// Retrieve the device name from the advertisement
			auto advertisement = args.Advertisement();
			if (!advertisement.LocalName().empty())
				wcscpy_s(di.name, ID_SIZE, advertisement.LocalName().c_str());
			else
				wcscpy_s(di.name, ID_SIZE, L"");

			if (receivedCallback)
				(*receivedCallback)(&di);
		});

	// Handle watcher stopped
	advertisementWatcher.Stopped([](BluetoothLEAdvertisementWatcher const&, BluetoothLEAdvertisementWatcherStoppedEventArgs const& args)
	{
		if (stoppedCallback)
			(*stoppedCallback)();
	});

	advertisementWatcher.Start();
}

void StopDeviceScan()
{
	if (advertisementWatcher == nullptr)
		return;

	advertisementWatcher.Stop();
	advertisementWatcher = nullptr;
}

void DisconnectDevice(uint64_t deviceAddress, DisconnectedCallback connectedCb)
{
	try
	{
		RemoveFromCache(deviceAddress);

		if (connectedCb)
			(*connectedCb)(deviceAddress);
	}
	catch (const std::exception& e)
	{
		Log(e.what());

		if (connectedCb)
			(*connectedCb)(0);
	}
}

void ScanServices(uint64_t deviceAddress, ServicesFoundCallback serviceFoundCb)
{
	ScanServicesAsync(deviceAddress, serviceFoundCb);
}

void ScanCharacteristics(uint64_t deviceAddress, guid serviceUuid, CharacteristicsFoundCallback characteristicFoundCb)
{
	ScanCharacteristicsAsync(deviceAddress, serviceUuid, characteristicFoundCb);
}

void SubscribeCharacteristic(uint64_t deviceAddress, guid serviceUuid, guid characteristicUuid, SubscribeCallback subscribeCallback)
{
	SubscribeCharacteristicAsync(deviceAddress, serviceUuid, characteristicUuid, subscribeCallback);
}

void ReadBytes(uint64_t deviceAddress, guid serviceUuid, guid characteristicUuid, ReadBytesCallback readBufferCb)
{
	//TODO
}

void WriteBytes(uint64_t deviceAddress, guid serviceUuid, guid characteristicUuid, WriteBytesCallback writeBytesCb)
{
	//TODO
}


IAsyncOperation<BluetoothLEDevice> ConnectAsync(uint64_t deviceAddress)
{
	BluetoothLEDevice device = co_await RetrieveDevice(deviceAddress);

	co_return device;
}

fire_and_forget ScanServicesAsync(uint64_t deviceAddress, ServicesFoundCallback servicesCb)
{
	BleServiceArray service_list;

	try
	{
		// Connect to device if not already connected
		auto device = co_await RetrieveDevice(deviceAddress);
		if (device == nullptr)
		{
			//wprintf(L"Failed to retrieve device at address: %llu\n", deviceAddress);
			if (servicesCb)
				(*servicesCb)(&service_list);
			co_return;
		}

		// Try using BluetoothCacheMode::Cached to see if it improves results
		GattDeviceServicesResult result = co_await device.GetGattServicesAsync(BluetoothCacheMode::Uncached);

		if (result.Status() == GattCommunicationStatus::Unreachable)
			result = co_await device.GetGattServicesAsync(BluetoothCacheMode::Cached);

		if (result.Status() == GattCommunicationStatus::Success)
		{
			auto services = result.Services();
			service_list.count = services.Size();

			if (service_list.count == 0)
			{
				wprintf(L"No services found for device at address: %llu\n", deviceAddress);
			}
			else
			{
				service_list.services = new BleService[service_list.count];
				int i = 0;

				for (auto&& service : services)
				{
					BleService service_carrier;

					service_carrier.serviceUuid = service.Uuid();

					service_list.services[i++] = service_carrier;

					{
						std::lock_guard lock(quitLock);
						if (quitFlag)
							break;
					}
				}
			}
		}
	}
	catch (hresult_error& ex)
	{
		wprintf(L"%s:%d ScanServicesAsync catch: %s\n", __WFILE__, __LINE__, ex.message().c_str());
	}

	// Call the callback with the service list, even if it's empty
	if (servicesCb)
		(*servicesCb)(&service_list);
}


fire_and_forget ScanCharacteristicsAsync(uint64_t deviceAddress, guid serviceUuid, CharacteristicsFoundCallback characteristicsCb)
{
	BleCharacteristicArray char_list;

	try
	{
		auto service = co_await RetrieveService(deviceAddress, serviceUuid);
		if (service == nullptr)
		{
			if (characteristicsCb)
				(*characteristicsCb)(&char_list);
			co_return;
		}
		
		GattCharacteristicsResult charScan = co_await service.GetCharacteristicsAsync(BluetoothCacheMode::Uncached);

		if (charScan.Status() != GattCommunicationStatus::Success)
		{
			LogError(L"%s:%d Error scanning characteristics from service %s width status %d\n", __WFILE__, __LINE__, serviceUuid, (int)charScan.Status());

			if (characteristicsCb)
				(*characteristicsCb)(&char_list);
			co_return;
		}

		auto characteristics = charScan.Characteristics();

		char_list.count = characteristics.Size();
		char_list.characteristics = new BleCharacteristic[char_list.count];

		int i = 0;
		
		for (auto c : characteristics)
		{
			BleCharacteristic char_carrier;

			char_carrier.characteristicUuid = c.Uuid();

			// retrieve user description
			GattDescriptorsResult descriptorScan = co_await c.GetDescriptorsForUuidAsync(make_guid(L"00002901-0000-1000-8000-00805F9B34FB"), BluetoothCacheMode::Uncached);

			if (descriptorScan.Descriptors().Size() == 0)
			{
				const wchar_t* defaultDescription = L"no description available";
				wcscpy_s(char_carrier.userDescription, sizeof(char_carrier.userDescription) / sizeof(wchar_t), defaultDescription);
			}
			else
			{
				//get first descriptor
				GattDescriptor descriptor = descriptorScan.Descriptors().GetAt(0);

				//read name descriptor
				auto nameResult = co_await descriptor.ReadValueAsync();
				if (nameResult.Status() != GattCommunicationStatus::Success)
				{
					LogError(L"%s:%d couldn't read user description for charasteristic %s, status %d", __WFILE__, __LINE__, to_hstring(c.Uuid()).c_str(), nameResult.Status());
					continue;
				}
				
				auto dataReader = DataReader::FromBuffer(nameResult.Value());
				auto output = dataReader.ReadString(dataReader.UnconsumedBufferLength());
				wcscpy_s(char_carrier.userDescription, sizeof(char_carrier.userDescription) / sizeof(wchar_t), output.c_str());
			}

			char_list.characteristics[i++] = char_carrier;

			{
				lock_guard lock(quitLock);
				if (quitFlag)
					break;
			}
		}
	}
	catch (hresult_error& ex)
	{
		LogError(L"%s:%d ScanCharacteristicsAsync catch: %s\n", __WFILE__, __LINE__, ex.message().c_str());
	}

	if (characteristicsCb)
		(*characteristicsCb)(&char_list);
}

fire_and_forget SubscribeCharacteristicAsync(uint64_t deviceAddress, guid serviceUuid, guid characteristicUuid, SubscribeCallback subscribeCallback)
{
	try
	{
		auto characteristic = co_await RetrieveCharacteristic(deviceAddress, serviceUuid, characteristicUuid);
		if (characteristic != nullptr)
		{
			auto status = co_await characteristic.WriteClientCharacteristicConfigurationDescriptorAsync(GattClientCharacteristicConfigurationDescriptorValue::Notify);
			if (status != GattCommunicationStatus::Success)
			{
				LogError(L"%s:%d Error subscribing to characteristic with uuid %s and status %d", __WFILE__, __LINE__, characteristicUuid, status);
			}
			else
			{
				Subscription* subscription = new Subscription();
				subscription->characteristic = characteristic;
//				subscription->revoker = characteristic.ValueChanged(auto_revoke, &Characteristic_ValueChanged);
				subscriptions.push_back(subscription);

				if (subscribeCallback)
					(*subscribeCallback)();
			}
		}
	}
	catch (hresult_error& ex)
	{
		LogError(L"%s:%d SubscribeCharacteristicAsync catch: %s", __WFILE__, __LINE__, ex.message().c_str());
	}
}

/*
fire_and_forget SendDataAsync(BleData data, condition_variable* signal, bool* result)
{
	try
	{
		auto characteristic = co_await RetrieveCharacteristic(data.id, data.serviceUuid, data.characteristicUuid);
		if (characteristic != nullptr)
		{
			// create IBuffer from data
			DataWriter writer;
			writer.WriteBytes(array_view<uint8_t const>(data.buf, data.buf + data.size));
			IBuffer buffer = writer.DetachBuffer();
			auto status = co_await characteristic.WriteValueAsync(buffer, GattWriteOption::WriteWithoutResponse);

			if (status != GattCommunicationStatus::Success)
				LogError(L"%s:%d Error writing value to characteristic with uuid %s", __WFILE__, __LINE__, data.characteristicUuid);
			else if (result != 0)
				*result = true;
		}
	}
	catch (hresult_error& ex)
	{
		LogError(L"%s:%d SendDataAsync catch: %s\n", __WFILE__, __LINE__, ex.message().c_str());
	}

	if (signal != 0)
		signal->notify_one();
}
*/


void Characteristic_ValueChanged(GattCharacteristic const& characteristic, GattValueChangedEventArgs args)
{
	BleData data;

	//wcscpy_s(data.characteristicUuid, sizeof(data.characteristicUuid) / sizeof(wchar_t), to_hstring(characteristic.Uuid()).c_str());
	//wcscpy_s(data.serviceUuid, sizeof(data.serviceUuid) / sizeof(wchar_t), to_hstring(characteristic.Service().Uuid()).c_str());

	data.size = args.CharacteristicValue().Length();

	// IBuffer to array, copied from https://stackoverflow.com/a/55974934
	memcpy(data.buf, args.CharacteristicValue().data(), data.size);

	{
		lock_guard lock(quitLock);
		if (quitFlag)
			return;
	}

	//TODO: fire callback for data
}


void Quit()
{
	{
		lock_guard lock(quitLock);
		quitFlag = true;
	}

	StopDeviceScan();
	
	{
		for (auto subscription : subscriptions)
			subscription->revoker.revoke();

		subscriptions = {};
	}
	

	ClearCache();
}