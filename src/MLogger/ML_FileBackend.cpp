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

#include "MC_String.h"
#include "ML_FileBackend.h"

ML_FileBackend::ML_FileBackend(
   const char*			aFilename,
   bool					aUseTimestampFlag,
   bool					aUseFileLineFlag,
   int					aStringLimit)
   : myCurrentFileDate(0), myBaseName(aFilename), myCurrentFile(NULL)
   , ML_Backend(aUseTimestampFlag, aUseFileLineFlag, aStringLimit)
{
	myFileNameBufferLength = (int) strlen(myBaseName) + 60;
	myFileNameBuffer = new char[myFileNameBufferLength];
	memset(myFileNameBuffer, 9, myFileNameBufferLength);

	myCurrentFileDate = 0;
	RotateLog(time(NULL) / 86400);
}

ML_FileBackend::~ML_FileBackend()
{
	
}

void
ML_FileBackend::WriteLog(
	const char*		aString)
{
	time_t			now = time(NULL) / 86400;
	if(now > myCurrentFileDate)
		RotateLog(now);

	//fputs(aString, myCurrentFile);
	fprintf(myCurrentFile, "%s\r\n", aString);
}

void
ML_FileBackend::RotateLog(
	time_t			aDate)
{
	MT_MutexLock		lock(myLock);

	if(aDate > myCurrentFileDate)
	{
		myCurrentFileDate = aDate;

		time_t				rt = myCurrentFileDate * 86400;
		struct tm			tm;
		gmtime_s(&tm, &rt);

		char				dateBuffer[20];
		strftime(dateBuffer, 20, "%Y%m%d", &tm);

		_snprintf_s(myFileNameBuffer, myFileNameBufferLength, _TRUNCATE, "%s_%s.txt", myBaseName, dateBuffer);

		if(myCurrentFile)
			fclose(myCurrentFile);

		myCurrentFile = fopen(myFileNameBuffer, "a+b");
	}
}