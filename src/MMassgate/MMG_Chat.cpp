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

#include "MI_Time.h"

#include "MC_Debug.h"
#include "MC_Profiler.h"

#include "MN_Connection.h"

#include "MMG_Chat.h"
#include "MMG_IChatRoomListener.h"
#include "MMG_ServersAndPorts.h"
#include "MMG_MinimalProfileCache.h"
#include "MMG_Messaging.h"
#include "MN_Resolver.h"
#include "MMG_ProtocolDelimiters.h"
#include "MMG_MasterConnection.h"

static MMG_Chat* ourInstance = NULL;

MMG_Chat*
MMG_Chat::GetInstance()
{
	return ourInstance;
}

MMG_Chat::MMG_Chat(MN_WriteMessage& aWriteMessage, MMG_Messaging* aMessaging)
: myGatestoneMessaging(aMessaging)
, myOutgoingMessage(aWriteMessage)
{
	myChatRooms.Init(8,8,false);
	myHasEverGottenResponseFromServer = false; // for debugging
	myRoomsListingListener = NULL;
	ourInstance = this;
}

bool
MMG_Chat::HandleMessage(MMG_ProtocolDelimiters::Delimiter aDelimiter, 
						MN_ReadMessage& theStream)
{
	bool good = true; 

	switch(aDelimiter)
	{
	case MMG_ProtocolDelimiters::CHAT_ROOM_INFO_RSP:
		good = good && PrivHandleRoomInfo(theStream);
		break;
	case MMG_ProtocolDelimiters::CHAT_NO_MORE_ROOMS_IN_LIST:
		good = good && PrivHandleNoMoreRooms(theStream);
		break;
	case MMG_ProtocolDelimiters::CHAT_REAL_JOIN_RSP:
		good = good && PrivHandleJoinRoom(theStream);
		break;
	case MMG_ProtocolDelimiters::CHAT_USER_CHAT_MESSAGE:
		good = good && PrivHandleChatInRoom(theStream);
		break;
	case MMG_ProtocolDelimiters::CHAT_USER_IN_ROOM:
		good = good && PrivHandleUserInRoom(theStream);
		break;
	case MMG_ProtocolDelimiters::CHAT_USER_LEFT:
		good = good && PrivHandleUserLeft(theStream);
		break;
	case MMG_ProtocolDelimiters::CHAT_DENIED_ROOM_FULL:
	case MMG_ProtocolDelimiters::CHAT_DENIED_ROOM_PASSWORD_WRONG:
	case MMG_ProtocolDelimiters::CHAT_DENIED_COULD_NOT_JOIN_ROOM:
		good = good && PrivHandleJoinDenied(aDelimiter, theStream);
		break;
	default:
		MC_DEBUG("Protocol error!");
		assert(false);
		good = false;
		break;
	};

	if (good)
		HandleDelayedItems();

	return good; 
}


void
MMG_Chat::GetRoomListing(MMG_IChatRoomsListener* aListener)
{
	if (PrivGetMessaging())
		PrivGetMessaging()->AddProfileListener(this);

	MC_PROFILER_BEGIN(a, __FUNCTION__);
	myOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::CHAT_LIST_ROOMS_REQ);
	myRoomsListingListener = aListener;
}

void 
MMG_Chat::CancelRoomListing()
{ 
	myRoomsListingListener = NULL; 
}

bool 
MMG_Chat::PrivHandleRoomInfo(MN_ReadMessage& theRm)
{
	MC_PROFILER_BEGIN(a, __FUNCTION__);
	bool good = true;

	if (myRoomsListingListener)
	{
		MMG_Chat::ChatRoomName name;
		unsigned int roomId;
		unsigned char numChatters, isPasswordProtected;

		good = good && theRm.ReadLocString(name.GetBuffer(), name.GetBufferSize());
		good = good && theRm.ReadUInt(roomId);
		good = good && theRm.ReadUChar(numChatters);
		good = good && theRm.ReadUChar(isPasswordProtected);

		if (good)
		{
			if (!myRoomsListingListener->OnReceiveRoom(name, roomId, numChatters, isPasswordProtected!=0?true:false))
				myRoomsListingListener = NULL;
		}
	}
	if (!good)
	{
		MC_DEBUG("WARNING. Potential protocol error.");
	}
	return good;
}

