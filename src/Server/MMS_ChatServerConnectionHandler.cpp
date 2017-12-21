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
#include "MMS_BanManager.h"
#include "MMS_ChatServerConnectionHandler.h"
#include "MC_Debug.h"
#include "mc_commandline.h"
#include "MT_ThreadingTools.h"
#include "mi_time.h"
#include "MMS_ServerStats.h"
#include "MMS_MasterServer.h"
#include "MMG_ServersAndPorts.h"
#include "MT_ThreadingTools.h"
#include "MMS_InitData.h"
#include "MMS_Statistics.h"
#include "MMG_ProtocolDelimiters.h"

#include "ML_Logger.h"

// Separate function so it's easier to see in the stacktrace
__declspec(noinline)
void ForceCrasch()
{
	volatile int* a = 0; 
	*a = 0;
}

// Separate function so it's easier to see in the stacktrace
__declspec(noinline)
void ForceAssert()
{
	assert(false && "forced assert");
}


MT_Mutex MMS_ChatServerConnectionHandler::myRoomsMutex;
MC_GrowingArray<MMS_ChatRoom*> MMS_ChatServerConnectionHandler::myChatRooms;

MMS_ChatServerConnectionHandler::MMS_ChatServerConnectionHandler(MMS_Settings& theSettings)
: mySettings(theSettings)
, myRecycledBroadcastMessage(MMS_IocpServer::MAX_BUFFER_LEN)
{
	myMulticastRecipients.Init(64,64,false);
	myRoomsMutex.Lock();
	if (!myChatRooms.IsInited())
		myChatRooms.Init(512,512);
	myRoomsMutex.Unlock();

	myWriteSqlConnection = new MDB_MySqlConnection(
		theSettings.WriteDbHost,
		theSettings.WriteDbUser,
		theSettings.WriteDbPassword,
		MMS_InitData::GetDatabaseName(),
		false);
	if (!myWriteSqlConnection->Connect())
	{
		LOG_ERROR("COULD NOT CONNECT TO DATABASE");
		assert(false);
	}
	
	myBanManager = MMS_BanManager::GetInstance();

}

DWORD 
MMS_ChatServerConnectionHandler::GetTimeoutTime()
{
	return 100; 
}

void
MMS_ChatServerConnectionHandler::OnReadyToStart()
{
	MT_ThreadingTools::SetCurrentThreadName("ChatConnectionHandler");
}

void 
MMS_ChatServerConnectionHandler::OnSocketConnected(MMS_IocpServer::SocketContext* aContext)
{
}

MMS_ChatServerConnectionHandler::~MMS_ChatServerConnectionHandler()
{
	myWriteSqlConnection->Disconnect();
	delete myWriteSqlConnection;
}



