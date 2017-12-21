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
#include "mdb_prettyprinter.h"
#include "mc_debug.h"
#include <ctype.h>

MC_String 
MDB_PrettyPrinter::ToString(const MDB_IPrettyPrintable& anObject) 
{ 
#ifdef _DEBUG
	return anObject.ToString(); 
#else
	return "";
#endif
}

void
MDB_PrettyPrinter::ToDebug(const char* theData, unsigned int theDatalen)
{
	MC_String result;
	MC_String lineDecode;
	unsigned int printed = 0;

	MC_Debug::DebugMessage("Printing data of len %d", theDatalen);

	while (printed<theDatalen)
	{
		if (printed%16 == 0)
		{
			MC_String newLine;
			if (result.GetLength() > 0)
			{
				MC_Debug::DebugMessage("%s [%s]", result, lineDecode);
				result = "";
				lineDecode = "";
			}
			newLine.Format("%8x: ", theData+printed);
			result += newLine;
		}
		MC_String val;
		val.Format("%s%2X", lineDecode.GetLength()?",":"", (unsigned char)theData[printed]);
		unsigned char v = ((unsigned char)theData[printed]);
		v = isprint(v) ? v:'.';
		lineDecode += (TCHAR)v;
		result += val;
		printed++;
	}
	for (unsigned int i=0; i<16-theDatalen % 16;i++)
		result += "   ";
	MC_Debug::DebugMessage("%s [%s]", result, lineDecode);
}

