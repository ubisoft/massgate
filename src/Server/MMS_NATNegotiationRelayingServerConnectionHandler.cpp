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
#include "MMS_NATNegotiationRelayingServerConnectionHandler.h"
#include "MMS_MasterServer.h"
#include "MMG_ServersAndPorts.h"
#include "MMG_INATNegotiationMessagingListener.h"
#include "MMG_NATNegotiationProtocol.h"
#include "MT_ThreadingTools.h"
#include "MMS_InitData.h"
#include "MMS_Statistics.h"
#include "MMG_ProtocolDelimiters.h"
#include "ML_Logger.h"

volatile long MMS_NATNegotiationRelayingServerConnectionHandler::ourNextThreadNum = -1; 
volatile long MMS_NATNegotiationRelayingServerConnectionHandler::ourNumConnected = 0; 
volatile long MMS_NATNegotiationRelayingServerConnectionHandler::ourMessagesSent = 0; 
volatile long MMS_NATNegotiationRelayingServerConnectionHandler::ourLastReportTime = 0; 

MMS_NATNegotiationRelayingServerConnectionHandler::MMS_NATNegotiationRelayingServerConnectionHandler(MMS_Settings& theSettings)
: mySettings(theSettings)
, myWorkerThread(NULL)
{
	myWriteSqlConnection = new MDB_MySqlConnection(
		theSettings.WriteDbHost,
		theSettings.WriteDbUser,
		theSettings.WriteDbPassword,
		MMS_InitData::GetDatabaseName(),
		false);
	if (!myWriteSqlConnection->Connect())
	{
		LOG_FATAL("COULD NOT CONNECT TO DATABASE");
		assert(false);
	}
}

MMS_NATNegotiationRelayingServerConnectionHandler::~MMS_NATNegotiationRelayingServerConnectionHandler()
{
	LOG_DEBUGPOS(); 
	myWriteSqlConnection->Disconnect();
	delete myWriteSqlConnection;
}

void 
MMS_NATNegotiationRelayingServerConnectionHandler::OnSocketConnected(MMS_IocpServer::SocketContext* aContext)
{
}

void 
MMS_NATNegotiationRelayingServerConnectionHandler::OnSocketClosed(MMS_IocpServer::SocketContext* aContext)
{
	MMS_PerConnectionData* pcd = aContext->GetUserdata(); 

	MMS_MasterServer::GetInstance()->myNatLutContainer.RemovePeer(pcd->natnegSessionCookie, pcd->natnegProfileId); 

	MMS_NATNegotiationLutContainer::LutRef peer = MMS_MasterServer::GetInstance()->myNatLutContainer.GetPeerLut(pcd->natnegSessionCookie, pcd->natnegPeersProfileId); 
	if(peer.IsGood())
	{
		myWorkerThread->CloseSocket(peer->mySocketContext); 
		_InterlockedDecrement(&ourNumConnected); 
	}
}

void 
MMS_NATNegotiationRelayingServerConnectionHandler::OnIdleTimeout()
{
	static MT_Mutex mutex;
	static unsigned int lastUpdate = 0;
	mutex.Lock();
	const unsigned int currentTime = (unsigned int) time(NULL);
	if (lastUpdate + 10 < currentTime)
		lastUpdate = currentTime;
	else
	{
		mutex.Unlock();
		return;
	}
	mutex.Unlock();
	PrivReportStatus(); 
	MMS_MasterServer::GetInstance()->myNatLutContainer.PurgeOldLuts(); 
}

DWORD 
MMS_NATNegotiationRelayingServerConnectionHandler::GetTimeoutTime()
{
	myThreadNumber = _InterlockedIncrement(&ourNextThreadNum); 
	return INFINITE; 
}