#define LOG_EXECUTION_TIME(MESSAGE, START_TIME) \
	{ unsigned int currentTime = GetTickCount(); \
	MMS_MasterServer::GetInstance()->AddSample("CHAT:" #MESSAGE, currentTime - START_TIME); \
	  START_TIME = GetTickCount(); } 


void 
MMS_ChatServerConnectionHandler::PrivUpdateStats()
{
	MMS_Statistics* stats = MMS_Statistics::GetInstance(); 
	stats->HandleMsgChat(); 
	stats->SQLQueryChat(myWriteSqlConnection->myNumExecutedQueries); 
	myWriteSqlConnection->myNumExecutedQueries = 0; 
}

bool 
MMS_ChatServerConnectionHandler::HandleMessage(MMG_ProtocolDelimiters::Delimiter theDelimeter, 
											   MN_ReadMessage& theIncomingMessage, 
											   MN_WriteMessage& theOutgoingMessage, 
											   MMS_IocpServer::SocketContext* thePeer)
{
	unsigned int startTime = GetTickCount(); 
	bool good = true;

	MMS_PerConnectionData* pcd = thePeer->GetUserdata();
	pcd->hasChatted = true; // When socket closed, we need to examine if he should leave rooms
	const MMG_AuthToken& authToken = pcd->authToken;

	switch (theDelimeter)
	{
	case MMG_ProtocolDelimiters::CHAT_LIST_ROOMS_REQ:
		good = good && PrivHandleListRooms(theIncomingMessage, theOutgoingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(CHAT_LIST_ROOMS_REQ, startTime); 
		break;
	case MMG_ProtocolDelimiters::CHAT_REAL_JOIN_REQ:
		good = good && PrivHandleJoinRoom(theIncomingMessage, theOutgoingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(CHAT_GET_ROOM_ID_REQ, startTime); 
		break;
	case MMG_ProtocolDelimiters::CHAT_LEAVE_ROOM_REQ:
		good = good && PrivHandleLeaveRoom(theIncomingMessage, theOutgoingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(CHAT_LEAVE_ROOM_REQ, startTime); 
		break;
	case MMG_ProtocolDelimiters::CHAT_SEND_MESSAGE_REQ:
		good = good && PrivHandleIncomingChat(theIncomingMessage, theOutgoingMessage, authToken, thePeer);
		LOG_EXECUTION_TIME(CHAT_SEND_MESSAGE_REQ, startTime); 
		break;
	default:
		LOG_ERROR("Unknown delimiter %u from client %s", (unsigned int)theDelimeter, thePeer->myPeerIpNumber);
		good = false;
	};

	PrivUpdateStats(); 
	PrivBroadcastTouchedRooms();


	return good;
}


bool
MMS_ChatServerConnectionHandler::PrivHandleJoinRoom(MN_ReadMessage& theRm, MN_WriteMessage& theWm, const MMG_AuthToken& theAuthToken, MMS_IocpServer::SocketContext* const thePeer)
{
	bool good = true;
	MMG_Chat::ChatRoomName roomName;
	MC_LocString password;

	good = good && theRm.ReadLocString(roomName.GetBuffer(), roomName.GetBufferSize());
	good = good && theRm.ReadLocString(password);

	if(!good)
	{
		LOG_ERROR("failed to parse data for chat room creation, disconnecting %s", thePeer->myPeerIpNumber); 
		return false; 
	}
	if(roomName.GetLength() > MMG_Chat::ChatRoomName::static_size - 8)
	{
		LOG_ERROR("user tried to create a chat room that is too long, UI shouldn't allow this, hacker? disconnecting %s", thePeer->myPeerIpNumber);
		return false; 
	}
	

	bool declineJoin = false;

	if (roomName.Find(L'\\') != -1)
	{
		// cannot have backslash in roomname as it will screw up the multicolumntextlist in game
		declineJoin = true;
	}
	else if (roomName.BeginsWith(L"_clanchat"))
	{
		declineJoin = true;
		ClientLutRef user = MMS_PersistenceCache::GetClientLut(myWriteSqlConnection, theAuthToken.profileId);
		if (user.IsGood())
		{
			MMG_Profile chatter;
			user->PopulateProfile(chatter);

			// Doublecheck that the caller really is part of that clan
			unsigned int clanId;
			if (1 == swscanf_s(roomName.GetBuffer(), L"_clanchat_%u", &clanId))
			{
				if (chatter.myClanId == clanId)
					declineJoin = false;
				else
					LOG_ERROR("Profile %u (clan %u) tried to join clanroom %u", theAuthToken.profileId, chatter.myClanId, clanId);	
			}
		}
	}
	else if (roomName.BeginsWith(L"_gang"))
	{
	}

	if (!good)
	{
		LOG_ERROR("Protocol error from ip %s", thePeer->myPeerIpNumber);
		return good;
	}

	if (declineJoin)
	{
		theWm.WriteDelimiter(MMG_ProtocolDelimiters::CHAT_DENIED_COULD_NOT_JOIN_ROOM);
		theWm.WriteLocString(roomName.GetBuffer());
		return true; // do not disconnect the user
	}

	//if (MMS_IsBannedName(roomName.GetBuffer()))
	if(MMS_BanManager::GetInstance()->IsNameBanned(roomName.GetBuffer()))
	{
		theWm.WriteDelimiter(MMG_ProtocolDelimiters::CHAT_DENIED_COULD_NOT_JOIN_ROOM);
		theWm.WriteLocString(roomName.GetBuffer());
		return true; // do not disconnect the user
	}

	MMS_ChatRoom* room = PrivGetOrCreateChatRoom(roomName, password, thePeer);
	if (room == NULL)
	{
		// The user requested to join a specific room, and it is full
		theWm.WriteDelimiter(MMG_ProtocolDelimiters::CHAT_DENIED_ROOM_FULL);
		theWm.WriteLocString(roomName.GetBuffer());
		return good;
	}

	if (!room->IsPasswordOk(password))
	{
		room->GetMutex().Unlock();
		// The user is probably already in the room
		theWm.WriteDelimiter(MMG_ProtocolDelimiters::CHAT_DENIED_ROOM_PASSWORD_WRONG);
		theWm.WriteLocString(roomName.GetBuffer());
		return true; // do NOT disconnect the user
	}
	else if (room->AddUser(theAuthToken.profileId, theWm, myWriteSqlConnection, thePeer))
	{
		theWm.WriteDelimiter(MMG_ProtocolDelimiters::CHAT_REAL_JOIN_RSP);
		theWm.WriteLocString(roomName.GetBuffer(), roomName.GetLength()); // Requested room
		theWm.WriteLocString(room->GetIdentifier().GetBuffer(), room->GetIdentifier().GetLength()); // Instanced name
		theWm.WriteUInt(room->GetRoomId());

		room->GetMutex().Unlock();
	}
	else
	{
		room->GetMutex().Unlock();
		// The user is probably already in the room
		theWm.WriteDelimiter(MMG_ProtocolDelimiters::CHAT_DENIED_ROOM_FULL);
		theWm.WriteLocString(roomName.GetBuffer());
		return true; // do NOT disconnect the user
	}
	return good;
}


bool
MMS_ChatServerConnectionHandler::PrivHandleLeaveRoom(
	MN_ReadMessage&			theRm,
	MN_WriteMessage&		theWm,
	const MMG_AuthToken&	theAuthToken,
	MMS_IocpServer::SocketContext* const	thePeer)
{
	bool good = true;
	unsigned int roomId;

	good = good && theRm.ReadUInt(roomId);

	if (!good)
	{
		LOG_ERROR("Protocol error from ip %s", thePeer->myPeerIpNumber);
		return false;
	}

	MMS_ChatRoom* room = PrivFindChatRoom(roomId);
	if (room == NULL)
	{
		LOG_INFO("Leave illegal room %u", int(roomId));
		return true;
	}
	
	room->RemoveUser(theAuthToken.profileId, thePeer);
	room->GetMutex().Unlock();

	theWm.WriteDelimiter(MMG_ProtocolDelimiters::CHAT_USER_LEFT);
	theWm.WriteUInt(room->GetRoomId());
	theWm.WriteUInt(theAuthToken.profileId);

	return good;
}

bool
MMS_ChatServerConnectionHandler::PrivHandleIncomingChat(MN_ReadMessage& theRm, MN_WriteMessage& theWm, const MMG_AuthToken& theAuthToken, MMS_IocpServer::SocketContext* const thePeer)
{
	bool good = true;

	unsigned int roomId;

	MC_StaticLocString<1024> chatString, lowerCaseChat;

	good = good && theRm.ReadUInt(roomId);
	good = good && theRm.ReadLocString(chatString.GetBuffer(), 256);

	if (!good)
		return false;

	chatString.TrimLeft().TrimRight();

	if (chatString.GetLength() == 0)
		return false; // Client should not be allowed to send empty chat messages

	// Easter eggs

	if (chatString == L"eeeeeeeeeeeeeeeeeeeeeee") // from MARKETING
	{
		chatString = L"Greetings, Count Neph von Pwnage of Castle Ownzor. Would you like to play a game of thermonuclear war?";
	}

	MC_StaticLocString<1024>	filteredString(chatString);
	myBanManager->FilterBannedWords(filteredString);

	/*
	const MC_LocChar* badWords[] = { 
			L"fuck", L"cunt", L"nigger", L"anus", L"company of heroes is better than wic by far.", 
			L"whore", L"cock", L"twat", L"pussy", L"asshole", 
			L"tubgirl.com", L"goatse.cx", L"coon", L"dick", L"bitch", 
			L"beatch", L"biatch", L"byatch", L"slut", L"fitta", L"sieg heil", L"heil hitler",
			NULL };
		bool foundBadWord;
		do 
		{
			lowerCaseChat = chatString;
			lowerCaseChat.MakeLower();
			foundBadWord = false;
			for (int i = 0; badWords[i] != NULL; i++)
			{
				const MC_LocChar* src = badWords[i];
				int index = lowerCaseChat.Find(src);
				if (index != -1)
				{
					foundBadWord = true;
					MC_LocChar* replace = chatString.GetBuffer() + index;
					while (*src++ && *replace)
						*replace++ = L'*';
				}
			}
		} while(foundBadWord);
		*/
	

	// Debugging tools follows
	if (chatString == L"/flush all caches")
	{
		chatString = L"--[Command accepted]--";
		MMS_PersistenceCache::FlushAllCaches();
	}

	if (good)
	{
		if (chatString[0] == '/')
			return good; 

		MMS_ChatRoom* room = PrivFindChatRoom(roomId);
		if (!room)
		{
			LOG_INFO("Chat illegal room");
			return false;
		}
		//room->HandleIncomingChatMessage(theAuthToken, chatString, thePeer);
		room->HandleIncomingChatMessage(theAuthToken, filteredString, thePeer);
		room->GetMutex().Unlock();
		MMS_MasterServer::GetInstance()->AddChatMessage(roomId, theAuthToken.profileId, chatString);
	}
	else
	{
		LOG_ERROR("Protocol error ip %s", thePeer->myPeerIpNumber);
	}
	return good;
}

bool 
MMS_ChatServerConnectionHandler::PrivHandleListRooms(MN_ReadMessage& theRm, MN_WriteMessage& theWm, const MMG_AuthToken& theAuthToken, MMS_IocpServer::SocketContext* const thePeer)
{
	bool good = true;
	const unsigned int currentTime = MI_Time::GetSystemTime();
	MT_MutexLock locker(myRoomsMutex);
	for (int i = 0; i < myChatRooms.Count(); i++)
	{
		myChatRooms[i]->GetMutex().Lock();
		// If the room has been empty for 5 minutes, delete it.
		if ((myChatRooms[i]->GetNumberOfChatters() == 0) && (myChatRooms[i]->GetTimeOfLastActivity() + 5*60*1000 < currentTime))
		{
			myChatRooms[i]->GetMutex().Unlock();
			myChatRooms.DeleteCyclicAtIndex(i--);
			continue;
		}

		// Skip 'non-public' rooms such as gang chat
		if (myChatRooms[i]->GetIdentifier()[0] != L'_')
		{
			theWm.WriteDelimiter(MMG_ProtocolDelimiters::CHAT_ROOM_INFO_RSP);
			theWm.WriteLocString(myChatRooms[i]->GetIdentifier().GetBuffer(), myChatRooms[i]->GetIdentifier().GetLength());
			theWm.WriteUInt(myChatRooms[i]->GetRoomId());
			theWm.WriteUChar(myChatRooms[i]->GetNumberOfChatters());
			theWm.WriteUChar(myChatRooms[i]->IsPasswordProtected());
		}
		myChatRooms[i]->GetMutex().Unlock();
	}
	theWm.WriteDelimiter(MMG_ProtocolDelimiters::CHAT_NO_MORE_ROOMS_IN_LIST);
	return good;
}


// ========= State consistency support functions ================

MMS_ChatRoom*
MMS_ChatServerConnectionHandler::PrivGetOrCreateChatRoom(const MMG_Chat::ChatRoomName& theIdentifier, const MC_LocString& aPassword, MMS_IocpServer::SocketContext* const thePeer)
{
	bool doesRequestSpecificRoom = theIdentifier.Find(L'#') != -1;
	MC_StaticLocString<512> requestRoomName = theIdentifier;

	MT_MutexLock locker(myRoomsMutex);
	// Linear search, we won't have that many chatrooms, and besides, the primary rooms are created first anyway so..
	for (int i = 0; i < myChatRooms.Count(); i++)
	{
		myChatRooms[i]->GetMutex().Lock();
		MC_StaticLocString<512> chatRoomName = myChatRooms[i]->GetIdentifier();
		if (!doesRequestSpecificRoom)
			chatRoomName = chatRoomName.Mid(0, chatRoomName.Find(L'#')-1);

		if (chatRoomName == requestRoomName)
		{
			if (myChatRooms[i]->GetNumberOfChatters() >= MMS_ChatRoom::MAX_CHATTERS_IN_ROOM)
			{
				if (doesRequestSpecificRoom)
				{
					myChatRooms[i]->GetMutex().Unlock();
					return NULL;
				}
				// Keep looking for room
			}
			else
			{
				return myChatRooms[i];
			}
		}
		myChatRooms[i]->GetMutex().Unlock();
	}
	// Chatroom not found. Create it.
	for (int i=1; i < 1000; i++)
	{
		MC_StaticLocString<512> roomName;
		roomName.Format(L"%.255s #%u", requestRoomName.GetBuffer(), i);
		// If the room name exists, try next
 		bool didRoomExist = false;
		for (int rn = 0; rn < myChatRooms.Count(); rn++)
		{
			if (roomName == myChatRooms[rn]->GetIdentifier().GetBuffer())
			{
				didRoomExist = true;
				break;
			}
		}
		if (!didRoomExist)
		{
			LOG_INFO("Creating room #%u", i);
			MMS_ChatRoom* room = new MMS_ChatRoom(this, roomName.GetBuffer(), aPassword);
			room->GetMutex().Lock();
			myChatRooms.Add(room);

			room->myRoomId = PrivLogChatRoomCreation(room, thePeer);

			if(0 == room->myRoomId)
			{
				LOG_ERROR("Failed to log chat room creation, database broken"); 
				return NULL; 
			}
			
			return room;
		}
	}
	return NULL;
}

unsigned int
MMS_ChatServerConnectionHandler::PrivLogChatRoomCreation(MMS_ChatRoom* aRoom, MMS_IocpServer::SocketContext* const thePeer)
{
	MC_StaticString<1024> escapedRoomName, escapedPassword; 
	myWriteSqlConnection->MakeSqlString(escapedRoomName, aRoom->GetIdentifier().GetBuffer()); 
	myWriteSqlConnection->MakeSqlString(escapedPassword, aRoom->myPassword.GetBuffer());
	const MMG_AuthToken& authToken = thePeer->GetUserdata()->authToken;

	MC_StaticString<1024> sql; 
	sql.Format("INSERT INTO Log_ChatRoomCreation (name, password, createdByProfileId) VALUES('%s', '%s', %u)", escapedRoomName.GetBuffer(), escapedPassword.GetBuffer(), authToken.profileId); 

	MDB_MySqlQuery query(*myWriteSqlConnection);
	MDB_MySqlResult result; 

	if(!query.Modify(result, sql))
		return 0; 
	return (unsigned int)query.GetLastInsertId(); 
}

MMS_ChatRoom*
MMS_ChatServerConnectionHandler::PrivFindChatRoom(unsigned int theIdentifier)
{
	MT_MutexLock locker(myRoomsMutex);
	const unsigned int count = myChatRooms.Count();
	for (unsigned int i = 0; i < count; i++)
	{
		if (myChatRooms[i]->GetRoomId() == theIdentifier)
		{
			myChatRooms[i]->GetMutex().Lock();
			return myChatRooms[i];
		}
	}
	return NULL;
}

void
MMS_ChatServerConnectionHandler::OnSocketClosed(MMS_IocpServer::SocketContext* aContext)
{
	MMS_PerConnectionData* pcd = aContext->GetUserdata();

	if (!pcd)
		return;

	if(!pcd->hasChatted)
		return; 

	MT_MutexLock locker(myRoomsMutex);

	const unsigned int count = myChatRooms.Count();
	for (unsigned int i = 0; i < count; i++)
		myChatRooms[i]->RemoveUser(pcd->authToken.profileId, aContext);

	pcd->hasChatted = false;
}

static MT_SkipLock ourSkipLock;
static MC_EggClockTimer ourEggClock(100);

void 
MMS_ChatServerConnectionHandler::OnIdleTimeout()
{
	PrivBroadcastTouchedRooms();
}

void 
MMS_ChatServerConnectionHandler::PrivBroadcastTouchedRooms()
{
	unsigned int startTime = GetTickCount(); 

	if (ourSkipLock.TryLock())
	{
		if (ourEggClock.HasExpired())
		{
			MT_MutexLock locker(myRoomsMutex);

			for (int i = 0; i < myChatRooms.Count(); i++)
			{
				myChatRooms[i]->GetMutex().Lock();
				myMulticastRecipients.RemoveAll();
				myRecycledBroadcastMessage.Clear();
				if (myChatRooms[i]->ConsumeBroadcastCache(myRecycledBroadcastMessage, myMulticastRecipients))
				{
					myChatRooms[i]->GetMutex().Unlock();
					myWorkerThread->MulticastMessage(myRecycledBroadcastMessage, myMulticastRecipients);
					continue; // So we don't unlock twice
				}
				myChatRooms[i]->GetMutex().Unlock();
			}
		}
		ourSkipLock.Unlock();
	}

	LOG_EXECUTION_TIME(MMS_ChatServerConnectionHandler::PrivBroadcastTouchedRooms(), startTime); 
}

