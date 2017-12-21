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

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_TCPSocket.h"
#include <intrin.h>
#include <Mswsock.h>
#include <Mstcpip.h>

#include "MSB_Types.h"
#include "MSB_MessageHandler.h"
#include "MSB_ReadMessage.h"
#include "MSB_WriteBuffer.h"
#include "MSB_WriteMessage.h"

#define MAX_OUTGOING_LENGTH		1048576		// 1MB

#define LOG_DEBUG3(...)

MSB_TCPSocket::MSB_TCPSocket(
	SOCKET					aSocket,
	MSB_Xtea*				anXtea,
	int32					aKeepAlive,
	bool					aEnableNoDelayFlag)
	: MSB_Socket_Win(aSocket)
	, myIsConnecting(false)
	, myIsClosing(false)
	, myHasCalledOnError(false)
	, myMessageHandler(NULL)
	, myXtea(anXtea)
	, myIsWaitingFirstMessage(true)
{
	myWriteList.SetSocket(this);

	if(aKeepAlive)
		SetKeepAlive(aKeepAlive);

	if(aEnableNoDelayFlag)
		SetNoDelay();
}

MSB_TCPSocket::~MSB_TCPSocket()
{
	if ( GetSocket() != INVALID_SOCKET )
		Close();
	
	LOG_DEBUG3("MSB_TCPSocket destroyed");
}

int32
MSB_TCPSocket::Connect(
	struct sockaddr*		anAddress)
{
	MT_MutexLock			lock(myLock);
	myIsConnecting = true;

	struct sockaddr_in		addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = INADDR_ANY;
	if(bind(GetSocket(), (struct sockaddr*) &addr, sizeof(addr)) != 0)
	{
		int err = WSAGetLastError();
		MC_ERROR("Failed to bind socket; Error = %d", err);
		SetError(MSB_ERROR_SYSTEM, err);
		return err;
	}
	
	LPFN_CONNECTEX			ConnectEx;	
	GUID					GuiConnectEx = WSAID_CONNECTEX;
	DWORD					dwBytes;
	WSAIoctl(GetSocket(),
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuiConnectEx,
		sizeof(GuiConnectEx),
		&ConnectEx,
		sizeof(ConnectEx),
		&dwBytes,
		NULL,
		NULL
		); 
	
	memset(&myOverlapped, 0, sizeof(myOverlapped));
	myOverlapped.write = true;

	SetRemoteAddress((struct sockaddr_in&) *anAddress);

	int32 err = Register();
	if (err != 0)
		return err;

	DWORD					temp;
	if(ConnectEx(GetSocket(), anAddress, sizeof(struct sockaddr), NULL, 0, &temp, (LPOVERLAPPED) &myOverlapped) != TRUE)
	{
		if(WSAGetLastError() != WSA_IO_PENDING) 
		{
			int err = WSAGetLastError();
			MC_ERROR("Failed to connect; Error=%d", err);
			SetError(MSB_ERROR_SYSTEM, err);
			return err;
		}
	}

	Retain();

	return 0;
}

void
MSB_TCPSocket::SetMaxOutgoingLength(
	uint32					aMaxOutgoingLength)
{
	MT_MutexLock			lock(myLock);
	myWriteList.SetMaxOutgoingLength(aMaxOutgoingLength);
}

void
MSB_TCPSocket::StartGracePeriod()
{
	myIsWaitingFirstMessage = true;
	MSB_IoCore::GetInstance().StartGracePeriod(this);
}

bool 
MSB_TCPSocket::GracePeriodCheck() const
{
	return !myIsWaitingFirstMessage;
}

int32
MSB_TCPSocket::SetKeepAlive(
	int32				aKeepAliveTime)
{
	assert(myIsConnecting == false && "You may not SetKeepAliveDelay() on an connecting socket.");
	
	tcp_keepalive settings;
	settings.onoff = 1;
	settings.keepalivetime = aKeepAliveTime;
	settings.keepaliveinterval = 1000;

	DWORD nBytes=0;
	if ( WSAIoctl( GetSocket(), 
			SIO_KEEPALIVE_VALS, &settings, sizeof(settings), 
			NULL, 0, &nBytes, NULL, NULL ) == SOCKET_ERROR )
	{
		int err = WSAGetLastError();
		MC_ERROR("Can't set keepalive on MSB_TCPSocket. Error=%d", err);
		SetError(MSB_NO_ERROR, err); //This is not a serious enough error, but save the system error
		return err;
	}

	return 0;
}

