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

#include <intrin.h>

// #include "MC_Logger.h"

#include "MSB_ReadMessage.h"
#include "MSB_TCPConnection.h"
#include "MSB_TCPConnectionListener.h"

#include "MSB_SimpleCounterStat.h"
#include "MSB_Stats.h"

#define WARN_MAX_READ_QUEUE (512 * 1024)

#define LOG_DEBUG3(...)

MSB_TCPConnection::MSB_TCPConnection(
	SOCKET						aSocket,
	const struct sockaddr_in&	aRemoteAddress,
	const struct sockaddr_in&	aLocalAddress,
	MSB_TCPConnectionListener*	aListener)
	: myTcpHandler(NULL)
	, myDetachedMessageCount(0)
	, myConnectionReady(false)
	, myIsClosed(false)
	, myError(MSB_NO_ERROR)
	, myRefCount(0)
	, myIsDeleting(false)
	, myFlushBeforeCloseFlag(false)
	, myListener(aListener)
	, myXtea(NULL)
	, myMaxOutgoingLength(-1)
	, myIsConnecting(true)
{
	myMessageStreamNotEmpty.ClearSignal();

	Retain();

	myTcpSocket = new MSB_TCPSocket(aSocket);
	myTcpSocket->SetRemoteAddress(aRemoteAddress);
	myTcpSocket->SetLocalAddress(aLocalAddress);
	myTcpSocket->Retain();

	myTcpHandler = new TcpHandler(*myTcpSocket, *this);
	myTcpSocket->SetMessageHandler(myTcpHandler);

	if ( aListener !=  NULL )
		StartGracePeriod(); //We are a server side Connection

	myTcpSocket->Register();
}

MSB_TCPConnection::MSB_TCPConnection(
	const char*					aHostname,
	MSB_Port					aPort,
	MSB_Xtea*					anXtea)
	: myTcpHandler(NULL)
	, myDetachedMessageCount(0)
	, myConnectionReady(false)
	, myTcpSocket(NULL)
	, myIsClosed(false)
	, myError(MSB_NO_ERROR)
	, myRefCount(0)
	, myIsDeleting(false)
	, myConnectToPort( aPort )
	, myListener(NULL)
	, myXtea(anXtea)
	, myMaxOutgoingLength(-1)
{
	myMessageStreamNotEmpty.ClearSignal();

	Retain();

	Retain();		// This is for the async callback (OnResolveComplete)
	MSB_Resolver::GetInstance().Resolve(aHostname, this);
}

MSB_TCPConnection::~MSB_TCPConnection()
{
	Close();
	LOG_DEBUG3("MSB_TCPConnection deleted");
}

void
MSB_TCPConnection::SetMaxOutgoingLength(
	uint32						aMaxOutgoingLength)
{
	MT_MutexLock				lock(myLock);

	myMaxOutgoingLength = aMaxOutgoingLength;
	if(myTcpSocket)
		myTcpSocket->SetMaxOutgoingLength(aMaxOutgoingLength);
}

void
MSB_TCPConnection::StartGracePeriod()
{
	myTcpSocket->StartGracePeriod();
}

void
MSB_TCPConnection::Delete(
	bool				aFlushBeforeCloseFlag)
{
	assert(myIsDeleting == false);

	myFlushBeforeCloseFlag = aFlushBeforeCloseFlag; 

	if(!myIsDeleting)
	{
		myIsDeleting = true;
		
		if(myConnectionReady)
		{
			Close(aFlushBeforeCloseFlag);
			Release();
		}
	}
}

void
MSB_TCPConnection::Close(
	bool				aFlushBeforeCloseFlag)
{
	MT_MutexLock		lock(myLock);

	if(myIsClosed == false)
	{
		LOG_INFO("Closing MSB_TCPConnection");
		myIsClosed = true;

		// Release potential waiters
		myMessageStreamNotEmpty.Signal();

		lock.Unlock();

		if(myTcpSocket)
		{
			if(aFlushBeforeCloseFlag)
				myTcpSocket->FlushAndClose();
			else
				myTcpSocket->Close();

			myTcpSocket->Release();
			myTcpSocket = NULL;
		}
	}
}

