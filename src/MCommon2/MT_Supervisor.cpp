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
#include "MT_Supervisor.h"
#include "MT_WorkerThread.h"
#include "MT_Job.h"
#include "MT_ThreadingTools.h"

MT_Supervisor* MT_Supervisor::ourDefaultInstance = 0;

// Mark all tasks as free and create threads and 
MT_Supervisor::MT_Supervisor(const char* aThreadNameBase, int aNumWorkerThreads, int aRelativePrio)
{
	memset(myThreads, 0, sizeof(myThreads));

	myRelativePriority = aRelativePrio;

	// Create a completion port that is not bound to the IO system.
	// Tell the kernel that it can process up to n CPU completion packets concurrently
	const unsigned int maxConcurrentThreads = 0; // Let kernel decide (i.e. as many as there are cpu's)
	myCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, maxConcurrentThreads);

	if (!myCompletionPort)
	{
		MC_DEBUG("Error: %u", GetLastError());
	}
	assert(myCompletionPort);

#if IS_PC_BUILD		// SWFM:SWR - PC specific section
#ifndef _DEBUG
	_WriteBarrier(); // Make sure myCompletionPort is written to memory before completion threads access it.
#endif
#endif

	// AMD recommends running only one heavy thread per core
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);

	if(aNumWorkerThreads == 0)
	{
		if(sysInfo.dwNumberOfProcessors >= 8)
			aNumWorkerThreads = sysInfo.dwNumberOfProcessors - 2;
		else
			aNumWorkerThreads = __max(sysInfo.dwNumberOfProcessors, 1);
	}

	for(int i=0; i<aNumWorkerThreads; i++)
	{
		MT_WorkerThread* wt = new MT_WorkerThread;
		wt->mySupervisor = this;
		sprintf(wt->myName, "%s %u", aThreadNameBase ? aThreadNameBase : "Task thread", i);
		// SWFM: RTS
		// Spread worker threads among HW threads 2 - 5
		wt->Start();
		myThreads[i] = wt;
	}
}

MT_Supervisor::~MT_Supervisor()
{
	// Destroy the threads
	int i = 0;
	while(myThreads[i++])
		PostQueuedCompletionStatus(myCompletionPort, 0 /* kill yourself */, NULL, NULL);

	MC_Sleep(1);
	i = 0;
	while (myThreads[i])
		myThreads[i++]->StopAndDelete();
}

// Adds a task to the supervisor. 
void MT_Supervisor::AddTask(MT_TASKFUNC aFunc, const void* aTaskData, int aDataSize, MT_Job* aJob)
{
	if (aJob)
		_InterlockedIncrement(&aJob->myNumPendingTasks);

	// Create a "completion packet", i.e. a MT_Task object with a task to run
	MT_Task* task = new MT_Task;
	assert(aDataSize <= sizeof(task->myData));
	memcpy(task->myData, aTaskData, aDataSize);
	task->myFunctionPointer = aFunc;
	task->myJob = aJob;
	task->mySupervisor = this;

	// Let one of the workerthreads handle the task
	PostQueuedCompletionStatus(myCompletionPort, 1 /* do process */, (ULONG_PTR)task, NULL);
}


void MT_Supervisor::CreateDefaultInstance()
{
	assert(ourDefaultInstance == 0);
	ourDefaultInstance = new MT_Supervisor();
}

void MT_Supervisor::DestroyDefaultInstance()
{
	delete ourDefaultInstance;
	ourDefaultInstance = 0;
}

MT_Supervisor* MT_Supervisor::GetDefaultInstance()
{
	return ourDefaultInstance;
}

