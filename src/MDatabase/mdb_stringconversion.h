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
#ifndef __MDB_STRINGCONVERSION_H__
#define __MDB_STRINGCONVERSION_H__

#include "mc_string.h"
#include "mc_prettyprinter.h"
#include "mc_debug.h"

/* Get a string containing only alphabetic characters and no numbers etc. Works for all character sets. */
const MC_LocChar* getNormalizedString(const MC_LocChar* theString);

template<int SIZE>
void convertFromMultibyteToWideChar(MC_StaticLocString<SIZE>& dest, const char* aSource)
{
	const size_t sourceLen = strlen(aSource);
	int numChars = MultiByteToWideChar(CP_UTF8, 0, aSource, int(sourceLen), dest.GetBuffer(), dest.GetBufferSize()-1);
	if (numChars)
	{
		dest[numChars]=0;
	}
	else if (0 == numChars && sourceLen)
	{
		MC_Debug::DebugMessage(__FUNCTION__ " Failed to convert the following from UTF8 to UTF16 (errorcode: %d) :", GetLastError());
		MC_PrettyPrinter::ToDebug((const char*)aSource, sourceLen);
	}
}


template<int SIZE>
void convertToMultibyte(MC_StaticString<SIZE>& dest, const MC_LocChar* aSource)
{
	const size_t sourceLen = wcslen(aSource);
	int numChars = WideCharToMultiByte(CP_UTF8, 0, aSource, int(sourceLen), dest.GetBuffer(), dest.GetBufferSize()-1, NULL, NULL);
	if (numChars)
	{
		dest[numChars]=0;
	}
	else if (0 == numChars && sourceLen)
	{
		MC_DEBUG(" Failed to convert the following from UTF16 to UTF8 (errorcode: %d) :", GetLastError());
		MC_PrettyPrinter::ToDebug((const char*)((const MC_LocChar*)aSource), sourceLen*sizeof(MC_LocChar));
	}
}



#endif //__MDB_STRINGCONVERSION_H__
