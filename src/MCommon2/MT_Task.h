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
#ifndef MT_TASK_H
#define MT_TASK_H

typedef void (* MT_TASKFUNC)(void* aTaskData);

// MT_Task defines a task, ie. what to do (function pointer) and what to do it on (data buffer).
__declspec(align(64))	// cache friendly alignment
class MT_Task
{
#ifdef MC_USE_SMALL_OBJECT_ALLOCATOR
	void* operator new(size_t aSize);
	void operator delete(void* aPointer);
#endif

private:
	friend class MT_Supervisor;
	friend class MT_WorkerThread;
	friend class MT_Job;

	MT_Supervisor*			mySupervisor;
	MT_TASKFUNC				myFunctionPointer;
	MT_Job*					myJob;
	int						myData[12];

	void Execute();
};

// Note - classes that derive and implement MT_TaskRunner should only have
// simple data members and no important code in the constructor/destructor.
class MT_TaskRunner
{
public:
	virtual void RunTask() = 0;
};

void MT_TaskRunnerCallback(void* aData);

#endif //MT_TASK_H

