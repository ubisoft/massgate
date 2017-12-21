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
#ifndef _MMG_STATS____H__
#define _MMG_STATS____H__

#include "MN_ReadMessage.h"
#include "MN_WriteMessage.h"
#include "MC_HybridArray.h"

#pragma pack(push,1)

#define MATCHTYPE_DOMINATION 0
#define MATCHTYPE_ASSAULT	 1
#define MATCHTYPE_TOW		 2
#define MATCHTYPE_TDM		 3 // todo: remove if removed from game modes

class MMG_Stats 
{
public: 

	// DS reports stats to massgate 
	class PlayerMatchStats 
	{
	public:
		PlayerMatchStats(); 

		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		unsigned int gameProtocolVersion;					// DS
		unsigned int profileId;								// DS

		unsigned short scoreTotal;							

		unsigned short scoreAsInfantry;						
		unsigned short scoreAsSupport;						
		unsigned short scoreAsArmor;						
		unsigned short scoreAsAir;							

		unsigned short scoreByDamagingEnemies;				
		unsigned short scoreByUsingTacticalAids;			
		unsigned short scoreByCommandPointCaptures;			
		unsigned short scoreByRepairing;					
		unsigned short scoreByFortifying;					

		unsigned short scoreLostByKillingFriendly;			

		unsigned short timeTotalMatchLength;							

		unsigned short timePlayedAsUSA;						
		unsigned short timePlayedAsUSSR;					
		unsigned short timePlayedAsNATO;					

		unsigned short timePlayedAsInfantry;				
		unsigned short timePlayedAsAir;						
		unsigned short timePlayedAsArmor;					
		unsigned short timePlayedAsSupport;					

		unsigned char matchWon;								
		unsigned char matchType;							
		unsigned char matchLost;							
		unsigned char matchWasFlawlessVictory;				

		unsigned short numberOfUnitsKilled;					
		unsigned short numberOfUnitsLost;					
		unsigned short numberOfCommandPointCaptures;		
		unsigned short numberOfReinforcementPointsSpent;	
		unsigned short numberOfRoleChanges;					
		unsigned short numberOfTacticalAidPointsSpent;		
		unsigned short numberOfNukesDeployed;				
		unsigned short numberOfTacticalAidCriticalHits;		

		const static unsigned char BEST_PLAYER	 = 0x01; 
		const static unsigned char BEST_INFANTRY = 0x02; 
		const static unsigned char BEST_SUPPORT  = 0x04; 
		const static unsigned char BEST_AIR		 = 0x08; 
		const static unsigned char BEST_ARMOR	 = 0x10; 

		unsigned char bestData;

		// WORKING VARIABLES! NOT UPLOADED
		float totalTimePlayed;
		unsigned char wasPlayingAtMatchEnd;
	};

	class ClanMatchStats
	{
	public:
		ClanMatchStats(); 

		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		unsigned int		gameProtocolVersion;					

		unsigned __int64	mapHash; 
		MC_StaticLocString<256> mapName;
		unsigned int		clanId;								
		unsigned char		matchType;							

		// used only in domination 
		unsigned short		dominationPercetage; 

		// used only in assault 
		unsigned char		assaultNumCommandPoints;
		unsigned int		assaultEndTime; 

		// used only in tug of war
		unsigned char		towNumLinesPushed; 
		unsigned char		towNumPerimiterPoints; 

		unsigned char		matchWon;								

		unsigned char		isTournamentMatch;					
		unsigned char		matchWasFlawlessVictory;				

		unsigned int		numberOfUnitsKilled;					
		unsigned int		numberOfUnitsLost;						
		unsigned int		numberOfNukesDeployed;					
		unsigned int		numberOfTACriticalHits;				
		unsigned int		timeAsUSA;								
		unsigned int		timeAsUSSR; 
		unsigned int		timeAsNATO; 

		unsigned int		totalScore;							
		unsigned int		scoreByDamagingEnemies;				
		unsigned int		scoreByRepairing;						
		unsigned int		scoreByTacticalAid;					
		unsigned int		scoreByFortifying;						
		unsigned int		scoreByInfantry;						
		unsigned int		scoreBySupport;						
		unsigned int		scoreByAir;							
		unsigned int		scoreByArmor;		

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

		void AddPlayer(
			unsigned int aProfileId, 
			unsigned short aScore, 
			unsigned char aRole);

		MC_HybridArray<PerPlayerData, 8> perPlayerData; 
	};

	// request stats from massgate 

	class PlayerStatsReq 
	{
	public: 
		PlayerStatsReq(); 

		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		unsigned int profileId;
	};

