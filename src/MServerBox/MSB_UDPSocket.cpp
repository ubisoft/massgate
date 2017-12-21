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

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_UDPSocket.h"

#include "MSB_MemoryStream.h"
#include "MSB_ReadMessage.h"
#include "MSB_WriteBuffer.h"
#include "MSB_WriteMessage.h"

// #include "MC_Logger.h"

#define LOG_DEBUG3(...)

#define SIO_UDP_CONNRESET           _WSAIOW(IOC_VENDOR,12)

MSB_UDPSocket::MSB_UDPSocket(
	SOCKET				aSocket)
	: MSB_Socket_Win(aSocket)
	, myClients(8)
	, myIsClosing(false)
{
	DWORD nBytes=0;
	int32 setting = FALSE;
	int32 r = WSAIoctl( 
				GetSocket(), 
				SIO_UDP_CONNRESET, 
				&setting, 
				sizeof(setting), 
				NULL, 
				0, 
				&nBytes, 
				NULL, 
				NULL );    // succeeds, r==0

	if(r == SOCKET_ERROR) 
		MC_ERROR("Can't unset IO_UDP_CONNRESET, this is really bad.");
}

MSB_UDPSocket::~MSB_UDPSocket()
{
	LOG_DEBUG3("MSB_UDPSocket destroyed");
}

int32
MSB_UDPSocket::AddRemoteClient(
   MSB_UDPClientID				aRemoteID,
   MSB_UDPClientID				aLocalID,
	const struct sockaddr_in&	anAddr,
	MSB_MessageHandler*			anUDPClient)
{
	MT_MutexLock		lock(myLock);

	Remote remote;
	remote.myMessageHandler = anUDPClient;
	remote.myRemoteAddr = anAddr;
	remote.myRemoteClientID = aRemoteID;
	remote.myLocalClientId = aLocalID;

	myClients.Add(aRemoteID, remote);
	LOG_DEBUG3("Added UDP client 0x%08x", aRemoteID);

	MSB_WriteMessage writeMessage;
	bool res = anUDPClient->OnInit(writeMessage);
	if (res == false)
	{
		ProtSetError(MSB_ERROR_INVALID_DATA, -1);
		return -1;
	}

	if (writeMessage.IsEmpty() == false)
	{
		int err = Send(aRemoteID, writeMessage);
		if ( err != 0 )
			return err;
	}
	return 0;
}

bool
MSB_UDPSocket::HasRemoteClient(
	MSB_UDPClientID			aRemoteId)
{
	MT_MutexLock		lock(myLock);

	return myClients.HasKey(aRemoteId);
}


void
MSB_UDPSocket::RemoveRemoteClient(
	MSB_UDPClientID			aRemoteId)
{
	MT_MutexLock		lock(myLock);

	Remote remote;
	if ( myClients.Get(aRemoteId, remote) == false )
	{
		return; //Client already removed
	}

	myClients.Remove(aRemoteId);

	remote.myMessageHandler->OnClose();
	
	LOG_DEBUG3("Removed UDP client 0x%08x", aRemoteId);
}


int32
MSB_UDPSocket::Start()
{
	LOG_INFO("Started");
	return PrivRecvNext();
}

int32
MSB_UDPSocket::Send( 
	MSB_UDPClientID			aRemoteId, 
	MSB_WriteMessage&		aMessage )
{
	if(aMessage.IsEmpty() == false)
		return Send(aRemoteId, aMessage.GetHeadBuffer());

	return 0;
}

int32
MSB_UDPSocket::Send(
	 MSB_UDPClientID		aRemoteId, 
	 MSB_WriteBuffer*		aHeadBuffer)
{
	MT_MutexLock		lock(myLock);

	assert(aHeadBuffer != NULL);
	assert(ProtIsRegistered() == true);

	if(myIsClosing)
		return 0;

	Remote remote;

	if ( myClients.Get(aRemoteId, remote) == false)
	{
		MC_ERROR("UDP clientID=0x%08x does not exist.", aRemoteId);
		ProtSetError(MSB_NO_ERROR, -1); //Error recovery
		return -1; 
	}

	bool wasEmpty = myUDPWriteList.IsEmpty();

	myUDPWriteList.Add(remote.myLocalClientId, remote.myRemoteAddr, aHeadBuffer );

	//If the UDPWrite list (send queue) was empty, we need to start an iocp sender as well
	if ( wasEmpty )
	{
		int err = myUDPWriteList.Send( this );
		if ( err != 0 )
			ProtSetError(MSB_ERROR_SYSTEM, err);
		return err;
	}

	return 0;
}

void
MSB_UDPSocket::FlushAndClose()
{
	MT_MutexLock		lock(myLock);

	myIsClosing = true;
	if(myUDPWriteList.IsIdle())
		Close();
}

int32
MSB_UDPSocket::OnSocketWriteReady(
	int					aByteCount,
	OVERLAPPED*			anOverlapped)
{
	MT_MutexLock		lock(myLock);

	// The packet was successfully sent, remove it from in transit list
	myUDPWriteList.RemoveFromInTransit(anOverlapped, this);

	if(ProtIsClosed() == true)
		return 0;

	int32				error = myUDPWriteList.Send(this);

	if(myIsClosing && myUDPWriteList.IsIdle())
		Close();
	
	return 0;
}

