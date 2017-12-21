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

#include "MC_Base.h"
#if IS_PC_BUILD

#include "mdb_stringconversion.h"

#include "MSB_Types.h"
#include "MSB_RawUDPSocket.h"
#include "MSB_ReadMessage.h"
#include "MSB_Stats.h"
#include "MSB_StatsContext.h"
#include "MSB_WriteMessage.h"
//#include "MC_MemoryTracking.h"

class MSB_StatsStopper
{
public:
					MSB_StatsStopper() : myStats(NULL){}
					~MSB_StatsStopper()
					{
						//Do not delete myStats. Some might still be using it (MSB_Iocp for example)
						if ( myStats != NULL )
							myStats->Stop();
					}

	MSB_Stats*		myStats;
};

MSB_StatsStopper	ourStatsStopper;

MSB_Stats::MSB_Stats()
	: myExportSocket(NULL)
	, myStatHandler(NULL)
	, myPeriodicalThread(NULL)
{
	ourStatsStopper.myStats = this;
}

MSB_Stats::~MSB_Stats()
{
	assert(false); //Should never run
	//Stop();
}

int32
MSB_Stats::Start(
	uint16				aPort,
	uint16				aNumberOfAttempts,
	int16				aPortDelta)
{
	if(myExportSocket != NULL)
		return 0;

	SOCKET				sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == INVALID_SOCKET)
		return WSAGetLastError();
	
	struct sockaddr_in	address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	bool				success = false;
	do {
		address.sin_port = htons(aPort);
		
		if(bind(sock, (struct sockaddr*) &address, sizeof(address)) == 0)
		{
			success = true;
			break;
		}
		
		aPort += aPortDelta;
		aNumberOfAttempts --;

	}while(aNumberOfAttempts > 0);

	if(!success)
		return WSAGetLastError();

	myExportSocket = new MSB_RawUDPSocket(sock);
	myStatHandler = new ExportHandler(*this, *myExportSocket);
	myExportSocket->SetMessageHandler(myStatHandler);
	myExportSocket->Retain();

	myExportSocket->Register();

	PrivRecreateStats();

	myPeriodicalThread = new Periodical(*this);
	myPeriodicalThread->Start();

	return 0;
}
void
MSB_Stats::Stop()
{
	if(myPeriodicalThread)
		myPeriodicalThread->StopAndDelete();

	if(myExportSocket)
	{
		myExportSocket->Close();
		myExportSocket->Release();
	}
}

MSB_StatsContext&
MSB_Stats::CreateContext(
	const char*			aName)
{
	MT_MutexLock		lock(myLock);

	assert(myContextTable.HasKey(aName) == false);

	MSB_StatsContext*	context = new MSB_StatsContext();
	myContextTable.Add(aName, context);
	return *context;
}

bool
MSB_Stats::HasContext(
	const char*			aName)
{
	return myContextTable.HasKey(aName);
}

MSB_StatsContext&
MSB_Stats::FindContext(
	const char*			aName)
{
	return *myContextTable[aName];
}

void
MSB_Stats::GetNames(
	MSB_IArray<MC_String>& someNames)
{
	MSB_HashTable<const char*, MSB_StatsContext*>::Iterator iter(myContextTable);

	while(iter.Next())
		someNames.Add(MC_String((const char*) iter.GetKey()));
}

MSB_Stats&
MSB_Stats::GetInstance()
{
//	MC_MEMORYTRACKING_ALLOW_LEAKS_SCOPE();

	static MSB_Stats*			stats = new MSB_Stats();
	return *stats;
}

void
MSB_Stats::PrivGetStatsResponse(
	const char*				aContextName,
	MSB_WriteMessage&		aWriteMessage)
{
	MT_MutexLock			lock(myLock);

	MSB_WriteMessage*		cache;
	if(myContextCache.Get(aContextName, cache))
		cache->Clone(aWriteMessage);
}

