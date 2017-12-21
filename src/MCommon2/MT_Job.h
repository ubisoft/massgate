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
#ifndef MT_JOB_H
#define MT_JOB_H

#include "MT_Task.h"

// The MT_Job class helps syncronizing tasks by grouping dependent tasks into a job.
// When tasks are added to a MT_Job, the job in turn adds them to it's supervisor.
// The Finish() function blocks until all tasks that were added to the job are done.
// MT_Job is a light weight class and can be created on the stack on frame by frame
// basis if desirable.
class MT_Job
{
	friend class MT_Task;
	friend class MT_Supervisor;

public:
	MT_Job(MT_Supervisor* pSupervisor);
	~MT_Job();

	void AddTask(MT_TASKFUNC aFunction, const void* aTaskData, int aDataSize);

	template <class T>
	void AddTask(const T& aTaskRunner)
	{
		AddTask(MT_TaskRunnerCallback, &aTaskRunner, sizeof(aTaskRunner));
	}

	// Finish is a blocking call - it returns when all tasks have been carried out.
	void Finish();

private:
	MT_Supervisor* mySupervisor;

	__declspec(align(64)) struct  
	{
		volatile long	myNumPendingTasks;
	};

};

#endif //MT_JOB_H