bool
MSB_TCPConnection::IsClosed() const
{
	MT_MutexLock		lock(const_cast<MT_Mutex&>(myLock));
	if(myIsConnecting)
		return false;
	else if(myIsClosed)
		return true;
	else if (myTcpSocket) //If we have a TCPSocket OnClose() wont run until we do our last release. 
	{                // So ask it for its close status. But not under our lock. Deadlock prevention
		myTcpSocket->Retain();
		lock.Unlock();
		bool res = myTcpSocket->IsClosed();
		myTcpSocket->Release();
		return res;
	}

	return myIsClosed;
}

bool
MSB_TCPConnection::GetNext(
	MSB_ReadMessage&		aReadMessage)
{
	MT_MutexLock			lock(myLock);
	if (aReadMessage.AttachToStream(myTcpMessageStream, myCurrentMessage) )
		return true;

	return false;
}

/**
 * Please not that you may not call WaitNext() from more than one thread.
 *
 */
bool
MSB_TCPConnection::WaitNext(
	MSB_ReadMessage&			aReadMessage)
{
	myMessageStreamNotEmpty.WaitForSignal();

	MT_MutexLock			lock(myLock);
	if(myIsClosed)
		return false;
	
	if (aReadMessage.AttachToStream(myTcpMessageStream, myCurrentMessage)){
		myDetachedMessageCount --;
		if(myDetachedMessageCount == 0)
			myMessageStreamNotEmpty.ClearSignal();

		return true;
	}

	return false;
}

/**
* Sends a message over tcp.
*
* If the connection has not yet been established it's queued for later
* transmission.
*
* DANGER! Mine field ahead.
*/
void
MSB_TCPConnection::Send(
	MSB_WriteMessage&		aMessage)
{
	MT_MutexLock			lock(myLock);

	if(myIsClosed || HasError() || myIsDeleting)
		return;

	if(myConnectionReady)
	{
		lock.Unlock();
		myTcpSocket->Send(aMessage);
	}
	else
	{
		myTcpWriteList.Push(aMessage.GetHeadBuffer());
	}
}

const char*
MSB_TCPConnection::GetRemoteAddress() const
{
	MT_MutexLock			lock((MT_Mutex&) myLock);

	if(myTcpSocket)
		return myTcpSocket->GetRemoteAddressString();

	return NULL;
}

const char* 
MSB_TCPConnection::GetLocalAddress() const
{
	MT_MutexLock			lock((MT_Mutex&) myLock);

	if(myTcpSocket)
		return myTcpSocket->GetLocalAddressString();

	return NULL;
}

bool
MSB_TCPConnection::GetLocalAddressSockAddrIn(
	struct sockaddr_in& aAddress)
{
	MT_MutexLock			lock((MT_Mutex&) myLock);

	if(myTcpSocket)
	{
		aAddress = myTcpSocket->GetLocalAddress(); 		
		return true; 
	}

	return false; 
}

void
MSB_TCPConnection::Retain()
{
	int32	ref = _InterlockedIncrement(&myRefCount);
	assert(ref > 0 && "Invalid MSB_TCPConnection::Retain() ref count.");
}

void
MSB_TCPConnection::Release()
{
	int32	ref = _InterlockedDecrement(&myRefCount);
	assert(ref >= 0 && "Invalid MSB_TCPConnection::Release() ref count.");

	if(ref == 0)
		delete this;
}

/**
* Start the connection procedure
*/
void
MSB_TCPConnection::OnResolveComplete(
	const char*				aHostname,
	struct sockaddr*		anAddress,
	size_t					anAddressLength)
{
	myLock.Lock();
	if(!ProtConnect(anAddress))
	{
		myIsConnecting = false;
		myMessageStreamNotEmpty.Signal();
	}
	myLock.Unlock();

	Release(); //OnResolveComplete Callback Done 
}

bool 
MSB_TCPConnection::ProtConnect( 
	struct sockaddr*		anAddress )
{
	if(anAddress == NULL)
	{
		myIsClosed = true;
		myError = MSB_ERROR_HOST_NOT_FOUND;;
		return false;
	}

	if(myIsDeleting)
	{
		if(!myFlushBeforeCloseFlag || !myTcpWriteList.HasData())
		{
			myIsClosed = true;
			return false;
		}
	}

	SOCKET				s = socket(AF_INET, SOCK_STREAM, 0);
	if(s == INVALID_SOCKET)
	{
		myError = MSB_ERROR_SYSTEM;
		return false;
	}

