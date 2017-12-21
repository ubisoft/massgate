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
#pragma once 
#ifndef MT_THREADING_TOOLS_H
#define MT_THREADING_TOOLS_H

#if IS_PC_BUILD
	#include "mc_mem.h"
	#pragma push_macro("assert")
	#undef assert
	#include <intrin.h>
	#pragma pop_macro("assert")
#endif

#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedCompareExchange)

#pragma intrinsic(_InterlockedAnd)
#pragma intrinsic(_InterlockedOr)

#if IS_PC_BUILD	// PC only intrinsics
	#pragma intrinsic(_WriteBarrier)
	#pragma intrinsic(_ReadBarrier)
	#pragma intrinsic(_ReadWriteBarrier)
#endif

#ifndef _RELEASE_
#define THREAD_TOOLS_DEBUG
#endif

#define THREAD_TOOLS_ABSOLUTE_MAX_NUM_THREADS (256)

void MC_Sleep(int aMillis);
void MC_Yield();

namespace MT_ThreadingTools
{
	unsigned int GetLogicalProcessorCount();
	unsigned int GetProcessorAPICID();

	void SetCurrentThreadName(const char* aName);

	/* int GetMyThreadIndex()
	*
	* Returns: An index 0..n of the current running thread in the application.
	*
	* Comments: Assigns an Index for the current thread in the application, and returns it.
	* The indices are sequential and per thread (i.e Thread A will always get id 0, B always id 1, etc).
	* This is useful when having thread-specific stacks (e.g. see MN_WriteMessage).

	* Usage: Given a class that has a static member variable, like
	* static unsigned char Classname::ourBuffer[256];
	* This will most likely fail in a multithreaded environment, but we still want all instances in the same thread to
	* share the variable - then do like this:
	*
	* static unsigned char ourBufferPerThread[THREAD_TOOLS_ABSOLUTE_MAX_NUM_THREADS][256]; // or dynamic alloc but you get the idea
	* ...and in the ctor:
	*    int threadIndex;
	*    GetMyThreadIndex(threadIndex);
	*    ourBuffer = ourBufferPerThread[threadIndex]; // simple one-time init in the constructor. No performance loss later.
	*
	*/
	int GetMyThreadIndex();


	long Increment(long volatile* aValue);
	long Decrement(long volatile* aValue);


#ifdef THREAD_TOOLS_DEBUG
	void SetSemaphoreStatus( bool aWaiting);
	void SetCriticalSectionStatus( bool aWaiting);
	bool PrintThreadList( char *aBuffer = NULL, unsigned int aBufferSize = 0);
	bool PrintThreadCallStack( unsigned int aId, char *aBuffer = NULL, unsigned int aBufferSize = 0);
	bool PrintThreadingStatus( bool aAllFlag = false, char *aBuffer = NULL, unsigned int aBufferSize = 0);
#else
	inline void SetSemaphoreStatus( bool aWaiting)																	{}
	inline void SetCriticalSectionStatus( bool aWaiting)															{}
	inline bool PrintThreadList( char *aBuffer = NULL, unsigned int aBufferSize = 0)								{ return false; }
	inline bool PrintThreadCallStack( unsigned int aId, char *aBuffer = NULL, unsigned int aBufferSize = 0)			{ return false; }
	inline bool PrintThreadingStatus( bool aAllFlag = false, char *aBuffer = NULL, unsigned int aBufferSize = 0)	{ return false; }
#endif
}

#endif //MT_THREADING_TOOLS_H

