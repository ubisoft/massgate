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
#include "MMS_ClanStats.h"
#include "MMS_Constants.h"
#include "MMS_InitData.h"
#include "MC_Debug.h"
#include "MT_ThreadingTools.h"
#include "MMS_LadderUpdater.h"
#include "MMS_Statistics.h"
#include "ML_Logger.h"
#include "MMS_MasterServer.h"

MMS_ClanStats* MMS_ClanStats::ourInstance = NULL; 

MMS_ClanStats::MMS_ClanStats(const MMS_Settings& theSettings)
: myTweakablesReReadTimeout(15 * 60 * 1000)
{
	ourInstance = this; 

	myClanStats.Init(4096, 4096); 

	myReadDatabaseConnection = new MDB_MySqlConnection(
		theSettings.WriteDbHost,
		theSettings.WriteDbUser,
		theSettings.WriteDbPassword,
		MMS_InitData::GetDatabaseName(),
		true);
	if (!myReadDatabaseConnection->Connect())
	{
		LOG_FATAL("Could not connect to database.");
		delete myReadDatabaseConnection;
		myReadDatabaseConnection = NULL;
		assert(false);
		return; 
	}	
	myWriteDatabaseConnection = new MDB_MySqlConnection(
		theSettings.WriteDbHost,
		theSettings.WriteDbUser,
		theSettings.WriteDbPassword,
		MMS_InitData::GetDatabaseName(),
		false);
	if (!myWriteDatabaseConnection->Connect())
	{
		LOG_FATAL("Could not connect to database.");
		delete myWriteDatabaseConnection;
		myWriteDatabaseConnection = NULL;
		assert(false);
		return; 
	}	

	myTweakables.Load(myReadDatabaseConnection); 
}

MMS_ClanStats* 
MMS_ClanStats::GetInstance()
{
	return ourInstance; 
}

MMS_ClanStats::~MMS_ClanStats()
{
	ourInstance = NULL; 
}

void 
MMS_ClanStats::Run()
{
	MT_ThreadingTools::SetCurrentThreadName("ClanStats");

	LOG_INFO("Started.");

	while (!StopRequested())
	{
		Sleep(1000);

		unsigned int startTime = GetTickCount(); 

		if(myTweakablesReReadTimeout.HasExpired())
			myTweakables.Load(myReadDatabaseConnection); 

		DirtyEntryList dirtyEntries;

		if (PrivLockDirtyEntries(dirtyEntries))
		{
			PrivFlushStatsToDb(dirtyEntries); 
			PrivFlushMedalsToDb(dirtyEntries); 
			PrivUnMarkDirty(dirtyEntries);
			PrivUnlockDirtyEntries(dirtyEntries);
		}

		MMS_Statistics::GetInstance()->SQLQueryServerTracker(myWriteDatabaseConnection->myNumExecutedQueries); 
		myWriteDatabaseConnection->myNumExecutedQueries = 0; 
		MMS_Statistics::GetInstance()->SQLQueryServerTracker(myReadDatabaseConnection->myNumExecutedQueries); 
		myReadDatabaseConnection->myNumExecutedQueries = 0; 

		GENERIC_LOG_EXECUTION_TIME(MMS_ClanStats::Run(), startTime);
	}

	LOG_INFO("Stopped.");
}

bool 
MMS_ClanStats::PrivLockDirtyEntries(DirtyEntryList& theEntries)
{
	assert(theEntries.Count() == 0);
	MT_MutexLock locker(globalLock);

	for (int i = 0; (i < myClanStats.Count()) && (theEntries.Count()<32); i++)
	{
		ClanStatsEntry* entry = myClanStats[i];
		if (entry->myMutex->TryLock())
		{
			if (entry->dataIsDirty)
			{
				theEntries.Add(entry);
			}
			else if(!entry->usageCount && entry->timeLeftToPurge.HasExpired())
			{
				myClanStats.RemoveItemConserveOrder(i--); 
				entry->myMutex->Unlock();
				delete entry; 
			}
			else
			{
				entry->myMutex->Unlock();
			}
		}
	}
	return theEntries.Count() > 0;
}

void 
MMS_ClanStats::PrivUnlockDirtyEntries(DirtyEntryList& theEntries)
{
	for (int i = 0; i < theEntries.Count(); i++)
		theEntries[i]->myMutex->Unlock();
}

