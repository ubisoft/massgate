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
#include "stdafx.h"

#include "MMS_PersistenceCache.h"

#include "mdb_stringconversion.h"

#include "MT_ReadWriteLock.h"

#include "MC_EggClockTimer.h"
#include "MC_KeyTreeInorderIterator.h"
#include "MC_SortedGrowingArray.h"
#include "MC_StackWalker.h"

#include "ML_Logger.h"

#include "MMS_MasterServer.h"

MMS_HashTable<uint32, ClientLUT*>		MMS_PersistenceCache::myLuts; 
MT_Mutex								MMS_PersistenceCache::myClientMutexes[MMS_PersistenceCache::NUM_MUTEXES];
MT_Mutex								MMS_PersistenceCache::myClientlistMutex;
volatile uint32							MMS_PersistenceCache::myNumRankDefinitions = 0;
MMS_PersistenceCache::RankDefinition	MMS_PersistenceCache::myRankDefinitions[256];
MC_HybridArray<ClientLUT*, 1>			MMS_PersistenceCache::myPurgeList; 
MC_HybridArray<ClientLUT*, 1>			MMS_PersistenceCache::myInsertPurgeList;
MT_Mutex								MMS_PersistenceCache::myInsertPurgeListLock; 

MMS_PersistenceCache::PurgeThread::PurgeThread()
{
	// Start(); 
}

void
MMS_PersistenceCache::PurgeThread::Run()
{
// 	MC_EggClockTimer moveToPurgeListTimer(1000);
// 	MC_EggClockTimer purgeTimer(1500); // based on the assumption that there are 300k luts and all will be visited in less than 10 minutes 
// 	int lastIndex = 0; 
// 
// 	while(!StopRequested())
// 	{
// 		if(moveToPurgeListTimer.HasExpired())
// 			PrivMoveFromInsertToPurgeList(); 
// 
// 		if(purgeTimer.HasExpired())
// 			lastIndex = PrivPurgeUnused(lastIndex); 
// 
// 		MC_Sleep(1000); 
// 	}
}

void 
MMS_PersistenceCache::StartPurgeThread()
{
	static PurgeThread* purgeThread = new PurgeThread(); 
}

ClientLUT*
MMS_PersistenceCache::GetClientLut(
	MDB_MySqlConnection*			aReadSqlConnection, 
	uint32							aProfileId, 
	uint32							anAccountId, 
	MMS_IocpServer::SocketContext*	aSocket)
{
	if (aProfileId == 0)
		return NULL;

	myClientlistMutex.Lock();

	ClientLUT* lut = NULL;

	if(!myLuts.Get(aProfileId, lut))
	{
		static __declspec(thread) uint32 assignedMutexIndex = 0;
		lut = new ClientLUT(&myClientMutexes[++assignedMutexIndex & (NUM_MUTEXES - 1)], aProfileId, anAccountId, aSocket);
		myLuts.Add(aProfileId, lut); 
	}

	assert(lut->profileId == aProfileId && "hash table gave us the wrong lut"); 
	// assert(lut->inUse == 0 && "got lut with use count != 0"); 

	if(lut)
		_InterlockedIncrement(&lut->inUse); // Prevent it from being purged.

	myClientlistMutex.Unlock();

	// Done operating on myClientLUTs

	if(lut)
	{
		// Lock the lut
		lut->myMutex->Lock();
		lut->myNumTimesUsed++;
		lut->myLastUpdateTime = time(NULL);
	}

	if(lut && aReadSqlConnection)
	{
		if(!lut->hasLoadedFromDatabase)
			PrivLoadLutFromDatabase(aReadSqlConnection, lut); 

		if(aSocket != NULL)
		{
			if(lut->theSocket)
			{
				if(lut->theSocket != aSocket)
				{
					LOG_ERROR("Client LUT socket mismatch, crash -> relogin before crash detected on profile %u?", aProfileId);
				}
			}
		}
	}
	else if(aReadSqlConnection)
	{
		LOG_INFO("Could not load LUT for profile %u", aProfileId);
	}

	return lut;
}

void
MMS_PersistenceCache::ReleaseClientLut(
	ClientLUT*& aClientLut)
{
	if (aClientLut == NULL)
		return;

	aClientLut->myMutex->Unlock();
	_InterlockedDecrement(&aClientLut->inUse);
	aClientLut = NULL;
}

void
MMS_PersistenceCache::FlushAllCaches()
{
	MT_MutexLock locker(myClientlistMutex);

	MMS_HashTable<uint32, ClientLUT*>::Iterator iter(myLuts); 
	while(iter.Next())
	{
		ClientLUT*& lut = iter.GetItem(); 

		_InterlockedIncrement(&lut->inUse);
		lut->GetMutex()->Lock();
		lut->Clear();
		lut->GetMutex()->Unlock();
		_InterlockedDecrement(&lut->inUse);
	}
}

