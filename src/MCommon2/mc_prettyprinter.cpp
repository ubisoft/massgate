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
#include "mc_prettyprinter.h"
#include "mc_debug.h"
#include <ctype.h>

MC_String 
MC_PrettyPrinter::ToString(const MDB_IPrettyPrintable& anObject) 
{ 
#ifdef _DEBUG
	return anObject.ToString(); 
#else
	return "";
#endif
}

void 
MC_PrettyPrinter::ToDebug(const char* theData, 
						  size_t theDatalen, 
						  const char *aTag)
{
	MC_Debug::DebugMessage("%s Printing data of len %d", aTag, theDatalen);

	for(size_t i = 0; i < theDatalen; i += 16)
	{
		size_t remaining; 
		MC_StaticString<128> hexLine, readableLine, tmp; 

		remaining = theDatalen - i; 
		if(remaining > 16)
		{
			remaining = 16; 
		}
		for(size_t p = 0; p < remaining; p++)
		{
			tmp.Format("%s%.2X", p == 0 ? "" : ",", (unsigned char)theData[i + p]);
			hexLine += tmp; 
			readableLine += isprint((unsigned char)theData[i + p]) ? (TCHAR)theData[i + p] : (TCHAR)'.';  
		}
		remaining = 16 - remaining; 
		for(size_t p = 0; p < remaining; p++)
		{
			hexLine += "   "; 
			readableLine += " "; 
		}
		MC_Debug::DebugMessage("%s 0x%.8X %s [%s]", aTag, theData + i, hexLine.GetBuffer(), readableLine.GetBuffer());
	}
}