bool 
MMS_ClanStats::PrivFlushStatsToDb(DirtyEntryList& theEntries)
{
	MMS_LadderUpdater* ladderUpdater = MMS_LadderUpdater::GetInstance(); 

	static const char sqlFirstPart[] = "INSERT INTO ClanStats "
		"(clanId,n_matches,n_matcheswon,n_bestmatchstreak,n_currentmatchstreak,n_toursplayed,n_tourswon,n_dommatch,n_dommatchwon,"
		"n_currentdomstreak,n_dommatchwbtd,n_dommatchwbtdstreak,n_assmatch,n_assmatchwon,n_currentassstreak,n_assmatchpd,n_assmatchpdstreak,n_towmatch,n_towmatchwon,"
		"n_currenttowstreak,n_towmatchpp,n_towmatchppstreak,n_unitslost,n_unitskilled,n_nukesdep,n_tacrithits,t_playingAsUSA,t_playingAsUSSR,t_playingAsNATO,"
		"s_scoreTotal,s_scoreByDamagingEnemies,s_scoreByRepairing,s_scoreByTacticalAid,s_scoreByFortifying,s_scorebyInfantry,s_scoreByAir,s_scoreByArmor,s_scoreBySupport,"
		"s_highestAsInfantry,s_highestAsSupport,s_highestAsAir,s_highestAsArmor,s_highestScoreInAMatch) VALUES"; 

	static const char sqlMiddlePart[] = "("
		"%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
		"%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
		"%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
		"%u,%u,%u,%u,%u,%u,%u,%u,%u,"
		"%u,%u,%u,%u),"; 
	static const char sqlLastPart[] = "ON DUPLICATE KEY UPDATE "
		"n_matches=VALUES(n_matches),"
		"n_matcheswon=VALUES(n_matcheswon),"
		"n_bestmatchstreak=VALUES(n_bestmatchstreak),"
		"n_currentmatchstreak=VALUES(n_currentmatchstreak),"
		"n_toursplayed=VALUES(n_toursplayed),"
		"n_tourswon=VALUES(n_tourswon),"
		"n_dommatch=VALUES(n_dommatch),"
		"n_dommatchwon=VALUES(n_dommatchwon),"
		"n_currentdomstreak=VALUES(n_currentdomstreak),"
		"n_dommatchwbtd=VALUES(n_dommatchwbtd),"
		"n_dommatchwbtdstreak=VALUES(n_dommatchwbtdstreak),"
		"n_assmatch=VALUES(n_assmatch),"
		"n_assmatchwon=VALUES(n_assmatchwon),"
		"n_currentassstreak=VALUES(n_currentassstreak),"
		"n_assmatchpd=VALUES(n_assmatchpd),"
		"n_assmatchpdstreak=VALUES(n_assmatchpdstreak),"
		"n_towmatch=VALUES(n_towmatch),"
		"n_towmatchwon=VALUES(n_towmatchwon),"
		"n_currenttowstreak=VALUES(n_currenttowstreak),"
		"n_towmatchpp=VALUES(n_towmatchpp),"
		"n_towmatchppstreak=VALUES(n_towmatchppstreak),"
		"n_unitslost=VALUES(n_unitslost),"
		"n_unitskilled=VALUES(n_unitskilled),"
		"n_nukesdep=VALUES(n_nukesdep),"
		"n_tacrithits=VALUES(n_tacrithits),"
		"t_playingAsUSA=VALUES(t_playingAsUSA),"
		"t_playingAsUSSR=VALUES(t_playingAsUSSR),"
		"t_playingAsNATO=VALUES(t_playingAsNATO),"
		"s_scoreTotal=VALUES(s_scoreTotal),"
		"s_scoreByDamagingEnemies=VALUES(s_scoreByDamagingEnemies),"
		"s_scoreByRepairing=VALUES(s_scoreByRepairing),"
		"s_scoreByTacticalAid=VALUES(s_scoreByTacticalAid),"
		"s_scoreByFortifying=VALUES(s_scoreByFortifying),"
		"s_scoreByInfantry=VALUES(s_scoreByInfantry),"
		"s_scoreBySupport=VALUES(s_scoreBySupport),"
		"s_scoreByAir=VALUES(s_scoreByAir),"
		"s_scoreByArmor=VALUES(s_scoreByArmor),"
		"s_highestAsInfantry=VALUES(s_highestAsInfantry),"
		"s_highestAsSupport=VALUES(s_highestAsSupport),"
		"s_highestAsAir=VALUES(s_highestAsAir),"
		"s_highestAsArmor=VALUES(s_highestAsArmor),"
		"s_highestScoreInAMatch=VALUES(s_highestScoreInAMatch)"; 

	char sql[1024*32]; 
	unsigned int offset = 0;
	bool startNew = true; 

	for(int i = 0; i < theEntries.Count(); i++)
	{
		ClanStatsEntry* entry = theEntries[i]; 

		if(startNew)
		{
			memcpy(sql, sqlFirstPart, sizeof(sqlFirstPart) - 1); 
			offset = sizeof(sqlFirstPart) - 1;
			startNew = false; 
		}

		offset += sprintf(sql + offset, sqlMiddlePart, 
			entry->clanId, 
			entry->matchesPlayed,
			entry->matchesWon,
			entry->bestWinningStreak,
			entry->currentWinningStreak,
			entry->tournamentsPlayed,
			entry->tournamentsWon,
			entry->dominationMatchesPlayed,
			entry->dominationMatchesWon,

			entry->currentDominationStreak,
			entry->dominationMatchesWonByTotalDomination,
			entry->currentTotalDominationStreak,
			entry->assaultMatchesPlayed,
			entry->assaultMatchesWon,
			entry->currentAssaultStreak,
			entry->assaultMatchesPerfectDefense,
			entry->currentPerfectDefenseStreak,
			entry->towMatchesPlayed,
			entry->towMatchesWon,

			entry->currentTowStreak,
			entry->towMatchesPerfectPushes,
			entry->currentPerfectPushStreak,
			entry->numberOfUnitsLost,
			entry->numberOfUnitsKilled,
			entry->numberOfNukesDeployed,
			entry->numberOfTACriticalHits,
			entry->timeAsUSA,
			entry->timeAsUSSR,
			entry->timeAsNATO,

			entry->totalScore,
			entry->scoreByDamagingEnemies,
			entry->scoreByRepairing,
			entry->scoreByTacticalAid,
			entry->scoreByFortifying,
			entry->scoreByInfantry,
			entry->scoreBySupport,
			entry->scoreByAir,
			entry->scoreByArmor, 

			entry->highestScoreAsInfantry,
			entry->highestScoreAsSuppport,
			entry->highestScoreAsAir,
			entry->highestScoreAsArmor, 
			entry->highestScoreInAMatch); 
	}

	if(offset)
	{
		memcpy(sql + offset - 1, sqlLastPart, sizeof(sqlLastPart)); 
		MDB_MySqlQuery query(*myWriteDatabaseConnection); 
		MDB_MySqlResult result; 
		if(!query.Modify(result, sql))
			return false; 
	}

	return true; 
}

