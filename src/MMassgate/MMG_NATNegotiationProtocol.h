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
#ifndef MMG_NATNEGOTIATIONPROTOCOL____H_
#define MMG_NATNEGOTIATIONPROTOCOL____H_

#include "MN_ReadMessage.h"
#include "MN_WriteMessage.h"
#include "MMG_AuthToken.h"
#include "MC_Debug.h"
#include "MN_TcpConnection.h"
#include "MMG_Constants.h"

#ifdef DEBUG
#define SET_HUMAN_READABLE() ToString(myHumanReadable)
#else
#define SET_HUMAN_READABLE() ((void)0)
#endif

class MMG_NATNegotiationProtocol
{
public: 

	// the size of the data buffers sent back and forth, don't 
	// change this value unless you know what you are doing 
	static const unsigned int PEER_DATA_BUFFER_SIZE = MMG_MAX_BUFFER_SIZE; 

	class Endpoint 
	{
	public: 
		Endpoint() 
		{
			myAddr = 0; 
			myPort = 0; 
			SET_HUMAN_READABLE(); 
		}
		Endpoint(const unsigned int anAddr, 
				 const unsigned short aPort)
		{
			myAddr = anAddr; 
			myPort = aPort; 
			SET_HUMAN_READABLE(); 
		}
		Endpoint(const Endpoint& anEndpoint)
		{
			myAddr = anEndpoint.myAddr; 
			myPort = anEndpoint.myPort; 
			SET_HUMAN_READABLE(); 
		}
		Endpoint(const SOCKADDR_IN& aSockaddr)
		{
			myAddr = aSockaddr.sin_addr.s_addr; 
			myPort = aSockaddr.sin_port; 
			SET_HUMAN_READABLE(); 
		}

		~Endpoint() { }

		void Init()
		{
			myAddr = 0; 
			myPort = 0; 
			SET_HUMAN_READABLE(); 
		}
		void Init(const unsigned int anAddr, 
				  const unsigned short aPort)
		{
			myAddr = anAddr; 
			myPort = aPort; 
			SET_HUMAN_READABLE(); 
		}
		void Init(const Endpoint& anEndpoint)
		{
			myAddr = anEndpoint.myAddr; 
			myPort = anEndpoint.myPort; 
			SET_HUMAN_READABLE(); 
		}
		void Init(const SOCKADDR_IN& aSockaddr)
		{
			myAddr = aSockaddr.sin_addr.s_addr; 
			myPort = aSockaddr.sin_port; 
			SET_HUMAN_READABLE(); 
		}
		bool IsUsable()
		{
			return (myAddr != 0) && (myPort != 0); 
		}
		bool Equals(const Endpoint& anEndpoint)
		{
			return (myAddr == anEndpoint.myAddr) && (myPort == anEndpoint.myPort); 
		}
		bool Equals(const SOCKADDR_IN& aSockaddr)
		{
			return (myAddr == aSockaddr.sin_addr.s_addr) && (myPort == aSockaddr.sin_port); 
		}
		void Print(const char* aTag = "")
		{
			MC_DEBUG("%s Address: %d.%d.%d.%d:%d", aTag, 
				myAddr & 0xff, (myAddr >> 8) & 0xff, (myAddr >> 16) & 0xff, (myAddr >> 24) & 0xff, htons(myPort)); 
		}
		void ToString(char *aBuffer)
		{
			sprintf(aBuffer, "%d.%d.%d.%d:%d", 
				myAddr & 0xff, (myAddr >> 8) & 0xff, (myAddr >> 16) & 0xff, (myAddr >> 24) & 0xff, htons(myPort)); 
		}

		unsigned int myAddr; 
		unsigned short myPort; 
#ifdef DEBUG
		char myHumanReadable[24];
#endif
	};

/*
	typedef enum NATNegotiationCommands
	{
		// Client to server				Server to client
		UNDEFINED,						UNDEFINED2,
		PROTOCOL_ERROR,					REQUEST_DENIED,
		
		// commands for mediating has to be in MMG_Messaging ...  

		// relaying server 
		NATNEG_CLIENT_TO_RELAYING_REQUEST_REGISTER,				
		NATNEG_RELAYING_TO_SERVER_RESPONSE_REGISTER, 
		NATNEG_CLIENT_TO_RELAYING_REQUEST_CONNECT_TO_PEER,		
		NATNEG_RELAYING_TO_CLIENT_RESPONSE_CONNECT_TO_PEER, 

		// between peers 
		NATNEG_COOKIE, 
		NATNEG_PING, 
		NATNEG_PONG, 

		// betwwen peers and to relaying server 
		NATNEG_DATA, 

		// External sanity ping 
		SANITY_PING, 

		// Protocol place holders 
		RESERVED1,						RESERVED2,
		RESERVED3,						RESERVED4,
		RESERVED5,						RESERVED6,
		RESERVED7,						RESERVED8,
		RESERVED9,						RESERVED10
	};

*/
	// Mediating 

	class ClientToMediatingRequestConnect
	{
	public: 
		ClientToMediatingRequestConnect(); 

