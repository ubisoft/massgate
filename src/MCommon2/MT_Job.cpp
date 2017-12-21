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
#include "MT_Job.h"
#include "MT_Task.h"
#include "MT_Supervisor.h"
#include "MT_ThreadingTools.h"
#include "MC_String.h"


MT_Job::MT_Job(MT_Supervisor* pSupervisor)
{
	_InterlockedExchange(&myNumPendingTasks, 0);
	mySupervisor = pSupervisor;
	assert(mySupervisor);
}

// The MT_Job destructor blocks until all it's tasks are done.
MT_Job::~MT_Job()
{
	Finish();
}

// Just pass the task on to the supervisor.
void MT_Job::AddTask(MT_TASKFUNC aFunction, const void* aTaskData, int aDataSize)
{
	mySupervisor->AddTask(aFunction, aTaskData, aDataSize, this);
}


void MT_Job::Finish()
{
	int spinCount = 0;

	while(_InterlockedCompareExchange(&myNumPendingTasks, 0, 0) != 0)
		::Sleep(0);
}

