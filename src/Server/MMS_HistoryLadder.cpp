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

#include "MC_Debug.h"
#include "MMS_HistoryLadder.h"
#include "MMS_InitData.h"
#include "MMS_MasterServer.h"
#include "MMS_Statistics.h"
#include "ML_Logger.h"

#include <math.h>
#include <time.h>

MT_Mutex MMS_HistoryLadder::myMutex; 

MMS_HistoryLadder::MMS_HistoryLadder(const MMS_Settings& theSettings, 
									 const char* aTableName, 
									 const unsigned int anUpdateTime)
: myTimeOfNextUpdate(0)
, myTimeOfNextRemoveInvalid(0)
, myTableName(aTableName)
, myUpdateInterval(24 * 60 * 60)
, myItemsAreDirty(true)
, myUpdateTime(anUpdateTime)
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

	LOG_INFO("Loading history ladder"); 

	LoadLadder(); 
	PrivSetupFriendsLadder(); 
}

MMS_HistoryLadder::~MMS_HistoryLadder()
{
	myWriteDatabaseConnection->Disconnect();
	delete myWriteDatabaseConnection;
	myReadDatabaseConnection->Disconnect();
	delete myReadDatabaseConnection;
}

void 
MMS_HistoryLadder::Update()
{
	PrivRemoveInvalid(); 
	PrivUpdateCache(); 
	PrivUpdateStats(); 
}

MMS_HistoryLadder::LadderItem::LadderItem() 
: myId(0)
, myScore(0) 
, myTailScore(0)
{
}

MMS_HistoryLadder::LadderItem::LadderItem(const unsigned int anId, 
										  const unsigned int aScore, 
										  const unsigned int aTailScore) 
: myId(anId)
, myScore(aScore) 
, myTailScore(aTailScore)
{
}

bool 
MMS_HistoryLadder::LadderItem::operator>(const LadderItem& aRhs) const 
{ 
	return myScore > aRhs.myScore; 
}

bool 
MMS_HistoryLadder::LadderItem::operator<(const LadderItem& aRhs) const 
{ 
	return myScore < aRhs.myScore; 
}

bool 
MMS_HistoryLadder::LadderItem::operator==(const LadderItem& aRhs) const 
{ 
	return myScore == aRhs.myScore; 
}

void 
MMS_HistoryLadder::Add(unsigned int anId, 
					   unsigned int aScore, 
					   unsigned int aTailScore)
{
	MT_MutexLock lock(myMutex);

	LadderItem *item = NULL; 
	PrivGetById(&item, anId); 
	if(item)
	{
		item->myScore += aScore; 
		myFriendsLadder.AddOrUpdate(anId, item->myScore); 
	}
	else 
	{
		myItems.Add(LadderItem(anId, aScore, aTailScore)); 		
		myFriendsLadder.AddOrUpdate(anId, aScore); 
	}

	myItemsAreDirty = true; 
}

unsigned int 
MMS_HistoryLadder::PrivGetById(LadderItem **aTargetItem,
							   const unsigned int anId)
{
	const unsigned int count = myItems.Count();
	int i = 0;

	if (count)
	{
		int iterations = (count+7)/8;
		switch(count%8)
		{
		case 0: do {		if (myItems[i++].myId == anId) goto match;
		case 7:				if (myItems[i++].myId == anId) goto match;
		case 6:				if (myItems[i++].myId == anId) goto match;
		case 5:				if (myItems[i++].myId == anId) goto match;
		case 4:				if (myItems[i++].myId == anId) goto match;
		case 3:				if (myItems[i++].myId == anId) goto match;
		case 2:				if (myItems[i++].myId == anId) goto match;
		case 1:				if (myItems[i++].myId == anId) goto match;
				} while (--iterations>0);
		}
	}
	if(aTargetItem)
		*aTargetItem = NULL;
	return -1;

match:
	assert(myItems[i - 1].myId == anId);
	if(aTargetItem)
		*aTargetItem = &myItems[i - 1];
	return i - 1;	
}