int32
MSB_UDPSocket::OnSocketReadReady(
	int					aByteCount, 
	OVERLAPPED*			anOverlapped)
{
	Release();	//Last read operation has finished (or we have at least read its data from kernel)

	if ( aByteCount < sizeof(MSB_UDPClientID) + MSB_PACKET_HEADER_TOTSIZE + sizeof(MSB_DelimiterType) )
	{
		MC_ERROR("Too small UDP-packet was received. Only %d bytes.\n", aByteCount);
		ProtSetError(MSB_NO_ERROR, -1); //Error recovery
		return PrivRecvNext();
	}

	//We got an UDP message, parse it. Give it to the client and possibly send one message back
	MSB_UDPClientID clientID = MSB_SWAP_TO_BIG_ENDIAN( *(MSB_UDPClientID*) myReadBuffer );

	Remote remote;
	if ( myClients.Get(clientID, remote) == false )
	{
		MC_ERROR("Someone (%s) sent us an UDP packet with unknown client ID (0x%08x) in it.", 
				inet_ntoa(myLastAddr.sin_addr), clientID);
		return PrivRecvNext();
	}
	if ( memcmp(&myLastAddr.sin_addr, &remote.myRemoteAddr.sin_addr, sizeof(struct in_addr)) != 0 ) 
	{
		char invalidAddr[22];
		char shouldBeAddr[22];
		strncpy_s(invalidAddr, sizeof(invalidAddr), inet_ntoa(myLastAddr.sin_addr), _TRUNCATE); 
		strncpy_s(shouldBeAddr, sizeof(shouldBeAddr), inet_ntoa(remote.myRemoteAddr.sin_addr), _TRUNCATE); 
		MC_ERROR("Invalid remote address (%s:%d) for client ID (0x%08x). Should have been %s:ANY.", 
			invalidAddr, ntohs(myLastAddr.sin_port), clientID, shouldBeAddr);
		return PrivRecvNext();
	}

	//Set the port number from client if the port form the client was "ANY"
	if ( remote.myRemoteAddr.sin_port == 0 )
	{
		remote.myRemoteAddr.sin_port = myLastAddr.sin_port;
		myClients.Add(clientID, remote); //Overwrite old
	}

	MSB_MemoryStream		mstream(myReadBuffer + sizeof(MSB_UDPClientID), 
									aByteCount - sizeof(MSB_UDPClientID) );
	MSB_ReadMessage			message(&mstream);
	while(message.IsMessageComplete())	
	{		
		MSB_WriteMessage			response;
		if( message.DecodeHeader() == true && message.DecodeDelimiter() == true )
		{
			bool res;

			if(message.IsSystemMessage())
			{
				bool used = false;
				res = remote.myMessageHandler->OnSystemMessage(message, response, used);
				if (res == true && used == false)
					res = ProtHandleSystemMessage(message, response);

				if ( res == false )
				{
					SetError(MSB_ERROR_INVALID_SYSTEM_MSG, -2);
					//Report to handler
					remote.myMessageHandler->OnError(GetError());
					RemoveRemoteClient(clientID);
				}
			}
			else 
			{
				res = remote.myMessageHandler->OnMessage(message, response);
				if ( res == false )
				{
					// Even if OnMessage() returned false, do send (if any) the last response.
					// This is a feature and goes along with MSB_TCPSocket which also will send 
					// the last response and then call FlushAndClose()
					Send(clientID, response);
					RemoveRemoteClient(clientID);
					break;
				}
			}
		}

		//Send any response
		int err = Send(clientID, response);
		if ( err != 0 )
			return err;
	}

	return PrivRecvNext(); //Always start a new recv on the one and only UDP socket
}

void
MSB_UDPSocket::OnSocketError(
	MSB_Error		anError,
	int				aSystemError,
	OVERLAPPED*		anOverlapped)
{
	if (IsClosed() == false) 
		MC_ERROR("Error on UDP socket. Error=%d SystemError=%d", anError, aSystemError);

	//Ignore any error on UDP socket as we serve many clients on one socket.
}

void
MSB_UDPSocket::OnSocketClose()
{
	LOG_DEBUG3("UDPSocket closed");
}

bool
MSB_UDPSocket::ProtHandleSystemMessage(
	MSB_ReadMessage&		aReadMessage,
	MSB_WriteMessage&		aResponse)
{
	//We do not have any system messages yet so just return false, everything is invalid
	return false;
}


int32
MSB_UDPSocket::PrivRecvNext()
{
	ZeroMemory(&myOverlapped, sizeof(myOverlapped));
	myOverlapped.write = false;

	myWsaBuffer.buf = (char*) myReadBuffer;
	myWsaBuffer.len = sizeof(myReadBuffer);

	DWORD		bytesReceived;
	DWORD		options = 0;
	myLastAddrSize = sizeof(myLastAddr);
	int res = WSARecvFrom(GetSocket(), &myWsaBuffer, 1, &bytesReceived, &options, 
						  (struct sockaddr*) &myLastAddr, (LPINT) &myLastAddrSize,
						  (LPOVERLAPPED) &myOverlapped, NULL);

	if (res != 0 && WSAGetLastError() != ERROR_IO_PENDING)
	{
		int err = WSAGetLastError();
		MC_ERROR("Error from WSARecvFrom in MSB_UDPSocket. Error=%d", err);
		ProtSetError(MSB_ERROR_SYSTEM, err);
		return err;
	}
	

	Retain(); //We have successfully started a new read operation.

	return 0;
}

#endif // IS_PC_BUILD
