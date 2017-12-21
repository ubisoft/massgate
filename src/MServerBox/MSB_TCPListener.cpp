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
#include <stdafx.h>

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_TCPListener.h"

MSB_TCPListener::MSB_TCPListener( 
	MSB_Port			aPort, 
	int					aBacklog)
	: MSB_Socket_Win(INVALID_SOCKET, true)
	, myAcceptEx(NULL)
	, myGetAcceptExSockaddrs(NULL)
{
	 MSB_PortRange		portRange(aPort, aPort);
	 PrivCommonConstructor(portRange, aBacklog);
}

MSB_TCPListener::MSB_TCPListener(
	MSB_PortRange&		aPortRange,
	int					aBacklog)
	 : MSB_Socket_Win(INVALID_SOCKET)
	 , myAcceptEx(NULL)
	 , myGetAcceptExSockaddrs(NULL)
{
	PrivCommonConstructor(aPortRange, aBacklog);
}

int32
MSB_TCPListener::PrivCommonConstructor(
	MSB_PortRange&		aPortRange,
	int					aBacklog)
{
	int32 res = PrivInit(aPortRange, aBacklog);
	if (res != 0)
	{
		//reset any half done init
		myListenPort = 0; 
		SetSocket(INVALID_SOCKET);
	}
	return res;
}


int32
MSB_TCPListener::PrivInit(
	 MSB_PortRange&		aPortRange,
	 int				aBacklog)
{
	//Create the TCP listen socket
	int32 res = ProtCreateSocket(); 
	if (res != 0)
		return res;

	//Loop until we find a free port in our PortRange
	res = -1;
	while (res != 0 && aPortRange.HasNext())
	{
		MSB_Port port = aPortRange.GetNext();
		res = ProtTryBind(port);
	}
	if (res != 0)
	{
		int err = WSAGetLastError();
		LOG_ERROR("Failed to bind() in MSB_TCPListener. Error=%d", err);
		SetError(MSB_ERROR_SYSTEM, err);
		return err;
	}

	//Listen to the found TCP port
	return ProtListen(aBacklog);
}

MSB_TCPListener::~MSB_TCPListener()
{
}

int32
MSB_TCPListener::ProtCreateSocket()
{
	SOCKET listenerSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if(listenerSocket == INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		LOG_ERROR("Failed to create listen socket; error=%d", err);
		SetError(MSB_ERROR_SYSTEM, err);
		return err;
	}
	SetSocket(listenerSocket);
	return 0;
}

int32
MSB_TCPListener::ProtTryBind(
	MSB_Port			aPort)
{
	struct sockaddr_in		address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;

	myListenPort = aPort;
	address.sin_port = htons(myListenPort);

	return bind(GetSocket(), (struct sockaddr*) &address, sizeof(address));
}


int32
MSB_TCPListener::ProtListen(
	int					aBacklog)
{
	if (listen(GetSocket(), aBacklog) == 0)
	{
		PrivLoadAcceptEx();
		PrivLoadGetAcceptExSockaddrs();
		return 0;
	}
	
	int err = WSAGetLastError();
	LOG_ERROR("Faild to put listen socket in listen-mode; error=%d", err);
	SetError(MSB_ERROR_SYSTEM, err);

	return err;
}

int32
MSB_TCPListener::Start()
{
	return PrivStartNextAccept(); 
}

int32
MSB_TCPListener::OnSocketWriteReady(
	int					aByteCount, 
	OVERLAPPED*			anOverlapped)
{
	return 0;
}

int32
MSB_TCPListener::OnSocketReadReady(
	int					aByteCount,
	OVERLAPPED*			anOverlapped)
{
	Release();

	if ( IsClosed() )
	{
		//An operation completed but the socket has been closed. 
		// Typically happens when issuing Connect() and then Close() directly afterwards
		return 0; //This is not an error. Just a late operation.
	}

	struct sockaddr_in*	localAddress;
	struct sockaddr_in*	remoteAddress;
	int					localLength;
	int					remoteLength;
	myGetAcceptExSockaddrs(
		myDummyBuffer,
		aByteCount,
		sizeof(struct sockaddr_in) + 16,
		sizeof(struct sockaddr_in) + 16,
		(LPSOCKADDR*) &localAddress,
		&localLength,
		(LPSOCKADDR*) &remoteAddress,
		&remoteLength);

	//Free/inherit it self from the listen socket context
	SOCKET asock = GetSocket();
	int res = setsockopt( myCurrentAcceptSocket, 
				 SOL_SOCKET, 
				 SO_UPDATE_ACCEPT_CONTEXT, 
				 (char *) &asock, 
				 sizeof(asock) );
	if (res == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		LOG_ERROR("Can't setsockopt() SO_UPDATE_ACCEPT_CONTEXT on newly accepted socket (%d).", err);
		SetError(MSB_ERROR_SYSTEM, err);
		return err;
	}

	LOG_INFO("Accepted connection");

	res = 0;
	if((res = OnAccepted(myCurrentAcceptSocket, *localAddress, *remoteAddress)) != 0)
		return res;

	return PrivStartNextAccept();
}

