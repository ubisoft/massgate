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
#include "MT_Task.h"
#include "MT_Mutex.h"
#include "MT_Supervisor.h"
#include "MT_Job.h"
#include "MC_SmallObjectAllocator.h"
#include "MT_ThreadingTools.h"


// Execute calls the user defined function with the user defined data and then
// marks the thread as free.
void MT_Task::Execute()
{
	CT_ASSERT(sizeof(MT_Task) == 64);	// We always want this for cache efficiency.

	myFunctionPointer(&myData);

	if (myJob)
		_InterlockedDecrement(&myJob->myNumPendingTasks);
}

void MT_TaskRunnerCallback(void* aData)
{
	MT_TaskRunner* taskRunner = (MT_TaskRunner*)aData;
	taskRunner->RunTask();
}


#ifdef MC_USE_SMALL_OBJECT_ALLOCATOR

// Lockless allocator for the Tasks for zero-context-switching performance

__declspec(align(64))	// cache friendly alignment
volatile long locQuickMutex = 0;

__declspec(align(64))	// cache friendly alignment
static MC_SmallObjectAllocator<sizeof(MT_Task), 512> locTaskAllocator;

void* MT_Task::operator new(size_t aSize)
{
	for(int i=0; i<400; i++)
	{
		if(0 == _InterlockedCompareExchange(&locQuickMutex, 1, 0))
		{
			void* p = locTaskAllocator.Allocate();
			_InterlockedDecrement(&locQuickMutex);
			return p;
		}
#if IS_PC_BUILD
		__asm wait
		__asm wait
#endif
		// MSV:NW - Is there a nop instruction or something similar on the 360?
	}

	while(true)
	{
		if(0 == _InterlockedCompareExchange(&locQuickMutex, 1, 0))
		{
			void* p = locTaskAllocator.Allocate();
			_InterlockedDecrement(&locQuickMutex);
			return p;
		}

		::SwitchToThread();
	}
}

void MT_Task::operator delete(void* aPointer)
{
	for(int i=0; i<400; i++)
	{
		if(0 == _InterlockedCompareExchange(&locQuickMutex, 1, 0))
		{
			locTaskAllocator.Free(aPointer);
			_InterlockedDecrement(&locQuickMutex);
			return;
		}

#if IS_PC_BUILD
		__asm wait
		__asm wait
#endif
		// MSV:NW - Is there a nop instruction or something similar on the 360?
	}

	while(true)
	{
		if(0 == _InterlockedCompareExchange(&locQuickMutex, 1, 0))
		{
			locTaskAllocator.Free(aPointer);
			_InterlockedDecrement(&locQuickMutex);
			return;
		}

		::SwitchToThread();
	}
}

#endif

