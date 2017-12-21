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
#ifndef MMS_CHATSERVERONNECTIONHANDLER___H_
#define MMS_CHATSERVERONNECTIONHANDLER___H_

#include "MMS_IocpWorkerThread.h"
#include "MMS_Constants.h"
#include "MN_WriteMessage.h"
#include "MN_ReadMessage.h"
#include "MC_String.h"
#include "MMG_ICryptoHashAlgorithm.h"
#include "MC_String.h"
#include "MMS_ChatRoom.h"
#include "mdb_mysqlconnection.h"
#include <time.h>

class MMS_BanManager;

class MMS_ChatServerConnectionHandler : public MMS_IocpWorkerThread::FederatedHandler
{
	friend class MMS_MasterConnectionHandler;
	friend class MMS_ChatRoom;
	MMS_MasterConnectionHandler* myWorkerThread;
public:
	MMS_ChatServerConnectionHandler(MMS_Settings& someSettings);
	virtual ~MMS_ChatServerConnectionHandler();

	virtual bool HandleMessage(MMG_ProtocolDelimiters::Delimiter theDelimeter, MN_ReadMessage& theIncomingMessage, MN_WriteMessage& theOutgoingMessage, MMS_IocpServer::SocketContext* thePeer);


	virtual void OnSocketConnected(MMS_IocpServer::SocketContext* aContext);
	virtual void OnSocketClosed(MMS_IocpServer::SocketContext* aContext);
	virtual void OnReadyToStart();

	virtual DWORD GetTimeoutTime();
	virtual void OnIdleTimeout();
protected:

private:
	bool PrivHandleJoinRoom(MN_ReadMessage& theRm, MN_WriteMessage& theWm, const MMG_AuthToken& theAuthToken, MMS_IocpServer::SocketContext* const thePeer);
	bool PrivHandleJoinComplete(MN_ReadMessage& theRm, MN_WriteMessage& theWm, const MMG_AuthToken& theAuthToken, MMS_IocpServer::SocketContext* const thePeer);
	bool PrivHandleIncomingChat(MN_ReadMessage& theRm, MN_WriteMessage& theWm, const MMG_AuthToken& theAuthToken, MMS_IocpServer::SocketContext* const thePeer);
	bool PrivHandleLeaveRoom(MN_ReadMessage& theRm, MN_WriteMessage& theWm, const MMG_AuthToken& theAuthToken, MMS_IocpServer::SocketContext* const thePeer);
	bool PrivHandleListRooms(MN_ReadMessage& theRm, MN_WriteMessage& theWm, const MMG_AuthToken& theAuthToken, MMS_IocpServer::SocketContext* const thePeer);
	void PrivBroadcastTouchedRooms();
	unsigned int PrivLogChatRoomCreation(MMS_ChatRoom* aRoom, MMS_IocpServer::SocketContext* const thePeer); 

	MMS_ChatRoom* PrivGetOrCreateChatRoom(const MMG_Chat::ChatRoomName& theIdentifier, const MC_LocString& aPassword, MMS_IocpServer::SocketContext* const thePeer);
	MMS_ChatRoom* PrivFindChatRoom(unsigned int theIdentifier);


	MDB_MySqlConnection*				myWriteSqlConnection;

	MN_WriteMessage						myRecycledBroadcastMessage;
	const MMS_Settings&					mySettings;
	MC_LocString						myRecycledChatString; // so we don't have to allocate/deallocate, or use the stack.

	MC_GrowingArray<MMS_IocpServer::SocketContext*> myMulticastRecipients;
	MMS_BanManager*						myBanManager;

	// Shared variables that must be protected with a mutex:
	static MT_Mutex							myRoomsMutex;
	static MC_GrowingArray<MMS_ChatRoom*>	myChatRooms;

	void PrivUpdateStats(); 
};

#endif

