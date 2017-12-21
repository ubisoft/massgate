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
#include "MMG_ClanMatchHistoryProtocol.h"
#include "MMG_ProtocolDelimiters.h"

void 
MMG_ClanMatchHistoryProtocol::GetListingReq::ToStream(
	MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_MATCH_HISTORY_LISTING_REQ); 
	theStream.WriteUInt(clanId); 
	theStream.WriteUInt(requestId);
}

bool 
MMG_ClanMatchHistoryProtocol::GetListingReq::FromStream(
	MN_ReadMessage& theStream)
{
	bool good = true;

	good = good && theStream.ReadUInt(clanId); 
	good = good && theStream.ReadUInt(requestId);

	return good; 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_ClanMatchHistoryProtocol::GetListingRsp::ToStream(
	MN_WriteMessage& theStream) const
{
	if(!matches.Count())
	{
		theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_MATCH_HISTORY_LISTING_RSP); 
		theStream.WriteUInt(requestId);
		theStream.WriteInt(0); 
		return;
	}

	for(int i = 0; i < matches.Count(); i++)
	{
		if(i % 12 == 0)
		{
			theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_MATCH_HISTORY_LISTING_RSP); 
			theStream.WriteUInt(requestId);
			theStream.WriteInt(__min(matches.Count() - i, 12));		
		}

		Match match = matches[i]; 

		theStream.WriteUInt(match.matchId);
		theStream.WriteUInt(match.winnerClan); 
		theStream.WriteUInt(match.loserClan); 
		theStream.WriteUInt(match.datePlayed); 
		theStream.WriteLocString(match.mapName.GetBuffer(), match.mapName.GetLength()); 		
	}
}

bool 
MMG_ClanMatchHistoryProtocol::GetListingRsp::FromStream(
	MN_ReadMessage& theStream)
{
	bool good = true; 
	int numMatches; 

	good = good && theStream.ReadUInt(requestId); 
	good = good && theStream.ReadInt(numMatches);

	for(int i = 0; good && i < numMatches; i++)
	{
		Match match;

		good = good && theStream.ReadUInt(match.matchId); 
		good = good && theStream.ReadUInt(match.winnerClan); 
		good = good && theStream.ReadUInt(match.loserClan); 
		good = good && theStream.ReadUInt(match.datePlayed); 
		good = good && theStream.ReadLocString(match.mapName.GetBuffer(), match.mapName.GetBufferSize()); 

		if(good)
			matches.Add(match);
	}

	return good; 
}

void 
MMG_ClanMatchHistoryProtocol::GetListingRsp::AddMatch(
	MMG_ClanMatchHistoryProtocol::GetListingRsp::Match& aMatch)
{
	matches.Add(aMatch);	
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_ClanMatchHistoryProtocol::GetDetailedReq::ToStream(
	MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_MATCH_HISTORY_DETAILS_REQ); 
	theStream.WriteUInt(matchId); 
}

bool 
MMG_ClanMatchHistoryProtocol::GetDetailedReq::FromStream(
	MN_ReadMessage& theStream)
{
	return theStream.ReadUInt(matchId); 
}


//////////////////////////////////////////////////////////////////////////

void 
MMG_ClanMatchHistoryProtocol::GetDetailedRsp::ToStream(
	MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_MATCH_HISTORY_DETAILS_RSP);
	theStream.WriteUInt(matchId); 
	theStream.WriteUInt(datePlayed);
	theStream.WriteLocString(mapName.GetBuffer(), mapName.GetLength()); 
	theStream.WriteUInt(winnerClan); 
	theStream.WriteUInt(loserClan); 
	theStream.WriteUShort(winnerScore); 
	theStream.WriteUShort(loserScore); 
	theStream.WriteUChar(winnerFaction);
	theStream.WriteUChar(loserFaction);
	theStream.WriteUShort(winnerCriticalStrikes); 
	theStream.WriteUShort(loserCriticalStrikes); 
	theStream.WriteUChar(gameMode); 

	if(gameMode == 0) // domination
	{
		theStream.WriteUShort(winnerDominationPercentage);
		theStream.WriteUShort(loserDominationPercentage);
	}
	else if(gameMode == 1) // assault 
	{
		theStream.WriteUChar(winnerAssaultNumCommandPoints);
		theStream.WriteUInt(winnerAssaultEndTime); 
		theStream.WriteUChar(loserAssaultNumCommandPoints);
		theStream.WriteUInt(loserAssaultEndTime); 
	}
	else if(gameMode == 2) // tug of war
	{
		theStream.WriteUChar(winnerTowNumLinesPushed); 
		theStream.WriteUChar(winnerTowNumPerimiterPoints); 
		theStream.WriteUChar(loserTowNumLinesPushed); 
		theStream.WriteUChar(loserTowNumPerimiterPoints); 	
	}

	theStream.WriteUShort(timeUsed);

	theStream.WriteUChar(winners.Count()); 
	for(int i = 0; i < winners.Count(); i++)
	{
		theStream.WriteUInt(winners[i].profileId); 
		theStream.WriteUShort(winners[i].score); 
		theStream.WriteUChar(winners[i].role); 
	}

	theStream.WriteUChar(losers.Count()); 
	for(int i = 0; i < losers.Count(); i++)
	{
		theStream.WriteUInt(losers[i].profileId); 
		theStream.WriteUShort(losers[i].score); 
		theStream.WriteUChar(losers[i].role); 
	}
}

