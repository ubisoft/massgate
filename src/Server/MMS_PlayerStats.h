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
#ifndef MMS_PLAYERSTATS_H
#define MMS_PLAYERSTATS_H

#include "MT_Thread.h"
#include "mdb_mysqlconnection.h"
#include "MMS_Constants.h"
#include "MT_Mutex.h"
#include "MC_SortedGrowingArray.h"
#include "MMG_Stats.h"
#include "MMG_Profile.h"
#include "MC_EggClockTimer.h"
#include "MMG_DecorationProtocol.h"
#include "MC_SmallObjectAllocator.h"

class MMS_MasterServer;
class MMS_MasterConnectionHandler;

class MMS_PlayerStats
{
public:
	MMS_PlayerStats(const MMS_Settings& theSettings, MMS_MasterServer* theServer);
	~MMS_PlayerStats();

	void Update(MMS_MasterConnectionHandler* currentConnectionHandler);

	static MMS_PlayerStats* GetInstance();

	bool UpdatePlayerStats(MMG_Stats::PlayerMatchStats& someStats); 
	bool GetPlayerStats(MMG_Stats::PlayerStatsRsp& someStats, unsigned int aProfileId); 
	bool GetPlayerMedals(MMG_DecorationProtocol::PlayerMedalsRsp& someMedals, unsigned int aProfileId); 
	bool GetPlayerBadges(MMG_DecorationProtocol::PlayerBadgesRsp& someBadges, unsigned int aProfileId); 
	bool GetRankAndTotalScore(unsigned int aProfileId, unsigned int& aRank, unsigned int& aTotalScore);

	bool GivePreorderMedal(unsigned int aProfileId);
	bool GiveRecruitmentMedal(unsigned int aProfileId);
	void InvalidateProfile(unsigned int aProfileId);

private: 
	MMS_MasterServer*				myServer;
	static MMS_PlayerStats*			ourInstance; 
	const MMS_Settings&				mySettings;

	class TweakableValues 
	{
	public: 
		TweakableValues(); 

		bool Load(MDB_MySqlConnection* aReadConnection); 

