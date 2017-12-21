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

#include "MSB_ConnectionListener.h"
#include "MSB_ComboSocket.h"
#include "MSB_Connection_Win.h"
#include "MSB_TCPSocket.h"

#include "MSB_SimpleCounterStat.h"
#include "MSB_Stats.h"

#define LOG_DEBUG3(...)
#define LOG_DEBUG3(...)
#define LOG_WARN(...)
#define LOG_DEBUG5(...)

#define WARN_MAX_READ_QUEUE (512*1024)

MSB_Connection::MSB_Connection(
	MSB_UDPSocket*				anUdpSocket,
	SOCKET						aTcpSocket,
	const struct sockaddr_in&	aRemoteAddress,
	const struct sockaddr_in&	aLocalAddress,
	MSB_ConnectionListener*		aListener)
	: myTcpHandler(NULL)
	, myUdpHandler(NULL)
	, myComboSocket(NULL)
	, myComboReady(false)
	, myConnectionReady(false)
	, myUdpSocket(anUdpSocket)
	, myIsClosed(false)
	, myError(MSB_NO_ERROR)
	, myRefCount(0)
	, myOwnsUdpSocket(false)
	, myIsDeleting(false)
	, myUdpPortReady(false)
	, myListener(aListener)
	, myXtea(NULL)
	, myDetachedMessageCount(0)
{
	++ (MSB_SimpleCounterStat&) MSB_Stats::GetInstance().FindContext("ServerBox")["NumActiveConnections"];

	Retain();

	myUdpSocket->Retain();
	myUdpHandler = new UdpHandler(*myUdpSocket, *this);

	myComboSocket = new MSB_ComboSocket(myUdpSocket, this, aTcpSocket);
	myComboSocket->SetRemoteAddress(aRemoteAddress);
	myComboSocket->SetLocalAddress(aLocalAddress);
	myComboSocket->Retain();

	myTcpHandler = new TcpHandler(*myComboSocket, *this);

	myComboSocket->SetMessageHandler(myTcpHandler);
	myComboSocket->SetUdpHandler(myUdpHandler);

	if ( aListener !=  NULL )
		StartGracePeriod(); //We are a server side Connection

	myComboSocket->Register();
	myComboSocket->HandshakeStart();
}

MSB_Connection::MSB_Connection(
	const char*					aHostname,
	MSB_Port					aPort,
	MSB_Xtea*					anXtea)
	: myTcpHandler(NULL)
	, myUdpHandler(NULL)
	, myComboSocket(NULL)
	, myConnectToPort(aPort)
	, myComboReady(false)
	, myIsClosed(false)
	, myError(MSB_NO_ERROR)
	, myRefCount(0)
	, myOwnsUdpSocket(false) //not yet
	, myIsDeleting(false)
	, myConnectionReady(false)
	, myUdpPortReady(false)
	, myUdpSocket(NULL)
	, myListener(NULL)
	, myXtea(anXtea)
	, myDetachedMessageCount(0)
{
	++ (MSB_SimpleCounterStat&) MSB_Stats::GetInstance().FindContext("ServerBox")["NumActiveConnections"];

	Retain();

	Retain();		// This is for the async callback
	MSB_Resolver::GetInstance().Resolve(aHostname, this);
}

MSB_Connection::~MSB_Connection()
{
	Close();
	LOG_DEBUG3("Connection deleted; comboSocket = %p", myComboSocket);

	-- (MSB_SimpleCounterStat&) MSB_Stats::GetInstance().FindContext("ServerBox")["NumActiveConnections"];
}

void
MSB_Connection::SetMaxOutgoingLength(
	uint32						aMaxBufferLength)
{
	MT_MutexLock				lock(myLock);
	myMaxOutgoingDataLength = aMaxBufferLength;

	if(myComboSocket)
		myComboSocket->SetMaxOutgoingLength(aMaxBufferLength);
}

void
MSB_Connection::StartGracePeriod()
{
	myComboSocket->StartGracePeriod();
}

