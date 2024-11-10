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
	map<guid, CharacteristicCacheEntry> characteristics = { };
};

struct DeviceCacheEntry
{
	BluetoothLEDevice device = nullptr;
	map<guid, ServiceCacheEntry> services = { };
};


IAsyncOperation<BluetoothLEDevice> RetrieveDevice(uint64_t id);
IAsyncOperation<GattDeviceService> RetrieveService(uint64_t id, guid serviceUuid);
IAsyncOperation<GattCharacteristic> RetrieveCharacteristic(uint64_t deviceAddress, guid serviceUuid, guid characteristicUuid);

void RemoveFromCache(uint64_t id);
void ClearCache();
