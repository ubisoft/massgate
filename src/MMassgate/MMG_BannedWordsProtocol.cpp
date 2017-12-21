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
#include "MMG_BannedWordsProtocol.h"
#include "MMG_ProtocolDelimiters.h"


void 
MMG_BannedWordsProtocol::GetReq::ToStream(
	MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_DS_GET_BANNED_WORDS_REQ); 
}

bool 
MMG_BannedWordsProtocol::GetReq::FromStream(
	MN_ReadMessage& theStream)
{
	return true; 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_BannedWordsProtocol::GetRsp::ToStream(
	MN_WriteMessage& theStream) const
{
	for(int i = 0; i < myBannedStrings.Count(); i++)
	{
		if(i % 15 == 0)
		{
			theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_DS_GET_BANNED_WORDS_RSP);
			unsigned stringsLeft = myBannedStrings.Count() - i; 
			theStream.WriteUInt(__min(stringsLeft, 15)); 
		}

		theStream.WriteLocString(myBannedStrings[i].GetBuffer(), myBannedStrings[i].GetLength()); 
	}
}

bool 
MMG_BannedWordsProtocol::GetRsp::FromStream(
	MN_ReadMessage& theStream)
{
	unsigned int numStrings; 
	bool good = true; 

	good = theStream.ReadUInt(numStrings); 
	for(unsigned int i = 0; good && i < numStrings; i++)
	{
		MC_StaticLocString<255> bannedWord; 
		good = theStream.ReadLocString(bannedWord.GetBuffer(), 255); 
		if(good)
			myBannedStrings.AddUnique(bannedWord.GetBuffer()); 
	}

	return good; 
}

void 
MMG_BannedWordsProtocol::GetRsp::Add(
	MC_LocString& aString)
{
	if(aString.GetLength() > 255)
		return; 
	myBannedStrings.AddUnique(aString.GetBuffer()); 
}

