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
#ifndef MMS_HEAPIMPLEMENTATION_H
#define MMS_HEAPIMPLEMENTATION_H

#include "MC_EggClockTimer.h"
#include "MC_HashMap.h"
#include "MC_StackWalker.h"

#include "MMS_Constants.h"
#include "MMS_MasterServer.h"

#include "MT_Thread.h"
#include "MT_ThreadingTools.h"

#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>

#ifdef new
#undef new 
#endif 

class MMS_AllocationLogger : MT_Thread
{
public: 
	static MMS_AllocationLogger& GetInstance()
	{
		static MMS_AllocationLogger* instance = new MMS_AllocationLogger();
		return *instance; 
	}
	
	void Run()
	{
		for(;;)
		{
			if(MMS_Settings::TrackMemoryAllocations)
			{
				uint32 startTime = GetTickCount(); 
				PrivFlushToFile(); 
				GENERIC_LOG_EXECUTION_TIME(MMS_AllocationLogger::Run(), startTime); 
			}
			
		
			MC_Sleep(1000); 
		}
	}

	void AddSample()
	{
		MC_StackWalker stackWalker;
		MC_StackTrace stackTrace; 
		stackWalker.SaveCallstack(&stackTrace);

		MT_MutexLock lock(myGlobLock);

		if(myFrontBuffer->Exists(stackTrace))
		{
			Allocation& t = myFrontBuffer->operator[](stackTrace);
			t.myNumCalls++; 
		}
		else
		{
			Allocation t(stackTrace);
			myFrontBuffer->operator [](stackTrace) = t; 
		}
	}

private: 
	MMS_AllocationLogger()
		: myABuffer(1024)
		, myBBuffer(1024)
		, myFrontBuffer(&myABuffer)
		, myFlushTimer(5 * 60 * 1000)
	{
		Start();
	}

	class Allocation
	{
	public: 
		Allocation()
			: myNumCalls(0)
		{
		}

		Allocation(
			MC_StackTrace& aStackTrace)
			: myStackTrace(aStackTrace)
			, myNumCalls(1)
		{
		}

		MC_StackTrace	myStackTrace; 
		unsigned int	myNumCalls; 
	}; 

	MT_Mutex myGlobLock; 

	MC_HashMap<MC_StackTrace, Allocation>  myABuffer; 
	MC_HashMap<MC_StackTrace, Allocation>  myBBuffer; 
	MC_HashMap<MC_StackTrace, Allocation>* myFrontBuffer; 

	MC_EggClockTimer myFlushTimer;

	void PrivFlushToFile()
	{
		if(!MMS_Settings::TrackMemoryAllocations)
			return; 

		if(!myFlushTimer.HasExpired())
			return; 

		myGlobLock.Lock();

		MC_HashMap<MC_StackTrace, Allocation>* toFileBuffer = myFrontBuffer; 

		if(myFrontBuffer == &myABuffer)
			myFrontBuffer = &myBBuffer;
		else if(myFrontBuffer == &myBBuffer)
			myFrontBuffer = &myABuffer;

		myGlobLock.Unlock();

		// save to file 
		char fileName[128];
		sprintf(fileName, "callstacks_%06d.csd", GetTickCount() / 1000); 
		int fd = open(fileName, O_BINARY | O_TRUNC | O_WRONLY | O_CREAT, S_IREAD | S_IWRITE);
		if(fd == -1)
		{
			LOG_ERROR("failed to open %s for writing, bailing, last error: %d", GetLastError());
			return; 
		}

		unsigned int timeStamp = (unsigned int) time(NULL);
		if(write(fd, &timeStamp, sizeof(timeStamp)) != sizeof(timeStamp))
		{
			LOG_ERROR("failed to write time stamp to file, bailing, last error: %d", GetLastError());
			close(fd); 
			return; 
		}

		MC_StackWalker stackWalker;
		MC_HashMap<MC_StackTrace, Allocation>::Iterator iter(toFileBuffer);

		for(; iter; iter++)
		{
			Allocation& t = iter.Item();

			if(t.myNumCalls == 0)
				continue; 

			if(write(fd, &(t.myNumCalls), sizeof(t.myNumCalls)) != sizeof(t.myNumCalls))
			{
				LOG_ERROR("failed to write num recent calls to file, bailing out");
				close(fd);
				return; 
			}

			MC_StaticString<1024 * 10> callStackStr;
			for(size_t j = 0; j < t.myStackTrace.myStackSize; j++)
			{
				MC_StaticString<1024> line; 
				stackWalker.ResolveSymbolName(line, &(t.myStackTrace), (int) j);
				callStackStr += line; 
				callStackStr += "\n"; 
			}

			unsigned int callStackStrLength = callStackStr.GetLength() + 1;
			if(write(fd, &callStackStrLength, sizeof(callStackStrLength)) != sizeof(callStackStrLength))
			{
				LOG_ERROR("failed to write string length, bailing out");
				close(fd); 
				return; 
			}

			if(write(fd, callStackStr.GetBuffer(), callStackStrLength) != callStackStrLength)
			{
				LOG_ERROR("failed to write callstack, bailing");
				close(fd);
				return; 
			}

			t.myNumCalls = 0; 
		}

		close(fd); 
	}
}; 

void 
MMS_HeapImplementation_TrackAllocation(
	size_t		aSize)
{
	static __declspec(thread) bool logAllocation = true; 

	if(logAllocation && MMS_Settings::TrackMemoryAllocations)
	{
		logAllocation = false; 
		MMS_AllocationLogger::GetInstance().AddSample();
		logAllocation = true; 
	}
}

void* 
operator new(
	size_t		aSize, 
	const char* aFileName, 
	int			aLine)
{
	MMS_HeapImplementation_TrackAllocation(aSize); 
	static HANDLE heap = GetProcessHeap(); 
	void *t = HeapAlloc(heap, 0, aSize);
	assert(t && "out of memory"); 
	return t; 
}

void* 
operator new[](
	size_t		aSize, 
	const char* aFileName, 
	int			aLine)
{
	MMS_HeapImplementation_TrackAllocation(aSize); 
	static HANDLE heap = GetProcessHeap(); 
	void *t = HeapAlloc(heap, 0, aSize);
	assert(t && "out of memory"); 
	return t; 
}

void* 
operator new[](
	size_t		aSize)
{
	MMS_HeapImplementation_TrackAllocation(aSize); 
	static HANDLE heap = GetProcessHeap(); 
	void *t = HeapAlloc(heap, 0, aSize);
	assert(t && "out of memory"); 
	return t; 
}

void* 
operator new(
	size_t		aSize)
{
	MMS_HeapImplementation_TrackAllocation(aSize); 
	static HANDLE heap = GetProcessHeap(); 
	void *t = HeapAlloc(heap, 0, aSize);
	assert(t && "out of memory"); 
	return t; 
}

void 
operator delete(
	void*		aPointer)
{
	static HANDLE heap = GetProcessHeap(); 
	if(aPointer)
		HeapFree(heap, 0, aPointer);
}

void 
operator delete[](
	void *		aPointer)
{
	static HANDLE heap = GetProcessHeap(); 
	if(aPointer)
		HeapFree(heap, 0, aPointer);
}


#endif // MMS_HEAPIMPLEMENTATION_H