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
#ifndef MDB_QUERYLOGGER_H
#define MDB_QUERYLOGGER_H

#include "MC_EggClockTimer.h"
#include "MC_HashMap.h"
#include "MC_StackWalker.h"

#include "ML_Logger.h"

#include "MT_Thread.h"
#include "MT_ThreadingTools.h"

#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <time.h>

class MDB_QueryLogger : MT_Thread
{
public: 
	static MDB_QueryLogger& GetInstance()
	{
		static MDB_QueryLogger* instance = new MDB_QueryLogger();
		return *instance; 
	}

	void Run()
	{
		for(;;)
		{
			if(myIsEnabled)
				PrivFlushToFile(); 
			MC_Sleep(1000); 
		}
	}

	void AddSample(
		uint32		aTimeUsed, 
		const char*	aQuery)
	{
		if(!myIsEnabled)
			return; 

		MC_StackWalker stackWalker;
		MC_StackTrace stackTrace; 
		stackWalker.SaveCallstack(&stackTrace);

		MT_MutexLock lock(myGlobLock);

		if(myFrontBuffer->Exists(stackTrace))
		{
			QueryItem& t = myFrontBuffer->operator[](stackTrace);
			t.AddSample(aTimeUsed); 
		}
		else
		{
			QueryItem t(stackTrace, aQuery);
			t.AddSample(aTimeUsed); 
			myFrontBuffer->operator [](stackTrace) = t; 
		}
	}

	void SetEnabled(
		bool aIsEnabled)
	{
		myIsEnabled = aIsEnabled; 	
	}

private: 
	MDB_QueryLogger()
		: myABuffer(1024)
		, myBBuffer(1024)
		, myFrontBuffer(&myABuffer)
		, myFlushTimer(30 * 60 * 1000)
		, myIsEnabled(false)
	{
		Start();
	}

	class QueryItem
	{
	public: 
		QueryItem()
			: myNumCalls(0)
			, myCounters(NULL)
		{
		}

		QueryItem(
			MC_StackTrace&	aStackTrace, 
			const char*		aQuery)
			: myStackTrace(aStackTrace)
			, myNumCalls(0)
		{
			myCounters = new MC_HashMap<unsigned int, unsigned int>(); 
			myQuery = aQuery; 
		}

		void AddSample(
			unsigned int	aTimeUsed)
		{
			unsigned int index = aTimeUsed / 100; 
			if(!myCounters->Exists(index))
				myCounters->operator[](index) = 0; 
			myCounters->operator[](index)++; 

			myNumCalls++; 
		}

		MC_StackTrace	myStackTrace; 
		
		MC_HashMap<unsigned int, unsigned int>* myCounters; 
		unsigned int myNumCalls; 
		MC_String myQuery; 
	}; 

	MT_Mutex myGlobLock; 

	MC_HashMap<MC_StackTrace, QueryItem>  myABuffer; 
	MC_HashMap<MC_StackTrace, QueryItem>  myBBuffer; 
	MC_HashMap<MC_StackTrace, QueryItem>* myFrontBuffer; 

	MC_EggClockTimer myFlushTimer;
	volatile bool myIsEnabled; 

	void PrivFlushToFile()
	{
		if(!myFlushTimer.HasExpired())
			return; 

		myGlobLock.Lock();

		MC_HashMap<MC_StackTrace, QueryItem>* toFileBuffer = myFrontBuffer; 

		if(myFrontBuffer == &myABuffer)
			myFrontBuffer = &myBBuffer;
		else if(myFrontBuffer == &myBBuffer)
			myFrontBuffer = &myABuffer;

		myGlobLock.Unlock();

		// save to file 
		char fileName[128];
		sprintf(fileName, "querytimers_%06d.qts", time(NULL)); 
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
		MC_HashMap<MC_StackTrace, QueryItem>::Iterator iter(toFileBuffer);
		unsigned int numStacktraces = 0; 
		for(; iter; iter++)
		{
			QueryItem& t = iter.Item();

			if(t.myNumCalls == 0)
				continue; 
			numStacktraces++; 
		}		

		if(write(fd, &numStacktraces, sizeof(numStacktraces)) != sizeof(numStacktraces))
		{
			LOG_ERROR("failed to write num stack traces to file, bailing, last error: %d", GetLastError());
			close(fd); 
			return; 
		}

		MC_HashMap<MC_StackTrace, QueryItem>::Iterator iter2(toFileBuffer);
		for(; iter2; iter2++)
		{
			QueryItem& t = iter2.Item();

			if(t.myNumCalls == 0)
				continue; 

			if(write(fd, &(t.myNumCalls), sizeof(t.myNumCalls)) != sizeof(t.myNumCalls))
			{
				LOG_ERROR("failed to write num recent calls to file, bailing out");
				close(fd);
				return; 
			}

			unsigned int queryLength = t.myQuery.GetLength() + 1; 
			if(write(fd, &queryLength, sizeof(queryLength)) != sizeof(queryLength))
			{
				LOG_ERROR("failed to write query length, bailing out");
				close(fd); 
				return; 			
			}

			if(write(fd, t.myQuery.GetBuffer(), queryLength) != queryLength)
			{
				LOG_ERROR("failed to write query, bailing out");
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

			MC_HashMap<unsigned int, unsigned int>::Iterator counterIter(t.myCounters); 

			unsigned int numEntries = 0;
			for(; counterIter; counterIter++)
			{
				unsigned int& count = counterIter.Item(); 
				if(count == 0)
					continue; 

				numEntries++; 
			}

			if(write(fd, &numEntries, sizeof(unsigned int)) != sizeof(unsigned int))
			{
				LOG_ERROR("failed to write num entries, bailing");
				close(fd);
				return; 
			}
			
			MC_HashMap<unsigned int, unsigned int>::Iterator counterIter2(t.myCounters); 
			for(; counterIter2; counterIter2++)
			{
				const unsigned int  index = counterIter2.Key(); 
				unsigned int& count = counterIter2.Item(); 

				if(count == 0)
					continue; 

				if(write(fd, &index, sizeof(unsigned int)) != sizeof(unsigned int))
				{
					LOG_ERROR("faild to write index to file, bailing out");
					close(fd);
					return; 
				}
				if(write(fd, &count, sizeof(unsigned int)) != sizeof(unsigned int))
				{
					LOG_ERROR("faild to write count to file, bailing out");
					close(fd);
					return; 
				}
	
				count = 0; 			
			}

			t.myNumCalls = 0; 
		}

		close(fd); 
	}
}; 

void 
MDB_QueryLogger_Log(
	unsigned int	aTimeUsed, 
	const char*		aQuery); 


#endif // MDB_QUERYLOGGER_H