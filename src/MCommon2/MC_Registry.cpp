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
#include "MC_Registry.h"


MC_Registry::MC_Registry()
:myCurrentPath("")
#if IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile
,myRootKey(HKEY_LOCAL_MACHINE)
#endif
{

}

MC_Registry::~MC_Registry()
{
	ClearKey();
}

bool MC_Registry::ClearKey()
{
#if IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile
	myRootKey = HKEY_LOCAL_MACHINE;
#endif
	myCurrentPath = "";

	return true;
}

bool MC_Registry::SetRootKey(HKEY aRootKey)
{
#if !IS_PC_BUILD 		// SWFM:AW - To get the xb360 to compile
	return false;
#else

	// The key must be valid
	if (aRootKey != HKEY_CLASSES_ROOT &&
		aRootKey != HKEY_CURRENT_USER &&
		aRootKey != HKEY_LOCAL_MACHINE &&
		aRootKey != HKEY_USERS)
	{
		return false;
	}

	myRootKey = aRootKey;

	return true;
#endif
}

bool MC_Registry::CreateKey(MC_String aKey)
{
#if !IS_PC_BUILD 		// SWFM:AW - To get the xb360 to compile
	return false;
#else

	HKEY hKey;
	DWORD disposition = 0;

	if (::RegCreateKeyEx(myRootKey,
		(LPCTSTR)aKey,
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,
		NULL,
		&hKey,
		&disposition) != ERROR_SUCCESS)
	{
		return false;
	}

	::RegCloseKey(hKey);
	myCurrentPath = aKey;

	return true;
#endif
}

bool MC_Registry::DeleteKey(MC_String aKey)
{
#if !IS_PC_BUILD 		// SWFM:AW - To get the xb360 to compile
	return false;
#else

	if (!KeyExists(aKey))
		return true;

	if (::RegDeleteKey(myRootKey, aKey) != ERROR_SUCCESS)
		return false;

	return true;
#endif
}

bool MC_Registry::DeleteValue(MC_String aName)
{
	assert(myCurrentPath.GetLength() > 0);

#if !IS_PC_BUILD 		// SWFM:AW - To get the xb360 to compile
	return false;
#else

	HKEY hKey;
	LONG lResult;

	if (::RegOpenKeyEx(myRootKey,
		(LPCTSTR)myCurrentPath,
		0,
		KEY_SET_VALUE,
		&hKey) != ERROR_SUCCESS)
	{
		return false;
	}

	lResult = ::RegDeleteValue(hKey, (LPCTSTR)aName);
	::RegCloseKey(hKey);

	if (lResult == ERROR_SUCCESS)
		return true;

	return false;
#endif
}

int MC_Registry::GetDataSize(MC_String aValueName)
{
	assert(myCurrentPath.GetLength() > 0);

#if !IS_PC_BUILD 		// SWFM:AW - To get the xb360 to compile
	return -1;
#else

	HKEY hKey;
	LONG lResult;
	DWORD size = 1;

	if (::RegOpenKeyEx(myRootKey,
		(LPCTSTR)myCurrentPath,
		0,
		KEY_QUERY_VALUE,
		&hKey) != ERROR_SUCCESS)
	{
		return -1;
	}

	lResult = ::RegQueryValueEx(hKey,
		(LPCTSTR)aValueName,
		NULL,
		NULL,
		NULL,
		&size);

	::RegCloseKey(hKey);

	if (lResult != ERROR_SUCCESS)
		return -1;

	return (int)size;
#endif
}


DWORD MC_Registry::GetDataType(MC_String aValueName)
{
	assert(myCurrentPath.GetLength() > 0);

#if !IS_PC_BUILD 		// SWFM:AW - To get the xb360 to compile
	return 0;
#else

	HKEY hKey;
	LONG lResult;
	DWORD type = 1;

	if (::RegOpenKeyEx(myRootKey,
		(LPCTSTR)myCurrentPath,
		0,
		KEY_QUERY_VALUE,
		&hKey) != ERROR_SUCCESS)
	{
		return 0;
	}

	lResult = ::RegQueryValueEx(hKey,
		(LPCTSTR)aValueName,
		NULL,
		&type,
		NULL,
		NULL);

	::RegCloseKey(hKey);

	if (lResult == ERROR_SUCCESS)
		return type;

	return 0;
#endif
}

