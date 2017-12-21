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

#if IS_PC_BUILD

#include "mF_file.h"
#include "mf_file_platform.h"

UINT PLATFORM::GetTempFileName(const char* aPathName, const char* aPrefixString, UINT aUniqueFlag, char* aDestName)
{
	return ::GetTempFileName(aPathName, aPrefixString, aUniqueFlag, aDestName);
}

DWORD PLATFORM::GetTempPath(DWORD aLength, char* aDestName)
{
	return ::GetTempPath(aLength, aDestName);
}

HANDLE PLATFORM::FindFirstFile(LPCSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData)
{
	if (lpFileName[0] && lpFileName[1] && lpFileName[1] == ':' && lpFileName[2] && lpFileName[2] == '\\' && !lpFileName[3])
	{
		char tmp[5] = "c:\\*";
		tmp[0] = lpFileName[0];
		return ::FindFirstFile(tmp, lpFindFileData);
	}
	else if (lpFileName[strlen(lpFileName)-1] == '\\')
	{
		MC_StaticString<260> tmp(lpFileName);
		tmp[tmp.GetLength()-1] = 0;
		return ::FindFirstFile(tmp, lpFindFileData);
	}
	else
	{
		return ::FindFirstFile(lpFileName, lpFindFileData);
	}
}

HANDLE PLATFORM::CreateFile(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,HANDLE hTemplateFile)
{
	return ::CreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

#endif // IS_PC_BUILD

