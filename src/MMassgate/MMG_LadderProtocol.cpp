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
#include "MMG_LadderProtocol.h"
#include "MMG_ServerProtocol.h"
#include "MMG_Protocols.h"
#include "MMG_ProtocolDelimiters.h"

void 
MMG_LadderProtocol::LadderReq::ToStream(MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_LADDER_GET_REQ); 
	theStream.WriteUInt(startPos); 
	theStream.WriteUInt(profileId); 
	theStream.WriteUInt(numItems); 
	theStream.WriteUInt(requestId); 
}

bool 
MMG_LadderProtocol::LadderReq::FromStream(MN_ReadMessage& theStream)
{
	bool good = true; 

	good = good && theStream.ReadUInt(startPos); 
	good = good && theStream.ReadUInt(profileId); 
	good = good && theStream.ReadUInt(numItems); 
	good = good && theStream.ReadUInt(requestId); 

	return good; 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_LadderProtocol::LadderRsp::ToStream(MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_LADDER_GET_RSP); 
	theStream.WriteUChar(1); // basic data coming

	theStream.WriteUInt(startPos);
	theStream.WriteUInt(requestId); 
	theStream.WriteUInt(ladderSize); 
	theStream.WriteUInt(ladderItems.Count()); 
	for(int i = 0; i < ladderItems.Count(); i++)
	{
		theStream.WriteUInt(ladderItems[i].score); 
	}
	int profilesWrittenInChunk = 0;
	for (int i = 0; i < ladderItems.Count(); i++)
	{
		if (profilesWrittenInChunk == 0)
		{
			theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_LADDER_GET_RSP); 
			theStream.WriteUChar(2); // profileInfo coming
			theStream.WriteUInt(__min(35,ladderItems.Count()-i));
		}
		ladderItems[i].profile.ToStream(theStream);
		profilesWrittenInChunk++;
		if (profilesWrittenInChunk == 35)
		{
			profilesWrittenInChunk = 0;
		}
	}
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_LADDER_GET_RSP); 
	theStream.WriteUChar(3); // all data sent
}

bool 
MMG_LadderProtocol::LadderRsp::FromStream(MN_ReadMessage& theStream)
{
	myResponseIsFull = false;
	unsigned int numItems; 
	bool good = true; 
	unsigned char chunk = 0;
	good = good && theStream.ReadUChar(chunk);
	if (good && chunk == 1)
	{
		good = good && theStream.ReadUInt(startPos); 
		good = good && theStream.ReadUInt(requestId);
		good = good && theStream.ReadUInt(ladderSize);
		good = good && theStream.ReadUInt(numItems); 
		ladderItems.RemoveAll();
		for(unsigned int i = 0; good && i < numItems; i++)
		{
			MMG_Profile temp;
			temp.myProfileId = -1;
			unsigned int score; 
			good = good && theStream.ReadUInt(score); 
			if(good)
				ladderItems.Add(LadderItem(temp, score)); 
		}
	}
	else if (good && chunk == 2)
	{
		int lastWritten=0;
		for (; (ladderItems[lastWritten].profile.myProfileId != -1) && (lastWritten<ladderItems.Count()); lastWritten++)
			;
		unsigned int numProfilesInChunk = 0;
		good = good && theStream.ReadUInt(numProfilesInChunk);
		for (unsigned int i = 0; good && i < numProfilesInChunk; i++)
			good = good && ladderItems[lastWritten+i].profile.FromStream(theStream);

	}
	else if (good && chunk == 3)
	{
		myResponseIsFull = true;
	}
	return good; 
}

void 
MMG_LadderProtocol::LadderRsp::Add(const MMG_Profile& aProfile, 
								   unsigned int aScore)
{
	ladderItems.Add(LadderItem(aProfile, aScore)); 
}

