#pragma once

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Devices::Bluetooth;


//callback definitions
using DeviceInfoCallback = void(DeviceInfo*);
using DeviceUpdateCallback = void(DeviceInfoUpdate*);
using CompletedCallback = void();
using StoppedCallback = void();

using ConnectedCallback = void(wchar_t*);
using DisconnectedCallback = void(wchar_t*);
using ServiceFoundCallback = void(BleService*);
using CharacteristicFoundCallback = void(BleCharacteristic*);

using SubscribeCallback = void();
using ReadBytesCallback = void();
using WriteBytesCallback = void();


IAsyncOperation<BluetoothLEDevice> ConnectAsync(wchar_t* id);
fire_and_forget ScanServicesAsync(wchar_t* id, ServiceFoundCallback serviceFoundCb);
fire_and_forget ScanCharacteristicsAsync(wchar_t* id, wchar_t* serviceUuid, CharacteristicFoundCallback characteristicFoundCb);


//these functions will be available through the native DLL interface, exposed to Unity
extern "C"
{
	__declspec(dllexport) void StartDeviceScan(DeviceInfoCallback addedCb, DeviceUpdateCallback updatedCb, DeviceUpdateCallback removedCb, CompletedCallback completedCb, StoppedCallback stoppedCb);
	__declspec(dllexport) void StopDeviceScan();

	__declspec(dllexport) void Connect(wchar_t* id, ConnectedCallback connectedCb);
	__declspec(dllexport) void Disconnect(wchar_t* id, ConnectedCallback connectedCb);

	__declspec(dllexport) void ScanServices(wchar_t* id, ServiceFoundCallback serviceFoundCb);
	__declspec(dllexport) void ScanCharacteristics(wchar_t* id, wchar_t* serviceUuid, CharacteristicFoundCallback characteristicFoundCb);
	__declspec(dllexport) void SubscribeCharacteristic(wchar_t* id, wchar_t* serviceUuid, wchar_t* characteristicUuid, SubscribeCallback subscribeCallback);

	__declspec(dllexport) void ReadBytes(wchar_t* id, wchar_t* serviceUuid, wchar_t* characteristicUuid, ReadBytesCallback readBufferCb);
	__declspec(dllexport) void WriteBytes(wchar_t* id, wchar_t* serviceUuid, wchar_t* characteristicUuid, WriteBytesCallback writeBytesCb);

	__declspec(dllexport) void Quit();
}
