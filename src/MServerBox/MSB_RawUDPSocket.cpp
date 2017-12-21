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

// #include "MC_Logger.h"
#include "MSB_Types.h"
#include "MSB_MemoryStream.h"
#include "MSB_MessageHandler.h"
#include "MSB_RawUDPSocket.h"
#include "MSB_ReadMessage.h"
#include "MSB_WriteMessage.h"

MSB_RawUDPSocket::MSB_RawUDPSocket(
	SOCKET				aSocket)
	: MSB_Socket_Win(aSocket)
	, myMessageHandler(NULL)
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

MSB_RawUDPSocket::~MSB_RawUDPSocket()
{

}

int32
MSB_RawUDPSocket::Start()
{
	return PrivRecvNext();
}

int32
MSB_RawUDPSocket::Send(
	struct sockaddr_in*	anAddress, 
	MSB_WriteMessage&	aWriteMessage)
{
	if(aWriteMessage.IsEmpty() == false)
	{
		MT_MutexLock		lock(myLock);

		bool wasEmpty = myUDPWriteList.IsEmpty();
		myUDPWriteList.Add(*anAddress, aWriteMessage.GetHeadBuffer());

		//If the UDPWrite list (send queue) was empty, we need to start an iocp sender as well
		if ( wasEmpty )
		{
			int err = myUDPWriteList.Send( this );
			if ( err != 0 )
				ProtSetError(MSB_ERROR_SYSTEM, err);
			return err;
		}

	}

	return 0;
}

int32
MSB_RawUDPSocket::OnSocketWriteReady(
	int					aByteCount,
	OVERLAPPED*			anOverlapped)
{
	MT_MutexLock		lock(myLock);

	// The packet was successfully sent, remove it from in transit list
	myUDPWriteList.RemoveFromInTransit(anOverlapped, this);

	// Try to send some more
	return myUDPWriteList.Send(this);
}

int32
MSB_RawUDPSocket::OnSocketReadReady(
	int					aByteCount,
	OVERLAPPED*			anOverlapped)
{
	Release();

	if(aByteCount < MSB_PACKET_HEADER_TOTSIZE + sizeof(MSB_DelimiterType))
	{
		ProtSetError(MSB_NO_ERROR, -1); //Error recovery
		return PrivRecvNext();
	}

	MSB_MemoryStream		stream(myReadBuffer, aByteCount);
	MSB_ReadMessage			rmsg(&stream);
	while(rmsg.IsMessageComplete())
	{
		if(rmsg.DecodeHeader() == false || rmsg.DecodeDelimiter() == false)
			break;

		MSB_WriteMessage	response;
		if(!myMessageHandler->OnMessage(rmsg, response))
			break;
		
		Send(&myLastAddr, response);
	}

	return PrivRecvNext();
}

void
MSB_RawUDPSocket::OnSocketError(
	MSB_Error			anError,
	int					aSystemError,
	OVERLAPPED*			anOverlapped)
{
	MC_ERROR("MSB_RawUDPSocket::OnSocketError Error=%d SystemError=%d", anError, aSystemError);
}

void
MSB_RawUDPSocket::OnSocketClose()
{
	myMessageHandler->OnClose();
	myMessageHandler = NULL;
}

int32
MSB_RawUDPSocket::PrivRecvNext()
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

	int err = WSAGetLastError();
	if (res != 0 && err != ERROR_IO_PENDING)
	{
		SetError(MSB_ERROR_SYSTEM, err);
		return err;
	}

	Retain(); //We have successfully started a new read operation.

	return 0;
}
