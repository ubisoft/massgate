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
#include "MMG_DecorationProtocol.h"
#include "MMG_ServerProtocol.h"
#include "MMG_Protocols.h"
#include "MMG_BitStream.h"
#include "MMG_ProtocolDelimiters.h"

void 
MMG_DecorationProtocol::PlayerMedalsReq::ToStream(MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_MEDALS_REQ); 
	theStream.WriteUInt(profileId);
}

bool 
MMG_DecorationProtocol::PlayerMedalsReq::FromStream(MN_ReadMessage& theStream)
{
	bool good = true; 

	good = good && theStream.ReadUInt(profileId); 

	return good; 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_DecorationProtocol::PlayerMedalsRsp::Add(unsigned short aMedalLevel, 
									   unsigned short aNumStars)
{
	medals.Add(Medal(aMedalLevel, aNumStars));
}

void 
MMG_DecorationProtocol::PlayerMedalsRsp::ToStream(MN_WriteMessage& theStream) const 
{
	unsigned int buffer[256] = {0};
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_MEDALS_RSP);
	theStream.WriteUInt(profileId);
	theStream.WriteUInt(medals.Count());
	MMG_BitWriter<unsigned int> bw(buffer, sizeof(buffer)*8);
	for(int i = 0; i < medals.Count(); i++)
	{
		bw.WriteBits(medals[i].medalLevel, 2);
		bw.WriteBits(medals[i].numStars, 2);
	}
	theStream.WriteRawData(buffer, bw.Tell()/8+1);
}

bool
MMG_DecorationProtocol::PlayerMedalsRsp::FromStream(MN_ReadMessage& theStream)
{
	bool good = true;
	unsigned int count;

	unsigned int buffer[256]={0};
	
	good = good && theStream.ReadUInt(profileId); 
	good = good && theStream.ReadUInt(count); 
	good = good && theStream.ReadRawData(buffer, sizeof(buffer));

	MMG_BitReader<unsigned int> br(buffer, sizeof(buffer)*8);
	for(unsigned int i = 0; good && i < count; i++)
	{
		unsigned int medalLevel = 0;
		unsigned int numStars = 0;
		medalLevel = br.ReadBits(2);
		numStars = br.ReadBits(2);
		medals.Add(Medal(medalLevel, numStars));
	}
	return good; 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_DecorationProtocol::PlayerBadgesReq::ToStream(MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_BADGES_REQ);
	theStream.WriteUInt(profileId);
}

bool 
MMG_DecorationProtocol::PlayerBadgesReq::FromStream(MN_ReadMessage& theStream)
{
	bool good = true;

	good = good && theStream.ReadUInt(profileId); 

	return good; 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_DecorationProtocol::PlayerBadgesRsp::Add(unsigned short aBadgeLevel, 
											 unsigned short aNumStars)
{
	badges.Add(Badge(aBadgeLevel, aNumStars));
}

void 
MMG_DecorationProtocol::PlayerBadgesRsp::ToStream(MN_WriteMessage& theStream) const 
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_BADGES_RSP);
	theStream.WriteUInt(profileId);
	theStream.WriteUInt(badges.Count());

	unsigned int buffer[256]={0};

	MMG_BitWriter<unsigned int> bw(buffer, sizeof(buffer)*8);

	for(int i = 0; i < badges.Count(); i++)
	{
		bw.WriteBits(badges[i].badgeLevel, 2);
		bw.WriteBits(badges[i].numStars, 2);
	}
	theStream.WriteRawData(buffer, bw.Tell()/8+1);
}

bool
MMG_DecorationProtocol::PlayerBadgesRsp::FromStream(MN_ReadMessage& theStream)
{
	bool good = true;
	unsigned int count;
	unsigned int buffer[256]={0};

	good = good && theStream.ReadUInt(profileId); 
	good = good && theStream.ReadUInt(count); 
	good = good && theStream.ReadRawData(buffer, sizeof(buffer));

	MMG_BitReader<unsigned int> br(buffer, sizeof(buffer)*8);

	for(unsigned int i = 0; good && i < count; i++)
	{
		unsigned int level = 0;
		unsigned int stars = 0;
		level = br.ReadBits(2);
		stars = br.ReadBits(2);
		badges.Add(Badge(level, stars));
	}
	return good; 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_DecorationProtocol::ClanMedalsReq::ToStream(MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_MEDALS_REQ); 
	theStream.WriteUInt(clanId);
}

bool 
MMG_DecorationProtocol::ClanMedalsReq::FromStream(MN_ReadMessage& theStream)
{
	bool good = true; 

	good = good && theStream.ReadUInt(clanId); 

	return good; 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_DecorationProtocol::ClanMedalsRsp::Add(unsigned short aMedalLevel, 
											 unsigned short aNumStars)
{
	medals.Add(Medal(aMedalLevel, aNumStars));
}

void 
MMG_DecorationProtocol::ClanMedalsRsp::ToStream(MN_WriteMessage& theStream) const 
{
	unsigned int buffer[256]={0};

	theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_MEDALS_RSP);
	theStream.WriteUInt(clanId);
	theStream.WriteUInt(medals.Count());

	MMG_BitWriter<unsigned int> bw(buffer, sizeof(buffer)*8);

	for(int i = 0; i < medals.Count(); i++)
	{
		bw.WriteBits(medals[i].medalLevel, 2);
		bw.WriteBits(medals[i].numStars, 2);
	}

	theStream.WriteRawData(buffer, bw.Tell()/8+1);

}

bool
MMG_DecorationProtocol::ClanMedalsRsp::FromStream(MN_ReadMessage& theStream)
{
	unsigned int buffer[256]={0};

	bool good = true;
	unsigned int count;

	good = good && theStream.ReadUInt(clanId); 
	good = good && theStream.ReadUInt(count); 
	good = good && theStream.ReadRawData(buffer, sizeof(buffer));

	MMG_BitReader<unsigned int> br(buffer, sizeof(buffer)*8);

	for(unsigned int i = 0; good && i < count; i++)
	{
		unsigned int level = 0;
		unsigned int stars = 0;
		level = br.ReadBits(2);
		stars = br.ReadBits(2);
		medals.Add(Medal(level,stars));
	}
	return good; 
}
