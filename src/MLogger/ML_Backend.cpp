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

// GetCurrentThreadId() not included by the precompiled headers :(
#include <windows.h> 

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/types.h>
#include <sys/timeb.h>
#include <malloc.h>
#include <time.h>

#include "ML_Backend.h"

ML_Backend::ML_Backend(
	bool		aUseTimestampFlag,
	bool		aUseFileLineFlag,
	int			aStringLimit)
	: myUseTimestampFlag(aUseTimestampFlag)
	, myUseFileLineFlag(aUseFileLineFlag)
	, myStringLimit(aStringLimit)
{
	myLevelStrings[LOG_LEVEL_FATAL]		= "FATAL";
	myLevelStrings[LOG_LEVEL_WARN]		= "WARN";
	myLevelStrings[LOG_LEVEL_ERROR]		= "ERROR";
	myLevelStrings[LOG_LEVEL_INFO]		= "INFO";
	myLevelStrings[LOG_LEVEL_DEBUG]		= "DEBUG";
}

ML_Backend::~ML_Backend()
{
}

void
ML_Backend::SetLevelString(
	LogLevel		aLevel,
	const char*		aString)
{
	myLevelStrings[aLevel] = aString;
}

char* 
ML_Backend::PrivStripPath(
	const char* aFile)
{
	if(!aFile)
		return "";

	int lastSlashFound = -1;

	for(int i = 0; aFile[i] != '\0'; i++)
		if(aFile[i] == '\\')
			lastSlashFound = i;

	return (char*) aFile + lastSlashFound + 1; 
}

void
ML_Backend::Log(
	LogLevel		aLevel,
	const char*		aFile, 
	const int		aLine,
	const char*		aString)
{
	char			timestring[40] = { 0 };
	char			filelinestring[256] = { 0 }; 
	
	if(myUseTimestampFlag)
	{
		time_t		now = time(NULL);
		struct tm	tm;
		localtime_s(&tm, &now);

		strftime(timestring, 40, "%Y-%m-%d %H:%M:%S", &tm);
	}

	if(myUseFileLineFlag)
		_snprintf_s(filelinestring, 255, _TRUNCATE, "%s (%d) : ", PrivStripPath(aFile), aLine);		

	int				totsize = (int) strlen(aString) + 150;
	char*			complete = (char*) alloca(totsize);
	int				length = _snprintf_s(complete, totsize, _TRUNCATE, "%s [%u] [%-5s] %s%s", 
								timestring, GetCurrentThreadId(), myLevelStrings[aLevel], filelinestring, aString);

	if(length > myStringLimit && myStringLimit > 4)
	{
		complete[myStringLimit] = 0;
		complete[myStringLimit-3] = '.';
		complete[myStringLimit-2] = '.';
		complete[myStringLimit-1] = '.';
	}

	WriteLog(complete);
}