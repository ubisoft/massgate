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
#ifndef MT_SUPERVISOR_H
#define MT_SUPERVISOR_H

#include "MT_Task.h"
#include "MT_Mutex.h"
#include "MT_Semaphore.h"

// The MT_Supervisor class owns the worker threads, the task buffer and the mutex.
// It is responsible for keeping track of tasks and for deligating them to worker threads.
// MT_Supervisor should NOT be created and destroyed every frame. It is a heavy weight
// object and should be created/destroyed only once per program run (or session).
// In most cases, one MT_Supervisor per program run should be enough. Use MT_Jobs to
// handle task grouping and syncronization.
class MT_Supervisor
{
	friend class MT_WorkerThread;
	friend class MT_Task;
	friend class MT_Job;

public:
	MT_Supervisor(const char* aThreadNameBase=0, int aNumWorkerThreads = 0, int aRelativePrio = -1);
	~MT_Supervisor();

	// Set aJob = 0 if the task does not belong to a job.
	void AddTask(MT_TASKFUNC aFunc, const void* aTaskData, int aDataSize, MT_Job* aJob);

	// Set aJob = 0 if the task does not belong to a job.
	template <class T>
	void AddTask(const T& aTaskRunner, MT_Job* aJob)
	{
		AddTask(MT_TaskRunnerCallback, &aTaskRunner, sizeof(aTaskRunner), aJob);
	}

	static void				CreateDefaultInstance();
	static void				DestroyDefaultInstance();
	static MT_Supervisor*	GetDefaultInstance();

private:
	static const unsigned int MAX_NUM_THREADS = 16;
	MT_WorkerThread* myThreads[MAX_NUM_THREADS];
	HANDLE myCompletionPort;
	int myRelativePriority;

	static MT_Supervisor* ourDefaultInstance;
};

#endif //MT_SUPERVISOR_H

