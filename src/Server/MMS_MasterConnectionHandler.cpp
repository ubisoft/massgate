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
#include "MMS_MasterConnectionHandler.h"
#include "MMS_Statistics.h"
#include "MC_Debug.h"
#include "MMS_MasterServer.h"
#include "MMS_PlayerStats.h"
#include "ML_Logger.h"


MMS_MasterConnectionHandler::MMS_MasterConnectionHandler(MMS_Settings& someSettings, MMS_MasterServer* aServer)
: MMS_IocpWorkerThread((MMS_IocpServer*)aServer)
, mySettings(someSettings)
, myAccountHandler(someSettings)
, myMessagingHandler(someSettings)
, myServertrackerHandler(someSettings)
, myChatHandler(someSettings)
, myNatNegHandler(someSettings)
{
	myAccountHandler.myWorkerThread = this;
	myMessagingHandler.myWorkerThread = this;
	myServertrackerHandler.myWorkerThread = this;
	myChatHandler.myWorkerThread = this;
	myNatNegHandler.myWorkerThread = this;
}

bool 
MMS_MasterConnectionHandler::HandleMessage(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer)
{
	bool good = true;
	MMS_PerConnectionData* pcd = thePeer->GetUserdata();
	const int threadNum = GetThreadNum();

	while (!theIncomingMessage.AtEnd() && good)
	{
		unsigned int startTime = GetTickCount(); 

		Heartbeat();

		if (pcd->shouldBeDisconnectedFromAccount || thePeer->myIsClosingFlag)
			return false;
		pcd->timeOfLastMessage = GetTickCount();
		pcd->numMessages++;
		if (pcd->isPlayer && pcd->timeOfLastLeaseUpdate + MMS_Settings::NUM_SECONDS_BETWEEN_SECRETUPDATE*1000 < GetTickCount())
		{
			// Time to make sure that the authtoken is valid (client should request new one real soon)
			if (!mySettings.HasValidSecret(pcd->authToken))
			{
				pcd->isAuthorized = false;
				LOG_ERROR("Client from %s has invalid token after %u messages and %u seconds. Disconnecting.", thePeer->myPeerIpNumber, pcd->numMessages, (GetTickCount()-pcd->timeOfLogin)/1000);
				return false;
			}
		}

		MN_DelimiterType delimRaw = 0;
		MMG_ProtocolDelimiters::Delimiter delim = MMG_ProtocolDelimiters::ACCOUNT_START;
		if (!theIncomingMessage.ReadDelimiter(delimRaw))
		{
			LOG_ERROR("Could not read delimiter from client %s. Previously read %u messages. Disconnecting.", thePeer->myPeerIpNumber, thePeer->myNumMessages);
			return false;
		}
		delim = MMG_ProtocolDelimiters::Delimiter(delimRaw);

		myServer->WorkerthreadDelimiter(threadNum, delim);
		
		MMS_IocpServer::CPUClock cpuClock;
		cpuClock.Start();
		
		if(delim > MMG_ProtocolDelimiters::ACCOUNT_START && delim < MMG_ProtocolDelimiters::ACCOUNT_END)
		{
			good = good && myAccountHandler.HandleMessage(delim, theIncomingMessage, theOutgoingMessage, thePeer);
		}
		else if(delim > MMG_ProtocolDelimiters::MESSAGING_START && delim < MMG_ProtocolDelimiters::MESSAGING_END)
		{
			good = good && pcd->isPlayer && pcd->isAuthorized;
			good = good && myMessagingHandler.HandleMessage(delim, theIncomingMessage, theOutgoingMessage, thePeer);
		}
		else if(delim > MMG_ProtocolDelimiters::MESSAGING_DS_START && delim < MMG_ProtocolDelimiters::MESSAGING_DS_END)
		{
			good = good && myMessagingHandler.HandleMessage(delim, theIncomingMessage, theOutgoingMessage, thePeer);
		}
		else if(delim >	MMG_ProtocolDelimiters::SERVERTRACKER_USER_START && delim < MMG_ProtocolDelimiters::SERVERTRACKER_USER_END)
		{
			good = good && pcd->isPlayer && pcd->isAuthorized;
			good = good && myServertrackerHandler.HandleMessage(delim, theIncomingMessage, theOutgoingMessage, thePeer);
		}
		else if(delim > MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_START && delim < MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_END)
		{
			good = good && myServertrackerHandler.HandleMessage(delim, theIncomingMessage, theOutgoingMessage, thePeer);
		}
		else if(delim > MMG_ProtocolDelimiters::CHAT_START && delim < MMG_ProtocolDelimiters::CHAT_END)
		{
			good = good && pcd->isPlayer && pcd->isAuthorized;
			good = good && myChatHandler.HandleMessage(delim, theIncomingMessage, theOutgoingMessage, thePeer);
		}
		else if(delim > MMG_ProtocolDelimiters::NATNEG_START && delim < MMG_ProtocolDelimiters::NATNEG_END)
		{
			good = good && myNatNegHandler.HandleMessage(delim, theIncomingMessage, theOutgoingMessage, thePeer);
		}
		else 
		{
			myServer->WorkerthreadDelimiter(threadNum, 0);

			LOG_ERROR("Abuse! Delimiter out of range, %u, from %s after %u messages. Disconnecting.", delim, thePeer->myPeerIpNumber, thePeer->myNumMessages);
			return false; 
		}

		GENERIC_LOG_EXECUTION_TIME(MMS_MasterConnectionHandler::HandleMessage(), startTime); 
		int cpuTime; 
		if(cpuClock.Stop(cpuTime))
		{
			char name[256];
			sprintf(name, "CPU %d", delim);
			if(cpuTime < 0)
				cpuTime = 0;
			MMS_MasterServer::GetInstance()->AddSample(name, cpuTime);
		}
	}
	MMS_PlayerStats::GetInstance()->Update(this);

	myServer->WorkerthreadDelimiter(threadNum, 0);
	return good;
}