		unsigned int medal_infAch_NumForBronze;
		unsigned int medal_infAch_NumForSilver;
		unsigned int medal_infAch_StreakForGold;
		unsigned int medal_infAch_BronzeScoreMod;
		unsigned int medal_infAch_SilverScoreMod;
		unsigned int medal_infAch_GoldScoreMod;
		unsigned int medal_supAch_NumForBronze;
		unsigned int medal_supAch_NumForSilver;
		unsigned int medal_supAch_StreakForGold;
		unsigned int medal_supAch_BronzeScoreMod;
		unsigned int medal_supAch_SilverScoreMod;
		unsigned int medal_supAch_GoldScoreMod;
		unsigned int medal_armAch_NumForBronze;
		unsigned int medal_armAch_NumForSilver;
		unsigned int medal_armAch_StreakForGold;
		unsigned int medal_armAch_BronzeScoreMod;
		unsigned int medal_armAch_SilverScoreMod;
		unsigned int medal_armAch_GoldScoreMod;
		unsigned int medal_airAch_NumForBronze;
		unsigned int medal_airAch_NumForSilver;
		unsigned int medal_airAch_StreakForGold;
		unsigned int medal_airAch_BronzeScoreMod;
		unsigned int medal_airAch_SilverScoreMod;
		unsigned int medal_airAch_GoldScoreMod;
		unsigned int medal_scoreAch_NumForBronze;
		unsigned int medal_scoreAch_NumForSilver;
		unsigned int medal_scoreAch_StreakForGold;
		unsigned int medal_scoreAch_BronzeScoreMod;
		unsigned int medal_scoreAch_SilverScoreMod;
		unsigned int medal_scoreAch_GoldScoreMod;
		unsigned int medal_taAch_NumForBronze ;
		unsigned int medal_taAch_NumForSilver;
		unsigned int medal_taAch_NumForGold;
		unsigned int medal_taAch_BronzeScoreMod;
		unsigned int medal_taAch_SilverScoreMod;
		unsigned int medal_taAch_GoldScoreMod;
		unsigned int medal_infComEx_ScoreForBronze;
		unsigned int medal_infComEx_ScoreForSilver;
		unsigned int medal_infComEx_ScoreForGold;
		unsigned int medal_infComEx_BronzeScoreMod;
		unsigned int medal_infComEx_SilverScoreMod;
		unsigned int medal_infComEx_GoldScoreMod;
		unsigned int medal_supComEx_ScoreForBronze;
		unsigned int medal_supComEx_ScoreForSilver;
		unsigned int medal_supComEx_ScoreForGold;
		unsigned int medal_supComEx_BronzeScoreMod;
		unsigned int medal_supComEx_SilverScoreMod;
		unsigned int medal_supComEx_GoldScoreMod;
		unsigned int medal_armComEx_ScoreForBronze;
		unsigned int medal_armComEx_ScoreForSilver;
		unsigned int medal_armComEx_ScoreForGold;
		unsigned int medal_armComEx_BronzeScoreMod;
		unsigned int medal_armComEx_SilverScoreMod;
		unsigned int medal_armComEx_GoldScoreMod;
		unsigned int medal_airComEx_ScoreForBronze;
		unsigned int medal_airComEx_ScoreForSilver;
		unsigned int medal_airComEx_ScoreForGold;
		unsigned int medal_airComEx_BronzeScoreMod;
		unsigned int medal_airComEx_SilverScoreMod;
		unsigned int medal_airComEx_GoldScoreMod;
		unsigned int medal_winStreak_NumForBronze;
		unsigned int medal_winStreak_NumForSilver;
		unsigned int medal_winStreak_NumForGold;
		unsigned int medal_winStreak_BronzeScoreMod;
		unsigned int medal_winStreak_SilverScoreMod;
		unsigned int medal_winStreak_GoldScoreMod;
		unsigned int medal_taSpec_NumForBronze;
		unsigned int medal_taSpec_NumForSilver;
		unsigned int medal_taSpec_NumForGold;
		unsigned int medal_taSpec_BronzeScoreMod;
		unsigned int medal_taSpec_SilverScoreMod;
		unsigned int medal_taSpec_GoldScoreMod;
		unsigned int medal_domSpec_NumForBronze;
		unsigned int medal_domSpec_NumForSilver;
		unsigned int medal_domSpec_StreakForGold;
		unsigned int medal_domSpec_BronzeScoreMod;
		unsigned int medal_domSpec_SilverScoreMod;
		unsigned int medal_domSpec_GoldScoreMod;
		unsigned int medal_domEx_NumForBronze;
		unsigned int medal_domEx_NumForSilver;
		unsigned int medal_domEx_StreakForGold;
		unsigned int medal_domEx_BronzeScoreMod;
		unsigned int medal_domEx_SilverScoreMod;
		unsigned int medal_domEx_GoldScoreMod;
		unsigned int medal_assSpec_NumForBronze;
		unsigned int medal_assSpec_NumForSilver;
		unsigned int medal_assSpec_StreakForGold;
		unsigned int medal_assSpec_BronzeScoreMod;
		unsigned int medal_assSpec_SilverScoreMod;
		unsigned int medal_assSpec_GoldScoreMod;
		unsigned int medal_assEx_NumForBronze;
		unsigned int medal_assEx_NumForSilver;
		unsigned int medal_assEx_StreakForGold;
		unsigned int medal_assEx_BronzeScoreMod;
		unsigned int medal_assEx_SilverScoreMod;
		unsigned int medal_assEx_GoldScoreMod;
		unsigned int medal_towSpec_NumForBronze;
		unsigned int medal_towSpec_NumForSilver;
		unsigned int medal_towSpec_StreakForGold;
		unsigned int medal_towSpec_BronzeScoreMod;
		unsigned int medal_towSpec_SilverScoreMod;
		unsigned int medal_towSpec_GoldScoreMod;
		unsigned int medal_towEx_NumForBronze;
		unsigned int medal_towEx_NumForSilver;
		unsigned int medal_towEx_StreakForGold;
		unsigned int medal_towEx_BronzeScoreMod;
		unsigned int medal_towEx_SilverScoreMod;
		unsigned int medal_towEx_GoldScoreMod;
		unsigned int medal_nukeSpec_NumForBronze;
		unsigned int medal_nukeSpec_NumForSilver;
		unsigned int medal_nukeSpec_NumForStreak;
		unsigned int medal_nukeSpec_StreakForGold;
		unsigned int medal_nukeSpec_BronzeScoreMod;
		unsigned int medal_nukeSpec_SilverScoreMod;
		unsigned int medal_nukeSpec_GoldScoreMod;
		unsigned int badge_infSp_ScoreForBronze;
		unsigned int badge_infSp_ScoreForSilver;
		unsigned int badge_infSp_ScoreForGold;
		unsigned int badge_infSp_BronzeScoreMod;
		unsigned int badge_infSp_SilverScoreMod;
		unsigned int badge_infSp_GoldScoreMod;
		unsigned int badge_airSp_ScoreForBronze;
		unsigned int badge_airSp_ScoreForSilver;
		unsigned int badge_airSp_ScoreForGold;
		unsigned int badge_airSp_BronzeScoreMod;
		unsigned int badge_airSp_SilverScoreMod;
		unsigned int badge_airSp_GoldScoreMod;
		unsigned int badge_armSp_ScoreForBronze;
		unsigned int badge_armSp_ScoreForSilver;
		unsigned int badge_armSp_ScoreForGold;
		unsigned int badge_armSp_BronzeScoreMod;
		unsigned int badge_armSp_SilverScoreMod;
		unsigned int badge_armSp_GoldScoreMod;
		unsigned int badge_supSp_ScoreForBronze;
		unsigned int badge_supSp_ScoreForSilver;
		unsigned int badge_supSp_ScoreForGold;
		unsigned int badge_supSp_BronzeScoreMod;
		unsigned int badge_supSp_SilverScoreMod;
		unsigned int badge_supSp_GoldScoreMod;
		unsigned int badge_scoreAch_ScoreForBronze;
		unsigned int badge_scoreAch_ScoreForSilver;
		unsigned int badge_scoreAch_ScoreForGold;
		unsigned int badge_scoreAch_BronzeScoreMod;
		unsigned int badge_scoreAch_SilverScoreMod;
		unsigned int badge_scoreAch_GoldScoreMod;
		unsigned int badge_cpAch_NumForBronze;
		unsigned int badge_cpAch_NumForSilver;
		unsigned int badge_cpAch_NumForGold;
		unsigned int badge_cpAch_BronzeScoreMod;
		unsigned int badge_cpAch_SilverScoreMod;
		unsigned int badge_cpAch_GoldScoreMod;
		unsigned int badge_fortAch_NumForBronze;
		unsigned int badge_fortAch_NumForSilver;
		unsigned int badge_fortAch_NumForGold;
		unsigned int badge_fortAch_BronzeScoreMod;
		unsigned int badge_fortAch_SilverScoreMod;
		unsigned int badge_fortAch_GoldScoreMod;
		unsigned int badge_mgAch_TimeForBronze;
		unsigned int badge_mgAch_TimeForSilver;
		unsigned int badge_mgAch_TimeForGold;
		unsigned int badge_mgAch_BronzeScoreMod;
		unsigned int badge_mgAch_SilverScoreMod;
		unsigned int badge_mgAch_GoldScoreMod;
		unsigned int badge_matchAch_NumForBronze;
		unsigned int badge_matchAch_NumForSilver;
		unsigned int badge_matchAch_NumForGold;
		unsigned int badge_matchAch_BronzeScoreMod;
		unsigned int badge_matchAch_SilverScoreMod;
		unsigned int badge_matchAch_GoldScoreMod;
		unsigned int badge_USAAch_TimeForBronze;
		unsigned int badge_USAAch_TimeForSilver;
		unsigned int badge_USAAch_TimeForGold;
		unsigned int badge_USAAch_BronzeScoreMod;
		unsigned int badge_USAAch_SilverScoreMod;
		unsigned int badge_USAAch_GoldScoreMod;
		unsigned int badge_USSRAch_TimeForBronze;
		unsigned int badge_USSRAch_TimeForSilver;
		unsigned int badge_USSRAch_TimeForGold;
		unsigned int badge_USSRAch_BronzeScoreMod;
		unsigned int badge_USSRAch_SilverScoreMod;
		unsigned int badge_USSRAch_GoldScoreMod;
		unsigned int badge_NATOAch_TimeForBronze;
		unsigned int badge_NATOAch_TimeForSilver;
		unsigned int badge_NATOAch_TimeForGold;
		unsigned int badge_NATOAch_BronzeScoreMod;
		unsigned int badge_NATOAch_SilverScoreMod;
		unsigned int badge_NATOAch_GoldScoreMod;
		unsigned int badge_preOrdAch_TimeForBronze;
		unsigned int badge_preOrdAch_TimeForSilver;
		unsigned int badge_preOrdAch_TimeForGold;
		unsigned int badge_preOrdAch_BronzeScoreMod;
		unsigned int badge_preOrdAch_SilverScoreMod;
		unsigned int badge_preOrdAch_GoldScoreMod;
	};

