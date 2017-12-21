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
#include "MT_PooledThread.h"
#include "MT_ThreadPool.h"
#include "MT_ThreadingTools.h"

MT_PooledThread::MT_PooledThread()
:mySemaphore(0, 1)
{
	myFunction = 0;
	myData = 0;
	myPool = 0;
}

MT_PooledThread::~MT_PooledThread()
{
}

void MT_PooledThread::Run()
{
	MT_ThreadingTools::SetCurrentThreadName(myName);

	while(!StopRequested())
	{
		if(myFunction)
		{
			myFunction(this, myData);

			// Reset priority in case myFunction changed it.
			SetThreadPriority((HANDLE)GetHandle(), THREAD_PRIORITY_NORMAL);

			myFunction = 0;
		}
		mySemaphore.Acquire();
	}
}