MMS_HistoryLadder::LadderItem* 
MMS_HistoryLadder::GetAtPosition(const unsigned int aPosition) 
{
	MT_MutexLock lock(myMutex); 

	if(aPosition < 0 || aPosition >= (unsigned int) myItems.Count())
		return NULL; 
	PrivSort(); 
	return &myItems[aPosition]; 
}

unsigned int 
MMS_HistoryLadder::GetLadderSize() const
{
	MT_MutexLock lock(myMutex); 

	return myItems.Count(); 
}

unsigned int 
MMS_HistoryLadder::GetScore(const unsigned int anId)
{
	MT_MutexLock lock(myMutex); 

	LadderItem *item = NULL; 
	if(PrivGetById(&item, anId) == -1)
		return 0; 
	return item->myScore; 
}

unsigned int 
MMS_HistoryLadder::GetPosition(const unsigned int anId)
{
	MT_MutexLock lock(myMutex); 

	PrivSort(); 
	return PrivGetById(NULL, anId); 
}

unsigned int 
MMS_HistoryLadder::GetPercentage(const unsigned int anId)
{
	MT_MutexLock lock(myMutex); 

	float numItems = float(__max(myItems.Count(), 1));
	unsigned int position = PrivGetById(NULL, anId);
	if(position == -1)
		return -1; 
	return 100 - unsigned int(((float)position*100.0f) / numItems);
}

void 
MMS_HistoryLadder::PrivSort()
{
	if(myItemsAreDirty)
	{
		myItems.Sort(-1, -1, true); 
		myItemsAreDirty = false; 	
	}
}

void 
MMS_HistoryLadder::Remove(const unsigned int anId)
{
	MT_MutexLock lock(myMutex); 

	unsigned int index = PrivGetById(NULL, anId); 
	if(index != -1)
	{
		// maybe we should update the database as well? 
		myItems.RemoveCyclicAtIndex(index); 
		myItemsAreDirty = true; 
	}
}

void 
MMS_HistoryLadder::RemoveAll()
{
	MT_MutexLock lock(myMutex);

	myItems.RemoveAll(); 
}

void 
MMS_HistoryLadder::LoadLadder()
{
	MDB_MySqlQuery query(*myWriteDatabaseConnection);

	MC_StaticString<256> sql;
	sql.Format("DELETE FROM %s WHERE dayOfEntry<DATE_ADD(NOW(), INTERVAL -%u DAY)", myTableName.GetBuffer(), MAX_AGE_IN_DAYS);
	MDB_MySqlResult res;
	MDB_MySqlRow row;

	if (!query.Modify(res, sql))
	{
		LOG_FATAL("Cannot delete from database. bailing out");
		return; 
	}

	sql.Format("SELECT Count(DISTINCT id) AS numItems FROM %s", myTableName.GetBuffer()); 
	if (!query.Modify(res, sql) || !res.GetNextRow(row))
	{
		LOG_FATAL("Cannot count in database. bailing out");
		return; 
	}

	const time_t t = time(NULL);
	struct tm* formatedTime = localtime(&t);
	time_t todaysSeconds = formatedTime->tm_hour * 60 * 60 + formatedTime->tm_min * 60 + formatedTime->tm_sec; 
	myTimeOfNextUpdate = (unsigned int) (t - todaysSeconds + myUpdateTime + myUpdateInterval); 

	unsigned int numItems = row["numItems"]; 

	myItems.Init(max(numItems, 128000), max(numItems, 128000), false); 
	myItemsAreDirty = true;

	unsigned int requestOffset = 0;

	bool foundResult;
	do
	{
		foundResult = false;
		sql.Format("SELECT " 
				   "id, " 
				   "SUM(scoreOnDay) AS totScore, " 
				   "IF(TO_DAYS(NOW())-%u=TO_DAYS(MIN(dayOfEntry)), scoreOnDay,0) AS tailScore " 
			       "FROM %s GROUP BY id ORDER BY id LIMIT %u,1000000", 
			       MAX_AGE_IN_DAYS, myTableName.GetBuffer(), requestOffset); 

		query.Ask(res, sql, true);
		while (res.GetNextRow(row))
		{
			foundResult = true;
			requestOffset++;
			const unsigned int id = row["id"];
			const unsigned int totScore = row["totScore"];
			const unsigned int tailScore = row["tailScore"]; 
			if(totScore)
				myItems.Add(LadderItem(id, totScore, tailScore)); 		
		}
	}
	while (foundResult);
}

