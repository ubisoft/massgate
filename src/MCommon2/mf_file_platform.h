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
#ifndef MF_FILE_PLATFORM_H
#define MF_FILE_PLATFORM_H

// Platform namespace for stuff supported per platform, intended only for MCommon2 inclusion
namespace PLATFORM
{
	UINT	GetTempFileName	(const char* aPathName, const char* aPrefixString, UINT aUniqueFlag, char* aDestName);
	DWORD	GetTempPath		(DWORD aLength, char* aDestName);
	HANDLE	FindFirstFile	(LPCSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData);
	HANDLE	CreateFile		(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,HANDLE hTemplateFile);
};

#endif // not MF_FILE_PLATFORM_H

