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
#include "MT_Semaphore.h"
#include "MT_ThreadingTools.h"

MT_Semaphore::MT_Semaphore(long anInitialCount, long aMaximumCount)
{
	mySemaphore = CreateSemaphore(NULL, anInitialCount, aMaximumCount, NULL);
	assert(mySemaphore != NULL);
}

MT_Semaphore::~MT_Semaphore()
{
	CloseHandle(mySemaphore);
}

bool 
MT_Semaphore::Acquire()
{
	MC_THREADPROFILER_ENTER_WAIT();
	MT_ThreadingTools::SetSemaphoreStatus( true);
	if (WaitForSingleObject(mySemaphore, INFINITE) != WAIT_FAILED)
	{
		MT_ThreadingTools::SetSemaphoreStatus( false);
		return true;
	}
	MT_ThreadingTools::SetSemaphoreStatus( false);
	MC_THREADPROFILER_LEAVE_WAIT();
	return false;
}

void 
MT_Semaphore::Release()
{
	ReleaseSemaphore(mySemaphore, 1, NULL);
}
