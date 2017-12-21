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
#include "StdAfx.h"
#include "MMG_MasterConnection.h"
#include "MMG_ServersAndPorts.h"
#include "MN_Resolver.h"
#include "MN_TcpConnection.h"
#include "MMG_ProtocolDelimiters.h"

#include <time.h>

static MMG_MasterConnection* ourInstance = NULL; 

MMG_MasterConnection::MMG_MasterConnection()
: myState(DO_NOTHING)
, myConnection(NULL)
, myAccountProxy(NULL)
, myMessaging(NULL)
, myServerTracker(NULL)
, myChat(NULL)
, myTrackableServer(NULL)
, myLastMessageTime(0)
, myWriteMessage(4096)
, myReadMessage(4096)
{
	SOCKADDR_IN temp; 
	MN_Resolver::ResolveName(MASSGATE_SERVER_NAME, ServersAndPorts::GetServerPort(), temp);
	MN_Resolver::ResolveName(MASSGATE_SERVER_NAME_BACKUP, ServersAndPorts::GetServerPort(), temp);
}

MMG_MasterConnection::~MMG_MasterConnection()
{
	Disconnect();
}

void 
MMG_MasterConnection::SetAccountProxy(MMG_AccountProxy* anAccountProxy)
{
	myAccountProxy = anAccountProxy; 
}

void 
MMG_MasterConnection::SetMessaging(MMG_Messaging* aMessaging)
{
	myMessaging = aMessaging; 
}

void 
MMG_MasterConnection::SetServerTracker(MMG_ServerTracker* aServerTracker)
{
	myServerTracker = aServerTracker; 
}

void 
MMG_MasterConnection::SetChat(MMG_Chat* aChat)
{
	myChat = aChat; 
}

void 
MMG_MasterConnection::SetTrackableserver(MMG_TrackableServer* aTrackableServer)
{
	myTrackableServer = aTrackableServer; 
}

MMG_MasterConnection* 
MMG_MasterConnection::GetInstance2()
{
	return ourInstance; 
}

MMG_MasterConnection* 
MMG_MasterConnection::Create()
{
	if(!ourInstance)
		ourInstance = new MMG_MasterConnection(); 
	return ourInstance; 
}

void 
MMG_MasterConnection::Destroy()
{
	if(ourInstance)
		delete ourInstance; 
	ourInstance = NULL; 
}

void 
MMG_MasterConnection::Connect()
{
	myState = START_CONNECTING;
	myLastMessageTime = 0;
}

void 
MMG_MasterConnection::Disconnect()
{
	if(myConnection)
	{
		myLastMessageTime = 0;
		myConnection->Close(); 
		delete myConnection; 
		myConnection = NULL; 
	}
	myState = DISCONNECTED;
}

time_t
MMG_MasterConnection::GetLastMessageTime() const
{
	return myLastMessageTime;
}

MMG_MasterConnection::State 
MMG_MasterConnection::GetState()
{
	return myState; 
}

bool 
MMG_MasterConnection::IsConnected() const
{
	return myState == CONNECTED;
}

const bool 
MMG_MasterConnection::GetLocalAddress(SOCKADDR_IN& anAddress)
{
	assert(myConnection && "operating on NULL connection"); 
	return myConnection->GetLocalAddress(anAddress); 
}

bool 
MMG_MasterConnection::PrivSetupConnection()
{
	myState = CONNECTING; 
	return myConnectionFactory.CreateConnection(MASSGATE_SERVER_NAME, ServersAndPorts::GetServerPort());
}

bool 
MMG_MasterConnection::PrivSetupConnectionBackup()
{
	myState = CONNECTING_BACKUP_SERVER;
	return myConnectionFactory.CreateConnection(MASSGATE_SERVER_NAME_BACKUP, ServersAndPorts::GetServerPort());
}

bool 
MMG_MasterConnection::PrivWaitForConnection()
{
	if (myConnectionFactory.IsAttemptingConnection())
	{
		if (myConnectionFactory.AwaitConnection(&myConnection))
		{
			myState = CONNECTED;
			myLastMessageTime = time(NULL);
		}
		else if (myConnectionFactory.DidConnectFail())
		{
			if(myState == CONNECTING)
			{
				myState = START_CONNECTING_BACKUP_SERVER; 
				return true; 
			}
			else 
			{
				MC_DEBUG("Could not connect.");
				return false;
			}
		}
	}
	else 
	{
		MC_DEBUG("could not connect");
		return false; 
	}

	return true; 
}