int32
MSB_TCPSocket::SetNoDelay()
{
	assert(myIsConnecting == false && "You may not SetNoDelay() on an connecting socket.");
	
	int			enable = 1;
	if ( setsockopt(GetSocket(), IPPROTO_TCP, TCP_NODELAY, 
			(const char*) &enable, sizeof(enable)) == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		MC_ERROR("Can't set TCP_NODELAY on MSB_TCPSocket. Error=%d", err);
		SetError(MSB_NO_ERROR, err); //This is not a serious enough error, but save the system error
		return WSAGetLastError();
	}

	return 0;
}

int32
MSB_TCPSocket::OnSocketWriteReady(
	int					aByteCount, 
	OVERLAPPED*			anOverlapped)
{
	MT_MutexLock		lock(myLock);

	// One less iocp-completion to wait for
	Release();

	int32				error = 0;
	if(myIsConnecting) //A Connect() has completed
	{
		if ( ProtIsClosed() )
		{
			//An operation completed but the socket has been closed. 
			// Typically happens when issuing Connect() and then Close() directly afterwards
			return 0; //This is not an error. Just a late operation.
		}

		// Free/inherit it self from the listen socket context
		SOCKET asock = GetSocket();
		int res = setsockopt( GetSocket(),
			SOL_SOCKET, 
			SO_UPDATE_CONNECT_CONTEXT, 
			NULL, 
			0 );

		if (res == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			MC_ERROR("Can't setsockopt() SO_UPDATE_CONNECT_CONTEXT on newly connected socket. Error=%d.", err);
			ProtSetError(MSB_ERROR_SYSTEM, err);
			return err;
		}

		struct sockaddr_in	addr;
		int					len = sizeof(addr);
		if(getsockname(GetSocket(), (struct sockaddr*) &addr, &len) != 0)
		{
			int err = WSAGetLastError();
			MC_ERROR("Failed to get local address; Error = %d", err);
			SetError(MSB_ERROR_SYSTEM, err);
			return err;
		}
		SetLocalAddress(addr);

		myIsConnecting = false;
		myIsWaitingFirstMessage = false;

		Start();
	}

	error = myWriteList.SendComplete();
	if(myWriteList.IsIdle() && myIsClosing)
		Close();

	return error;
}


int32
MSB_TCPSocket::OnSocketReadReady(
	 int				aByteCount, 
	 OVERLAPPED*		anOverlapped)
{
	MT_MutexLock		lock(myLock);

	//End last started buffer filling
	myBuffer.EndFill(aByteCount);
	Release(); //Last operation has finished (or we have at least read its data from kernel)

	if(aByteCount == 0)		// Socket is closed, lets kill it
	{
		Close();
		return 0;
	}

	if(ProtIsClosed() || ProtHasError())
		return 0;

	if(myIsClosing == true)
		return 0;

	//Handle the messages in the buffer
	MSB_ReadMessage		message(&myBuffer);
	while(message.IsMessageComplete())
	{
		if(!message.DecodeHeader())
		{
			SetError(MSB_ERROR_INVALID_DATA, -1);
			return -1;
		}

		int32 err = PrivProcessReadMessage(message);
		if ( err != 0 )
		{
			FlushAndClose(); 
			SetError(MSB_ERROR_INVALID_DATA, -1);
			return err;
		}
	}

	int err = myBuffer.BeginFill( *this ); 	//Start filling the buffer again
	if (err != 0)
		SetError(MSB_ERROR_SYSTEM, err);

	return err;
}

void
MSB_TCPSocket::OnSocketError(
	MSB_Error	anError,
	int			aSystemError,
	OVERLAPPED*	anOverlapped)
{
	MT_MutexLock		lock(myLock);

	//Only call OnError once and if socket isn't closed (Socket closed isn't an error)
	if ( myHasCalledOnError == false && ProtIsClosed() == false ) 
	{
		MC_ERROR("MSB_TCPSocket::OnSocketError Error=%d, SystemError=%d", anError, aSystemError);
		myHasCalledOnError = true;
		lock.Unlock();
		
		myMessageHandler->OnError(anError);
		Close();
	}
}