bool 
MMS_ClanStats::PrivFlushMedalsToDb(DirtyEntryList& theEntries)
{
	static const char sqlFirstPart[] = "INSERT INTO ClanMedals ("
		"clanId,winStreak,winStreakStars,domSpec,domSpecStars,domEx,domExStars,assSpec,assSpecStars,"
		"assEx,assExStars,towSpec,towSpecStars,towEx,towExStars,hsAwd,hsAwdStars,tcAwd,tcAwdStars"
		") VALUES"; 
	static const char sqlMiddlePart[] = "(%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u),";
	static const char sqlLastPart[] = "ON DUPLICATE KEY UPDATE "
		"winStreak=VALUES(winStreak),"
		"winStreakStars=VALUES(winStreakStars),"
		"domSpec=VALUES(domSpec),"
		"domSpecStars=VALUES(domSpecStars),"
		"domEx=VALUES(domEx),"
		"domExStars=VALUES(domExStars),"
		"assSpec=VALUES(assSpec),"
		"assSpecStars=VALUES(assSpecStars),"
		"assEx=VALUES(assEx),"
		"assExStars=VALUES(assExStars),"
		"towSpec=VALUES(towSpec),"
		"towSpecStars=VALUES(towSpecStars),"
		"towEx=VALUES(towEx),"
		"towExStars=VALUES(towExStars),"
		"hsAwd=VALUES(hsAwd),"
		"hsAwdStars=VALUES(hsAwdStars),"
		"tcAwd=VALUES(tcAwd),"
		"tcAwdStars=VALUES(tcAwdStars)"; 

	char sql[1024*32]; 
	unsigned int offset = 0;
	bool startNew = true; 

	for(int i = 0; i < theEntries.Count(); i++)
	{
		ClanStatsEntry* entry = theEntries[i]; 

		if(startNew)
		{
			memcpy(sql, sqlFirstPart, sizeof(sqlFirstPart) - 1); 
			offset = sizeof(sqlFirstPart) - 1;
			startNew = false; 
		}

		offset += sprintf(sql + offset, sqlMiddlePart, 
			entry->clanId,
			entry->medal_winStreak,
			entry->medal_winStreakStars,
			entry->medal_domSpec,
			entry->medal_domSpecStars,
			entry->medal_domEx,
			entry->medal_domExStars,
			entry->medal_assSpec,
			entry->medal_assSpecStars,
			entry->medal_assEx,
			entry->medal_assExStars,
			entry->medal_towSpec,
			entry->medal_towSpecStars,
			entry->medal_towEx,
			entry->medal_towExStars,
			entry->medal_hsAwd,
			entry->medal_hsAwdStars,
			entry->medal_tcAwd,
			entry->medal_tcAwdStars); 
	}	

	if(offset)
	{
		memcpy(sql + offset - 1, sqlLastPart, sizeof(sqlLastPart)); 
		MDB_MySqlQuery query(*myWriteDatabaseConnection); 
		MDB_MySqlResult result; 
		if(!query.Modify(result, sql))
			return false; 
	}

	return true; 
}

void 
MMS_ClanStats::PrivUnMarkDirty(DirtyEntryList& theEntries)
{
	for(int i = 0; i < theEntries.Count(); i++)
		theEntries[i]->dataIsDirty = false; 
}

bool 
MMS_ClanStats::UpdateClanStats(MMG_Stats::ClanMatchStats& someStats, 
							   unsigned int* someExtraScore)
{
	if(!someStats.clanId)
		return false; 

	ClanStatsEntry* entry = PrivGetInCache(someStats.clanId); 
	entry->myMutex->Lock(); 	
	
	bool good = true;

	if(entry->loadThisEntry)
		good = entry->Load(myReadDatabaseConnection); 
	
	if (good)
	{
		entry->UpdateStats(someStats, myTweakables);
		*someExtraScore = entry->UpdateMedals(someStats, myTweakables); 
	}

	entry->myMutex->Unlock(); 	
	PrivReturnToCache(entry); 

	return good; 
}

bool 
MMS_ClanStats::GetClanStats(MMG_Stats::ClanStatsRsp& someStats, 
							unsigned int aClanId)
{
	ClanStatsEntry* entry = PrivGetInCache(aClanId); 
	entry->myMutex->Lock(); 	
	bool good = true; 

	if(entry->loadThisEntry)
		good = entry->Load(myReadDatabaseConnection); 

	if(good)
		entry->GetStats(someStats); 

	entry->myMutex->Unlock(); 	

	PrivReturnToCache(entry); 

	return good; 
}

