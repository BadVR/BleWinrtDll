#include "stdafx.h"
#include "logging.h"
#include "serialization.h"


LogCallback* loggerCallback = nullptr;
ErrorCallback* errorCallback = nullptr;


void RegisterLogCallback(LogCallback cb)
{
	loggerCallback = cb;
}

void RegisterErrorCallback(ErrorCallback cb)
{
	errorCallback = cb;
}

void Log(const wstring& s)
{
	if (loggerCallback)
		(*loggerCallback)(convert_to_string(s).c_str());
}

void Log(const char* s)
{
	if (loggerCallback)
		(*loggerCallback)(s);
}

void LogError(const wchar_t* message, ...)
{
	wchar_t last_error[2048];
	va_list args;
	va_start(args, message);
	vswprintf_s(last_error, message, args);
	va_end(args);

	if (errorCallback)
		(*errorCallback)(last_error);
}