bool 
MMG_ClanMatchHistoryProtocol::GetDetailedRsp::FromStream(
	MN_ReadMessage& theStream)
{
	bool good = true; 

	good = good && theStream.ReadUInt(matchId); 
	good = good && theStream.ReadUInt(datePlayed); 
	good = good && theStream.ReadLocString(mapName.GetBuffer(), mapName.GetBufferSize()); 
	good = good && theStream.ReadUInt(winnerClan); 
	good = good && theStream.ReadUInt(loserClan); 
	good = good && theStream.ReadUShort(winnerScore); 
	good = good && theStream.ReadUShort(loserScore); 
	good = good && theStream.ReadUChar(winnerFaction);
	good = good && theStream.ReadUChar(loserFaction);
	good = good && theStream.ReadUShort(winnerCriticalStrikes); 
	good = good && theStream.ReadUShort(loserCriticalStrikes); 
	good = good && theStream.ReadUChar(gameMode); 

	if(good)
	{
		if(gameMode == 0) // domination
		{
			good = good && theStream.ReadUShort(winnerDominationPercentage);
			good = good && theStream.ReadUShort(loserDominationPercentage);
		}
		else if(gameMode == 1) // assault 
		{
			good = good && theStream.ReadUChar(winnerAssaultNumCommandPoints);
			good = good && theStream.ReadUInt(winnerAssaultEndTime);
			good = good && theStream.ReadUChar(loserAssaultNumCommandPoints);
			good = good && theStream.ReadUInt(loserAssaultEndTime);
		}
		else if(gameMode == 2)
		{
			good = good && theStream.ReadUChar(winnerTowNumLinesPushed); 
			good = good && theStream.ReadUChar(winnerTowNumPerimiterPoints); 
			good = good && theStream.ReadUChar(loserTowNumLinesPushed); 
			good = good && theStream.ReadUChar(loserTowNumPerimiterPoints); 
		}
	}

	good = good && theStream.ReadUShort(timeUsed);

	unsigned char numPlayers; 
	
	good = good && theStream.ReadUChar(numPlayers); 
	for(unsigned char i = 0; good && i < numPlayers; i++)
	{
		PerPlayerData winner; 
	
		good = good && theStream.ReadUInt(winner.profileId); 
		good = good && theStream.ReadUShort(winner.score); 
		good = good && theStream.ReadUChar(winner.role);

		if(good)
			AddWinner(winner); 
	}

	good = good && theStream.ReadUChar(numPlayers); 
	for(unsigned char i = 0; good && i < numPlayers; i++)
	{
		PerPlayerData loser; 

		good = good && theStream.ReadUInt(loser.profileId); 
		good = good && theStream.ReadUShort(loser.score); 
		good = good && theStream.ReadUChar(loser.role);

		if(good)
			AddLoser(loser); 
	}
	
	return good; 
}

void 
MMG_ClanMatchHistoryProtocol::GetDetailedRsp::AddWinner(
	PerPlayerData& aWinner)
{
	winners.Add(aWinner); 
}

void 
MMG_ClanMatchHistoryProtocol::GetDetailedRsp::AddLoser(
	PerPlayerData& aLoser)
{
	losers.Add(aLoser); 
}
