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
#ifndef MMS_ACCOUNTCONNECTIONHANDLER_H__
#define MMS_ACCOUNTCONNECTIONHANDLER_H__

#include "MMS_IocpWorkerThread.h"

#include "MMG_AccountProtocol.h"
#include "MMG_CipherFactory.h"
#include "MMG_Tiger.h"
#include "MMS_ConnectionLog.h"
#include "MMS_Constants.h"
#include "MMS_DatabasePool.h"
#include "MMS_MultiKeyManager.h"
#include "mdb_mysqlconnection.h"
#include "MMS_ThreadSafeQueue.h"
#include "MMS_TimeoutTimer.h"
#include "mysql.h"
#include <time.h>

class MMS_BanManager;

class MMS_AccountConnectionHandler : public MMS_IocpWorkerThread::FederatedHandler
{
public:
	friend class MMS_MasterConnectionHandler;
	MMS_AccountConnectionHandler(MMS_Settings& someSettings);
	virtual bool HandleMessage(MMG_ProtocolDelimiters::Delimiter theDelimiter, MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);

	virtual ~MMS_AccountConnectionHandler();

	virtual DWORD GetTimeoutTime();
	virtual void OnIdleTimeout();

	void OnReadyToStart();
	void OnSocketConnected(MMS_IocpServer::SocketContext* aContext);
	void OnSocketClosed(MMS_IocpServer::SocketContext* aContext);

protected:

private:
	bool myHandleAuthenticateAccount(const MMG_AccountProtocol::Query& theQuery, MMG_AccountProtocol::Response& response, MMS_IocpServer::SocketContext* thePeer);
	bool myHandleRetrieveProfiles(const MMG_AccountProtocol::Query& theQuery, MMG_AccountProtocol::Response& response, MMS_IocpServer::SocketContext* thePeer, MDB_MySqlConnection* theSqlConnectionToUse=NULL);
	bool myHandleModifyProfile(const MMG_AccountProtocol::Query& theQuery, MMG_AccountProtocol::Response& response, MMS_IocpServer::SocketContext* thePeer);
	bool myHandleCreateAccount(const MMG_AccountProtocol::Query& theQuery, MMG_AccountProtocol::Response& response, const MMS_IocpServer::SocketContext* const thePeer);
	bool myHandleActivateAccessCode(const MMG_AccountProtocol::Query& theQuery, MMG_AccountProtocol::Response& response, MMS_IocpServer::SocketContext* thePeer);
	bool myHandlePrepareCreateAccount(const MMG_AccountProtocol::Query& theQuery, MMG_AccountProtocol::Response& response, const MMS_IocpServer::SocketContext* const thePeer);
	bool myHandleCredentialsRequest(const MMG_AccountProtocol::Query& theQuery, MMG_AccountProtocol::Response& response, MMS_IocpServer::SocketContext* thePeer);
	bool myHandleLogout(const MMG_AccountProtocol::Query& theQuery, MMG_AccountProtocol::Response& response, MMS_IocpServer::SocketContext* thePeer);

	bool PrivMayClientGetPatch(const char* thePeerIp);

	MDB_MySqlConnection*			myWriteSqlConnection;
	MDB_MySqlConnection*			myReadSqlConnection;

	MC_StaticString<1024>			mySqlStr;

	MMG_Tiger						myHasher;
	MMG_ICipher*					myCipher;
	unsigned long					myTimeOfLastUpdateAuthentication;

	const MMS_Settings&				mySettings;
	MMS_BanManager*					myBanManager;

	static volatile long			ourNextThreadNum; 
	unsigned						myThreadNumber; 

	char myTempDebugDescriptionBuffer[2048]; // Filled by MDB_MySqlConnection::GetEscapeString()

	struct ConnectionInfo
	{
		MMG_AuthToken	token;
		unsigned int	fingerPrint;
		unsigned int	loginTime;
	};

	void PrivUpdateStats(); 
	bool PrivIpIsBanned(const MMS_IocpServer::SocketContext* thePeer);
	bool PrivValidGuestKeyIp(MMS_MultiKeyManager::CdKey& aKey, const MMS_IocpServer::SocketContext* thePeer);
	unsigned int PrivGetOutgoingMessageCount(unsigned int aProfileId);
	MMS_IocpWorkerThread* myWorkerThread;
};

#endif
