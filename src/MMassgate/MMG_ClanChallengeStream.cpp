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
#include "MMG_ClanChallengeStream.h"
#include "MMG_Messaging.h"
#include "MMG_ServerProtocol.h"
#include "MMG_ProtocolDelimiters.h"

void 
MMG_MatchChallenge::ClanToClanReq::ToStream(MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_CLAN_MATCH_CHALLENGE_REQ); 
	theStream.WriteLocString(challengerClanName.GetBuffer(), challengerClanName.GetLength());
	theStream.WriteUInt64(mapHash); 
	theStream.WriteChar(useEslRules ? 1 : 0);
	theStream.WriteUInt(challengeID); 
	theStream.WriteUInt(challengerProfileID); 
	theStream.WriteUInt(challengedProfileID); 
	theStream.WriteUInt(allocatedServer);
	theStream.WriteUInt(serverPassword);	
	theStream.WriteUChar(canPassOn); 
	theStream.WriteUChar(challengerFaction);
	theStream.WriteUChar(challengedFaction);
	theStream.WriteUChar(maxPlayers); 
}

bool 
MMG_MatchChallenge::ClanToClanReq::FromStream(MN_ReadMessage& theStream)
{
	bool good = true; 

	good = good && theStream.ReadLocString(challengerClanName.GetBuffer(), challengerClanName.GetBufferSize());
	good = good && theStream.ReadUInt64(mapHash);
	char t; 
	good = good && theStream.ReadChar(t);
	if(good)
	{
		if(t)
			useEslRules = true; 
		else 
			useEslRules = false; 
	}
	good = good && theStream.ReadUInt(challengeID); 
	good = good && theStream.ReadUInt(challengerProfileID); 
	good = good && theStream.ReadUInt(challengedProfileID); 
	good = good && theStream.ReadUInt(allocatedServer);
	good = good && theStream.ReadUInt(serverPassword);	
	good = good && theStream.ReadUChar(canPassOn); 
	good = good && theStream.ReadUChar(challengerFaction);
	good = good && theStream.ReadUChar(challengedFaction);
	good = good && theStream.ReadUChar(maxPlayers); 
	
	return good; 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_MatchChallenge::ClanToClanRsp::ToStream(MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_CLAN_MATCH_CHALLENGE_RSP); 
	theStream.WriteUInt(challengeID); 
	theStream.WriteUInt(challengerProfile); 
	theStream.WriteUInt(allocatedServer); 
	theStream.WriteUInt(serverPassword); 
	theStream.WriteUChar(challengerFaction);
	theStream.WriteUChar(challengedFaction);
	unsigned char tmp = status; 
	theStream.WriteUChar(tmp);
}

bool 
MMG_MatchChallenge::ClanToClanRsp::FromStream(MN_ReadMessage& theStream)
{
	bool good = true; 

	good = good && theStream.ReadUInt(challengeID);
	good = good && theStream.ReadUInt(challengerProfile);
	good = good && theStream.ReadUInt(allocatedServer);
	good = good && theStream.ReadUInt(serverPassword); 
	good = good && theStream.ReadUChar(challengerFaction);
	good = good && theStream.ReadUChar(challengedFaction);
	unsigned char tmp; 
	good = good && theStream.ReadUChar(tmp);
	status = (MMG_MatchChallenge::ClanToClanRsp::Status) tmp; 

	return good; 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_MatchChallenge::ServerAllocationReq::ToStream(MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_REQUEST_CLAN_MATCH_SERVER_REQ);
	theStream.WriteUInt(challengeID); 
	theStream.WriteUChar(maxNumMaps); 
	theStream.WriteUInt(clanId);
	theStream.WriteUInt(profileId);
}

bool 
MMG_MatchChallenge::ServerAllocationReq::FromStream(MN_ReadMessage& theStream)
{
	bool good = true; 

	good = good && theStream.ReadUInt(challengeID);
	good = good && theStream.ReadUChar(maxNumMaps); 
	good = good && theStream.ReadUInt(clanId);
	good = good && theStream.ReadUInt(profileId);

	return good; 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_MatchChallenge::ServerAllocationRsp::ToStream(MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_REQUEST_CLAN_MATCH_SERVER_RSP);
	theStream.WriteUInt(challengeID);
	theStream.WriteUInt(allocatedServerId);
	unsigned char numMaps = mapHashes.Count();
	theStream.WriteUChar(numMaps);
	while (numMaps--)
		theStream.WriteUInt64(mapHashes[numMaps]);
}
bool 
MMG_MatchChallenge::ServerAllocationRsp::FromStream(MN_ReadMessage& theStream)
{
	bool good = true;
	good = good && theStream.ReadUInt(challengeID);
	good = good && theStream.ReadUInt(allocatedServerId);
	unsigned char numMaps;
	good = good && theStream.ReadUChar(numMaps);
	while(good && numMaps--)
	{
		unsigned __int64 hash;
		good = good && theStream.ReadUInt64(hash);
		mapHashes.Add(hash);
	}
	return good;
}


//////////////////////////////////////////////////////////////////////////

void
MMG_MatchChallenge::MatchSetup::ToStream(MN_WriteMessage& theStream, 
										 bool aReceiverIsDS) const 
{
	assert(numBytes < sizeof(data) && "invalid parameters"); 
	if(aReceiverIsDS)
		theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_DS_FINAL_INIT_FOR_MATCH_SERVER);
	else 
		theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_FINAL_INIT_FOR_MATCH_SERVER);
	theStream.WriteUInt(receiver);
	theStream.WriteUInt(password);
	theStream.WriteUInt(cookie); 
	theStream.WriteRawData(data, numBytes); 
}