bool 
MMG_MasterConnection::PrivSendReceiveData()
{
	// send data 
	if (myWriteMessage.GetMessageSize())
	{
		if (myWriteMessage.SendMe(myConnection) == MN_CONN_OK)
		{
			myLastMessageTime = time(NULL);
			myWriteMessage.Clear();
		}
		else
		{
			MC_DEBUG("Connection error talking to server.");
			return false; 
		}
	}
	
	bool good = true; 

	// receive data 
	MN_ConnectionErrorType status = MN_CONN_BROKEN;
	while (good && ((status = myConnection->Receive()) == MN_CONN_OK))
	{
		if(myConnection->GetDataLength() < 2)
			continue; // MN_CONN_NO_DATA will break loop

		unsigned short packetLen = ((*((unsigned short*)myConnection->GetData())) & 0x3fff) + sizeof(unsigned short);
		if (packetLen <= myConnection->GetDataLength())
		{
			MN_ReadMessage rm(4096);
			unsigned int numBytes = rm.BuildMe((const unsigned char*)myConnection->GetData(), packetLen);
			if (numBytes > 0)
			{
				MMG_ProtocolDelimiters::Delimiter delim;

				while (good && !rm.AtEnd())
				{
					MN_DelimiterType delimRaw;
					if(!rm.ReadDelimiter(delimRaw))
					{
						MC_DEBUG("couldn't read delimiter"); 
						return false; 
					}
					delim = MMG_ProtocolDelimiters::Delimiter(delimRaw);

					if(delim > MMG_ProtocolDelimiters::ACCOUNT_START && delim < MMG_ProtocolDelimiters::ACCOUNT_END)
					{
						if(myAccountProxy)
							good = good && myAccountProxy->HandleMessage(delim, rm); 
						else 
							MC_DEBUG("received data for non existing account proxy"); 
					}
					else if(delim > MMG_ProtocolDelimiters::MESSAGING_START && delim < MMG_ProtocolDelimiters::MESSAGING_END)
					{
						if(myMessaging)
							good = good && myMessaging->HandleMessage(delim, rm); 	
						else 
							MC_DEBUG("received data for non existing messaging server"); 
					}
					else if(delim > MMG_ProtocolDelimiters::MESSAGING_DS_START && delim < MMG_ProtocolDelimiters::MESSAGING_DS_END)
					{
						if(myTrackableServer)
							good = good && myTrackableServer->HandleMessage(delim, rm);
						else 
							MC_DEBUG("received data for non existing trackable server"); 
					}
					else if(delim >	MMG_ProtocolDelimiters::SERVERTRACKER_USER_START && delim < MMG_ProtocolDelimiters::SERVERTRACKER_USER_END)
					{
						if(myServerTracker)
							good = good && myServerTracker->HandleMessage(delim, rm); 
						else 
							MC_DEBUG("received data for non existing server tracker"); 
					}
					else if(delim > MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_START && delim < MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_END)
					{
						if(myTrackableServer)
							good = good && myTrackableServer->HandleMessage(delim, rm);
						else 
							MC_DEBUG("received data for non existing trackable server"); 
					}
					else if(delim > MMG_ProtocolDelimiters::CHAT_START && delim < MMG_ProtocolDelimiters::CHAT_END)
					{
						if(myChat)
							good = good && myChat->HandleMessage(delim, rm); 
						else 
							MC_DEBUG("received data for non existing chat"); 
					}
					else if(delim > MMG_ProtocolDelimiters::NATNEG_START && delim < MMG_ProtocolDelimiters::NATNEG_END)
					{
						MC_DEBUG("nat neg delimiters in master connection, should be handled by it's own connection"); 
						return false; 
					}
					else 
					{
						MC_DEBUG("delimiter out of range, %u", delim);
						return false; 
					}
				}

				myConnection->UseData(numBytes); 
			}
			else 
			{
				MC_DEBUG("couldn't build message");
				return false; 
			}
		}
	}

	if(status == MN_CONN_BROKEN)
	{
		MC_DEBUG("connection to massgate broken"); 
		return false; 
	}

	return good; 
}

bool 
MMG_MasterConnection::Update()
{
	if (myAccountProxy)
		myAccountProxy->Update(); // Renew lease if needed.
	if (myMessaging)
		myMessaging->Update();
	if (myServerTracker)
		myServerTracker->Update();
	if (myChat)
		myChat->Update();

	switch(myState)
	{
	case DO_NOTHING:
		return true; 
	case START_CONNECTING:
		return PrivSetupConnection();
	case START_CONNECTING_BACKUP_SERVER:
		return PrivSetupConnectionBackup();
	case CONNECTING:
	case CONNECTING_BACKUP_SERVER:
		return PrivWaitForConnection();
	case CONNECTED:
		return PrivSendReceiveData(); 
	case DISCONNECTED:
		return true; 
	default:
		MC_DEBUG("Unknown state");
		assert(false && "implementation error");
		return false; 
	}
}

MN_WriteMessage& 
MMG_MasterConnection::GetWriteMessage()
{
	return myWriteMessage; 
}

