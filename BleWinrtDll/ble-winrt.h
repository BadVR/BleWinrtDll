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

using DeviceInfoCallback = void(DeviceInfo*);
using DeviceUpdateCallback = void(DeviceInfoUpdate*);
using CompletedCallback = void();
using StoppedCallback = void();

using ConnectedCallback = void(wchar_t*);
using DisconnectedCallback = void(wchar_t*);
using ServicesFoundCallback = void(BleServiceArray *);
using CharacteristicsFoundCallback = void(BleCharacteristicArray *);

using SubscribeCallback = void();
using ReadBytesCallback = void();
using WriteBytesCallback = void();


IAsyncOperation<BluetoothLEDevice> ConnectAsync(wchar_t* id);
fire_and_forget ScanServicesAsync(wchar_t* id, ServicesFoundCallback servicesCb);
fire_and_forget ScanCharacteristicsAsync(wchar_t* id, wchar_t* serviceUuid, CharacteristicsFoundCallback characteristicsCb);
fire_and_forget SubscribeCharacteristicAsync(wchar_t* deviceId, wchar_t* serviceId, wchar_t* characteristicId, SubscribeCallback subscribeCallback);


//these functions will be available through the native DLL interface, exposed to Unity
extern "C"
{
	__declspec(dllexport) void StartDeviceScan(DeviceInfoCallback addedCb, DeviceUpdateCallback updatedCb, DeviceUpdateCallback removedCb, CompletedCallback completedCb, StoppedCallback stoppedCb);
	__declspec(dllexport) void StopDeviceScan();

	__declspec(dllexport) void ConnectDevice(wchar_t* id, ConnectedCallback connectedCb);
	__declspec(dllexport) void DisconnectDevice(wchar_t* id, ConnectedCallback connectedCb);

	__declspec(dllexport) void ScanServices(wchar_t* id, ServicesFoundCallback serviceFoundCb);
	__declspec(dllexport) void ScanCharacteristics(wchar_t* id, wchar_t* serviceUuid, CharacteristicsFoundCallback characteristicFoundCb);

	__declspec(dllexport) void SubscribeCharacteristic(wchar_t* id, wchar_t* serviceUuid, wchar_t* characteristicUuid, SubscribeCallback subscribeCallback);

	__declspec(dllexport) void ReadBytes(wchar_t* id, wchar_t* serviceUuid, wchar_t* characteristicUuid, ReadBytesCallback readBufferCb);
	__declspec(dllexport) void WriteBytes(wchar_t* id, wchar_t* serviceUuid, wchar_t* characteristicUuid, WriteBytesCallback writeBytesCb);

	__declspec(dllexport) void Quit();
}
