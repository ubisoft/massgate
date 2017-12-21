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
#include "mn_packet.h"
#include "MC_Debug.h"
#include "MC_Platform.h"

#include "MT_Mutex.h"


static MC_GrowingArray<MN_Packet*>	locPacketArray;
static MT_Mutex	locPacketAllocatorMutex;

MT_Mutex& GetLocPacketAllocatorMutex()
{
	return locPacketAllocatorMutex;
}


MN_Packet* MN_Packet::Create(unsigned int theTransportSize)
{
	MT_MutexLock locker(GetLocPacketAllocatorMutex());
	MN_Packet* newPacket = GetNewPacket(theTransportSize);
	assert( newPacket );
	newPacket->Clear();
	return newPacket;
}

MN_Packet* MN_Packet::Create(const MN_Packet* aPacket)
{
	MT_MutexLock locker(GetLocPacketAllocatorMutex());
	MN_Packet* newPacket = GetNewPacket(aPacket->myRawDataBufferLen);
	assert( newPacket );
	assert( newPacket != aPacket );
	*newPacket = *aPacket;
	return newPacket;
}


MN_Packet::MN_Packet(unsigned char* anAllocatedBuffPtr, unsigned int theTransportSize)
: myRawDataBufferLen(theTransportSize)
, myRawDataBuffer(anAllocatedBuffPtr)
, myTransportSize(theTransportSize)
, myWriteOffset(*(unsigned short*)anAllocatedBuffPtr)
{
	assert(anAllocatedBuffPtr);
	Clear();
}

void MN_Packet::Deallocate(MN_Packet*& aPacket)
{
	MT_MutexLock locker(GetLocPacketAllocatorMutex());
	assert(locPacketArray.IsInited());
#ifdef _DEBUG
	for(int i = 0; i < locPacketArray.Count(); i++)
	{
		assert(locPacketArray[i] != aPacket);
	}
#endif

	if (locPacketArray.Count() < 128)
	{
		locPacketArray.Add(aPacket);
	}
	else
	{
		delete aPacket;
	}
	aPacket = NULL;
}


MN_Packet::~MN_Packet()
{
	delete [] myRawDataBuffer;
}

void
MN_Packet::AppendData(const void* someData, unsigned int someDatalen)
{
	if(GetWriteOffset() + sizeof(short) + someDatalen > myRawDataBufferLen)
	{
		MC_DEBUG("FATAL: Too much data for packet, discarding");
		return;
	}

	memcpy(myRawDataBuffer+sizeof(short)+GetWriteOffset(), someData, someDatalen);
	myWriteOffset += someDatalen;
}


void MN_Packet::Clear()
{
	myWriteOffset = 0;
}

void MN_Packet::operator=(const MN_Packet& aPacket)
{
	assert(myRawDataBuffer);
	assert(myRawDataBufferLen >= aPacket.myRawDataBufferLen);

	memcpy( myRawDataBuffer, aPacket.GetBinaryData(), aPacket.GetBinarySize()); // implicit copy of dataSize
	myTransportSize = aPacket.myTransportSize;
}


MN_Packet* MN_Packet::GetNewPacket(unsigned int theTransportSize)
{
	MT_MutexLock locker(GetLocPacketAllocatorMutex());
	MN_Packet* newPacket = NULL;
	if (!locPacketArray.IsInited())
		locPacketArray.Init(128,128,false);

	for (int i = 0; i < locPacketArray.Count(); i++)
	{
		if (locPacketArray[i]->myTransportSize == theTransportSize)
		{
			newPacket = locPacketArray[i];
			locPacketArray.RemoveCyclicAtIndex(i);
			return newPacket;
		}
	}

	newPacket = new MN_Packet(new unsigned char[theTransportSize], theTransportSize);
	return newPacket;
}

void MN_Packet::CleanUp()
{
	MT_MutexLock locker(GetLocPacketAllocatorMutex());
	for (int i = 0; i < locPacketArray.Count(); i++)
	{
		delete locPacketArray[i];
		locPacketArray.RemoveCyclicAtIndex(i--);
	}
}
