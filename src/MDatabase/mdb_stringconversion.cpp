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
#include "mdb_stringconversion.h"

#include "mc_debug.h"
#include "mc_prettyprinter.h"




// Get a string containing only printable characters. Works for all character sets. 
// Returns pointer to a per-thread temporary storage. Copy or compare immediately upon return!
// Maximum stringlength 255 characters.
const MC_LocChar* 
getNormalizedString(const MC_LocChar* theString)
{
	const unsigned int MAXSTRINGSIZE = 255;
	__declspec(thread) static MC_LocChar retString[MAXSTRINGSIZE+1];
	retString[0] = 0;

	int d = 0;
	int s = 0;

	for (; theString[s] && (d < MAXSTRINGSIZE);)
	{
		const MC_LocChar c = theString[s++];
		if (iswprint(c))
			retString[d++] = c;
	}
	retString[d] = 0;
	return retString;
}

