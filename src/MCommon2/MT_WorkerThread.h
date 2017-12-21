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
#ifndef MT_WORKER_THREAD_H
#define MT_WORKER_THREAD_H

#include "mt_thread.h"
#include "mt_mutex.h"

// The MT_WorkerThread grabs and executes tasks whenevery it's supervisor have any available.
class MT_WorkerThread : public MT_Thread
{
private:
	friend class MT_Supervisor;

	MT_WorkerThread();
	~MT_WorkerThread();

	virtual void Run();

	MT_Supervisor*	mySupervisor;
	char			myName[256];
};

#endif //MT_WORKER_THREAD_H