bool 
MMG_Chat::PrivHandleNoMoreRooms(MN_ReadMessage& theRm)
{
	if (myRoomsListingListener)
		myRoomsListingListener->OnNoMoreRoomsAvailable();
	return true;
}

void 
MMG_Chat::JoinRoom(const ChatRoomName& theRoom, MMG_IChatRoomListener* aListener, const MC_LocChar* aPassword)
{
	if (PrivGetMessaging())
		PrivGetMessaging()->AddProfileListener(this);

	myOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::CHAT_REAL_JOIN_REQ);
	myOutgoingMessage.WriteLocString(theRoom.GetBuffer(), theRoom.GetLength());
	myOutgoingMessage.WriteLocString(aPassword ? aPassword:L"");

	ChatRoom* room = new ChatRoom(theRoom, aListener, aPassword);
	myChatRooms.Add(room);
}

void
MMG_Chat::LeaveRoom(const ChatRoomName& theRoom)
{
	MC_DEBUGPOS();
	ChatRoom* room = PrivFindRoom(theRoom);

	if (room == NULL)
	{
		MC_DEBUG("cannot leave a room we're not part of");
		return;
	}
	myOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::CHAT_LEAVE_ROOM_REQ);
	myOutgoingMessage.WriteUInt(room->GetRoomId());
	myChatRooms.Remove(room);

	for (int i = 0; i < myDelayedItems.Count(); i++)
		if (myDelayedItems[i].roomId == room->GetRoomId())
			myDelayedItems.RemoveAtIndex(i--);

	if (room->myListener)
		room->myListener->OnLeaveRoom(theRoom);
	delete room;
}

bool 
MMG_Chat::HasJoinedRoom(const ChatRoomName& theRoom)
{
	return PrivFindRoom(theRoom) != NULL;
}


void 
MMG_Chat::SendChat(const ChatRoomName& theRoom, const MC_LocChar* const theChatStringPtr)
{
	MC_PROFILER_BEGIN(a, __FUNCTION__);


	MC_StaticLocString<1024> theChatString = theChatStringPtr;

#ifdef _DEBUG
	assert(theChatString.GetLength() < MMG_InstantMessageStringSize);
#endif

	theChatString.TrimLeft().TrimRight();
	if (theChatString.GetLength() == 0)
		return;
	ChatRoom* room = PrivFindRoom(theRoom);
	if (room && room->GetRoomId() != 0)
	{
		myOutgoingMessage.WriteDelimiter(MMG_ProtocolDelimiters::CHAT_SEND_MESSAGE_REQ);
		myOutgoingMessage.WriteUInt(room->GetRoomId());
		myOutgoingMessage.WriteLocString(theChatString.GetBuffer(), theChatString.GetLength());
	}
}

MMG_Chat::~MMG_Chat()
{
	if (PrivGetMessaging())
		PrivGetMessaging()->RemoveProfileListener(this);
	ourInstance = NULL;
	myChatRooms.DeleteAll();
}

bool
MMG_Chat::PrivHandleJoinRoom(MN_ReadMessage& theRm)
{
	bool good = true;
	MMG_Chat::ChatRoomName roomName, roomInstance;
	unsigned int roomId;
	good = good && theRm.ReadLocString(roomName.GetBuffer(), roomName.GetBufferSize());
	good = good && theRm.ReadLocString(roomInstance.GetBuffer(), roomName.GetBufferSize());
	good = good && theRm.ReadUInt(roomId);

	if (good)
	{
		for (int i = 0; i < myChatRooms.Count(); i++)
		{
			if (myChatRooms[i]->myIdentifier == roomName)
			{
				myChatRooms[i]->myIdentifier = roomInstance;
				myChatRooms[i]->myRoomId = roomId;
				myChatRooms[i]->myHasJoined = true;
				myChatRooms[i]->myListener->OnJoinRoom(roomInstance, MMG_IChatRoomListener::JoinStatus::JOIN_OK);
				myChatRooms[i]->myJoinTime = MI_Time::ourCurrentSystemTime;

				break;
			}
		}
	}

	return good;
}