void
MSB_TCPListener::OnSocketError(
	MSB_Error			anError,
	int					aSystemError,
	OVERLAPPED*			anOverlapped)
{
	LOG_ERROR("MSB_TCPListener::OnSocketError() Error=%d SystemError=%d", anError, aSystemError);
	PrivStartNextAccept(); //ignore the error, and continue
}

void
MSB_TCPListener::OnSocketClose()
{
	LOG_INFO("TCPListener socket closed");
}

/**
 * Accepts next socket async.
 */
int32
MSB_TCPListener::PrivStartNextAccept()
{
	if(myPreallocatedSockets.Count() == 0)
	{
		if ( PrivPreallocateSockets() == false)
			return GetSystemError();
	}

	myCurrentAcceptSocket = myPreallocatedSockets[0];
	myPreallocatedSockets.RemoveCyclicAtIndex(0);

	myBytesReceived = 0;

	ZeroMemory(&myOverlapped, sizeof(myOverlapped));
	myOverlapped.write = false;

	int			ok = myAcceptEx(
						GetSocket(), 
						myCurrentAcceptSocket, 
						myDummyBuffer, 
						0,
						sizeof(struct sockaddr_in) + 16,
						sizeof(struct sockaddr_in) + 16,
						&myBytesReceived, 
						(LPOVERLAPPED) &myOverlapped);

	int err = WSAGetLastError();
	if(!ok)
	{
		if (err != ERROR_IO_PENDING)
		{
			LOG_ERROR("Failed to accept new socket; error=%d", err);
			SetError(MSB_ERROR_SYSTEM, err);
			return err;
		}
	}
	Retain();

	return 0;
}

bool
MSB_TCPListener::PrivPreallocateSockets()
{
	//ONLY run this when out of sockets
	LOG_INFO("Preallocating more sockets");

	for (int i = 0; i < 100; i++)
	{
		SOCKET		sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (sock == INVALID_SOCKET)
		{
			int err = WSAGetLastError();
			LOG_ERROR("Failed to allocate socket; error=%d", err);
			SetError(MSB_ERROR_SYSTEM, err);
			return false;
		}

		myPreallocatedSockets.Add(sock);
	}
	return true;
}

void
MSB_TCPListener::PrivLoadAcceptEx()
{
	GUID		GuidAcceptEx = WSAID_ACCEPTEX;
	DWORD		dwBytes = 0;

	if(WSAIoctl(GetSocket(), 
		SIO_GET_EXTENSION_FUNCTION_POINTER, 
		&GuidAcceptEx, 
		sizeof(GuidAcceptEx),
		&myAcceptEx, 
		sizeof(myAcceptEx), 
		&dwBytes, 
		NULL, 
		NULL) != 0)
	{
		LOG_ERROR("Could not load AcceptEx: error=%d", GetLastError());
	}
}

void
MSB_TCPListener::PrivLoadGetAcceptExSockaddrs()
{
	GUID		GuidAcceptEx = WSAID_GETACCEPTEXSOCKADDRS;
	DWORD		dwBytes = 0;

	if(WSAIoctl(GetSocket(), 
		SIO_GET_EXTENSION_FUNCTION_POINTER, 
		&GuidAcceptEx, 
		sizeof(GuidAcceptEx),
		&myGetAcceptExSockaddrs, 
		sizeof(myAcceptEx), 
		&dwBytes, 
		NULL, 
		NULL) != 0)
	{
		LOG_ERROR("Could not load AcceptEx: error=%d", GetLastError());
	}
}

#endif // IS_PC_BUILD
