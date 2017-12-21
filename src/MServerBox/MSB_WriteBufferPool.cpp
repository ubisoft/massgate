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

#include "MSB_WriteBufferPool.h"
#include "MSB_WriteBuffer.h"
#include "MSB_Stats.h"
#include "MSB_SimpleCounterStat.h"

MSB_WriteBufferPool::MSB_WriteBufferPool()
	: myHeadBuffer(NULL)
{
}

MSB_WriteBufferPool::~MSB_WriteBufferPool()
{
}

MSB_WriteBufferPool*		
MSB_WriteBufferPool::GetInstance()
{
	static MSB_WriteBufferPool*		instance = new MSB_WriteBufferPool();
	return instance;
}

MSB_WriteBuffer*				
MSB_WriteBufferPool::GetWriteBuffer()
{
	MSB_WriteBuffer *t = NULL; 

	MT_MutexLock lock(myGlobalLock);

	if(myHeadBuffer)
	{
		t = myHeadBuffer;
		myHeadBuffer = t->myNextBuffer;
		t->myNextBuffer = NULL; 
		t->myWriteOffset = 0; 
	}

	return t;
}

void						
MSB_WriteBufferPool::ReleaseWriteBuffer(
	MSB_WriteBuffer* aWriteBuffer)
{
	MT_MutexLock lock(myGlobalLock);

	aWriteBuffer->myNextBuffer = myHeadBuffer;
	myHeadBuffer = aWriteBuffer;
}

int32
MSB_WriteBufferPool::FreeUnusedWriteBuffers()
{
	MSB_WriteBuffer *curr, *next;

	{
		MT_MutexLock lock(myGlobalLock);
		curr = myHeadBuffer;
		myHeadBuffer = NULL;
	}

	int32 numReleasedBuffers = 0;

	while(curr)
	{
		next = curr->myNextBuffer;
		delete curr;
		curr = next;
		numReleasedBuffers++;
	}	

#if IS_PC_BUILD
	MSB_StatsContext&		context = MSB_Stats::GetInstance().FindContext("ServerBox");
	(MSB_SimpleCounterStat&) context["NumFreeBuffers"] -= numReleasedBuffers;
#endif // IS_PC_BUILD

	return numReleasedBuffers;
}

