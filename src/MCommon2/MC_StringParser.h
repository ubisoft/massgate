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

#ifndef MC_STRINGPARSER_H
#define MC_STRINGPARSER_H

#include "mc_string.h"

template<class StringType>
class MC_StringParserTemplate
{
public:
	MC_StringParserTemplate(const StringType& aString)
	{
		SetString(aString);
	}
	MC_StringParserTemplate()
	{
	}

	void SetString(const StringType& aString)
	{
		myString = aString;
		myString.TrimLeft().TrimRight();
	}

	const StringType& GetRest() const { return myString; }

	StringType Next()
	{
		static const MC_LocString spaceLocString = L" ";
		int charsToNext;
		StringType returnStr;

		charsToNext = myString.Find(spaceLocString);
		if (charsToNext >= 0)
		{
			returnStr = myString.Left(charsToNext);
			myString = myString.Mid(charsToNext + 1, myString.GetLength()+1 - charsToNext);
			myString.TrimLeft();
			return returnStr;
		}
		else
		{
			returnStr = myString;
			myString.Clear();
			return returnStr;
		}

	}

private:
	StringType myString;
};

typedef MC_StringParserTemplate<MC_String> MC_StringParser;
typedef MC_StringParserTemplate<MC_LocString> MC_StringParserLoc;

#endif // end #ifndef MC_STRINGPARSER_H