	TweakableValues myTweakables; 
	MC_EggClockTimer myTweakablesReReadTimeout; 

	class PlayerStatsEntry
	{
	public: 
		PlayerStatsEntry() 
		: profileId(0) 
		, myMutex(NULL) 
		{ 
		};

		void Init(unsigned int aProfileId, MT_Mutex* aMutex); 

		void* operator new(size_t aSize);
		void operator delete(void* aPointer);

		void UpdateStats(MMG_Stats::PlayerMatchStats& someStats, TweakableValues& someMods); 
		unsigned int UpdateMedal(unsigned int* value, 
								 unsigned int* numStars, 
								 unsigned int featsA, 
								 unsigned int featsB, 
								 unsigned int bronzeTrigger, 
								 unsigned int silverTrigger, 
								 unsigned int goldTrigger, 
								 unsigned int bronzeMod, 
								 unsigned int silverMod, 
								 unsigned int goldMod); 
		void UpdateHighlyDecorated(); 
		unsigned int UpdateMedals(MMG_Stats::PlayerMatchStats& someStats, TweakableValues& someMods); 
		unsigned int UpdateBadge(unsigned int* value, 
								 unsigned int* numStars, 
								 unsigned int feats, 
								 unsigned int bronzeTrigger, 
								 unsigned int silverTrigger, 
								 unsigned int goldTrigger, 
								 unsigned int bronzeMod, 
								 unsigned int silverMod, 
								 unsigned int goldMod); 
		unsigned int UpdateBadges(MMG_Stats::PlayerMatchStats& someStats, TweakableValues& someMods); 
		bool Load(MDB_MySqlConnection* aReadConnection); 
		void GetStats(MMG_Stats::PlayerStatsRsp& someStats); 
		void GetMedals(MMG_DecorationProtocol::PlayerMedalsRsp& someMedals); 
		void GetBadges(MMG_DecorationProtocol::PlayerBadgesRsp& someBadges); 

