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
#include "MT_ThreadingTools.h"
#include "MT_Mutex.h"
#include "MC_Profiler.h"
#include "MT_ThreadingTools.h"

#include "MT_Thread.h"
#include "MC_debug.h"
#include "MC_StackWalker.h"

void MC_Sleep(int aMillis)
{
	if(aMillis < 0)
		aMillis = 0;

	MC_THREADPROFILER_ENTER_WAIT();

	::Sleep(aMillis);

	MC_THREADPROFILER_LEAVE_WAIT();
}

void MC_Yield()
{
	MC_THREADPROFILER_ENTER_WAIT();

	::SwitchToThread();
	//::Sleep(0);

	MC_THREADPROFILER_LEAVE_WAIT();
}

unsigned int MT_ThreadingTools::GetLogicalProcessorCount()
{
#if IS_PC_BUILD	// PC specific
	// Intel ref: http://cache-www.intel.com/cd/00/00/27/66/276611_276611.txt
	// AMD ref: http://www.amd.com/us-en/assets/content_type/white_papers_and_tech_docs/25481.pdf#search=%22amd%20cpuid%20specification%22

	unsigned int maxInputValue = 0;

	// Get standard max input value and vendor ID string ("AuthenticAMD" or "GenuineIntel").
	__asm		
	{
		xor eax, eax
    	cpuid
		mov maxInputValue, eax
	}		

	if(maxInputValue < 1)
		return 1;

	unsigned int htt;
	unsigned int logicalProcessorCount;

	__asm
	{
		mov eax, 1
		cpuid

		shr ebx, 16
		and ebx, 255
		mov logicalProcessorCount, ebx

		shr edx, 28
		and edx, 1
		mov htt, edx
	}

	// When HTT=0, LogicalProcessorCount is reserved and the processor contains one CPU core and that one CPU core is single-threaded.
	if(!htt)
		return 1;

	return logicalProcessorCount;
#else
	// SWFM:SWR - currently all other platforms default to 1
	return 1;
#endif
}

unsigned int MT_ThreadingTools::GetProcessorAPICID()
{
#if IS_PC_BUILD	// PC specific function
	//  In EBX, bits 31..24 gives the default APIC ID, bits 23..16 gives Logical
	//  Processor Count, bits 15..8 gives the Cache Flush Chunk size and
	//  bits 7..0 give away the Brand ID(Only Pentium III and upwards).

	// Initial APIC ID. This field contains the initial value of the processor’s local APIC
	// physical ID register. This value is composed of the Northbridge NodeID (bits 26–24)
	// and the CPU number within the node (bits 31–27). Subsequent writes by software to
	// the local APIC physical ID register do not change the value of the initial APIC ID field.

	unsigned int extraInfo;

	__asm push eax
	__asm push ebx
	__asm push ecx
	__asm push edx
	__asm mov eax, 1
	__asm cpuid
	__asm mov extraInfo, ebx
	__asm pop edx
	__asm pop ecx
	__asm pop ebx
	__asm pop eax

	return (extraInfo >> 24) & 255;
#else
	return 0;
#endif
}



#ifdef THREAD_TOOLS_DEBUG

struct DebugThreadInfo
{
	DebugThreadInfo()
	{
		myName[0]  = '\0';
		myThreadId = 0;
		mySemaWaiting = false;
		myCriticalWaiting = false;
	}
	char			myName[128];
	unsigned int	myThreadId;
	bool			mySemaWaiting;
	bool			myCriticalWaiting;
};

DebugThreadInfo locDebugThreadInfo[THREAD_TOOLS_ABSOLUTE_MAX_NUM_THREADS];

void MT_ThreadingTools::SetSemaphoreStatus( bool aWaiting)
{
	unsigned int idx = GetMyThreadIndex();
	locDebugThreadInfo[idx].mySemaWaiting = aWaiting;
}

void MT_ThreadingTools::SetCriticalSectionStatus( bool aWaiting)
{
	unsigned int idx = GetMyThreadIndex();
	locDebugThreadInfo[idx].myCriticalWaiting = aWaiting;
}



// Print status of all known threads
bool MT_ThreadingTools::PrintThreadList( char *aBuffer, unsigned int aBufferSize)
{
	if( aBuffer && aBufferSize > 0)
		aBuffer[0] = 0;

	char tmp[256];
	MC_String str;

	str += "\n------------------------------------------------------\n";
	str += "Thread status:\n";
	str += "   No.  Handle       ThreadID     cr se su bo pr   Name\n";
	str += "------------------------------------------------------\n";
	unsigned int currentThreadId = GetCurrentThreadId();

	for( int i=0; i<THREAD_TOOLS_ABSOLUTE_MAX_NUM_THREADS; i++)
	{
		if( locDebugThreadInfo[i].myThreadId && locDebugThreadInfo[i].myName[0] )
		{
			HANDLE threadhandle = OpenThread( THREAD_ALL_ACCESS, false, locDebugThreadInfo[i].myThreadId);
			if( threadhandle )
			{
				// Get suspend count by suspending and resuming.
				// It's important that we don't suspend the current thread!
				DWORD suspendcount = 0;
				if( locDebugThreadInfo[i].myThreadId != currentThreadId )
				{
					suspendcount = SuspendThread(threadhandle);
					if( suspendcount != (DWORD)-1)
						ResumeThread(threadhandle);
				}

				BOOL boost = false;
				int prio = GetThreadPriority( threadhandle);
				GetThreadPriorityBoost( threadhandle, &boost);

				char marker = ' ';
				if( locDebugThreadInfo[i].myThreadId == currentThreadId )
					marker = '*';

				sprintf(tmp, " %c %03d - 0x%08x - 0x%08x -  %d  %d  %d  %d  %d - \"%s\"\n",
					marker,
					(unsigned int) i,
					(unsigned int) threadhandle,
					(unsigned int) locDebugThreadInfo[i].myThreadId,
					(int) locDebugThreadInfo[i].myCriticalWaiting,
					(int) locDebugThreadInfo[i].mySemaWaiting,
					(int) suspendcount,
					(int) boost,
					(int) prio,
					locDebugThreadInfo[i].myName
				);

				CloseHandle(threadhandle);
				str += tmp;
			}
		}
	}

	// Dump to debug output
	MC_DEBUG(str);

	// Write back to buffer
	if( aBuffer && aBufferSize > 1)
	{
		unsigned int len = str.GetLength();
		if( len >= aBufferSize)
		{
			len = (aBufferSize-1);
			str[len] = 0;
		}
		strcpy( aBuffer, str);
	}

	return true;
}

