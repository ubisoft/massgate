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
#ifndef MMS_NATNEGOTIATIONSERVERONNECTIONHANDLER___H_
#define MMS_NATNEGOTIATIONSERVERONNECTIONHANDLER___H_

#include "MMS_IocpWorkerThread.h"
#include "MMS_Constants.h"
#include "MMG_AuthToken.h"
#include "MMS_NATNegotiationLut.h"
#include "MC_SmallObjectAllocator.h"
#include "mdb_mysqlconnection.h"

class MMS_MasterServer;

const unsigned int MMS_NATNEG_CONN_HANDLER_MAX_NUM_MUTEX = 8; 

class MMS_NATNegotiationRelayingServerConnectionHandler : public MMS_IocpWorkerThread::FederatedHandler
{
	friend class MMS_MasterConnectionHandler;
	MMS_IocpWorkerThread* myWorkerThread;

public:
	MMS_NATNegotiationRelayingServerConnectionHandler(MMS_Settings& someSettings);

	virtual ~MMS_NATNegotiationRelayingServerConnectionHandler();

	virtual bool HandleMessage(	MMG_ProtocolDelimiters::Delimiter theDelimeter,
								MN_ReadMessage& theIncomingMessage, 
								MN_WriteMessage& theOutgoingMessage, 
								MMS_IocpServer::SocketContext* thePeer);


	virtual void OnSocketClosed(MMS_IocpServer::SocketContext* aContext);
	virtual void OnSocketConnected(MMS_IocpServer::SocketContext* aContext);
	virtual void OnIdleTimeout();
	virtual DWORD GetTimeoutTime();
protected:

private: 
	bool PrivRegister(MN_ReadMessage& theRm, 
					  MN_WriteMessage& theWm, 
					  MMS_IocpServer::SocketContext* const thePeer); 

	bool PrivConnectToPeer(MN_ReadMessage& theRm, 
						   MN_WriteMessage& theWm, 
						   MMS_IocpServer::SocketContext* const theClient); 

	bool PrivSendCookie(MN_ReadMessage& theRm, 
						MN_WriteMessage& theWm, 
						MMS_IocpServer::SocketContext* const thePeer); 

	bool PrivSendData(MN_ReadMessage& theRm, 
					  MN_WriteMessage& theWm, 
					  MMS_IocpServer::SocketContext* const thePeer); 

	bool PrivHandlePing(MN_ReadMessage& theRm, 
					  MN_WriteMessage& theWm, 
					  MMS_IocpServer::SocketContext* const thePeer); 

	bool PrivHandlePong(MN_ReadMessage& theRm, 
			   		    MN_WriteMessage& theWm, 
					    MMS_IocpServer::SocketContext* const thePeer); 

	const MMS_Settings&				mySettings; 

	static volatile long				ourNextThreadNum; 
	unsigned int						myThreadNumber;

	MDB_MySqlConnection*				myWriteSqlConnection;


	void PrivReportStatus(); 

	static volatile long ourNumConnected; 
	static volatile long ourMessagesSent; 
	static volatile long ourLastReportTime; 

	void PrivUpdateStats(); 
}; 

#endif