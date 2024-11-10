#include "stdafx.h"
#include "carriers.h"
#include "ble-winrt.h"
#include "serialization.h"
#include "logging.h"
#include "cache.h"

#pragma comment(lib, "windowsapp")

// macro for file, see also https://stackoverflow.com/a/14421702
#define __WFILE__ L"ble-winrt.cpp"

using namespace std;
using namespace winrt;

using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;

using namespace Windows::Devices::Bluetooth;
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;
using namespace Windows::Devices::Enumeration;

using namespace Windows::Storage::Streams;

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
mutex subscribeQueueLock;
condition_variable subscribeQueueSignal;


void Connect(wchar_t* id, ConnectedCallback connectedCb)
{
	auto device = ConnectAsync(id);

	if (connectedCb)
	{
		if (device == nullptr)
			(*connectedCb)(nullptr);
		else
			(*connectedCb)(id);
	}
}

void Disconnect(wchar_t* id, ConnectedCallback connectedCb)
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

void ScanServices(wchar_t* id, ServiceFoundCallback serviceFoundCb)
{
	ScanServicesAsync(id, serviceFoundCb);
}

void ScanCharacteristics(wchar_t* id, wchar_t* serviceUuid, CharacteristicFoundCallback characteristicFoundCb)
{
	ScanCharacteristicsAsync(id, serviceUuid, characteristicFoundCb);
}

void SubscribeCharacteristic(wchar_t* id, wchar_t* serviceUuid, wchar_t* characteristicUuid, SubscribeCallback subscribeCallback)
{
}

void ReadBytes(wchar_t* id, wchar_t* serviceUuid, wchar_t* characteristicUuid, ReadBytesCallback readBufferCb)
{
}

void WriteBytes(wchar_t* id, wchar_t* serviceUuid, wchar_t* characteristicUuid, WriteBytesCallback writeBytesCb)
{
}


IAsyncOperation<BluetoothLEDevice> ConnectAsync(wchar_t* deviceId)
{
	auto device = RetrieveDevice(deviceId);
	if (device != nullptr)
		co_return device;

	// !!!! BluetoothLEDevice.FromIdAsync may prompt for consent, in this case bluetooth will fail in unity!
	BluetoothLEDevice result = co_await BluetoothLEDevice::FromIdAsync(deviceId);
	if (result == nullptr)
		co_return nullptr;
	
	//add to cache
	StoreDevice(deviceId, result);

	co_return device;
}

fire_and_forget ScanServicesAsync(wchar_t* id, ServiceFoundCallback serviceFoundCb)
{
	try
	{
		//connect to device if not already connected
		auto device = co_await ConnectAsync(id);
		if (device == nullptr)
			co_return;

		GattDeviceServicesResult result = co_await device.GetGattServicesAsync(BluetoothCacheMode::Uncached);

		if (result.Status() != GattCommunicationStatus::Success)
		{
			LogError(L"%s:%d Failed retrieving services.", __WFILE__, __LINE__);
			co_return;
		}

		IVectorView<GattDeviceService> services = result.Services();
		for (auto&& service : services)
		{
			BleService service_carrier;

			wcscpy_s(service_carrier.serviceUuid, sizeof(service_carrier.serviceUuid) / sizeof(wchar_t), to_hstring(service.Uuid()).c_str());

			if (serviceFoundCb)
				(*serviceFoundCb)(&service_carrier);

			{
				lock_guard lock(quitLock);
				if (quitFlag)
					break;
			}
		}
	}
	catch (hresult_error& ex)
	{
		LogError(L"%s:%d ScanServicesAsync catch: %s", __WFILE__, __LINE__, ex.message().c_str());
	}
}

fire_and_forget ScanCharacteristicsAsync(wchar_t* id, wchar_t* serviceUuid, CharacteristicFoundCallback characteristicFoundCb)
{
	try
	{
		auto service = co_await RetrieveService(id, serviceUuid);
		if (service == nullptr)
			co_return;
		
		GattCharacteristicsResult charScan = co_await service.GetCharacteristicsAsync(BluetoothCacheMode::Uncached);

		if (charScan.Status() != GattCommunicationStatus::Success)
		{
			LogError(L"%s:%d Error scanning characteristics from service %s width status %d", __WFILE__, __LINE__, serviceUuid, (int)charScan.Status());
			co_return;
		}
		
		for (auto c : charScan.Characteristics())
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

			if (characteristicFoundCb)
				(*characteristicFoundCb)(&char_carrier);

			{
				lock_guard lock(quitLock);
				if (quitFlag)
					break;
			}
		}
	}
	catch (hresult_error& ex)
	{
		LogError(L"%s:%d ScanCharacteristicsAsync catch: %s", __WFILE__, __LINE__, ex.message().c_str());
	}
}

bool QuittableWait(condition_variable& signal, unique_lock<mutex>& waitLock)
{
	{
		lock_guard quit_lock(quitLock);
		if (quitFlag)
			return true;
	}

	signal.wait(waitLock);
	lock_guard quit_lock(quitLock);
	return quitFlag;
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

fire_and_forget SubscribeCharacteristicAsync(wchar_t* deviceId, wchar_t* serviceId, wchar_t* characteristicId, bool* result)
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
				Subscription *subscription = new Subscription();
				subscription->characteristic = characteristic;
				subscription->revoker = characteristic.ValueChanged(auto_revoke, &Characteristic_ValueChanged);
				subscriptions.push_back(subscription);

				if (result != 0)
					*result = true;
			}
		}
	}
	catch (hresult_error& ex)
	{
		LogError(L"%s:%d SubscribeCharacteristicAsync catch: %s", __WFILE__, __LINE__, ex.message().c_str());
	}

	subscribeQueueSignal.notify_one();
}

bool SubscribeCharacteristic(wchar_t* deviceId, wchar_t* serviceId, wchar_t* characteristicId, bool block)
{
	unique_lock<mutex> lock(subscribeQueueLock);
	bool result = false;
	SubscribeCharacteristicAsync(deviceId, serviceId, characteristicId, block ? &result : 0);

	if (block && QuittableWait(subscribeQueueSignal, lock))
		return false;

	return result;
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
			writer.WriteBytes(array_view<uint8_t const> (data.buf, data.buf + data.size));
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
		LogError(L"%s:%d SendDataAsync catch: %s", __WFILE__, __LINE__, ex.message().c_str());
	}

	if (signal != 0)
		signal->notify_one();
}

bool SendData(BleData* data, bool block)
{
	mutex _mutex;
	unique_lock<mutex> lock(_mutex);
	condition_variable signal;
	bool result = false;

	// copy data to stack so that caller can free its memory in non-blocking mode
	SendDataAsync(*data, block ? &signal : 0, block ? &result : 0);

	if (block)
		signal.wait(lock);

	return result;
}

void Quit()
{
	{
		lock_guard lock(quitLock);
		quitFlag = true;
	}

	StopDeviceScan();
	
	subscribeQueueSignal.notify_one();
	{
		lock_guard lock(subscribeQueueLock);

		for (auto subscription : subscriptions)
			subscription->revoker.revoke();

		subscriptions = {};
	}
	

	ClearCache();
}