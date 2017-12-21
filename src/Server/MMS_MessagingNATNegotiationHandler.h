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
#ifndef __MMS_MESSAGINGNATNEGOTIATIONHANDLER_H__
#define __MMS_MESSAGINGNATNEGOTIATIONHANDLER_H__

#include "MMS_IocpWorkerThread.h"
#include "mdb_mysqlconnection.h"
#include "MN_WriteMessage.h"
#include "MN_ReadMessage.h"
#include "MMG_AuthToken.h"

class MMS_MessagingConnectionHandler; 

class MMS_MessagingNATNegotiationHandler
{
public:
	MMS_MessagingNATNegotiationHandler();
	~MMS_MessagingNATNegotiationHandler();

	bool HandleMessage(const MN_DelimiterType aDelimeter, 
					   MN_ReadMessage& theIncomingMessage, 
					   MN_WriteMessage& theOutgoingMessage, 
					   MMS_IocpServer::SocketContext* thePeer, 
					   const MMG_AuthToken& theToken);

	void UpdateDatabaseConnections(MDB_MySqlConnection* aReadConnection, 
								   MDB_MySqlConnection* aWriteConnection, 
								   MMS_MessagingConnectionHandler* aMessagingConnectionHandler);

private:
	bool PrivHandleClientConnectRequest(MN_WriteMessage& theWm, 
			 							MN_ReadMessage& theRm, 
										const MMG_AuthToken& theToken, 
										MMS_IocpServer::SocketContext* const thePeer); 

	bool PrivHandleClientConnectResponse(MN_WriteMessage& theWm, 
										 MN_ReadMessage& theRm, 
										 const MMG_AuthToken& theToken, 
										 MMS_IocpServer::SocketContext* const thePeer); 


	MDB_MySqlConnection* myWriteSqlConnection;
	MDB_MySqlConnection* myReadSqlConnection;
	MMS_MessagingConnectionHandler* myMessagingConnectionHandler; 

	static volatile long ourSessionCookie; 

};

#endif