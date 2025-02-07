// stdafx.h: Includedatei für Include-Standardsystemdateien
// oder häufig verwendete projektspezifische Includedateien,
// die nur in unregelmäßigen Abständen geändert werden.
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Selten verwendete Komponenten aus Windows-Headern ausschließen
// Windows-Headerdateien
#include <windows.h>



// Hier auf zusätzliche Header verweisen, die das Programm benötigt.

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <queue>
#include <map>
#include <mutex>
#include <condition_variable>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Web.Syndication.h>

#include "winrt/Windows.Devices.Bluetooth.h"
#include "winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h"
#include "winrt/Windows.Devices.Enumeration.h"

#include "winrt/Windows.Storage.Streams.h"
