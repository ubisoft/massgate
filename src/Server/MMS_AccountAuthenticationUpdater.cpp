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
#include "MMS_AccountAuthenticationUpdater.h"
#include "MC_String.h"
#include "MC_Debug.h"
#include "MT_ThreadingTools.h"
#include "MMS_ServerStats.h"
#include <time.h>
#include "MMS_InitData.h"
#include "MMS_Statistics.h"
#include "ML_Logger.h"
#include "MMS_MasterServer.h"

MMS_AccountAuthenticationUpdater::MMS_AccountAuthenticationUpdater(const MMS_Settings& theSettings)
: mySettings(theSettings)
, myWriteDatabaseConnection(NULL)
{
	myUpdateAuthenticationCache.Init(256,256,false); // 256/sec == ~60k concurrency. Plenty I say!
	myUpdateLogLoginCache.Init(256,256,false);

	if (myWriteDatabaseConnection == NULL)
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
		}
	}
}


MMS_AccountAuthenticationUpdater::~MMS_AccountAuthenticationUpdater()
{
	myWriteDatabaseConnection->Disconnect();
	delete myWriteDatabaseConnection;
}

void 
MMS_AccountAuthenticationUpdater::AddAuthenticationUpdate(unsigned int theId)
{
	MT_MutexLock locker(myMutex);
	myUpdateAuthenticationCache.Add(theId);
}

void 
MMS_AccountAuthenticationUpdater::AddLogLogin(
	const unsigned int	accountId, 
	const unsigned int	productId, 
	const unsigned int	statusCode, 
	const char*			peerIp, 
	const unsigned int	keySequenceNumber, 
	const unsigned int	fingerPrint, 
	const unsigned int	aSessionTime)
{
	LogLoginItem lli;
	lli.accountId = accountId;
	lli.productId = productId;
	lli.statusCode = statusCode;
	memcpy(lli.ip, peerIp, 16);
	lli.keySequence = keySequenceNumber;
	lli.fingerPrint = fingerPrint;
	lli.sessionTime = aSessionTime;

	MT_MutexLock locker(myMutex);
	myUpdateLogLoginCache.Add(lli);
}


void
MMS_AccountAuthenticationUpdater::Run()
{
	MT_ThreadingTools::SetCurrentThreadName("AccountAuthenticationUpdater");

	LOG_INFO("Started.");

	while (!StopRequested())
	{
		Sleep(1000);

		unsigned int startTime = GetTickCount(); 

		myUpdateAllAuthentications();
		myUpdateAllLogLogins();
		
		MMS_Statistics::GetInstance()->SQLQueryAccount(myWriteDatabaseConnection->myNumExecutedQueries);
		myWriteDatabaseConnection->myNumExecutedQueries = 0; 

		GENERIC_LOG_EXECUTION_TIME(MMS_AccountAuthenticationUpdater::Run(), startTime);
	}

	// Oooops we must shutdown! Flush everything to db so we can restart (and users reconnect without loosing lease - MAYBE).
	LOG_INFO("Shutdown requested.");
	myMutex.Lock();
	while (myWriteDatabaseConnection->IsConnected() && (myUpdateAuthenticationCache.Count() || myUpdateLogLoginCache.Count()))
	{
		LOG_INFO("Flushing to database...");
		myUpdateAllAuthentications();
		myUpdateAllLogLogins();
	}
	myMutex.Unlock();
	LOG_INFO("Stopped.");
}

