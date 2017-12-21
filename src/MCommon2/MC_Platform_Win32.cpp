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
#include "stdafx.h"
#include "mc_platform.h"
#include "mc_misc.h"

#if IS_PC_BUILD
#include "MC_Registry.h"

namespace MC_Platform
{

bool GetCDKeyFromRegistry( const char* aCdKeyLocation, MC_String& aCdKey)
{
	// Read the key from machine settings, and if not there, then from user settings
	assert( aCdKeyLocation && aCdKeyLocation[0] );

	MC_Registry reg;
	if (reg.SetRootKey(HKEY_CURRENT_USER))
	{
		if (reg.SetKey(aCdKeyLocation, false))
		{
			aCdKey = reg.ReadString(MC_CDKEY_REGISTRY_NAME, "");
			if (aCdKey.GetLength())
				return true;
		}
	}
	return false;
}


bool WriteCdKeyToRegistry( const char* aCdKeyLocation, const char* aCdKey)
{
	assert( aCdKey && aCdKey[0] );
	assert( aCdKeyLocation && aCdKeyLocation[0] );
	MC_Registry reg;

	if (reg.SetRootKey(HKEY_CURRENT_USER))
		if (reg.SetKey(aCdKeyLocation, true))
			return reg.WriteString(MC_CDKEY_REGISTRY_NAME, aCdKey);

	return false;
}

bool IsWindowsVista()
{
	static bool didThisAlready = false;
	static bool isVista = false;

	if(!didThisAlready)
	{
		didThisAlready = true;

		OSVERSIONINFOEX osvi;
		ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

		if( GetVersionEx ((OSVERSIONINFO *) &osvi) )
		{
			if(osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
			{
				if ( osvi.dwMajorVersion >= 6 )
				{
					isVista = true;
				}
			}
		}
	}

	return isVista;
}

bool CopyTextToClipboard(const char* aString)
{
	bool res = false;

	if(OpenClipboard(NULL))
	{
		EmptyClipboard();

		int numCR = 0;
		int len = 0;
		for(len=0; aString[len]; len++)
			if(aString[len] == '\n')
				numCR++;

		HANDLE hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (len + numCR + 1) * sizeof(char));
		if(hglbCopy)
		{ 
			char* glbStr = (char*) GlobalLock(hglbCopy);
			if(glbStr)
			{
				int writeIndex = 0;
				for(int i=0; aString[i]; i++)
				{
					if(aString[i] == '\n')
						glbStr[writeIndex++] = '\r';

					glbStr[writeIndex++] = aString[i];
				}
				glbStr[writeIndex] = 0;
		
				GlobalUnlock(hglbCopy);

				if(SetClipboardData(CF_TEXT, hglbCopy))
					res = true;
			}
		}

		CloseClipboard();
	}

	return res;
}

bool IsLaptop()
{
	SYSTEM_POWER_STATUS pwrs;
	if (!GetSystemPowerStatus(&pwrs))
		return false; // os error
	if (pwrs.BatteryFlag == 128)
		return false; // no system battery, i.e. no laptop

	return true;
}

bool IsACOffline()
{
	SYSTEM_POWER_STATUS pwrs;

	if (!GetSystemPowerStatus(&pwrs))
		return false; // os error
	if (pwrs.ACLineStatus == 1 || pwrs.ACLineStatus == 255)
		return false; // ac power is online (or unknown -> 255)

	return true;
}

}; // namespace MC_Platform

MC_String MC_ComputerUserName()
{
	char userName[256];
	DWORD userNameSize = 255;
	GetUserName(userName, &userNameSize);
	return userName;
}

MC_String MC_ComputerName()
{
	DWORD computerNameSize = 255;
	char computerName[256];
	GetComputerName(computerName, &computerNameSize);
	return computerName;
}


#endif //IS_PC_BUILD