extern MC_StackWalker* locSw;

// Print callstack for a thread
bool MT_ThreadingTools::PrintThreadCallStack( unsigned int aId, char *aBuffer, unsigned int aBufferSize)
{
	unsigned int currentThreadId = GetCurrentThreadId();
	if( locDebugThreadInfo[aId].myThreadId && locDebugThreadInfo[aId].myName[0] )
	{
		if( locDebugThreadInfo[aId].myThreadId != currentThreadId )
		{
			HANDLE threadhandle = OpenThread( THREAD_ALL_ACCESS, false, locDebugThreadInfo[aId].myThreadId);
			if( threadhandle)
			{
				MC_String str;

				str += "\n------------------------------------------------------\n";
				str += MC_Strfmt<256>("   %03d - \"%s\"\n", aId, locDebugThreadInfo[aId].myName);
				str += "------------------------------------------------------\n";


				if( SuspendThread(threadhandle) != DWORD(-1) )
				{
					MC_StaticString<8192> output = "";
					if( locSw )
						locSw->ShowCallstack(output, threadhandle);
					str += output;
					ResumeThread(threadhandle);
				}

				CloseHandle(threadhandle);

				// dump to debug output
				MC_DEBUG(str);

				// write back to buffer
				if( aBuffer && aBufferSize > 1)
				{
					unsigned int len = str.GetLength();
					if( len >= aBufferSize)
					{
						len = aBufferSize-1;
						str[len] = 0;
					}
					strcpy(aBuffer, str);
				}

				return true;
			}
		}
	}
	return false;
}

// Print status of all threads
bool MT_ThreadingTools::PrintThreadingStatus( bool aAllFlag, char *aBuffer, unsigned int aBufferSize)
{
	MC_String str;
	char tempstring[32 * 1024];
	memset(tempstring, 0, (32 * 1024) );

	// Thread list
	if( PrintThreadList(tempstring, 8191) )
	{
		str += tempstring;
	}

	// Callstack for all waiting threads
	for( int i=0; i<THREAD_TOOLS_ABSOLUTE_MAX_NUM_THREADS; i++)
	{
		if( aAllFlag || (locDebugThreadInfo[i].mySemaWaiting || locDebugThreadInfo[i].myCriticalWaiting) )
		{
			if( PrintThreadCallStack( i, tempstring, (32 * 1024) ))
			{
				str += tempstring;
			}
		}
	}

	// Write back to buffer
	if( aBuffer && aBufferSize > 1 )
	{
		unsigned int len = str.GetLength();
		if( len >= aBufferSize )
		{
			len = aBufferSize-1;
			str[len] = 0;
		}
		strcpy(aBuffer, str);
	}

	return true;
}


#endif //THREAD_TOOLS_DEBUG




void MT_ThreadingTools::SetCurrentThreadName(const char* aName)
{
#ifndef _RELEASE_
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType; // must be 0x1000
		LPCSTR szName; // pointer to name (in user addr space)
		DWORD dwThreadID; // thread ID (-1=caller thread)
		DWORD dwFlags; // reserved for future use, must be zero
	} THREADNAME_INFO;

	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = aName;
	info.dwThreadID = -1;
	info.dwFlags = 0;

	__try{
		RaiseException(0x406D1388, 0, sizeof(info)/sizeof(DWORD), (DWORD*)&info);
	}
	__except (EXCEPTION_CONTINUE_EXECUTION)
	{
	}

#ifdef THREAD_TOOLS_DEBUG
	unsigned int idx = GetMyThreadIndex();
	strcpy( locDebugThreadInfo[idx].myName, aName);
	locDebugThreadInfo[idx].myThreadId = GetCurrentThreadId();
#endif

	MC_PROFILER_SET_THREAD_NAME(aName);
#endif
}


static __declspec(thread) long locMyThreadIndex = -1; 
static volatile long locThreadCount = 0;

int MT_ThreadingTools::GetMyThreadIndex()
{
	if(locMyThreadIndex == -1)
	{
		locMyThreadIndex = Increment(&locThreadCount)-1;

		assert(locMyThreadIndex >= 0 && locMyThreadIndex < THREAD_TOOLS_ABSOLUTE_MAX_NUM_THREADS);
	}

	return locMyThreadIndex;
}

long MT_ThreadingTools::Increment(long volatile* aValue)
{
	return _InterlockedIncrement(aValue);
}

long MT_ThreadingTools::Decrement(long volatile* aValue)
{
	return _InterlockedDecrement(aValue);
}