MMS_MasterConnectionHandler::~MMS_MasterConnectionHandler()
{
}

DWORD 
MMS_MasterConnectionHandler::GetTimeoutTime()
{
	unsigned int startTime = GetTickCount(); 

	DWORD timeout = myAccountHandler.GetTimeoutTime();
	timeout = MC_Min(timeout, myMessagingHandler.GetTimeoutTime());
	timeout = MC_Min(timeout, myChatHandler.GetTimeoutTime());
	timeout = MC_Min(timeout, myServertrackerHandler.GetTimeoutTime());
	timeout = MC_Min(timeout, myNatNegHandler.GetTimeoutTime());

	GENERIC_LOG_EXECUTION_TIME(MMS_MasterConnectionHandler::GetTimeoutTime(), startTime); 

	return timeout;
}

void 
MMS_MasterConnectionHandler::OnIdleTimeout()
{
	unsigned int startTime = GetTickCount(); 

	myAccountHandler.OnIdleTimeout();
	myMessagingHandler.OnIdleTimeout();
	myChatHandler.OnIdleTimeout();
	myServertrackerHandler.OnIdleTimeout();
	myNatNegHandler.OnIdleTimeout();

	GENERIC_LOG_EXECUTION_TIME(MMS_MasterConnectionHandler::OnIdleTimeout(), startTime); 
}

void 
MMS_MasterConnectionHandler::OnSocketConnected(MMS_IocpServer::SocketContext* aContext)
{
	unsigned int startTime = GetTickCount(); 

	MMS_Statistics::GetInstance()->OnSocketConnected(); 

	aContext->SetUserData2(new MMS_PerConnectionData);

	myAccountHandler.OnSocketConnected(aContext);
	myMessagingHandler.OnSocketConnected(aContext);
	myServertrackerHandler.OnSocketConnected(aContext);
	myChatHandler.OnSocketConnected(aContext);

	GENERIC_LOG_EXECUTION_TIME(MMS_MasterConnectionHandler::OnSocketConnected(), startTime); 
}

void 
MMS_MasterConnectionHandler::OnSocketClosed(MMS_IocpServer::SocketContext* aContext)
{
	unsigned int startTime = GetTickCount(); 

	if (aContext->GetUserdata() == NULL)
	{
		LOG_ERROR("ERROR! CONNECTION ALREADY CLOSED. IGNORING.");
		return;
	}
	myChatHandler.OnSocketClosed(aContext);
	myServertrackerHandler.OnSocketClosed(aContext);
	myMessagingHandler.OnSocketClosed(aContext);
	myAccountHandler.OnSocketClosed(aContext);

	MMS_Statistics::GetInstance()->OnSocketClosed(); 
	delete aContext->GetUserdata();
	aContext->SetUserData2(NULL);

	GENERIC_LOG_EXECUTION_TIME(MMS_MasterConnectionHandler::OnSocketClosed(), startTime); 
}

void 
MMS_MasterConnectionHandler::OnReadyToStart()
{
	unsigned int startTime = GetTickCount(); 

	myChatHandler.OnReadyToStart();
	myMessagingHandler.OnReadyToStart();
	myServertrackerHandler.OnReadyToStart();
	myAccountHandler.OnReadyToStart();

	GENERIC_LOG_EXECUTION_TIME(MMS_MasterConnectionHandler::OnReadyToStart(), startTime); 
}


