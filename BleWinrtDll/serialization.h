#pragma once

#include "stdafx.h"

using namespace std;
using namespace winrt;

union to_guid
{
	uint8_t buf[16];
	guid guid;
};

guid make_guid(const wchar_t* value);

string convert_to_string(const wstring& wstr);

uint64_t ConvertMacAddressToULong(const winrt::hstring& macAddress);