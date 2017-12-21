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

// #include "MC_Logger.h"

#include "MSB_UDPSocket.h"
#include "MSB_UDPWriteList.h"

#define LOG_DEBUG3(...)

MSB_UDPWriteList::MSB_UDPWriteList()
	: myWritePending(false)
{

}


MSB_UDPWriteList::~MSB_UDPWriteList()
{
	LOG_DEBUG3("Deallocating udp write list");

	Entry* entry = myWriteList.GetFirst();
	while (entry)
	{	
		myWriteList.Remove(entry);
		entry->Deallocate();
		entry = myWriteList.GetFirst();
	}
	
	entry = myInTransit.GetFirst();
	while (entry)
	{	
		myInTransit.Remove(entry);
		entry->Deallocate();
		entry = myInTransit.GetFirst();
	}
}

/**
 * Adds a list of write buffers for sending _without_ the standard udp-header.
 *
 */
void
MSB_UDPWriteList::Add(
	const sockaddr_in&		anAddr, 
	MSB_WriteBuffer*		aHeadBuffer)
{
	assert(aHeadBuffer != NULL);

	MT_MutexLock	lock(myLock);

	Entry* entry = Entry::Allocate();

	entry->myIncludeHeaderFlag = false;
	entry->myDest = anAddr;
	entry->myWriteBuffer = aHeadBuffer;

	//Set up the overlapped
	ZeroMemory(&entry->myOverlappedEntry.myOverlapped, sizeof(entry->myOverlappedEntry.myOverlapped));
	entry->myOverlappedEntry.myOverlapped.write = true;
	entry->myOverlappedEntry.mySelf = entry;

	// First buffer, always present.
	aHeadBuffer->Retain();
	entry->myWsaBuf[0].buf = (char *) aHeadBuffer->myBuffer;
	entry->myWsaBuf[0].len = aHeadBuffer->myWriteOffset;

	// Second buffer, might be present.
	if ( aHeadBuffer->myNextBuffer != NULL ) 
	{	
		MSB_WriteBuffer* buf = aHeadBuffer->myNextBuffer;

		buf->Retain();
		entry->myWsaBuf[1].buf = (char *) buf->myBuffer;
		entry->myWsaBuf[1].len = buf->myWriteOffset;

		//If WriteBuffer has a third component, we won't support it
		if ( buf->myNextBuffer ) 
		{
			MC_ERROR("Too long UDP WriteMessage.");
			assert(false && "Too long UDP WriteMessage.");
		}
	}
	myWriteList.Append(entry);
}

/**
 * Adds the writebuffers for sending, it will be prefixed with the standard udp-header.
 *
 */
void
MSB_UDPWriteList::Add(
	const MSB_UDPClientID	aClientID,
	const sockaddr_in&		anAddr,
	MSB_WriteBuffer*		aHeadBuffer)
{
	assert(aHeadBuffer != NULL);

	MT_MutexLock	lock(myLock);

	Entry* entry = Entry::Allocate();

	entry->myIncludeHeaderFlag = true;
	entry->myClientID = MSB_SWAP_TO_BIG_ENDIAN(aClientID);
	entry->myDest = anAddr;
	entry->myWriteBuffer = aHeadBuffer;

	//Set up the overlapped
	ZeroMemory(&entry->myOverlappedEntry.myOverlapped, sizeof(entry->myOverlappedEntry.myOverlapped));
	entry->myOverlappedEntry.myOverlapped.write = true;
	entry->myOverlappedEntry.mySelf = entry;

	//First buffer, client id
	entry->myWsaBuf[0].buf = (char *) &entry->myClientID;
	entry->myWsaBuf[0].len = sizeof(MSB_UDPClientID);

	//Second buffer, always present
	aHeadBuffer->Retain();
	entry->myWsaBuf[1].buf = (char *) aHeadBuffer->myBuffer;
	entry->myWsaBuf[1].len = aHeadBuffer->myWriteOffset;

	//Third buffer, might be present.
	if ( aHeadBuffer->myNextBuffer != NULL ) 
	{	
		MSB_WriteBuffer* buf = aHeadBuffer->myNextBuffer;

		buf->Retain();
		entry->myWsaBuf[2].buf = (char *) buf->myBuffer;
		entry->myWsaBuf[2].len = buf->myWriteOffset;

		//If WriteBuffer has a third component, we won't support it
		if ( buf->myNextBuffer ) 
		{
			MC_ERROR("Too long UDP WriteMessage.");
			assert(false && "Too long UDP WriteMessage.");
		}
	}
	myWriteList.Append(entry);
}

