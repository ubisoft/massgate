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

#if IS_PC_BUILD

// #include "MC_Logger.h"
#include "MSB_TCPSocket.h"
#include "MSB_TCPWriteList.h"
#include "MSB_WriteBuffer.h"

#define LOG_WARN(...)

MSB_TCPWriteList::MSB_TCPWriteList(
	uint32			aMaxOutgoingLength)
	: myTcpSocket(NULL)
	, myLock()
	, myMaxOutgoingLength(aMaxOutgoingLength)
{
	memset(&myTransitBuffers, 0, sizeof(myTransitBuffers));
	memset(&myWsaBuffs, 0, sizeof(myWsaBuffs));
}

MSB_TCPWriteList::~MSB_TCPWriteList()
{
	Entry*		e = myWriteList.GetFirst();
	while(e)
	{
		Entry*	next = e->myNext;
		e->Release();
		e = next;
	}
}

void
MSB_TCPWriteList::SetMaxOutgoingLength(
	uint32 aMaxOutgoingLength)
{
	myMaxOutgoingLength = aMaxOutgoingLength;
}

void
MSB_TCPWriteList::SetSocket(
	MSB_TCPSocket* aSocket)
{
	assert(myTcpSocket == NULL);

	myTcpSocket = aSocket;
}

int32
MSB_TCPWriteList::SendComplete()
{
	for(uint32 i = 0; myTransitBuffers[i].myEntry && i < NUM_TRANSIT_BUFFERS; i++)
	{
		myTransitBuffers[i].myEntry->BufferSent(myTransitBuffers[i].myBuffer);
		if(myTransitBuffers[i].myEntry->IsEmpty())
		{
			myWriteList.Remove(myTransitBuffers[i].myEntry);
			myCurrentOutgoingLength -= myTransitBuffers[i].myBuffer->myWriteOffset;
			myTransitBuffers[i].myEntry->Release();
		}
	}

	memset(&myTransitBuffers, 0, sizeof(myTransitBuffers));

	return PrivStartNextSend();
}

int32
MSB_TCPWriteList::Add(	
	MSB_WriteBuffer*		aBufferList)
{
	MSB_WriteBuffer*		current = aBufferList;

	while(current)
	{
		myCurrentOutgoingLength += current->myWriteOffset;
		current = current->myNextBuffer;
	}
	
	if(myCurrentOutgoingLength > myMaxOutgoingLength)
	{
		LOG_WARN("Outgoing buffer exceeded max length ( %d bytes )", myCurrentOutgoingLength);
		myTcpSocket->ProtSetError(MSB_ERROR_OUTGOING_BUFFER_FULL, -1);
		return -1;
	}
	
	myWriteList.Append(EntryPool::GetInstance().GetEntry(aBufferList));	

	return PrivStartNextSend();
}

int32
MSB_TCPWriteList::PrivStartNextSend()
{
	if(myTransitBuffers[0].myBuffer != NULL)
		return 0;		// We're already sending something

	if(myWriteList.IsEmpty())
		return 0;

 	uint32		count = 0;
 	Entry*		entry = myWriteList.GetFirst();
 	for(uint32 i = 0; entry && count < NUM_TRANSIT_BUFFERS; entry = entry->myNext)
		count += entry->StartNextSend(&myTransitBuffers[count], &myWsaBuffs[count], NUM_TRANSIT_BUFFERS - count);

	if(count)
	{
		ZeroMemory(&myOverlapped, sizeof(myOverlapped));
		myOverlapped.write = true;

		DWORD			tmp;
		if(WSASend(myTcpSocket->GetSocket(), myWsaBuffs, count, 
			&tmp, 0, (LPOVERLAPPED) &myOverlapped, NULL) == SOCKET_ERROR)
		{
			int32		error = WSAGetLastError();
			if(error != WSA_IO_PENDING)
			{
				myTcpSocket->ProtSetError(MSB_ERROR_SYSTEM, WSAGetLastError());
				return error;
			}
		}

		myTcpSocket->Retain(); //Starting one operation
	}

	return 0;
}
 
MSB_TCPWriteList::Entry::Entry(
	MSB_WriteBuffer*		aBufferList)
	: myBufferList(NULL)
	, myTransitHead(NULL)
	, myTransitTail(NULL)
{
	SetBufferList(aBufferList);
}

void
MSB_TCPWriteList::Entry::Release()
{
	MSB_WriteBuffer*		current = myBufferList;
	while(current)
	{
		MSB_WriteBuffer*	next = current->myNextBuffer;
		current->Release();
		current = next;
	}
	
	myBufferList = myTransitHead = myTransitTail = NULL;
	EntryPool::GetInstance().PutEntry(this);
}

bool
MSB_TCPWriteList::Entry::IsEmpty()
{
	return myBufferList == NULL && myTransitHead == NULL;
}

void
MSB_TCPWriteList::Entry::BufferSent(
	MSB_WriteBuffer*		aBuffer)
{
	assert(aBuffer == myTransitHead);
	if(myTransitHead == myTransitTail)
		myTransitHead = myTransitTail = NULL;
	else
		myTransitHead = aBuffer->myNextBuffer;

	aBuffer->Release();
}

uint32
MSB_TCPWriteList::Entry::StartNextSend(
	TransitBuffer*			aTransitList,
	WSABUF*					aWsaList,
	uint32					aMaxCount)
{
	MSB_WriteBuffer*		current = myBufferList;
	uint32					count = 0;

	myTransitHead = myBufferList;

	for(; count < aMaxCount && current; count ++, current = current->myNextBuffer)
	{
		aTransitList->myBuffer = current;
		aTransitList->myEntry = this;
		aTransitList++;

		aWsaList->buf = (char*) current->myBuffer;
		aWsaList->len = current->myWriteOffset;
		aWsaList++;

		myTransitTail = current;
	}
	
	myBufferList = current;
	
	return count;
}

void
MSB_TCPWriteList::Entry::SetBufferList(
	MSB_WriteBuffer*		aBufferList)
{
	assert(myBufferList == NULL);
	assert(myTransitHead == NULL);
	myBufferList = aBufferList;
	MSB_WriteBuffer*		current = aBufferList;
	while(current)
	{
		current->Retain();
		current = current->myNextBuffer;
	}
}

//
// EntryPool
//

MSB_TCPWriteList::EntryPool		MSB_TCPWriteList::EntryPool::ourInstance;

MSB_TCPWriteList::Entry* 
MSB_TCPWriteList::EntryPool::GetEntry(
	MSB_WriteBuffer*		aBufferList)
{
	static volatile LONG	lastRequest = 0;

	Entry*			e = NULL;
	MT_MutexLock	lock(myGlobalLock);

	if (myStack)
	{
		e = myStack;
		myStack = myStack->myNext;
	}

	if ( e == NULL )
		e = new Entry(aBufferList);
	else
		e->SetBufferList(aBufferList);

	return e;
}

void 
MSB_TCPWriteList::EntryPool::PutEntry(
	Entry*			anEntry)
{
	MT_MutexLock lock(myGlobalLock);

	anEntry->myNext = myStack;
	myStack = anEntry;
}

#endif // IS_PC_BUILD
