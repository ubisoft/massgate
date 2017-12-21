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
#ifndef MMS_STATISTICS_H
#define MMS_STATISTICS_H

#include "MC_EggClockTimer.h"
#include "mdb_mysqlconnection.h"
#include "MT_Mutex.h"
#include "MT_ThreadingTools.h"

class MMS_Settings;
class MMS_Statistics
{
public:
	typedef enum 
	{
		numUsersLoggedIn = 0,
		numUsersPlaying,
		numMsgsAccount,		
		numMsgsChat,
		numMsgsServerTracker,
		numMsgsMessaging,
		numMsgsNATRelay,
		numSQLsAccount,
		numSQLsChat,
		numSQLsServerTracker,
		numSQLsMessaging,
		numSQLsNATRelay,
		numSockets,
		numSocketContexts,
		numDataChunks,
		dataChunksPoolSize,
		numActiveWorkerthreads,
		NUM_LOGGABLE_VALUES
	} LoggableValueIndexes; 

	static MMS_Statistics* GetInstance();

	void Init(
			const MMS_Settings& aSettings);

	void Update(); 

	void UserLoggedIn(); 
	void UserLoggedOut(); 
	void UserStartedPlaying();
	void UserStoppedPlaying(); 

	void HandleMsgAccount(); 
	void HandleMsgChat(); 
	void HandleMsgServerTracker(); 
	void HandleMsgMessaging(); 
	void HandleMsgNATRelay(); 

	void SQLQueryAccount(int aNumQueries); 
	void SQLQueryChat(int aNumQueries); 
	void SQLQueryServerTracker(int aNumQueries); 
	void SQLQueryMessaging(int aNumQueries); 
	void SQLQueryNATRelay(int aNumQueries); 

	void OnSocketConnected(); 
	void OnSocketClosed(); 

	void OnSocketContextCreated(); 
	void OnSocketContextDestroyed(); 

	void OnDataChunkCreated();
	void OnDataChunkDestroyed();
	void SetDataChunkPoolSize(int aPoolSize);
	
	void OnWorkerthreadActivated();
	void OnWorkerthreadDone();

	bool MMS_Statistics::GetLoggableValue(
			LoggableValueIndexes	anIndex,
			long&					aValue);

private: 
	MMS_Statistics();
	~MMS_Statistics();

	MC_EggClockTimer myFlushtoDBInterval; 
	static const unsigned int MY_DB_FLUSH_INTERVAL = 5 * 60 * 1000; 
	MDB_MySqlConnection *mySQLWriteConnection; 
	MT_Mutex myMutex; 

	class LoggableValue // boring name 
	{
	public: 
		LoggableValue()
		: theValue(0)
		, theName(NULL)
		, haveBeenLoggedAtAll(false)
		{
		}

		volatile long theValue; 
		const char* theName; 
		volatile bool haveBeenLoggedAtAll; 

		void operator++(int dummy) // postfix 
		{
			_InterlockedIncrement(&theValue); 
			haveBeenLoggedAtAll = true; 
		}
		void operator--(int dummy) // postfix
		{
			_InterlockedDecrement(&theValue); 
			haveBeenLoggedAtAll = true; 
		}
		void operator=(int aValue)
		{
			_InterlockedExchange(&theValue, aValue); 
			haveBeenLoggedAtAll = true; 		
		}
		void operator+=(int aValue)
		{
			_InterlockedExchangeAdd(&theValue, aValue); 
			haveBeenLoggedAtAll = true; 		
		}
	};

	LoggableValue myLoggables[NUM_LOGGABLE_VALUES]; 

	void PrivReset(); 
};

#endif