void 
MSB_UDPWriteList::RemoveFromInTransit(
	OVERLAPPED*			anOverlapped,
	MSB_Socket_Win*		anUDPSocket)
{
	MT_MutexLock	lock(myLock);

	Entry* entry = Entry::GetFromOverlapped(anOverlapped);

	myInTransit.Remove(entry);
	entry->Deallocate();

	anUDPSocket->Release(); // Write operation done
}

int 
MSB_UDPWriteList::Send(
	MSB_Socket_Win*		anUDPSocket)
{
	MT_MutexLock lock(myLock);

	if(myWriteList.IsEmpty())
		return 0;

	Entry* entry = myWriteList.GetFirst();

	while(entry)
	{
		myWriteList.Remove(entry);

		//We can have a max of two writeBuffer on any UDP write. Its a set limit.
		//Add one for the UDP client/cookie id
		int bufferCount; 
		if(entry->myIncludeHeaderFlag)
		{
			if (entry->myWriteBuffer->myNextBuffer)
				bufferCount = 3;
			else
				bufferCount = 2;
		}
		else
		{
			if (entry->myWriteBuffer->myNextBuffer)
				bufferCount = 2;
			else
				bufferCount = 1;
		}

		//The overlapped entry has already been setup
		DWORD tmp;
		int		retval;
		retval = WSASendTo(anUDPSocket->GetSocket(), entry->myWsaBuf, bufferCount, 
			&tmp, 0, (struct sockaddr*) &entry->myDest, sizeof(entry->myDest),
			(LPOVERLAPPED) &entry->myOverlappedEntry, NULL);
		if(retval == SOCKET_ERROR )
		{
			int32		error = WSAGetLastError();
			if(error != WSA_IO_PENDING) 
			{
				entry->Deallocate();	//Just discard the message
				return error;
			}
		}
		anUDPSocket->Retain();			//In flight send operation
		
		myInTransit.Append(entry);

		entry = myWriteList.GetFirst();
	}

	return 0;
}

//
// Entry
// 

MSB_UDPWriteList::Entry::Entry()
{
}

MSB_UDPWriteList::Entry::~Entry()
{
	MSB_WriteBuffer* buf = myWriteBuffer;
	while(buf)
	{
		MSB_WriteBuffer* next = buf->myNextBuffer;
		buf->Release();
		buf = next;
	}
}

MSB_UDPWriteList::Entry*
MSB_UDPWriteList::Entry::Allocate()
{	
	EntryPool&		pool = EntryPool::GetInstance();
	return pool.GetEntry();
}

MSB_UDPWriteList::Entry*
MSB_UDPWriteList::Entry::GetFromOverlapped(
	OVERLAPPED* anOverlapped)
{
	return ((OverlappedEntry*) anOverlapped)->mySelf;
}

void 
MSB_UDPWriteList::Entry::Deallocate()
{
	MSB_WriteBuffer*		current = myWriteBuffer;
	while(current)
	{
		MSB_WriteBuffer*	next = current->myNextBuffer;
		current->Release();
		current = next;
	}
	
	myWriteBuffer = NULL;

	EntryPool&		pool = EntryPool::GetInstance();
	pool.PutEntry(this);
}

//
// EntryPool
//

MSB_UDPWriteList::EntryPool MSB_UDPWriteList::EntryPool::ourInstance;

MSB_UDPWriteList::Entry* 
MSB_UDPWriteList::EntryPool::GetEntry()
{
	Entry*			e = NULL;
	MT_MutexLock	lock(myGlobalLock);

	if (myStack)
	{
		e = myStack;
		myStack = myStack->myNext;
	}

	if ( e == NULL )
		e = new Entry();

	return e;
}

void 
MSB_UDPWriteList::EntryPool::PutEntry(
	Entry*			anEntry)
{
	MT_MutexLock lock(myGlobalLock);

	anEntry->myNext = myStack;
	myStack = anEntry;
}

#endif // IS_PC_BUILD