bool 
MMG_MatchChallenge::MatchSetup::FromStream(MN_ReadMessage& theStream)
{
	bool good = true; 

	good = good && theStream.ReadUInt(receiver);
	good = good && theStream.ReadUInt(password);
	good = good && theStream.ReadUInt(cookie); 
	good = good && theStream.ReadRawData(data, sizeof(data), &numBytes); 

	return good; 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_MatchChallenge::MatchSetupAck::ToStream(MN_WriteMessage& theStream,
											bool aReceiverIsClient) const
{
	if(aReceiverIsClient)
		theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_FINAL_ACK_FROM_MATCH_SERVER);
	else 
		theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_DS_FINAL_ACK_FROM_MATCH_SERVER);
	theStream.WriteUInt(receiver);
	theStream.WriteUInt(cookie); 
	theStream.WriteUChar(statusCode);
}
bool 
MMG_MatchChallenge::MatchSetupAck::FromStream(MN_ReadMessage& theStream)
{
	bool good = true;
	good = good && theStream.ReadUInt(receiver);
	good = good && theStream.ReadUInt(cookie); 
	good = good && theStream.ReadUChar(statusCode);
	return good;
}


//////////////////////////////////////////////////////////////////////////

void 
MMG_MatchChallenge::PreloadMap::ToStream(MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_CLAN_MATCH_PRELOAD); 
	theStream.WriteUInt64(mapHash);
	theStream.WriteUInt(password);
	theStream.WriteChar(eslRules ? 1 : 0);
}

bool 
MMG_MatchChallenge::PreloadMap::FromStream(MN_ReadMessage& theStream)
{
	bool good = true;
	good = good && theStream.ReadUInt64(mapHash);
	good = good && theStream.ReadUInt(password);
	char t;
	good = good && theStream.ReadChar(t);
	if(good)
	{
		if(t)
			eslRules = true; 
		else 
			eslRules = false; 
	}
	return good;
}

//////////////////////////////////////////////////////////////////////////

void
MMG_MatchChallenge::InviteToClanMatchReq::ToStream(MN_WriteMessage& theStream) const 
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_MATCH_INVITE_REQ); 
	theStream.WriteUInt64(mapHash); 
	theStream.WriteUInt(myMatchType); 
	theStream.WriteUInt(myClanLeaderProfileID); 
	theStream.WriteUInt(opClanLeaderProfileID); 
	theStream.WriteUInt(receiver); 
	theStream.WriteUInt(challengeID); 
	theStream.WriteUInt(allocatedServerID); 
	theStream.WriteUInt(serverPassword); 
	theStream.WriteUInt(opClanID); 
	theStream.WriteUChar(myFaction); 
	theStream.WriteUChar(opFaction); 
	theStream.WriteUChar(maxPlayers); 
	theStream.WriteUChar(myClanStartedThis); 
	theStream.WriteLocString(opClanName.GetBuffer(), opClanName.GetLength()); 
}

bool
MMG_MatchChallenge::InviteToClanMatchReq::FromStream(MN_ReadMessage& theStream)
{
	bool good = true; 

	good = good && theStream.ReadUInt64(mapHash); 
	good = good && theStream.ReadUInt(myMatchType); 
	good = good && theStream.ReadUInt(myClanLeaderProfileID); 
	good = good && theStream.ReadUInt(opClanLeaderProfileID); 
	good = good && theStream.ReadUInt(receiver); 
	good = good && theStream.ReadUInt(challengeID); 
	good = good && theStream.ReadUInt(allocatedServerID); 
	good = good && theStream.ReadUInt(serverPassword); 
	good = good && theStream.ReadUInt(opClanID); 
	good = good && theStream.ReadUChar(myFaction); 
	good = good && theStream.ReadUChar(opFaction); 
	good = good && theStream.ReadUChar(maxPlayers); 
	good = good && theStream.ReadUChar(myClanStartedThis); 
	good = good && theStream.ReadLocString(opClanName.GetBuffer(), opClanName.GetBufferSize()); 

	return good; 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_MatchChallenge::InviteToTournamentMatchReq::ToStream(MN_WriteMessage& theStream) const
{
}

bool 
MMG_MatchChallenge::InviteToTournamentMatchReq::FromStream(MN_ReadMessage& theStream)
{
	return true; 
}

//////////////////////////////////////////////////////////////////////////

void
MMG_MatchChallenge::InviteToMatchRsp::ToStream(MN_WriteMessage& theStream) const 
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_MATCH_INVITE_RSP); 
	theStream.WriteUInt(receiver); 
	theStream.WriteUChar(accepted); 
	unsigned char tmp = (unsigned char) declineReason; 
	theStream.WriteUChar(tmp); 
}

bool
MMG_MatchChallenge::InviteToMatchRsp::FromStream(MN_ReadMessage& theStream)
{
	bool good = true; 

	good = good && theStream.ReadUInt(receiver);
	good = good && theStream.ReadUChar(accepted); 
	unsigned char tmp; 
	good = good && theStream.ReadUChar(tmp); 
	declineReason = (MMG_MatchChallenge::InviteToMatchRsp::DelineReason) tmp; 

	return good; 
}
