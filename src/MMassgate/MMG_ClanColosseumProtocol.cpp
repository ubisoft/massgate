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
#include "MMG_ClanColosseumProtocol.h"
#include "MMG_ProtocolDelimiters.h"

namespace MMG_ClanColosseumProtocol
{

	void
	Filter::ToStream(
		MN_WriteMessage&	theStream)
	{
		theStream.WriteChar(useEsl);
		theStream.WriteInt(fromRanking);
		theStream.WriteInt(toRanking);
		theStream.WriteUChar(playerMask);
		theStream.WriteInt(maps.Count());
		for(int i = 0; i < maps.Count(); i++)
		{
			theStream.WriteUInt64(maps[i]);
		}
	}

	bool
	Filter::FromStream(
		MN_ReadMessage&		theStream)
	{
		bool			good = true; 
		int				mapCount;
		
		good = good && theStream.ReadChar(useEsl);
		good = good && theStream.ReadInt(fromRanking);
		good = good && theStream.ReadInt(toRanking);
		good = good && theStream.ReadUChar(playerMask);
		good = good && theStream.ReadInt(mapCount);

		for(int i = 0; good && i < mapCount; i++)
		{
			uint64		mapHash;
			good = good && theStream.ReadUInt64(mapHash);
	
			if(good)
				maps.Add(mapHash);
		}

		return good;
	}

	//////////////////////////////////////////////////////////////////////////

	void
	FilterWeightsReq::ToStream(
		MN_WriteMessage&	theStream)
	{
		theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_COLOSSEUM_GET_FILTER_WEIGHTS_REQ); 
	}

	bool
	FilterWeightsReq::FromStream(
		MN_ReadMessage&		theStream)
	{
		return true; 
	}

	//////////////////////////////////////////////////////////////////////////

	void
	FilterWeightsRsp::ToStream(
		MN_WriteMessage&	theStream)
	{
		theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_COLOSSEUM_GET_FILTER_WEIGHTS_RSP);
		theStream.WriteFloat(myFilterWeights.myClanWarsHaveMapWeight); 
		theStream.WriteFloat(myFilterWeights.myClanWarsDontHaveMapWeight); 
		theStream.WriteFloat(myFilterWeights.myClanWarsPlayersWeight); 
		theStream.WriteFloat(myFilterWeights.myClanWarsWrongOrderWeight);
		theStream.WriteFloat(myFilterWeights.myClanWarsRatingDiffWeight);
		theStream.WriteFloat(myFilterWeights.myClanWarsMaxRatingDiff);
	}

	bool
	FilterWeightsRsp::FromStream(
		MN_ReadMessage&		theStream)
	{
		bool good = true; 

		good = good && theStream.ReadFloat(myFilterWeights.myClanWarsHaveMapWeight); 
		good = good && theStream.ReadFloat(myFilterWeights.myClanWarsDontHaveMapWeight); 
		good = good && theStream.ReadFloat(myFilterWeights.myClanWarsPlayersWeight); 
		good = good && theStream.ReadFloat(myFilterWeights.myClanWarsWrongOrderWeight);
		good = good && theStream.ReadFloat(myFilterWeights.myClanWarsRatingDiffWeight);
		good = good && theStream.ReadFloat(myFilterWeights.myClanWarsMaxRatingDiff);

		return good; 
	}


	//////////////////////////////////////////////////////////////////////////

	void
	RegisterReq::ToStream(
		MN_WriteMessage&	theStream)
	{
		theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_COLOSSEUM_REGISTER_REQ);
		filter.ToStream(theStream);
	}

	bool
	RegisterReq::FromStream(
		MN_ReadMessage&		theStream)
	{
		return filter.FromStream(theStream);
	}
	
	//////////////////////////////////////////////////////////////////////////

	void
	UnregisterReq::ToStream(
		MN_WriteMessage&	theStream)
	{
		theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_COLOSSEUM_UNREGISTER_REQ);
	}

	bool
	UnregisterReq::FromStream(
		MN_ReadMessage&		theStream)
	{
		return true; 
	}
	
	//////////////////////////////////////////////////////////////////////////

	void
	GetReq::ToStream(
		MN_WriteMessage&	theStream)
	{
		theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_COLOSSEUM_GET_REQ);
		theStream.WriteInt(reqId);
	}

	bool
	GetReq::FromStream(
		MN_ReadMessage&	theStream)
	{
		return theStream.ReadInt(reqId);
	}

	//////////////////////////////////////////////////////////////////////////

	void
	GetRsp::ToStream(
		MN_WriteMessage&	theStream)
	{
		if(!entries.Count())
		{
			theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_COLOSSEUM_GET_RSP);
			theStream.WriteInt(reqId);
			theStream.WriteFloat(myRating);
			theStream.WriteInt(0);
			
			return; 
		}

		for(int i = 0; i < entries.Count(); i++)
		{
			if(i % 10 == 0)
			{
				theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_COLOSSEUM_GET_RSP);
				theStream.WriteInt(reqId);
				theStream.WriteFloat(myRating);
				theStream.WriteInt(__min(10, entries.Count() - i));
			}

			theStream.WriteInt(entries[i].clanId);
			theStream.WriteInt(entries[i].profileId);
			theStream.WriteInt(entries[i].ladderPos);
			theStream.WriteFloat(entries[i].rating);
			entries[i].filter.ToStream(theStream); 
		}
	}

	bool
	GetRsp::FromStream(
		MN_ReadMessage&	theStream)
	{
		bool		good = true; 
		int			count;

		good = good && theStream.ReadInt(reqId); 
		good = good && theStream.ReadFloat(myRating);
		good = good && theStream.ReadInt(count); 

		for(int i = 0; good && i < count; i++)
		{
			GetRsp::Entry	e;

			good = good && theStream.ReadInt(e.clanId); 
			good = good && theStream.ReadInt(e.profileId);
			good = good && theStream.ReadInt(e.ladderPos);
			good = good && theStream.ReadFloat(e.rating);
			good = good && e.filter.FromStream(theStream); 

			if(good)
				entries.Add(e);
		}

		return true;
	}

};
