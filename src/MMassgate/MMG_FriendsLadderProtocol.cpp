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
#include "MMG_FriendsLadderProtocol.h"
#include "MMG_ServerProtocol.h"
#include "MMG_Protocols.h"
#include "MC_Debug.h"
#include "MMG_ProtocolDelimiters.h"

MMG_FriendsLadderProtocol::FriendsLadderReq::FriendsLadderReq()
{
	myFriendsProfileIds.Init(32, 32); 
}

void 
MMG_FriendsLadderProtocol::FriendsLadderReq::ToStream(MN_WriteMessage& theStream) const
{
	if(myFriendsProfileIds.Count() > (512+128))
	{
		MC_DEBUG("too many friends in this FriendsLadderReq, bailing out"); 
		return; 
	}

	theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_FRIENDS_LADDER_GET_REQ); 
	theStream.WriteUInt(requestId); 
	theStream.WriteUInt(myFriendsProfileIds.Count()); 
	for(int i = 0; i < myFriendsProfileIds.Count(); i++)
		theStream.WriteUInt(myFriendsProfileIds[i]); 
}

bool 
MMG_FriendsLadderProtocol::FriendsLadderReq::FromStream(MN_ReadMessage& theStream)
{
	unsigned int numFriends; 
	bool good = true;

	good = good && theStream.ReadUInt(requestId);
	good = good && theStream.ReadUInt(numFriends); 
	
	if(good && numFriends > (512+128))
	{
		MC_DEBUG("Someone tried to request more than max friends+clanmates entries from the friends ladder"); 
		return false; // possible DOS 
	}
	
	for(unsigned int i = 0; good && i < numFriends; i++)
	{
		unsigned int friendsProfileId; 
		good = theStream.ReadUInt(friendsProfileId); 
		if(good)
			myFriendsProfileIds.AddUnique(friendsProfileId); 
	}

	return good; 
}

void 
MMG_FriendsLadderProtocol::FriendsLadderReq::Add(unsigned int aProfileId)
{
	myFriendsProfileIds.AddUnique(aProfileId); 
}

//////////////////////////////////////////////////////////////////////////

MMG_FriendsLadderProtocol::FriendsLadderRsp::FriendsLadderRsp()
{
	myFriendsLadderData.Init(32, 32); 
}

void 
MMG_FriendsLadderProtocol::FriendsLadderRsp::ToStream(MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_FRIENDS_LADDER_GET_RSP); 
	theStream.WriteUInt(requestId);
	unsigned int totalFriends = myFriendsLadderData.Count();
	unsigned int numFriendsInThisBatch = min(500, totalFriends);
	theStream.WriteUInt(numFriendsInThisBatch); 
	
	unsigned int writesInThisBatch = 0; 
	for(unsigned int i = 0; i < totalFriends; i++, writesInThisBatch++)
	{
		if(writesInThisBatch == numFriendsInThisBatch)
		{
			theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_FRIENDS_LADDER_GET_RSP); 
			theStream.WriteUInt(requestId);
			numFriendsInThisBatch = min(500, totalFriends - i);
			theStream.WriteUInt(numFriendsInThisBatch); 		
			writesInThisBatch = 0; 
		}

		theStream.WriteUInt(myFriendsLadderData[i].myProfileId); 
		theStream.WriteUInt(myFriendsLadderData[i].myScore); 
	}
}

bool 
MMG_FriendsLadderProtocol::FriendsLadderRsp::FromStream(MN_ReadMessage& theStream)
{
	unsigned int numFriends; 
	bool good = true; 
	
	good = good && theStream.ReadUInt(requestId);
	good = good && theStream.ReadUInt(numFriends);

	for(unsigned int i = 0; good && i < numFriends; i++)
	{
		unsigned int profileId; 
		unsigned int score; 
		good = good && theStream.ReadUInt(profileId); 
		good = good && theStream.ReadUInt(score); 
		if(good)
			Add(profileId, score); 
	}

	return good; 
}

void 
MMG_FriendsLadderProtocol::FriendsLadderRsp::Sort(
	MC_GrowingArray<LadderEntry>& aFriendsLadderData)
{
	class LadderItemComparer
	{
	public: 
		static bool LessThan(const LadderEntry& a, 
			const LadderEntry& b) 
		{ 
			return a.myScore > b.myScore;
		}
		static bool Equals(const LadderEntry& a, 
			const LadderEntry& b)
		{ 
			return a.myScore == b.myScore; 
		}
	};

	aFriendsLadderData.Sort<LadderItemComparer>(); 
}

void 
MMG_FriendsLadderProtocol::FriendsLadderRsp::Add(unsigned int aProfileId, 
												 unsigned int aScore)
{
	LadderEntry tmp; 
	tmp.myProfileId = aProfileId; 
	tmp.myScore = aScore; 
	myFriendsLadderData.Add(tmp); 
}

