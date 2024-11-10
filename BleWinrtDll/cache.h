#pragma once

using namespace std;
using namespace winrt;

using namespace Windows::Foundation;

using namespace Windows::Devices::Bluetooth;
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;
using namespace Windows::Devices::Enumeration;

using namespace Windows::Storage::Streams;


struct CharacteristicCacheEntry
{
	GattCharacteristic characteristic = nullptr;
};

struct ServiceCacheEntry
{
	GattDeviceService service = nullptr;
	map<long, CharacteristicCacheEntry> characteristics = { };
};

struct DeviceCacheEntry
{
	BluetoothLEDevice device = nullptr;
	map<long, ServiceCacheEntry> services = { };
};

BluetoothLEDevice RetrieveDevice(wchar_t* deviceId);
void StoreDevice(wchar_t* deviceId, BluetoothLEDevice device);
IAsyncOperation<GattDeviceService> RetrieveService(wchar_t* id, wchar_t* serviceUuid);
IAsyncOperation<GattCharacteristic> RetrieveCharacteristic(wchar_t* deviceId, wchar_t* serviceId, wchar_t* characteristicId);
void RemoveFromCache(wchar_t* deviceId);
void ClearCache();
