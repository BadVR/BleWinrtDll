#pragma once

using namespace std;
using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Devices::Bluetooth;

struct BleServiceArray
{
	BleService* services;
	int count = 0;
};

struct BleCharacteristicArray
{
	BleCharacteristic* characteristics;
	int count = 0;
};

using ReceivedCallback = void(BleAdvert*);
using StoppedCallback = void();
using ConnectedCallback = void(uint64_t);
using DisconnectedCallback = void(uint64_t);
using ServicesFoundCallback = void(BleServiceArray *);
using CharacteristicsFoundCallback = void(BleCharacteristicArray *);

using SubscribeCallback = void(uint64_t deviceAddress, guid serviceUuid, guid characteristicUuid, const uint8_t* data, size_t size);
using ReadBytesCallback = void(const uint8_t* data, size_t size);
using WriteBytesCallback = void(bool success);


fire_and_forget ScanServicesAsync(uint64_t deviceAddress, ServicesFoundCallback servicesCb);
fire_and_forget ScanCharacteristicsAsync(uint64_t deviceAddress, guid serviceUuid, CharacteristicsFoundCallback characteristicsCb);
fire_and_forget SubscribeCharacteristicAsync(uint64_t deviceAddress, guid serviceUuid, guid characteristicUuid, SubscribeCallback subscribeCallback);

fire_and_forget ConnectDeviceAsync(uint64_t deviceAddress, ConnectedCallback connectedCb);

fire_and_forget ReadBytesAsync(uint64_t deviceAddress, guid serviceUuid, guid characteristicUuid, ReadBytesCallback readBufferCb);
fire_and_forget WriteBytesAsync(uint64_t deviceAddress, guid serviceUuid, guid characteristicUuid, const uint8_t* data, size_t size, WriteBytesCallback writeCallback);


//these functions will be available through the native DLL interface, exposed to Unity
extern "C"
{
	__declspec(dllexport) void InitializeScan(const wchar_t* nameFilter, guid serviceFilter, ReceivedCallback addedCb, StoppedCallback stoppedCb);
	__declspec(dllexport) void StartScan();
	__declspec(dllexport) void StopScan();

	__declspec(dllexport) void ConnectDevice(uint64_t deviceAddress, ConnectedCallback connectedCb);
	__declspec(dllexport) void DisconnectDevice(uint64_t deviceAddress, DisconnectedCallback connectedCb);

	__declspec(dllexport) void ScanServices(uint64_t deviceAddress, ServicesFoundCallback serviceFoundCb);
	__declspec(dllexport) void ScanCharacteristics(uint64_t deviceAddress, guid serviceUuid, CharacteristicsFoundCallback characteristicFoundCb);

	__declspec(dllexport) void SubscribeCharacteristic(uint64_t deviceAddress, guid serviceUuid, guid characteristicUuid, SubscribeCallback subscribeCallback);

	__declspec(dllexport) void ReadBytes(uint64_t deviceAddress, guid serviceUuid, guid characteristicUuid, ReadBytesCallback readBufferCb);
	__declspec(dllexport) void WriteBytes(uint64_t deviceAddress, guid serviceUuid, guid characteristicUuid, const uint8_t* data, size_t size, WriteBytesCallback writeBytesCb);

	__declspec(dllexport) void Quit();
}