void 
MMS_PersistenceCache::ReloadRankDefinitions(
	MDB_MySqlConnection* aReadSqlConnection) // Reload rank definitions from database and update all current luts
{
	MT_MutexLock locker(myClientlistMutex);

	LoadRankDefinitions(aReadSqlConnection);
	FlushAllCaches();
}

void 
MMS_PersistenceCache::LoadRankDefinitions(
	MDB_MySqlConnection* aReadSqlConnection)
{
	MT_MutexLock	locker(myClientlistMutex);
	MDB_MySqlQuery	query(*aReadSqlConnection);
	MDB_MySqlResult res;

	if (query.Ask(res, "SELECT id,totalScore,ladderPercentage FROM WICRankDefinitions ORDER BY id ASC"))
	{
		myNumRankDefinitions = 0;
		MDB_MySqlRow row;
		while (res.GetNextRow(row))
		{
			myRankDefinitions[myNumRankDefinitions].rankId = row["id"];
			myRankDefinitions[myNumRankDefinitions].minTotalScore = row["totalScore"];
			myRankDefinitions[myNumRankDefinitions].maxLadderPercentage = row["ladderPercentage"];
			myNumRankDefinitions++;
		}
	}
	else
	{
		LOG_ERROR("FATAL! COULD NOT GET RANK DEFINITIONS FOR WIC PROFILES!!!");
		assert(false);
	}
}

uint32
MMS_PersistenceCache::GetRank(
	uint32	aLadderPosition, 
	uint32	aTotalScore)
{
	return PrivGetHighestRank(aTotalScore, aLadderPosition);
}

void
MMS_PersistenceCache::GetValuesForNextRank(
	uint32	aRank,
	uint32	aTotalScore, 
	uint32&	aNextRank, 
	uint32&	aNeededLadderPercentage, 
	uint32&	aNeededScore)
{
	uint32	rankIndex = -1;

	for(uint32 i = 0; i < myNumRankDefinitions; i++)
	{
		if(aRank == myRankDefinitions[i].rankId)
		{
			rankIndex = i; 
			break; 
		}
	}

	// player is at highest rank or we couldn't find his rank ... ??? 
	if((rankIndex == -1) || (rankIndex == (myNumRankDefinitions - 1)))
	{
		aNextRank = -1; 
		aNeededLadderPercentage = -1; 
		aNeededScore = -1;
		return; 
	}

	aNextRank = myRankDefinitions[rankIndex + 1].rankId; 
	if(myRankDefinitions[aNextRank].minTotalScore < aTotalScore)
		aNeededScore = 0;
	else 
		aNeededScore = myRankDefinitions[aNextRank].minTotalScore - aTotalScore;
	aNeededLadderPercentage = myRankDefinitions[aNextRank].maxLadderPercentage; 
}

uint32 
MMS_PersistenceCache::PrivGetHighestRank(
	uint32	totalScore, 
	uint32	maxLadderPercent)
{
	uint32 best = myRankDefinitions[0].rankId;

	for (uint32 i = 1; i < myNumRankDefinitions; i++)
	{
		if ((myRankDefinitions[i].minTotalScore <= totalScore) && (myRankDefinitions[i].maxLadderPercentage <= maxLadderPercent))
			best = myRankDefinitions[i].rankId;
	}

	return best;
}

void
MMS_PersistenceCache::InvalidateManyClientLuts(
	MC_GrowingArray<ClientLUT*>& someLuts)
{
	MT_MutexLock locker(myClientlistMutex);

	// First purge the ones we can get a lock on (don't wait for locks)

	for (int i = 0; i < someLuts.Count(); i++)
	{
		ClientLUT* lut = someLuts[i];

		_InterlockedIncrement(&lut->inUse);

		if (lut->GetMutex()->TryLock())
		{
			lut->Clear();
			lut->GetMutex()->Unlock();
			_InterlockedDecrement(&lut->inUse);
			someLuts.RemoveCyclicAtIndex(i--);
		}
		else
		{
			_InterlockedDecrement(&lut->inUse);
		}
	}
	// Then wait for the rest
	for (int i = 0; i < someLuts.Count(); i++)
	{
		ClientLUT* lut = someLuts[i];

		_InterlockedIncrement(&lut->inUse);
		lut->GetMutex()->Lock();
		lut->Clear();
		lut->GetMutex()->Unlock();
		_InterlockedDecrement(&lut->inUse);
		someLuts.RemoveCyclicAtIndex(i--);
	}
}