bool 
MMS_NATNegotiationRelayingServerConnectionHandler::PrivRegister(MN_ReadMessage& theRm, 
																MN_WriteMessage& theWm, 
																MMS_IocpServer::SocketContext* const thePeer)
{
	bool good = true; 
	MMG_NATNegotiationProtocol::ClientToRelayingRequestRegister registerRequest; 
	
	MMS_PerConnectionData* pcd = thePeer->GetUserdata();

	good = good && registerRequest.FromStream(theRm);
	good = good && registerRequest.IsSane(); 
	if(good)
	{
		if (mySettings.HasValidSecret(registerRequest.myAuthToken))
		{
			pcd->isPlayer = true; 
			pcd->isAuthorized = true; 
			pcd->natnegSessionCookie = 0;
			pcd->natnegPeersProfileId = 0;
			pcd->isNatNegConnection = true; 

			MMG_NATNegotiationProtocol::RelayingToClientResponseRegister registerResponse;
			registerResponse.myRequestId = registerRequest.myRequestId; 
			registerResponse.myPublicEndpoint.Init(thePeer->myPeerAddr, thePeer->myPeerPort); 
			registerResponse.myReservedDummy = registerRequest.myReservedDummy; 
			registerResponse.ToStream(theWm); 
		}
		else 
		{
			LOG_ERROR("Unauthorized request from %s, disconnecting", thePeer->myPeerIpNumber);
			return false; 
		}
	}
	else 
	{
		LOG_ERROR("protocol error from: %s, disconnecting", thePeer->myPeerIpNumber);
		return false; 
	}

	return good; 
}

bool 
MMS_NATNegotiationRelayingServerConnectionHandler::PrivConnectToPeer(MN_ReadMessage& theRm, 
																	 MN_WriteMessage& theWm, 
																	 MMS_IocpServer::SocketContext* const theClient)
{
	MMS_PerConnectionData* pcd = theClient->GetUserdata();
	if(!pcd->isPlayer || !pcd->isAuthorized)
	{
		LOG_ERROR("Unauthorized request from %s, disconnecting", theClient->myPeerIpNumber);
		return false; 
	}

	bool good = true; 
	MMG_NATNegotiationProtocol::ClientToRelayingRequestConnectToPeer connectionRequest; 

	if(!mySettings.AllowNATNegDataRelaying)
	{
		LOG_ERROR("Server is not accepting any new data relaying connections, disconnecting", theClient->myPeerIpNumber);
		return false; 
	}

	good = good && connectionRequest.FromStream(theRm);
	good = good && connectionRequest.IsSane(); 
	if(good)
	{
		// check if I have added myself already, if so ERROR 
		// add myself key: (session cookie + my own profile id), data: (my socket context, my request id, my reserved dummy)
		// set userdata (the session cookie + peers profile id) 
		// check if peer is added 
		// if not return 
		// else, respond to peers

		MMS_PerConnectionData* pcd = theClient->GetUserdata(); 

		// test that user don't add himself twice
		MMS_NATNegotiationLutContainer::LutRef me = MMS_MasterServer::GetInstance()->myNatLutContainer.GetPeerLut(connectionRequest.mySessionCookie, connectionRequest.myProfileId); 
		if(me.IsGood())
		{
			me.Release(); 
			LOG_ERROR("Peer (%d) is trying to connect to peer (%d) twice within the same session, disconnecting %s", 
				pcd->natnegProfileId, pcd->natnegPeersProfileId, theClient->myPeerIpNumber);
			// I have to tear down the session here 
			return false; 
		}
		me.Release(); 

		// add myself 
		MMS_MasterServer::GetInstance()->myNatLutContainer.AddPeer(connectionRequest.mySessionCookie, connectionRequest.myProfileId, theClient, 
			connectionRequest.myRequestId, connectionRequest.myReservedDummy); 

		// set my user data 
		pcd->natnegSessionCookie = connectionRequest.mySessionCookie; 
		pcd->natnegProfileId = connectionRequest.myProfileId; 
		pcd->natnegPeersProfileId = connectionRequest.myPeersProfileId; 

		// check if peer is added 
		MMS_NATNegotiationLutContainer::LutRef peer = MMS_MasterServer::GetInstance()->myNatLutContainer.GetPeerLut(connectionRequest.mySessionCookie, connectionRequest.myPeersProfileId); 
		if(peer.IsGood())
		{
			// send response to myself 
			MMG_NATNegotiationProtocol::RelayingToClientResponseConnectToPeer connectionResponse; 
			connectionResponse.myRequestId = connectionRequest.myRequestId; 
			connectionResponse.myRequestOk = 1; 
			connectionResponse.myReservedDummy = connectionRequest.myReservedDummy; 
			connectionResponse.ToStream(theWm); 
				
			// send response to my peer 
			connectionResponse.myRequestId = peer->myRequestId; 
			connectionResponse.myRequestOk = 1; 
			connectionResponse.myReservedDummy = peer->myReservedDummy; 
			MN_WriteMessage message; 
			connectionResponse.ToStream(message); 
			good = good && myWorkerThread->SendMessageTo(message, peer->mySocketContext); 
			if(good)
				_InterlockedIncrement(&ourNumConnected);
			else
				LOG_ERROR("failing to send data from %s to peer, disconnecting", theClient->myPeerIpNumber);	
		}
	}
	else 
	{
		LOG_ERROR("protocol error from: %s, returning false", theClient->myPeerIpNumber);
		return false; 
	}


	return good; 
}

