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
#include "MMS_EloLadder.h"
#include "MMS_InitData.h"
#include "MMS_AccountAuthenticationUpdater.h"
#include "MC_String.h"
#include "MC_Debug.h"
#include "MT_ThreadingTools.h"
#include "mi_time.h"
#include "MC_HybridArray.h"
#include "MC_Profiler.h"
#include "MMS_Statistics.h"
#include "ML_Logger.h"

#include <math.h>
#include <time.h>

MT_Mutex MMS_EloLadder::myMutex; 

MMS_EloLadder::MMS_EloLadder(const MMS_Settings& theSettings, 
							 const char* aTableName, 
							 const unsigned int anUpdateTime)
: myTimeOfNextUpdate(0)
, myTimeOfNextRemoveInvalid(0)
, myItemsAreDirty(true)
, myUpdateInterval(24 * 60 * 60)
, myTableName(aTableName)
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

	LOG_INFO("Loading elo ladder"); 

	LoadLadder(); 
}

MMS_EloLadder::~MMS_EloLadder()
{
	myWriteDatabaseConnection->Disconnect();
	delete myWriteDatabaseConnection;
	myReadDatabaseConnection->Disconnect();
	delete myReadDatabaseConnection;
}

// if changing 1500 to something else, also change in MMG_ClanColosseumProtocol::GetRsp
float MMS_EloLadder::LadderItem::START_RATING = 1500.0f;
float MMS_EloLadder::LadderItem::START_DEVIATION = 120.0f;

MMS_EloLadder::LadderItem::LadderItem()
: myId(0)
, myRating(0.0f)
, myLadderRating(0.0f)
, myDeviation(0.0f)
, myGracePeriodEnd(0)
{
}

MMS_EloLadder::LadderItem::LadderItem(const unsigned int anId, 
									  const float aRating, 
									  const float aDeviation, 
									  const unsigned int aGracePeriodEnd)
: myId(anId)
, myRating(aRating)
, myDeviation(aDeviation)
, myGracePeriodEnd(aGracePeriodEnd)
{
}

bool 
MMS_EloLadder::LadderItem::operator>(const LadderItem& aRhs) const
{
	return myLadderRating > aRhs.myLadderRating; 
}

bool 
MMS_EloLadder::LadderItem::operator<(const LadderItem& aRhs) const
{
	return myLadderRating < aRhs.myLadderRating; 
}

bool 
MMS_EloLadder::LadderItem::operator==(const LadderItem& aRhs) const
{
	return myLadderRating == aRhs.myLadderRating; 
}

void 
MMS_EloLadder::LadderItem::operator=(const LadderItem& aRhs)
{
	myId = aRhs.myId; 
	myRating = aRhs.myRating; 
	myDeviation = aRhs.myDeviation; 
	myLadderRating = aRhs.myLadderRating;
	myGracePeriodEnd = aRhs.myGracePeriodEnd; 
}

void 
MMS_EloLadder::LadderItem::SetScore(const float aScore, 
									const float anOpRating, 
									const unsigned int aCurrentTime)
{
	float t = aScore; 
	if(aScore < 0.0f)
		t = 0.0f; 
	else if(aScore > 1.0f)
		t = 1.0f; 

	float rDiff = (anOpRating - myRating) / 400.0f;
	float winProb = 1.0f / (1.0f + powf(10.0f, rDiff));

	myRating = floorf(myRating + myDeviation * (t - winProb) + 0.5f); 
	myDeviation = floorf(0.98f * myDeviation + 0.6f); 

	UpdateGracePeriod(aCurrentTime); 
	DecayRating(aCurrentTime); 
}

void 
MMS_EloLadder::LadderItem::DecayRating(const unsigned int aCurrentTime)
{
	float rm = 1.0f; 

	if(myGracePeriodEnd < aCurrentTime)
	{
		float d = (float)(aCurrentTime - myGracePeriodEnd) / (24.0f * 60.0f * 60.0f); 
		rm = pow(0.97f, d); 
		rm = max(rm, 0.1f); 
	}

	myLadderRating = myRating * rm; 	
}

