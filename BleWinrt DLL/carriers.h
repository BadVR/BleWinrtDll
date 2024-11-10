#pragma once

using namespace winrt;
using namespace winrt::Windows::Devices::Bluetooth::GenericAttributeProfile;

const int ID_SIZE = 128;
const int UUID_SIZE = 100;

struct BleAdvert
{
	uint64_t mac = 0;
	wchar_t name[ID_SIZE];

	int32_t signalStrength = 0;
	int16_t powerLevel = 0;
};

struct BleService
{
	guid serviceUuid;
};

struct BleCharacteristic
{
	guid characteristicUuid;
	wchar_t userDescription[ID_SIZE];
};

struct BleData
{
	uint8_t buf[512];
	uint16_t size;

	guid serviceUuid;
	guid characteristicUuid;
};

struct Subscription
{
	GattCharacteristic characteristic = nullptr;
	GattCharacteristic::ValueChanged_revoker revoker;
};