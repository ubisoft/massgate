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

// #include "MC_Logger.h"

#include "MSB_SimpleCounterStat.h"
#include "MSB_Stats.h"
#include "MSB_StatsContext.h"
#include "MSB_Types.h"
#include "MSB_WriteBuffer.h"

#include "MT_ThreadingTools.h"

MSB_WriteBuffer::MSB_WriteBuffer()
	: myBufferSize(MSB_WRITEBUFFER_DEFAULT_SIZE)
	, myWriteOffset(0)
	, myNextBuffer(NULL)
	, myRefCount(0)
	, myAllocationPool(NULL)
{
}

MSB_WriteBuffer::~MSB_WriteBuffer()
{
}



MSB_WriteBuffer*
MSB_WriteBuffer::PrivAllocateWin()
{
#if IS_PC_BUILD

	MSB_WriteBuffer* writeBuffer;
	MSB_WriteBufferPool* allocationPool = MSB_WriteBufferPool::GetInstance(); 
	writeBuffer = allocationPool->GetWriteBuffer();

	MSB_StatsContext&		context = MSB_Stats::GetInstance().FindContext("ServerBox");
	if(!writeBuffer)
	{
		++ (MSB_SimpleCounterStat&) context["NumAllocatedBuffers"];
		++ (MSB_SimpleCounterStat&) context["NumFreeBuffers"];
		writeBuffer = new MSB_WriteBuffer();
	}

	-- (MSB_SimpleCounterStat&) context["NumFreeBuffers"];
	++ (MSB_SimpleCounterStat&) context["NumCheckedOutBuffers"];

	writeBuffer->myAllocationPool = allocationPool; 

	writeBuffer->Retain();

	return writeBuffer; 

#endif // IS_PC_BUILD
	
	return NULL;
}


MSB_WriteBuffer*
MSB_WriteBuffer::PrivAllocateConsole()
{	
	return NULL; 
}


MSB_WriteBuffer*
MSB_WriteBuffer::Allocate()
{
	return PrivAllocateWin();
}

/**
 * Recycles the memory used by the buffer, as appropriate to how it was
 * allocated.
 */
void
MSB_WriteBuffer::Deallocate()
{
	if(myAllocationPool)
	{
#if IS_PC_BUILD
		MSB_StatsContext&		context = MSB_Stats::GetInstance().FindContext("ServerBox");
		++ (MSB_SimpleCounterStat&) context["NumFreeBuffers"];
		-- (MSB_SimpleCounterStat&) context["NumCheckedOutBuffers"];
#endif // IS_PC_BUILD

		myAllocationPool->ReleaseWriteBuffer(this);
	}
	else
	{
		delete this;
	}
}

void
MSB_WriteBuffer::Retain()
{
	int32	ref = MT_ThreadingTools::Increment(&myRefCount);
	assert(ref > 0 && "Invalid MSB_WriteBuffer ref count");
}

void
MSB_WriteBuffer::Release()
{
	int32	ref = MT_ThreadingTools::Decrement(&myRefCount);
	assert(ref >= 0 && "Invalid MSB_WriteBuffer ref count");

	if(ref == 0)
		Deallocate();
}

void
MSB_WriteBuffer::RetainAll()
{
	MSB_WriteBuffer*		current = this;
	while(current)
	{
		current->Retain();
		current = current->myNextBuffer;
	}
}

void
MSB_WriteBuffer::ReleaseAll()
{
	MSB_WriteBuffer*		current = this;
	while(current)
	{
		MSB_WriteBuffer*	next = current->myNextBuffer;
		current->Release();
		current = next;
	}
}