bool 
MMS_NATNegotiationRelayingServerConnectionHandler::PrivSendData(MN_ReadMessage& theRm, 
																MN_WriteMessage& theWm, 
																MMS_IocpServer::SocketContext* const thePeer)
{
	MMS_PerConnectionData* pcd = thePeer->GetUserdata(); 
	if(!pcd->isPlayer || !pcd->isAuthorized)
	{
		LOG_ERROR("Unauthorized request from %s, disconnecting", thePeer->myPeerIpNumber);
		return false; 
	}

	bool good = true; 

	MMG_NATNegotiationProtocol::Data data; 
	unsigned char buffer[MMG_NATNegotiationProtocol::PEER_DATA_BUFFER_SIZE];
	data.myData = (void*) buffer; 
	data.myDataLength = MMG_NATNegotiationProtocol::PEER_DATA_BUFFER_SIZE; 
	good = good && data.FromStream(theRm); 
	good = good && data.IsSane(); 
	if(good)
	{
		MN_WriteMessage message(MMG_MAX_BUFFER_SIZE); 
		data.ToStream(message); 

		MMS_NATNegotiationLutContainer::LutRef peer = MMS_MasterServer::GetInstance()->myNatLutContainer.GetPeerLut(pcd->natnegSessionCookie, pcd->natnegPeersProfileId); 
		if(!peer.IsGood())
		{
			LOG_ERROR("Failed to fetch peer LUT for %d, closing peer %d, %s", 
				pcd->natnegPeersProfileId, pcd->natnegProfileId, thePeer->myPeerIpNumber);
			return false; 
		}

		good = good && myWorkerThread->SendMessageTo(message, peer->mySocketContext); 	
		if(good)
			_InterlockedIncrement(&ourMessagesSent);
		else
			LOG_ERROR("failed to send data from %s to peer, disconnecting", thePeer->myPeerIpNumber);
	}
	else 
	{
		LOG_ERROR("protocol error from: %s", thePeer->myPeerIpNumber);
		return false; 
	}

	return good; 
}

bool 
MMS_NATNegotiationRelayingServerConnectionHandler::PrivSendCookie(MN_ReadMessage& theRm, 
																  MN_WriteMessage& theWm, 
																  MMS_IocpServer::SocketContext* const thePeer)
{
	MMS_PerConnectionData* pcd = thePeer->GetUserdata(); 
	if(!pcd->isPlayer || !pcd->isAuthorized)
	{
		LOG_ERROR("Unauthorized request from %s, disconnecting", thePeer->myPeerIpNumber);
		return false; 
	}

	bool good = true; 

	MMG_NATNegotiationProtocol::Cookie cookie; 
	good = good && cookie.FromStream(theRm); 
	good = good && cookie.IsSane(); 
	if(good)
	{
		MN_WriteMessage message; 
		cookie.ToStream(message); 

		MMS_NATNegotiationLutContainer::LutRef peer = MMS_MasterServer::GetInstance()->myNatLutContainer.GetPeerLut(pcd->natnegSessionCookie, pcd->natnegPeersProfileId); 
		if(!peer.IsGood())
		{
			LOG_ERROR("Failed to fetch peer LUT for %d, closing peer %d, %s", 
				pcd->natnegPeersProfileId, pcd->natnegProfileId, thePeer->myPeerIpNumber);
			// the connection is closed ... go figure, close peer
			return false; 
		}

		good = myWorkerThread->SendMessageTo(message, peer->mySocketContext); 	
		if(!good)
			LOG_ERROR("Failed to send data to peer, disconnecting %s", thePeer->myPeerIpNumber); 
	}
	else 
	{
		LOG_ERROR("protocol error from: %s", thePeer->myPeerIpNumber);
		return false; 
	}

	return good; 
}

