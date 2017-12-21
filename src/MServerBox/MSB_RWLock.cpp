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
#include "MSB_RWLock.h"

#include "MC_Base.h"
#if IS_PC_BUILD

MSB_RWLock::MSB_RWLock()
	: myWaiters(INVALID_HANDLE_VALUE)
	, myNumReaders(0)
	, myHaveWriter(false)
{
	myWaiters = CreateEvent(0, true, true, 0);
}

int32
MSB_RWLock::ReadLock()
{
	for(;;)
	{
		if(WaitForSingleObject(myWaiters, INFINITE) != WAIT_OBJECT_0)
			return GetLastError();

		MT_MutexLock		myLock(myMutexLock);

		if(!myHaveWriter)
		{
			myNumReaders ++;
			return 0;
		}

		if(!ResetEvent(myWaiters))
			return GetLastError();
	}

	return -1;
}

int32
MSB_RWLock::ReadUnlock()
{
	return WriteUnlock();
}

int32
MSB_RWLock::WriteLock()
{
	for(;;)
	{
		if(WaitForSingleObject(myWaiters, INFINITE) != WAIT_OBJECT_0)
			return GetLastError();

		MT_MutexLock			lock(myMutexLock);

		if(myNumReaders == 0 && !myHaveWriter)
		{
			myHaveWriter = true;
			return 0;
		}

		if(!ResetEvent(myWaiters))
			return GetLastError();
	}

	return -1;
}

int32
MSB_RWLock::WriteUnlock()
{
	MT_MutexLock		lock(myMutexLock);
	
	// Must be locked
	assert(myNumReaders > 0 || myHaveWriter);
	myHaveWriter = false;
	if(myNumReaders > 0)
		myNumReaders --;

	if(myNumReaders == 0)
	{
		if(!SetEvent(myWaiters))
			return GetLastError();
	}

	return 0;
}

#endif // IS_PC_BUILD