bool 
MMS_ClanStats::GetClanMedals(MMG_DecorationProtocol::ClanMedalsRsp& someMedals, 
							 unsigned int aClanId)
{
	ClanStatsEntry* entry = PrivGetInCache(aClanId); 
	entry->myMutex->Lock(); 	
	bool good = true; 

	if(entry->loadThisEntry)
		good = entry->Load(myReadDatabaseConnection); 

	if(good)
		entry->GetMedals(someMedals); 

	entry->myMutex->Unlock(); 	

	PrivReturnToCache(entry); 

	return good; 
}

void 
MMS_ClanStats::InvalidateClan(
	unsigned int aClanId)
{
	unsigned int index = myClanStats.FindInSortedArray2<ClanIdComparer, unsigned int>(aClanId); 
	if(index == -1)
		return; 

	ClanStatsEntry* entry = PrivGetInCache(aClanId); 
	entry->myMutex->Lock(); 	
	entry->Load(myReadDatabaseConnection); 
	entry->myMutex->Unlock(); 	
	PrivReturnToCache(entry); 
}

MT_Mutex*
MMS_ClanStats::PrivGetEntryMutex()
{
	static __declspec(thread) int nextMutexIndex = 0; 
	return &(entryMutexes[(nextMutexIndex++) & (NUM_ENTRY_MUTEXES - 1)]); 
}

MMS_ClanStats::ClanStatsEntry*
MMS_ClanStats::PrivGetInCache(unsigned int aClanId)
{
	MT_MutexLock locker(globalLock); 

	ClanStatsEntry* entry = NULL; 

	unsigned int index = myClanStats.FindInSortedArray2<ClanIdComparer, unsigned int>(aClanId); 
	if (index != -1)
	{
		entry = myClanStats[index];
	}
	else 
	{
		entry = new ClanStatsEntry(); 
		entry->Init(aClanId, PrivGetEntryMutex());
		myClanStats.InsertSorted<ClanIdComparer>(entry);
	}

	assert(entry && "implementation error"); 

	_InterlockedIncrement(&(entry->usageCount)); 

	return entry; 
}

void
MMS_ClanStats::PrivReturnToCache(ClanStatsEntry* someEntry)
{
	_InterlockedDecrement(&(someEntry->usageCount)); 
	assert(someEntry->usageCount != -1 && "implementation error, tried to release entry too many times"); 
}

void 
MMS_ClanStats::ClanStatsEntry::Init(unsigned int aClanId, 
									MT_Mutex* aMutex)
{
	memset(this, 0, sizeof(*this)); 
	clanId = aClanId; 
	loadThisEntry = true; 
	dataIsDirty = false; 
	myMutex = aMutex; 
	timeLeftToPurge.SetTimeout(2 * 60 * 60 * 1000); 
}

void 
MMS_ClanStats::ClanStatsEntry::UpdateStats(MMG_Stats::ClanMatchStats& someStats, 
										   TweakableValues& someMods)
{
	matchesPlayed++; 
	if(someStats.matchWon)
	{
		matchesWon++;
		currentWinningStreak++; 
		if(currentWinningStreak > bestWinningStreak)
			bestWinningStreak = currentWinningStreak; 
	}
	else
	{
		currentWinningStreak = 0; 
	}

	if(someStats.isTournamentMatch)
	{
		tournamentsPlayed++;
		if(someStats.matchWon)
			tournamentsWon++;

	}

	if(someStats.matchType == MATCHTYPE_DOMINATION)
	{
		dominationMatchesPlayed++;
		if(someStats.matchWon)
		{
			dominationMatchesWon++; 
			currentDominationStreak++; 
		}
		else 
		{
			currentDominationStreak = 0; 
		}
		if(someStats.matchWasFlawlessVictory)
		{
			dominationMatchesWonByTotalDomination++;
			currentTotalDominationStreak++; 
		}
		else 
		{
			currentTotalDominationStreak = 0; 
		}
	}

	if(someStats.matchType == MATCHTYPE_ASSAULT)
	{
		assaultMatchesPlayed++; 
		if(someStats.matchWon)
		{
			assaultMatchesWon++; 
			currentAssaultStreak++; 
		}
		else 
		{
			currentAssaultStreak = 0; 
		}
		if(someStats.matchWasFlawlessVictory)
		{
			assaultMatchesPerfectDefense++; 
			currentPerfectDefenseStreak++; 
		}
		else 
		{
			currentPerfectDefenseStreak = 0; 
		}
	}

	if(someStats.matchType == MATCHTYPE_TOW)
	{
		towMatchesPlayed++; 
		if(someStats.matchWon)
		{
			towMatchesWon++; 
			currentTowStreak++; 
		}
		else 
		{
			currentTowStreak = 0; 
		}
		if(someStats.matchWasFlawlessVictory)
		{
			towMatchesPerfectPushes++; 
			currentPerfectPushStreak++;
		}
		else
		{
			currentPerfectPushStreak = 0; 
		}
	}

	numberOfUnitsKilled += someStats.numberOfUnitsKilled; 
	numberOfUnitsLost += someStats.numberOfUnitsLost; 
	numberOfNukesDeployed += someStats.numberOfNukesDeployed; 
	numberOfTACriticalHits += someStats.numberOfTACriticalHits; 
	timeAsUSA += someStats.timeAsUSA; 
	timeAsUSSR += someStats.timeAsUSSR; 
	timeAsNATO += someStats.timeAsNATO; 
	totalScore += someStats.totalScore; 
	if(someStats.totalScore > highestScoreInAMatch)
		highestScoreInAMatch = someStats.totalScore; 
	scoreByDamagingEnemies += someStats.scoreByDamagingEnemies; 
	scoreByRepairing += someStats.scoreByRepairing; 
	scoreByTacticalAid += someStats.scoreByTacticalAid; 
	scoreByFortifying += someStats.scoreByFortifying; 
	scoreByInfantry += someStats.scoreByInfantry; 
	scoreBySupport += someStats.scoreBySupport; 
	scoreByAir += someStats.scoreByAir;
	scoreByArmor += someStats.scoreByArmor; 
	if(someStats.scoreByInfantry > highestScoreAsInfantry)
		highestScoreAsInfantry = someStats.scoreByInfantry; 
	if(someStats.scoreBySupport > highestScoreAsSuppport)
		highestScoreAsSuppport = someStats.scoreBySupport;
	if(someStats.scoreByAir > highestScoreAsAir)
		highestScoreAsAir = someStats.scoreByAir;
	if(someStats.scoreByArmor > highestScoreAsArmor)
		highestScoreAsArmor = someStats.scoreByArmor; 

	dataIsDirty = true; 
}

