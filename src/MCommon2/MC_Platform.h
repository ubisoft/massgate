// Massgate
// Copyright (C) 2017 Ubisoft Entertainment
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
#pragma once 
#ifndef MCPLATFORM__H__INCLUDED
#define MCPLATFORM__H__INCLUDED

#include "MC_Platform_Win32.h"

namespace MC_Platform
{
	bool GetCDKeyFromRegistry( const char* aCdKeyLocation, MC_String& aCDKey);
	bool WriteCdKeyToRegistry( const char* aCdKeyLocation, const char* aCdKey);
	bool IsWindowsVista();
	bool CopyTextToClipboard(const char* aString);

	bool IsLaptop();
	bool IsACOffline();
};

#endif // MCPLATFORM__H__INCLUDED