int MC_Registry::GetSubKeyCount()
{
	assert(myCurrentPath.GetLength() > 0);

#if !IS_PC_BUILD 		// SWFM:AW - To get the xb360 to compile
	return -1;
#else

	HKEY hKey;
	LONG lResult;
	DWORD subKeyCount, valueCount, classNameLength,
		maxSubKeyName, maxValueName, maxValueLength;

	FILETIME lastWritten;
	_TCHAR classBuffer[CLASS_NAME_LENGTH];
	classNameLength = CLASS_NAME_LENGTH;

	if (::RegOpenKeyEx(myRootKey,
		(LPCTSTR)myCurrentPath,
		0,
		KEY_READ,
		&hKey) != ERROR_SUCCESS)
	{
		return -1;
	}

	lResult = ::RegQueryInfoKey(hKey,
		classBuffer,
		&classNameLength,
		NULL,
		&subKeyCount,
		&maxSubKeyName,
		NULL,
		&valueCount,
		&maxValueName,
		&maxValueLength,
		NULL,
		&lastWritten);

	::RegCloseKey(hKey);

	if (lResult != ERROR_SUCCESS)
		return -1;

	return (int)subKeyCount;
#endif
}

int MC_Registry::GetValueCount()
{
	assert(myCurrentPath.GetLength() > 0);

#if !IS_PC_BUILD 		// SWFM:AW - To get the xb360 to compile
	return -1;
#else

	HKEY hKey;
	LONG lResult;
	DWORD subKeyCount, valueCount, classNameLength,
		maxSubKeyName, maxValueName, maxValueLength;

	FILETIME lastWritten;
	_TCHAR classBuffer[CLASS_NAME_LENGTH];
	classNameLength = CLASS_NAME_LENGTH;

	if (::RegOpenKeyEx(myRootKey,
		(LPCTSTR)myCurrentPath,
		0,
		KEY_READ,
		&hKey) != ERROR_SUCCESS)
	{
		return -1;
	}

	lResult = ::RegQueryInfoKey(hKey,
		classBuffer,
		&classNameLength,
		NULL,
		&subKeyCount,
		&maxSubKeyName,
		NULL,
		&valueCount,
		&maxValueName,
		&maxValueLength,
		NULL,
		&lastWritten);

	::RegCloseKey(hKey);

	if (lResult != ERROR_SUCCESS)
		return -1;

	return (int)valueCount;
#endif
}

bool MC_Registry::KeyExists(MC_String aKey, HKEY aRootKey)
{
#if !IS_PC_BUILD 		// SWFM:AW - To get the xb360 to compile
	return false;
#else

	HKEY hKey;

	if (aRootKey == NULL)
		aRootKey = myRootKey;

	LONG lResult = ::RegOpenKeyEx(aRootKey,
		(LPCTSTR)aKey,
		0,
		KEY_READ,
		&hKey);

	::RegCloseKey(hKey);

	if (lResult == ERROR_SUCCESS)
		return true;

	return false;
#endif
}

bool MC_Registry::SetKey(MC_String aKey, bool aCanCreateFlag)
{

#if !IS_PC_BUILD 		// SWFM:AW - To get the xb360 to compile
	return false;
#else

	HKEY hKey;
	DWORD disposition;

	if (aKey.GetLength() == 0)
	{
		myCurrentPath = "";
		return true;
	}

	if (aCanCreateFlag)
	{
		if (::RegCreateKeyEx(myRootKey,
			(LPCTSTR)aKey,
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,
			NULL,
			&hKey,
			&disposition) != ERROR_SUCCESS)
		{
			return false;
		}

		myCurrentPath = aKey;

		::RegFlushKey(hKey);
		::RegCloseKey(hKey);

		return true;
	}

	if (::RegOpenKeyEx(myRootKey,
		(LPCTSTR)aKey,
		0,
		KEY_READ,
		&hKey) != ERROR_SUCCESS)
	{
		return false;
	}

	myCurrentPath = aKey;

	::RegFlushKey(hKey);
	::RegCloseKey(hKey);

	return true;
#endif
}

bool MC_Registry::ValueExists(MC_String aName)
{
	assert(myCurrentPath.GetLength() > 0);

#if !IS_PC_BUILD 	// SWFM:AW - To get the xb360 to compile
	return false;
#else

	HKEY hKey;
	LONG lResult;

	if (::RegOpenKeyEx(myRootKey,
		(LPCTSTR)myCurrentPath,
		0,
		KEY_READ,
		&hKey) != ERROR_SUCCESS)
	{
		return false;
	}

	lResult = ::RegQueryValueEx(hKey,
		(LPCTSTR)aName,
		NULL,
		NULL,
		NULL,
		NULL);

	::RegCloseKey(hKey);

	if (lResult == ERROR_SUCCESS)
		return true;

	return false;
#endif
}

void MC_Registry::GetLocalComputerName(MC_String& aName)
{
	aName = "UNKNOWN";

#if IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile
	HKEY rkey; 
	char name[512];

	if(::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		"SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ComputerName",
		0,
		KEY_READ,
		&rkey) == ERROR_SUCCESS) 
	{ 
		unsigned long d1 = sizeof(name) - 1;
		unsigned long d2 = REG_SZ;

		if (::RegQueryValueEx(rkey, "ComputerName", 0, &d2, (unsigned char*) name, &d1) == ERROR_SUCCESS) 
		{ 
			aName = name;
		} 

		RegCloseKey(rkey); 
	}