unsigned int 
MMS_ClanStats::ClanStatsEntry::UpdateMedal(unsigned int* value, 
										   unsigned int* numStars, 
										   unsigned int featsA, 
										   unsigned int featsB, 
										   unsigned int featsC, 
										   unsigned int bronzeTrigger, 
										   unsigned int silverTrigger, 
										   unsigned int goldTrigger, 
										   unsigned int bronzeMod, 
										   unsigned int silverMod, 
										   unsigned int goldMod)
{
	unsigned int extraScore = 0; 

	if(*value == NONE && featsA >= bronzeTrigger)
	{
		*value = BRONZE; 
		extraScore += bronzeMod; 
	}
	if(*value == BRONZE)
		*numStars = __max(*numStars, MC_Clamp<unsigned short>(featsA / bronzeTrigger - 1, 0, 3));
	if (*value == BRONZE && featsB >= silverTrigger)
	{
		*value = SILVER; 
		*numStars = 0;
		extraScore += silverMod; 
	}
	if(*value == SILVER)
		*numStars = __max(*numStars, MC_Clamp<unsigned short>(featsB / silverTrigger - 1, 0, 3));
	if(*value == SILVER && featsC >= goldTrigger)
	{
		*value = GOLD; 
		*numStars = 0;
		extraScore += goldMod; 
	}

	return extraScore; 
}

unsigned int  
MMS_ClanStats::ClanStatsEntry::UpdateMedals(MMG_Stats::ClanMatchStats& someStats, 
											TweakableValues& someMods)
{
	unsigned int extraScore = 0;

	extraScore += UpdateMedal(&medal_winStreak, &medal_winStreakStars, currentWinningStreak, currentWinningStreak, currentWinningStreak, 
		someMods.medal_winStreak_BronzeTrigger, someMods.medal_winStreak_SilverTrigger, someMods.medal_winStreak_GoldTrigger, 
		someMods.medal_winStreak_BronzeScoreMod, someMods.medal_winStreak_SilverScoreMod, someMods.medal_winStreak_GoldScoreMod); 

	extraScore += UpdateMedal(&medal_domSpec, &medal_domSpecStars, dominationMatchesWon, dominationMatchesWon, currentDominationStreak, 
		someMods.medal_domSpec_BronzeTrigger, someMods.medal_domSpec_SilverTrigger, someMods.medal_domSpec_GoldTrigger, 
		someMods.medal_domSpec_BronzeScoreMod, someMods.medal_domSpec_SilverScoreMod, someMods.medal_domSpec_GoldScoreMod); 

	extraScore += UpdateMedal(&medal_domEx, &medal_domExStars, dominationMatchesWonByTotalDomination, dominationMatchesWonByTotalDomination,currentTotalDominationStreak, 
		someMods.medal_domEx_BronzeTrigger, someMods.medal_domEx_SilverTrigger, someMods.medal_domEx_GoldTrigger, 
		someMods.medal_domEx_BronzeScoreMod, someMods.medal_domEx_SilverScoreMod, someMods.medal_domEx_GoldScoreMod); 

	extraScore += UpdateMedal(&medal_assSpec, &medal_assSpecStars, assaultMatchesWon, assaultMatchesWon, currentAssaultStreak, 
		someMods.medal_assSpec_BronzeTrigger, someMods.medal_assSpec_SilverTrigger, someMods.medal_assSpec_GoldTrigger, 
		someMods.medal_assSpec_BronzeScoreMod, someMods.medal_assSpec_SilverScoreMod, someMods.medal_assSpec_GoldScoreMod); 

	extraScore += UpdateMedal(&medal_assEx, &medal_assExStars, assaultMatchesPerfectDefense, assaultMatchesPerfectDefense, currentPerfectDefenseStreak, 
		someMods.medal_assEx_BronzeTrigger, someMods.medal_assEx_SilverTrigger, someMods.medal_assEx_GoldTrigger, 
		someMods.medal_assEx_BronzeScoreMod, someMods.medal_assEx_SilverScoreMod, someMods.medal_assEx_GoldScoreMod); 

	extraScore += UpdateMedal(&medal_towSpec, &medal_towSpecStars, towMatchesWon, towMatchesWon, currentTowStreak, 
		someMods.medal_towSpec_BronzeTrigger, someMods.medal_towSpec_SilverTrigger, someMods.medal_towSpec_GoldTrigger, 
		someMods.medal_towSpec_BronzeScoreMod, someMods.medal_towSpec_SilverScoreMod, someMods.medal_towSpec_GoldScoreMod); 

	extraScore += UpdateMedal(&medal_towEx, &medal_towExStars, towMatchesPerfectPushes, towMatchesPerfectPushes, currentPerfectPushStreak, 
		someMods.medal_towEx_BronzeTrigger, someMods.medal_towEx_SilverTrigger, someMods.medal_towEx_GoldTrigger, 
		someMods.medal_towEx_BronzeScoreMod, someMods.medal_towEx_SilverScoreMod, someMods.medal_towEx_GoldScoreMod); 

	extraScore += UpdateMedal(&medal_hsAwd, &medal_hsAwdStars, totalScore, totalScore, totalScore, 
		someMods.medal_hsAwd_BronzeTrigger, someMods.medal_hsAwd_SilverTrigger, someMods.medal_hsAwd_GoldTrigger, 
		someMods.medal_hsAwd_BronzeScoreMod, someMods.medal_hsAwd_SilverScoreMod, someMods.medal_hsAwd_GoldScoreMod); 

	extraScore += UpdateMedal(&medal_tcAwd, &medal_tcAwdStars, tournamentsPlayed, tournamentsWon, tournamentsWon, 
		someMods.medal_tcAwd_BronzeTrigger, someMods.medal_tcAwd_SilverTrigger, someMods.medal_tcAwd_GoldTrigger, 
		someMods.medal_tcAwd_BronzeScoreMod, someMods.medal_tcAwd_SilverScoreMod, someMods.medal_tcAwd_GoldScoreMod); 

	dataIsDirty = true; 
	return extraScore; 
}