void 
MMS_EloLadder::LadderItem::UpdateGracePeriod(const unsigned int aCurrentTime)
{
	if(myGracePeriodEnd > aCurrentTime)
	{
		myGracePeriodEnd = aCurrentTime + GRACE_PERIOD_MAX_LENGTH; 
	}
	else 
	{
		if(aCurrentTime - myGracePeriodEnd > GRACE_PERIOD_MAX_LENGTH)
		{
			myGracePeriodEnd = aCurrentTime - GRACE_PERIOD_MAX_LENGTH; 
		}
		myGracePeriodEnd += GRACE_PERIOD_MIN_UPDATE; 		
	}
}

void 
MMS_EloLadder::Add(const unsigned int aClanId, 
				   const float aRating, 
				   const float aDeviation, 
				   const unsigned int aGracePeriodEnd)
{
	MT_MutexLock lock(myMutex); 

	myItemsAreDirty = true; 
	myItems.Add(LadderItem(aClanId, aRating, aDeviation, aGracePeriodEnd)); 
}

unsigned int 
MMS_EloLadder::PrivGetById(LadderItem **aTargetItem,
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
	assert(myItems[i-1].myId == anId);
	if(aTargetItem)
		*aTargetItem = &myItems[i-1];
	return i-1;	
}

void 
MMS_EloLadder::PrivSort()
{
	if(myItemsAreDirty)
	{
		myItems.Sort(-1, -1, true); 
		myItemsAreDirty = false; 
	}
}

void 
MMS_EloLadder::ReportGame(const unsigned int aAId, 
						  const unsigned int aBId, 
						  unsigned int aAScore, 
						  unsigned int aBScore)
{
	// ask the DB about the current time 
	MDB_MySqlResult res; 
	MDB_MySqlRow row; 
	MDB_MySqlQuery readQuery(*myReadDatabaseConnection); 
	MC_StaticString<64> timeSql = "SELECT UNIX_TIMESTAMP(NOW()) AS currentTime";

	if(!readQuery.Ask(res, timeSql))
	{
		LOG_FATAL("FATAL ERROR. Could not query database! Assuming server is busted. Disconnecting it.");	
		return; 
	}

	if(!res.GetNextRow(row))
	{
		LOG_FATAL("FATAL ERROR. Could not query database! Assuming server is busted. Disconnecting it.");	
		return; 		
	}
	unsigned int currentTime = row["currentTime"]; 

	unsigned int tmpAGPE, tmpBGPE, tmpARating, tmpBRating, tmpADeviation, tmpBDeviation; 
	LadderItem *itemA = NULL; 
	LadderItem *itemB = NULL; 

	// if clan not found, add it 
	{
		MT_MutexLock lock(myMutex); 

		if(PrivGetById(&itemA, aAId) == -1)
		{
			LadderItem ladderItem(aAId, LadderItem::START_RATING, LadderItem::START_DEVIATION, 
				currentTime + LadderItem::GRACE_PERIOD_MAX_LENGTH); 
			myItems.Add(ladderItem); itemA = &myItems[myItems.Count() - 1]; 
			assert(itemA->myId == aAId && "MC_GrowingArray have changed add implementaiton"); 
		}
		if(PrivGetById(&itemB, aBId) == -1)
		{
			LadderItem ladderItem(aBId, LadderItem::START_RATING, LadderItem::START_DEVIATION, 
				currentTime + LadderItem::GRACE_PERIOD_MAX_LENGTH); 
			myItems.Add(ladderItem); itemB = &myItems[myItems.Count() - 1]; 
			assert(itemB->myId == aBId && "MC_GrowingArray have changed add implementaiton"); 
		}

		/* win loss draw scoring */ 
		float AScore = 0.0f; 
		float BScore = 0.0f; 
		if(aAScore > aBScore)
		{
			AScore = 1.0f; 
		}
		else if(aAScore < aBScore)
		{
			BScore = 1.0f; 
		}
		else 
		{
			AScore = 0.5f; 
			BScore = 0.5f; 
		}

		float ARating = itemA->myRating; 
		float BRating = itemB->myRating; 

		itemA->SetScore(AScore, BRating, myPreviousLadderUpdateTime); 
		itemB->SetScore(BScore, ARating, myPreviousLadderUpdateTime);

		tmpAGPE = itemA->myGracePeriodEnd; 
		tmpBGPE = itemB->myGracePeriodEnd; 
		tmpARating = (unsigned int)(itemA->myRating + 0.5f); 
		tmpBRating = (unsigned int)(itemB->myRating + 0.5f); 
		tmpADeviation = (unsigned int)(itemA->myDeviation + 0.5f); 
		tmpBDeviation = (unsigned int)(itemB->myDeviation + 0.5f); 
	}

	// insert into database 
	myItemsAreDirty = true; 

	MC_StaticString<1024> ladderSql;
	ladderSql.Format("INSERT INTO %s(id, gracePeriodEnd, rating, deviation) "
		"VALUES (%u, FROM_UNIXTIME(%u), %u, %u), (%u, FROM_UNIXTIME(%u), %u, %u) "
		"ON DUPLICATE KEY UPDATE "
		"gracePeriodEnd = VALUES(gracePeriodEnd), "
		"rating = VALUES(rating), "
		"deviation = VALUES(deviation)", 
		myTableName.GetBuffer(), 
		aAId, tmpAGPE, tmpARating, tmpADeviation, 
		aBId, tmpBGPE, tmpBRating, tmpBDeviation); 

	MDB_MySqlQuery writeQuery(*myWriteDatabaseConnection);
	if(!writeQuery.Modify(res, ladderSql))
	{
		LOG_FATAL("FATAL ERROR. Could not insert into %s! Assuming server is busted. Disconnecting it.", 
			myTableName.GetBuffer());	
		return; 
	}
}

