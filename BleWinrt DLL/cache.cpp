#include "stdafx.h"
#include "carriers.h"
#include "cache.h"
#include "serialization.h"
#include "logging.h"
#include "ble-winrt.h"

#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>

#define __WFILE__ L"cache.cpp"


// implement own caching instead of using the system-provicded cache as there is an AccessDenied error when trying to
// call GetCharacteristicsAsync on a service for which a reference is hold in global scope
// cf. https://stackoverflow.com/a/36106137
map<uint64_t, DeviceCacheEntry> cache;


IAsyncOperation<BluetoothLEDevice> RetrieveDevice(uint64_t deviceAddress)
{
	auto item = cache.find(deviceAddress);
	if (item != cache.end())
		co_return item->second.device;

	try
	{
		BluetoothLEDevice device = co_await BluetoothLEDevice::FromBluetoothAddressAsync(deviceAddress);
		if (device == nullptr)
			co_return nullptr;

		//store in cache
		cache[deviceAddress] = { device };

		// Wait for the connection to stabilize
		//co_await winrt::resume_after(std::chrono::milliseconds(100));

		co_return device;
	}
	catch (hresult_error const&)
	{
		co_return nullptr;
	}
}

IAsyncOperation<GattDeviceService> RetrieveService(uint64_t deviceAddress, guid serviceUuid)
{
	//connect to device if not already connected
	auto device = co_await RetrieveDevice(deviceAddress);
	if (device == nullptr)
		co_return nullptr;

	//pull service if present
	if (cache[deviceAddress].services.count(serviceUuid))
		co_return cache[deviceAddress].services[serviceUuid].service;

	//get specific service from device
	GattDeviceServicesResult result = co_await device.GetGattServicesForUuidAsync(serviceUuid, BluetoothCacheMode::Cached);

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
	cache[deviceAddress].services[serviceUuid] = { result.Services().GetAt(0) };
	co_return cache[deviceAddress].services[serviceUuid].service;
}

IAsyncOperation<GattCharacteristic> RetrieveCharacteristic(uint64_t deviceAddress, guid serviceUuid, guid characteristicUuid)
{
	auto service = co_await RetrieveService(deviceAddress, serviceUuid);
	if (service == nullptr)
		co_return nullptr;

	//pull characteristic if present
	if (cache[deviceAddress].services[serviceUuid].characteristics.count(characteristicUuid))
		co_return cache[deviceAddress].services[serviceUuid].characteristics[characteristicUuid].characteristic;

	//get specific characteristic from device
	GattCharacteristicsResult result = co_await service.GetCharacteristicsForUuidAsync(characteristicUuid, BluetoothCacheMode::Cached);

	if (result.Status() != GattCommunicationStatus::Success)
	{
		LogError(L"%s:%d Error scanning characteristics from service %s with status %d", __WFILE__, __LINE__, serviceUuid, result.Status());
		co_return nullptr;
	}

	if (result.Characteristics().Size() == 0)
	{
		LogError(L"%s:%d No characteristic found with uuid %s", __WFILE__, __LINE__, characteristicUuid);
		co_return nullptr;
	}

	//add to cache
	cache[deviceAddress].services[serviceUuid].characteristics[characteristicUuid] = { result.Characteristics().GetAt(0) };
	co_return cache[deviceAddress].services[serviceUuid].characteristics[characteristicUuid].characteristic;
}

void RemoveFromCache(uint64_t deviceAddress)
{
	const auto devP = cache.find(deviceAddress);
	if (devP == cache.end())
		return;

	// Copying device and services as in Quit function. 
	auto dev = devP->second;
	if (dev.device != nullptr)
		dev.device.Close();

	for (auto service : dev.services)
		service.second.service.Close();

	cache.erase(deviceAddress);
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