#endif

}

bool MC_Registry::EnumValueAsString(MC_String& aName, MC_String& aValue, int anIndex)
{
	assert(myCurrentPath.GetLength() > 0);

#if !IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile
	return false;
#else

	HKEY key;
	TCHAR sz[255]; 
	DWORD szSize = 255;
	DWORD type = REG_SZ;
	TCHAR val[255];
	DWORD valSize = 255;

	if (anIndex >= GetValueCount() || anIndex < 0)
		return false;

	if (::RegOpenKeyEx(myRootKey,
		(LPCTSTR)myCurrentPath,
		0,
		KEY_READ,
		&key) != ERROR_SUCCESS)
	{
		return false;
	}

	if (::RegEnumValue(key, anIndex, sz, &szSize, 0, &type, (LPBYTE)val, &valSize) != ERROR_SUCCESS)
	{
		::RegCloseKey(key);
		return false;
	}

	if (type != REG_SZ)
	{
		::RegCloseKey(key);
		return false;
	}

	aValue = val;
	aName = sz;

	::RegCloseKey(key);

	return true;
#endif
}

bool MC_Registry::EnumSubKey(MC_String& aName, int anIndex)
{
	assert(myCurrentPath.GetLength() > 0);

#if !IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile
	return false;
#else

	HKEY key;
	TCHAR sz[255]; 
	DWORD szSize = 255;
	PFILETIME ft = NULL;

	if (anIndex >= GetSubKeyCount() || anIndex < 0)
		return false;

	if (::RegOpenKeyEx(myRootKey,
		(LPCTSTR)myCurrentPath,
		0,
		KEY_READ,
		&key) != ERROR_SUCCESS)
	{
		return false;
	}

	if (::RegEnumKeyEx(key, anIndex, sz, &szSize, NULL, NULL, NULL, ft) != ERROR_SUCCESS)
	{
		::RegCloseKey(key);
		return false;
	}

	aName = sz;

	::RegCloseKey(key);

	return true;
#endif
}

double MC_Registry::ReadFloat(MC_String aName, double aDefaultVal)
{
	assert(myCurrentPath.GetLength() > 0);

#if !IS_PC_BUILD 		// SWFM:AW - To get the xb360 to compile
	return aDefaultVal;
#else

	DWORD type = REG_BINARY;
	double d;
	DWORD size = sizeof(d);
	HKEY hKey;

	if (::RegOpenKeyEx(myRootKey,
		(LPCTSTR)myCurrentPath,
		0,
		KEY_READ,
		&hKey) != ERROR_SUCCESS)
	{
		return aDefaultVal;
	}

	if (::RegQueryValueEx(hKey,
		(LPCTSTR)aName,
		NULL,
		&type,
		(LPBYTE)&d,
		&size) != ERROR_SUCCESS)
	{
		d = aDefaultVal;
	}

	::RegCloseKey(hKey);

	return d;
#endif
}

MC_String MC_Registry::ReadString(MC_String aName, MC_String aDefaultVal)
{
	assert(myCurrentPath.GetLength() > 0);

#if !IS_PC_BUILD 		// SWFM:AW - To get the xb360 to compile
	return aDefaultVal;
#else

	DWORD type = REG_SZ;
	DWORD size = 255;
	bool success = true;
	_TCHAR sz[255];
	HKEY hKey;

	// Make sure it is the proper type
	type = GetDataType(aName);

	if (type != REG_SZ && type != REG_EXPAND_SZ)
		return aDefaultVal;

	if (::RegOpenKeyEx(myRootKey,
		(LPCTSTR)myCurrentPath,
		0,
		KEY_READ,
		&hKey) != ERROR_SUCCESS)
	{
		return aDefaultVal;
	}

	if (::RegQueryValueEx(hKey,
		(LPCTSTR)aName,
		NULL,
		&type,
		(LPBYTE)sz,
		&size) != ERROR_SUCCESS)
	{
		success = false;
	}

	::RegCloseKey(hKey);

	if (!success)
		return aDefaultVal;

	return MC_String((LPCTSTR)sz);
#endif
}

int MC_Registry::ReadInt(MC_String aName, int aDefaultVal)
{
	assert(myCurrentPath.GetLength() > 0);

#if !IS_PC_BUILD 		// SWFM:AW - To get the xb360 to compile
	return aDefaultVal;
#else

	DWORD type = REG_BINARY;
	int n;
	DWORD size = sizeof(n);
	HKEY hKey;

	if (::RegOpenKeyEx(myRootKey,
		(LPCTSTR)myCurrentPath,
		0,
		KEY_READ,
		&hKey) != ERROR_SUCCESS)
	{
		return aDefaultVal;
	}

	if (::RegQueryValueEx(hKey,
		(LPCTSTR)aName,
		NULL,
		&type,
		(LPBYTE)&n,
		&size) != ERROR_SUCCESS)
	{
		n = aDefaultVal;
	}

	::RegCloseKey(hKey);

	return n;
#endif
}