void
MMS_PersistenceCache::InvalidateManyClientLuts(
	const uint32*	someProfilesArray, 
	int				aNumProfileIds)
{
	MT_MutexLock locker(myClientlistMutex);
	
	while(aNumProfileIds--)
	{
		ClientLUT* lut = NULL; 
		if(!myLuts.Get(someProfilesArray[aNumProfileIds], lut))
			continue; 

		_InterlockedIncrement(&lut->inUse);
		lut->GetMutex()->Lock();
		lut->Clear();
		lut->GetMutex()->Unlock();
		_InterlockedDecrement(&lut->inUse);
	}
}


void
MMS_PersistenceCache::PrivLoadLutFromDatabase(
	MDB_MySqlConnection* aReadSqlConnection, 
	ClientLUT* aClient)
{
	// The lut system is optimized for getting many luts at a time (since that is the normal case). So, we must
	// construct 

	__declspec(thread) static MC_GrowingArray<ClientLUT*>* tempArrray = NULL;

	if (tempArrray == NULL)
	{
		tempArrray = new MC_GrowingArray<ClientLUT*>();
		tempArrray->Init(1,1,false);
	}
	else
	{
		tempArrray->RemoveAll();
	}
	tempArrray->Add(aClient);

	PrivLoadManyLutsFromDatabase(aReadSqlConnection, *tempArrray);
}

uint32
MMS_PersistenceCache::PrivLoadManyLutsFromDatabase(
	MDB_MySqlConnection* aReadSqlConnection, 
	MC_GrowingArray<ClientLUT*>& someLuts)
{
	uint32 startTime = GetTickCount(); 

	const uint32 numLutsToLoad = __min(128,someLuts.Count());

	_set_printf_count_output(1); // enable %n

	// Build the "IN (1,2,3,4,6)" string.
	char sqlIn[16*1024];
	char* sqlInPtr = sqlIn;


	for (uint32 i = 0; i < numLutsToLoad; i++)
	{
		if (someLuts[i]->profileId == 0)
		{
			LOG_ERROR("Tried to load profile 0, %u profiles requested.", someLuts.Count());
			return numLutsToLoad; // Pretend that all failed
		}
		someLuts[i]->Clear();

		int len = 0;
		sprintf(sqlInPtr, "%u,%n", someLuts[i]->profileId, &len);
		sqlInPtr += len;
	}
	*(sqlInPtr-1) = 0; // overwrite last ,


	MC_StaticString<16*1048> sqlString;

	sqlString.Format(""
		"SELECT "
		"IF(ISNULL(Clans.tagFormat),'P',Clans.tagFormat) AS tagFormat,"
		"IF(ISNULL(Clans.shortName),'',Clans.shortName) AS shortName,"
		"Profiles.profileId, Profiles.accountId, Profiles.clanId, Profiles.rankInClan, Profiles.profileName, Profiles.communicationOptions, " 
		"PlayerStats.sc_tot, PlayerStats.maxLadderPercent, PlayerStats.rank "
		"FROM PlayerStats, Clans RIGHT JOIN Profiles ON Clans.clanId=Profiles.clanId WHERE Profiles.profileId IN(%s) AND "
		"Profiles.profileId=PlayerStats.profileId"
		, sqlIn);


	// Get properties of the player we are creating the lut for
	MDB_MySqlQuery query(*aReadSqlConnection);
	MDB_MySqlResult res;
	if (query.Ask(res, sqlString, true))
	{
		MDB_MySqlRow row;
		while (res.GetNextRow(row))
		{
			// Find the lut we got the result row for
			for (uint32 i = 0; i < numLutsToLoad; i++)
			{
				ClientLUT* lut = someLuts[i];
				if (lut->profileId == uint32(row["profileId"]))
				{
					lut->clanId = row["clanId"];
					lut->rankInClan = unsigned char(int(row["rankInClan"]));
					lut->accountId = row["accountId"];
					convertFromMultibyteToWideChar<MMG_ProfilenameString::static_size>(lut->profileName, row["profileName"]);

					lut->totalScore = row["sc_tot"];
					lut->maxLadderPercentage = row["maxLadderPercent"];
					lut->rank = (int)row["rank"];
					lut->communicationOptions = (uint32) row["communicationOptions"]; 
					lut->hasLoadedFromDatabase = true;

					if (lut->clanId)
					{
						MC_StaticLocString<32> shortName, tagFormat;
						convertFromMultibyteToWideChar<32>(shortName, row["shortName"]);
						convertFromMultibyteToWideChar<32>(tagFormat, row["tagFormat"]);

						MMG_Profile p;
						lut->PopulateProfile(p);
						p.CreateTaggedName(tagFormat.GetBuffer(), shortName.GetBuffer());
						lut->profileName = p.myName;
					}

// 					MT_MutexLock lock(myInsertPurgeListLock);
// 					myInsertPurgeList.Add(lut); 

					break;
				}
			}
		}
	}
	
	uint32 stopTime = GetTickCount(); 
	PrivAddSample(__FUNCTION__, stopTime - startTime); 

	return numLutsToLoad;
}

