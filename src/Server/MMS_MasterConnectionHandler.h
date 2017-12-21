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
#ifndef MMS_MASTERCONNECTIONHANDLER__H_
#define MMS_MASTERCONNECTIONHANDLER__H_



// MMS Includes
#include "MMS_Constants.h"
#include "MMS_IOCPServer.h"
#include "MMS_IocpWorkerThread.h"
#include "MMS_AccountConnectionHandler.h"
#include "MMS_MessagingConnectionHandler.h"
#include "MMS_ServerTrackerConnectionHandler.h"
#include "MMS_ChatServerConnectionHandler.h"
#include "MMS_NATNegotiationRelayingServerConnectionHandler.h"

// MMG Includes
#include "MMG_AuthToken.h"
#include "MMG_ProtocolDelimiters.h"
#include "MMG_Tiger.h"
#include "MMG_ICipher.h"

// MN Includes
#include "MN_ReadMessage.h"
#include "MN_WriteMessage.h"

// Framework includes
#include "MT_Thread.h"

// System and other includes
#include "mysql.h"
#include <time.h>

class MMS_MasterServer;


class MMS_MasterConnectionHandler : public MMS_IocpWorkerThread
{
public:
	MMS_MasterConnectionHandler(MMS_Settings& someSettings, MMS_MasterServer* aServer);
	virtual bool HandleMessage(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);
//	virtual void ServicePostMortemDebug(MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer); 

	virtual ~MMS_MasterConnectionHandler();

	virtual DWORD GetTimeoutTime();
	virtual void OnIdleTimeout();

	struct PerSocketConnectionInfo
	{
		MMG_AuthToken	token;

		struct
		{
			unsigned int	fingerPrint;
			unsigned int	loginTime;
		}account;



	};

	MMS_AccountConnectionHandler myAccountHandler;
	MMS_MessagingConnectionHandler myMessagingHandler;
	MMS_ServerTrackerConnectionHandler myServertrackerHandler;
	MMS_ChatServerConnectionHandler myChatHandler;
	MMS_NATNegotiationRelayingServerConnectionHandler myNatNegHandler;

protected:
	virtual void OnSocketConnected(MMS_IocpServer::SocketContext* aContext);
	virtual void OnSocketClosed(MMS_IocpServer::SocketContext* aContext);

private:

	void OnReadyToStart();

	MDB_MySqlConnection*	myWriteSqlConnection;
	MDB_MySqlConnection*	myReadSqlConnection;

	MC_StaticString<1024>	mySqlStr;

	MMG_Tiger				myHasher;
	MMG_ICipher*			myCipher;
	unsigned long			myTimeOfLastUpdateAuthentication;

	const MMS_Settings&		mySettings;

};


#endif