bool MC_Registry::ReadBool(MC_String aName, bool aDefaultVal)
{
	assert(myCurrentPath.GetLength() > 0);

#if !IS_PC_BUILD 		// SWFM:AW - To get the xb360 to compile
	return false;
#else

	DWORD type = REG_BINARY;
	BOOL b;
	DWORD size = sizeof(b);
	HKEY hKey;

	if (::RegOpenKeyEx(myRootKey,
		(LPCTSTR)myCurrentPath,
		0,
		KEY_READ,
		&hKey) != ERROR_SUCCESS)
	{
		return aDefaultVal;
	}

	if (::RegQueryValueEx(hKey,
		(LPCTSTR)aName,
		NULL,
		&type,
		(LPBYTE)&b,
		&size) != ERROR_SUCCESS)
	{
		b = aDefaultVal;
	}

	::RegCloseKey(hKey);

	return b != 0;
#endif
}

bool MC_Registry::WriteBool(MC_String aName, bool aValue)
{
	assert(myCurrentPath.GetLength() > 0);

#if !IS_PC_BUILD	 	// SWFM:AW - To get the xb360 to compile
	return false;
#else

	const BOOL value = aValue;
	bool success = true;
	HKEY hKey;

	if (::RegOpenKeyEx(myRootKey,
		(LPCTSTR)myCurrentPath,
		0,
		KEY_WRITE,
		&hKey) != ERROR_SUCCESS)
	{
		return false;
	}

	if (::RegSetValueEx(hKey,
		(LPCTSTR)aName,
		0,
		REG_BINARY,
		(LPBYTE)&value,
		sizeof(value)) != ERROR_SUCCESS)
	{
		success = false;
	}

	::RegFlushKey(hKey);
	::RegCloseKey(hKey);

	return success;
#endif
}

bool MC_Registry::WriteString(MC_String aName, MC_String aValue)
{
	assert(myCurrentPath.GetLength() > 0);

#if !IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile
	return false;
#else

	bool success = true;
	HKEY hKey;
	_TCHAR sz[255];

	if (aValue.GetLength() > 254)
		return false;

	_tcscpy(sz, (LPCTSTR)aValue);

	if (::RegOpenKeyEx(myRootKey,
		(LPCTSTR)myCurrentPath,
		0,
		KEY_WRITE,
		&hKey) != ERROR_SUCCESS)
	{
		return false;
	}

	if (::RegSetValueEx(hKey,
		(LPCTSTR)aName,
		0,
		REG_SZ,
		(LPBYTE)&sz,
		(DWORD)_tcslen(sz) + 1) != ERROR_SUCCESS)
	{
		success = false;
	}

	::RegFlushKey(hKey);
	::RegCloseKey(hKey);

	return success;
#endif
}

bool MC_Registry::WriteFloat(MC_String aName, double aValue)
{
	assert(myCurrentPath.GetLength() > 0);

#if !IS_PC_BUILD 		// SWFM:AW - To get the xb360 to compile
	return false;
#else

	bool success = true;
	HKEY hKey;

	if (::RegOpenKeyEx(myRootKey,
		(LPCTSTR)myCurrentPath,
		0,
		KEY_WRITE,
		&hKey) != ERROR_SUCCESS)
	{
		return false;
	}

	if (::RegSetValueEx(hKey,
		(LPCTSTR)aName,
		0,
		REG_BINARY,
		(LPBYTE)&aValue,
		sizeof(aValue)) != ERROR_SUCCESS)
	{
		success = false;
	}

	::RegFlushKey(hKey);
	::RegCloseKey(hKey);

	return success;
#endif
}

bool MC_Registry::WriteInt(MC_String aName, int aValue)
{
	assert(myCurrentPath.GetLength() > 0);

#if !IS_PC_BUILD 		// SWFM:AW - To get the xb360 to compile
	return false;
#else

	bool success = true;
	HKEY hKey;

	if (::RegOpenKeyEx(myRootKey,
		(LPCTSTR)myCurrentPath,
		0,
		KEY_WRITE,
		&hKey) != ERROR_SUCCESS)
	{
		return false;
	}

	if (::RegSetValueEx(hKey,
		(LPCTSTR)aName,
		0,
		REG_BINARY,
		(LPBYTE)&aValue,
		sizeof(aValue)) != ERROR_SUCCESS)
	{
		success = false;
	}

	::RegFlushKey(hKey);
	::RegCloseKey(hKey);

	return success;
#endif
}