	myTcpSocket = new MSB_TCPSocket(s);
	myTcpSocket->Retain();
	myTcpHandler = new TcpHandler(*myTcpSocket, *this);

	myTcpSocket->SetMessageHandler(myTcpHandler);
	
	struct sockaddr_in*		addr = (struct sockaddr_in*) anAddress;
	addr->sin_port = htons(myConnectToPort);
	int err = myTcpSocket->Connect(anAddress);
	if ( err != 0 )
	{
		MC_ERROR("Failed to Connect() in OnResolveComplete()");
		myError = MSB_ERROR_CONNECTION;
		return false;
	}

	return true;
}

void
MSB_TCPConnection::ProtConnectionReady()
{
	myTcpSocket->SetCrypto(myXtea);
	myTcpSocket->SetMaxOutgoingLength(myMaxOutgoingLength);

	MT_MutexLock			lock(myLock);

	LOG_DEBUG3("Sending stored TCP packets.");
	MSB_WriteBuffer*		buffer;

	while((buffer = myTcpWriteList.Pop()) != NULL)
	{
		myTcpSocket->Send(buffer);
		while(buffer)
		{
			MSB_WriteBuffer*	next = buffer->myNextBuffer;
			buffer->Release();
			buffer = next;
		}
	}

	myConnectionReady = true;
	myIsConnecting = false;

	if(myIsDeleting)
	{
		// We release here since we can't release in Delete() when
		// we're not yet connected
		Release();
		lock.Unlock();

		Close(myFlushBeforeCloseFlag);
	}
	else if(myListener)
	{
		myListener->AppendConnection(this);
	}
}

void 
MSB_TCPConnection::SetCrypto( 
	MSB_Xtea*				anXtea)
{
	MT_MutexLock			lock(myLock);
	myXtea = anXtea;
	
	if (myTcpSocket)
		myTcpSocket->SetCrypto(anXtea);
}

//
// TcpHandler
//

MSB_TCPConnection::TcpHandler::TcpHandler(
	MSB_TCPSocket&			aSocket,
	MSB_TCPConnection&		aConnection)
	: MSB_MessageHandler((MSB_Socket_Win&) aSocket)
	, myConnection(aConnection)
{
	myConnection.Retain();
}

bool
MSB_TCPConnection::TcpHandler::OnInit(
	MSB_WriteMessage&	aWriteMessage)
{
	myConnection.ProtConnectionReady();

	return true;
}

bool
MSB_TCPConnection::TcpHandler::OnMessage(
	MSB_ReadMessage&	aReadMessage,
	MSB_WriteMessage&	aWriteMessage)
{
	MT_MutexLock		lock(myConnection.myLock);

	aReadMessage.DetachToStream(myConnection.myTcpMessageStream);

	// Message stream is no longer empty
	if(myConnection.myDetachedMessageCount == 0)
		myConnection.myMessageStreamNotEmpty.Signal();
	myConnection.myDetachedMessageCount ++;
	
// 	if ( myConnection.myTcpMessageStream.GetUsed() > WARN_MAX_READ_QUEUE )
// 		LOG_WARN("MSB_TCPConnection::TcpHandler::OnMessage data queue is %d bytes.", myConnection.myTcpMessageStream.GetUsed() );

	return true;
}

void
MSB_TCPConnection::TcpHandler::OnClose()
{
	// myConnection.myIsClosed = true;

	// Release potential waiters
	myConnection.myMessageStreamNotEmpty.Signal();
	myConnection.Close();
	myConnection.Release();
	delete this;
}

void
MSB_TCPConnection::TcpHandler::OnError(
	MSB_Error			anError)
{
	MC_ERROR("MSB_TCPConnection::TcpHandler::OnError Error=%d", anError);
	myConnection.myError = anError;
	myConnection.myIsConnecting = false;
	myConnection.Close();
}

bool 
MSB_TCPConnection::TcpHandler::OnSystemMessage( 
	MSB_ReadMessage&	aReadMessage, 
	MSB_WriteMessage&	aWriteMessage,
	bool&				aUsed)
{
	return myConnection.ProtOnSystemMessage(aReadMessage, aWriteMessage, aUsed);
}

//
// WriteList
//

MSB_TCPConnection::WriteList::WriteList()
	: myHead(NULL)
	, myTail(NULL)
{
}