		unsigned int profileId;
		
		bool pushPromotedPlayer;
		bool dataIsDirty; 
		bool loadThisEntry; 
		volatile long usageCount; 
		MT_Mutex* myMutex; 
		MC_EggClockTimer timeLeftToPurge; 

		static const unsigned int NONE = 0; 
		static const unsigned int BRONZE = 1; 
		static const unsigned int SILVER = 2; 
		static const unsigned int GOLD = 3; 
		
		// stats data 
		unsigned int rank;
		unsigned int lastMatchPlayed; 
		unsigned int massgateMemberSince; 
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
		unsigned int scoreLostByKillingFriendly;
		unsigned int highestScore; 
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
		unsigned int numberOfDominationMatches; 
		unsigned int numberOfDominationMatchesWon; 
		unsigned int numberOfDominationStreak; 
		unsigned int numberOfAssaultMatches; 
		unsigned int numberOfAssaultMatchesWon; 
		unsigned int numberOfAssaultStreak; 
		unsigned int numberOfTugOfWarMatches; 
		unsigned int numberOfTugOfWarMatchesWon; 
		unsigned int numberOfTugOfWarStreak; 
		unsigned int numberOfMatchesWonByTotalDomination;
		unsigned int numberOfTotalDominationStreak;
		unsigned int numberOfPerfectDefendsInAssultMatch;
		unsigned int numberOfPerfectDefendsInAssultStreak;
		unsigned int numberOfPerfectPushesInTugOfWarMatch;
		unsigned int numberOfPerfectPushesInTugOfWarStreak;
		unsigned int numberOfUnitsKilled;
		unsigned int numberOfUnitsLost;
		unsigned int numberOfReinforcementPointsSpent;
		unsigned int numberOfTacticalAidPointsSpent;
		unsigned int numberOfNukesDeployed;
		unsigned int numberOfNukesDeployedStreak;
		unsigned int numberOfTacticalAidCristicalHits;
		unsigned int numberOfCommandPointCaptures;
		unsigned int numberOftimesAsBestPlayer;
		unsigned int currentBestPlayerStreak; 
		unsigned int numberOftimesAsBestInfPlayer;
		unsigned int currentBestInfStreak;
		unsigned int numberOftimesAsBestSupPlayer; 
		unsigned int currentBestSupStreak; 
		unsigned int numberOftimesAsBestArmPlayer;
		unsigned int currentBestArmStreak;
		unsigned int numberOftimesAsBestAirPlayer;
		unsigned int currentBestAirStreak;
		unsigned int numStatPadsDetected; // Number of times score has been ignored due to invalid score detection
		unsigned int ignoreStatsUntil;

