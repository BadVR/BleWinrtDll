#include "stdafx.h"
#include "carriers.h"
#include "cache.h"
#include "serialization.h"
#include "logging.h"
#include "ble-winrt.h"

#define __WFILE__ L"cache.cpp"


// implement own caching instead of using the system-provicded cache as there is an AccessDenied error when trying to
// call GetCharacteristicsAsync on a service for which a reference is hold in global scope
// cf. https://stackoverflow.com/a/36106137
map<long, DeviceCacheEntry> cache;


// using hashes of uuids to omit storing the c-strings in reliable storage
static long hsh(wchar_t* wstr)
{
	long seed = 5381;
	int c;

	while (c = *wstr++)
		seed = ((seed << 5) + seed) + c;

	return seed;
}

IAsyncOperation<BluetoothLEDevice> RetrieveDevice(wchar_t* deviceId)
{
	auto deviceHash = hsh(deviceId);

	auto item = cache.find(deviceHash);
	if (item != cache.end())
		co_return item->second.device;

	BluetoothLEDevice device = co_await BluetoothLEDevice::FromIdAsync(deviceId);
	if (device == nullptr)
	{
		cout << "unable to connect\n";
		co_return nullptr;
	}

	//store in cache
	cache[deviceHash] = { device };

	co_return device;
}

IAsyncOperation<GattDeviceService> RetrieveService(wchar_t* id, wchar_t* serviceUuid)
{
	auto deviceHash = hsh(id);
	auto serviceHash = hsh(serviceUuid);

	//connect to device if not already connected
	auto device = co_await RetrieveDevice(id);
	if (device == nullptr)
		co_return nullptr;

	//pull service if present
	if (cache[hsh(id)].services.count(hsh(serviceUuid)))
		co_return cache[hsh(id)].services[hsh(serviceUuid)].service;

	//get specific service from device
	GattDeviceServicesResult result = co_await device.GetGattServicesForUuidAsync(make_guid(serviceUuid), BluetoothCacheMode::Cached);

	if (result.Status() != GattCommunicationStatus::Success)
	{
		LogError(L"%s:%d Failed retrieving services.", __WFILE__, __LINE__);
		co_return nullptr;
	}

	if (result.Services().Size() == 0)
	{
		LogError(L"%s:%d No service found with uuid ", __WFILE__, __LINE__);
		co_return nullptr;
	}

	//add to cache
	cache[hsh(id)].services[hsh(serviceUuid)] = { result.Services().GetAt(0) };
	co_return cache[hsh(id)].services[hsh(serviceUuid)].service;
}

IAsyncOperation<GattCharacteristic> RetrieveCharacteristic(wchar_t* deviceId, wchar_t* serviceId, wchar_t* characteristicId)
{
	auto service = co_await RetrieveService(deviceId, serviceId);
	if (service == nullptr)
		co_return nullptr;

	//pull characteristic if present
	if (cache[hsh(deviceId)].services[hsh(serviceId)].characteristics.count(hsh(characteristicId)))
		co_return cache[hsh(deviceId)].services[hsh(serviceId)].characteristics[hsh(characteristicId)].characteristic;

	//get specific characteristic from device
	GattCharacteristicsResult result = co_await service.GetCharacteristicsForUuidAsync(make_guid(characteristicId), BluetoothCacheMode::Cached);

	if (result.Status() != GattCommunicationStatus::Success)
	{
		LogError(L"%s:%d Error scanning characteristics from service %s with status %d", __WFILE__, __LINE__, serviceId, result.Status());
		co_return nullptr;
	}

	if (result.Characteristics().Size() == 0)
	{
		LogError(L"%s:%d No characteristic found with uuid %s", __WFILE__, __LINE__, characteristicId);
		co_return nullptr;
	}

	//add to cache
	cache[hsh(deviceId)].services[hsh(serviceId)].characteristics[hsh(characteristicId)] = { result.Characteristics().GetAt(0) };
	co_return cache[hsh(deviceId)].services[hsh(serviceId)].characteristics[hsh(characteristicId)].characteristic;
}

void RemoveFromCache(wchar_t* deviceId)
{
	auto deviceHash = hsh(deviceId);

	const auto devP = cache.find(deviceHash);
	if (devP == cache.end())
		return;

	// Copying device and services as in Quit function. 
	auto dev = devP->second;
	if (dev.device != nullptr)
		dev.device.Close();

	for (auto service : dev.services)
		service.second.service.Close();

	cache.erase(deviceHash);
}

void ClearCache()
{
	for (auto device : cache)
	{
		device.second.device.Close();

		for (auto service : device.second.services)
			service.second.service.Close();
	}

	cache.clear();
}