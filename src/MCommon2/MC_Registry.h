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
#ifndef MC_REGISTRY_H
#define MC_REGISTRY_H


// Imported from ED_Common


#define CLASS_NAME_LENGTH 255


#define WIN32_LEAN_AND_MEAN
#include "MC_Platform.h"
#include "mc_string.h"


class MC_Registry
{
public: // Interface

	MC_Registry();
	virtual    ~MC_Registry();

	inline bool PathIsValid()
	{
		return (myCurrentPath.GetLength() > 0);
	}

	inline MC_String GetCurrentPath()
	{
		return myCurrentPath;
	}

	inline HKEY GetRootKey()
	{
		return myRootKey;
	}

	bool ClearKey();
	bool SetRootKey(HKEY aRootKey);
	bool CreateKey(MC_String aKey);
	bool DeleteKey(MC_String aKey);
	bool DeleteValue(MC_String aName);
	int GetDataSize(MC_String aValueName);
	DWORD GetDataType(MC_String aValueName);
	int GetSubKeyCount();
	int GetValueCount();
	bool KeyExists(MC_String aKey, HKEY aRootKey = NULL);
	bool SetKey(MC_String aKey, bool aCanCreateFlag);
	bool ValueExists(MC_String aName);
	static void GetLocalComputerName(MC_String& aName);

	// Data read functions
	double ReadFloat(MC_String aName, double aDefaultVal);
	MC_String ReadString(MC_String aName, MC_String aDefaultVal);
	int ReadInt(MC_String aName, int aDefaultVal);
	bool ReadBool(MC_String aName, bool aDefaultVal);

	bool EnumValueAsString(MC_String& aName, MC_String& aValue, int anIndex);
	bool EnumSubKey(MC_String& aName, int anIndex);

	// Data writing functions
	bool WriteBool(MC_String aName, bool aValue);
	bool WriteString(MC_String aName, MC_String aValue);
	bool WriteFloat(MC_String aName, double aValue);
	bool WriteInt(MC_String aName, int aValue);


private: // Properties

	HKEY myRootKey;
	MC_String myCurrentPath;
};

#endif // MC_REGISTRY_H