	class PlayerStatsRsp
	{
	public: 
		PlayerStatsRsp(); 

		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		unsigned int profileId;

		unsigned int lastMatchPlayed;						
		unsigned int scoreTotal;							
		unsigned int scoreAsInfantry;						
		unsigned int highScoreAsInfantry;					
		unsigned int scoreAsSupport;						
		unsigned int highScoreAsSupport;					
		unsigned int scoreAsArmor;							
		unsigned int highScoreAsArmor;						
		unsigned int scoreAsAir;							
		unsigned int highScoreAsAir;						
		unsigned int scoreByDamagingEnemies;				
		unsigned int scoreByUsingTacticalAids;				
		unsigned int scoreByCapturingCommandPoints;			
		unsigned int scoreByRepairing;						
		unsigned int scoreByFortifying;						
		unsigned int highestScore;							
		unsigned int currentLadderPosition;					
		unsigned int timeTotalMatchLength;					
		unsigned int timePlayedAsUSA;						
		unsigned int timePlayedAsUSSR;						
		unsigned int timePlayedAsNATO;						
		unsigned int timePlayedAsInfantry;					
		unsigned int timePlayedAsSupport;					
		unsigned int timePlayedAsArmor;						
		unsigned int timePlayedAsAir;						
		unsigned int numberOfMatches;						
		unsigned int numberOfMatchesWon;
		unsigned int numberOfMatchesLost;
		unsigned int currentWinningStreak;					
		unsigned int bestWinningStreak;						
		unsigned int numberOfAssaultMatches;				
		unsigned int numberOfAssaultMatchesWon;				
		unsigned int numberOfDominationMatches;				
		unsigned int numberOfDominationMatchesWon;			
		unsigned int numberOfTugOfWarMatches;				
		unsigned int numberOfTugOfWarMatchesWon;			
		unsigned int numberOfMatchesWonByTotalDomination;	
		unsigned int numberOfPerfectDefendsInAssultMatch;	
		unsigned int numberOfPerfectPushesInTugOfWarMatch;	
		unsigned int numberOfUnitsKilled;					
		unsigned int numberOfUnitsLost;						
		unsigned int numberOfReinforcementPointsSpent;		
		unsigned int numberOfTacticalAidPointsSpent;		
		unsigned int numberOfNukesDeployed;					
		unsigned int numberOfTacticalAidCristicalHits;		
	};

	class ClanStatsReq
	{
	public: 
		ClanStatsReq(); 

		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		unsigned int clanId;
	}; 

	class ClanStatsRsp
	{
	public: 
		ClanStatsRsp(); 

		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		unsigned int clanId;									
		unsigned int lastMatchPlayed;							
		unsigned int matchesPlayed;								
		unsigned int matchesWon;								
		unsigned int bestWinningStreak;							
		unsigned int currentWinningStreak;						
		unsigned int currentLadderPosition;						
		unsigned int tournamentsPlayed;							
		unsigned int tournamentsWon;							
		unsigned int dominationMatchesPlayed;					
		unsigned int dominationMatchesWon;						
		unsigned int dominationMatchesWonByTotalDomination;		
		unsigned int assaultMatchesPlayed;						
		unsigned int assaultMatchesWon;							
		unsigned int assaultMatchesPerfectDefense;				
		unsigned int towMatchesPlayed;							
		unsigned int towMatchesWon;								
		unsigned int towMatchesPerfectPushes;					
		unsigned int numberOfUnitsKilled;						
		unsigned int numberOfUnitsLost; 
		unsigned int numberOfNukesDeployed; 
		unsigned int numberOfTACriticalHits; 
		unsigned int timeAsUSA; 
		unsigned int timeAsUSSR; 
		unsigned int timeAsNATO; 
		unsigned int totalScore; 
		unsigned int highestScoreInAMatch; 
		unsigned int scoreByDamagingEnemies; 
		unsigned int scoreByRepairing; 
		unsigned int scoreByTacticalAid; 
		unsigned int scoreByFortifying; 
		unsigned int scoreByInfantry; 
		unsigned int scoreBySupport; 
		unsigned int scoreByAir;
		unsigned int scoreByArmor; 
		unsigned int highestScoreAsInfantry; 
		unsigned int highestScoreAsSuppport; 
		unsigned int highestScoreAsAir;
		unsigned int highestScoreAsArmor; 
	};

};

class MMG_IStatsListerner 
{
public: 
	virtual void ReceivePlayerStats(MMG_Stats::PlayerStatsRsp& someStats) = 0; 
	virtual void ReceiveClanStats(MMG_Stats::ClanStatsRsp& someStats) = 0; 
};

#pragma pack(pop)
#endif