void
MSB_TCPSocket::OnSocketClose()
{
	assert(myMessageHandler != NULL);

	myMessageHandler->OnClose();
	myMessageHandler = NULL; //Trigger assert on usage of this closed socket
}

/**
* Called when a connection has been accepted, used to bootstrap the 
* read-processing.
*/
int32
MSB_TCPSocket::Start()
{
	assert(myMessageHandler != NULL);

	if(myIsConnecting)
		return 0;

	MSB_WriteMessage writeMessage;
	bool res = myMessageHandler->OnInit(writeMessage);
	if ( res == false )
	{
		SetError(MSB_ERROR_INVALID_DATA, -1);
		return -1;
	}
	int err = Send(writeMessage);
	if ( err != 0 )
		return err;

	return myBuffer.BeginFill( *this ); //Start a read operation
}

/**
 * Sends a WriteMessage.
 *
 * The data is sent as soon as possible on socket.
 *
 * @returns 0 if successful otherwise an error code.
 */
int32
MSB_TCPSocket::Send(
	MSB_WriteMessage&	aMessage)
{
	if( aMessage.IsEmpty() )
		return 0;

	int res = Send( aMessage.GetHeadBuffer() );

	return res;
}

/**
* Queues all the buffer chained with aBuffer to be sent to the client.
* If the send causes the outstanding queue size to grow beyond a 1MB limit
* the client will be disconnected.
*/
int32
MSB_TCPSocket::Send(
	MSB_WriteBuffer*	aHeadBuffer)
{
	MT_MutexLock		lock(myLock);

	if(ProtIsClosed())
		return 0;

	if(myWriteList.Add(aHeadBuffer) == -1)
	{
		lock.Unlock();
		Close();
		return -1;
	}

	return 0;
}

/**
 * Called for every system message received.
 * 
 * @returns True if the connection should stay alive, false to close it.
 */
bool
MSB_TCPSocket::ProtHandleSystemMessage(
	MSB_ReadMessage&		aReadMessage,
	MSB_WriteMessage&		aResponse)
{
	//We do not have any system message. Everything is invalid.
	return false;
}

void 
MSB_TCPSocket::FlushAndClose()
{
	MT_MutexLock		lock(myLock);

	myIsClosing = true;
	if(myWriteList.IsIdle() && !myIsConnecting)
		Close();
}

void
MSB_TCPSocket::SetCrypto(
	MSB_Xtea*			anXtea)
{
	myXtea = anXtea;
}

bool
MSB_TCPSocket::HasCrypto() const
{
	return myXtea != NULL;
}

int32
MSB_TCPSocket::PrivProcessReadMessage(
	MSB_ReadMessage&	aMessage)
{
	if(!aMessage.DecodeDelimiter())
	{
		ProtSetError(MSB_ERROR_INVALID_DATA, -1);
		return -1;
	}

	if(myIsWaitingFirstMessage)
	{
		myIsWaitingFirstMessage = false;
		MSB_IoCore::GetInstance().StopGracePeriod(this);
	}

	bool res;
	MSB_WriteMessage response;
	if(aMessage.IsSystemMessage())
	{
		bool used = false;
		res = myMessageHandler->OnSystemMessage(aMessage, response, used);
		if (res == true && used == false)
			res = ProtHandleSystemMessage(aMessage, response);
		if ( res == false )
		{
			//Data error on a system message, kill the connection
			SetError(MSB_ERROR_INVALID_SYSTEM_MSG, -2);
			return -2; //Will report the error upwards
		}
	}
	else
	{
		res = myMessageHandler->OnMessage(aMessage, response);
		if ( res == false )
		{
			Send(response); //Do send (if any) the last response. We do not care about any errors here.
			return -1;
		}
	}
	
	return Send(response);
}

//
// MSB_TCPSocket::Buffer
//