void
MSB_Stats::PrivGetMetaDataResponse(
	const char*				aContextName,
	MSB_WriteMessage&		aWriteMessage)
{
	MT_MutexLock			lock(myLock);
	MSB_WriteMessage*		cache;
	MSB_StatsContext*		context;

	if(myMetaCache.Get(aContextName, cache))
	{
		cache->Clone(aWriteMessage);
	}
	else if(myContextTable.Get(aContextName, context))
	{
		cache = new MSB_WriteMessage();
		cache->WriteDelimiter(GET_METADATA_RSP);
		cache->WriteString(aContextName);

		context->MarshalMetaData(*cache);
		myMetaCache.Add(aContextName, cache);
		
		cache->Clone(aWriteMessage);
	}
}

/**
 * Updates the write message we clone for all our stat responses
 *
 * Note that while this is running no stats can be updated and therefore
 * any marshaling should keep their objects simple.
 * It's much better to do any heavy lifting IValue::Set(), ofc heavy lifting
 * should be kept to a minimum to avoid back problems.
 */
void
MSB_Stats::PrivRecreateStats()
{
	MT_MutexLock			lock(myLock);

	ContextTable::Iterator	i(myContextTable);
	while(i.Next())
	{
		PrivRebuildContext(i.GetKey(), i.GetItem());
	}
}

/**
 * 
 */
void
MSB_Stats::PrivRebuildContext(
	const char*			aKey,
	MSB_StatsContext*	aContext)
{
	MSB_WriteMessage*	msg;

	if(myContextCache.Get(aKey, msg) == false)
	{
		msg = new MSB_WriteMessage();
		myContextCache.Add(aKey, msg);
	}

	msg->Clear();
	msg->WriteDelimiter(GET_STATS_RSP);
	msg->WriteUInt16(1);		// Version
	msg->WriteString(aKey);

	aContext->Marshal(*msg);
}

//
// StatsHandler
// 

MSB_Stats::ExportHandler::ExportHandler(
	MSB_Stats&			aStatsInstance,
	MSB_Socket_Win&			aSocket)
	: MSB_MessageHandler(aSocket)
	, myStats(aStatsInstance)
{
	
}

bool
MSB_Stats::ExportHandler::OnInit(
	MSB_WriteMessage&	aWriteMessage)
{
	return 0;
}

bool
MSB_Stats::ExportHandler::OnMessage(
	MSB_ReadMessage&	aReadMessage,
	MSB_WriteMessage&	aWriteMessage)
{
	bool			good = false;

	switch(aReadMessage.GetDelimiter())
	{
		case GET_STATS_REQ:
			good = PrivHandleGetStatsReq(aReadMessage, aWriteMessage);
			break;

		case GET_METADATA_REQ:
			good = PrivHandleGetMetaDataReq(aReadMessage, aWriteMessage);
			break;
	}

	return good;
}

void
MSB_Stats::ExportHandler::OnClose()
{
	
}

void
MSB_Stats::ExportHandler::OnError(
	MSB_Error			anError)
{
	
}

/**
* Handles an incoming request for all our stats.
*
*/
bool
MSB_Stats::ExportHandler::PrivHandleGetStatsReq(
	MSB_ReadMessage&	aReadMessage,
	MSB_WriteMessage&	aWriteMessage)
{
	MC_StaticString<60>	name;
	if(aReadMessage.ReadString(name.GetBuffer(), name.GetBufferSize()))
		myStats.PrivGetStatsResponse(name, aWriteMessage);

	return true;
}

bool
MSB_Stats::ExportHandler::PrivHandleGetMetaDataReq(
	MSB_ReadMessage&	aReadMessage,
	MSB_WriteMessage&	aWriteMessage)
{
	MC_StaticString<60>	name;
	if(aReadMessage.ReadString(name.GetBuffer(), name.GetBufferSize()))
		myStats.PrivGetMetaDataResponse(name, aWriteMessage);

	return true;
}

//
// Periodical
//

MSB_Stats::Periodical::Periodical(
	MSB_Stats&			aStats)
	: MT_Thread()
	, myStats(aStats)
	, myRecreateTimer(5 * 1000)
{

}

void
MSB_Stats::Periodical::Run()
{
	while(!StopRequested())
	{
		if(myRecreateTimer.HasExpired())
			myStats.PrivRecreateStats();

		Sleep(100);
	}
}

#endif // IS_PC_BUILD