bool 
MMS_ClanStats::ClanStatsEntry::Load(MDB_MySqlConnection* aReadConnection)
{
	MC_StaticString<1024> sql; 

	sql.Format("SELECT *, UNIX_TIMESTAMP(lastUpdated) AS lastMatchPlayed FROM ClanStats WHERE clanId = %u", clanId);

	MDB_MySqlQuery query(*aReadConnection); 
	MDB_MySqlResult result; 
	MDB_MySqlRow row; 

	loadThisEntry = false; 

	if(!query.Ask(result, sql))
		return false; 

	if(!result.GetAffectedNumberOrRows())
		return false; 


	if(result.GetNextRow(row))
	{
		matchesPlayed = row["n_matches"]; 
		matchesWon = row["n_matcheswon"]; 
		bestWinningStreak = row["n_bestmatchstreak"]; 
		currentWinningStreak = row["n_currentmatchstreak"]; 
		tournamentsPlayed = row["n_toursplayed"]; 
		tournamentsWon = row["n_tourswon"]; 
		dominationMatchesPlayed = row["n_dommatch"]; 
		dominationMatchesWon = row["n_dommatchwon"]; 
		currentDominationStreak = row["n_currentdomstreak"]; 
		dominationMatchesWonByTotalDomination = row["n_dommatchwbtd"]; 
		currentTotalDominationStreak = row["n_dommatchwbtdstreak"]; 
		assaultMatchesPlayed = row["n_assmatch"]; 
		assaultMatchesWon = row["n_assmatchwon"]; 
		currentAssaultStreak = row["n_currentassstreak"]; 
		assaultMatchesPerfectDefense = row["n_assmatchpd"]; 
		currentPerfectDefenseStreak = row["n_assmatchpdstreak"]; 
		towMatchesPlayed = row["n_towmatch"]; 
		towMatchesWon = row["n_towmatchwon"]; 
		currentTowStreak = row["n_currenttowstreak"]; 
		towMatchesPerfectPushes = row["n_towmatchpp"]; 
		currentPerfectPushStreak = row["n_towmatchppstreak"]; 
		numberOfUnitsLost = row["n_unitslost"]; 
		numberOfUnitsKilled = row["n_unitskilled"]; 
		numberOfNukesDeployed = row["n_nukesdep"]; 
		numberOfTACriticalHits = row["n_tacrithits"]; 
		timeAsUSA = row["t_playingAsUSA"]; 
		timeAsUSSR = row["t_playingAsUSSR"]; 
		timeAsNATO = row["t_playingAsNATO"]; 
		totalScore = row["s_scoreTotal"]; 
		scoreByDamagingEnemies = row["s_scoreByDamagingEnemies"]; 
		scoreByRepairing = row["s_scoreByRepairing"]; 
		scoreByTacticalAid = row["s_scoreByTacticalAid"]; 
		scoreByFortifying = row["s_scoreByFortifying"]; 
		scoreByArmor = row["s_scoreByArmor"]; 
		scoreByAir = row["s_scoreByAir"]; 
		scoreBySupport = row["s_scoreBySupport"]; 
		scoreByInfantry = row["s_scoreByInfantry"]; 
		highestScoreAsInfantry = row["s_highestAsInfantry"]; 
		highestScoreAsSuppport = row["s_highestAsSupport"]; 
		highestScoreAsAir= row["s_highestAsAir"]; 
		highestScoreAsArmor = row["s_highestAsArmor"]; 
		highestScoreInAMatch = row["s_highestScoreInAMatch"];
		lastMatchPlayed = row["lastMatchPlayed"]; 
	}

	sql.Format("SELECT * FROM ClanMedals WHERE clanId = %u", clanId); 

	if(!query.Ask(result, sql))
		return false; 

	if(!result.GetAffectedNumberOrRows())
		return false; 

	if(result.GetNextRow(row))
	{
		medal_winStreak = row["winStreak"]; 
		medal_winStreakStars = row["winStreakStars"];
		medal_domSpec = row["domSpec"];
		medal_domSpecStars = row["domSpecStars"];
		medal_domEx = row["domEx"];
		medal_domExStars = row["domExStars"];
		medal_assSpec = row["assSpec"];
		medal_assSpecStars = row["assSpecStars"];
		medal_assEx = row["assEx"];
		medal_assExStars = row["assExStars"];
		medal_towSpec = row["towSpec"];
		medal_towSpecStars = row["towSpecStars"];
		medal_towEx = row["towEx"];
		medal_towExStars = row["towExStars"];
		medal_hsAwd = row["hsAwd"];
		medal_hsAwdStars = row["hsAwdStars"];
		medal_tcAwd = row["tcAwd"];
		medal_tcAwdStars = row["tcAwdStars"];
	}

	return true; 
}

