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
#include "MMG_ClanNamesProtocol.h"
#include "MMG_ProtocolDelimiters.h"

void 
MMG_ClanNamesProtocol::GetRsp::ToStream(
	MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_NAMES_RSP); 
	theStream.WriteUInt(clanNames.Count()); 

	for(int i = 0; i < clanNames.Count(); i++)
	{
		theStream.WriteUInt(clanNames[i].clanId); 
		theStream.WriteLocString(clanNames[i].clanName.GetBuffer(), clanNames[i].clanName.GetLength()); 
	}
}

bool 
MMG_ClanNamesProtocol::GetRsp::FromStream(
	MN_ReadMessage& theStream)
{
	bool good = true; 
	unsigned int numClans; 

	good = good && theStream.ReadUInt(numClans);
	for(unsigned int i = 0; good && i < numClans; i++)
	{
		MMG_ClanName clan; 

		good = good && theStream.ReadUInt(clan.clanId); 
		good = good && theStream.ReadLocString(clan.clanName.GetBuffer(), clan.clanName.GetBufferSize()); 
		
		if(good)
			clanNames.Add(clan);
	}

	return good; 
}
