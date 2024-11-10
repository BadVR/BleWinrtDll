#include "stdafx.h"
#include "carriers.h"
#include "ble-winrt.h"
#include "serialization.h"
#include "logging.h"
#include "cache.h"

#define __WFILE__ L"ble-winrt.cpp"

using namespace Windows::Foundation::Collections;


DeviceInfoCallback* addedCallback = nullptr;
DeviceUpdateCallback* updatedCallback = nullptr;
DeviceUpdateCallback* removedCallback = nullptr;

CompletedCallback* completedCallback = nullptr;
StoppedCallback* stoppedCallback = nullptr;

DeviceWatcher deviceWatcher{ nullptr };
DeviceWatcher::Added_revoker deviceWatcherAddedRevoker;
DeviceWatcher::Removed_revoker deviceWatcherRemovedRevoker;
DeviceWatcher::Updated_revoker deviceWatcherUpdatedRevoker;
DeviceWatcher::EnumerationCompleted_revoker deviceWatcherEnumerationCompletedRevoker;
DeviceWatcher::Stopped_revoker deviceWatcherStoppedRevoker;

// global flag to release calling thread
mutex quitLock;
bool quitFlag = false;

list<Subscription*> subscriptions;


void DisconnectDevice(wchar_t* id, ConnectedCallback connectedCb)
{
	try
	{
		RemoveFromCache(id);

		if (connectedCb)
			(*connectedCb)(id);
	}
	catch (const std::exception& e)
	{
		Log(e.what());

		if (connectedCb)
			(*connectedCb)(nullptr);
	}
}

void ScanServices(wchar_t* id, ServicesFoundCallback serviceFoundCb)
{
	ScanServicesAsync(id, serviceFoundCb);
}

void ScanCharacteristics(wchar_t* id, wchar_t* serviceUuid, CharacteristicsFoundCallback characteristicFoundCb)
{
	ScanCharacteristicsAsync(id, serviceUuid, characteristicFoundCb);
}

void SubscribeCharacteristic(wchar_t* id, wchar_t* serviceUuid, wchar_t* characteristicUuid, SubscribeCallback subscribeCallback)
{
	SubscribeCharacteristicAsync(id, serviceUuid, characteristicUuid, subscribeCallback);
}

void ReadBytes(wchar_t* id, wchar_t* serviceUuid, wchar_t* characteristicUuid, ReadBytesCallback readBufferCb)
{
}

void WriteBytes(wchar_t* id, wchar_t* serviceUuid, wchar_t* characteristicUuid, WriteBytesCallback writeBytesCb)
{
}


IAsyncOperation<BluetoothLEDevice> ConnectAsync(wchar_t* deviceId)
{
	BluetoothLEDevice device = co_await RetrieveDevice(deviceId);

	co_return device;
}

fire_and_forget ScanServicesAsync(wchar_t* id, ServicesFoundCallback servicesCb)
{
	BleServiceArray service_list;

    try
    {
        // Connect to device if not already connected
        auto device = co_await RetrieveDevice(id);
		if (device == nullptr)
		{
			if (servicesCb)
				(*servicesCb)(&service_list);
			co_return;
		}

		GattDeviceServicesResult result = co_await device.GetGattServicesAsync(BluetoothCacheMode::Uncached);
		if (result.Status() == GattCommunicationStatus::Success)
		{
			IVectorView<GattDeviceService> services = result.Services();

			service_list.count = services.Size();
			service_list.services = new BleService[service_list.count];

			int i = 0;

			for (auto&& service : services)
			{
				BleService service_carrier;

				//wprintf(L"%s", service_carrier.serviceUuid);

				wcscpy_s(service_carrier.serviceUuid, sizeof(service_carrier.serviceUuid) / sizeof(wchar_t), to_hstring(service.Uuid()).c_str());

				{
					lock_guard lock(quitLock);
					if (quitFlag)
						break;
				}

				service_list.services[i++] = service_carrier;
			}
		}
    }
    catch (hresult_error& ex)
    {
        wprintf(L"%s:%d ScanServicesAsync catch: %s\n", __WFILE__, __LINE__, ex.message().c_str());
    }

	if (servicesCb)
		(*servicesCb)(&service_list);
}