void 
MMG_Chat::Update()
{
	HandleDelayedItems();
}

void 
MMG_Chat::UpdatedProfileInfo(const MMG_Profile& theProfile)
{
	HandleDelayedItems();
}

void 
MMG_Chat::HandleDelayedItems()
{
	if (PrivGetMessaging() == NULL)
		return;

	for (int i = 0; i < myDelayedItems.Count(); i++)
	{
		DelayedItem copy = myDelayedItems[i];

		ChatRoom* room = PrivFindRoom(copy.roomId);
		if (room == NULL)
		{
			// Don't do anything until we know the room
			continue;
		}

		MMG_Chat::ChatRoomName roomName = room->GetIdentifier();
		MMG_Profile prof;
		bool knowsUser = PrivGetMessaging()->GetProfile(copy.profileId, prof);
		if (!knowsUser)
			PrivGetMessaging()->RetreiveUsername(copy.profileId);

		if(copy.type == DelayedItem::CHAT)
		{
			// Delayed because we don't know the room, or because we don't know the player?
			if (knowsUser)
			{
				myDelayedItems.RemoveAtIndex(i--);
				room->myListener->ReceiveChatMessage(roomName, prof, copy.msg);
			}
		}
		else if (copy.type == DelayedItem::USER_IN_ROOM)
		{
			if (knowsUser)
			{
				myDelayedItems.RemoveAtIndex(i--);
				room->myListener->ReceiveChatUserInRoom(roomName, prof);
			}
		}
		else if (copy.type == DelayedItem::LEAVE)
		{
			if (knowsUser)
			{
				myDelayedItems.RemoveAtIndex(i--);
				room->myListener->ReceiveChatUserLeave(roomName, prof);
			}
		}
	}
}


bool
MMG_Chat::PrivHandleJoinDenied(MN_DelimiterType errCode, MN_ReadMessage& theRm)
{
	MC_DEBUGPOS();
	MC_PROFILER_BEGIN(a, __FUNCTION__);
	MMG_Chat::ChatRoomName roomName;
	if (!theRm.ReadLocString(roomName.GetBuffer(), roomName.GetBufferSize()))
	{
		return false;
	}
	ChatRoom* room = PrivFindRoom(roomName);
	if (room)
	{
		if (errCode == MMG_ProtocolDelimiters::CHAT_DENIED_ROOM_FULL)
			room->myListener->OnJoinRoom(roomName, MMG_IChatRoomListener::JoinStatus::JOIN_DENIED_ROOM_FULL);
		else if (errCode == MMG_ProtocolDelimiters::CHAT_DENIED_ROOM_PASSWORD_WRONG)
			room->myListener->OnJoinRoom(roomName, MMG_IChatRoomListener::JoinStatus::JOIN_DENIED_WRONG_PASSWORD);
		else if (errCode == MMG_ProtocolDelimiters::CHAT_DENIED_COULD_NOT_JOIN_ROOM)
			room->myListener->OnJoinRoom(roomName, MMG_IChatRoomListener::JoinStatus::JOIN_DENIED_CANNOT_JOIN);
		myChatRooms.Remove(room);
		delete room;
	}
	return true;
}



