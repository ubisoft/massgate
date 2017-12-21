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
#ifndef MMS_CLANSTATS_H
#define MMS_CLANSTATS_H

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

class MMS_ClanStats : public MT_Thread
{
public:
	MMS_ClanStats(const MMS_Settings& theSettings);
	~MMS_ClanStats();

	virtual void Run();

	static MMS_ClanStats* GetInstance();

	bool UpdateClanStats(MMG_Stats::ClanMatchStats& someStats, unsigned int* someExtraScore); 
	bool GetClanStats(MMG_Stats::ClanStatsRsp& someStats, unsigned int aClanId); 
	bool GetClanMedals(MMG_DecorationProtocol::ClanMedalsRsp& someMedals, unsigned int aClanId); 

	void InvalidateClan(unsigned int aClanId); 

private: 
	static MMS_ClanStats* ourInstance; 

	class TweakableValues 
	{
	public: 
		TweakableValues(); 

		bool Load(MDB_MySqlConnection* aReadConnection); 

		unsigned int medal_winStreak_BronzeTrigger;
		unsigned int medal_winStreak_SilverTrigger;
		unsigned int medal_winStreak_GoldTrigger;
		unsigned int medal_winStreak_BronzeScoreMod;
		unsigned int medal_winStreak_SilverScoreMod;
		unsigned int medal_winStreak_GoldScoreMod;	
		unsigned int medal_domSpec_BronzeTrigger;
		unsigned int medal_domSpec_SilverTrigger;
		unsigned int medal_domSpec_GoldTrigger;
		unsigned int medal_domSpec_BronzeScoreMod;
		unsigned int medal_domSpec_SilverScoreMod;
		unsigned int medal_domSpec_GoldScoreMod;
		unsigned int medal_domEx_BronzeTrigger;
		unsigned int medal_domEx_SilverTrigger;
		unsigned int medal_domEx_GoldTrigger;
		unsigned int medal_domEx_BronzeScoreMod;
		unsigned int medal_domEx_SilverScoreMod;
		unsigned int medal_domEx_GoldScoreMod;
		unsigned int medal_assSpec_BronzeTrigger;
		unsigned int medal_assSpec_SilverTrigger;
		unsigned int medal_assSpec_GoldTrigger;
		unsigned int medal_assSpec_BronzeScoreMod;
		unsigned int medal_assSpec_SilverScoreMod;
		unsigned int medal_assSpec_GoldScoreMod;
		unsigned int medal_assEx_BronzeTrigger;
		unsigned int medal_assEx_SilverTrigger;
		unsigned int medal_assEx_GoldTrigger;
		unsigned int medal_assEx_BronzeScoreMod;
		unsigned int medal_assEx_SilverScoreMod;
		unsigned int medal_assEx_GoldScoreMod;
		unsigned int medal_towSpec_BronzeTrigger;
		unsigned int medal_towSpec_SilverTrigger;
		unsigned int medal_towSpec_GoldTrigger;
		unsigned int medal_towSpec_BronzeScoreMod;
		unsigned int medal_towSpec_SilverScoreMod;
		unsigned int medal_towSpec_GoldScoreMod;
		unsigned int medal_towEx_BronzeTrigger;
		unsigned int medal_towEx_SilverTrigger;
		unsigned int medal_towEx_GoldTrigger;
		unsigned int medal_towEx_BronzeScoreMod;
		unsigned int medal_towEx_SilverScoreMod;
		unsigned int medal_towEx_GoldScoreMod;
		unsigned int medal_hsAwd_BronzeTrigger;
		unsigned int medal_hsAwd_SilverTrigger;
		unsigned int medal_hsAwd_GoldTrigger;
		unsigned int medal_hsAwd_BronzeScoreMod;
		unsigned int medal_hsAwd_SilverScoreMod;
		unsigned int medal_hsAwd_GoldScoreMod;
		unsigned int medal_tcAwd_BronzeTrigger;
		unsigned int medal_tcAwd_SilverTrigger;
		unsigned int medal_tcAwd_GoldTrigger;
		unsigned int medal_tcAwd_BronzeScoreMod;
		unsigned int medal_tcAwd_SilverScoreMod;
		unsigned int medal_tcAwd_GoldScoreMod;
	}; 

	TweakableValues myTweakables; 
	MC_EggClockTimer myTweakablesReReadTimeout; 

	class ClanStatsEntry 
	{
	public: 
		ClanStatsEntry() 
		: clanId(0) 
		, myMutex(NULL) 
		{ 
		};

		void Init(unsigned int aClanId, MT_Mutex* aMutex); 

		void* operator new(size_t aSize);
		void operator delete(void* aPointer);