fire_and_forget ScanCharacteristicsAsync(wchar_t* id, wchar_t* serviceUuid, CharacteristicsFoundCallback characteristicsCb)
{
	BleCharacteristicArray char_list;

	try
	{
		auto service = co_await RetrieveService(id, serviceUuid);
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

			wcscpy_s(char_carrier.characteristicUuid, sizeof(char_carrier.characteristicUuid) / sizeof(wchar_t), to_hstring(c.Uuid()).c_str());

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

fire_and_forget SubscribeCharacteristicAsync(wchar_t* deviceId, wchar_t* serviceId, wchar_t* characteristicId, SubscribeCallback subscribeCallback)
{
	try
	{
		auto characteristic = co_await RetrieveCharacteristic(deviceId, serviceId, characteristicId);
		if (characteristic != nullptr)
		{
			auto status = co_await characteristic.WriteClientCharacteristicConfigurationDescriptorAsync(GattClientCharacteristicConfigurationDescriptorValue::Notify);
			if (status != GattCommunicationStatus::Success)
			{
				LogError(L"%s:%d Error subscribing to characteristic with uuid %s and status %d", __WFILE__, __LINE__, characteristicId, status);
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


void DeviceWatcher_Added(DeviceWatcher sender, DeviceInformation deviceInfo)
{
	DeviceInfo di;

	wcscpy_s(di.id, sizeof(di.id) / sizeof(wchar_t), deviceInfo.Id().c_str());
	wcscpy_s(di.name, sizeof(di.name) / sizeof(wchar_t), deviceInfo.Name().c_str());
	
	if (deviceInfo.Properties().HasKey(L"System.Devices.Aep.DeviceAddress"))
	{
		hstring mac_string = unbox_value<hstring>(deviceInfo.Properties().Lookup(L"System.Devices.Aep.DeviceAddress")).c_str();
		di.mac = ConvertMacAddressToULong(mac_string);
	}
	if (deviceInfo.Properties().HasKey(L"System.Devices.Aep.Bluetooth.Le.IsConnectable"))
	{
		di.isConnectable = unbox_value<bool>(deviceInfo.Properties().Lookup(L"System.Devices.Aep.Bluetooth.Le.IsConnectable"));
		di.isConnectablePresent = true;
	}
	if (deviceInfo.Properties().HasKey(L"System.Devices.Aep.Bluetooth.Le.IsConnected"))
	{
		di.isConnected = unbox_value<bool>(deviceInfo.Properties().Lookup(L"System.Devices.Aep.Bluetooth.Le.IsConnected"));
		di.isConnectedPresent = true;
	}
	if (deviceInfo.Properties().HasKey(L"System.Devices.Aep.SignalStrength"))
	{
		di.signalStrength = unbox_value<int32_t>(deviceInfo.Properties().Lookup(L"System.Devices.Aep.SignalStrength"));
		di.signalStrengthPresent = true;
	}

	//fire callback if present
	if (addedCallback)
		(*addedCallback)(&di);
}

void DeviceWatcher_Updated(DeviceWatcher sender, DeviceInformationUpdate deviceInfoUpdate)
{
	DeviceInfoUpdate diu;

	wcscpy_s(diu.id, sizeof(diu.id) / sizeof(wchar_t), deviceInfoUpdate.Id().c_str());

	if (deviceInfoUpdate.Properties().HasKey(L"System.Devices.Aep.Bluetooth.Le.IsConnectable"))
	{
		diu.isConnectable = unbox_value<bool>(deviceInfoUpdate.Properties().Lookup(L"System.Devices.Aep.Bluetooth.Le.IsConnectable"));
		diu.isConnectablePresent = true;
	}
	if (deviceInfoUpdate.Properties().HasKey(L"System.Devices.Aep.Bluetooth.Le.IsConnected"))
	{
		diu.isConnected = unbox_value<bool>(deviceInfoUpdate.Properties().Lookup(L"System.Devices.Aep.Bluetooth.Le.IsConnected"));
		diu.isConnectedPresent = true;
	}
	if (deviceInfoUpdate.Properties().HasKey(L"System.Devices.Aep.SignalStrength"))
	{
		diu.signalStrength = unbox_value<int32_t>(deviceInfoUpdate.Properties().Lookup(L"System.Devices.Aep.SignalStrength"));
		diu.signalStrengthPresent = true;
	}

	//fire callback if present
	if (updatedCallback)
		(*updatedCallback)(&diu);
}

void DeviceWatcher_Removed(DeviceWatcher sender, DeviceInformationUpdate deviceInfoUpdate)
{
	DeviceInfoUpdate diu;

	wcscpy_s(diu.id, sizeof(diu.id) / sizeof(wchar_t), deviceInfoUpdate.Id().c_str());

	//fire callback if present
	if (removedCallback)
		(*removedCallback)(&diu);
}

void DeviceWatcher_EnumerationCompleted(DeviceWatcher sender, IInspectable const&)
{
	StopDeviceScan();

	if (completedCallback)
		(*completedCallback)();
}

void DeviceWatcher_Stopped(DeviceWatcher sender, IInspectable const&)
{
	if (stoppedCallback)
		(*stoppedCallback)();
}

IVector<hstring> requestedProperties = single_threaded_vector<hstring>(
{
	L"System.Devices.Aep.DeviceAddress",
	L"System.Devices.Aep.Bluetooth.Le.IsConnectable",
	L"System.Devices.Aep.IsConnected",
	L"System.Devices.Aep.SignalStrength"
});

void StartDeviceScan(DeviceInfoCallback addedCb, DeviceUpdateCallback updatedCb, DeviceUpdateCallback removedCb, CompletedCallback completedCb, StoppedCallback stoppedCb)
{
	// as this is the first function that must be called, if Quit() was called before, assume here that the client wants to restart
	{
		lock_guard lock(quitLock);
		quitFlag = false;
	}

	addedCallback = addedCb;
	updatedCallback = updatedCb;
	removedCallback = removedCb;
	completedCallback = completedCb;
	stoppedCallback = stoppedCb;

	//list Bluetooth LE devices
	hstring aqsAllBluetoothLEDevices = L"(System.Devices.Aep.ProtocolId:=\"{bb7bb05e-5972-42b5-94fc-76eaa7084d49}\")";

	//create the device watcher
	deviceWatcher = DeviceInformation::CreateWatcher(
		aqsAllBluetoothLEDevices,
		requestedProperties,
		DeviceInformationKind::AssociationEndpoint);

	//add lsiteners
	// see https://docs.microsoft.com/en-us/windows/uwp/cpp-and-winrt-apis/handle-events#revoke-a-registered-delegate
	deviceWatcherAddedRevoker = deviceWatcher.Added(auto_revoke, &DeviceWatcher_Added);
	deviceWatcherUpdatedRevoker = deviceWatcher.Updated(auto_revoke, &DeviceWatcher_Updated);
	deviceWatcherRemovedRevoker = deviceWatcher.Removed(auto_revoke, &DeviceWatcher_Removed);

	deviceWatcherEnumerationCompletedRevoker = deviceWatcher.EnumerationCompleted(auto_revoke, &DeviceWatcher_EnumerationCompleted);
	deviceWatcherStoppedRevoker = deviceWatcher.Stopped(auto_revoke, &DeviceWatcher_Stopped);

	// ~30 seconds scan ; for permanent scanning use BluetoothLEAdvertisementWatcher, see the BluetoothAdvertisement.zip sample
	deviceWatcher.Start();
}

void StopDeviceScan()
{
	if (deviceWatcher == nullptr)
		return;

	deviceWatcherAddedRevoker.revoke();
	deviceWatcherUpdatedRevoker.revoke();
	deviceWatcherRemovedRevoker.revoke();

	deviceWatcherEnumerationCompletedRevoker.revoke();
	deviceWatcherStoppedRevoker.revoke();

	deviceWatcher.Stop();

	//reset
	deviceWatcher = nullptr;
}


void Characteristic_ValueChanged(GattCharacteristic const& characteristic, GattValueChangedEventArgs args)
{
	BleData data;

	wcscpy_s(data.characteristicUuid, sizeof(data.characteristicUuid) / sizeof(wchar_t), to_hstring(characteristic.Uuid()).c_str());
	wcscpy_s(data.serviceUuid, sizeof(data.serviceUuid) / sizeof(wchar_t), to_hstring(characteristic.Service().Uuid()).c_str());
	wcscpy_s(data.id, sizeof(data.id) / sizeof(wchar_t), characteristic.Service().Device().DeviceId().c_str());

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