		void ToStream(MN_WriteMessage& aWm); 
		bool FromStream(MN_ReadMessage& aRm); 
		bool IsSane(); 

		unsigned int myRequestId; 
		unsigned int myPassiveProfileId; 
		Endpoint myPrivateEndpoint; 
		Endpoint myPublicEndpoint; 
		unsigned char myUseRelayingServer; 
		unsigned int myReservedDummy; 
	};

	class MediatingToClientRequestConnect
	{
	public: 
		MediatingToClientRequestConnect(); 

		void ToStream(MN_WriteMessage& aWm); 
		bool FromStream(MN_ReadMessage& aRm); 
		bool IsSane(); 

		unsigned int myRequestId; 
		unsigned int myActiveProfileId; 
		unsigned int mySessionCookie; 
		Endpoint myActivePeersPrivateEndpoint; 
		Endpoint myActivePeersPublicEndpoint; 
		unsigned char myUseRelayingServer; 
		unsigned int myReservedDummy; 
	};

	class ClientToMediatingResponseConnect
	{
	public: 
		ClientToMediatingResponseConnect(); 

		void ToStream(MN_WriteMessage& aWm); 
		bool FromStream(MN_ReadMessage& aRm); 
		bool IsSane(); 

		unsigned int myRequestId; 
		unsigned int myActiveProfileId; 
		unsigned char myAcceptRequest;
		unsigned int mySessionCookie; 
		Endpoint myPrivateEndpoint; 
		Endpoint myPublicEndpoint; 
		unsigned char myUseRelayingServer; 
		unsigned int myReservedDummy; 
	};

	class MediatingToClientResponseConnect
	{
	public:
		MediatingToClientResponseConnect(); 

		void ToStream(MN_WriteMessage& aWm); 
		bool FromStream(MN_ReadMessage& aRm); 
		bool IsSane(); 

		unsigned int myRequestId; 
		unsigned int myPassiveProfileId; 
		unsigned char myAcceptRequest; 
		unsigned int mySessionCookie; 
		Endpoint myPassivePeersPrivateEndpoint; 
		Endpoint myPassivePeersPublicEndpoint; 
		unsigned char myUseRelayingServer; 
		unsigned int myReservedDummy; 
	}; 

	// RELAYING 

	class ClientToRelayingRequestRegister
	{
	public: 
		ClientToRelayingRequestRegister(); 

		void ToStream(MN_WriteMessage& aWm); 
		bool FromStream(MN_ReadMessage& aRm); 
		bool IsSane(); 

		unsigned int myRequestId; 
		MMG_AuthToken myAuthToken; 
		unsigned int myReservedDummy; 
	};

	class RelayingToClientResponseRegister
	{
	public: 
		RelayingToClientResponseRegister(); 

		void ToStream(MN_WriteMessage& aWm); 
		bool FromStream(MN_ReadMessage& aRm); 
		bool IsSane(); 

		unsigned int myRequestId; 
		Endpoint myPublicEndpoint; 
		unsigned int myReservedDummy; 
	};

	class ClientToRelayingRequestConnectToPeer
	{
	public: 
		ClientToRelayingRequestConnectToPeer(); 

		void ToStream(MN_WriteMessage& aWm); 
		bool FromStream(MN_ReadMessage& aRm); 
		bool IsSane(); 

		unsigned int myRequestId; 
		unsigned int mySessionCookie; 
		unsigned int myProfileId; 
		unsigned int myPeersProfileId; 
		unsigned int myReservedDummy; 
	};

	class RelayingToClientResponseConnectToPeer
	{
	public: 
		RelayingToClientResponseConnectToPeer(); 

		void ToStream(MN_WriteMessage& aWm); 
		bool FromStream(MN_ReadMessage& aRm); 
		bool IsSane(); 

		unsigned int myRequestId; 
		unsigned int myRequestOk; 
		unsigned int myReservedDummy; 
	};

	class Cookie
	{
	public:
		Cookie(); 

		void ToStream(MN_WriteMessage& aWm); 
		bool FromStream(MN_ReadMessage& aRm); 
		bool IsSane(); 

		unsigned int mySessionCookie; 
		unsigned int myReservedDummy; 
	};

	class Ping
	{
	public:
		Ping(); 

		void ToStream(MN_WriteMessage& aWm); 
		bool FromStream(MN_ReadMessage& aRm); 
		bool IsSane(); 

		unsigned int myReservedDummy; 
	}; 

	class Pong
	{
	public:
		Pong(); 

		void ToStream(MN_WriteMessage& aWm); 
		bool FromStream(MN_ReadMessage& aRm); 
		bool IsSane(); 

		unsigned int myReservedDummy; 
	}; 

	class Data
	{
	public:
		Data(); 

		void ToStream(MN_WriteMessage& aWm); 
		bool FromStream(MN_ReadMessage& aRm); 
		bool IsSane(); 

		void *myData; 
		int myDataLength; 
		unsigned int myReservedDummy; 
	};
};

#endif