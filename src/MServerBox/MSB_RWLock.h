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
#ifndef MSB_RWLOCK_H
#define MSB_RWLOCK_H

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MT_Mutex.h"

class MSB_RWLock 
{
	public:
							MSB_RWLock();

		int32				ReadLock();
		int32				ReadUnlock();

		int32				WriteLock();
		int32				WriteUnlock();

	private:
		MT_Mutex			myMutexLock;
		HANDLE				myWaiters;
		int					myNumReaders;
		bool				myHaveWriter;

};

class MSB_RWAutoLock
{
public:
	typedef enum {
		RWLOCK_READ_MODE,
		RWLOCK_WRITE_MODE
	}LockMode;
							MSB_RWAutoLock(
								MSB_RWLock&		aLock,
								LockMode		aLockMode)
								: myLock(aLock)
								, myLockMode(aLockMode)
							{
								if(myLockMode == RWLOCK_READ_MODE)
									myLock.ReadLock();
								else
									myLock.WriteLock();
							}

							~MSB_RWAutoLock()
							{
								if(myLockMode == RWLOCK_READ_MODE)
									myLock.ReadUnlock();
								else
									myLock.WriteUnlock();
							}

	void					Unlock()
							{
								if(myLockMode == RWLOCK_READ_MODE)
									myLock.ReadUnlock();
								else
									myLock.WriteUnlock();
							}


private:
	MSB_RWLock&				myLock;
	LockMode				myLockMode;
};

#endif // IS_PC_BUILD

#endif /* MSB_RWLOCK_H */
