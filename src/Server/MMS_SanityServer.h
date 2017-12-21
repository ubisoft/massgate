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
#ifndef MMS_SANITYSERVER___H_
#define MMS_SANITYSERVER___H_
/*
#include "MT_Thread.h"
#include "mdb_mysqlconnection.h"
#include "MMS_Constants.h"
#include "MC_GrowingArray.h"

// value is milli seconds 
#define MMS_SANITY_SERVER_CHECK_INTERVAL_IN_SECONDS 60

class MMS_SanityServer : public MT_Thread
{
public:
	typedef enum ServerType
	{
		Account, 
		Tracker, 
		Chat, 
		Messaging,
		Sanity,
		NATNegotiation, 
		SERVER_TYPES_END
	}; 

	static MMS_SanityServer* GetInstance(); 

	void Run();

	void UpdateDatabase(MDB_MySqlConnection* aWritableConnection, 
						const int aServerPort, 
						ServerType aServerType, 
						unsigned int aThreadId = 0xFFFFFFFF);

	const char* GetHostName(); 

private:
	static MMS_SanityServer *ourInstance; 

	MMS_SanityServer(); 
	~MMS_SanityServer(); 

	void Update(); 
	
	unsigned int GetLastUpdateByType(ServerType aType); 
	void SetLastUpdateByType(ServerType aType, unsigned int aNewLastUpdate); 
	const char* GetServerTypeString(ServerType aType);  

	MMS_Settings mySettings;

	const char *myServerStrings[SERVER_TYPES_END]; 

	MC_StaticString<256> myHostName; 
	time_t myLastUpdateTimeInSeconds;

	volatile long myUpdateSanityNowFlag;
};
*/
#endif
