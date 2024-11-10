#include "stdafx.h"
#include "serialization.h"

using namespace std;

const uint8_t BYTE_ORDER[] = { 3, 2, 1, 0, 5, 4, 7, 6, 8, 9, 10, 11, 12, 13, 14, 15 };

guid make_guid(const wchar_t* value)
{
	to_guid to_guid;
	memset(&to_guid, 0, sizeof(to_guid));
	int offset = 0;
	for (unsigned int i = 0; i < wcslen(value); i++)
	{
		if (value[i] >= '0' && value[i] <= '9')
		{
			uint8_t digit = value[i] - '0';
			to_guid.buf[BYTE_ORDER[offset / 2]] += offset % 2 == 0 ? digit << 4 : digit;
			offset++;
		}
		else if (value[i] >= 'A' && value[i] <= 'F')
		{
			uint8_t digit = 10 + value[i] - 'A';
			to_guid.buf[BYTE_ORDER[offset / 2]] += offset % 2 == 0 ? digit << 4 : digit;
			offset++;
		}
		else if (value[i] >= 'a' && value[i] <= 'f')
		{
			uint8_t digit = 10 + value[i] - 'a';
			to_guid.buf[BYTE_ORDER[offset / 2]] += offset % 2 == 0 ? digit << 4 : digit;
			offset++;
		}
		else
		{
			// skip char
		}
	}

	return to_guid.guid;
}

string convert_to_string(const wstring& wstr)
{
	// https://stackoverflow.com/questions/215963/how-do-you-properly-use-widechartomultibyte
	if (wstr.empty())
		return string();

	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);

	return strTo;
}


uint64_t ConvertMacAddressToULong(const winrt::hstring& macAddress)
{
	std::wstring macAddressStr(macAddress);
	std::wstring macWithoutDelimiter;

	for (wchar_t ch : macAddressStr)
	{
		if (ch != L':')
		{
			macWithoutDelimiter += ch;
		}
	}

	std::wstringstream ssConverted(macWithoutDelimiter);
	uint64_t result;
	ssConverted >> std::hex >> result;

	return result;
}