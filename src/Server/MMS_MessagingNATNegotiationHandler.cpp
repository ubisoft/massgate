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
#include "MMS_MessagingNATNegotiationHandler.h"
#include "MMG_Messaging.h"
#include "MMG_INATNegotiationMessagingListener.h"
#include "MMS_PersistenceCache.h"
#include "MMS_MessagingConnectionHandler.h"
#include "MT_ThreadingTools.h"
#include "MMG_ProtocolDelimiters.h"
#include "ML_Logger.h"

volatile long MMS_MessagingNATNegotiationHandler::ourSessionCookie = 0; 

MMS_MessagingNATNegotiationHandler::MMS_MessagingNATNegotiationHandler()
{
	myWriteSqlConnection = NULL;
	myReadSqlConnection = NULL;
	myMessagingConnectionHandler = NULL; 

	_InterlockedExchangeAdd(&ourSessionCookie, 1 + rand()); 
}

MMS_MessagingNATNegotiationHandler::~MMS_MessagingNATNegotiationHandler()
{
	LOG_DEBUGPOS(); 
}

bool
MMS_MessagingNATNegotiationHandler::HandleMessage(const MN_DelimiterType aDelimeter, 
												  MN_ReadMessage& theIncomingMessage, 
												  MN_WriteMessage& theOutgoingMessage, 
												  MMS_IocpServer::SocketContext* thePeer, 
												  const MMG_AuthToken& anAuthToken)
{
	bool good = true; 

	switch(aDelimeter)
	{
	case MMG_ProtocolDelimiters::MESSAGING_NATNEG_CLIENT_TO_MEDIATING_REQUEST_CONNECT:
		good = good && PrivHandleClientConnectRequest(theOutgoingMessage, theIncomingMessage, anAuthToken, thePeer); 
		break; 
	case MMG_ProtocolDelimiters::MESSAGING_NATNEG_CLIENT_TO_MEDIATING_RESPONSE_CONNECT: 
		good = good && PrivHandleClientConnectResponse(theOutgoingMessage, theIncomingMessage, anAuthToken, thePeer);
		break; 
	default: 
		LOG_ERROR("Delimiter: %d, should not be handled by MMS_NATNegHandler", aDelimeter);
		assert(false); 
		good = false; 
		break; 
	}

	return good; 
}

void 
MMS_MessagingNATNegotiationHandler::UpdateDatabaseConnections(MDB_MySqlConnection* aReadConnection, 
															  MDB_MySqlConnection* aWriteConnection, 
															  MMS_MessagingConnectionHandler* aMessagingConnectionHandler)
{
	myReadSqlConnection = aReadConnection; 
	myWriteSqlConnection = aWriteConnection; 
	myMessagingConnectionHandler = aMessagingConnectionHandler; 
}

