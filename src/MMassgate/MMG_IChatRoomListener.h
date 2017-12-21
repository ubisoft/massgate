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
#ifndef MMG_ICHATROOMLISTENER____H__
#define MMG_ICHATROOMLISTENER____H__

#include "MMG_Chat.h"
#include "MMG_Profile.h"

class MMG_IChatRoomsListener
{
public:
	virtual bool OnReceiveRoom(const MMG_Chat::ChatRoomName& theRoomName, const unsigned int theRoomId, const unsigned int theNumChatters, const bool aPasswordProtectedFlag) = 0;
	virtual void OnNoMoreRoomsAvailable() = 0;
};

class MMG_IChatRoomListener
{
public:
	enum JoinStatus { NO_STATUS, JOIN_OK, JOIN_DENIED_ROOM_FULL, JOIN_DENIED_WRONG_PASSWORD, JOIN_DENIED_CANNOT_JOIN};
	virtual void OnJoinRoom(const MMG_Chat::ChatRoomName& theRoomName, JoinStatus theStatus) = 0;
	virtual void OnLeaveRoom(const MMG_Chat::ChatRoomName& theRoomName) = 0;

	virtual bool ReceiveChatMessage(const MMG_Chat::ChatRoomName& theRoom, const MMG_Profile& theProfile, const MC_LocString& theMessage) = 0;
	virtual bool ReceiveChatUserLeave(const MMG_Chat::ChatRoomName& theRoom, const MMG_Profile& theProfile) = 0;
	virtual bool ReceiveChatUserInRoom(const MMG_Chat::ChatRoomName& theRoom, const MMG_Profile& theProfile) = 0;
};


#endif
