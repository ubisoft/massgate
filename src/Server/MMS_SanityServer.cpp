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
static int i = 0;
/*#include "MMS_SanityServer.h"

#include "MT_ThreadingTools.h"
#include "MC_String.h"
#include "MC_Debug.h"

#include "MMS_InitData.h"

MMS_SanityServer* MMS_SanityServer::ourInstance = NULL; 

void 
MMS_SanityServer::UpdateDatabase(MDB_MySqlConnection* aWritableConnection, 
								 const int aServerPort, 
								 ServerType aServerType, 
								 unsigned int aThreadId)
{
	assert(!aWritableConnection->IsReadOnly());

	MC_StaticString<1024> queryString;
	MDB_MySqlQuery query(*aWritableConnection);
	MDB_MySqlResult res;

	assert(aServerType >= 0 && aServerType < SERVER_TYPES_END); 

	queryString.Format("INSERT INTO _MassgateAlive (ServerIP,ServerPort,type,lastSeen,threadId) "
		"VALUES (\"%s\",\"%d\",\"%s\",Now(),\"%d\") ON DUPLICATE KEY UPDATE lastSeen=Now()", 
		(const char*)myHostName, aServerPort, myServerStrings[aServerType], aThreadId);

	query.Modify(res, queryString);

	// Update alive for Sanityserver itself, if needed
	if (_InterlockedCompareExchange(&myUpdateSanityNowFlag, 0, 1) == 1)
	{
		UpdateDatabase(aWritableConnection, 0, Sanity);
	}
}

const char* 
MMS_SanityServer::GetHostName()
{
	return myHostName; 
}


MMS_SanityServer::MMS_SanityServer()
: myUpdateSanityNowFlag(0)
{
	MC_DEBUGPOS(); 

	myServerStrings[Account] = "Account"; 
	myServerStrings[Tracker] = "Tracker"; 
	myServerStrings[Chat] = "Chat"; 
	myServerStrings[Messaging] = "Messaging"; 
	myServerStrings[Sanity] = "Sanity"; 
	myServerStrings[NATNegotiation] = "NATNeg"; 

	if(gethostname(myHostName.GetBuffer(), myHostName.GetBufferSize() - 1) != 0)
	{
		myHostName[0] = '\0'; 
	}
	myHostName[255] = '\0'; 
}

MMS_SanityServer::~MMS_SanityServer()
{
	MC_DEBUGPOS(); 
}

MMS_SanityServer*
MMS_SanityServer::GetInstance()
{
	if(ourInstance == NULL)
	{
		ourInstance = new MMS_SanityServer(); 
	}
	return ourInstance; 
}


void 
MMS_SanityServer::Update()
{
	time_t currentTime = time(NULL); 
	if((currentTime - myLastUpdateTimeInSeconds) > MMS_SANITY_SERVER_CHECK_INTERVAL_IN_SECONDS)
	{
		myLastUpdateTimeInSeconds = currentTime;
		_InterlockedExchange(&myUpdateSanityNowFlag, 1); // Signal that Sanity should update itself
	}
}

void 
MMS_SanityServer::Run()
{
	MT_ThreadingTools::SetCurrentThreadName("MMS_SanityServer");

	while (!StopRequested())
	{
		Sleep(1000);
		Update();
	}
}
*/