void
MSB_Connection::Delete(
	bool						aFlushBeforeCloseFlag)
{
	assert(myIsDeleting == false);

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
MSB_Connection::Close(
	bool						aFlushBeforeCloseFlag)
{
	MT_MutexLock		lock(myLock);

	if(myIsClosed == false)
	{
		LOG_DEBUG3("Closing; server side = %s", myOwnsUdpSocket ? "false" : "true");
		myIsClosed = true;

		// Release potential waiters
		myMessageStreamNotEmpty.Signal();

		lock.Unlock();

		MSB_IoCore::GetInstance().StopUdpPortCheck(myComboSocket);

 		if(myComboSocket && myComboSocket->GetRemoteClientId() != 0)
 			myUdpSocket->RemoveRemoteClient(myComboSocket->GetRemoteClientId());
 		else if(myUdpHandler)
 			myUdpHandler->OnClose();

		if(myUdpSocket)
		{
			if(myOwnsUdpSocket)
			{
				if(aFlushBeforeCloseFlag)
					myUdpSocket->FlushAndClose();
				else
					myUdpSocket->Close();
			}

			myUdpSocket->Release();
			myUdpSocket = NULL;
		}

		if(myComboSocket)
		{
			if(aFlushBeforeCloseFlag)
				myComboSocket->FlushAndClose();
			else
				myComboSocket->Close();

			myComboSocket->Release();
			myComboSocket = NULL;
		}

	}
}

bool
MSB_Connection::IsClosed() const
{
	MT_MutexLock		lock(const_cast<MT_Mutex&>(myLock));

	if(myComboSocket)
		return myComboSocket->IsClosed();

	return myIsClosed;
}

bool
MSB_Connection::GetNext(
	MSB_ReadMessage&			aReadMessage)
{
	for(uint32 i = 0; i < 2; i++)
	{
		switch(myStreamCounter++ & 1)
		{
			case 0:
				{
					MT_MutexLock		lock(myUdpStreamLock);
					if(aReadMessage.AttachToStream(myUdpMessageStream, myCurrentMessage))
						return true;
				}
				break;

			case 1:
				{
					MT_MutexLock		lock(myTcpStreamLock);
					if(aReadMessage.AttachToStream(myTcpMessageStream, myCurrentMessage))
						return true;
				}
				break;
		}
	}
	
	return false;
}

bool
MSB_Connection::WaitNext(
	MSB_ReadMessage&			aMessage)
{
	myMessageStreamNotEmpty.WaitForSignal();

	for(uint32 i = 0; i < 2; i++){
		switch(myStreamCounter++ & 1){
			case 0:
				{
					MT_MutexLock		lock(myUdpStreamLock);
					if(PrivTryDetachFromStream(myUdpMessageStream, aMessage))
						return true;
				}
				break;
				

			case 1:
				{
					MT_MutexLock		lock(myTcpStreamLock);
					if(PrivTryDetachFromStream(myTcpMessageStream, aMessage))
						return true;
				}
				break;
		}
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
MSB_Connection::Send(
	MSB_WriteMessage&		aMessage)
{
	MT_MutexLock			lock(myLock);
	if(myIsClosed || HasError() || myIsDeleting)
		return;

	if(myConnectionReady)
	{
		lock.Unlock();
		myComboSocket->Send(aMessage);
	}
	else
	{
		myTcpWriteList.Push(aMessage.GetHeadBuffer());
	}
}

/**
 * Sends a message over udp.
 *
 * If the connection has not yet been established the packet is queued for
 * later transmission.
 */
void
MSB_Connection::SendUdp(
	MSB_WriteMessage&			aWriteMessage)
{
	MT_MutexLock			lock(myLock);
	if(myIsClosed || HasError() || myIsDeleting)
		return;

	if(myConnectionReady)
	{
		lock.Unlock();
		myComboSocket->SendUdp(aWriteMessage);
	}
	else
		myUdpWriteList.Push(aWriteMessage.GetHeadBuffer());
}

const char*
MSB_Connection::GetRemoteAddress() const
{
	MT_MutexLock			lock((MT_Mutex&) myLock);
	
	if(myComboSocket)
		return myComboSocket->GetRemoteAddressString();

	return NULL;
}

void
MSB_Connection::Retain()
{
	int32	ref = _InterlockedIncrement(&myRefCount);
	assert(ref > 0);
}

void
MSB_Connection::Release()
{
	int32	ref = _InterlockedDecrement(&myRefCount);
	assert(ref >= 0);

	if(ref == 0)
		delete this;
}

void
MSB_Connection::OnResolveComplete(
	const char*				aHostname,
	struct sockaddr*		anAddress,
	size_t					anAddressLength)
{
	myLock.Lock();
	PrivConnect(anAddress);
	myLock.Unlock();

	Release(); //OnResolveComplete Callback Done 
}

/**
 * Start the connection procedure
 */
bool
MSB_Connection::PrivConnect(
	struct sockaddr*		anAddress)
{
	if(anAddress == NULL)
	{
		myIsClosed = true;
		myError = MSB_ERROR_HOST_NOT_FOUND;;
		return false;
	}

	SOCKET				s = socket(AF_INET, SOCK_STREAM, 0);
	if(s == INVALID_SOCKET)
	{
		myError = MSB_ERROR_SYSTEM;
		return false;
	}

	//SOCKET			udp = socket(AF_INET, SOCK_DGRAM, 0);
	SOCKET				udp = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, NULL, WSA_FLAG_OVERLAPPED);
	if(udp == INVALID_SOCKET)
	{
		MC_ERROR("Failed to create udp socket; Error = %d\n", GetLastError());
		myError = MSB_ERROR_SYSTEM;
		return false;
	}

	struct sockaddr_in		udpAddress;
	memset(&udpAddress, 0, sizeof(udpAddress));
	udpAddress.sin_family = AF_INET;
	udpAddress.sin_port = 0;
	udpAddress.sin_addr.S_un.S_addr = INADDR_ANY;

	// This bind() has been shown to return with error fairly often during load test. 
	// Don't know why. Let it try 3 times with a small sleep between
	int bindTries;
	for (bindTries = 1; bindTries <= 3; bindTries++)
	{
		if(bind(udp, (struct sockaddr*) &udpAddress, sizeof(udpAddress)) == 0)
			break;
		//LOG_WARN("Failed to bind() UDP port in MSB_Connection. Error=%d. Retrying in 10ms", WSAGetLastError());
		Sleep(10);
	}
	if ( bindTries == 3)
	{
		MC_ERROR("Failed to bind udp socket to port %d; Error = %d\n", ntohs(udpAddress.sin_port), WSAGetLastError());
		myError = MSB_ERROR_SYSTEM;
		return false;
	}

	MC_DEBUG("Bound udp socket to port %d", ntohs(udpAddress.sin_port));

	struct sockaddr_in		udpTarget;
	memcpy(&udpTarget, anAddress, sizeof(udpTarget));
	udpTarget.sin_port = htons(myConnectToPort);

	myUdpSocket = new MSB_UDPSocket(udp);
	myUdpSocket->Retain();
	myOwnsUdpSocket = true;
	myUdpHandler = new UdpHandler(*myUdpSocket, *this);

	if ( myUdpSocket->Register() != 0 )
		return false;

	myComboSocket = new MSB_ComboSocket(myUdpSocket, this, s);
	myComboSocket->Retain();
	myTcpHandler = new TcpHandler(*myComboSocket, *this);

	myComboSocket->SetMessageHandler(myTcpHandler);
	myComboSocket->SetUdpHandler(myUdpHandler);

	struct sockaddr_in*		addr = (struct sockaddr_in*) anAddress;
	addr->sin_port = htons(myConnectToPort);
	int err = myComboSocket->Connect(anAddress);
	if ( err != 0 )
	{
		MC_ERROR("Failed to Connect() in OnResolveComplete()");
		myError = MSB_ERROR_CONNECTION;
		return false;
	}
	
	return true;
}

/**
 * Called when the combo socket has received both local and remote id. Last step
 * in the setup process is locating our external udp-port.
 * 
 */
void
MSB_Connection::OnComboSocketReady()
{
	MT_MutexLock			lock(myLock);
	
	myComboReady = true;

	if(myConnectionReady == false && myListener == NULL)
	{
		PrivConnectionReady();	// client side: all ready. 
								// server side: we're still waiting 
								// for our port-check packets.
	}
}

void 
MSB_Connection::SetCrypto( 
	MSB_Xtea*				anXtea)
{
	MT_MutexLock			lock(myLock);
	myXtea = anXtea;
	
	if (myComboReady)
		myComboSocket->SetCrypto(anXtea);
}

void
MSB_Connection::PrivConnectionReady()
{
	if(myConnectionReady == false)
	{
		MSB_IoCore::GetInstance().StopUdpPortCheck(myComboSocket);

		myComboSocket->SetMaxOutgoingLength(myMaxOutgoingDataLength);
		myComboSocket->SetCrypto(myXtea);

		MT_MutexLock		lock(myLock);

		// Udp is connected, send our queue post haste!
		MSB_WriteBuffer*		buffer;
		while((buffer = myUdpWriteList.Pop()) != NULL)
		{
			myComboSocket->SendUdp(buffer);
			while(buffer)
			{
				MSB_WriteBuffer*	next = buffer->myNextBuffer;
				buffer->Release();
				buffer = next;
			}
		}

		int32		pktCount = 0;
		while((buffer = myTcpWriteList.Pop()) != NULL)
		{
			pktCount ++;

			myComboSocket->Send(buffer);
			while(buffer)
			{
				MSB_WriteBuffer*	next = buffer->myNextBuffer;
				buffer->Release();
				buffer = next;
			}
		}

		myConnectionReady = true;

		if(myIsDeleting)
		{
			// We release here since we can't release in Delete() while
			// we're still connecting.
			Release();

			lock.Unlock();

			Close();
		}
		else if(myListener)
			myListener->AppendConnection(this);
	}
}

bool
MSB_Connection::PrivTryDetachFromStream(
	MSB_WriteableMemoryStream&	aStream,
	MSB_ReadMessage&			aReadMessage)
{
	if(aReadMessage.AttachToStream(aStream, myCurrentMessage))
	{
		MT_MutexLock	lock(myLock);
		myDetachedMessageCount --;
		if(myDetachedMessageCount == 0)
			myMessageStreamNotEmpty.ClearSignal();
		
		return true;
	}

	return false;
}

//
// TcpHandler
//

MSB_Connection::TcpHandler::TcpHandler(
	MSB_TCPSocket&		aSocket,
	MSB_Connection&		aConnection)
	: MSB_MessageHandler((MSB_Socket_Win&) aSocket)
	, myConnection(aConnection)
{
	myConnection.Retain();
}

bool
MSB_Connection::TcpHandler::OnInit(
	MSB_WriteMessage&	aWriteMessage)
{
	return true;
}

bool
MSB_Connection::TcpHandler::OnMessage(
	MSB_ReadMessage&	aReadMessage,
	MSB_WriteMessage&	aWriteMessage)
{
	MT_MutexLock		lock(myConnection.myTcpStreamLock);

	aReadMessage.DetachToStream(myConnection.myTcpMessageStream);

// 	if ( myConnection.myTcpMessageStream.GetUsed() > WARN_MAX_READ_QUEUE )
// 		LOG_WARN("MSB_Connection::TcpHandler::OnMessage data queue is %d bytes.", myConnection.myTcpMessageStream.GetUsed() );

	uint32				current = MT_ThreadingTools::Increment(&myConnection.myDetachedMessageCount);
	if(current == 1)
		myConnection.myMessageStreamNotEmpty.Signal();

	return true;
}

void
MSB_Connection::TcpHandler::OnClose()
{
	LOG_DEBUG3("TcpHandler closed");

	myConnection.myTcpHandler = NULL;		// Makes debugging a bit easier

	myConnection.myMessageStreamNotEmpty.Signal();
	myConnection.Close();
	myConnection.Release();

	delete this;
}

void
MSB_Connection::TcpHandler::OnError(
	MSB_Error			anError)
{
	MC_ERROR("MSB_Connection::TcpHandler::OnError Error=%d", anError);
	myConnection.myError = anError;
	myConnection.Close();
}

//
// UdpHandler
//
	
MSB_Connection::UdpHandler::UdpHandler(
	MSB_UDPSocket&		aSocket,
	MSB_Connection&		aConnection)
	: MSB_MessageHandler((MSB_Socket_Win&) aSocket)
	, myConnection(aConnection)
{
	myConnection.Retain();
}

bool
MSB_Connection::UdpHandler::OnInit(
	MSB_WriteMessage&	aWriteMessage)
{
	return true;
}

bool
MSB_Connection::UdpHandler::OnMessage(
	MSB_ReadMessage&	aReadMessage,
	MSB_WriteMessage&	aWriteMessage)
{
	MT_MutexLock		lock(myConnection.myUdpStreamLock);

	aReadMessage.DetachToStream(myConnection.myUdpMessageStream);

// 	if ( myConnection.myUdpMessageStream.GetUsed() > WARN_MAX_READ_QUEUE )
// 		LOG_WARN("MSB_Connection::UdpHandler::OnMessage data queue is %d bytes.", myConnection.myTcpMessageStream.GetUsed() );

	uint32				current = MT_ThreadingTools::Increment(&myConnection.myDetachedMessageCount);
	if(current == 1)
		myConnection.myMessageStreamNotEmpty.Signal();

	return true;
}

bool
MSB_Connection::UdpHandler::OnSystemMessage(
	MSB_ReadMessage&	aReadMessage,
	MSB_WriteMessage&	aWriteMessage,
	bool&				aUsed)
{
	MT_MutexLock		lock(myConnection.myLock);

	aUsed = true;

	switch(aReadMessage.GetDelimiter())
	{
		case MSB_SYSTEM_MESSAGE_UDP_PORT_CHECK:
			PrivHandleUdpPortMessage(aReadMessage);
			break;

		default:
			aUsed = false;
	}

	return true;
}

void
MSB_Connection::UdpHandler::OnClose()
{
	LOG_DEBUG3("UdpHandler closed; handler=%p", this);

	myConnection.myUdpHandler = NULL;

	myConnection.myMessageStreamNotEmpty.Signal();
	myConnection.Release();
	delete this;
}

void
MSB_Connection::UdpHandler::OnError(
	MSB_Error			anError)
{
	MC_ERROR("MSB_Connection::UdpHandler::OnError Error=%d", anError);
	myConnection.myError = anError;
	myConnection.Close();
}

void
MSB_Connection::UdpHandler::PrivHandleUdpPortMessage(
	MSB_ReadMessage& aReadMessage)
{
	MT_MutexLock			lock(myConnection.myLock);
	if(myConnection.myComboReady && myConnection.myListener)
	{
		myConnection.myComboSocket->HandshakeComplete();
		myConnection.PrivConnectionReady();
	}
}

//
// ReadStream
//

MSB_Connection::ReadStream::ReadStream()
	: myBufferList(NULL)
	, mySize(0)
	, myReadOffset(0)
{
	
}

MSB_Connection::ReadStream::~ReadStream()
{
	
}

void
MSB_Connection::ReadStream::SetBufferList(
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
MSB_Connection::ReadStream::GetUsed()
{
	return mySize;
}

uint32
MSB_Connection::ReadStream::Read(
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
MSB_Connection::ReadStream::Peek(
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
MSB_Connection::ReadStream::Write(
	const void*		aBuffer,
	uint32			aSize)
{
	assert(false && "Don't even try it");	
	return 0;
}

//
// WriteList
//

MSB_Connection::WriteList::WriteList()
	: myHead(NULL)
	, myTail(NULL)
{
	
}

MSB_Connection::WriteList::~WriteList()
{
	while(myHead)
	{
		Entry*		e = myHead;
		myHead = e->myNext;

		e->myBuffers->Release();
		delete e;
	}
}

MSB_WriteBuffer*
MSB_Connection::WriteList::Pop()
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
MSB_Connection::WriteList::Push(
	MSB_WriteBuffer*		aBuffer)
{
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

#endif // IS_PC_BUILD
