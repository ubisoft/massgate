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
#ifndef MMG_CLANMATCHHISTORYPROTOCOL_H
#define MMG_CLANMATCHHISTORYPROTOCOL_H

#include "MN_WriteMessage.h"
#include "MN_ReadMessage.h"
#include "MC_Hybridarray.h"

class MMG_ClanMatchHistoryProtocol
{
public:
	class GetListingReq
	{
	public:	
		void ToStream(
			MN_WriteMessage& theStream) const;
		bool FromStream(
			MN_ReadMessage& theStream);

		unsigned int clanId;
		unsigned int requestId;
	};

	class GetListingRsp
	{
	public:	
		void ToStream(
			MN_WriteMessage& theStream) const;
		bool FromStream(
			MN_ReadMessage& theStream);

		class Match 
		{
		public: 
			Match()
			: matchId(0)
			, winnerClan(0)
			, loserClan(0)
			, datePlayed(0)
			{
			}

			Match(
				unsigned int aMatchId,
				unsigned int aWinnerClan, 
				unsigned int aLoserClan, 
				unsigned int aDatePlayed, 
				MC_StaticLocString<256>& aMapName)
			: matchId(aMatchId)
			, winnerClan(aWinnerClan)
			, loserClan(aLoserClan)
			, datePlayed(aDatePlayed)
			, mapName(aMapName)
			{
			}

			unsigned int matchId; 
			unsigned int winnerClan; 
			unsigned int loserClan; 
			unsigned int datePlayed; 
			MC_StaticLocString<256> mapName;
		};

		void AddMatch(
			Match& aMatch);

		static const int NUM_MATCHES = 64; // 12 per package

		MC_HybridArray<Match, NUM_MATCHES> matches; 

		unsigned int requestId;
	};

	class GetDetailedReq
	{
	public:	
		void ToStream(
			MN_WriteMessage& theStream) const;
		bool FromStream(
			MN_ReadMessage& theStream);
		
		unsigned int matchId; 
	};

	class GetDetailedRsp
	{
	public:	
		void ToStream(
			MN_WriteMessage& theStream) const;
		bool FromStream(
			MN_ReadMessage& theStream);

		unsigned int				matchId; 
		unsigned int				datePlayed; 
		MC_StaticLocString<256>		mapName;
		unsigned int				winnerClan;
		unsigned int				loserClan; 
		unsigned short				winnerScore; 
		unsigned short				loserScore; 
		unsigned char				winnerFaction;
		unsigned char				loserFaction;
		unsigned short				winnerCriticalStrikes; 
		unsigned short				loserCriticalStrikes; 
		unsigned char				gameMode; // 0 = DOMINATION, 1 = ASSAULT, 2 = TUG OF WAR 

		// used only in domination 
		unsigned short				winnerDominationPercentage;  
		unsigned short				loserDominationPercentage; 

		// used only in assault 
		unsigned char				winnerAssaultNumCommandPoints;
		unsigned int				winnerAssaultEndTime; 
		unsigned char				loserAssaultNumCommandPoints;
		unsigned int				loserAssaultEndTime; 

		// used only in tug of war
		unsigned char				winnerTowNumLinesPushed; 
		unsigned char				winnerTowNumPerimiterPoints; 
		unsigned char				loserTowNumLinesPushed; 
		unsigned char				loserTowNumPerimiterPoints; 

		unsigned short timeUsed;

		class PerPlayerData 
		{
		public:
			PerPlayerData()
				: profileId(0)
				, score(0)
				, role(0)
			{
			}

			PerPlayerData(
				unsigned int aProfileId, 
				unsigned short aScore, 
				unsigned char aRole)
				: profileId(aProfileId)
				, score(aScore)
				, role(aRole)
			{
			}

			unsigned int profileId; 
			unsigned short score; 
			unsigned char role; // 0 = NONE, 1 = ARMOUR, 2 = AIR, 3 = INFANTRY, 4 = SUPPORT 
		};

		void AddWinner(
			PerPlayerData& aWinner);

		void AddLoser(
			PerPlayerData& aLoser);

		MC_HybridArray<PerPlayerData, 8> winners; 
		MC_HybridArray<PerPlayerData, 8> losers; 
	};
};

class MMG_IClanMatchHistoryListener 
{
public: 
	virtual void OnClanMatchListingResponse(MMG_ClanMatchHistoryProtocol::GetListingRsp& aGetRsp) = 0; 
	virtual void OnClanMatchDetailsResponse(MMG_ClanMatchHistoryProtocol::GetDetailedRsp& aGetRsp) = 0; 
};

#endif