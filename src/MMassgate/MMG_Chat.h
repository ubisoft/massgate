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
#ifndef MMG_CHAT___HOHOHO___H_
#define MMG_CHAT___HOHOHO___H_

#include "MMG_AuthToken.h"
#include "MN_TcpClientConnectionFactory.h"
#include "MN_WriteMessage.h"
#include "MMG_IProfileListener.h"
#include "MMG_ProtocolDelimiters.h"

class MMG_IChatRoomListener;
class MMG_IChatRoomsListener;
class MMG_Messaging;

class MMG_Chat : private MMG_IProfileListener
{
public:
	typedef MC_StaticLocString<40> ChatRoomName;

	MMG_Chat(MN_WriteMessage& aWriteMessage, MMG_Messaging* aMessaging=NULL);

	static MMG_Chat* GetInstance();

	bool HandleMessage(MMG_ProtocolDelimiters::Delimiter aDelimiter, MN_ReadMessage& theStream); 

	void SendChat(const ChatRoomName& theRoom, const MC_LocChar* const theChatString);
	void JoinRoom(const ChatRoomName& theRoom, MMG_IChatRoomListener* aListener, const MC_LocChar* aPassword = NULL);
	void LeaveRoom(const ChatRoomName& theRoom);
	bool HasJoinedRoom(const ChatRoomName& theRoom);

	void CancelRoomListing();
	void GetRoomListing(MMG_IChatRoomsListener* aListener);

	void Update();

	~MMG_Chat();

	void UpdatedProfileInfo(const MMG_Profile& theProfile);

protected:
	class ChatRoom
	{
	public:

		ChatRoom(const ChatRoomName& theRoomName, MMG_IChatRoomListener* aListener, const MC_LocChar* aPassword);
		~ChatRoom();

		ChatRoomName	GetIdentifier();
		unsigned int	GetRoomId();

		bool			myHasJoined;

	protected:
		unsigned int	myRoomId;
		ChatRoomName	myIdentifier;
		MC_LocString	myPassword;
		MMG_IChatRoomListener* myListener;
	private:
		unsigned int	myJoinTime;
		friend class MMG_Chat;
	};

private:

	MMG_Messaging* PrivGetMessaging(); // so we can run this in Gatestone with unique messaging components

	bool PrivHandleRoomInfo(MN_ReadMessage& theRm);
	bool PrivHandleNoMoreRooms(MN_ReadMessage& theRm);
	bool PrivHandleJoinDenied(MN_DelimiterType errCode, MN_ReadMessage& theRm);
	bool PrivHandleJoinRoom(MN_ReadMessage& theRm);
	bool PrivHandleLeaveRoom(MN_ReadMessage& theRm);
	bool PrivHandleChatInRoom(MN_ReadMessage& theRm);
	bool PrivHandleUserInRoom(MN_ReadMessage& theRm); 
	bool PrivHandleUserLeft(MN_ReadMessage& theRm);

	void HandleDelayedItems();

	ChatRoom* PrivFindRoom(unsigned int theRoomId);
	ChatRoom* PrivFindRoom(const ChatRoomName& theName);

	MMG_IChatRoomsListener*		myRoomsListingListener;
	MC_GrowingArray<ChatRoom*>	myChatRooms;
	bool						myHasEverGottenResponseFromServer;
	MN_WriteMessage&			myOutgoingMessage;
	MMG_Messaging*				myGatestoneMessaging;


	struct DelayedItem
	{
		enum type_t { CHAT, USER_IN_ROOM, LEAVE };
		type_t type;
		MC_LocString	msg;
		unsigned int profileId;
		unsigned int roomId;
	};
	MC_HybridArray<DelayedItem, 64> myDelayedItems;
};

#endif
