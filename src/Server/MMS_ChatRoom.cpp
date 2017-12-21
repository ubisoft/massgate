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
#include "MMS_ChatRoom.h"
#include "MC_KeyTreeInorderIterator.h"
#include "MC_Debug.h"
#include "MI_Time.h"
#include "MC_String.h"
#include "mdb_stringconversion.h"
#include "MMG_ServersAndPorts.h"
#include "mc_prettyprinter.h"
#include "MMG_ProtocolDelimiters.h"
#include "MMS_PersistenceCache.h"
#include "MMS_ChatServerConnectionHandler.h"
#include "MMS_MasterConnectionHandler.h"

#include "ML_Logger.h"

MMS_ChatRoom::MMS_ChatRoom(MMS_ChatServerConnectionHandler* theConnectionHandler, const MMG_Chat::ChatRoomName& theIdentifier, const MC_LocString& aPassword)
: myIdentifier(theIdentifier)
, myPassword(aPassword)
, myBroadcastMessage(MMS_IocpServer::MAX_BUFFER_LEN)
, myConnectionHandler(theConnectionHandler)
{
	myTimeOfLastActivity = 0;
}

MMS_ChatRoom::~MMS_ChatRoom()
{
	LOG_DEBUGPOS();
}

bool 
MMS_ChatRoom::IsPasswordOk(const MC_LocString& aPassword)
{
	MT_MutexLock locker(myMutex);
	return myPassword == aPassword;
}


bool
MMS_ChatRoom::AddUser(unsigned int aProfileId, MN_WriteMessage& aResponse, MDB_MySqlConnection* aReadConnection, MMS_IocpServer::SocketContext* const thePeer)
{
	MT_MutexLock locker(myMutex);

	// Is the user disconnected but we don't know it yet?
	bool wasDuplicate = false;
	for (int i = 0; i < myUsersInRoom.Count(); i++)
	{
		if (myUsersInRoom[i].profileId == aProfileId)
		{
			LOG_ERROR("Found player %u in chat, duplicate due to disconnect", aProfileId);
			myUsersInRoom[i].theSocket = thePeer;
			wasDuplicate = true; // yes you could join
			break;
		}
	}

	// Tell user which players are in the room
	for (int i = 0; i < myUsersInRoom.Count(); i++)
	{
		aResponse.WriteDelimiter(MMG_ProtocolDelimiters::CHAT_USER_IN_ROOM);
		aResponse.WriteUInt(myRoomId);
		aResponse.WriteUInt(myUsersInRoom[i].profileId);
	}

	if (!wasDuplicate)
	{
		// Add user to room
		ChatUser user;
		user.profileId = aProfileId;
		user.theSocket = thePeer;
		myUsersInRoom.Add(user);

		// broadcast to all that the user joined
		myBroadcastMessage.WriteDelimiter(MMG_ProtocolDelimiters::CHAT_USER_IN_ROOM);
		myBroadcastMessage.WriteUInt(myRoomId);
		myBroadcastMessage.WriteUInt(aProfileId);
		Touch();
	}

	return true;
}

void
MMS_ChatRoom::Touch()
{
	MT_MutexLock locker(myMutex);
	myTimeOfLastActivity = MI_Time::GetSystemTime();
}

unsigned int
MMS_ChatRoom::GetNumberOfChatters()
{
	MT_MutexLock locker(myMutex);
	return myUsersInRoom.Count();
}

unsigned long 
MMS_ChatRoom::GetTimeOfLastActivity()
{
	MT_MutexLock locker(myMutex);
	return myTimeOfLastActivity;
}


void 
MMS_ChatRoom::RemoveUser(unsigned int theProfileId, MMS_IocpServer::SocketContext* aContext)
{
	MT_MutexLock locker(myMutex);

	bool foundUser = false;

	for (int i = 0; i < myUsersInRoom.Count(); i++)
	{
		if ((myUsersInRoom[i].profileId == theProfileId) && (myUsersInRoom[i].theSocket == aContext))
		{
			myUsersInRoom.RemoveAtIndex(i);
			foundUser = true;
			break;
		}
	}

	if (foundUser)
	{
		myBroadcastMessage.WriteDelimiter(MMG_ProtocolDelimiters::CHAT_USER_LEFT);
		myBroadcastMessage.WriteUInt(myRoomId);
		myBroadcastMessage.WriteUInt(theProfileId);
	}
}


void
MMS_ChatRoom::HandleIncomingChatMessage(const MMG_AuthToken& theToken, const MC_LocChar* theChatString, MMS_IocpServer::SocketContext* const thePeer)
{
	MT_MutexLock locker(myMutex);

	myBroadcastMessage.WriteDelimiter(MMG_ProtocolDelimiters::CHAT_USER_CHAT_MESSAGE);
	myBroadcastMessage.WriteUInt(myRoomId);
	myBroadcastMessage.WriteUInt(theToken.profileId);
	myBroadcastMessage.WriteLocString(theChatString, (int)wcslen(theChatString));

	Touch();
}


bool
MMS_ChatRoom::ConsumeBroadcastCache(MN_WriteMessage& aTarget, MC_GrowingArray<MMS_IocpServer::SocketContext*>& theDestinationSockets)
{
	MT_MutexLock locker(myMutex);
	if (myBroadcastMessage.Empty())
		return false;
	aTarget.Append(myBroadcastMessage);
	myBroadcastMessage.Clear();
	for (int i = 0; i < myUsersInRoom.Count(); i++)
		theDestinationSockets.Add(myUsersInRoom[i].theSocket);
	return theDestinationSockets.Count() > 0;
}


MT_Mutex& 
MMS_ChatRoom::GetMutex()
{
	return myMutex;
}

unsigned int 
MMS_ChatRoom::GetRoomId()
{
	MT_MutexLock locker(myMutex);
	return myRoomId;
}

MMG_Chat::ChatRoomName 
MMS_ChatRoom::GetIdentifier()
{
	MT_MutexLock locker(myMutex);
	return myIdentifier;
}


