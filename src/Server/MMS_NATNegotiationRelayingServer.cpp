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
/*#include "MMS_NATNegotiationRelayingServer.h"
#include "MMS_NATNegotiationRelayingServerConnectionHandler.h"
#include "MMG_ServersAndPorts.h"
#include "MC_Debug.h"
#include "MT_ThreadingTools.h"
#include "MMS_InitData.h"
#include "MMS_Statistics.h"

MMS_NATNegotiationRelayingServer::MMS_NATNegotiationRelayingServer()
: MMS_IocpServer(MASSGATE_SERVER_PORT)
{
	MC_DEBUGPOS(); 

	myWriteSqlConnection = new MDB_MySqlConnection(MASSGATE_WRITE_DB_HOST, MASSGATE_WRITE_DB_USER, MASSGATE_WRITE_DB_PASSWORD, MMS_InitData::GetDatabaseName(), false);
	if (!myWriteSqlConnection->Connect())
	{
		MC_Debug::DebugMessage(__FUNCTION__ ": Could not connect to database.");
		assert(false);
	}
	myTimeSampler.SetDataBaseConnection(myWriteSqlConnection); 

	MC_Debug::DebugMessage(__FUNCTION__": NATNegotiation server up.");
}

MMS_NATNegotiationRelayingServer::~MMS_NATNegotiationRelayingServer()
{
	MC_DEBUGPOS(); 
	delete myWriteSqlConnection;
	MC_Debug::DebugMessage(__FUNCTION__": NATNegotiation server down.");
}

MMS_IocpWorkerThread* 
MMS_NATNegotiationRelayingServer::AllocateWorkerThread()
{
	return new MMS_NATNegotiationRelayingServerConnectionHandler(mySettings, this);
}

void 
MMS_NATNegotiationRelayingServer::Tick()
{
	MMS_Statistics::GetInstance()->Update(); 
	mySettings.Update(); 
	myTimeSampler.Update(); 
}

void 
MMS_NATNegotiationRelayingServer::AddSample(const char* aMessageName, unsigned int aValue)
{
	myTimeSampler.AddSample(aMessageName, aValue); 
}

const char* 
MMS_NATNegotiationRelayingServer::GetServiceName() const
{
	return "MMS_NatNegServer";
}

*/
