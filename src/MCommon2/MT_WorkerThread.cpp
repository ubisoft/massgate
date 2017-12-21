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
#include "MT_WorkerThread.h"
#include "MT_Task.h"
#include "MT_Supervisor.h"
#include "MT_ThreadingTools.h"


MT_WorkerThread::MT_WorkerThread()
{
	mySupervisor = 0;
}

MT_WorkerThread::~MT_WorkerThread()
{
}

// The worker threads just try to grab tasks from it's supervisor, and executes them.
void MT_WorkerThread::Run()
{
	assert(mySupervisor);
	MT_ThreadingTools::SetCurrentThreadName(myName);
	SetThreadPriorityBoost(GetCurrentThread(), TRUE);	// Disable boost!

// TODO: Uncomment these lines when MT_Job::Finish is not spin-sleeping anymore.
//	if(mySupervisor->myRelativePriority < 0)
//		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	MC_THREADPROFILER_ENTER_WAIT();

	while(!StopRequested())
	{
		MT_Task* task;
		ULONG_PTR taskptr;
		LPOVERLAPPED overlapped; // ignored
		DWORD shouldProcess = 0; 

		if (GetQueuedCompletionStatus(mySupervisor->myCompletionPort, &shouldProcess, &taskptr, &overlapped, INFINITE))
		{
			if (shouldProcess)
			{
				MC_THREADPROFILER_LEAVE_WAIT();

				task = (MT_Task*)taskptr;
				assert(task);
				task->Execute();
				delete task;

				MC_THREADPROFILER_ENTER_WAIT();
			}
			else
			{
				Stop();
				//MC_DEBUG("Workerthread %s stopping.", myName);
				break;
			}
		}
		else
		{
			MC_DEBUG("HYPERFATAL ULTRA (%u)! CompletionPort deleted while workers are active!!!", GetLastError());
			Stop();
		}
	}
}

