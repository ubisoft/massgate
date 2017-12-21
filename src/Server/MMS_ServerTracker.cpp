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
static int nop = 0;
/*
#include "MC_Debug.h"
#include "mc_mem.h"
#include "MN_ReadMessage.h"
#include "mc_commandline.h"

#include "mdb_mysqlconnection.h"
#include "MMS_ProfileCache.h"
#include "MMS_ServerTracker.h"
#include "MMS_ServerTrackerConnectionHandler.h"
#include "mdb_stringconversion.h"
#include "MMS_LadderUpdater.h"
#include "MMS_PlayerStats.h"
#include "MMS_ClanStats.h"
#include "MMS_InitData.h"

#include "MMG_AccessControl.h"
#include "MMG_AuthToken.h"
#include "MMG_ICryptoHashAlgorithm.h"
#include "MMG_Protocols.h"
#include "MMG_ReliableUdpSocketServerFactory.h"
#include "MMG_ServerFilter.h"
#include "MMG_TrackableServer.h"
#include "MMG_ServersAndPorts.h"

#include "MI_Time.h"
#include "MT_ThreadingTools.h"
#include "MDB_MySqlConnection.h"

#include <malloc.h>

MMS_ServerTracker::MMS_ServerTracker()
: MMS_IocpServer(MASSGATE_SERVER_PORT)
{
	myWriteSqlConnection = new MDB_MySqlConnection(MASSGATE_WRITE_DB_HOST, MASSGATE_WRITE_DB_USER, MASSGATE_WRITE_DB_PASSWORD, MMS_InitData::GetDatabaseName(), false);
	if (!myWriteSqlConnection->Connect())
	{
		MC_Debug::DebugMessage(__FUNCTION__ ": Could not connect to database.");
		assert(false);
	}
	myTimeSampler.SetDataBaseConnection(myWriteSqlConnection); 

	myLadderUpdater = new MMS_LadderUpdater(mySettings);
	myLadderUpdater->Start();

	myPlayerStats = new MMS_PlayerStats(mySettings, this); 
	myPlayerStats->Start(); 

	myClanStats = new MMS_ClanStats(mySettings); 
	myClanStats->Start(); 

	MC_Debug::DebugMessage(__FUNCTION__": Servertracker up.");
}

MMS_ServerTracker::~MMS_ServerTracker()
{
	delete myWriteSqlConnection;
	MC_Debug::DebugMessage(__FUNCTION__": Servertracker down.");
	myLadderUpdater->StopAndDelete();
	myPlayerStats->StopAndDelete(); 
	myClanStats->StopAndDelete(); 
}


MMS_IocpWorkerThread*
MMS_ServerTracker::AllocateWorkerThread()
{
	return new MMS_ServerTrackerConnectionHandler(mySettings, this, &myServerLUTs);
}

void
MMS_ServerTracker::Tick()
{
	mySettings.Update();
	myTimeSampler.Update(); 
	UpdateDs();
}

void 
MMS_ServerTracker::AddSample(const char* aMessageName, unsigned int aValue)
{
	myTimeSampler.AddSample(aMessageName, aValue); 
}

const char* 
MMS_ServerTracker::GetServiceName() const
{
	return "MMS_ServerTracker";
}

static MC_GrowingArray<MMG_TrackableServerHeartbeat> ourPendingHeartbeats;
static MT_Mutex ourHeartbeatMutex;

void 
MMS_ServerTracker::AddDsHeartbeat(const MMG_TrackableServerHeartbeat& aHearbeat)
{
	MT_MutexLock locker(ourHeartbeatMutex);
	if (!ourPendingHeartbeats.IsInited())
		ourPendingHeartbeats.Init(1024,1024,false);
	ourPendingHeartbeats.Add(aHearbeat);
}

void
MMS_ServerTracker::UpdateDs()
{
	const char sqlFirstPart[] = "INSERT INTO WICServers(serverId,numPlayers,gameTime,maxPlayers,currentMap,isOnline,startTime) VALUES ";
	const char sqlMiddlePart[] = "(%I64u,%u,%f,%u,%I64u,1,NOW()),%n";
	const char sqlLastPart[] = "ON DUPLICATE KEY UPDATE numPlayers=VALUES(numPlayers),gameTime=VALUES(gameTime),maxPlayers=VALUES(maxPlayers),currentMap=VALUES(currentMap),isOnline=VALUES(isOnline)";

	char sql[8192];
	memcpy(sql, sqlFirstPart, sizeof(sqlFirstPart));
	char* sqlPtr = sql + sizeof(sqlFirstPart)-1;

	ourHeartbeatMutex.Lock();
	if (ourPendingHeartbeats.Count() == 0)
	{
		ourHeartbeatMutex.Unlock();
		return;
	}

	const unsigned int startTime = GetTickCount();
	_set_printf_count_output(1); // enable %n

	int index = 0;
	for (; (sqlPtr-sql < (sizeof(sql)-sizeof(sqlLastPart)-100)) && (index<ourPendingHeartbeats.Count()); index++)
	{
		const MMG_TrackableServerHeartbeat& hb = ourPendingHeartbeats[index];
		int len;
		sprintf(sqlPtr, sqlMiddlePart, hb.myCookie.trackid, hb.myNumPlayers, hb.myGameTime, hb.myMaxNumPlayers, hb.myCurrentMapHash, &len);
		sqlPtr += len;
	}

	assert(index);
	ourPendingHeartbeats.RemoveFirstN(index);
	ourHeartbeatMutex.Unlock();

	memcpy(sqlPtr-1, sqlLastPart, sizeof(sqlLastPart));

	MDB_MySqlQuery updater(*myWriteSqlConnection);
	MDB_MySqlResult updateResult;
	if (!updater.Modify(updateResult, sql))
	{
		MC_DEBUG("Could not update wicservers from heartbeats");
	}
	AddSample(__FUNCTION__, GetTickCount()-startTime);
}

*/