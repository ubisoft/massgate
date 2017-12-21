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
#include "MT_Mutex.h"
#include "MT_ThreadingTools.h"

MT_Mutex::MT_Mutex(int spinLockCount)
{
	if(spinLockCount > 0)
		InitializeCriticalSectionAndSpinCount(&myMutex, spinLockCount);
	else
		InitializeCriticalSection(&myMutex);

	myNumLocks = 0;
}

MT_Mutex::~MT_Mutex()
{
	DeleteCriticalSection(&myMutex);
}

void 
MT_Mutex::Lock()
{
	if(!TryEnterCriticalSection(&myMutex))
	{
		MC_THREADPROFILER_ENTER_WAIT();
		MT_ThreadingTools::SetCriticalSectionStatus(true);
		EnterCriticalSection(&myMutex);
		MT_ThreadingTools::SetCriticalSectionStatus(false);
		MC_THREADPROFILER_LEAVE_WAIT();
	}
	++myNumLocks;
}

bool
MT_Mutex::TryLock()
{
	if (TryEnterCriticalSection(&myMutex) != 0)
	{
		++myNumLocks;
		return true;
	}
	return false;
}

void 
MT_Mutex::Unlock()
{
	assert(myNumLocks > 0);
	--myNumLocks;
	LeaveCriticalSection(&myMutex);
}

unsigned int 
MT_Mutex::GetLockCount()
{
	return myNumLocks;
}

//////////////////////////////////////////////////////////////////////////

MT_SpinLockMutex::MT_SpinLockMutex()
: myLock(0)
, myNumLocks(0)
{
}

MT_SpinLockMutex::~MT_SpinLockMutex()
{
}

void 
MT_SpinLockMutex::Lock()
{
	myNumLocks++; 
	MT_ThreadingTools::SetCriticalSectionStatus(true);
	while(_InterlockedExchange(&myLock, 1)); 
	MT_ThreadingTools::SetCriticalSectionStatus(false);
}

bool 
MT_SpinLockMutex::TryLock()
{
	if(!_InterlockedExchange(&myLock, 1))
	{
		myNumLocks++; 
		return true; 
	}
	else 
	{
		return false; 
	}
}

void 
MT_SpinLockMutex::Unlock()
{
	_InterlockedExchange(&myLock, 0);
	myNumLocks--; 
}

unsigned int 
MT_SpinLockMutex::GetLockCount()
{
	return myNumLocks; 
}

//////////////////////////////////////////////////////////////////////////

MT_SkipLock::MT_SkipLock()
: myLock(0)
{
}

MT_SkipLock::~MT_SkipLock()
{
	assert(myLock == 0);
}

bool
MT_SkipLock::TryLock()
{
	return _InterlockedCompareExchange(&myLock, 1, 0) == 0;
}

void
MT_SkipLock::Unlock()
{
	_InterlockedExchange(&myLock, 0);
}