void 
MMS_ClanStats::ClanStatsEntry::GetStats(MMG_Stats::ClanStatsRsp& someStats)
{
	someStats.clanId = clanId; 
	someStats.lastMatchPlayed = lastMatchPlayed; 
	someStats.matchesPlayed = matchesPlayed; 
	someStats.matchesWon = matchesWon; 
	someStats.bestWinningStreak = bestWinningStreak; 
	someStats.currentWinningStreak = currentWinningStreak; 
	someStats.currentLadderPosition = MMS_LadderUpdater::GetInstance()->GetClanPosition(clanId); 
	someStats.tournamentsPlayed = tournamentsPlayed; 
	someStats.tournamentsWon = tournamentsWon; 
	someStats.dominationMatchesPlayed = dominationMatchesPlayed; 
	someStats.dominationMatchesWon = dominationMatchesWon; 
	someStats.dominationMatchesWonByTotalDomination = dominationMatchesWonByTotalDomination;
	someStats.assaultMatchesPlayed = assaultMatchesPlayed; 
	someStats.assaultMatchesWon = assaultMatchesWon; 
	someStats.assaultMatchesPerfectDefense = assaultMatchesPerfectDefense;
	someStats.towMatchesPlayed = towMatchesPlayed; 
	someStats.towMatchesWon = towMatchesWon; 
	someStats.towMatchesPerfectPushes = towMatchesPerfectPushes; 
	someStats.numberOfUnitsKilled = numberOfUnitsKilled; 
	someStats.numberOfUnitsLost = numberOfUnitsLost; 
	someStats.numberOfNukesDeployed = numberOfNukesDeployed; 
	someStats.numberOfTACriticalHits = numberOfTACriticalHits; 
	someStats.timeAsUSA = timeAsUSA; 
	someStats.timeAsUSSR = timeAsUSSR; 
	someStats.timeAsNATO = timeAsNATO; 
	someStats.totalScore = totalScore; 
	someStats.highestScoreInAMatch = highestScoreInAMatch; 
	someStats.scoreByDamagingEnemies = scoreByDamagingEnemies; 
	someStats.scoreByRepairing = scoreByRepairing; 
	someStats.scoreByTacticalAid = scoreByTacticalAid; 
	someStats.scoreByFortifying = scoreByFortifying; 
	someStats.scoreByInfantry = scoreByInfantry; 
	someStats.scoreBySupport = scoreBySupport; 
	someStats.scoreByAir = scoreByAir;
	someStats.scoreByArmor = scoreByArmor; 
	someStats.highestScoreAsInfantry = highestScoreAsInfantry; 
	someStats.highestScoreAsSuppport = highestScoreAsSuppport; 
	someStats.highestScoreAsAir = highestScoreAsAir;
	someStats.highestScoreAsArmor = highestScoreAsArmor; 
}

void 
MMS_ClanStats::ClanStatsEntry::GetMedals(MMG_DecorationProtocol::ClanMedalsRsp& someMedals)
{
	someMedals.clanId = clanId; 
	someMedals.Add(medal_winStreak, medal_winStreakStars); 
	someMedals.Add(medal_domSpec, medal_domSpecStars); 
	someMedals.Add(medal_domEx, medal_domExStars); 
	someMedals.Add(medal_assSpec, medal_assSpecStars); 
	someMedals.Add(medal_assEx, medal_assExStars); 
	someMedals.Add(medal_towSpec, medal_towSpecStars); 
	someMedals.Add(medal_towEx, medal_towExStars); 
	someMedals.Add(medal_hsAwd, medal_hsAwdStars); 		
//	someMedals.Add(medal_tcAwd, medal_tcAwdStars); 		
}

MMS_ClanStats::TweakableValues::TweakableValues()
{
	memset(this, 0, sizeof(*this)); 
}