bool 
MMS_MessagingNATNegotiationHandler::PrivHandleClientConnectRequest(MN_WriteMessage& theWm, 
																   MN_ReadMessage& theRm, 
																   const MMG_AuthToken& theToken, 
																   MMS_IocpServer::SocketContext* const thePeer)
{
	bool good = true; 
	MMG_NATNegotiationProtocol::ClientToMediatingRequestConnect clientConnectionRequest; 

	good = good && clientConnectionRequest.FromStream(theRm); 
	good = good && clientConnectionRequest.IsSane(); 
	if(good)
	{
		ClientLutRef passivePeer = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, clientConnectionRequest.myPassiveProfileId);
		good = passivePeer.IsGood(); 
		if(good)
		{
			good = passivePeer->myState != 0; 
			if(good)
			{
				// pack message to passive peer, to inform him that the active peer wants to talk 
				MMG_NATNegotiationProtocol::MediatingToClientRequestConnect serverConnectionRequest; 

				serverConnectionRequest.myRequestId = clientConnectionRequest.myRequestId; 
				serverConnectionRequest.myActiveProfileId = theToken.profileId; 
				serverConnectionRequest.mySessionCookie = (unsigned int) _InterlockedExchangeAdd(&ourSessionCookie, 1 + rand()); 
				serverConnectionRequest.myActivePeersPrivateEndpoint.Init(clientConnectionRequest.myPrivateEndpoint); 
				serverConnectionRequest.myActivePeersPublicEndpoint.Init(clientConnectionRequest.myPublicEndpoint); 
				serverConnectionRequest.myUseRelayingServer = clientConnectionRequest.myUseRelayingServer; 
				serverConnectionRequest.myReservedDummy = clientConnectionRequest.myReservedDummy; 

				MN_WriteMessage message; 

				serverConnectionRequest.ToStream(message); 

				good = myMessagingConnectionHandler->myWorkerThread->SendMessageTo(message, passivePeer->theSocket);

				LOG_INFO("sending nat neg connection request from: %d to %d", theToken.profileId, clientConnectionRequest.myPassiveProfileId); 
			}
			else 
			{
				MMG_NATNegotiationProtocol::MediatingToClientResponseConnect serverConnectionResponse; 

				serverConnectionResponse.myRequestId = clientConnectionRequest.myRequestId; 
				serverConnectionResponse.myPassiveProfileId = clientConnectionRequest.myPassiveProfileId; 
				serverConnectionResponse.myAcceptRequest = 0; 
				serverConnectionResponse.myReservedDummy = clientConnectionRequest.myReservedDummy; 

				serverConnectionResponse.ToStream(theWm); 

				LOG_ERROR("Passive peer is not on-line");
			}
		}
		else 
		{
			MMG_NATNegotiationProtocol::MediatingToClientResponseConnect serverConnectionResponse; 

			serverConnectionResponse.myRequestId = clientConnectionRequest.myRequestId; 
			serverConnectionResponse.myPassiveProfileId = clientConnectionRequest.myPassiveProfileId; 
			serverConnectionResponse.myAcceptRequest = 0; 
			serverConnectionResponse.myReservedDummy = clientConnectionRequest.myReservedDummy; 

			serverConnectionResponse.ToStream(theWm); 

			LOG_ERROR("Broken client LUT"); 		
		}
	}
	else 
	{
		LOG_ERROR("Protocol error"); 
	}

	return good; 
}

bool 
MMS_MessagingNATNegotiationHandler::PrivHandleClientConnectResponse(MN_WriteMessage& theWm, 
																    MN_ReadMessage& theRm, 
																    const MMG_AuthToken& theToken, 
																    MMS_IocpServer::SocketContext* const thePeer)
{
	bool good = true; 
	MMG_NATNegotiationProtocol::ClientToMediatingResponseConnect clientConnectionResponse; 

	good = good && clientConnectionResponse.FromStream(theRm); 
	good = good && clientConnectionResponse.IsSane(); 
	if(good)
	{
		ClientLutRef activePeer = MMS_PersistenceCache::GetClientLut(myReadSqlConnection, clientConnectionResponse.myActiveProfileId);
		
		good = activePeer.IsGood(); 
		if(good)
		{
			good = activePeer->myState != 0;
			if(good)
			{
				MMG_NATNegotiationProtocol::MediatingToClientResponseConnect serverConnectionResponse; 

				serverConnectionResponse.myRequestId = clientConnectionResponse.myRequestId; 
				serverConnectionResponse.myPassiveProfileId = theToken.profileId; 
				serverConnectionResponse.myAcceptRequest = clientConnectionResponse.myAcceptRequest; 
				if(clientConnectionResponse.myAcceptRequest == 1)
				{
					serverConnectionResponse.mySessionCookie = clientConnectionResponse.mySessionCookie; 
					serverConnectionResponse.myPassivePeersPrivateEndpoint.Init(clientConnectionResponse.myPrivateEndpoint); 
					serverConnectionResponse.myPassivePeersPublicEndpoint.Init(clientConnectionResponse.myPublicEndpoint); 
					serverConnectionResponse.myUseRelayingServer = clientConnectionResponse.myUseRelayingServer; 
				}
				else 
				{
					good = good; 
				}
				serverConnectionResponse.myReservedDummy = clientConnectionResponse.myReservedDummy; 

				MN_WriteMessage message; 

				serverConnectionResponse.ToStream(message); 

				good = myMessagingConnectionHandler->myWorkerThread->SendMessageTo(message, activePeer->theSocket); 		

				LOG_INFO("sending nat neg connection response from: %d to %d", theToken.profileId, clientConnectionResponse.myActiveProfileId); 
			}
			else		
			{
				LOG_ERROR("Active peer is not on-line"); 
			}
		}
		else 
		{
			LOG_ERROR("Broken client LUT"); 
		}
	}
	else 
	{
		LOG_ERROR("Protocol error"); 
	}

	return good; 
}

