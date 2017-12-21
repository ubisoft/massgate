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
#include "MMS_PlayerStats.h"
#include "MMS_Constants.h"
#include "MMS_InitData.h"
#include "MC_Debug.h"
#include "MT_ThreadingTools.h"
#include "MMS_LadderUpdater.h"
#include "MMS_MasterServer.h"
#include "MMS_PersistenceCache.h"
#include "MMS_Statistics.h"
#include "MMS_MasterConnectionHandler.h"
#include "ML_Logger.h"

MMS_PlayerStats* MMS_PlayerStats::ourInstance = NULL; 

MMS_PlayerStats::MMS_PlayerStats(const MMS_Settings& theSettings, MMS_MasterServer* theServer)
: myTweakablesReReadTimeout(15 * 60 * 1000)
, myServer(theServer)
, mySettings(theSettings)
{
	ourInstance = this; 

	myPlayerStats.Init(4096, 4096); 

	myReadDatabaseConnection = new MDB_MySqlConnection(
		theSettings.ReadDbHost,
		theSettings.ReadDbUser,
		theSettings.ReadDbPassword,
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

MMS_PlayerStats* 
MMS_PlayerStats::GetInstance()
{
	return ourInstance; 
}


MMS_PlayerStats::~MMS_PlayerStats()
{
	ourInstance = NULL; 
}

static MT_SkipLock ourSkipLock;

void 
MMS_PlayerStats::Update(MMS_MasterConnectionHandler* aConnectionHandler)
{
	// We are called after every message from every connection handler (since we must be able to broadcast profile changes
	// we can use the connection handler for that

	if (!ourSkipLock.TryLock())
	{
		// Someone else is updating
		return;
	}

	static MC_EggClockTimer myUpdateInterval(1000*1);
	if (!myUpdateInterval.HasExpired())
	{
		ourSkipLock.Unlock();
		return;
	}

	if(myTweakablesReReadTimeout.HasExpired())
		myTweakables.Load(myReadDatabaseConnection); 

	for (int i = 0; i < 10; i++)
	{
		DirtyEntryList dirtyEntries;
		if (PrivLockDirtyEntries(dirtyEntries))
		{
			const unsigned int START_TIME = GetTickCount();

			PrivFlushStatsToDb(dirtyEntries); 
			PrivFlushMedalsToDb(dirtyEntries); 
			PrivFlushBadgesToDb(dirtyEntries);
			PrivInvalidateLuts(dirtyEntries, aConnectionHandler); 
			PrivUnMarkDirty(dirtyEntries);

			PrivUnlockDirtyEntries(dirtyEntries);

			MMS_MasterServer::GetInstance()->AddSample("MMS_PlayerStats_FlushToDatabase", GetTickCount() - START_TIME); 
		}
		else
		{
			break;
		}
	}

	MMS_Statistics::GetInstance()->SQLQueryServerTracker(myWriteDatabaseConnection->myNumExecutedQueries); 
	myWriteDatabaseConnection->myNumExecutedQueries = 0; 
	MMS_Statistics::GetInstance()->SQLQueryServerTracker(myReadDatabaseConnection->myNumExecutedQueries); 
	myReadDatabaseConnection->myNumExecutedQueries = 0; 

	ourSkipLock.Unlock();
}

bool 
MMS_PlayerStats::PrivLockDirtyEntries(DirtyEntryList& theEntries)
{
	assert(theEntries.Count() == 0);
	MT_MutexLock locker(globalLock);

	for (int i = 0; (i < myPlayerStats.Count()) && (theEntries.Count()<32); i++)
	{
		PlayerStatsEntry* entry = myPlayerStats[i];
		if (entry->myMutex->TryLock())
		{
			if (entry->dataIsDirty)
			{
				theEntries.Add(entry);
			}
			else if(!entry->usageCount && entry->timeLeftToPurge.HasExpired())
			{
				myPlayerStats.RemoveItemConserveOrder(i--); 
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
MMS_PlayerStats::PrivUnlockDirtyEntries(DirtyEntryList& theEntries)
{
	for (int i = 0; i < theEntries.Count(); i++)
		theEntries[i]->myMutex->Unlock();
}

bool 
MMS_PlayerStats::PrivFlushStatsToDb(DirtyEntryList& theEntries)
{
	MMS_LadderUpdater* ladderUpdater = MMS_LadderUpdater::GetInstance(); 

	static const char sqlFirstPart[] = "INSERT INTO PlayerStats ("
		"profileId,rank,maxLadderPercent,lastUpdated,sc_tot,sc_inf,sc_highinf,sc_sup,sc_highsup,"
		"sc_arm,sc_higharm,sc_air,sc_highair,sc_damen,sc_ta,sc_cpc,sc_rep,sc_fort,"
		"sc_tk,sc_highest,t_USA,t_USSR,t_NATO,t_sup,t_arm,t_air,t_inf,n_matches,n_matcheswon,"
		"n_matcheslost,n_dommatches,n_dommatcheswon,n_domstreak,n_towmatches,n_towmatcheswon,"
		"n_towstreak,n_assmatches,n_assmatcheswon,n_assstreak,n_cwinstr,n_bwinstr,n_pptw,n_pptwstreak,"
		"n_pdas,n_pdasstreak,n_wbtd,n_wbtdstreak,n_ukills,n_ulost,n_cpc,n_rps,n_taps,n_nukes,"
		"n_nukestreak,n_tach,n_bplayer,n_bplayerstr,n_binf,n_binfstr,n_bsup,n_bsupstr,n_barm,n_barmstr,"
		"n_bair,n_bairstr,n_statpad) VALUES"; 
	static const char sqlMiddlePart[] = "("
		"%u,%u,%u,NOW(),%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
		"%u,%u,%u,%u,%u,%u,%u,%u,%u,"
		"%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
		"%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
		"%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
		"%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
		"%u,%u,%u),"; 
	static const char sqlLastPart[] = "ON DUPLICATE KEY UPDATE " 
		"rank=GREATEST(rank,VALUES(rank)),maxLadderPercent=VALUES(maxLadderPercent),sc_tot=VALUES(sc_tot),"
		"sc_inf=VALUES(sc_inf),sc_highinf=VALUES(sc_highinf),"
		"sc_sup=VALUES(sc_sup),sc_highsup=VALUES(sc_highsup),"
		"sc_arm=VALUES(sc_arm),sc_higharm=VALUES(sc_higharm),"
		"sc_air=VALUES(sc_air),sc_highair=VALUES(sc_highair),"
		"sc_damen=VALUES(sc_damen),sc_ta=VALUES(sc_ta),"
		
		"sc_cpc=VALUES(sc_cpc),sc_rep=VALUES(sc_rep),sc_fort=VALUES(sc_fort),"
		"sc_tk=VALUES(sc_tk),sc_highest=VALUES(sc_highest),"
		"t_USA=VALUES(t_USA),t_USSR=VALUES(t_USSR),t_NATO=VALUES(t_NATO),t_sup=VALUES(t_sup),"

		"t_arm=VALUES(t_arm),t_air=VALUES(t_air),t_inf=VALUES(t_inf),n_matches=VALUES(n_matches),"
		"n_matcheswon=VALUES(n_matcheswon),n_matcheslost=VALUES(n_matcheslost),"
		"n_dommatches=VALUES(n_dommatches),n_dommatcheswon=VALUES(n_dommatcheswon),n_domstreak=VALUES(n_domstreak),"
		"n_towmatches=VALUES(n_towmatches),n_towmatcheswon=VALUES(n_towmatcheswon),n_towstreak=VALUES(n_towstreak),"
		"n_assmatches=VALUES(n_assmatches),n_assmatcheswon=VALUES(n_assmatcheswon),n_assstreak=VALUES(n_assstreak),"
		
		"n_cwinstr=VALUES(n_cwinstr),n_bwinstr=VALUES(n_bwinstr),"
		"n_pptw=VALUES(n_pptw),n_pptwstreak=VALUES(n_pptwstreak),n_pdas=VALUES(n_pdas),n_pdasstreak=VALUES(n_pdasstreak),"
		"n_wbtd=VALUES(n_wbtd),n_wbtdstreak=VALUES(n_wbtdstreak),n_ukills=VALUES(n_ukills),"

		"n_ulost=VALUES(n_ulost),n_cpc=VALUES(n_cpc),"
		"n_rps=VALUES(n_rps),n_taps=VALUES(n_taps),"
		"n_nukes=VALUES(n_nukes),n_nukestreak=VALUES(n_nukestreak),"
		
		"n_tach=VALUES(n_tach),"
		"n_bplayer=VALUES(n_bplayer),n_bplayerstr=VALUES(n_bplayerstr),n_binf=VALUES(n_binf),n_binfstr=VALUES(n_binfstr),"
		"n_bsup=VALUES(n_bsup),n_bsupstr=VALUES(n_bsupstr),n_barm=VALUES(n_barm),n_barmstr=VALUES(n_barmstr),"

		"n_bair=VALUES(n_bair),n_bairstr=VALUES(n_bairstr),n_statpad=VALUES(n_statpad)"; 

	char sql[1024*32]; 
	unsigned int offset = 0;
	bool startNew = true; 

	for(int i = 0; i < theEntries.Count(); i++)
	{
		PlayerStatsEntry* pse = theEntries[i]; 
		pse->pushPromotedPlayer = false;

		if(startNew)
		{
			memcpy(sql, sqlFirstPart, sizeof(sqlFirstPart) - 1); 
			offset = sizeof(sqlFirstPart) - 1;
			startNew = false; 
		}

		unsigned int ladderPercent = ladderUpdater->GetPlayerPercentage(pse->profileId); 
		unsigned int rank = 0;
		if(ladderPercent == -1)
			ladderPercent = 0; 
		else
		{
			rank = MMS_PersistenceCache::GetRank(ladderPercent, pse->scoreTotal);
			if (pse->rank < rank)
			{
				// Profile was promoted!
				pse->rank = rank;
				pse->pushPromotedPlayer = true;
			}
		}
		offset += sprintf(sql + offset, sqlMiddlePart, 
			pse->profileId,
			rank,
			ladderPercent,  
			pse->scoreTotal,
			pse->scoreAsInfantry,
			pse->highScoreAsInfantry,
			pse->scoreAsSupport,
			pse->highScoreAsSupport,
			pse->scoreAsArmor,
			pse->highScoreAsArmor,
			pse->scoreAsAir,
			pse->highScoreAsAir,
			pse->scoreByDamagingEnemies,
			pse->scoreByUsingTacticalAids,
			pse->scoreByCapturingCommandPoints,

			pse->scoreByRepairing,
			pse->scoreByFortifying,
			pse->scoreLostByKillingFriendly,
			pse->highestScore,

			pse->timePlayedAsUSA,
			pse->timePlayedAsUSSR,
			pse->timePlayedAsNATO,
			pse->timePlayedAsSupport,
			pse->timePlayedAsArmor,

			pse->timePlayedAsAir,
			pse->timePlayedAsInfantry,
			pse->numberOfMatches,
			pse->numberOfMatchesWon,
			pse->numberOfMatchesLost,
			pse->numberOfDominationMatches,
			pse->numberOfDominationMatchesWon,
			pse->numberOfDominationStreak,
			pse->numberOfTugOfWarMatches,
			pse->numberOfTugOfWarMatchesWon,
			pse->numberOfTugOfWarStreak,
			pse->numberOfAssaultMatches,
			pse->numberOfAssaultMatchesWon,
			pse->numberOfAssaultStreak,

			pse->currentWinningStreak,
			pse->bestWinningStreak,
			pse->numberOfPerfectPushesInTugOfWarMatch,
			pse->numberOfPerfectPushesInTugOfWarStreak,
			pse->numberOfPerfectDefendsInAssultMatch,
			pse->numberOfPerfectDefendsInAssultStreak,
			pse->numberOfMatchesWonByTotalDomination,
			pse->numberOfTotalDominationStreak,
			pse->numberOfUnitsKilled,
			pse->numberOfUnitsLost,

			pse->numberOfCommandPointCaptures,
			pse->numberOfReinforcementPointsSpent,
			pse->numberOfTacticalAidPointsSpent,
			pse->numberOfNukesDeployed,
			pse->numberOfNukesDeployedStreak,
			pse->numberOfTacticalAidCristicalHits,

			pse->numberOftimesAsBestPlayer,
			pse->currentBestPlayerStreak,
			pse->numberOftimesAsBestInfPlayer,
			pse->currentBestInfStreak,
			pse->numberOftimesAsBestSupPlayer,
			pse->currentBestSupStreak,
			pse->numberOftimesAsBestArmPlayer,
			pse->currentBestArmStreak,
			pse->numberOftimesAsBestAirPlayer,

			pse->currentBestAirStreak,
			pse->numStatPadsDetected

			); 
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
MMS_PlayerStats::PrivFlushBadgesToDb(DirtyEntryList& theEntries)
{
	static const char sqlFirstPart[] = "INSERT INTO PlayerBadges ("
		"profileId,infSp,infSpStars,airSp,airSpStars,armSp,armSpStars,supSp,supSpStars,scoreAch,scoreAchStars,cpAch,cpAchStars,fortAch,fortAchStars,"
		"mgAch,mgAchStars,matchAch,matchAchStars,USAAch,USAAchStars,USSRAch,USSRAchStars,NATOAch,NATOAchStars,preOrdAch,preOrdAchStars,reqruitAFriendAch,reqruitAFriendAchStars"
		") VALUES"; 
		static const char sqlMiddlePart[] = "(%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u),"; 
	static const char sqlLastPart[] = "ON DUPLICATE KEY UPDATE " 
		"infSp=VALUES(infSp),"
		"infSpStars=VALUES(infSpStars),"
		"airSp=VALUES(airSp),"
		"airSpStars=VALUES(airSpStars),"
		"armSp=VALUES(armSp),"
		"armSpStars=VALUES(armSpStars),"
		"supSp=VALUES(supSp),"
		"supSpStars=VALUES(supSpStars),"
		"scoreAch=VALUES(scoreAch),"
		"scoreAchStars=VALUES(scoreAchStars),"
		"cpAch=VALUES(cpAch),"
		"cpAchStars=VALUES(cpAchStars),"
		"fortAch=VALUES(fortAch),"
		"fortAchStars=VALUES(fortAchStars),"
		"mgAch=VALUES(mgAch),"
		"mgAchStars=VALUES(mgAchStars),"
		"matchAch=VALUES(matchAch),"
		"matchAchStars=VALUES(matchAchStars),"
		"USAAch=VALUES(USAAch),"
		"USAAchStars=VALUES(USAAchStars),"
		"USSRAch=VALUES(USSRAch),"
		"USSRAchStars=VALUES(USSRAchStars),"
		"NATOAch=VALUES(NATOAch),"
		"NATOAchStars=VALUES(NATOAchStars),"
		"preOrdAch=VALUES(preOrdAch),"
		"preOrdAchStars=VALUES(preOrdAchStars)," 
		"reqruitAFriendAch=VALUES(reqruitAFriendAch),"
		"reqruitAFriendAchStars=VALUES(reqruitAFriendAchStars)"; 

	char sql[32*1024]; 
	unsigned int offset = 0;
	bool startNew = true; 

	for(int i = 0; i < theEntries.Count(); i++)
	{
		PlayerStatsEntry* pse = theEntries[i]; 

		if(startNew)
		{
			memcpy(sql, sqlFirstPart, sizeof(sqlFirstPart) - 1); 
			offset = sizeof(sqlFirstPart) - 1;
			startNew = false; 
		}

		offset += sprintf(sql + offset, sqlMiddlePart, 
			pse->profileId,
			pse->badge_infSp,
			pse->badge_infSpStars,
			pse->badge_airSp,
			pse->badge_airSpStars,
			pse->badge_armSp,
			pse->badge_armSpStars,
			pse->badge_supSp,
			pse->badge_supSpStars,
			pse->badge_scoreAch,
			pse->badge_scoreAchStars,
			pse->badge_cpAch,
			pse->badge_cpAchStars,
			pse->badge_fortAch,
			pse->badge_fortAchStars,
			pse->badge_mgAch,
			pse->badge_mgAchStars,
			pse->badge_matchAch,
			pse->badge_matchAchStars,
			pse->badge_USAAch,
			pse->badge_USAAchStars,
			pse->badge_USSRAch,
			pse->badge_USSRAchStars,
			pse->badge_NATOAch,
			pse->badge_NATOAchStars,
			pse->badge_preOrdAch,
			pse->badge_preOrdAchStars,
			pse->badge_reqruitAFriendAch,
			pse->badge_reqruitAFriendAchStars); 

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
MMS_PlayerStats::PrivFlushMedalsToDb(DirtyEntryList& theEntries)
{
	static const char sqlFirstPart[] = "INSERT INTO PlayerMedals ("
		"profileId,infAch,infAchStars,airAch,airAchStars,armorAch,armorAchStars,supAch,supAchStars,scoreAch,scoreAchStars,"
		"taAch,taAchStars,infComEx,infComExStars,airComEx,airComExStars,armorComEx,armorComExStars,supComEx,supComExStars,"
		"winStreak,winStreakStars,domSpec,domSpecStars,domEx,domExStars,assSpec,assSpecStars,assEx,assExStars,towSpec,towSpecStars,"
		"towEx,towExStars,nukeSpec,nukeSpecStars,highDec,highDecStars"
		") VALUES"; 
	static const char sqlMiddlePart[] = "(%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u),"; 
	static const char sqlLastPart[] = "ON DUPLICATE KEY UPDATE "
		"infAch=VALUES(infAch),"
		"infAchStars=VALUES(infAchStars),"
		"airAch=VALUES(airAch),"
		"airAchStars=VALUES(airAchStars),"
		"armorAch=VALUES(armorAch),"
		"armorAchStars=VALUES(armorAchStars),"
		"supAch=VALUES(supAch),"
		"supAchStars=VALUES(supAchStars),"
		"scoreAch=VALUES(scoreAch),"
		"scoreAchStars=VALUES(scoreAchStars),"
		"taAch=VALUES(taAch),"
		"taAchStars=VALUES(taAchStars),"
		"infComEx=VALUES(infComEx),"
		"infComExStars=VALUES(infComExStars),"
		"airComEx=VALUES(airComEx),"
		"airComExStars=VALUES(airComExStars),"
		"armorComEx=VALUES(armorComEx),"
		"armorComExStars=VALUES(armorComExStars),"
		"supComEx=VALUES(supComEx),"
		"supComExStars=VALUES(supComExStars),"
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
		"nukeSpec=VALUES(nukeSpec),"
		"nukeSpecStars=VALUES(nukeSpecStars),"
		"highDec=VALUES(highDec),"
		"highDecStars=VALUES(highDecStars)"; 

	char sql[32*1024]; 
	unsigned int offset = 0;
	bool startNew = true; 

	for(int i = 0; i < theEntries.Count(); i++)
	{
		PlayerStatsEntry* pse = theEntries[i]; 

		if(startNew)
		{
			memcpy(sql, sqlFirstPart, sizeof(sqlFirstPart) - 1); 
			offset = sizeof(sqlFirstPart) - 1;
			startNew = false; 
		}

		offset += sprintf(sql + offset, sqlMiddlePart, 
			pse->profileId,
			pse->medal_infAch,
			pse->medal_infAchStars,
			pse->medal_airAch,
			pse->medal_airAchStars,
			pse->medal_armAch,
			pse->medal_armAchStars,
			pse->medal_supAch,
			pse->medal_supAchStars,
			pse->medal_scoreAch,
			pse->medal_scoreAchStars,
			pse->medal_taAch,
			pse->medal_taAchStars,
			pse->medal_infComEx,
			pse->medal_infComExStars,
			pse->medal_airComEx,
			pse->medal_airComExStars,
			pse->medal_armComEx,
			pse->medal_armComExStars,
			pse->medal_supComEx,
			pse->medal_supComExStars,
			pse->medal_winStreak,
			pse->medal_winStreakStars,
			pse->medal_domSpec,
			pse->medal_domSpecStars,
			pse->medal_domEx,
			pse->medal_domExStars,
			pse->medal_assSpec,
			pse->medal_assSpecStars,
			pse->medal_assEx,
			pse->medal_assExStars,
			pse->medal_towSpec,
			pse->medal_towSpecStars,
			pse->medal_towEx,
			pse->medal_towExStars,
			pse->medal_nukeSpec,
			pse->medal_nukeSpecStars,
			pse->medal_highDec,
			pse->medal_highDecStars); 

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
MMS_PlayerStats::PrivInvalidateLuts(DirtyEntryList& theEntries, MMS_MasterConnectionHandler* currentConnectionHandler)
{
	for (int i = 0; i < theEntries.Count(); i++)
	{
		if (theEntries[i]->pushPromotedPlayer)
		{
			theEntries[i]->pushPromotedPlayer = false;
			ClientLutRef clr = MMS_PersistenceCache::GetClientLut(myReadDatabaseConnection, theEntries[i]->profileId);
			if (clr.IsGood())
			{
				clr->rank = theEntries[i]->rank;
				currentConnectionHandler->myMessagingHandler.ReleaseAndMulticastUpdatedProfileInfo(clr);
			}
		}
	}

	return true; 
}

void 
MMS_PlayerStats::PrivUnMarkDirty(DirtyEntryList& theEntries)
{
	for(int i = 0; i < theEntries.Count(); i++)
		theEntries[i]->dataIsDirty = false; 
}

#define LOG_EXECUTION_TIME(MESSAGE, START_TIME) \
{ unsigned int currentTime = GetTickCount(); \
	MMS_MasterServer::GetInstance()->AddSample("SERVERTRACKER:" __FUNCTION__ #MESSAGE, currentTime - START_TIME); \
	START_TIME = currentTime; } 

bool 
MMS_PlayerStats::UpdatePlayerStats(MMG_Stats::PlayerMatchStats& someStats)
{
	if(!someStats.profileId)
		return false; 

	unsigned int startTime = GetTickCount();

	PlayerStatsEntry* entry = PrivGetInCache(someStats.profileId); 
	entry->myMutex->Lock(); 	

	bool good = true;

	if(entry->loadThisEntry)
		good = entry->Load(myReadDatabaseConnection); 

	if (good)
	{
		if (someStats.timePlayedAsNATO+someStats.timePlayedAsUSA+someStats.timePlayedAsUSSR > 5*60)
		{
			bool suspectEntry = false;
			// Log players that may be statpadding
			if (someStats.scoreTotal > mySettings.PlayerScoreSuspect && entry->rank > 10)
			{
				suspectEntry = true; // Officer got more than 3000 points
			}

			if ((someStats.numberOfCommandPointCaptures>10) && (someStats.scoreByDamagingEnemies < 100))
			{
				suspectEntry = true; // Player has > 10 captures, but less than 100 score for damaging enemies
			}

			float damenperkill = someStats.scoreByDamagingEnemies / (someStats.numberOfUnitsKilled+1.0f);
			if (damenperkill > 100.0f)
			{
				suspectEntry = true; // Player got more than 100pts per killed unit (average is 26)
			}

			if (suspectEntry)
			{
				// Write entry to database right now
				PrivLogSuspectEntry(entry, someStats);
			}
		}
		if (someStats.scoreTotal > mySettings.PlayerScoreCap) // SCORE CAP, STATS PADDERS BEWARE
		{
			LOG_INFO("Ignoring statspadder %u", someStats.profileId);
			entry->numStatPadsDetected++;
			entry->dataIsDirty = true;
		}
		else if (entry->ignoreStatsUntil > time(NULL))
		{
			LOG_INFO("Ignoring stats from previous padder %u", someStats.profileId);
		}
		else
		{
			unsigned int extraScore = 0;

			entry->UpdateStats(someStats, myTweakables);
			extraScore += entry->UpdateMedals(someStats, myTweakables); 
			extraScore += entry->UpdateBadges(someStats, myTweakables); 
			MMS_LadderUpdater* ladderUpdater = MMS_LadderUpdater::GetInstance(); 
			if(someStats.matchWon)
				ladderUpdater->AddScoreToPlayer(someStats.profileId, (unsigned short)((float)someStats.scoreTotal * mySettings.PlayerWinningScoreMultiplier));
			else
				ladderUpdater->AddScoreToPlayer(someStats.profileId, someStats.scoreTotal);

			entry->scoreTotal += extraScore;
		}
	}

	entry->myMutex->Unlock(); 	
	PrivReturnToCache(entry); 

	LOG_EXECUTION_TIME(, startTime);

	return good; 
}

bool 
MMS_PlayerStats::GetPlayerStats(MMG_Stats::PlayerStatsRsp& someStats, unsigned int aProfileId)
{
	PlayerStatsEntry* entry = PrivGetInCache(aProfileId); 
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
MMS_PlayerStats::GetPlayerMedals(MMG_DecorationProtocol::PlayerMedalsRsp& someMedals, unsigned int aProfileId)
{
	PlayerStatsEntry* entry = PrivGetInCache(aProfileId); 
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

bool 
MMS_PlayerStats::GetPlayerBadges(MMG_DecorationProtocol::PlayerBadgesRsp& someBadges, unsigned int aProfileId)
{
	PlayerStatsEntry* entry = PrivGetInCache(aProfileId); 
	entry->myMutex->Lock(); 	
	bool good = true; 

	if(entry->loadThisEntry)
		good = entry->Load(myReadDatabaseConnection); 

	if(good)
	{
		const unsigned int currentTime = (unsigned int)time(NULL);
		// Adjust for database time differences
		const unsigned int memberTime = (currentTime > entry->massgateMemberSince) ? currentTime - entry->massgateMemberSince : 0; 
		if(entry->badge_mgAch == PlayerStatsEntry::NONE && memberTime >= myTweakables.badge_mgAch_TimeForBronze)
			entry->badge_mgAch = PlayerStatsEntry::BRONZE; 
		if(entry->badge_mgAch == PlayerStatsEntry::BRONZE)
			entry->badge_mgAchStars = MC_Clamp<unsigned short>(memberTime / myTweakables.badge_mgAch_TimeForBronze - 1, 0, 3);
		if (entry->badge_mgAch == PlayerStatsEntry::BRONZE && memberTime >= myTweakables.badge_mgAch_TimeForSilver)
			entry->badge_mgAch = PlayerStatsEntry::SILVER; 
		if(entry->badge_mgAch == PlayerStatsEntry::SILVER)
			entry->badge_mgAchStars = MC_Clamp<unsigned short>(memberTime / myTweakables.badge_mgAch_TimeForSilver - 1, 0, 3);
		if(entry->badge_mgAch == PlayerStatsEntry::SILVER && memberTime >= myTweakables.badge_mgAch_TimeForGold)
			entry->badge_mgAch = PlayerStatsEntry::GOLD; 

		entry->GetBadges(someBadges); 
	}

	entry->myMutex->Unlock(); 	

	PrivReturnToCache(entry); 

	return good; 
}

bool 
MMS_PlayerStats::GetRankAndTotalScore(
	unsigned int aProfileId, 
	unsigned int& aRank, 
	unsigned int& aTotalScore)
{
	PlayerStatsEntry* entry = PrivGetInCache(aProfileId); 
	entry->myMutex->Lock(); 	
	bool good = true; 

	if(entry->loadThisEntry)
		good = entry->Load(myReadDatabaseConnection); 

	if(good)
	{
		aRank = entry->rank; 
		aTotalScore = entry->scoreTotal; 
	}

	entry->myMutex->Unlock(); 	

	PrivReturnToCache(entry); 

	return good; 	
}


bool
MMS_PlayerStats::GivePreorderMedal(unsigned int aProfileId)
{
	PlayerStatsEntry* entry = PrivGetInCache(aProfileId);
	entry->myMutex->Lock();

	bool good = true; 
	if(entry->loadThisEntry)
		good = entry->Load(myReadDatabaseConnection); 

	if(good)
	{
		entry->badge_preOrdAch = 1;
		entry->dataIsDirty = true;
	}

	entry->myMutex->Unlock();
	PrivReturnToCache(entry);
	return true;
}

bool
MMS_PlayerStats::GiveRecruitmentMedal(unsigned int aProfileId)
{
	PlayerStatsEntry* entry = PrivGetInCache(aProfileId);
	entry->myMutex->Lock();

	bool good = true; 
	if(entry->loadThisEntry)
		good = entry->Load(myReadDatabaseConnection); 

	if(good)
	{
		entry->badge_reqruitAFriendAch++;
		entry->dataIsDirty = true;
	}

	entry->myMutex->Unlock();
	PrivReturnToCache(entry);
	return true;
}

void 
MMS_PlayerStats::InvalidateProfile(unsigned int aProfileId)
{
	PlayerStatsEntry* entry = PrivGetInCache(aProfileId);
	entry->myMutex->Lock();

	entry->Load(myReadDatabaseConnection);

	entry->myMutex->Unlock();
	PrivReturnToCache(entry);
}

MT_Mutex* 
MMS_PlayerStats::PrivGetEntryMutex()
{
	static __declspec(thread) int nextMutexIndex = 0; 
	return &(entryMutexes[(nextMutexIndex++) & (NUM_ENTRY_MUTEXES - 1)]); 
}

MMS_PlayerStats::PlayerStatsEntry* 
MMS_PlayerStats::PrivGetInCache(unsigned int aProfileId)
{
	MT_MutexLock locker(globalLock); 

	PlayerStatsEntry* entry = NULL; 

	unsigned int index = myPlayerStats.FindInSortedArray2<ProfileIdComparer, unsigned int>(aProfileId); 
	if (index != -1)
	{
		entry = myPlayerStats[index];
	}
	else 
	{
		entry = new PlayerStatsEntry(); 
		entry->Init(aProfileId, PrivGetEntryMutex());
		myPlayerStats.InsertSorted<ProfileIdComparer>(entry);
	}

	assert(entry && "implementation error"); 

	_InterlockedIncrement(&(entry->usageCount)); 

	return entry; 
}

void 
MMS_PlayerStats::PrivReturnToCache(PlayerStatsEntry* someEntry)
{
	_InterlockedDecrement(&(someEntry->usageCount)); 
	assert(someEntry->usageCount != -1 && "implementation error, tried to release entry too many times"); 
}

void 
MMS_PlayerStats::PlayerStatsEntry::Init(unsigned int aProfileId, MT_Mutex* aMutex)
{
	memset(this, 0, sizeof(*this)); 
	profileId = aProfileId; 
	loadThisEntry = true; 
	dataIsDirty = false; 
	myMutex = aMutex; 
	timeLeftToPurge.SetTimeout(2 * 60 * 60 * 1000); 
}

void 
MMS_PlayerStats::PlayerStatsEntry::UpdateStats(MMG_Stats::PlayerMatchStats& someStats, 
											   TweakableValues& someMods)
{
	lastMatchPlayed = (unsigned int) time(NULL); 
	scoreTotal += someStats.scoreTotal; 
	if(someStats.scoreTotal > highestScore)
		highestScore = someStats.scoreTotal; 
	scoreAsInfantry += someStats.scoreAsInfantry; 
	if(someStats.scoreAsInfantry > highScoreAsInfantry)
		highScoreAsInfantry = someStats.scoreAsInfantry; 
	scoreAsSupport += someStats.scoreAsSupport; 
	if(someStats.scoreAsSupport > highScoreAsSupport)
		highScoreAsSupport = someStats.scoreAsSupport; 
	scoreAsArmor += someStats.scoreAsArmor; 
	if(someStats.scoreAsArmor > highScoreAsArmor)
		highScoreAsArmor = someStats.scoreAsArmor; 
	scoreAsAir += someStats.scoreAsAir; 
	if(someStats.scoreAsAir > highScoreAsAir)
		highScoreAsAir = someStats.scoreAsAir; 
	scoreByDamagingEnemies += someStats.scoreByDamagingEnemies; 
	scoreByUsingTacticalAids += someStats.scoreByUsingTacticalAids; 
	scoreByCapturingCommandPoints += someStats.scoreByCommandPointCaptures; 
	scoreByRepairing += someStats.scoreByRepairing; 
	scoreByFortifying += someStats.scoreByFortifying; 
	scoreLostByKillingFriendly += someStats.scoreLostByKillingFriendly; 

	timePlayedAsUSA += someStats.timePlayedAsUSA; 
	timePlayedAsUSSR += someStats.timePlayedAsUSSR; 
	timePlayedAsNATO += someStats.timePlayedAsNATO; 
	timePlayedAsInfantry += someStats.timePlayedAsInfantry; 
	timePlayedAsSupport += someStats.timePlayedAsSupport; 
	timePlayedAsArmor += someStats.timePlayedAsArmor; 
	timePlayedAsAir += someStats.timePlayedAsAir; 

	numberOfMatches++; 
	numberOfMatchesWon += someStats.matchWon; 
	numberOfMatchesLost += someStats.matchLost; 
	numberOfUnitsKilled += someStats.numberOfUnitsKilled;
	numberOfUnitsLost += someStats.numberOfUnitsLost;
	numberOfReinforcementPointsSpent += someStats.numberOfReinforcementPointsSpent;
	numberOfTacticalAidPointsSpent += someStats.numberOfTacticalAidPointsSpent;
	numberOfTacticalAidCristicalHits += someStats.numberOfTacticalAidCriticalHits;
	numberOfCommandPointCaptures += someStats.numberOfCommandPointCaptures;

	numberOfNukesDeployed += someStats.numberOfNukesDeployed;
	if(someStats.numberOfNukesDeployed > someMods.medal_nukeSpec_NumForStreak)
		numberOfNukesDeployedStreak++; 
	else 
		numberOfNukesDeployedStreak = 0; 

	if(someStats.matchWon)
	{
		currentWinningStreak++; 
		if(currentWinningStreak > bestWinningStreak)
			bestWinningStreak = currentWinningStreak; 
	}
	else if(someStats.matchLost)
	{
		currentWinningStreak = 0; 
	}
	
	if(someStats.matchType == MATCHTYPE_DOMINATION)
	{
		numberOfDominationMatches++; 
		if(someStats.matchWon)
		{
			numberOfDominationMatchesWon++; 
			numberOfDominationStreak++; 
		}
		else if(someStats.matchLost)
		{
			numberOfDominationStreak = 0; 
		}

		if(someStats.matchWasFlawlessVictory)
		{
			numberOfMatchesWonByTotalDomination++; 
			numberOfTotalDominationStreak++; 
		}
		else if(someStats.matchWon || someStats.matchLost)
		{
			numberOfTotalDominationStreak = 0; 	
		}
	}

	if(someStats.matchType == MATCHTYPE_ASSAULT)
	{
		numberOfAssaultMatches++; 
		if(someStats.matchWon)
		{
			numberOfAssaultMatchesWon++; 
			numberOfAssaultStreak++; 
		}
		else if(someStats.matchLost)
		{
			numberOfAssaultStreak = 0; 
		}

		if(someStats.matchWasFlawlessVictory)
		{
			numberOfPerfectDefendsInAssultMatch++; 
			numberOfPerfectDefendsInAssultStreak++; 
		}
		else if(someStats.matchWon || someStats.matchLost)
		{
			numberOfPerfectDefendsInAssultStreak = 0; 	
		}
	}

	if(someStats.matchType == MATCHTYPE_TOW)
	{
		numberOfTugOfWarMatches++; 
		if(someStats.matchWon)
		{
			numberOfTugOfWarMatchesWon++; 
			numberOfTugOfWarStreak++; 
		}
		else if(someStats.matchLost)
		{
			numberOfTugOfWarStreak = 0; 		
		}

		if(someStats.matchWasFlawlessVictory)
		{
			numberOfPerfectPushesInTugOfWarMatch++; 
			numberOfPerfectPushesInTugOfWarStreak++; 
		}
		else if(someStats.matchWon || someStats.matchLost)
		{
			numberOfPerfectPushesInTugOfWarStreak = 0; 	
		}
	}

	if(someStats.bestData & MMG_Stats::PlayerMatchStats::BEST_PLAYER)
	{
		numberOftimesAsBestPlayer++; 
		currentBestPlayerStreak++; 
	}
	else if(someStats.matchWon || someStats.matchLost)
	{
		currentBestPlayerStreak = 0; 
	}

	if(someStats.bestData & MMG_Stats::PlayerMatchStats::BEST_INFANTRY)
	{
		numberOftimesAsBestInfPlayer++; 
		currentBestInfStreak++;
	}
	else if(someStats.matchWon || someStats.matchLost)
	{
		currentBestInfStreak = 0; 
	}

	if(someStats.bestData & MMG_Stats::PlayerMatchStats::BEST_SUPPORT)
	{
		numberOftimesAsBestSupPlayer++; 
		currentBestSupStreak++;
	}
	else if(someStats.matchWon || someStats.matchLost)
	{
		currentBestSupStreak = 0;	
	}

	if(someStats.bestData & MMG_Stats::PlayerMatchStats::BEST_ARMOR)
	{
		numberOftimesAsBestArmPlayer++;
		currentBestArmStreak++;
	}
	else if(someStats.matchWon || someStats.matchLost)
	{
		currentBestArmStreak = 0; 
	}

	if(someStats.bestData & MMG_Stats::PlayerMatchStats::BEST_AIR)
	{
		numberOftimesAsBestAirPlayer++;
		currentBestAirStreak++;
	}
	else if(someStats.matchWon || someStats.matchLost)
	{
		currentBestAirStreak = 0; 
	}

	dataIsDirty = true; 
}

unsigned int 
MMS_PlayerStats::PlayerStatsEntry::UpdateMedal(unsigned int* value, 
											   unsigned int* numStars, 
											   unsigned int featsA, 
											   unsigned int featsB, 
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
	if (*value == BRONZE && featsA >= silverTrigger)
	{
		*value = SILVER; 
		*numStars = 0;
		extraScore += silverMod; 
	}
	if(*value == SILVER)
		*numStars = __max(*numStars, MC_Clamp<unsigned short>(featsA / silverTrigger - 1, 0, 3));
	if(*value == SILVER && featsB >= goldTrigger)
	{
		*value = GOLD; 
		*numStars = 0;
		extraScore += goldMod; 
	}

	return extraScore; 
}

void 
MMS_PlayerStats::PlayerStatsEntry::UpdateHighlyDecorated()
{
	if(medal_infAch != NONE && medal_airAch != NONE && medal_armAch != NONE && medal_supAch != NONE && medal_scoreAch != NONE && medal_taAch != NONE && 
	   medal_infComEx != NONE && medal_airComEx != NONE && medal_armComEx != NONE && medal_supComEx != NONE && medal_winStreak != NONE && medal_domSpec != NONE && 
	   medal_domEx != NONE && medal_assSpec != NONE && medal_assEx != NONE && medal_towSpec != NONE && medal_towEx != NONE && medal_nukeSpec != NONE)
	{
		medal_highDec = GOLD; 
	}
}

unsigned int 
MMS_PlayerStats::PlayerStatsEntry::UpdateMedals(MMG_Stats::PlayerMatchStats& someStats, 
												TweakableValues& someMods)
{
	unsigned int extraScore = 0; 

	// infantry achievement medal 
	extraScore += UpdateMedal(&medal_infAch, &medal_infAchStars, numberOftimesAsBestInfPlayer, currentBestInfStreak, 
		someMods.medal_infAch_NumForBronze, someMods.medal_infAch_NumForSilver, someMods.medal_infAch_StreakForGold, 
		someMods.medal_infAch_BronzeScoreMod, someMods.medal_infAch_SilverScoreMod, someMods.medal_infAch_GoldScoreMod); 

	// air achievement medal 
	extraScore += UpdateMedal(&medal_airAch, &medal_airAchStars, numberOftimesAsBestAirPlayer, currentBestAirStreak, 
		someMods.medal_airAch_NumForBronze, someMods.medal_airAch_NumForSilver, someMods.medal_airAch_StreakForGold, 
		someMods.medal_airAch_BronzeScoreMod, someMods.medal_airAch_SilverScoreMod, someMods.medal_airAch_GoldScoreMod); 

	// armor achievement medal 
	extraScore += UpdateMedal(&medal_armAch, &medal_armAchStars, numberOftimesAsBestArmPlayer, currentBestArmStreak, 
		someMods.medal_armAch_NumForBronze, someMods.medal_armAch_NumForSilver, someMods.medal_armAch_StreakForGold, 
		someMods.medal_armAch_BronzeScoreMod, someMods.medal_armAch_SilverScoreMod, someMods.medal_armAch_GoldScoreMod); 

	// support achievement medal 
	extraScore += UpdateMedal(&medal_supAch, &medal_supAchStars, numberOftimesAsBestSupPlayer, currentBestSupStreak, 
		someMods.medal_supAch_NumForBronze, someMods.medal_supAch_NumForSilver, someMods.medal_supAch_StreakForGold, 
		someMods.medal_supAch_BronzeScoreMod, someMods.medal_supAch_SilverScoreMod, someMods.medal_supAch_GoldScoreMod); 

	// score achievement medal 
	extraScore += UpdateMedal(&medal_scoreAch, &medal_scoreAchStars, numberOftimesAsBestPlayer, currentBestPlayerStreak, 
		someMods.medal_scoreAch_NumForBronze, someMods.medal_scoreAch_NumForSilver, someMods.medal_scoreAch_StreakForGold, 
		someMods.medal_scoreAch_BronzeScoreMod, someMods.medal_scoreAch_SilverScoreMod, someMods.medal_scoreAch_GoldScoreMod); 

	// tactical aid critical hit medal
	extraScore += UpdateMedal(&medal_taAch, &medal_taAchStars, numberOfTacticalAidCristicalHits, someStats.numberOfTacticalAidCriticalHits, 
		someMods.medal_taAch_NumForBronze, someMods.medal_taAch_NumForSilver, someMods.medal_taAch_NumForGold, 
		someMods.medal_taAch_BronzeScoreMod, someMods.medal_taAch_SilverScoreMod, someMods.medal_taAch_GoldScoreMod); 

	// infantry combat excellency 
	extraScore += UpdateMedal(&medal_infComEx, &medal_infComExStars, someStats.scoreAsInfantry, someStats.scoreAsInfantry, 
		someMods.medal_infComEx_ScoreForBronze, someMods.medal_infComEx_ScoreForSilver, someMods.medal_infComEx_ScoreForGold, 
		someMods.medal_infComEx_BronzeScoreMod, someMods.medal_infComEx_SilverScoreMod, someMods.medal_infComEx_GoldScoreMod); 

	// air combat excellency 
	extraScore += UpdateMedal(&medal_airComEx, &medal_airComExStars, someStats.scoreAsAir, someStats.scoreAsAir, 
		someMods.medal_airComEx_ScoreForBronze, someMods.medal_airComEx_ScoreForSilver, someMods.medal_airComEx_ScoreForGold, 
		someMods.medal_airComEx_BronzeScoreMod, someMods.medal_airComEx_SilverScoreMod, someMods.medal_airComEx_GoldScoreMod); 

	// armor combat excellency 
	extraScore += UpdateMedal(&medal_armComEx, &medal_armComExStars, someStats.scoreAsArmor, someStats.scoreAsArmor, 
		someMods.medal_armComEx_ScoreForBronze, someMods.medal_armComEx_ScoreForSilver, someMods.medal_armComEx_ScoreForGold, 
		someMods.medal_armComEx_BronzeScoreMod, someMods.medal_armComEx_SilverScoreMod, someMods.medal_armComEx_GoldScoreMod); 

	// support combat excellency 
	extraScore += UpdateMedal(&medal_supComEx, &medal_supComExStars, someStats.scoreAsSupport, someStats.scoreAsSupport, 
		someMods.medal_supComEx_ScoreForBronze, someMods.medal_supComEx_ScoreForSilver, someMods.medal_supComEx_ScoreForGold, 
		someMods.medal_supComEx_BronzeScoreMod, someMods.medal_supComEx_SilverScoreMod, someMods.medal_supComEx_GoldScoreMod); 

	// winning streak medal
	extraScore += UpdateMedal(&medal_winStreak, &medal_winStreakStars, currentWinningStreak, currentWinningStreak, 
		someMods.medal_winStreak_NumForBronze, someMods.medal_winStreak_NumForSilver, someMods.medal_winStreak_NumForGold, 
		someMods.medal_winStreak_BronzeScoreMod, someMods.medal_winStreak_SilverScoreMod, someMods.medal_winStreak_GoldScoreMod); 
	
	// domination specialist medal 
	extraScore += UpdateMedal(&medal_domSpec, &medal_domSpecStars, numberOfDominationMatchesWon, numberOfDominationStreak, 
		someMods.medal_domSpec_NumForBronze, someMods.medal_domSpec_NumForSilver, someMods.medal_domSpec_StreakForGold, 
		someMods.medal_domSpec_BronzeScoreMod, someMods.medal_domSpec_SilverScoreMod, someMods.medal_domSpec_GoldScoreMod); 
	
	// domination excellency medal 
	extraScore += UpdateMedal(&medal_domEx, &medal_domExStars, numberOfMatchesWonByTotalDomination, numberOfTotalDominationStreak, 
		someMods.medal_domEx_NumForBronze, someMods.medal_domEx_NumForSilver, someMods.medal_domEx_StreakForGold, 
		someMods.medal_domEx_BronzeScoreMod, someMods.medal_domEx_SilverScoreMod, someMods.medal_domEx_GoldScoreMod);

	// assault specialist medal 
	extraScore += UpdateMedal(&medal_assSpec, &medal_assSpecStars, numberOfAssaultMatchesWon, numberOfAssaultStreak, 
		someMods.medal_assSpec_NumForBronze, someMods.medal_assSpec_NumForSilver, someMods.medal_assSpec_StreakForGold, 
		someMods.medal_assSpec_BronzeScoreMod, someMods.medal_assSpec_SilverScoreMod, someMods.medal_assSpec_GoldScoreMod);

	// assault excellency medal 
	extraScore += UpdateMedal(&medal_assEx, &medal_assExStars, numberOfPerfectDefendsInAssultMatch, numberOfPerfectDefendsInAssultStreak, 
		someMods.medal_assEx_NumForBronze, someMods.medal_assEx_NumForSilver, someMods.medal_assEx_StreakForGold, 
		someMods.medal_assEx_BronzeScoreMod, someMods.medal_assEx_SilverScoreMod, someMods.medal_assEx_GoldScoreMod);

	// tug of war specialist medal 	
	extraScore += UpdateMedal(&medal_towSpec, &medal_towSpecStars, numberOfTugOfWarMatchesWon, numberOfTugOfWarStreak, 
		someMods.medal_towSpec_NumForBronze, someMods.medal_towSpec_NumForSilver, someMods.medal_towSpec_StreakForGold, 
		someMods.medal_towSpec_BronzeScoreMod, someMods.medal_towSpec_SilverScoreMod, someMods.medal_towSpec_GoldScoreMod);

	// tug of war excellency medal 	
	extraScore += UpdateMedal(&medal_towEx, &medal_towExStars, numberOfPerfectPushesInTugOfWarMatch, numberOfPerfectPushesInTugOfWarStreak, 
		someMods.medal_towEx_NumForBronze, someMods.medal_towEx_NumForSilver, someMods.medal_towEx_StreakForGold, 
		someMods.medal_towEx_BronzeScoreMod, someMods.medal_towEx_SilverScoreMod, someMods.medal_towEx_GoldScoreMod);

	// nuclear specialist 
	extraScore += UpdateMedal(&medal_nukeSpec, &medal_nukeSpecStars, numberOfNukesDeployed, numberOfNukesDeployedStreak, 
		someMods.medal_nukeSpec_NumForBronze, someMods.medal_nukeSpec_NumForSilver, someMods.medal_nukeSpec_StreakForGold, 
		someMods.medal_nukeSpec_BronzeScoreMod, someMods.medal_nukeSpec_SilverScoreMod, someMods.medal_nukeSpec_GoldScoreMod);

	UpdateHighlyDecorated(); 

	dataIsDirty = true; 

	return extraScore; 
}

unsigned int
MMS_PlayerStats::PlayerStatsEntry::UpdateBadge(unsigned int* value, 
											   unsigned int* numStars, 
											   unsigned int feats, 
											   unsigned int bronzeTrigger, 
											   unsigned int silverTrigger, 
											   unsigned int goldTrigger, 
											   unsigned int bronzeMod, 
											   unsigned int silverMod, 
											   unsigned int goldMod)
{
	unsigned int extraScore = 0; 

	if(*value == NONE && feats >= bronzeTrigger)
	{
		*value = BRONZE; 
		extraScore += bronzeMod; 
	}
	if(*value == BRONZE)
		*numStars = MC_Clamp<unsigned short>(feats / bronzeTrigger - 1, 0, 3);
	if (*value == BRONZE && feats >= silverTrigger)
	{
		*value = SILVER; 
		extraScore += silverMod; 
	}
	if(*value == SILVER)
		*numStars = MC_Clamp<unsigned short>(feats / silverTrigger - 1, 0, 3);
	if(*value == SILVER && feats >= goldTrigger)
	{
		*value = GOLD; 
		*numStars = 0; 
		extraScore += goldMod; 
	}

	return extraScore; 
}

unsigned int 
MMS_PlayerStats::PlayerStatsEntry::UpdateBadges(MMG_Stats::PlayerMatchStats& someStats, TweakableValues& someMods)
{
	unsigned int extraScore = 0; 

	// infantry specialist badge 
	extraScore += UpdateBadge(&badge_infSp, &badge_infSpStars, scoreAsInfantry, 
		someMods.badge_infSp_ScoreForBronze, someMods.badge_infSp_ScoreForSilver, someMods.badge_infSp_ScoreForGold, 
		someMods.badge_infSp_BronzeScoreMod, someMods.badge_infSp_SilverScoreMod, someMods.badge_infSp_GoldScoreMod); 

	// air specialist badge 
	extraScore += UpdateBadge(&badge_airSp, &badge_airSpStars, scoreAsAir, 
		someMods.badge_airSp_ScoreForBronze, someMods.badge_airSp_ScoreForSilver, someMods.badge_airSp_ScoreForGold, 
		someMods.badge_airSp_BronzeScoreMod, someMods.badge_airSp_SilverScoreMod, someMods.badge_airSp_GoldScoreMod); 

	// armor specialist badge 
	extraScore += UpdateBadge(&badge_armSp, &badge_armSpStars, scoreAsArmor, 
		someMods.badge_armSp_ScoreForBronze, someMods.badge_armSp_ScoreForSilver, someMods.badge_armSp_ScoreForGold, 
		someMods.badge_armSp_BronzeScoreMod, someMods.badge_armSp_SilverScoreMod, someMods.badge_armSp_GoldScoreMod); 

	// support specialist badge 
	extraScore += UpdateBadge(&badge_supSp, &badge_supSpStars, scoreAsSupport, 
		someMods.badge_supSp_ScoreForBronze, someMods.badge_supSp_ScoreForSilver, someMods.badge_supSp_ScoreForGold, 
		someMods.badge_supSp_BronzeScoreMod, someMods.badge_supSp_SilverScoreMod, someMods.badge_supSp_GoldScoreMod); 

	// total score achievement badge 
	extraScore += UpdateBadge(&badge_scoreAch, &badge_scoreAchStars, scoreTotal, 
		someMods.badge_scoreAch_ScoreForBronze, someMods.badge_scoreAch_ScoreForSilver, someMods.badge_scoreAch_ScoreForGold, 
		someMods.badge_scoreAch_BronzeScoreMod, someMods.badge_scoreAch_SilverScoreMod, someMods.badge_scoreAch_GoldScoreMod); 

	// command point achievement badge 
	extraScore += UpdateBadge(&badge_cpAch, &badge_cpAchStars, numberOfCommandPointCaptures, 
		someMods.badge_cpAch_NumForBronze, someMods.badge_cpAch_NumForSilver, someMods.badge_cpAch_NumForGold, 
		someMods.badge_cpAch_BronzeScoreMod, someMods.badge_cpAch_SilverScoreMod, someMods.badge_cpAch_GoldScoreMod); 

	// fortification point achievement badge 
	extraScore += UpdateBadge(&badge_fortAch, &badge_fortAchStars, scoreByFortifying, 
		someMods.badge_fortAch_NumForBronze, someMods.badge_fortAch_NumForSilver, someMods.badge_fortAch_NumForGold, 
		someMods.badge_fortAch_BronzeScoreMod, someMods.badge_fortAch_SilverScoreMod, someMods.badge_fortAch_GoldScoreMod); 
	
	// matches achievement badge 
	extraScore += UpdateBadge(&badge_matchAch, &badge_matchAchStars, numberOfMatches, 
		someMods.badge_matchAch_NumForBronze, someMods.badge_matchAch_NumForSilver, someMods.badge_matchAch_NumForGold, 
		someMods.badge_matchAch_BronzeScoreMod, someMods.badge_matchAch_SilverScoreMod, someMods.badge_matchAch_GoldScoreMod); 

	// USA achievement badge 
	extraScore += UpdateBadge(&badge_USAAch, &badge_USAAchStars, timePlayedAsUSA, 
		someMods.badge_USAAch_TimeForBronze, someMods.badge_USAAch_TimeForSilver, someMods.badge_USAAch_TimeForGold, 
		someMods.badge_USAAch_BronzeScoreMod, someMods.badge_USAAch_SilverScoreMod, someMods.badge_USAAch_GoldScoreMod); 

	// USSR achievement badge 
	extraScore += UpdateBadge(&badge_USSRAch, &badge_USSRAchStars, timePlayedAsUSSR, 
		someMods.badge_USSRAch_TimeForBronze, someMods.badge_USSRAch_TimeForSilver, someMods.badge_USAAch_TimeForGold, 
		someMods.badge_USSRAch_BronzeScoreMod, someMods.badge_USSRAch_SilverScoreMod, someMods.badge_USSRAch_GoldScoreMod); 

	// NATO achievement badge 
	extraScore += UpdateBadge(&badge_NATOAch, &badge_NATOAchStars, timePlayedAsNATO, 
		someMods.badge_NATOAch_TimeForBronze, someMods.badge_NATOAch_TimeForSilver, someMods.badge_NATOAch_TimeForGold, 
		someMods.badge_NATOAch_BronzeScoreMod, someMods.badge_NATOAch_SilverScoreMod, someMods.badge_NATOAch_GoldScoreMod); 

	return extraScore; 
}

bool 
MMS_PlayerStats::PlayerStatsEntry::Load(MDB_MySqlConnection* aReadConnection)
{
	unsigned int startTime = GetTickCount();

	MC_StaticString<1024> sql; 

	sql.Format("SELECT *, UNIX_TIMESTAMP(lastUpdated) AS lastMatchPlayed, UNIX_TIMESTAMP(massgateMemberSince) AS memberSince, IF(disallowStatsUntil>NOW(),UNIX_TIMESTAMP(disallowStatsUntil)-UNIX_TIMESTAMP(NOW()) ,0) AS IgnoreStatsForAnotherXseconds FROM PlayerStats WHERE profileId = %u", profileId);

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
		lastMatchPlayed = row["lastMatchPlayed"]; 
		massgateMemberSince = row["memberSince"]; 
		rank = row["rank"];
		scoreTotal = row["sc_tot"];
		highestScore = row["sc_highest"];
		scoreAsInfantry = row["sc_inf"];
		highScoreAsInfantry = row["sc_highinf"];
		scoreAsSupport = row["sc_sup"];
		highScoreAsSupport = row["sc_highsup"];
		scoreAsArmor = row["sc_arm"];
		highScoreAsArmor = row["sc_higharm"];
		scoreAsAir = row["sc_air"];
		highScoreAsAir = row["sc_highair"];
		scoreByDamagingEnemies = row["sc_damen"];
		scoreByUsingTacticalAids = row["sc_ta"];
		scoreByCapturingCommandPoints = row["sc_cpc"];
		scoreByRepairing = row["sc_rep"];
		scoreByFortifying = row["sc_fort"];
		scoreLostByKillingFriendly = row["sc_tk"];
		timePlayedAsUSA = row["t_USA"];
		timePlayedAsUSSR = row["t_USSR"];
		timePlayedAsNATO = row["t_NATO"];
		timePlayedAsInfantry = row["t_inf"];
		timePlayedAsSupport = row["t_sup"];	
		timePlayedAsArmor = row["t_arm"];
		timePlayedAsAir = row["t_air"];
		numberOfMatches = row["n_matches"]; 
		numberOfMatchesWon = row["n_matcheswon"];
		numberOfMatchesLost = row["n_matcheslost"];
		numberOfAssaultMatches = row["n_assmatches"];
		numberOfAssaultMatchesWon = row["n_assmatcheswon"];
		numberOfAssaultStreak = row["n_assstreak"];
		numberOfDominationMatches = row["n_dommatches"];
		numberOfDominationMatchesWon = row["n_dommatcheswon"];
		numberOfDominationStreak = row["n_domstreak"];
		numberOfTugOfWarMatches = row["n_towmatches"];
		numberOfTugOfWarMatchesWon = row["n_towmatcheswon"];
		numberOfTugOfWarStreak = row["n_towstreak"];
		currentWinningStreak = row["n_cwinstr"];
		bestWinningStreak = row["n_bwinstr"];
		numberOfMatchesWonByTotalDomination = row["n_wbtd"];
		numberOfTotalDominationStreak = row["n_wbtdstreak"];
		numberOfPerfectDefendsInAssultMatch = row["n_pdas"];
		numberOfPerfectDefendsInAssultStreak = row["n_pdasstreak"];
		numberOfPerfectPushesInTugOfWarMatch = row["n_pptw"];
		numberOfPerfectPushesInTugOfWarStreak = row["n_pptwstreak"];
		numberOfUnitsKilled = row["n_ukills"];
		numberOfUnitsLost = row["n_ulost"];
		numberOfReinforcementPointsSpent = row["n_rps"];
		numberOfTacticalAidPointsSpent = row["n_taps"];
		numberOfNukesDeployed = row["n_nukes"];
		numberOfNukesDeployedStreak = row["n_nukestreak"];
		numberOfTacticalAidCristicalHits = row["n_tach"];
		numberOfCommandPointCaptures = row["n_cpc"];
		numberOftimesAsBestPlayer = row["n_bplayer"];
		currentBestPlayerStreak = row["n_bplayerstr"]; 
		numberOftimesAsBestInfPlayer = row["n_binf"];
		currentBestInfStreak = row["n_binfstr"];
		numberOftimesAsBestSupPlayer = row["n_bsup"]; 
		currentBestSupStreak = row["n_bsupstr"]; 
		numberOftimesAsBestArmPlayer = row["n_barm"];
		currentBestArmStreak = row["n_barmstr"];
		numberOftimesAsBestAirPlayer = row["n_bair"];
		currentBestAirStreak = row["n_bairstr"];
		numStatPadsDetected = row["n_statpad"];
		ignoreStatsUntil = unsigned int(time(NULL)) + unsigned int (row["IgnoreStatsForAnotherXseconds"]);
	}

	sql.Format("SELECT * FROM PlayerMedals WHERE profileId = %u", profileId); 

	if(!query.Ask(result, sql))
		return false; 

	if(!result.GetAffectedNumberOrRows())
		return false; 

	if(result.GetNextRow(row))
	{
		medal_infAch = row["infAch"]; 
		medal_infAchStars = row["infAchStars"]; 
		medal_airAch = row["airAch"];
		medal_airAchStars = row["airAchStars"];
		medal_armAch = row["armorAch"];
		medal_armAchStars = row["armorAchStars"];
		medal_supAch = row["supAch"];
		medal_supAchStars = row["supAchStars"];
		medal_scoreAch = row["scoreAch"];
		medal_scoreAchStars = row["scoreAchStars"];
		medal_taAch = row["taAch"];
		medal_taAchStars = row["taAchStars"];
		medal_infComEx = row["infComEx"];
		medal_infComExStars = row["infComExStars"];
		medal_airComEx = row["airComEx"];
		medal_airComExStars = row["airComExStars"];
		medal_armComEx = row["armorComEx"];
		medal_armComExStars = row["armorComExStars"];
		medal_supComEx = row["supComEx"];
		medal_supComExStars = row["supComExStars"];
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
		medal_nukeSpec = row["nukeSpec"];
		medal_nukeSpecStars = row["nukeSpecStars"];
		medal_highDec = row["highDec"];
		medal_highDecStars = row["highDecStars"];
	}

	sql.Format("SELECT * FROM PlayerBadges WHERE profileId = %u", profileId); 

	if(!query.Ask(result, sql))
		return false; 

	if(result.GetNextRow(row))
	{
		badge_infSp = row["infSp"];
		badge_infSpStars = row["infSpStars"];
		badge_airSp = row["airSp"];
		badge_airSpStars = row["airSpStars"];
		badge_armSp = row["armSp"];
		badge_armSpStars = row["armSpStars"];
		badge_supSp = row["supSp"];
		badge_supSpStars = row["supSpStars"];
		badge_scoreAch = row["scoreAch"];
		badge_scoreAchStars = row["scoreAchStars"];
		badge_cpAch = row["cpAch"];
		badge_cpAchStars = row["cpAchStars"];
		badge_fortAch = row["fortAch"];
		badge_fortAchStars = row["fortAchStars"];
		badge_mgAch = row["mgAch"];
		badge_mgAchStars = row["mgAchStars"];
		badge_matchAch = row["matchAch"];
		badge_matchAchStars = row["matchAchStars"];
		badge_USAAch = row["USAAch"];
		badge_USAAchStars = row["USAAchStars"];
		badge_USSRAch = row["USSRAch"];
		badge_USSRAchStars = row["USSRAchStars"];
		badge_NATOAch = row["NATOAch"];
		badge_NATOAchStars = row["NATOAchStars"];
		badge_preOrdAch = row["preOrdAch"];
		badge_preOrdAchStars = row["preOrdAchStars"];
		badge_reqruitAFriendAch = row["reqruitAFriendAch"];
		badge_reqruitAFriendAchStars = row["reqruitAFriendAchStars"];

		if (badge_reqruitAFriendAch != 0)
		{
			// The number of recruitments are per account, read that number instead
			sql.Format("SELECT COUNT(Log_Recruit.recruiterAccountId) AS NumRecruitment FROM Accounts,Log_Recruit,Profiles WHERE Profiles.profileId=%u AND Profiles.accountId=Accounts.accountid AND Log_Recruit.recruiterAccountId=Accounts.accountid", profileId);
			if (!query.Ask(result, sql))
				return false;
			if (result.GetNextRow(row))
			{
				badge_reqruitAFriendAch = row["NumRecruitment"];
			}
		}
	}
	else
	{
		return false;
	}

	LOG_EXECUTION_TIME(, startTime);

	return true; 
}

void 
MMS_PlayerStats::PlayerStatsEntry::GetStats(MMG_Stats::PlayerStatsRsp& someStats)
{
	MMS_LadderUpdater* ladderUpdater; 
	ladderUpdater = MMS_LadderUpdater::GetInstance(); 

	someStats.profileId = profileId; 
	someStats.lastMatchPlayed = lastMatchPlayed; 
	someStats.scoreTotal = scoreTotal;
	someStats.scoreAsInfantry = scoreAsInfantry;
	someStats.highScoreAsInfantry = highScoreAsInfantry;
	someStats.scoreAsSupport = scoreAsSupport;
	someStats.highScoreAsSupport = highScoreAsSupport;
	someStats.scoreAsArmor = scoreAsArmor;
	someStats.highScoreAsArmor = highScoreAsArmor;
	someStats.scoreAsAir = scoreAsAir;
	someStats.highScoreAsAir = highScoreAsAir;
	someStats.scoreByDamagingEnemies = scoreByDamagingEnemies;
	someStats.scoreByUsingTacticalAids = scoreByUsingTacticalAids;
	someStats.scoreByCapturingCommandPoints = scoreByCapturingCommandPoints; 
	someStats.scoreByRepairing = scoreByRepairing;
	someStats.scoreByFortifying = scoreByFortifying;
	someStats.highestScore = highestScore;
	someStats.currentLadderPosition = ladderUpdater->GetPlayerPosition(profileId);
	someStats.timePlayedAsUSA = timePlayedAsUSA;
	someStats.timePlayedAsUSSR = timePlayedAsUSSR;
	someStats.timePlayedAsNATO = timePlayedAsNATO;
	someStats.timePlayedAsInfantry = timePlayedAsInfantry;
	someStats.timePlayedAsSupport = timePlayedAsSupport;	
	someStats.timePlayedAsArmor = timePlayedAsArmor;
	someStats.timePlayedAsAir = timePlayedAsAir;
	someStats.numberOfMatches = numberOfMatches; 
	someStats.numberOfMatchesWon = numberOfMatchesWon;
	someStats.numberOfMatchesLost = numberOfMatchesLost;
	someStats.numberOfAssaultMatches = numberOfAssaultMatches;
	someStats.numberOfAssaultMatchesWon = numberOfAssaultMatchesWon;
	someStats.numberOfDominationMatches = numberOfDominationMatches;
	someStats.numberOfDominationMatchesWon = numberOfDominationMatchesWon;
	someStats.numberOfTugOfWarMatches = numberOfTugOfWarMatches;
	someStats.numberOfTugOfWarMatchesWon = numberOfTugOfWarMatchesWon;
	someStats.currentWinningStreak = currentWinningStreak; 
	someStats.bestWinningStreak = bestWinningStreak; 
	someStats.numberOfMatchesWonByTotalDomination = numberOfMatchesWonByTotalDomination;
	someStats.numberOfPerfectDefendsInAssultMatch = numberOfPerfectDefendsInAssultMatch;
	someStats.numberOfPerfectPushesInTugOfWarMatch = numberOfPerfectPushesInTugOfWarMatch;
	someStats.numberOfUnitsKilled = numberOfUnitsKilled;
	someStats.numberOfUnitsLost = numberOfUnitsLost;
	someStats.numberOfReinforcementPointsSpent = numberOfReinforcementPointsSpent;
	someStats.numberOfTacticalAidPointsSpent = numberOfTacticalAidPointsSpent;
	someStats.numberOfNukesDeployed = numberOfNukesDeployed;
	someStats.numberOfTacticalAidCristicalHits = numberOfTacticalAidCristicalHits;
}

void 
MMS_PlayerStats::PlayerStatsEntry::GetMedals(MMG_DecorationProtocol::PlayerMedalsRsp& someMedals)
{
	someMedals.profileId = profileId; 
	someMedals.Add(medal_infAch, medal_infAchStars); 
	someMedals.Add(medal_airAch, medal_airAchStars);
	someMedals.Add(medal_armAch, medal_armAchStars);
	someMedals.Add(medal_supAch, medal_supAchStars);
	someMedals.Add(medal_scoreAch, medal_scoreAchStars);
	someMedals.Add(medal_taAch, medal_taAchStars);
	someMedals.Add(medal_infComEx, medal_infComExStars);
	someMedals.Add(medal_airComEx, medal_airComExStars);
	someMedals.Add(medal_armComEx, medal_armComExStars);
	someMedals.Add(medal_supComEx, medal_supComExStars);
	someMedals.Add(medal_winStreak, medal_winStreakStars);
	someMedals.Add(medal_domSpec, medal_domSpecStars);
	someMedals.Add(medal_domEx, medal_domExStars);
	someMedals.Add(medal_assSpec, medal_assSpecStars);
	someMedals.Add(medal_assEx, medal_assExStars);
	someMedals.Add(medal_towSpec, medal_towSpecStars);
	someMedals.Add(medal_towEx, medal_towExStars);
	someMedals.Add(medal_nukeSpec, medal_nukeSpecStars);
	someMedals.Add(medal_highDec, medal_highDecStars);
}

void 
MMS_PlayerStats::PlayerStatsEntry::GetBadges(MMG_DecorationProtocol::PlayerBadgesRsp& someBadges)
{
	someBadges.profileId = profileId; 
	someBadges.Add(badge_infSp, badge_infSpStars);
	someBadges.Add(badge_airSp, badge_airSpStars);
	someBadges.Add(badge_armSp, badge_armSpStars);
	someBadges.Add(badge_supSp, badge_supSpStars);
	someBadges.Add(badge_scoreAch, badge_scoreAchStars);
	someBadges.Add(badge_cpAch, badge_cpAchStars);
	someBadges.Add(badge_fortAch, badge_fortAchStars);
	someBadges.Add(badge_mgAch, badge_mgAchStars);
	someBadges.Add(badge_matchAch, badge_matchAchStars);
	someBadges.Add(badge_USAAch, badge_USAAchStars);
	someBadges.Add(badge_USSRAch, badge_USSRAchStars);
	someBadges.Add(badge_NATOAch, badge_NATOAchStars);
	someBadges.Add(badge_preOrdAch, badge_preOrdAchStars);
	someBadges.Add(badge_reqruitAFriendAch, badge_reqruitAFriendAchStars);
}

MMS_PlayerStats::TweakableValues::TweakableValues()
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
MMS_PlayerStats::TweakableValues::Load(MDB_MySqlConnection* aReadConnection)
{
	MC_StaticString<1024> sql; 

	sql.Format("SELECT * FROM PlayerStatsTweakables");

	MDB_MySqlQuery query(*aReadConnection); 
	MDB_MySqlResult result; 
	MDB_MySqlRow row; 

	if(!query.Ask(result, sql))
		return false; 

	while(result.GetNextRow(row))
	{
		MC_StaticString<256> aKey = row["aKey"];

		READ_MOD_VALUE(medal_infAch_NumForBronze);
		READ_MOD_VALUE(medal_infAch_NumForSilver);
		READ_MOD_VALUE(medal_infAch_StreakForGold);
		READ_MOD_VALUE(medal_infAch_BronzeScoreMod);
		READ_MOD_VALUE(medal_infAch_SilverScoreMod);
		READ_MOD_VALUE(medal_infAch_GoldScoreMod);
		READ_MOD_VALUE(medal_supAch_NumForBronze);
		READ_MOD_VALUE(medal_supAch_NumForSilver);
		READ_MOD_VALUE(medal_supAch_StreakForGold);
		READ_MOD_VALUE(medal_supAch_BronzeScoreMod);
		READ_MOD_VALUE(medal_supAch_SilverScoreMod);
		READ_MOD_VALUE(medal_supAch_GoldScoreMod);
		READ_MOD_VALUE(medal_armAch_NumForBronze);
		READ_MOD_VALUE(medal_armAch_NumForSilver);
		READ_MOD_VALUE(medal_armAch_StreakForGold);
		READ_MOD_VALUE(medal_armAch_BronzeScoreMod);
		READ_MOD_VALUE(medal_armAch_SilverScoreMod);
		READ_MOD_VALUE(medal_armAch_GoldScoreMod);
		READ_MOD_VALUE(medal_airAch_NumForBronze);
		READ_MOD_VALUE(medal_airAch_NumForSilver);
		READ_MOD_VALUE(medal_airAch_StreakForGold);
		READ_MOD_VALUE(medal_airAch_BronzeScoreMod);
		READ_MOD_VALUE(medal_airAch_SilverScoreMod);
		READ_MOD_VALUE(medal_airAch_GoldScoreMod);
		READ_MOD_VALUE(medal_scoreAch_NumForBronze);
		READ_MOD_VALUE(medal_scoreAch_NumForSilver);
		READ_MOD_VALUE(medal_scoreAch_StreakForGold);
		READ_MOD_VALUE(medal_scoreAch_BronzeScoreMod);
		READ_MOD_VALUE(medal_scoreAch_SilverScoreMod);
		READ_MOD_VALUE(medal_scoreAch_GoldScoreMod);
		READ_MOD_VALUE(medal_taAch_NumForBronze );
		READ_MOD_VALUE(medal_taAch_NumForSilver);
		READ_MOD_VALUE(medal_taAch_NumForGold);
		READ_MOD_VALUE(medal_taAch_BronzeScoreMod);
		READ_MOD_VALUE(medal_taAch_SilverScoreMod);
		READ_MOD_VALUE(medal_taAch_GoldScoreMod);
		READ_MOD_VALUE(medal_infComEx_ScoreForBronze);
		READ_MOD_VALUE(medal_infComEx_ScoreForSilver);
		READ_MOD_VALUE(medal_infComEx_ScoreForGold);
		READ_MOD_VALUE(medal_infComEx_BronzeScoreMod);
		READ_MOD_VALUE(medal_infComEx_SilverScoreMod);
		READ_MOD_VALUE(medal_infComEx_GoldScoreMod);
		READ_MOD_VALUE(medal_supComEx_ScoreForBronze);
		READ_MOD_VALUE(medal_supComEx_ScoreForSilver);
		READ_MOD_VALUE(medal_supComEx_ScoreForGold);
		READ_MOD_VALUE(medal_supComEx_BronzeScoreMod);
		READ_MOD_VALUE(medal_supComEx_SilverScoreMod);
		READ_MOD_VALUE(medal_supComEx_GoldScoreMod);
		READ_MOD_VALUE(medal_armComEx_ScoreForBronze);
		READ_MOD_VALUE(medal_armComEx_ScoreForSilver);
		READ_MOD_VALUE(medal_armComEx_ScoreForGold);
		READ_MOD_VALUE(medal_armComEx_BronzeScoreMod);
		READ_MOD_VALUE(medal_armComEx_SilverScoreMod);
		READ_MOD_VALUE(medal_armComEx_GoldScoreMod);
		READ_MOD_VALUE(medal_airComEx_ScoreForBronze);
		READ_MOD_VALUE(medal_airComEx_ScoreForSilver);
		READ_MOD_VALUE(medal_airComEx_ScoreForGold);
		READ_MOD_VALUE(medal_airComEx_BronzeScoreMod);
		READ_MOD_VALUE(medal_airComEx_SilverScoreMod);
		READ_MOD_VALUE(medal_airComEx_GoldScoreMod);
		READ_MOD_VALUE(medal_winStreak_NumForBronze);
		READ_MOD_VALUE(medal_winStreak_NumForSilver);
		READ_MOD_VALUE(medal_winStreak_NumForGold);
		READ_MOD_VALUE(medal_winStreak_BronzeScoreMod);
		READ_MOD_VALUE(medal_winStreak_SilverScoreMod);
		READ_MOD_VALUE(medal_winStreak_GoldScoreMod);
		READ_MOD_VALUE(medal_taSpec_NumForBronze);
		READ_MOD_VALUE(medal_taSpec_NumForSilver);
		READ_MOD_VALUE(medal_taSpec_NumForGold);
		READ_MOD_VALUE(medal_taSpec_BronzeScoreMod);
		READ_MOD_VALUE(medal_taSpec_SilverScoreMod);
		READ_MOD_VALUE(medal_taSpec_GoldScoreMod);
		READ_MOD_VALUE(medal_domSpec_NumForBronze);
		READ_MOD_VALUE(medal_domSpec_NumForSilver);
		READ_MOD_VALUE(medal_domSpec_StreakForGold);
		READ_MOD_VALUE(medal_domSpec_BronzeScoreMod);
		READ_MOD_VALUE(medal_domSpec_SilverScoreMod);
		READ_MOD_VALUE(medal_domSpec_GoldScoreMod);
		READ_MOD_VALUE(medal_domEx_NumForBronze);
		READ_MOD_VALUE(medal_domEx_NumForSilver);
		READ_MOD_VALUE(medal_domEx_StreakForGold);
		READ_MOD_VALUE(medal_domEx_BronzeScoreMod);
		READ_MOD_VALUE(medal_domEx_SilverScoreMod);
		READ_MOD_VALUE(medal_domEx_GoldScoreMod);
		READ_MOD_VALUE(medal_assSpec_NumForBronze);
		READ_MOD_VALUE(medal_assSpec_NumForSilver);
		READ_MOD_VALUE(medal_assSpec_StreakForGold);
		READ_MOD_VALUE(medal_assSpec_BronzeScoreMod);
		READ_MOD_VALUE(medal_assSpec_SilverScoreMod);
		READ_MOD_VALUE(medal_assSpec_GoldScoreMod);
		READ_MOD_VALUE(medal_assEx_NumForBronze);
		READ_MOD_VALUE(medal_assEx_NumForSilver);
		READ_MOD_VALUE(medal_assEx_StreakForGold);
		READ_MOD_VALUE(medal_assEx_BronzeScoreMod);
		READ_MOD_VALUE(medal_assEx_SilverScoreMod);
		READ_MOD_VALUE(medal_assEx_GoldScoreMod);
		READ_MOD_VALUE(medal_towSpec_NumForBronze);
		READ_MOD_VALUE(medal_towSpec_NumForSilver);
		READ_MOD_VALUE(medal_towSpec_StreakForGold);
		READ_MOD_VALUE(medal_towSpec_BronzeScoreMod);
		READ_MOD_VALUE(medal_towSpec_SilverScoreMod);
		READ_MOD_VALUE(medal_towSpec_GoldScoreMod);
		READ_MOD_VALUE(medal_towEx_NumForBronze);
		READ_MOD_VALUE(medal_towEx_NumForSilver);
		READ_MOD_VALUE(medal_towEx_StreakForGold);
		READ_MOD_VALUE(medal_towEx_BronzeScoreMod);
		READ_MOD_VALUE(medal_towEx_SilverScoreMod);
		READ_MOD_VALUE(medal_towEx_GoldScoreMod);
		READ_MOD_VALUE(badge_infSp_ScoreForBronze);
		READ_MOD_VALUE(badge_infSp_ScoreForSilver);
		READ_MOD_VALUE(badge_infSp_ScoreForGold);
		READ_MOD_VALUE(badge_infSp_BronzeScoreMod);
		READ_MOD_VALUE(badge_infSp_SilverScoreMod);
		READ_MOD_VALUE(badge_infSp_GoldScoreMod);
		READ_MOD_VALUE(badge_airSp_ScoreForBronze);
		READ_MOD_VALUE(badge_airSp_ScoreForSilver);
		READ_MOD_VALUE(badge_airSp_ScoreForGold);
		READ_MOD_VALUE(badge_airSp_BronzeScoreMod);
		READ_MOD_VALUE(badge_airSp_SilverScoreMod);
		READ_MOD_VALUE(badge_airSp_GoldScoreMod);
		READ_MOD_VALUE(badge_armSp_ScoreForBronze);
		READ_MOD_VALUE(badge_armSp_ScoreForSilver);
		READ_MOD_VALUE(badge_armSp_ScoreForGold);
		READ_MOD_VALUE(badge_armSp_BronzeScoreMod);
		READ_MOD_VALUE(badge_armSp_SilverScoreMod);
		READ_MOD_VALUE(badge_armSp_GoldScoreMod);
		READ_MOD_VALUE(badge_supSp_ScoreForBronze);
		READ_MOD_VALUE(badge_supSp_ScoreForSilver);
		READ_MOD_VALUE(badge_supSp_ScoreForGold);
		READ_MOD_VALUE(badge_supSp_BronzeScoreMod);
		READ_MOD_VALUE(badge_supSp_SilverScoreMod);
		READ_MOD_VALUE(badge_supSp_GoldScoreMod);
		READ_MOD_VALUE(badge_scoreAch_ScoreForBronze);
		READ_MOD_VALUE(badge_scoreAch_ScoreForSilver);
		READ_MOD_VALUE(badge_scoreAch_ScoreForGold);
		READ_MOD_VALUE(badge_scoreAch_BronzeScoreMod);
		READ_MOD_VALUE(badge_scoreAch_SilverScoreMod);
		READ_MOD_VALUE(badge_scoreAch_GoldScoreMod);
		READ_MOD_VALUE(badge_cpAch_NumForBronze);
		READ_MOD_VALUE(badge_cpAch_NumForSilver);
		READ_MOD_VALUE(badge_cpAch_NumForGold);
		READ_MOD_VALUE(badge_cpAch_BronzeScoreMod);
		READ_MOD_VALUE(badge_cpAch_SilverScoreMod);
		READ_MOD_VALUE(badge_cpAch_GoldScoreMod);
		READ_MOD_VALUE(badge_fortAch_NumForBronze);
		READ_MOD_VALUE(badge_fortAch_NumForSilver);
		READ_MOD_VALUE(badge_fortAch_NumForGold);
		READ_MOD_VALUE(badge_fortAch_BronzeScoreMod);
		READ_MOD_VALUE(badge_fortAch_SilverScoreMod);
		READ_MOD_VALUE(badge_fortAch_GoldScoreMod);
		READ_MOD_VALUE(badge_mgAch_TimeForBronze);
		READ_MOD_VALUE(badge_mgAch_TimeForSilver);
		READ_MOD_VALUE(badge_mgAch_TimeForGold);
		READ_MOD_VALUE(badge_mgAch_BronzeScoreMod);
		READ_MOD_VALUE(badge_mgAch_SilverScoreMod);
		READ_MOD_VALUE(badge_mgAch_GoldScoreMod);
		READ_MOD_VALUE(badge_matchAch_NumForBronze);
		READ_MOD_VALUE(badge_matchAch_NumForSilver);
		READ_MOD_VALUE(badge_matchAch_NumForGold);
		READ_MOD_VALUE(badge_matchAch_BronzeScoreMod);
		READ_MOD_VALUE(badge_matchAch_SilverScoreMod);
		READ_MOD_VALUE(badge_matchAch_GoldScoreMod);
		READ_MOD_VALUE(badge_USAAch_TimeForBronze);
		READ_MOD_VALUE(badge_USAAch_TimeForSilver);
		READ_MOD_VALUE(badge_USAAch_TimeForGold);
		READ_MOD_VALUE(badge_USAAch_BronzeScoreMod);
		READ_MOD_VALUE(badge_USAAch_SilverScoreMod);
		READ_MOD_VALUE(badge_USAAch_GoldScoreMod);
		READ_MOD_VALUE(badge_USSRAch_TimeForBronze);
		READ_MOD_VALUE(badge_USSRAch_TimeForSilver);
		READ_MOD_VALUE(badge_USSRAch_TimeForGold);
		READ_MOD_VALUE(badge_USSRAch_BronzeScoreMod);
		READ_MOD_VALUE(badge_USSRAch_SilverScoreMod);
		READ_MOD_VALUE(badge_USSRAch_GoldScoreMod);
		READ_MOD_VALUE(badge_NATOAch_TimeForBronze);
		READ_MOD_VALUE(badge_NATOAch_TimeForSilver);
		READ_MOD_VALUE(badge_NATOAch_TimeForGold);
		READ_MOD_VALUE(badge_NATOAch_BronzeScoreMod);
		READ_MOD_VALUE(badge_NATOAch_SilverScoreMod);
		READ_MOD_VALUE(badge_NATOAch_GoldScoreMod);
		READ_MOD_VALUE(badge_preOrdAch_TimeForBronze);
		READ_MOD_VALUE(badge_preOrdAch_TimeForSilver);
		READ_MOD_VALUE(badge_preOrdAch_TimeForGold);
		READ_MOD_VALUE(badge_preOrdAch_BronzeScoreMod);
		READ_MOD_VALUE(badge_preOrdAch_SilverScoreMod);
		READ_MOD_VALUE(badge_preOrdAch_GoldScoreMod);
		READ_MOD_VALUE(medal_nukeSpec_NumForBronze);
		READ_MOD_VALUE(medal_nukeSpec_NumForSilver);
		READ_MOD_VALUE(medal_nukeSpec_NumForStreak);
		READ_MOD_VALUE(medal_nukeSpec_StreakForGold);
		READ_MOD_VALUE(medal_nukeSpec_BronzeScoreMod);
		READ_MOD_VALUE(medal_nukeSpec_SilverScoreMod);
		READ_MOD_VALUE(medal_nukeSpec_GoldScoreMod);
	}
	
	return true; 
}

MT_Mutex MMS_PlayerStats::ourNewEntryAllocationLock;
MC_SmallObjectAllocator<sizeof(MMS_PlayerStats::PlayerStatsEntry), 16*1024> MMS_PlayerStats::ourEntryAllocator;

void* 
MMS_PlayerStats::PlayerStatsEntry::operator new(size_t aSize)
{
	MT_MutexLock locker(MMS_PlayerStats::ourNewEntryAllocationLock);
	return MMS_PlayerStats::ourEntryAllocator.Allocate();
}

void 
MMS_PlayerStats::PlayerStatsEntry::operator delete(void* aPointer)
{
	MT_MutexLock locker(MMS_PlayerStats::ourNewEntryAllocationLock);
	MMS_PlayerStats::ourEntryAllocator.Free(aPointer);
}

void 
MMS_PlayerStats::PrivLogSuspectEntry(PlayerStatsEntry* thePlayerEntry, MMG_Stats::PlayerMatchStats& someStats)
{
	MC_StaticString<8192> sql;
	sql.Format("INSERT INTO SuspectStats ("
		"profileId,rank,sc_tot,sc_inf,sc_highinf,sc_sup,sc_highsup,"
		"sc_arm,sc_higharm,sc_air,sc_highair,sc_damen,sc_ta,sc_cpc,sc_rep,sc_fort,"
		"sc_tk,sc_highest,t_USA,t_USSR,t_NATO,t_sup,t_arm,t_air,t_inf,"
		"n_ukills,n_ulost,n_cpc,n_rps,n_taps,n_nukes,"
		"n_tach) VALUES ("
		"%u,%u,%u,%u,%u,%u,%u,"
		"%u,%u,%u,%u,%u,%u,%u,%u,%u,"
		"%u,%u,%u,%u,%u,%u,%u,%u,%u,"
		"%u,%u,%u,%u,%u,%u,"
		"%u)", thePlayerEntry->profileId, thePlayerEntry->rank, thePlayerEntry->scoreTotal, someStats.scoreAsInfantry, thePlayerEntry->highScoreAsInfantry,	someStats.scoreAsSupport, thePlayerEntry->highScoreAsSupport,
		someStats.scoreAsArmor, thePlayerEntry->highScoreAsArmor,	someStats.scoreAsAir, thePlayerEntry->highScoreAsAir,	someStats.scoreByDamagingEnemies, someStats.scoreByUsingTacticalAids, someStats.scoreByCommandPointCaptures, someStats.scoreByRepairing, someStats.scoreByFortifying,
		someStats.scoreLostByKillingFriendly, thePlayerEntry->highestScore, someStats.timePlayedAsUSA, someStats.timePlayedAsUSSR, someStats.timePlayedAsNATO, someStats.timePlayedAsSupport, someStats.timePlayedAsArmor, someStats.timePlayedAsAir, someStats.timePlayedAsInfantry, 
		someStats.numberOfUnitsKilled, someStats.numberOfUnitsLost, someStats.numberOfCommandPointCaptures, someStats.numberOfReinforcementPointsSpent, someStats.numberOfTacticalAidPointsSpent, someStats.numberOfNukesDeployed, 
		someStats.numberOfTacticalAidCriticalHits);

	MDB_MySqlQuery query(*myWriteDatabaseConnection); 
	MDB_MySqlResult result; 
	query.Modify(result, sql.GetBuffer());
}