MSB_TCPConnection::WriteList::~WriteList()
{
	while(myHead)
	{
		Entry*		entry = myHead;
		myHead = entry->myNext;

		MSB_WriteBuffer*		buffer = entry->myBuffers;
		while(buffer)
		{
			MSB_WriteBuffer*	next = buffer->myNextBuffer;
			buffer->Release();
			buffer = next;
		}
		delete entry;
	}
}

MSB_WriteBuffer*
MSB_TCPConnection::WriteList::Pop()
{
	if(!myHead)
		return NULL;

	Entry*		entry = myHead;
	myHead = myHead->myNext;
	if(myHead == NULL)
		myTail = NULL;

	MSB_WriteBuffer*	buffer = entry->myBuffers;
	delete entry;

	return buffer;
}

void
MSB_TCPConnection::WriteList::Push(
	MSB_WriteBuffer*		aBuffer)
{
	assert(aBuffer != NULL);

	MSB_WriteBuffer*		curr = aBuffer;
	while(curr)
	{
		MSB_WriteBuffer*	next = curr->myNextBuffer;
		curr->Retain();
		curr = next;
	}

	Entry*			e = new Entry();
	e->myNext = NULL;
	e->myBuffers = aBuffer;

	if(myTail)
	{
		myTail->myNext = e;
		myTail = e;
	}
	else
	{
		myHead = myTail = e;
	}
}

bool
MSB_TCPConnection::WriteList::HasData()
{
	return myHead != NULL; 
}


//
// ReadStream
//

MSB_TCPConnection::ReadStream::ReadStream()
	: myBufferList(NULL)
	, mySize(0)
	, myReadOffset(0)
{
}

MSB_TCPConnection::ReadStream::~ReadStream()
{
}

void
MSB_TCPConnection::ReadStream::SetBufferList(
	MSB_WriteBuffer*	aBufferList)
{
	MSB_WriteBuffer*	current = myBufferList;
	while(current)
	{
		MSB_WriteBuffer*	tmp = current->myNextBuffer;
		current->Release();
		current = tmp;
	}

	mySize = 0;
	myReadOffset = 0;
	myBufferList = current = aBufferList;
	while(current)
	{
		mySize += current->myWriteOffset;
		current = current->myNextBuffer;
	}
}

uint32
MSB_TCPConnection::ReadStream::GetUsed()
{
	return mySize;
}

uint32
MSB_TCPConnection::ReadStream::Read(
	void*			aBuffer,
	uint32			aSize)
{
	uint32			totalSize = __min(aSize, mySize);
	uint32			size = totalSize;
	uint8*			buffer = (uint8*) aBuffer;

	while(size > 0)
	{
		uint32		blockSize = __min(size, myBufferList->myWriteOffset - myReadOffset);
		memmove(buffer, &myBufferList->myBuffer[myReadOffset], blockSize);

		buffer += blockSize;
		myReadOffset += blockSize;
		mySize -= blockSize;
		size -= blockSize;

		if(myReadOffset == myBufferList->myWriteOffset)
		{
			MSB_WriteBuffer*		tmp = myBufferList->myNextBuffer;
			myBufferList->Release();
			myBufferList = tmp;
			myReadOffset = 0;
		}
	}

	return totalSize;
}

uint32
MSB_TCPConnection::ReadStream::Peek(
	void*			aBuffer,
	uint32			aSize)
{
	uint32				totalSize = totalSize = __min(aSize, mySize);
	uint32				size = totalSize;
	uint8*				buffer = (uint8*) aBuffer;
	uint32				readOffset = myReadOffset;
	MSB_WriteBuffer*	readBuffer = myBufferList;

	while(size > 0)
	{
		uint32		blockSize = __min(size, readBuffer->myWriteOffset - readOffset);
		memmove(buffer, &readBuffer->myBuffer[readOffset], blockSize);

		buffer += blockSize;
		readOffset += blockSize;
		size -= blockSize;

		if(readOffset == readBuffer->myWriteOffset)
		{
			readBuffer = readBuffer->myNextBuffer;
			readOffset = 0;
		}
	}

	return totalSize;
}

uint32
MSB_TCPConnection::ReadStream::Write(
	const void*		aBuffer,
	uint32			aSize)
{
	assert(false && "Don't even try it");	
	return 0;
}

#endif // IS_PC_BUILD