bool
MMG_Chat::PrivHandleLeaveRoom(MN_ReadMessage& theRm)
{
	MC_PROFILER_BEGIN(a, __FUNCTION__);
	MC_DEBUGPOS();
	unsigned int roomId;
	if (theRm.ReadUInt(roomId))
	{
		// find the room and remove it
		ChatRoom* room = PrivFindRoom(roomId);
		if (room)
		{
			// This will most likely not happen as the room should be deleted on leave request.
			myChatRooms.Remove(room);
			if (room->myListener)
				room->myListener->OnLeaveRoom(room->myIdentifier);
			delete room;
		}
	}
	else
		return false;
	return true;
}

bool
MMG_Chat::PrivHandleChatInRoom(MN_ReadMessage& theRm)
{
	MC_PROFILER_BEGIN(a, __FUNCTION__);
	MC_LocString chatString;
	unsigned int profileId;
	unsigned int roomId;
	bool good = true;
	good = good && theRm.ReadUInt(roomId);
	good = good && theRm.ReadUInt(profileId);
	good = good && theRm.ReadLocString(chatString);
	if (good)
	{
		DelayedItem di;
		di.msg = chatString;
		di.profileId = profileId;
		di.roomId = roomId;
		di.type = DelayedItem::CHAT;
		myDelayedItems.Add(di);
	}
	else
	{
		MC_DEBUG("Protocol error.");
	}
	return good;
}

bool 
MMG_Chat::PrivHandleUserInRoom(MN_ReadMessage& theRm)
{
	MC_PROFILER_BEGIN(a, __FUNCTION__);

	unsigned int roomId;
	unsigned int profileId;
	bool good = true;

	good = good && theRm.ReadUInt(roomId);
	good = good && theRm.ReadUInt(profileId);

	if (good)
	{
		DelayedItem di;
		di.roomId = roomId;
		di.profileId = profileId;
		di.type = DelayedItem::USER_IN_ROOM;
		myDelayedItems.Add(di);
	}
	else
	{
		MC_DEBUG("Protocol error. Ignoring.");
	}
	return good;
} 

bool 
MMG_Chat::PrivHandleUserLeft(MN_ReadMessage& theRm)
{
	unsigned int	profileId;
	unsigned int	roomId;
	bool good = true;

	good = good && theRm.ReadUInt(roomId);
	good = good && theRm.ReadUInt(profileId);

	if (good)
	{
		DelayedItem di;
		di.type = DelayedItem::LEAVE;
		di.roomId = roomId;
		di.profileId = profileId;
		myDelayedItems.Add(di);
	}
	else
	{
		MC_DEBUG("Protocol error.");
	}
	return good;
}



MMG_Chat::ChatRoom* 
MMG_Chat::PrivFindRoom(unsigned int theRoomId)
{
	for (int i = 0; i < myChatRooms.Count(); i++)
		if (myChatRooms[i]->GetRoomId() == theRoomId)
			return myChatRooms[i];
	return NULL;
}

MMG_Chat::ChatRoom* 
MMG_Chat::PrivFindRoom(const ChatRoomName& theName)
{
	for (int i = 0; i < myChatRooms.Count(); i++)
		if (myChatRooms[i]->GetIdentifier() == theName)
			return myChatRooms[i];
	return NULL;
}


// =================== MMG_Chat::ChatRoom =========================================

MMG_Chat::ChatRoom::ChatRoom(const ChatRoomName& theRoomName, MMG_IChatRoomListener* aListener, const MC_LocChar* aPassord)
: myIdentifier(theRoomName)
, myListener(aListener)
, myPassword(aPassord)
, myRoomId(0)
, myJoinTime(0)
, myHasJoined(false)
{
}

MMG_Chat::ChatRoom::~ChatRoom()
{
}

MMG_Chat::ChatRoomName
MMG_Chat::ChatRoom::GetIdentifier()
{
	return myIdentifier;
}

unsigned int	
MMG_Chat::ChatRoom::GetRoomId()
{
	return myRoomId;
}


MMG_Messaging* 
MMG_Chat::PrivGetMessaging()
{
	if(myGatestoneMessaging)
		return myGatestoneMessaging;
	return MMG_Messaging::GetInstance();
}


