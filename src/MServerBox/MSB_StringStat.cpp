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

#include "MC_Base.h"
#if IS_PC_BUILD

#include "mdb_stringconversion.h"

#include "MSB_StringStat.h"
#include "MSB_WriteMessage.h"

MSB_StringStat::MSB_StringStat()
	: MSB_StatsContext::IValue()
{

}

MSB_StringStat::~MSB_StringStat()
{

}

void
MSB_StringStat::Marshal(
	MSB_WriteMessage&		aWriteMessage)
{
	MT_MutexLock			lock(myLock);
	aWriteMessage.WriteString(myString);	
}

void
MSB_StringStat::ToString(
	MSB_WriteMessage&		aWriteMessage)
{
	MT_MutexLock			lock(myLock);
	aWriteMessage.WriteString(myString);	
}

/**
 * This function assumes the string is already encoded in UTF-8, if not
 * this is an error you will be punished.
 *
 * All 7-bit ascii strings are valid utf-8.
 */
void
MSB_StringStat::operator = (
	const char*				aString)
{
	MT_MutexLock			lock(myLock);
	myString = aString;
}

/**
 * 
 */
void
MSB_StringStat::operator = (
	const wchar_t*			aString)
{
	MC_StaticString<500>	result;
	convertToMultibyte<500>(result, aString);

	MT_MutexLock			lock(myLock);
	myString = result;
}

#endif // IS_PC_BUILD