		// medals data 
		unsigned int medal_infAch; 
		unsigned int medal_infAchStars; 
		unsigned int medal_airAch;
		unsigned int medal_airAchStars;
		unsigned int medal_armAch;
		unsigned int medal_armAchStars;
		unsigned int medal_supAch;
		unsigned int medal_supAchStars;
		unsigned int medal_scoreAch;
		unsigned int medal_scoreAchStars;
		unsigned int medal_taAch;
		unsigned int medal_taAchStars;
		unsigned int medal_infComEx;
		unsigned int medal_infComExStars;
		unsigned int medal_airComEx;
		unsigned int medal_airComExStars;
		unsigned int medal_armComEx;
		unsigned int medal_armComExStars;
		unsigned int medal_supComEx;
		unsigned int medal_supComExStars;
		unsigned int medal_winStreak;
		unsigned int medal_winStreakStars;
		unsigned int medal_domSpec;
		unsigned int medal_domSpecStars;
		unsigned int medal_domEx;
		unsigned int medal_domExStars;
		unsigned int medal_assSpec;
		unsigned int medal_assSpecStars;
		unsigned int medal_assEx;
		unsigned int medal_assExStars;
		unsigned int medal_towSpec;
		unsigned int medal_towSpecStars;
		unsigned int medal_towEx;
		unsigned int medal_towExStars;
		unsigned int medal_nukeSpec;
		unsigned int medal_nukeSpecStars;
		unsigned int medal_highDec;
		unsigned int medal_highDecStars;