void 
MMS_HistoryLadder::PrivUpdateCache()
{
	{
		MT_MutexLock lock(myMutex);
		if(myTimeOfNextUpdate > (unsigned int) time(NULL))
		{
			return; 
		}
		myTimeOfNextUpdate = (unsigned int) time(NULL) + myUpdateInterval; 
	}

	unsigned int startTime = GetTickCount();
	MC_StaticString<256> sql; 
	MDB_MySqlQuery query(*myWriteDatabaseConnection);
	MDB_MySqlResult res;
	MDB_MySqlRow row; 

	sql.Format("SELECT DATE_ADD(NOW(), INTERVAL -%u DAY) as oldDate, DATE_ADD(NOW(), INTERVAL -%u DAY) as tailDate", MAX_AGE_IN_DAYS, MAX_AGE_IN_DAYS - 1); 
	if(!query.Modify(res, sql) || !res.GetNextRow(row)) 
	{
		assert(false && "database error"); 
		return; 	
	}

	const MC_StaticString<32> oldDate = row["oldDate"]; 
	const MC_StaticString<32> tailDate = row["tailDate"]; 

	sql.Format("DELETE FROM %s WHERE dayOfEntry = '%s'", myTableName.GetBuffer(), oldDate.GetBuffer());	
	bool good = query.Modify(res, sql); 
	if (good && res.GetAffectedNumberOrRows())
	{
		MT_MutexLock lock(myMutex);
		unsigned int numItems = myItems.Count(); 
		for(unsigned int i = 0; i < numItems; i++)
		{
			myItems[i].myScore -= myItems[i].myTailScore; 
			myItems[i].myTailScore = 0; // we might not find a new one that would overwrite this value 
		}
	}

	// if we don't order by id the update tailscore loop below will suffer from n^2 complexity 
	sql.Format("SELECT id,scoreOnDay FROM %s WHERE dayOfEntry = '%s' ORDER BY id", 
		myTableName.GetBuffer(), tailDate.GetBuffer()); 
	good = good && query.Modify(res, sql); 
	if(!good)
	{
		assert(false && "database is dead");
		return; 
	}

	class SortByIdComparer
	{
	public: 
		static bool LessThan(const LadderItem& a, 
							 const LadderItem& b) 
		{ 
			return a.myId < b.myId;
		}
		static bool Equals(const LadderItem& a, 
						   const LadderItem& b)
		{ 
			return a.myId == b.myId; 
		}
	}; 


	// we have to lock here, PrivSort might otherwise sneak in behind our back 
	MT_MutexLock lock(myMutex);

	// if we don't sort by id the loop below will take forever to execute, 
	// it will get n^2 complexity rather than ~n 
	myItems.Sort<SortByIdComparer>(-1, -1, false); 
	myItemsAreDirty = true; 

	unsigned int lastIndex = -1; 
	unsigned int numProcessed = 0; 
	while(res.GetNextRow(row))
	{
		unsigned int id = row["id"];
		LadderItem *item = NULL;
		if(lastIndex != -1)
		{
			lastIndex++; 
			if(myItems[lastIndex].myId == id)
				item = &myItems[lastIndex]; 
		}
		if(!item)
			lastIndex = PrivGetById(&item, id); 
		if(item)
		{
			unsigned int tailScore = row["scoreOnDay"]; 
			// no locking needed, no one else should tamper with this value, 
			item->myTailScore = tailScore; 
		}

		numProcessed++; 
	}

	PrivRemoveZeroScored();

	PrivSetupFriendsLadder();
	MMS_MasterServer::GetInstance()->AddSample(__FUNCTION__, GetTickCount() - startTime);
}