bool 
MMS_NATNegotiationRelayingServerConnectionHandler::PrivHandlePing(MN_ReadMessage& theRm, 
																  MN_WriteMessage& theWm, 
																  MMS_IocpServer::SocketContext* const thePeer)
{
	MMS_PerConnectionData* pcd = thePeer->GetUserdata(); 
	if(!pcd->isPlayer || !pcd->isAuthorized)
	{
		LOG_ERROR("Unauthorized request from %s, disconnecting", thePeer->myPeerIpNumber);
		return false; 
	}

	bool good = true; 

	MMG_NATNegotiationProtocol::Ping ping; 
	good = good && ping.FromStream(theRm); 
	good = good && ping.IsSane(); 
	if(good)
	{
		MN_WriteMessage message; 
		ping.ToStream(message); 

		MMS_NATNegotiationLutContainer::LutRef peer = MMS_MasterServer::GetInstance()->myNatLutContainer.GetPeerLut(pcd->natnegSessionCookie, pcd->natnegPeersProfileId); 
		if(!peer.IsGood())
		{
			LOG_ERROR("Failed to fetch peer LUT for %d, closing peer %d, %s", 
				pcd->natnegPeersProfileId, pcd->natnegProfileId, thePeer->myPeerIpNumber);
			return false; 
		}

		good = myWorkerThread->SendMessageTo(message, peer->mySocketContext); 	
		if(!good)
			LOG_ERROR("Failed to send data to peer, disconnecting %s", thePeer->myPeerIpNumber); 
	}
	else 
	{
		LOG_ERROR("protocol error from: %s", thePeer->myPeerIpNumber);
		return false; 
	}

	return good; 
}

bool 
MMS_NATNegotiationRelayingServerConnectionHandler::PrivHandlePong(MN_ReadMessage& theRm, 
																  MN_WriteMessage& theWm, 
																  MMS_IocpServer::SocketContext* const thePeer)
{
	MMS_PerConnectionData* pcd = thePeer->GetUserdata(); 
	if(!pcd->isPlayer || !pcd->isAuthorized)
	{
		LOG_ERROR("Unauthorized request from %s, disconnecting", thePeer->myPeerIpNumber);
		return false; 
	}

	bool good = true; 

	MMG_NATNegotiationProtocol::Pong pong; 
	good = good && pong.FromStream(theRm); 
	good = good && pong.IsSane(); 
	if(good)
	{
		MN_WriteMessage message; 
		pong.ToStream(message); 

		MMS_NATNegotiationLutContainer::LutRef peer = MMS_MasterServer::GetInstance()->myNatLutContainer.GetPeerLut(pcd->natnegSessionCookie, pcd->natnegPeersProfileId); 
		if(!peer.IsGood())
		{
			LOG_ERROR("Failed to fetch peer LUT for %d, closing peer %d, %s", 
				pcd->natnegPeersProfileId, pcd->natnegProfileId, thePeer->myPeerIpNumber);
			return false; 
		}

		good = myWorkerThread->SendMessageTo(message, peer->mySocketContext); 	
		if(!good)
			LOG_ERROR("Failed to send data to peer, disconnecting %s", thePeer->myPeerIpNumber); 
	}
	else 
	{
		LOG_ERROR("protocol error from: %s", thePeer->myPeerIpNumber);
		return false; 
	}

	return good; 
}

