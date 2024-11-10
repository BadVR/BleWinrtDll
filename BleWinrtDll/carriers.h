#pragma once

using namespace winrt::Windows::Devices::Bluetooth::GenericAttributeProfile;

const int ID_SIZE = 128;
const int UUID_SIZE = 100;

struct DeviceInfo
{
	wchar_t id[ID_SIZE];
	wchar_t name[ID_SIZE];
	uint64_t mac = 0;

	int32_t signalStrength = 0;
	bool signalStrengthPresent = false;

	bool isConnected = false;
	bool isConnectedPresent = false;

	bool isConnectable = false;
	bool isConnectablePresent = false;
};

struct DeviceInfoUpdate
{
	wchar_t id[ID_SIZE];
	wchar_t name[ID_SIZE];
	bool namePresent = false;

	int32_t signalStrength;
	bool signalStrengthPresent = false;

	bool isConnected = false;
	bool isConnectedPresent = false;

	bool isConnectable = false;
	bool isConnectablePresent = false;
};

struct BleService
{
	wchar_t serviceUuid[UUID_SIZE];
};

struct BleCharacteristic
{
	wchar_t characteristicUuid[UUID_SIZE];
	wchar_t userDescription[ID_SIZE];
};

struct BleData
{
	uint8_t buf[512];
	uint16_t size;

	wchar_t id[ID_SIZE];

	wchar_t serviceUuid[UUID_SIZE];
	wchar_t characteristicUuid[UUID_SIZE];
};

struct Subscription
{
	GattCharacteristic characteristic = nullptr;
	GattCharacteristic::ValueChanged_revoker revoker;
};