/*
MMS_EloLadder::LadderItem* 
MMS_EloLadder::GetAtPosition(const unsigned int aPosition)
{
	MT_MutexLock lock(myMutex); 

	if(aPosition < 0 || aPosition >= (unsigned int) myItems.Count())
		return NULL; 
	PrivSort(); 
	return &myItems[aPosition]; 
}
*/

MMS_EloLadder::LadderItem* 
MMS_EloLadder::GetById(const unsigned int anId)
{
	MT_MutexLock lock(myMutex);

	LadderItem *clan; 
	PrivGetById(&clan, anId); 
	return clan; 
}

unsigned int 
MMS_EloLadder::GetLadderSize() const
{
	MT_MutexLock lock(myMutex);

	return myItems.Count(); 
}

unsigned int 
MMS_EloLadder::GetScore(const unsigned int anId)
{
	MT_MutexLock lock(myMutex);

	LadderItem *item; 
	if(PrivGetById(&item, anId) != -1)
		return (unsigned int) item->myLadderRating; 
	return -1; 
}

float 
MMS_EloLadder::GetEloScore(const unsigned int anId)
{
	MT_MutexLock lock(myMutex);

	LadderItem *item; 
	if(PrivGetById(&item, anId) != -1)
		return item->myRating; 
	return LadderItem::START_RATING; 
}

unsigned int 
MMS_EloLadder::GetPosition(const unsigned int anId)
{
	MT_MutexLock lock(myMutex);

	PrivSort(); 
	return PrivGetById(NULL, anId); 
}

unsigned int 
MMS_EloLadder::GetPercentage(const unsigned int anId)
{
	MT_MutexLock lock(myMutex);

	PrivSort(); 
	float numItems = float(__max(myItems.Count(), 1));
	unsigned int position = PrivGetById(NULL, anId);
	if(position == -1)
		return -1; 
	return 100 - unsigned int(((float)position*100.0f) / numItems);
}

void 
MMS_EloLadder::Remove(const unsigned int anId)
{
	MT_MutexLock lock(myMutex); 

	unsigned int index = PrivGetById(NULL, anId);
	if(index != -1)
	{
		myItems.RemoveCyclicAtIndex(index); 
		myItemsAreDirty = true; 
	}
}

void 
MMS_EloLadder::RemoveAll()
{
	MT_MutexLock lock(myMutex);

	myItems.RemoveAll(); 
}

