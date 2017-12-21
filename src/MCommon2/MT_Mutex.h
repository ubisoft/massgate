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
#ifndef MT_MUTEX__H___
#define MT_MUTEX__H___

#include "MC_Platform.h"

class MT_Mutex
{
public:
	MT_Mutex(int spinLockCount = 0);
	virtual ~MT_Mutex();

	void Lock();	// Lock the mutex. Will halt current thread until another thread has unlocked
	bool TryLock(); // If mutex is already locked by any other thread than me; fail instead of wait
	void Unlock();

	// For serverside use; where we can safely call TryEnterCriticalSection (not win9x portable otherwise)
	__forceinline CRITICAL_SECTION& GetCriticalSection() { return myMutex; }

	unsigned int GetLockCount();		//returns the number of times this mutex has been recursivly locked
private:
	CRITICAL_SECTION myMutex;
	unsigned int myNumLocks;
};

class MT_SpinLockMutex
{
public: 
	MT_SpinLockMutex(); 
	virtual ~MT_SpinLockMutex(); 

	void Lock(); 
	bool TryLock(); 
	void Unlock(); 

	unsigned int GetLockCount(); 
private: 
	volatile long myLock; 
	unsigned int myNumLocks; 
};

class MT_SkipLock
{
public:
	MT_SkipLock();
	~MT_SkipLock();

	bool TryLock();
	void Unlock();
private:
	volatile long myLock;
};


template<typename MUTEX>
class MT_AutoUnlocker
{
public:
	MT_AutoUnlocker() 
	: myMutex(NULL) 
	{ 
	}
	
	MT_AutoUnlocker(MUTEX& aMutex) 
	: myMutex(&aMutex) 
	{ 
		myMutex->Lock(); 
	}

	~MT_AutoUnlocker() 
	{ 
		Unlock(); 
	}

	__forceinline void Lock(MUTEX& aMutex)
	{
		assert(myMutex == NULL);
		myMutex = &aMutex;
		myMutex->Lock();
	}

	__forceinline void Unlock()
	{
		if(myMutex)
		{
			myMutex->Unlock();
			myMutex = NULL;
		}
	}

	__forceinline bool IsLocked() const
	{
		return myMutex != NULL;
	}

private:
	MUTEX* myMutex;
};

typedef MT_AutoUnlocker<MT_Mutex> MT_MutexLock;
typedef MT_AutoUnlocker<MT_SpinLockMutex> MT_SpinLock; 




#endif

