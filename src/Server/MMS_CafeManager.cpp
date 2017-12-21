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

#include "MMS_CafeManager.h"
#include "MMS_InitData.h"
#include "MT_ThreadingTools.h"
#include "ML_Logger.h"
#include "MMS_MasterServer.h"

MMS_CafeManager*		MMS_CafeManager::ourInstance = NULL;
MT_Mutex				MMS_CafeManager::ourMutex;

MMS_CafeManager::MMS_CafeManager()
	: myInvalidateTimer(10000)
{
}

MMS_CafeManager::~MMS_CafeManager()
{

}

void
MMS_CafeManager::ResetConcurrency()
{
	MC_StaticString<255>	sql;
	MDB_MySqlQuery			query(*myWriteSqlConnection);
	MDB_MySqlResult			result;

	sql.Format("UPDATE Cafe_Info SET currentConcur=0");
	if(!query.Modify(result, sql.GetBuffer()))
	{
		LOG_FATAL("Failed to reset concurrency");
		assert(false && "Database failed");
	}	
}

MMS_CafeManager*
MMS_CafeManager::GetInstance()
{
	MT_MutexLock	lock(ourMutex);

	if(ourInstance == NULL)
		ourInstance = new MMS_CafeManager();

	return ourInstance;
}

void
MMS_CafeManager::Init(
	const MMS_Settings&	aSettings)
{
	myWriteSqlConnection = new MDB_MySqlConnection(
		aSettings.WriteDbHost,
		aSettings.WriteDbUser,
		aSettings.WriteDbPassword,
		MMS_InitData::GetDatabaseName(),
		false);
	if (!myWriteSqlConnection->Connect())
	{
		LOG_FATAL("Could not connect to database.");
		assert(false);
		exit(0);
	}

	myReadSqlConnection = new MDB_MySqlConnection(
		aSettings.ReadDbHost,
		aSettings.ReadDbUser,
		aSettings.ReadDbPassword,
		MMS_InitData::GetDatabaseName(),
		true);
	if (!myReadSqlConnection->Connect())
	{
		LOG_FATAL("Could not connect to database.");
		assert(false);
		exit(0);
	}
}

bool
MMS_CafeManager::ValidateKey(
	const MMS_MultiKeyManager::CdKey&	aCdKey,
	const char*							anIp)
{
	MT_MutexLock	lock(ourMutex);

	CafeInfo*		info = PrivFindOrLoadByKey(aCdKey);
	if(!info)
		return false;

	MC_StaticString<255>	sql;
	MDB_MySqlQuery			query(*myWriteSqlConnection);
	MDB_MySqlResult			result;

	sql.Format("SELECT COUNT(*) AS num FROM Cafe_WhiteList WHERE cafeId=%u AND ip='%s'",
		info->id, anIp);
	if(!query.Ask(result, sql.GetBuffer()))
		assert(false && "Database failed");

	MDB_MySqlRow			row;
	result.GetNextRow(row);

	if((int) row["num"] == 0)
		return false;

	if(info->curConcur + 1 > info->maxConcur)
		return false;

	info->curConcur ++;

	sql.Format("UPDATE Cafe_Info SET currentConcur=currentConcur+1 WHERE id=%u", 
		info->id);

	if(!query.Modify(result, sql.GetBuffer()))
		assert(false && "Failed to update database");

	return true;
}

void
MMS_CafeManager::Logout(
	unsigned int		aKeySeqNum,
	unsigned int		aSessionId,
	time_t				aSessionStart)
{
	if(aSessionId == 0)
		return;

	MDB_MySqlQuery		query(*myWriteSqlConnection);
	MDB_MySqlResult		result;

	MC_StaticString<255>	sql;
	sql.Format("UPDATE Cafe_Log SET duration=%u WHERE id=%u",
		(unsigned int) (time(NULL) - aSessionStart),
		aSessionId);

	if(!query.Modify(result, sql.GetBuffer()))
		assert(false && "Failed to update database");

	MT_MutexLock		lock(ourMutex);

	CafeInfo*		info;
	if(myCafeMap.GetIfExists(aKeySeqNum, info))
	{
		info->curConcur --;

		sql.Format("UPDATE Cafe_Info SET currentConcur=currentConcur-1 WHERE id=%u",
			info->id);

		lock.Unlock();
		if(!query.Modify(result, sql.GetBuffer()))
			assert(false && "Update to database failed");
	}
}