MSB_TCPSocket::Buffer::Buffer()
	: myLength(0)
	, myReadOffset(0)
{
	myFirstAvailable = MSB_WriteBuffer::Allocate();
	myFirstAvailable->myNextBuffer = MSB_WriteBuffer::Allocate();

	myHead = myFirstAvailable;
}

MSB_TCPSocket::Buffer::~Buffer()
{
	MSB_WriteBuffer*		current = myHead;
	while(current)
	{
		MSB_WriteBuffer*	next = current->myNextBuffer;
		current->Release();
		current = next;
	}
}

int32
MSB_TCPSocket::Buffer::BeginFill(
	MSB_TCPSocket&	aSocket)
{
	ZeroMemory(&myOverlapped, sizeof(myOverlapped));
	myOverlapped.write = false;

	myWsaBuffers[0].buf = (char*) &myFirstAvailable->myBuffer[myFirstAvailable->myWriteOffset];
	myWsaBuffers[0].len = myFirstAvailable->myBufferSize - myFirstAvailable->myWriteOffset;

	myWsaBuffers[1].buf = (char*) myFirstAvailable->myNextBuffer->myBuffer;
	myWsaBuffers[1].len = myFirstAvailable->myNextBuffer->myBufferSize;

	DWORD		bytesReceived;
	DWORD		options = 0;
	if ( WSARecv(aSocket.GetSocket(), myWsaBuffers, 2, &bytesReceived, 
		&options, (LPOVERLAPPED) &myOverlapped, NULL) == SOCKET_ERROR )
	{
		int32		error = WSAGetLastError();
		if(error != WSA_IO_PENDING)
			return error;
	}
	aSocket.Retain(); //We have started one new read operation

	return 0;
}

void
MSB_TCPSocket::Buffer::EndFill(
	uint32			aDataLength)
{
	myLength += aDataLength;

	do
	{
		uint32			blockSize = __min(myFirstAvailable->myBufferSize - myFirstAvailable->myWriteOffset, aDataLength);

		myFirstAvailable->myWriteOffset += blockSize;
		aDataLength -= blockSize;

		if(myFirstAvailable->myWriteOffset == myFirstAvailable->myBufferSize)
		{
			myFirstAvailable = myFirstAvailable->myNextBuffer;
			myFirstAvailable->myNextBuffer = MSB_WriteBuffer::Allocate();
		}
	}while(aDataLength > 0);
}

//
// Buffer
//

uint32
MSB_TCPSocket::Buffer::GetUsed()
{
	return myLength;
}

uint32
MSB_TCPSocket::Buffer::Read(
	void*			aBuffer,
	uint32			aSize)
{
	uint32			size = 0;
	uint8*			buffer = (uint8*) aBuffer;

	while(myLength > 0 && aSize > 0)
	{
		uint32		blockSize = __min(myHead->myWriteOffset - myReadOffset, aSize);
		memmove(buffer, &myHead->myBuffer[myReadOffset], blockSize);

		myLength -= blockSize;
		aSize -= blockSize;
		buffer += blockSize;
		size += blockSize;
		myReadOffset += blockSize;

		if(myReadOffset == myHead->myBufferSize)
		{
			MSB_WriteBuffer*		current = myHead;
			myHead = myHead->myNextBuffer;
			current->Release();

			myReadOffset = 0;
		}
	}

	return size;
}

uint32
MSB_TCPSocket::Buffer::Peek(
	void*			aBuffer,
	uint32			aSize)
{
	uint32			size = 0;
	uint8*			buffer = (uint8*) aBuffer;

	uint32			tempOffset;

	MSB_WriteBuffer*	tempHead = myHead;
	tempOffset = myReadOffset;
	while(myLength > 0 && aSize > 0)
	{
		uint32		blockSize = __min(tempHead->myWriteOffset - tempOffset, aSize);
		memmove(buffer, &tempHead->myBuffer[tempOffset], blockSize);

		aSize -= blockSize;
		buffer += blockSize;
		size += blockSize;
		tempOffset += blockSize;

		if(tempOffset == tempHead->myBufferSize)
		{
			tempHead = tempHead->myNextBuffer;
			tempOffset = 0;
		}
	}

	return size;
}

#endif // IS_PC_BUILD
