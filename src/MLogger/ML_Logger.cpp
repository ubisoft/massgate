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
#include "mc_globaldefines.h"

#include <cstring>
#include <cstdarg>
#include <cstdio>

#include "MC_HybridArray.h"

#include "ML_Backend.h"
#include "ML_Logger.h"

#define MAX_STATIC_BUFFER 32*1024
ML_Logger*				ML_Logger::ourSlots[2] = {new ML_Logger(), new ML_Logger()};

ML_Logger::ML_Logger()
{
	
}

void
ML_Logger::Log(
	LogLevel		aLevel,
	const char*		aFile, 
	const int		aLine,
	const char*		aFormat,
	... )
{
	char		result[MAX_STATIC_BUFFER];

	va_list		ap;
	va_start(ap, aFormat);
	int			used = vsnprintf(result, MAX_STATIC_BUFFER, aFormat, ap);
	if(used >= MAX_STATIC_BUFFER)
	{
		strcpy((char*) result, "Got logging request for something larger than buffer, refusing to log it");
		aLevel = LOG_LEVEL_WARN;
	}
	va_end(ap);

	for(int i = 0; i < myBackends.Count(); i++)
	{
		myBackends[i]->Log(aLevel, aFile, aLine, result);
	}
}