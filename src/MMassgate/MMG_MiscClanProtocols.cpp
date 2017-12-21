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
#include "MMG_MiscClanProtocols.h"
#include "MMG_ServerProtocol.h"
#include "MMG_Protocols.h"
#include "MMG_ProtocolDelimiters.h"

void 
MMG_MiscClanProtocols::TopTenWinningStreakReq::ToStream(MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_LADDER_TOPTEN_REQ); 
}

bool 
MMG_MiscClanProtocols::TopTenWinningStreakReq::FromStream(MN_ReadMessage& theStream)
{
	return true; 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_MiscClanProtocols::TopTenWinningStreakRsp::ToStream(MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_LADDER_TOPTEN_RSP); 
	theStream.WriteUInt(myStreakData.Count()); 
	for(int i = 0; i < myStreakData.Count(); i++)
	{
		theStream.WriteUInt(myStreakData[i].myClanId); 
		theStream.WriteUInt(myStreakData[i].myStreak); 
		theStream.WriteLocString(myStreakData[i].myClanName.GetBuffer(), myStreakData[i].myClanName.GetLength()); 
	}
}

bool 
MMG_MiscClanProtocols::TopTenWinningStreakRsp::FromStream(MN_ReadMessage& theStream)
{
	unsigned int numClanIds; 
	bool good = theStream.ReadUInt(numClanIds); 
	for(unsigned int i = 0; good && i < numClanIds; i++)
	{
		unsigned int clanId, streak; 
		MMG_ClanFullNameString clanName; 
		good = good && theStream.ReadUInt(clanId);
		good = good && theStream.ReadUInt(streak);
		good = good && theStream.ReadLocString(clanName.GetBuffer(), clanName.GetBufferSize()); 
		if(good)
			Add(clanId, clanName, streak); 
	}
	return good; 
}

void 
MMG_MiscClanProtocols::TopTenWinningStreakRsp::Add(unsigned int aClanId, 
												   MMG_ClanFullNameString aClanName,
												   unsigned int aStreak)
{
	StreakData tmp; 
	tmp.myClanId = aClanId; 
	tmp.myClanName = aClanName; 
	tmp.myStreak = aStreak;
	myStreakData.Add(tmp); 
}
