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
#ifndef MMG_MASTERCONNECTION_H
#define MMG_MASTERCONNECTION_H

#include "MN_WriteMessage.h"
#include "MN_ReadMessage.h"
#include "MN_TcpClientConnectionFactory.h"
#include "MMG_AccountProxy.h"
#include "MMG_Messaging.h"
#include "MMG_ServerTracker.h"
#include "MMG_Chat.h"

class MMG_MasterConnection
{
public: 
	typedef enum 
	{
		DO_NOTHING, 
		START_CONNECTING, 
		START_CONNECTING_BACKUP_SERVER, 
		CONNECTING, 
		CONNECTING_BACKUP_SERVER,
		CONNECTED, 
		DISCONNECTED
	} State;

	MMG_MasterConnection(); 
	~MMG_MasterConnection(); 

	static MMG_MasterConnection* GetInstance2(); 
	static MMG_MasterConnection* Create(); 
	static void Destroy(); 

	bool Update(); 
	MN_WriteMessage& GetWriteMessage(); 

	void Connect();
	void Disconnect(); 

	State GetState(); 
	bool IsConnected() const;

	time_t GetLastMessageTime() const;

	const bool GetLocalAddress(SOCKADDR_IN& anAddress); 

	void SetAccountProxy(MMG_AccountProxy* anAccountProxy);
	void SetMessaging(MMG_Messaging* aMessaging);
	void SetServerTracker(MMG_ServerTracker* aServerTracker);
	void SetChat(MMG_Chat* aChat);
	void SetTrackableserver(MMG_TrackableServer* aTrackableServer);

private: 

	bool PrivSetupConnection(); 
	bool PrivSetupConnectionBackup(); 
	bool PrivWaitForConnection();
	bool PrivSendReceiveData();

	State myState; 

	MN_WriteMessage myWriteMessage;
	MN_ReadMessage myReadMessage; 
	MN_TcpClientConnectionFactory myConnectionFactory; 
	MN_Connection* myConnection; 

	MMG_AccountProxy* myAccountProxy;
	MMG_Messaging* myMessaging;
	MMG_ServerTracker* myServerTracker;
	MMG_Chat* myChat;
	MMG_TrackableServer* myTrackableServer; 
	time_t myLastMessageTime;
};

#endif