void
MMS_PersistenceCache::PrivMoveFromInsertToPurgeList()
{
	MT_MutexLock lock(myInsertPurgeListLock);

	for(int i = 0; i < myInsertPurgeList.Count(); i++)
		myPurgeList.Add(myInsertPurgeList[i]);

	myInsertPurgeList.RemoveAll(); 
}

int
MMS_PersistenceCache::PrivPurgeUnused(
	int aStartIndex)
{
	typedef struct 
	{
		ClientLUT* myLut;
		int myPurgeListIndex; 
	} 
	CandidateForRemoval;

	uint32 startTime1 = GetTickCount(); 

	MC_HybridArray<CandidateForRemoval, 1000> candidatesForRemoval;  
	time_t minAge = time(NULL) - 600; 
	int numToRemove = 1000; 

	int purgeListSize = myPurgeList.Count(); 
	for(; aStartIndex < purgeListSize && numToRemove; aStartIndex++, numToRemove--)
	{
		ClientLUT* lut = myPurgeList[aStartIndex]; 

		if(lut->myMutex->TryLock())
		{
			if ((lut->myState == OFFLINE) && 
				(lut->myInterestedParties.Count() == 0) && 
				(lut->inUse == 0) && 
				(lut->myLastUpdateTime < minAge ))
			{
				CandidateForRemoval cfr; 
				cfr.myLut = lut; 
				cfr.myPurgeListIndex = aStartIndex; 
				candidatesForRemoval.Add(cfr); 
			}
			lut->myMutex->Unlock();
		}
	}

	int numLutsRemoved = 0; 

	uint32 stopTime1 = GetTickCount(); 
	MT_MutexLock lock(myClientlistMutex); 
	uint32 startTime2 = GetTickCount(); 
	for(int i = candidatesForRemoval.Count() - 1; i >= 0; i--)
	{
		CandidateForRemoval& cfr = candidatesForRemoval[i]; 
		
		if(cfr.myLut->myMutex->TryLock())
		{
			if ((cfr.myLut->myState == OFFLINE) && 
				(cfr.myLut->myInterestedParties.Count() == 0) && 
				(cfr.myLut->inUse == 0) && 
				(cfr.myLut->myLastUpdateTime < minAge ))
			{
				myLuts.Remove(cfr.myLut->profileId); 
				cfr.myLut->myMutex->Unlock(); 
				delete cfr.myLut;

				myPurgeList.RemoveCyclicAtIndex(cfr.myPurgeListIndex); 
				numLutsRemoved++; 
			}
			else 
			{
				cfr.myLut->myMutex->Unlock(); 
			}
		}
	}

	if(aStartIndex == purgeListSize)
		aStartIndex = 0; 

	uint32 stopTime2 = GetTickCount();

	PrivAddSample("MMS_PersistenceCache Purge list first pass", stopTime1 - startTime1); 
	PrivAddSample("MMS_PersistenceCache Purge list second pass", stopTime2 - startTime2); 

	return aStartIndex; 
}

int
MMS_PersistenceCache::GetNumberOfLuts()
{
	MT_MutexLock	lock(myClientlistMutex);

	return myLuts.Count(); 
}

void			
MMS_PersistenceCache::PrivAddSample(
	const char* aString, 
	uint32		aTimeUsed)
{
	MMS_MasterServer::GetInstance()->AddSample(aString, aTimeUsed); 
}

void
MMS_PersistenceCache::PopulateWithClientSockets(
	MC_GrowingArray<MMS_IocpServer::SocketContext*>& theFillArray, 
	const MC_GrowingArray<uint32>&	theProfiles, 
	uint32							exceptProfile)
{
	myClientlistMutex.Lock(); 

	for (int i = 0; i < theProfiles.Count(); i++)
	{
		if (theProfiles[i] == exceptProfile)
			continue;

		ClientLUT* lut = NULL; 
		if(!myLuts.Get(theProfiles[i], lut))
			continue; 

		if(lut->myState != OFFLINE && lut->theSocket)
			theFillArray.Add(lut->theSocket); 
	}

	myClientlistMutex.Unlock(); 

	MC_Algorithm::Sort(theFillArray.GetBuffer(), theFillArray.GetBuffer() + theFillArray.Count());
}

void 
ClientLUT::AddInterestedParty(uint32 aProfileId)
{
	myInterestedParties.AddUnique(aProfileId);
}

void 
ClientLUT::RemoveInterestedParty(uint32 aProfileId)
{
	if (!myInterestedParties.Remove(aProfileId))
	{
		LOG_ERROR("%u could not find interested party %u", profileId, aProfileId);
	}
}
