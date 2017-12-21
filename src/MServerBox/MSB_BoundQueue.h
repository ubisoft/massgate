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
#ifndef MSB_QUEUE_H
#define MSB_QUEUE_H

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MT_Mutex.h"

/**
 * A bound queue, when the queue is full writes will block. Reads are optionally
 * blocking.
 */
template <typename T, int Size, typename Lock = MT_Mutex>
class MSB_BoundQueue
{
public:
					MSB_BoundQueue();
					~MSB_BoundQueue();

	uint32			GetCapacity() const { return Size; }
	uint32			GetUsed() const { return myItemCount; }

	bool			Push(
						const T&		anElement,
						int32			aTimeout = INFINITE);
	bool			Pop(
						T&				anElement);
	bool			WaitPop(
						T&				anElement,
						int32			aTimeout = INFINITE);

private:
	HANDLE			myQueueNotFull;
	HANDLE			myQueueNotEmpty;
	int32			myItemCount;
	int32			myFirstItem;
	Lock			myMutex;
	T				myData[Size];
};

template<typename T, int Size, typename Lock>
MSB_BoundQueue<T, Size, Lock>::MSB_BoundQueue()
	: myItemCount(0)
	, myFirstItem(0)
{
	myQueueNotFull = CreateEvent(NULL, false, true, NULL);
	assert(myQueueNotFull != INVALID_HANDLE_VALUE);

	myQueueNotEmpty = CreateEvent(NULL, false, false, NULL);
	assert(myQueueNotEmpty != INVALID_HANDLE_VALUE);
}

template<typename T, int Size, typename Lock>
MSB_BoundQueue<T, Size, Lock>::~MSB_BoundQueue()
{
	CloseHandle(myQueueNotFull);
	CloseHandle(myQueueNotEmpty);
}

/**
 * Push a new item onto the queue.
 *
 * Blocks if the queue is full. For a timeout period
 */
template<typename T, int Size, typename Lock>
bool
MSB_BoundQueue<T, Size, Lock>::Push(
	const T&		anElement,
	int32			aTimeout)
{
	myMutex.Lock();
	while(myItemCount + 1 > Size)
	{
		myMutex.Unlock();
		int32 result = WaitForSingleObject(myQueueNotFull, aTimeout);
		if (result == WAIT_TIMEOUT)
			return false;
		myMutex.Lock();
	}

	myData[(myFirstItem + myItemCount) % Size] = anElement;

	myItemCount ++;
	if(myItemCount == Size)
		ResetEvent(myQueueNotFull);
	if(myItemCount == 1)
		SetEvent(myQueueNotEmpty);

	myMutex.Unlock();
	return true;
}


/**
 * Pops the next item of the queue.
 *
 * @return True if an item was removed from the queue false otherwise.
 */
template<typename T, int Size, typename Lock>
bool
MSB_BoundQueue<T, Size, Lock>::Pop(
	T&				anElement)
{
	myMutex.Lock();

	if(myItemCount == 0)
	{
		myMutex.Unlock();
		return false;
	}

	anElement = myData[myFirstItem];
	
	myItemCount --;
	myFirstItem = (myFirstItem + 1) % Size;

	if(myItemCount == Size-1)
		SetEvent(myQueueNotFull);

	myMutex.Unlock();
	return true;
}

/**
 * Pops the next item from the queue.
 *
 * @return Blocks until an item has been popped.
 */
template<typename T, int Size, typename Lock>
bool
MSB_BoundQueue<T, Size, Lock>::WaitPop(
	T&				anElement,
	int32			aTimeout)
{
	myMutex.Lock();

	while(myItemCount == 0)
	{
		myMutex.Unlock();
		int32		result = WaitForSingleObject(myQueueNotEmpty, aTimeout);
		if(result == WAIT_TIMEOUT)
			return false;
		myMutex.Lock();
	}
	
	anElement = myData[myFirstItem];
	myItemCount --;
	myFirstItem = (myFirstItem + 1) % Size;
	

	if(myItemCount == 0)
		ResetEvent(myQueueNotEmpty);

	myMutex.Unlock();
	return true;
}

#endif // IS_PC_BUILD

#endif /* MSB_QUEUE_H */
