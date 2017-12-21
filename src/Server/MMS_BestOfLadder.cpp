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

#include "MC_String.h"
#include "MC_Debug.h"
#include "MT_ThreadingTools.h"
#include "mi_time.h"
#include "MMS_InitData.h"
#include "MMS_MasterServer.h"
#include "MC_HybridArray.h"
#include "MC_Profiler.h"
#include "MMS_Statistics.h"

#include "MMS_BestOfLadder.h"

#include "ML_Logger.h"

#define LOG_EXECUTION_TIME(MESSAGE, START_TIME) \
	 { unsigned int currentTime = GetTickCount(); \
	 MMS_MasterServer::GetInstance()->AddSample(__FUNCTION__ #MESSAGE, currentTime - START_TIME); \
	 START_TIME = currentTime; } 

MT_Mutex				MMS_BestOfLadder::ourMutex;

MMS_BestOfLadder::MMS_BestOfLadder(
	const MMS_Settings& theSettings, 
	const char*			aTableName, 
	const unsigned int	anUpdateTime)
	: myReadSettingsTimeout(5 * 60 * 1000)		// 5 min
	, myPurgeEmptyTimeout(1000)					// Every second
	, myBatchUpdateTimeout(1000)
	, myInvalidateTimeout(10 * 1000)			// 10 seconds
	, myMaxEntryAge(DEFAULT_LADDER_ENTRY_AGE)
	, myMaxEntryCount(DEFAULT_LADDER_SCORE_COUNT)
	, myUpdateTime(anUpdateTime)
	, myTableName(aTableName)
	, myLadderIsDirty(true)
{
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

	myActiveUpdateList = &myUpdateListA;

	PrivReadSettings(myMaxEntryCount, myMaxEntryAge);
	LoadLadder();
	PrivSetupFriendsLadder();
}

MMS_BestOfLadder::~MMS_BestOfLadder()
{
	myWriteDatabaseConnection->Disconnect();
	delete myWriteDatabaseConnection;
	myReadDatabaseConnection->Disconnect();
	delete myReadDatabaseConnection;
}


void
MMS_BestOfLadder::Add(
	unsigned int aProfileId, 
	unsigned int aScore, 
	time_t		 aTimestamp)
{
	if(aScore == 0)
		return;

	unsigned int startTime = GetTickCount(); 
	unsigned int startTime2 = startTime;

	uint64			dbId;
	LadderAction	action;
	{
		MT_MutexLock	lock(ourMutex);

		unsigned int	index = PrivGetById(aProfileId);
		if(index != -1)
		{
			action = myItems[index].Update(aScore, aTimestamp, myMaxEntryCount, &dbId, this);
			myFriendsLadder.AddOrUpdate(aProfileId, myItems[index].myTotalScore);
		}
		else
		{
			LadderItem		item(aProfileId, this);
			action = item.Update(aScore, aTimestamp, myMaxEntryCount, &dbId, this);
			myFriendsLadder.AddOrUpdate(aProfileId, item.myTotalScore);
			myItems.Add(item);
		}
	}

	switch(action)
	{
	case MMS_BestOfLadder::LADDER_ENTRY_ADDED:
		{
			dbId = PrivAddLadderEntryToDb(aProfileId, aScore, aTimestamp);
			bool		deleteItAgain = false;

			{
				MT_MutexLock	lock(ourMutex);
				myLadderIsDirty = true;
				unsigned int	index = PrivGetById(aProfileId);

				if(index != -1)
					deleteItAgain = !myItems[index].UpdateId(dbId, aScore, aTimestamp, this);
				else
					deleteItAgain = true;
			}

			if(deleteItAgain)
			{
				MDB_MySqlQuery			query(*myWriteDatabaseConnection);
				MC_StaticString<256>	sql;
				sql.Format("DELETE FROM %s WHERE id=%I64u", myTableName.GetBuffer(), dbId);

				MDB_MySqlResult		result;
				if(!query.Modify(result, sql.GetBuffer()))
					assert(false && "Failed to delete newly created row");
			}
			LOG_EXECUTION_TIME(_Entry_Added, startTime);
		}
		break;

	case MMS_BestOfLadder::LADDER_ENTRY_UPDATED:
		{
			MT_MutexLock	lock(ourMutex);
			myLadderIsDirty = true;
			PrivUpdateLadderEntryInDb(dbId, aProfileId, aScore, aTimestamp);
		}
		LOG_EXECUTION_TIME(_Entry_Updated, startTime);
		break;
	}

	LOG_EXECUTION_TIME(_TotalTime, startTime2);
}

unsigned int
MMS_BestOfLadder::GetLadderSize() const
{
	MT_MutexLock		lock(ourMutex);
	return myItems.Count();
}

unsigned int
MMS_BestOfLadder::GetScore(const unsigned int aProfileId)
{
	MT_MutexLock	lock(ourMutex);
	unsigned int	index = PrivGetById(aProfileId);
	return (index != -1) ? myItems[index].myTotalScore : 0;
}

unsigned int
MMS_BestOfLadder::GetPosition(const unsigned int aProfileId)
{
	MT_MutexLock		lock(ourMutex);
	PrivSort();
	return PrivGetById(aProfileId);
}

unsigned int
MMS_BestOfLadder::GetPercentage(const unsigned int aProfileId)
{
	MT_MutexLock	lock(ourMutex);
	float			numItems; 
	unsigned int	position;

	PrivSort();

	numItems = float(max(myItems.Count(), 1));
	position = PrivGetById(aProfileId);

	if(position == -1)
		return -1;

	return 100 - unsigned int(((float)position*100.0f) / numItems);
}

void
MMS_BestOfLadder::Remove(const unsigned int aProfileId)
{
	MT_MutexLock	lock(ourMutex);
	unsigned int	index = PrivGetById(aProfileId);

	if(index != -1)
	{
		myItems.RemoveAtIndex(index);
		myFriendsLadder.Remove(aProfileId);
	}
}

void
MMS_BestOfLadder::RemoveAll()
{
	MT_MutexLock	lock(ourMutex);
	myItems.RemoveAll();
	myFriendsLadder.Purge();
}

void
MMS_BestOfLadder::LoadLadder()
{
	unsigned int startTime = GetTickCount();

	MC_StaticString<1024> sql; 

	sql.Format("SELECT "
		"IF(NOW() < DATE_ADD(DATE(NOW()), INTERVAL 10 HOUR), UNIX_TIMESTAMP(DATE_ADD(DATE(NOW()), INTERVAL 10 HOUR)), UNIX_TIMESTAMP(DATE_ADD(DATE_ADD(DATE(NOW()), INTERVAL 1 DAY), INTERVAL 10 HOUR))) AS nextLadderUpdateTime, "
		"IF(NOW() < DATE_ADD(DATE(NOW()), INTERVAL 10 HOUR), UNIX_TIMESTAMP(DATE_ADD(DATE_ADD(DATE(NOW()), INTERVAL -1 DAY), INTERVAL 10 HOUR)), UNIX_TIMESTAMP(DATE_ADD(DATE(NOW()), INTERVAL 10 HOUR))) AS previousLadderUpdateTime, "
		"Count(*) FROM %s", myTableName.GetBuffer()); 

	MDB_MySqlQuery query(*myWriteDatabaseConnection);
	MDB_MySqlResult res;
	MDB_MySqlRow row;

	if(!query.Ask(res, sql))
	{
		LOG_FATAL("FATAL ERROR. Could not query database! Assuming server is busted. Disconnecting it.");	
		assert(false && "busted sql, bailing out");
		return; 
	}

	res.GetNextRow(row);

	myTimeOfNextUpdate = row["nextLadderUpdateTime"];
	myPreviousLadderUpdateTime = row["previousLadderUpdateTime"]; 

	sql.Format("DELETE FROM %s WHERE entered < FROM_UNIXTIME(%u)", myTableName.GetBuffer(), myPreviousLadderUpdateTime - myMaxEntryAge - 24 * 3600);	// 1 day drift
	if(!query.Modify(res, sql.GetBuffer()))
		assert(false && "Could not delete old entries from ladder");

	sql.Format("SELECT COUNT(distinct profileId) AS count FROM %s", myTableName.GetBuffer());

	MDB_MySqlResult			result;
	if(!query.Ask(result, sql.GetBuffer()))
		assert(false && "Unable to count profiles in laddder");

	if(!result.GetNextRow(row))
		assert(false && "Failed to get ladder profile count");
	
	int			numProfiles = row["count"];
	myItems.Init(max(128000, numProfiles), max(128000, numProfiles));
	myLadderScoreEntries.Init(max(128000, numProfiles), max(128000, numProfiles), true);

	sql.Format("SELECT id,profileId, score, UNIX_TIMESTAMP(entered) AS entered FROM %s WHERE entered >= FROM_UNIXTIME(%u) ORDER BY profileId desc,score desc,entered desc",
		myTableName.GetBuffer(), myPreviousLadderUpdateTime - myMaxEntryAge);
	
	if(!query.Ask(result, sql.GetBuffer(), true))
		assert(false && "Unable to load ladder");
	
	int				lastProfileId = -1;
	LadderItem*		currentProfile;
	while(result.GetNextRow(row))
	{	
		int			profileId = row["profileId"];
		if(profileId != lastProfileId)
		{	
			LadderItem		item(profileId, this);
			myItems.Add(item);
			currentProfile = &myItems[myItems.Count()-1];
			lastProfileId = profileId;
		}

		if((unsigned int) currentProfile->Count(this) < myMaxEntryCount)
			currentProfile->ForceAdd(row["id"] ,row["score"], row["entered"], this);
	}
	
	myLadderIsDirty = true;
	PrivSort();

	LOG_EXECUTION_TIME(, startTime);
}

void
MMS_BestOfLadder::Update()
{
	unsigned int		startTime = GetTickCount();

	if(myInvalidateTimeout.HasExpired())
		PrivRemoveInvalidated();

	if(time(NULL) > myTimeOfNextUpdate)
	{
		PrivRemoveOld(myTimeOfNextUpdate - myMaxEntryAge);
		myTimeOfNextUpdate += 24 * 3600;	// 24h

		MT_MutexLock lock(ourMutex);
		myLadderIsDirty = true;
		PrivSort();
	}

	unsigned int	targetGameCount = myMaxEntryCount;
	time_t			targetDays = myMaxEntryAge;
	if(myReadSettingsTimeout.HasExpired() && PrivReadSettings(targetGameCount, targetDays))
	{
		PrivRefitToNewSettings(targetGameCount, targetDays);

		MT_MutexLock lock(ourMutex);
		myLadderIsDirty = true;
		PrivSort();
	}

	if(myPurgeEmptyTimeout.HasExpired())
		PrivRemoveEmpty();

	if(myBatchUpdateTimeout.HasExpired())
		PrivDoBatchUpdate();

	LOG_EXECUTION_TIME(, startTime);
}

// Friends 
void
MMS_BestOfLadder::GetFriendsLadder(
	MMG_FriendsLadderProtocol::FriendsLadderReq& aLadderReq, 
	MMG_FriendsLadderProtocol::FriendsLadderRsp& aLadderRsp)
{
	myFriendsLadder.GetFriendsLadder(aLadderReq, aLadderRsp); 
}

bool
MMS_BestOfLadder::PrivReadSettings(
	unsigned int&		aNumGames,
	time_t&				aNumDays)
{
	MDB_MySqlQuery		query(*myReadDatabaseConnection);
	MDB_MySqlResult		result;

	if(!query.Ask(result, "SELECT aValue,aVariable FROM Settings WHERE aVariable='BestOfLadderNumGames' or aVariable='BestOfLadderNumDays'"))
	{
		LOG_ERROR("Failed to re-read settings");
		return false;
	}

	MDB_MySqlRow		row;
	if(!result.GetNextRow(row))
	{
		LOG_ERROR("No setting found for best-of-ladder");
		return false;
	}
	
	bool		changed = false;
	do
	{
		MC_StaticString<256>	name = row["aVariable"];
		if(name == "BestOfLadderNumDays")
		{
			int		value = row["aValue"];
			if(value * 24 * 3600 != myMaxEntryAge)
			{
				aNumDays = value * 24 * 3600;
				changed = true;
			}
		}
		else if(name == "BestOfLadderNumGames")
		{
			int		value = row["aValue"];
			if(value != myMaxEntryCount)
			{
				aNumGames = value;
				changed = true;
			}
		}

	}
	while(result.GetNextRow(row));
	
	return changed;
}

unsigned int
MMS_BestOfLadder::PrivGetById(
	unsigned int aProfileId)
{
// 	for(int i = 0; i < myItems.Count(); i++)
// 		if(myItems[i].myProfileId == aProfileId)
// 			return i;
// 
// 	return -1;

	MT_MutexLock	lock(ourMutex);

	const unsigned int count = myItems.Count();
	int i = 0;

	if (count)
	{
		int iterations = (count+7)/8;
		switch(count%8)
		{
		case 0: do {		if (myItems[i++].myProfileId == aProfileId) goto match;
		case 7:				if (myItems[i++].myProfileId == aProfileId) goto match;
		case 6:				if (myItems[i++].myProfileId == aProfileId) goto match;
		case 5:				if (myItems[i++].myProfileId == aProfileId) goto match;
		case 4:				if (myItems[i++].myProfileId == aProfileId) goto match;
		case 3:				if (myItems[i++].myProfileId == aProfileId) goto match;
		case 2:				if (myItems[i++].myProfileId == aProfileId) goto match;
		case 1:				if (myItems[i++].myProfileId == aProfileId) goto match;
				} while (--iterations>0);
		}
	}

	return -1;
match:
	return i - 1;	
}

uint64
MMS_BestOfLadder::PrivAddLadderEntryToDb(
	unsigned int	aProfileId,
	unsigned int	aScore,
	time_t			aTimestamp)
{
	MDB_MySqlQuery	query(*myWriteDatabaseConnection);
	MC_StaticString<255>	sql;
	sql.Format("INSERT INTO %s (profileId, score, entered) VALUES (%u, %u, FROM_UNIXTIME(%u))",
		myTableName.GetBuffer(), aProfileId, aScore, aTimestamp);

	MDB_MySqlResult	result;
	if(!query.Modify(result, sql.GetBuffer()))
		assert(false && "Database failed :(");

	return query.GetLastInsertId();
}

void
MMS_BestOfLadder::PrivUpdateLadderEntryInDb(
	uint64			anId,
	unsigned int	aProfileId,
	unsigned int	aScore,
	time_t			aTimestamp)
{
	MT_MutexLock	lock(ourMutex);
	MMS_BestOfLadder::DelayedScoreUpdateInfo	info = { anId, aProfileId, aScore, aTimestamp };
	myActiveUpdateList->Add(info);
}

void
MMS_BestOfLadder::PrivRemoveEmpty()
{
	MT_MutexLock	lock(ourMutex);

	for(int i = 0; i < myItems.Count(); i++)
	{
		if(myItems[i].myTotalScore == 0)
		{
			myFriendsLadder.Remove(myItems[i].myProfileId);
			myItems.RemoveCyclicAtIndex(i--);
		}
	}
}

void
MMS_BestOfLadder::PrivDoBatchUpdate()
{
	unsigned int		startTime = GetTickCount();

	ourMutex.Lock();
	
	MC_HybridArray<MMS_BestOfLadder::DelayedScoreUpdateInfo, 2000>*		list = myActiveUpdateList;
	if(myActiveUpdateList == &myUpdateListA)
		myActiveUpdateList = &myUpdateListB;
	else
		myActiveUpdateList = &myUpdateListA;
	
	ourMutex.Unlock();

	MDB_MySqlQuery			query(*myWriteDatabaseConnection);
	MDB_MySqlResult			result;
	

	const char sqlFirstPart[] = "INSERT INTO %s (id, score, entered, profileId) VALUES "; 
	const char sqlMiddlePart[] = "(%I64u,%u,FROM_UNIXTIME(%u),%u),"; 
	const char sqlLastPart[] = " ON DUPLICATE KEY UPDATE score=VALUES(score), entered=VALUES(entered), profileId=VALUES(profileId)"; 
	char sql[32768]; 
	unsigned int offset; 

	for(int i = 0; i < list->Count(); i += 400)
	{
		offset = sprintf(sql, sqlFirstPart, myTableName.GetBuffer());

		for(int j = 0; j < 400 && i + j < list->Count(); j++)
		{
			MMS_BestOfLadder::DelayedScoreUpdateInfo& item = (*list)[j+i];
			offset += sprintf(sql + offset, sqlMiddlePart, item.id, item.score, (unsigned int) item.timestamp, item.profileId);
		}

		memcpy(sql + offset -1, sqlLastPart, sizeof(sqlLastPart));

		if(!query.Modify(result, sql))
			assert(false && "Failed to delete old entries from best-of-ladder");
	}

	/*
	MC_StaticString<256>	sql;
	int						offset = 0;

	for(int i = 0; i < list->Count(); i++)
	{
		MMS_BestOfLadder::DelayedScoreUpdateInfo	info = list->operator[](i);
		sql.Format("UPDATE %s SET score=%u, entered=FROM_UNIXTIME(%u) WHERE id=%I64u AND profileId=%d",
			myTableName.GetBuffer(), info.score, (unsigned int) info.timestamp, info.id, info.profileId);		// For some reason, time_t is 64-bits when it should be 32

		if(!query.Modify(result, sql.GetBuffer()))
			assert(false && "Could not update database");
	}

	*/ 

	list->RemoveAll();
	LOG_EXECUTION_TIME(, startTime);
}

void
MMS_BestOfLadder::PrivRefitToNewSettings(
	unsigned int		aTargetGameCount,
	time_t				aTargetDaysInSecs)
{
	unsigned int		startTime = GetTickCount();
	if(aTargetDaysInSecs < myMaxEntryAge)
	{
		time_t		purgeTime;
		{
			MT_MutexLock		lock(ourMutex);

			purgeTime = time(NULL) - aTargetDaysInSecs;
			for(int i = 0; i < myItems.Count(); i++)
			{
				myItems[i].RemoveOld(purgeTime, this);
				myFriendsLadder.AddOrUpdate(myItems[i].myProfileId, myItems[i].myTotalScore);
			}
		}

		MDB_MySqlQuery	query(*myWriteDatabaseConnection);
		MC_StaticString<255>	sql;
		sql.Format("DELETE FROM %s WHERE entered < FROM_UNIXTIME(%u)", myTableName.GetBuffer(), purgeTime);

		MDB_MySqlResult	result;
		if(!query.Modify(result, sql.GetBuffer()))
			assert(false && "Failed to remove old entries from best-of-ladder");
	}
	myMaxEntryAge = aTargetDaysInSecs;

	if(aTargetGameCount < myMaxEntryCount)
	{
		MC_GrowingArray<uint64>		deleteList(10000, 10000);

		while(myMaxEntryCount-- != aTargetGameCount)
		{
			{
				MT_MutexLock		lock(ourMutex);
				for(int i = 0; i < myItems.Count(); i++)
				{
					myItems[i].Refit(myMaxEntryCount, deleteList, this);
					myFriendsLadder.AddOrUpdate(myItems[i].myProfileId, myItems[i].myTotalScore);
				}
			}

			PrivDeleteEntriesByIdInDb(deleteList);
			deleteList.RemoveAll();
		}
	}
	myMaxEntryCount = aTargetGameCount;

	PrivRemoveEmpty();

	LOG_EXECUTION_TIME(, startTime);
}

void
MMS_BestOfLadder::PrivRemoveOld(
	time_t			aMinTime)
{
	{
		MT_MutexLock	lock(ourMutex);

		for(int i = 0; i < myItems.Count(); i++)
		{
			unsigned int		oldScore = myItems[i].myTotalScore;
			myItems[i].RemoveOld(aMinTime, this);
			if(oldScore != myItems[i].myTotalScore)
				myFriendsLadder.AddOrUpdate(myItems[i].myProfileId, myItems[i].myTotalScore);
		}
	}

	MDB_MySqlQuery	query(*myWriteDatabaseConnection);
	MDB_MySqlResult	result;

	MC_StaticString<256>	sql;
	sql.Format("DELETE FROM %s WHERE entered < FROM_UNIXTIME(%u)", myTableName.GetBuffer(), aMinTime - 24*3600); // Still 1-day drift

	if(!query.Modify(result, sql.GetBuffer()))
		assert(false && "Failed to remove old entries from database");
}

void
MMS_BestOfLadder::PrivSetupFriendsLadder()
{
	MT_MutexLock locker(myFriendsLadder.myMutex);
	myFriendsLadder.Purge();
	for(int i = 0; i < myItems.Count(); i++)
		myFriendsLadder.Add(myItems[i].myProfileId, myItems[i].myTotalScore); 
}

void
MMS_BestOfLadder::PrivSort()
{
	MT_MutexLock	lock(ourMutex);

	if(myLadderIsDirty)
	{
		myItems.Sort(-1, -1, true);
		myLadderIsDirty = false;
	}
}

// ===== LadderItem 

MMS_BestOfLadder::LadderItem::LadderItem()
	: myProfileId(0)
	, myTotalScore(0)
	, myLadderScoreIndex(-1)
{
}

MMS_BestOfLadder::LadderItem::LadderItem(
	const unsigned int aProfileId, 
	MMS_BestOfLadder* aLadder)
	: myProfileId(aProfileId)
	, myTotalScore(0)
{
	MC_HybridArray<LadderScore, DEFAULT_LADDER_SCORE_COUNT> t;
	aLadder->myLadderScoreEntries.Add(t);
	myLadderScoreIndex = aLadder->myLadderScoreEntries.Count() - 1;
}

bool
MMS_BestOfLadder::LadderItem::operator>(const LadderItem& aRhs) const
{
	return myTotalScore > aRhs.myTotalScore;
}

bool
MMS_BestOfLadder::LadderItem::operator<(const LadderItem& aRhs) const
{
	return myTotalScore < aRhs.myTotalScore;
}

bool
MMS_BestOfLadder::LadderItem::operator==(const LadderItem& aRhs) const
{
	return myTotalScore == aRhs.myTotalScore;
}

MMS_BestOfLadder::LadderAction
MMS_BestOfLadder::LadderItem::Update(
	unsigned int	aScore,
	time_t			aTimestamp,
	int				aMaxEntries,
	uint64*			anUpdatedId, 
	MMS_BestOfLadder* aLadder)
{
	MC_HybridArray<LadderScore, DEFAULT_LADDER_SCORE_COUNT>& entries = aLadder->myLadderScoreEntries[myLadderScoreIndex];

	if(entries.Count() < aMaxEntries)
	{
		myTotalScore += aScore;
		LadderScore		score = {0, aScore, aTimestamp};
		entries.Add(score);

		return MMS_BestOfLadder::LADDER_ENTRY_ADDED;
	}
	else
	{
		time_t			minTime = UINT_MAX;
		unsigned int	minScore = aScore;
		int				index = -1;

		for(int i = 0; i < entries.Count(); i++)
		{
			if(entries[i].myScore < minScore)
			{
				minScore = entries[i].myScore;
				minTime = entries[i].myTimestamp;
				index = i;
			}
			else if(entries[i].myScore == minScore && entries[i].myTimestamp < minTime)
			{
				minScore = entries[i].myScore;
				minTime = entries[i].myTimestamp;
				index = i;
			}
		}
		
		if(index == -1)
			return MMS_BestOfLadder::LADDER_ENTRY_IGNORED;

		myTotalScore += aScore - minScore;

		entries[index].myScore = aScore;
		entries[index].myTimestamp = aTimestamp;
		
		*anUpdatedId = entries[index].myId;
		return MMS_BestOfLadder::LADDER_ENTRY_UPDATED;

	}
}

bool
MMS_BestOfLadder::LadderItem::UpdateId(
	uint64				anId,
	unsigned int		aScore,
	time_t				aTimestamp, 
	MMS_BestOfLadder* aLadder)
{
	MC_HybridArray<LadderScore, DEFAULT_LADDER_SCORE_COUNT>& entries = aLadder->myLadderScoreEntries[myLadderScoreIndex];

	bool			found = false;
	for(int i = 0; i < entries.Count(); i++)
	{
		if(entries[i].myScore == aScore && entries[i].myTimestamp == aTimestamp)
		{
			entries[i].myId = anId;
			found = true;
			break;
		}
	}
	return found;
}

void
MMS_BestOfLadder::LadderItem::ForceAdd(
	unsigned int		anId,
	unsigned int		aScore,
	time_t				aTimestamp, 
	MMS_BestOfLadder*	aLadder)
{
	MC_HybridArray<LadderScore, DEFAULT_LADDER_SCORE_COUNT>& entries = aLadder->myLadderScoreEntries[myLadderScoreIndex];

	myTotalScore += aScore;
	LadderScore		score = {anId, aScore, aTimestamp};
	entries.Add(score);
}

void
MMS_BestOfLadder::LadderItem::ForceUpdateNoDb(
	unsigned int		anId,
	unsigned int		aScore,
	MMS_BestOfLadder*	aLadder)
{
	MC_HybridArray<LadderScore, DEFAULT_LADDER_SCORE_COUNT>& entries = aLadder->myLadderScoreEntries[myLadderScoreIndex];

	for(int i = 0; i < entries.Count(); i++)
	{
		if(entries[i].myId == anId)
		{
			myTotalScore += aScore - entries[i].myScore;
			entries[i].myScore = aScore;
			break;
		}
	}
}

void
MMS_BestOfLadder::LadderItem::RemoveOld(
	time_t					aMinTime, 
	MMS_BestOfLadder*		aLadder)
{
	MC_HybridArray<LadderScore, DEFAULT_LADDER_SCORE_COUNT>& entries = aLadder->myLadderScoreEntries[myLadderScoreIndex];

	for(int i = 0; i < entries.Count(); i++)
	{
		if(entries[i].myTimestamp < aMinTime)
		{
			myTotalScore -= entries[i].myScore;
			entries.RemoveAtIndex(i--);
		}
	}
}

void
MMS_BestOfLadder::LadderItem::Refit(
	int							aMaxEntries,
	MC_GrowingArray<uint64>&	aDeleteList, 
	MMS_BestOfLadder*			aLadder)
{
	MC_HybridArray<LadderScore, DEFAULT_LADDER_SCORE_COUNT>& entries = aLadder->myLadderScoreEntries[myLadderScoreIndex];

	if(entries.Count() <= aMaxEntries)
		return;

	entries.Sort(-1, -1, true);

	myTotalScore = 0;
	int			i;
	for(i = 0; i < aMaxEntries; i++)
		myTotalScore += entries[i].myScore;

	for(; i < entries.Count(); i++)
		aDeleteList.Add(entries[i].myId);

	entries.Truncate(aMaxEntries);
}

unsigned int 
MMS_BestOfLadder::LadderItem::Count(MMS_BestOfLadder* myLadder)
{
	return myLadder->myLadderScoreEntries[myLadderScoreIndex].Count();
}

void
MMS_BestOfLadder::PrivDeleteEntriesByIdInDb(
	MC_GrowingArray<uint64>& aDeleteList)
{
	const char sqlFirstPart[] = "DELETE FROM %s WHERE id IN ("; 
	const char sqlMiddlePart[] = "%I64u,"; 
	const char sqlLastPart[] = ")"; 
	char deleteSql[32768]; 
	unsigned int offset; 

	MDB_MySqlQuery		query(*myWriteDatabaseConnection);
	MDB_MySqlResult		result;

	for(int i = 0; i < aDeleteList.Count(); i += 1000)
	{
		offset = sprintf(deleteSql, sqlFirstPart, myTableName.GetBuffer()); 

		for(int j = 0; j < 1000 && i + j < aDeleteList.Count(); j++)
			offset += sprintf(deleteSql + offset, sqlMiddlePart, aDeleteList[i+j]);

		memcpy(deleteSql + offset -1, sqlLastPart, sizeof(sqlLastPart));

		if(!query.Modify(result, deleteSql))
			assert(false && "Failed to delete old entries from best-of-ladder");
	}
}

void
MMS_BestOfLadder::PrivRemoveInvalidated()
{
	MC_StaticString<256> sql; 
	MDB_MySqlQuery query(*myWriteDatabaseConnection);
	MDB_MySqlResult res;
	MDB_MySqlRow row; 
	MC_HybridArray<unsigned int, 64> removedItems; 
	unsigned int maxId = 0; 

	sql.Format("SELECT id,inTableId AS profileId FROM _InvalidateLadder WHERE tableName = '%s' ORDER BY id LIMIT 64", 
		myTableName.GetBuffer()); 
	if (!query.Modify(res, sql))
	{
		LOG_ERROR("Couldn't query database"); 
		return; 
	}

	while(res.GetNextRow(row))
	{
		unsigned int id = row["id"]; 
		if(id > maxId)
			maxId = id; 
		removedItems.Add(row["profileId"]); 
	}
	
	ourMutex.Lock();
	for(int i = 0; i < removedItems.Count(); i++){
		PrivForceReload(removedItems[i]);
	}
	ourMutex.Unlock();

	sql.Format("DELETE FROM _InvalidateLadder WHERE tableName='%s' AND id <= %u",
		myTableName.GetBuffer(), maxId);
	if(!query.Modify(res, sql.GetBuffer()))
		assert(false && "DB is broken");

	myLadderIsDirty = true;
}

void
MMS_BestOfLadder::PrivForceReload(
	int				aProfileId)
{
	for(int i = 0; i < myUpdateListA.Count(); i++)
	{
		if(myUpdateListA[i].profileId == aProfileId)
		{
			myUpdateListA.RemoveCyclicAtIndex(i);
			i--;
		}
	}

	for(int i = 0; i < myUpdateListB.Count(); i++)
	{
		if(myUpdateListB[i].profileId == aProfileId)
		{
			myUpdateListB.RemoveCyclicAtIndex(i);
			i--;
		}
	}

	MC_StaticString<255>		sql;
	sql.Format("SELECT id,score FROM %s WHERE profileId=%u",
		myTableName.GetBuffer(), aProfileId);

	MDB_MySqlQuery		query(*myReadDatabaseConnection);
	MDB_MySqlResult		result;
	
	if(!query.Ask(result, sql.GetBuffer()))
		assert(false && "Failed to query database");
	
	unsigned int index = PrivGetById(aProfileId);
	if(index != -1)
	{
		MDB_MySqlRow		row;
		while(result.GetNextRow(row))
		{
			myItems[index].ForceUpdateNoDb(row["id"], row["score"], this);
		}
	}
}