		// badge data 
		unsigned int badge_infSp;
		unsigned int badge_infSpStars;
		unsigned int badge_airSp;
		unsigned int badge_airSpStars;
		unsigned int badge_armSp;
		unsigned int badge_armSpStars;
		unsigned int badge_supSp;
		unsigned int badge_supSpStars;
		unsigned int badge_scoreAch;
		unsigned int badge_scoreAchStars;
		unsigned int badge_cpAch;
		unsigned int badge_cpAchStars;
		unsigned int badge_fortAch;
		unsigned int badge_fortAchStars;
		unsigned int badge_mgAch;
		unsigned int badge_mgAchStars;
		unsigned int badge_matchAch;
		unsigned int badge_matchAchStars;
		unsigned int badge_USAAch;
		unsigned int badge_USAAchStars;
		unsigned int badge_USSRAch;
		unsigned int badge_USSRAchStars;
		unsigned int badge_NATOAch;
		unsigned int badge_NATOAchStars;
		unsigned int badge_preOrdAch;
		unsigned int badge_preOrdAchStars;
		unsigned int badge_reqruitAFriendAch;
		unsigned int badge_reqruitAFriendAchStars;
	};

	static MT_Mutex ourNewEntryAllocationLock;
	static MC_SmallObjectAllocator<sizeof(MMS_PlayerStats::PlayerStatsEntry), 16*1024> ourEntryAllocator;

	class ProfileIdComparer
	{
	public:
		static bool LessThan(const PlayerStatsEntry* pseA, 
							 const PlayerStatsEntry* pseB) 
		{ 
			return pseA->profileId < pseB->profileId; 
		}
		static bool GreaterThan(const PlayerStatsEntry* pseA, 
								const PlayerStatsEntry* pseB) 
		{ 
			return pseA->profileId > pseB->profileId; 
		}
		static bool Equals(const PlayerStatsEntry* pseA, 
						   const PlayerStatsEntry* pseB) 
		{ 
			return pseA->profileId == pseB->profileId; 
		}

		static bool LessThan(const PlayerStatsEntry* pseA, 
							 const unsigned int profileIdB) 
		{ 
			return pseA->profileId < profileIdB; 
		}
		static bool GreaterThan(const PlayerStatsEntry* pseA, 
								const unsigned int profileIdB) 
		{ 
			return pseA->profileId > profileIdB; 
		}
		static bool Equals(const PlayerStatsEntry* pseA, 
						   const unsigned int profileIdB) 
		{ 
			return pseA->profileId == profileIdB; 
		}
	};

	MC_SortedGrowingArray<PlayerStatsEntry*> myPlayerStats; 

	PlayerStatsEntry* PrivGetInCache(unsigned int profileId); 
	void PrivReturnToCache(PlayerStatsEntry* someEntry); 

	MT_Mutex globalLock; 

	const static unsigned int NUM_ENTRY_MUTEXES = 64; 
	MT_Mutex entryMutexes[NUM_ENTRY_MUTEXES]; 
	MT_Mutex* PrivGetEntryMutex(); 

	MDB_MySqlConnection* myReadDatabaseConnection;
	MDB_MySqlConnection* myWriteDatabaseConnection;

	typedef MC_HybridArray<PlayerStatsEntry*, 1024> DirtyEntryList;

	bool PrivFlushStatsToDb(DirtyEntryList& theEntries); 
	bool PrivFlushMedalsToDb(DirtyEntryList& theEntries); 
	bool PrivFlushBadgesToDb(DirtyEntryList& theEntries); 
	bool PrivInvalidateLuts(DirtyEntryList& theEntries, MMS_MasterConnectionHandler* currentConnectionHandler); 
	void PrivUnMarkDirty(DirtyEntryList& theEntries);
	bool PrivLockDirtyEntries(DirtyEntryList& theEntries);
	void PrivUnlockDirtyEntries(DirtyEntryList& theEntries);

	void PrivLogSuspectEntry(PlayerStatsEntry* thePlayerEntry, MMG_Stats::PlayerMatchStats& someStats);
};

#endif