		void UpdateStats(MMG_Stats::ClanMatchStats& someStats, TweakableValues& someMods);

		unsigned int UpdateMedal(unsigned int* value, 
								 unsigned int* numStars, 
								 unsigned int featsA, 
								 unsigned int featsB, 
								 unsigned int featsC, 
								 unsigned int bronzeTrigger, 
								 unsigned int silverTrigger, 
								 unsigned int goldTrigger, 
								 unsigned int bronzeMod, 
								 unsigned int silverMod, 
								 unsigned int goldMod); 

		unsigned int UpdateMedals(MMG_Stats::ClanMatchStats& someStats, TweakableValues& someMods); 

		bool Load(MDB_MySqlConnection* aReadConnection); 
		void GetStats(MMG_Stats::ClanStatsRsp& someStats); 
		void GetMedals(MMG_DecorationProtocol::ClanMedalsRsp& someMedals); 

		unsigned int clanId; 

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
		unsigned int lastMatchPlayed; 
		unsigned int matchesPlayed; 
		unsigned int matchesWon; 
		unsigned int bestWinningStreak; 
		unsigned int currentWinningStreak; 
		unsigned int tournamentsPlayed; 
		unsigned int tournamentsWon; 
		unsigned int dominationMatchesPlayed; 
		unsigned int dominationMatchesWon; 
		unsigned int currentDominationStreak; 
		unsigned int dominationMatchesWonByTotalDomination;
		unsigned int currentTotalDominationStreak; 
		unsigned int assaultMatchesPlayed; 
		unsigned int assaultMatchesWon; 
		unsigned int currentAssaultStreak; 
		unsigned int assaultMatchesPerfectDefense;
		unsigned int currentPerfectDefenseStreak; 
		unsigned int towMatchesPlayed; 
		unsigned int towMatchesWon; 
		unsigned int currentTowStreak; 
		unsigned int towMatchesPerfectPushes; 
		unsigned int currentPerfectPushStreak; 
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
	
		// medals data 
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
		unsigned int medal_hsAwd; 		
		unsigned int medal_hsAwdStars; 		
		unsigned int medal_tcAwd; 		
		unsigned int medal_tcAwdStars; 		
	};

	static MT_Mutex ourNewEntryAllocationLock;
	static MC_SmallObjectAllocator<sizeof(MMS_ClanStats::ClanStatsEntry), 16*1024> ourEntryAllocator;

	class ClanIdComparer
	{
	public:
		static bool LessThan(const ClanStatsEntry* cseA, 
							 const ClanStatsEntry* cseB) 
		{ 
			return cseA->clanId < cseB->clanId; 
		}
		static bool GreaterThan(const ClanStatsEntry* cseA, 
								const ClanStatsEntry* cseB) 
		{ 
			return cseA->clanId > cseB->clanId; 
		}
		static bool Equals(const ClanStatsEntry* cseA, 
						   const ClanStatsEntry* cseB) 
		{ 
			return cseA->clanId == cseB->clanId; 
		}

		static bool LessThan(const ClanStatsEntry* cseA, 
							 const unsigned int clanIdB) 
		{ 
			return cseA->clanId < clanIdB; 
		}
		static bool GreaterThan(const ClanStatsEntry* cseA, 
								const unsigned int clanIdB) 
		{ 
			return cseA->clanId > clanIdB; 
		}
		static bool Equals(const ClanStatsEntry* cseA, 
						   const unsigned int clanIdB) 
		{ 
			return cseA->clanId == clanIdB; 
		}
	};

	MC_SortedGrowingArray<ClanStatsEntry*> myClanStats; 

	ClanStatsEntry* PrivGetInCache(unsigned int aClanId); 
	void PrivReturnToCache(ClanStatsEntry* someEntry); 

	MT_Mutex globalLock; 

	const static unsigned int NUM_ENTRY_MUTEXES = 64; 
	MT_Mutex entryMutexes[NUM_ENTRY_MUTEXES]; 
	MT_Mutex* PrivGetEntryMutex(); 

	MDB_MySqlConnection* myReadDatabaseConnection;
	MDB_MySqlConnection* myWriteDatabaseConnection;

	typedef MC_HybridArray<ClanStatsEntry*, 1024> DirtyEntryList;

	bool PrivFlushStatsToDb(DirtyEntryList& theEntries); 
	bool PrivFlushMedalsToDb(DirtyEntryList& theEntries); 
	void PrivUnMarkDirty(DirtyEntryList& theEntries);
	bool PrivLockDirtyEntries(DirtyEntryList& theEntries);
	void PrivUnlockDirtyEntries(DirtyEntryList& theEntries);
};

#endif