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
#include "MMS_Statistics.h"
#include "MC_Debug.h"
#include "MMS_Constants.h"
#include "MMS_InitData.h"
#include "ML_Logger.h"
#include "MC_CommandLine.h"

MMS_Statistics::MMS_Statistics()
: myFlushtoDBInterval(MY_DB_FLUSH_INTERVAL)
{
	LOG_DEBUGPOS();
}

MMS_Statistics::~MMS_Statistics()
{
	LOG_DEBUGPOS(); 
}

MMS_Statistics* 
MMS_Statistics::GetInstance()
{
	static MMS_Statistics ourInstance;
	return &ourInstance; 
}

void
MMS_Statistics::Init(
	const MMS_Settings& aSettings)
{
	const char* writehost = aSettings.WriteDbHost;
	MC_CommandLine::GetInstance()->GetStringValue("writehost", writehost);
	mySQLWriteConnection = new MDB_MySqlConnection(writehost, aSettings.WriteDbUser, aSettings.WriteDbPassword, MMS_InitData::GetDatabaseName(), false);
	if (!mySQLWriteConnection->Connect())
	{
		LOG_FATAL("Could not connect to database.");
		assert(false);
	}

#define INIT_LOGGABLE(NAME) \
	myLoggables[NAME].theName = #NAME

	INIT_LOGGABLE(numUsersLoggedIn);
	INIT_LOGGABLE(numUsersPlaying);
	INIT_LOGGABLE(numMsgsAccount);
	INIT_LOGGABLE(numMsgsChat);
	INIT_LOGGABLE(numMsgsServerTracker);
	INIT_LOGGABLE(numMsgsMessaging);
	INIT_LOGGABLE(numMsgsNATRelay);
	INIT_LOGGABLE(numSQLsAccount);
	INIT_LOGGABLE(numSQLsChat);
	INIT_LOGGABLE(numSQLsServerTracker);
	INIT_LOGGABLE(numSQLsMessaging);
	INIT_LOGGABLE(numSQLsNATRelay);
	INIT_LOGGABLE(numSockets);
	INIT_LOGGABLE(numSocketContexts);
	INIT_LOGGABLE(numDataChunks);
	INIT_LOGGABLE(dataChunksPoolSize);
	INIT_LOGGABLE(numActiveWorkerthreads);
}

void
MMS_Statistics::Update()
{
	{
		MT_MutexLock lock(myMutex); 
		if(!myFlushtoDBInterval.HasExpired())
			return; 
	}

	char sql[8192]; 
	static const char sqlFirstPart[] = "INSERT INTO ServerStatistics (aKey, aValue) VALUES ";
	static const char sqlMiddlePart[] = "('%s', %d),";

	memcpy(sql, sqlFirstPart, sizeof(sqlFirstPart));
	unsigned int offset = sizeof(sqlFirstPart) - 1; 

	for(int i = 0; i < NUM_LOGGABLE_VALUES; i++)
	{
		if(myLoggables[i].haveBeenLoggedAtAll)
			offset += sprintf(sql + offset, sqlMiddlePart, myLoggables[i].theName, myLoggables[i].theValue); 
	}
	sql[offset - 1] = '\0'; 

	MDB_MySqlQuery query(*mySQLWriteConnection);
	MDB_MySqlResult res;

	if(!query.Modify(res, sql))
	{
		LOG_ERROR("couldn't update MMS_Statistics"); 
		return; 
	}

	PrivReset(); 
}

void 
MMS_Statistics::PrivReset()
{
	myLoggables[numSQLsAccount].theValue = 0; 
	myLoggables[numSQLsChat].theValue = 0; 
	myLoggables[numSQLsServerTracker].theValue = 0; 
	myLoggables[numSQLsMessaging].theValue = 0; 
	myLoggables[numSQLsNATRelay].theValue = 0; 
}

bool
MMS_Statistics::GetLoggableValue(
	LoggableValueIndexes	anIndex,
	long&					aValue)
{
	aValue = myLoggables[anIndex].theValue;
	return true;
}

void 
MMS_Statistics::UserLoggedIn()
{
	myLoggables[numUsersLoggedIn]++; 
}

void 
MMS_Statistics::UserLoggedOut()
{
	myLoggables[numUsersLoggedIn]--; 
}

void 
MMS_Statistics::UserStartedPlaying()
{
	myLoggables[numUsersPlaying]++;		
}

void 
MMS_Statistics::UserStoppedPlaying()
{
	myLoggables[numUsersPlaying]--;		
}

void 
MMS_Statistics::HandleMsgAccount()
{
	myLoggables[numMsgsAccount]++;
}

void 
MMS_Statistics::HandleMsgChat()
{
	myLoggables[numMsgsChat]++;					
}

void 
MMS_Statistics::HandleMsgServerTracker()
{
	myLoggables[numMsgsServerTracker]++;			
}

void 
MMS_Statistics::HandleMsgMessaging()
{
	myLoggables[numMsgsMessaging]++;				
}

void 
MMS_Statistics::HandleMsgNATRelay()
{
	myLoggables[numMsgsNATRelay]++;
}

void 
MMS_Statistics::SQLQueryAccount(int aNumQueries)
{
	myLoggables[numSQLsAccount] += aNumQueries; 
}

void 
MMS_Statistics::SQLQueryChat(int aNumQueries)
{
	myLoggables[numSQLsChat] += aNumQueries; 
}

void 
MMS_Statistics::SQLQueryServerTracker(int aNumQueries)
{
	myLoggables[numSQLsServerTracker] += aNumQueries; 
}

void 
MMS_Statistics::SQLQueryMessaging(int aNumQueries)
{
	myLoggables[numSQLsMessaging] += aNumQueries; 
}

void 
MMS_Statistics::SQLQueryNATRelay(int aNumQueries)
{
	myLoggables[numSQLsNATRelay] += aNumQueries; 
}

void 
MMS_Statistics::OnSocketConnected()
{
	myLoggables[numSockets]++; 
}

void 
MMS_Statistics::OnSocketClosed()
{
	myLoggables[numSockets]--;
}

void 
MMS_Statistics::OnSocketContextCreated()
{
	myLoggables[numSocketContexts]++; 
}

void 
MMS_Statistics::OnSocketContextDestroyed()
{
	myLoggables[numSocketContexts]--; 
}

void 
MMS_Statistics::OnDataChunkCreated()
{
	myLoggables[numDataChunks]++; 
}

void 
MMS_Statistics::OnDataChunkDestroyed()
{
	myLoggables[numDataChunks]--; 
}


void 
MMS_Statistics::SetDataChunkPoolSize(int aPoolSize)
{
	myLoggables[dataChunksPoolSize] = aPoolSize; 
}

void
MMS_Statistics::OnWorkerthreadActivated()
{
	myLoggables[numActiveWorkerthreads] ++;
}

void
MMS_Statistics::OnWorkerthreadDone()
{
	myLoggables[numActiveWorkerthreads] --;
}
