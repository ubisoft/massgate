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
#include "MT_ThreadPool.h"
#include "MT_Mutex.h"

static MT_Mutex locThreadPoolMutex;
MT_ThreadPool* MT_ThreadPool::ourDefaultInstance = 0;

MT_ThreadPool::MT_ThreadPool()
{
	MT_MutexLock lock(locThreadPoolMutex);

	myThreads.Init(16, 16);
}

MT_ThreadPool::~MT_ThreadPool()
{
	MT_MutexLock lock(locThreadPoolMutex);

	for(int i=0; i<myThreads.Count(); i++)
	{
		while(myThreads[i]->Resume() > 0)
		{
			// Loop intentionally left empty
		}
	}

	for(int i=0; i<myThreads.Count(); i++)
		myThreads[i]->Stop();

	for(int i=0; i<myThreads.Count(); i++)
		myThreads[i]->mySemaphore.Release();

	for(int i=0; i<myThreads.Count(); i++)
		myThreads[i]->StopAndDelete();
}

void MT_ThreadPool::AcquireAndRunThread(MT_POOLED_THREAD_FUNCTION aFunction, void* aData)
{
	MT_MutexLock lock(locThreadPoolMutex);

	for(int i=0; i<myThreads.Count(); i++)
	{
		if(myThreads[i]->myFunction == 0)
		{
			myThreads[i]->myPool = this;
			myThreads[i]->myData = aData;
			myThreads[i]->myFunction = aFunction;
			myThreads[i]->mySemaphore.Release();
			return;
		}
	}

	MT_PooledThread* pooled = new MT_PooledThread;

	pooled->myPool = this;
	pooled->myData = aData;
	pooled->myFunction = aFunction;
	sprintf(pooled->myName, "MT_PooledThread%d", myThreads.Count());
	pooled->Start();

	myThreads.Add(pooled);
}

void MT_ThreadPool::CreateDefaultInstance()
{
	assert(ourDefaultInstance == 0);
	ourDefaultInstance = new MT_ThreadPool;
}

void MT_ThreadPool::DestroyDefaultInstance()
{
	delete ourDefaultInstance;
	ourDefaultInstance = 0;
}

MT_ThreadPool* MT_ThreadPool::GetDefaultInstance()
{
	return ourDefaultInstance;
}