void
MMS_HistoryLadder::PrivRemoveInvalid()
{
	if(myTimeOfNextRemoveInvalid > (unsigned int) time(NULL))
	{
		return; 
	}
	myTimeOfNextRemoveInvalid = (unsigned int) time(NULL) + 10; 

	MC_StaticString<256> sql; 
	MDB_MySqlQuery query(*myWriteDatabaseConnection);
	MDB_MySqlResult res;
	MDB_MySqlRow row; 
	MC_HybridArray<unsigned int, 64> removedItems; 
	unsigned int maxId = 0; 

	sql.Format("SELECT id,inTableId FROM _InvalidateLadder WHERE tableName = '%s' ORDER BY id LIMIT 64", 
		myTableName.GetBuffer()); 
	if (!query.Modify(res, sql))
	{
		LOG_FATAL("Couldn't query database"); 
		return; 
	}
	while(res.GetNextRow(row))
	{
		unsigned int id = row["id"]; 
		if(id > maxId)
			maxId = id; 
		removedItems.Add(unsigned int(row["inTableId"])); 
	}

	if(maxId == 0)
		return; 

	sql.Format("DELETE FROM _InvalidateLadder WHERE tableName = '%s' AND id <= %u", 
		myTableName.GetBuffer(), maxId); 
	if (!query.Modify(res, sql))
	{
		LOG_FATAL("Couldn't delete from database"); 
		return; 
	}

	const char sqlFirstPart[] = "DELETE FROM %s WHERE id IN ("; 
	const char sqlMiddlePart[] = "%u,"; 
	const char sqlLastPart[] = ")"; 
	char deleteSql[8129]; 
	unsigned int offset; 
	bool hasDeletedSomething = false; 

	offset = sprintf(deleteSql, sqlFirstPart, myTableName.GetBuffer()); 

	{
		MT_MutexLock lock(myMutex);

		myItemsAreDirty = true; 
		for(int i = 0; i < removedItems.Count(); i++)
		{
			unsigned int index = PrivGetById(NULL, removedItems[i]); 
			if(index != -1)
			{
				offset += sprintf(deleteSql + offset, sqlMiddlePart, myItems[index].myId); 
				myFriendsLadder.Remove(myItems[index].myId); 
				myItems.RemoveCyclicAtIndex(index);
				hasDeletedSomething = true; 
			}
		}
	}

	if(!hasDeletedSomething)
		return; 

	memcpy(deleteSql + offset - 1, sqlLastPart, sizeof(sqlLastPart));
	if(!query.Modify(res, deleteSql))
	{
		LOG_FATAL("could not delete from player ladder"); 
		return; 
	}
}

void
MMS_HistoryLadder::PrivRemoveZeroScored()
{
	PrivSort();

	for(int i = myItems.Count()-1; i >= 0 && myItems[i].myScore == 0; i--)
	{
		myItems.RemoveCyclicAtIndex(i);
	}
}

void	
MMS_HistoryLadder::PrivUpdateStats()
{
	MMS_Statistics* stats = MMS_Statistics::GetInstance(); 
	stats->SQLQueryServerTracker(myWriteDatabaseConnection->myNumExecutedQueries); 
	myWriteDatabaseConnection->myNumExecutedQueries = 0; 
	stats->SQLQueryServerTracker(myReadDatabaseConnection->myNumExecutedQueries); 
	myReadDatabaseConnection->myNumExecutedQueries = 0; 
}

// Friends 
void 
MMS_HistoryLadder::GetFriendsLadder(MMG_FriendsLadderProtocol::FriendsLadderReq& aLadderReq, 
									MMG_FriendsLadderProtocol::FriendsLadderRsp& aLadderRsp)
{
	myFriendsLadder.GetFriendsLadder(aLadderReq, aLadderRsp); 
}

void 
MMS_HistoryLadder::PrivSetupFriendsLadder()
{
	MT_MutexLock locker(myFriendsLadder.myMutex);
	myFriendsLadder.Purge();
	for(int i = 0; i < myItems.Count(); i++)
		myFriendsLadder.Add(myItems[i].myId, myItems[i].myScore); 
}

