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
#ifndef MT_POOLED_THREAD_H
#define MT_POOLED_THREAD_H

#include "MT_Thread.h"
#include "MT_Semaphore.h"

typedef void (* MT_POOLED_THREAD_FUNCTION)(MT_Thread* aThread, void* aData);

// Pooled thread class. Used by MT_ThreadPool.
class MT_PooledThread : public MT_Thread
{
private:
	friend class MT_ThreadPool;
	MT_PooledThread();
	
	virtual void Run();
	virtual ~MT_PooledThread();

	MT_POOLED_THREAD_FUNCTION volatile	myFunction;
	void*								myData;
	MT_ThreadPool*						myPool;
	char								myName[32];
	MT_Semaphore						mySemaphore;
};

#endif MT_POOLED_THREAD_H

