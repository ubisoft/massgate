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
#include "StdAfx.h"
#include "MC_SystemPaths.h"
#include "MF_File.h"

#include <shlobj.h>

#define APPNAMESIZE 128
static char ourAppName[APPNAMESIZE] = {0};
static char ourDocumentsPathName[APPNAMESIZE] = {0};

#if IS_PC_BUILD
static bool GetFolderPath(MC_String& aBasePath, const char* aPathToAppend, int aFolderFlag)
{
	TCHAR szPath[MAX_PATH];

	if(SUCCEEDED(SHGetFolderPath(NULL, 
								 aFolderFlag|CSIDL_FLAG_CREATE, 
								 NULL, 
								 0, 
								 szPath))) 
	{
		aBasePath = szPath;

		if (strlen(ourDocumentsPathName) > 0)
		{
			if (!aBasePath.EndsWith("\\"))
				aBasePath += "\\";
			aBasePath += ourDocumentsPathName;
		}

		if (aPathToAppend)
		{
			if (!aBasePath.EndsWith("\\"))
				aBasePath += "\\";
			aBasePath += aPathToAppend;
		}

		return true;
	}

	return false;
}
#endif

void MC_SystemPaths::SetAppName(const char* anAppName, const char* aDocumentsPathName)
{
	ourAppName[0] = 0;
	ourDocumentsPathName[0] = 0;

	assert(strlen(anAppName) < APPNAMESIZE);
	
	if (anAppName)
		strcpy(ourAppName, anAppName);

	if(aDocumentsPathName)
	{
		assert(strlen(aDocumentsPathName) < APPNAMESIZE);
		strcpy(ourDocumentsPathName, aDocumentsPathName);
	}
	else
		strcpy(ourDocumentsPathName, anAppName);
		
}

const char* MC_SystemPaths::GetAppName()
{
	return ourAppName;
}

bool MC_SystemPaths::GetUserDocumentsPath(MC_String& aBasePath, const char* aPathToAppend)
{
	return GetFolderPath(aBasePath, aPathToAppend, CSIDL_PERSONAL);
}

bool MC_SystemPaths::GetCommonDocumentsPath(MC_String& aBasePath, const char* aPathToAppend)
{
	return GetFolderPath(aBasePath, aPathToAppend, CSIDL_COMMON_DOCUMENTS);
}
	
bool MC_SystemPaths::GetUserAppDataPath(MC_String& aBasePath, const char* aPathToAppend)
{
	return GetFolderPath(aBasePath, aPathToAppend, CSIDL_LOCAL_APPDATA);
}

bool MC_SystemPaths::GetCommonAppDataPath(MC_String& aBasePath, const char* aPathToAppend)
{

	return GetFolderPath(aBasePath, aPathToAppend, CSIDL_COMMON_APPDATA);
}

MC_StaticString<260> MC_SystemPaths::GetUserDocumentsFileName(const char* aFileName)
{
	MC_StaticString<260> path;

	{
		TCHAR szPath[MAX_PATH];
		if (FAILED(SHGetFolderPath(NULL, 
			CSIDL_PERSONAL|CSIDL_FLAG_CREATE, 
			NULL, 
			0, 
			szPath))) 
		{
			return "";
		}
		path = szPath;
	}

	if (strlen(ourDocumentsPathName) > 0)
	{
		if (!path.EndsWith("\\"))
			path += "\\";
		path += ourDocumentsPathName;
	}

	path += "\\";
	path += aFileName;
	return path;
}