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
#include "MMG_CanPlayClanMatchProtocol.h"

#include "MMG_ProtocolDelimiters.h"

void
MMG_CanPlayClanMatchProtocol::CanPlayClanMatchReq::ToStream(
	MN_WriteMessage&	theStream)
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CAN_PLAY_CLANMATCH_REQ);

	unsigned int numProfileIds = profileIds.Count(); 

	if((numProfileIds * sizeof(unsigned char) + sizeof(unsigned int) * 3) > MMG_MAX_BUFFER_SIZE)
		return; 

	theStream.WriteUInt(requestId);
	theStream.WriteUInt(numProfileIds);
	for(unsigned int i = 0; i < numProfileIds; i++)
		theStream.WriteUInt(profileIds[i]);
}
	
bool
MMG_CanPlayClanMatchProtocol::CanPlayClanMatchReq::FromStream(
	MN_ReadMessage&		theStream)
{
	bool good = true; 
	unsigned int numProfileIds; 

	good = good && theStream.ReadUInt(requestId);
	good = good && theStream.ReadUInt(numProfileIds);
	for(unsigned int i = 0; good && i < numProfileIds; i++)
	{
		unsigned int profileId; 
		good = theStream.ReadUInt(profileId);
		if(good)
			profileIds.AddUnique(profileId); 
	}

	return good; 
}

//////////////////////////////////////////////////////////////////////////

void
MMG_CanPlayClanMatchProtocol::CanPlayClanMatchRsp::ToStream(
	MN_WriteMessage&	theStream)
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CAN_PLAY_CLANMATCH_RSP);

	unsigned int numProfiles = canPlayInformation.Count();

	if((numProfiles * sizeof(unsigned char) + sizeof(unsigned int) * 3) > MMG_MAX_BUFFER_SIZE)
		return; 

	theStream.WriteUInt(requestId);
	theStream.WriteUInt(timeLimit);
	theStream.WriteUInt(numProfiles);

	for(unsigned int i = 0; i < numProfiles; i++)
	{
		theStream.WriteUInt(canPlayInformation[i].profileId);
		theStream.WriteBool(canPlayInformation[i].canPlay);
	}
}

bool
MMG_CanPlayClanMatchProtocol::CanPlayClanMatchRsp::FromStream(
	MN_ReadMessage&		theStream)
{
	bool good = true;
	unsigned int numProfiles; 

	good = good && theStream.ReadUInt(requestId);
	good = good && theStream.ReadUInt(timeLimit);
	good = good && theStream.ReadUInt(numProfiles);
	
	for(unsigned int i = 0; good && i < numProfiles; i++)
	{
		unsigned int profileId; 
		bool canPlay; 
		good = good && theStream.ReadUInt(profileId);
		good = good && theStream.ReadBool(canPlay);

		if(good)
			Add(profileId, canPlay);
	}

	return good;
}




