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
#ifndef MC_TOKENIZER_H__
#define MC_TOKENIZER_H__

inline const char* MC_Strchr(const char* aString, char aChar)
{
	while (*aString)
	{
		if (*aString == aChar)
			return aString;
		aString++;
	}
	return 0;
}

class MC_Tokenizer
{
	MC_String myStringData;
	MC_GrowingArray<const char*> myTokens;

public:
	bool Init(const char* aSourceString, const char* aWhitespaceString = " \n\r\t");
	bool InitInternal(const char* aWhitespaceString);
	const char* operator[](int anIndex) const;
	int Count() const;

	MC_Tokenizer();
	MC_Tokenizer(const char* aSourceString, const char* aWhitespaceString = " \n\r\t");
};

inline MC_Tokenizer::MC_Tokenizer()
: myStringData()
, myTokens(0, 32, false)
{
}

inline MC_Tokenizer::MC_Tokenizer(const char* aSourceString, const char* aWhitespaceString)
: myStringData()
, myTokens(0, 32, false)
{
	Init(aSourceString, aWhitespaceString);
}

inline bool MC_Tokenizer::Init(const char* aSourceString, const char* aWhitespaceString)
{
	myStringData = aSourceString;
	return InitInternal(aWhitespaceString);
}

inline bool MC_Tokenizer::InitInternal(const char* aWhitespaceString)
{
	myTokens.RemoveAll();

	const int len = myStringData.GetLength();
	for (int i = 0; i < len; ++i)
	{
		bool added = false;
		while ((i < len) && MC_Strchr(aWhitespaceString, myStringData[i]) == 0)
		{
			if (!added)
			{
				added = true;
				myTokens.Add(&myStringData[i]);
			}
			++i;
		}
		myStringData[i] = 0;
	}

	return true;
}

inline const char* MC_Tokenizer::operator[](int anIndex) const
{
	if (anIndex < 0 || anIndex >= Count())
		return "";
	assert(anIndex >= 0 && anIndex < Count());
	return myTokens[anIndex];
}

inline int MC_Tokenizer::Count() const
{
	return myTokens.Count();
}

#endif//MC_TOKENIZER_H__