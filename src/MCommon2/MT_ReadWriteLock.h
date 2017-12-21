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
#ifndef _MT_READWRITE_LOCK__H_
#define _MT_READWRITE_LOCK__H_

// Multiple (parallel!) readers, multiple (exclusive) writers type of mutex. Wicked low overhead.
// Uses spinlocks so there is no context-switching. Only use this type of lock when your Writes are few and very quick.

#include "MT_ThreadingTools.h"

class MT_ReadWriteLock
{
public:
	class ReadLocker
	{
	public:
		ReadLocker(MT_ReadWriteLock& anRwLock) : myLock(anRwLock) { myLock.BeginRead(); }
		~ReadLocker() { myLock.EndRead(); }
	private:
		MT_ReadWriteLock& myLock;
	};
	class WriteLocker
	{
	public:
		WriteLocker(MT_ReadWriteLock& anRwLock) : myLock(anRwLock) { myLock.BeginWrite(); }
		~WriteLocker() { myLock.EndWrite(); }
	private:
		MT_ReadWriteLock& myLock;
	};

	MT_ReadWriteLock() { _InterlockedExchange(&myLock, 0); }
	__forceinline void BeginRead()
	{
		// Add a reader
		while (_InterlockedIncrement(&myLock) & 0xffff0000)
		{
			// There is a writer active
			EndRead();
			MC_Sleep(0);
		}
	}

	__forceinline void EndRead()
	{
		_InterlockedDecrement(&myLock);
	}

	__forceinline void BeginWrite()
	{
		_InterlockedOr(&myLock, 0x80000000); // Stop any incoming readers
		while ((_InterlockedExchangeAdd(&myLock, 0x00010000) & 0x7fffffff) != 0)
		{
			// There is a reader or writer active
			PrivEndTryWrite();
			MC_Sleep(0);
		}
	}

	__forceinline void EndWrite()
	{
		PrivEndTryWrite();
		_InterlockedAnd(&myLock, 0x7fffffff); // Unlock so incoming readers can get lock
	}

private:
	__forceinline void PrivEndTryWrite()
	{
		_InterlockedExchangeAdd(&myLock, -0x00010000); // Remove our writercounter
	}

	__declspec(align(64)) volatile long myLock; // upper half: there is a writer active, lower half: there is a reader active
};

#endif