void
MMS_AccountAuthenticationUpdater::myUpdateAllAuthentications()
{
	// Update everything in the cache at the time the function is called. Do not keep a lock during the db-action since it'll stall 
	// everyone else. So, after this function is called the cache may have gotten new items in it. Fine.

	const unsigned int MAX_UPDATES_PER_LOOP=4096;
	static MC_GrowingArray<unsigned int> currentUpdates;
	if (!currentUpdates.IsInited())
		currentUpdates.Init(MAX_UPDATES_PER_LOOP,MAX_UPDATES_PER_LOOP, false);
	else
		currentUpdates.RemoveAll();

	myMutex.Lock();
	// Copy the items we'll write to the database and release the global lock
	const unsigned int nToUpdate = myUpdateAuthenticationCache.Count();
	const unsigned int numUpdates = __min(MAX_UPDATES_PER_LOOP, nToUpdate);

	if (numUpdates == 0)
	{
		myMutex.Unlock();
		return;
	}
	currentUpdates.Add(&myUpdateAuthenticationCache.GetFirst(), numUpdates);
	myUpdateAuthenticationCache.RemoveFirstN(numUpdates);
	myMutex.Unlock();

	currentUpdates.Sort(); // So we lower the risk of database deadlocks.


	_set_printf_count_output(1); // enable %n

	MDB_MySqlTransaction trans(*myWriteDatabaseConnection);

	while(true)
	{
		unsigned int numUpdated = 0;
		char query[4096*4];
		// Build coalesced SQL query
		while (numUpdated < numUpdates)
		{
			char* queryptr = query;
			int len = 0;
			sprintf(query, "UPDATE Authentication SET validUntil=DATE_ADD(NOW(), INTERVAL %u SECOND) WHERE tokenId IN (%n", MMS_Settings::NUM_SECONDS_BETWEEN_SECRETUPDATE+60, &len);
			queryptr += len;
			for (; (queryptr - query < sizeof(query) - 100) && (numUpdated < numUpdates); numUpdated++)
			{
				int len = 0;
				sprintf(queryptr, "%u,%n", currentUpdates[numUpdated], &len);
				queryptr += len;
			}
			strcpy(queryptr-1, ")");

			MDB_MySqlResult res;
			if (!trans.Execute(res, query))
			{
				break;
			}
		}
		if (trans.Commit())
		{
		}
		if (trans.ShouldTryAgain())
		{
			trans.Reset();
		}
		else
		{
			break;
		}
	}
}

void
MMS_AccountAuthenticationUpdater::myUpdateAllLogLogins()
{
	const unsigned int MAX_UPDATES_PER_LOOP = 32;
	static MC_GrowingArray<LogLoginItem> currentUpdates;
	if (!currentUpdates.IsInited())
		currentUpdates.Init(MAX_UPDATES_PER_LOOP, MAX_UPDATES_PER_LOOP, false);
	else
		currentUpdates.RemoveAll();

	myMutex.Lock();
	const unsigned int nToUpdate = myUpdateLogLoginCache.Count();
	const unsigned int numUpdates = __min(MAX_UPDATES_PER_LOOP, nToUpdate);

	if (numUpdates == 0)
	{
		myMutex.Unlock();
		return;
	}
	currentUpdates.Add(&myUpdateLogLoginCache.GetFirst(), numUpdates);
	myUpdateLogLoginCache.RemoveFirstN(numUpdates);
	myMutex.Unlock();

	_set_printf_count_output(1); // enable %n

	MDB_MySqlQuery sqlQuery(*myWriteDatabaseConnection);
	MDB_MySqlResult res;

	unsigned int numUpdated = 0;
	char query[4096*4];
	while (numUpdated < numUpdates)
	{
		char* queryptr = query;
		int len = 0;
		sprintf(query, "INSERT INTO Log_login(logintime, accountid, productid, loginstatus, fromIP, CdKeySequence, fingerprint, sessionTime) VALUES %n", &len);
		queryptr += len;

		for (; (queryptr - query < sizeof(query) -100) && (numUpdated < numUpdates); numUpdated++)
		{
			int len = 0;
			sprintf(queryptr, "(NOW(),%u,%u,%u,'%s',%u,%u,%u),%n"
			, currentUpdates[numUpdated].accountId
			, currentUpdates[numUpdated].productId
			, currentUpdates[numUpdated].statusCode
			, currentUpdates[numUpdated].ip
			, currentUpdates[numUpdated].keySequence
			, currentUpdates[numUpdated].fingerPrint
			, currentUpdates[numUpdated].sessionTime, &len);
			queryptr += len;
		}
		*(queryptr-1) = 0;

		sqlQuery.Modify(res, query);
	}
}

