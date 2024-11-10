#pragma once

#include "stdafx.h"

using namespace std;

void Log(const wstring& s);
void Log(const char* s);
void LogError(const wchar_t* message, ...);


//these functions will be available through the native DLL interface, exposed to Unity
extern "C"
{
	using LogCallback = void(const char*);
	using ErrorCallback = void(const wchar_t*);

	//register logging functions
	__declspec(dllexport) void RegisterLogCallback(LogCallback cb);
	__declspec(dllexport) void RegisterErrorCallback(ErrorCallback cb);
}