unsigned int
MMS_CafeManager::Login(
	unsigned int aKeySeqNum)
{
	MC_StaticString<255>	sql;
	sql.Format("INSERT INTO Cafe_Log (cafeId, action, entered, duration) VALUES ((SELECT id FROM Cafe_Info WHERE cdKey=%u), 'session', CURRENT_TIMESTAMP, -1)",
		aKeySeqNum);
	MDB_MySqlQuery		query(*myWriteSqlConnection);
	MDB_MySqlResult		result;
	if(!query.Modify(result, sql.GetBuffer()))
		assert(false && "Failed to update datebase");

	return (unsigned int) query.GetLastInsertId();
}

void
MMS_CafeManager::DecreaseConcurrency(
	unsigned int		aKeySeqNum)
{
	MDB_MySqlQuery		query(*myWriteSqlConnection);
	MDB_MySqlResult		result;

	MT_MutexLock		lock(ourMutex);
	CafeInfo*		info;
	if(myCafeMap.GetIfExists(aKeySeqNum, info))
	{
		info->curConcur --;

		MC_StaticString<255>	sql;
		sql.Format("UPDATE Cafe_Info SET currentConcur=currentConcur-1 WHERE id=%u",
			info->id);
		
		lock.Unlock();
		if(!query.Modify(result, sql.GetBuffer()))
			assert(false && "Update to database failed");
	}
}

void
MMS_CafeManager::Run()
{
	while(!StopRequested())
	{
		MC_Sleep(5000);

		unsigned int startTime = GetTickCount(); 

		if(myInvalidateTimer.HasExpired())
		{
			MC_StaticString<255>	sql;
			sql.Format("SELECT i.id, c.cdkey, c.maxConcur FROM _invalidatecafe i INNER JOIN Cafe_Info c ON c.id=i.cafeId");

			MDB_MySqlQuery			query(*myWriteSqlConnection);
			MDB_MySqlResult			result;
			if(!query.Ask(result, sql.GetBuffer()))
				assert(false && "Failed to query database");
			
			if(result.GetAffectedNumberOrRows())
			{
				MT_MutexLock			lock(ourMutex);
				MDB_MySqlRow			row;
				unsigned int			maxId = 0;
				while(result.GetNextRow(row))
				{
					unsigned int		id = row["id"];
					if(id > maxId)
						maxId = id;
					
					CafeInfo*			info;
					unsigned int		cdkey = row["cdkey"];

					if(myCafeMap.GetIfExists(cdkey, info))
						info->maxConcur = row["maxConcur"];

				}
				
				lock.Unlock();

				sql.Format("DELETE FROM _invalidatecafe WHERE id<=%u", maxId);
				if(!query.Modify(result, sql.GetBuffer()))
					assert(false && "Failed to used items from _invalidatecafe");
			}
		}

		GENERIC_LOG_EXECUTION_TIME(MMS_CafeManager::Run(), startTime);
	}
}

/**
 * Returns the CafeInfo struct for the key, if the key is not in cache it is
 * loaded from db. Returns NULL if the key cannot be loaded.
 */
MMS_CafeManager::CafeInfo*
MMS_CafeManager::PrivFindOrLoadByKey(
	const MMS_MultiKeyManager::CdKey&	aCdKey)
{
	CafeInfo*		info = NULL;

	if(!myCafeMap.GetIfExists(aCdKey.mySequenceNumber, info))
	{
		MC_StaticString<255>	sql;
		sql.Format("SELECT * FROM Cafe_Info c WHERE cdkey=%u", aCdKey.mySequenceNumber);

		MDB_MySqlQuery			query(*myReadSqlConnection);	
		MDB_MySqlResult			result;
		if(!query.Ask(result, sql.GetBuffer()))
			assert(false && "Failed to query database");

		MDB_MySqlRow			row;
		if(!result.GetNextRow(row))
			return NULL;
		
		info = new CafeInfo;

		info->id = row["id"];
		info->curConcur = 0;
		info->maxConcur = row["maxConcur"];
		info->keySeqNum = aCdKey.mySequenceNumber;

		myCafeMap[aCdKey.mySequenceNumber] = info;
	}

	return info;
}