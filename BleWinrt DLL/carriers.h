#pragma once

using namespace winrt;
using namespace winrt::Windows::Devices::Bluetooth::GenericAttributeProfile;

const int NAME_SIZE = 128;
const int DESCRIPTION_SIZE = 128;

struct BleAdvert
{
	uint64_t mac = 0;
	wchar_t name[NAME_SIZE];

	//unix time in seconds
	int64_t timestamp = 0;

	int32_t signalStrength = 0;
	int32_t powerLevel = 0; //16-bit is enough, but 32 for C# serialization

	guid *serviceUuids;
	int32_t numServiceUuids = 0; //8-bit is enough, but 32 for C# serialization
};

struct BleService
{
	guid serviceUuid;
};

struct BleCharacteristic
{
	guid characteristicUuid;
	wchar_t userDescription[DESCRIPTION_SIZE];
};

struct Subscription
{
	GattCharacteristic characteristic = nullptr;
	GattCharacteristic::ValueChanged_revoker revoker;
};