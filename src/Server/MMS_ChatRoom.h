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
#ifndef MMS_CHATROOM__H__
#define MMS_CHATROOM__H__

#include "MC_KeyTree.h"
#include "MMG_Profile.h"
#include "MMS_Constants.h"
#include "MC_SortedGrowingArray.h"
#include "MMG_Chat.h"
#include "MMS_IOCPServer.h"

class MMS_ChatServerConnectionHandler;

class MMS_ChatRoom
{
public:
	static const unsigned int MAX_CHATTERS_IN_ROOM = 64;
	MMS_ChatRoom(MMS_ChatServerConnectionHandler* theConnectionHandler, const MMG_Chat::ChatRoomName& theIdentifier, const MC_LocString& aPassword);
	~MMS_ChatRoom();

	void HandleIncomingChatMessage(const MMG_AuthToken& theToken, const MC_LocChar* theChatString, MMS_IocpServer::SocketContext* const thePeer);
	void AppendUserList(MN_WriteMessage& theWm);

	bool AddUser(unsigned int aProfileId, MN_WriteMessage& aResponse, MDB_MySqlConnection* aReadConnection, MMS_IocpServer::SocketContext* const thePeer);
	void RemoveUser(unsigned int theProfileId, MMS_IocpServer::SocketContext* aContext);

	bool IsPasswordOk(const MC_LocString& aPassword);
	bool IsPasswordProtected() const { return myPassword.GetLength() != 0; }

	MT_Mutex& GetMutex();
	unsigned int GetRoomId();
	MMG_Chat::ChatRoomName GetIdentifier();

	void Touch();

	unsigned int GetNumberOfChatters();
	unsigned long GetTimeOfLastActivity();
	bool ConsumeBroadcastCache(MN_WriteMessage& aTarget, MC_GrowingArray<MMS_IocpServer::SocketContext*>& theDestinationSockets);

	MC_LocString				myPassword;
	unsigned int				myRoomId;

protected:
	struct ChatUser
	{
		unsigned int profileId;
		MMS_IocpServer::SocketContext* theSocket;
	};
	MC_HybridArray<ChatUser,64> myUsersInRoom;

private:
	MMS_ChatServerConnectionHandler* myConnectionHandler;
	MT_Mutex					myMutex;
	MMG_Chat::ChatRoomName		myIdentifier;
	MN_WriteMessage				myBroadcastMessage;
	volatile unsigned long		myTimeOfLastActivity;
};

#endif
