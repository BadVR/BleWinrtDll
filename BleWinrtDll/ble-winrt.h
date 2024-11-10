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
using DisconnectedCallback = void(uint64_t);
using ServicesFoundCallback = void(BleServiceArray *);
using CharacteristicsFoundCallback = void(BleCharacteristicArray *);

using SubscribeCallback = void();
using ReadBytesCallback = void();
using WriteBytesCallback = void();

fire_and_forget ScanServicesAsync(uint64_t deviceAddress, ServicesFoundCallback servicesCb);
fire_and_forget ScanCharacteristicsAsync(uint64_t deviceAddress, guid serviceUuid, CharacteristicsFoundCallback characteristicsCb);
fire_and_forget SubscribeCharacteristicAsync(uint64_t deviceAddress, guid serviceUuid, guid characteristicUuid, SubscribeCallback subscribeCallback);


//these functions will be available through the native DLL interface, exposed to Unity
extern "C"
{
	__declspec(dllexport) void StartDeviceScan(ReceivedCallback addedCb, StoppedCallback stoppedCb);
	__declspec(dllexport) void StopDeviceScan();

	__declspec(dllexport) void DisconnectDevice(uint64_t deviceAddress, DisconnectedCallback connectedCb);

	__declspec(dllexport) void ScanServices(uint64_t deviceAddress, ServicesFoundCallback serviceFoundCb);
	__declspec(dllexport) void ScanCharacteristics(uint64_t deviceAddress, guid serviceUuid, CharacteristicsFoundCallback characteristicFoundCb);

	__declspec(dllexport) void SubscribeCharacteristic(uint64_t deviceAddress, guid serviceUuid, guid characteristicUuid, SubscribeCallback subscribeCallback);

	__declspec(dllexport) void ReadBytes(uint64_t deviceAddress, guid serviceUuid, guid characteristicUuid, ReadBytesCallback readBufferCb);
	__declspec(dllexport) void WriteBytes(uint64_t deviceAddress, guid serviceUuid, guid characteristicUuid, WriteBytesCallback writeBytesCb);

	__declspec(dllexport) void Quit();
}