#define LOG_EXECUTION_TIME(MESSAGE, START_TIME) \
	{ unsigned int currentTime = GetTickCount(); \
	MMS_MasterServer::GetInstance()->AddSample("NATNEGRELAY:" #MESSAGE, currentTime - START_TIME); \
	  START_TIME = currentTime; } 


void 
MMS_NATNegotiationRelayingServerConnectionHandler::PrivUpdateStats()
{
	MMS_Statistics* stats = MMS_Statistics::GetInstance(); 
	stats->HandleMsgNATRelay(); 
	stats->SQLQueryNATRelay(myWriteSqlConnection->myNumExecutedQueries); 
	myWriteSqlConnection->myNumExecutedQueries = 0; 
}

bool 
MMS_NATNegotiationRelayingServerConnectionHandler::HandleMessage(MMG_ProtocolDelimiters::Delimiter theDelimeter,
																 MN_ReadMessage& theIncomingMessage, 
																 MN_WriteMessage& theOutgoingMessage, 
																 MMS_IocpServer::SocketContext* thePeer)
{
	unsigned int startTime = GetTickCount(); 
	bool good = true;


	switch (theDelimeter)
	{
		case MMG_ProtocolDelimiters::NATNEG_CLIENT_TO_RELAYING_REQUEST_REGISTER:
			good = good && PrivRegister(theIncomingMessage, theOutgoingMessage, thePeer);
			LOG_EXECUTION_TIME(NATNEG_CLIENT_TO_RELAYING_REQUEST_REGISTER, startTime); 
			break;

		case MMG_ProtocolDelimiters::NATNEG_CLIENT_TO_RELAYING_REQUEST_CONNECT_TO_PEER: 
			good = good && PrivConnectToPeer(theIncomingMessage, theOutgoingMessage, thePeer); 
			LOG_EXECUTION_TIME(NATNEG_CLIENT_TO_RELAYING_REQUEST_CONNECT_TO_PEER, startTime); 
			break; 

		case MMG_ProtocolDelimiters::NATNEG_COOKIE: 
			good = good && PrivSendCookie(theIncomingMessage, theOutgoingMessage, thePeer); 
			LOG_EXECUTION_TIME(, startTime); 
			break; 

		case MMG_ProtocolDelimiters::NATNEG_DATA: 
			good = good && PrivSendData(theIncomingMessage, theOutgoingMessage, thePeer); 
			LOG_EXECUTION_TIME(NATNEG_COOKIE, startTime); 
			break; 

		case MMG_ProtocolDelimiters::NATNEG_PING: 
			good = good && PrivHandlePing(theIncomingMessage, theOutgoingMessage, thePeer); 
			LOG_EXECUTION_TIME(NATNEG_PING, startTime); 
			break; 

		case MMG_ProtocolDelimiters::NATNEG_PONG:
			good = good && PrivHandlePong(theIncomingMessage, theOutgoingMessage, thePeer); 
			LOG_EXECUTION_TIME(NATNEG_PONG, startTime); 
			break; 

		default:
			LOG_ERROR("Unknown delimiter %u from client %s", (unsigned int)theDelimeter, thePeer->myPeerIpNumber);
			good = false;
			break; 
	}

	if (!good)
	{
		LOG_ERROR("RETURNING FALSE");
	}

	PrivUpdateStats(); 

	return good;
}


void 
MMS_NATNegotiationRelayingServerConnectionHandler::PrivReportStatus()
{
	unsigned int currentTime; 

	currentTime = GetTickCount(); 

	if(myThreadNumber != 0)
	{
		return; 
	}

	if((currentTime - ourLastReportTime) >= 20000)
	{
		ourLastReportTime = currentTime; 
		LOG_INFO("MMS_NATNegotiationRelayingServerConnectionHandler: num connected: %d", ourNumConnected); 
		LOG_INFO("MMS_NATNegotiationRelayingServerConnectionHandler: messages sent: %d", ourMessagesSent); 	
	}
}