void 
MMS_EloLadder::LoadLadder()
{
	MT_MutexLock lock(myMutex);
	MC_StaticString<1024> sql; 
	
	sql.Format("SELECT "
		"IF(NOW() < DATE_ADD(DATE(NOW()), INTERVAL 11 HOUR), UNIX_TIMESTAMP(DATE_ADD(DATE(NOW()), INTERVAL 11 HOUR)), UNIX_TIMESTAMP(DATE_ADD(DATE_ADD(DATE(NOW()), INTERVAL 1 DAY), INTERVAL 11 HOUR))) AS nextLadderUpdateTime, "
		"IF(NOW() < DATE_ADD(DATE(NOW()), INTERVAL 11 HOUR), UNIX_TIMESTAMP(DATE_ADD(DATE_ADD(DATE(NOW()), INTERVAL -1 DAY), INTERVAL 11 HOUR)), UNIX_TIMESTAMP(DATE_ADD(DATE(NOW()), INTERVAL 11 HOUR))) AS previousLadderUpdateTime, "
			   "Count(*) FROM %s", myTableName.GetBuffer()); 

	MDB_MySqlQuery query(*myReadDatabaseConnection);
	MDB_MySqlResult res;
	MDB_MySqlRow row;

	if(!query.Ask(res, sql))
	{
		LOG_FATAL("FATAL ERROR. Could not query database! Assuming server is busted. Disconnecting it.");	
		return; 
	}

	res.GetNextRow(row);

	// myTimeOfNextUpdate = (unsigned int) time(NULL) + myUpdateInterval; 	

// 	const time_t t = time(NULL);
// 	struct tm* formatedTime = localtime(&t);
// 	time_t todaysSeconds = formatedTime->tm_hour * 60 * 60 + formatedTime->tm_min * 60 + formatedTime->tm_sec; 
//	myTimeOfNextUpdate = (unsigned int) (t - todaysSeconds + myUpdateTime + myUpdateInterval); 

	myTimeOfNextUpdate = row["nextLadderUpdateTime"];
	myPreviousLadderUpdateTime = row["previousLadderUpdateTime"]; 

	myItemsAreDirty = true; 
	unsigned int numLadderItems = row["Count(*)"];
	myItems.Init(numLadderItems + 10240, 10240, false);

	for (unsigned int i = 0; i < numLadderItems; i += 1000000)
	{
		sql.Format("SELECT id, rating, deviation, UNIX_TIMESTAMP(gracePeriodEnd) AS gracePeriodEndUT "
				   "FROM %s " 
				   "ORDER BY id "
				   "LIMIT %u,1000000", 
				   myTableName.GetBuffer(), i);

		if(!query.Ask(res, sql, true))
		{
			LOG_FATAL("FATAL ERROR. Could not query database! Assuming server is busted. Disconnecting it.");	
			return; 
		}

		while (res.GetNextRow(row))
		{
			const unsigned int id = row["id"];
			const unsigned int rating = row["rating"]; 
			const unsigned int deviation = row["deviation"]; 
			const unsigned int gracePeriodEnd = row["gracePeriodEndUT"]; 

			LadderItem item(id, (float)rating, (float)deviation, gracePeriodEnd); 
			myItems.Add(item); 
		}
	}

	// when starting server decay everything as if it was the update time last day 
	// so we don't have to wait a day or so for updated ladder 
	const unsigned int numItems = myItems.Count(); 
	for(unsigned int i = 0; i < numItems; i++)
		myItems[i].DecayRating(myPreviousLadderUpdateTime); 
}

void 
MMS_EloLadder::PrivUpdateGrace()
{
	MT_MutexLock lock(myMutex);

	unsigned int currentTime = (unsigned int) time(NULL); 
	if(myTimeOfNextUpdate > currentTime)
		return; 

	myPreviousLadderUpdateTime = myTimeOfNextUpdate; 
	myTimeOfNextUpdate += myUpdateInterval; 

	myItemsAreDirty = true; 
	const unsigned int numItems = myItems.Count(); 
	for(unsigned int i = 0; i < numItems; i++)
		myItems[i].DecayRating(myPreviousLadderUpdateTime); 
}

void
MMS_EloLadder::Update()
{
	PrivUpdateGrace(); 
	PrivRemoveInvalid(); 
	PrivUpdateStats(); 
}

void 
MMS_EloLadder::PrivRemoveInvalid()
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

	sql.Format("SELECT id,inTableId FROM _InvalidateLadder WHERE tableName = '%s' ORDER BY id LIMIT 64", myTableName.GetBuffer()); 
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

	sql.Format("DELETE FROM _InvalidateLadder WHERE tableName = '%s' AND id <= %u", myTableName.GetBuffer(), maxId); 
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
		LOG_FATAL("could not delete from ladder"); 
		return; 
	}
}

void 
MMS_EloLadder::PrivUpdateStats()
{
	MMS_Statistics* stats = MMS_Statistics::GetInstance(); 
	stats->SQLQueryServerTracker(myWriteDatabaseConnection->myNumExecutedQueries); 
	myWriteDatabaseConnection->myNumExecutedQueries = 0; 
	stats->SQLQueryServerTracker(myReadDatabaseConnection->myNumExecutedQueries); 
	myReadDatabaseConnection->myNumExecutedQueries = 0; 
}