#define READ_MOD_VALUE(V) \
	if(aKey == #V) \
	{ \
		V = row["aValue"]; \
		continue; \
	} \

bool 
MMS_ClanStats::TweakableValues::Load(MDB_MySqlConnection* aReadConnection)
{
	MC_StaticString<1024> sql; 

	sql.Format("SELECT * FROM ClanStatsTweakables");

	MDB_MySqlQuery query(*aReadConnection); 
	MDB_MySqlResult result; 
	MDB_MySqlRow row; 

	if(!query.Ask(result, sql))
		return false; 

	while(result.GetNextRow(row))
	{
		MC_StaticString<256> aKey = row["aKey"];

		READ_MOD_VALUE(medal_winStreak_BronzeTrigger);
		READ_MOD_VALUE(medal_winStreak_SilverTrigger);
		READ_MOD_VALUE(medal_winStreak_GoldTrigger);
		READ_MOD_VALUE(medal_winStreak_BronzeScoreMod);
		READ_MOD_VALUE(medal_winStreak_SilverScoreMod);
		READ_MOD_VALUE(medal_winStreak_GoldScoreMod);
		READ_MOD_VALUE(medal_domSpec_BronzeTrigger);
		READ_MOD_VALUE(medal_domSpec_SilverTrigger);
		READ_MOD_VALUE(medal_domSpec_GoldTrigger);
		READ_MOD_VALUE(medal_domSpec_BronzeScoreMod);
		READ_MOD_VALUE(medal_domSpec_SilverScoreMod);
		READ_MOD_VALUE(medal_domSpec_GoldScoreMod);
		READ_MOD_VALUE(medal_domEx_BronzeTrigger);
		READ_MOD_VALUE(medal_domEx_SilverTrigger);
		READ_MOD_VALUE(medal_domEx_GoldTrigger);
		READ_MOD_VALUE(medal_domEx_BronzeScoreMod);
		READ_MOD_VALUE(medal_domEx_SilverScoreMod);
		READ_MOD_VALUE(medal_domEx_GoldScoreMod);
		READ_MOD_VALUE(medal_assSpec_BronzeTrigger);
		READ_MOD_VALUE(medal_assSpec_SilverTrigger);
		READ_MOD_VALUE(medal_assSpec_GoldTrigger);
		READ_MOD_VALUE(medal_assSpec_BronzeScoreMod);
		READ_MOD_VALUE(medal_assSpec_SilverScoreMod);
		READ_MOD_VALUE(medal_assSpec_GoldScoreMod);
		READ_MOD_VALUE(medal_assEx_BronzeTrigger);
		READ_MOD_VALUE(medal_assEx_SilverTrigger);
		READ_MOD_VALUE(medal_assEx_GoldTrigger);
		READ_MOD_VALUE(medal_assEx_BronzeScoreMod);
		READ_MOD_VALUE(medal_assEx_SilverScoreMod);
		READ_MOD_VALUE(medal_assEx_GoldScoreMod);
		READ_MOD_VALUE(medal_towSpec_BronzeTrigger);
		READ_MOD_VALUE(medal_towSpec_SilverTrigger);
		READ_MOD_VALUE(medal_towSpec_GoldTrigger);
		READ_MOD_VALUE(medal_towSpec_BronzeScoreMod);
		READ_MOD_VALUE(medal_towSpec_SilverScoreMod);
		READ_MOD_VALUE(medal_towSpec_GoldScoreMod);
		READ_MOD_VALUE(medal_towEx_BronzeTrigger);
		READ_MOD_VALUE(medal_towEx_SilverTrigger);
		READ_MOD_VALUE(medal_towEx_GoldTrigger);
		READ_MOD_VALUE(medal_towEx_BronzeScoreMod);
		READ_MOD_VALUE(medal_towEx_SilverScoreMod);
		READ_MOD_VALUE(medal_towEx_GoldScoreMod);
		READ_MOD_VALUE(medal_hsAwd_BronzeTrigger);
		READ_MOD_VALUE(medal_hsAwd_SilverTrigger);
		READ_MOD_VALUE(medal_hsAwd_GoldTrigger);
		READ_MOD_VALUE(medal_hsAwd_BronzeScoreMod);
		READ_MOD_VALUE(medal_hsAwd_SilverScoreMod);
		READ_MOD_VALUE(medal_hsAwd_GoldScoreMod);
		READ_MOD_VALUE(medal_tcAwd_BronzeTrigger);
		READ_MOD_VALUE(medal_tcAwd_SilverTrigger);
		READ_MOD_VALUE(medal_tcAwd_GoldTrigger);
		READ_MOD_VALUE(medal_tcAwd_BronzeScoreMod);
		READ_MOD_VALUE(medal_tcAwd_SilverScoreMod);
		READ_MOD_VALUE(medal_tcAwd_GoldScoreMod);
	}

	return true; 
}

MT_Mutex MMS_ClanStats::ourNewEntryAllocationLock;
MC_SmallObjectAllocator<sizeof(MMS_ClanStats::ClanStatsEntry), 16*1024> MMS_ClanStats::ourEntryAllocator;

void* 
MMS_ClanStats::ClanStatsEntry::operator new(size_t aSize)
{
	MT_MutexLock locker(MMS_ClanStats::ourNewEntryAllocationLock);
	return MMS_ClanStats::ourEntryAllocator.Allocate();
}

void 
MMS_ClanStats::ClanStatsEntry::operator delete(void* aPointer)
{
	MT_MutexLock locker(MMS_ClanStats::ourNewEntryAllocationLock);
	MMS_ClanStats::ourEntryAllocator.Free(aPointer);
}
