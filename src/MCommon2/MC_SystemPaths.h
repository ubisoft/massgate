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
#ifndef MC_SYSTEMPATHS_H
#define MC_SYSTEMPATHS_H

#include "mc_string.h"


class MC_SystemPaths
{
public:
	// is aDocumentsPathName is NULL the documents path is set to the app name
	static void SetAppName(const char* anAppName, const char* aDocumentsPathName = 0);
	static const char* GetAppName();

	static bool GetUserDocumentsPath(MC_String& aBasePath, const char* aPathToAppend);
	static bool GetCommonDocumentsPath(MC_String& aBasePath, const char* aPathToAppend);
	
	static bool GetUserAppDataPath(MC_String& aBasePath, const char* aPathToAppend);
	static bool GetCommonAppDataPath(MC_String& aBasePath, const char* aPathToAppend);

	static MC_StaticString<260> GetUserDocumentsFileName(const char* aFileName);
};

#endif

