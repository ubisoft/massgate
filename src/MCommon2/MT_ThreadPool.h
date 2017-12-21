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
#ifndef MT_THREAD_POOL_H
#define MT_THREAD_POOL_H

#include "MC_GrowingArray.h"
#include "MT_PooledThread.h"

#include "MT_Thread.h"

typedef void (* MT_POOLED_THREAD_FUNCTION)(MT_Thread* aThread, void* aData);

// The thread pool creates new threads if necessary but never deletes old ones.
// If an old, previously used, thread is available it will be used in stead of
// creating a new thread.
class MT_ThreadPool
{
	friend class MT_PooledThread;
public:
	MT_ThreadPool();

	// All the threads that belong to this pool must have exited before the thread
	// pool object is destroyed.
	~MT_ThreadPool();

	// Gets a thread from the pool and starts it, calling aFunction with aData.
	// The thread is automatically given back to the pool when aFunction exits.
	void		AcquireAndRunThread(MT_POOLED_THREAD_FUNCTION aFunction, void* aData);

	int			GetActualThreadCount() const { return myThreads.Count(); }

	static void				CreateDefaultInstance();
	static void				DestroyDefaultInstance();
	static MT_ThreadPool*	GetDefaultInstance();

private:
	MC_GrowingArray<MT_PooledThread*> myThreads;

	static MT_ThreadPool* ourDefaultInstance;
};

#endif MT_THREAD_POOL_H

