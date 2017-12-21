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
#ifndef MMS_THREADSAFEQUEUE__H_
#define MMS_THREADSAFEQUEUE__H_

#include "MC_GrowingArray.h"
#include "MT_Semaphore.h"
#include "MT_Mutex.h"
#include <windows.h>

// NOTE! The name queue is a misnomer. The data may or may not be ordered, and those who wait in Dequeue() may or may
// not be awaken in the order the called it.


template<typename T>
class MMS_ThreadSafeQueue
{
public:
	MMS_ThreadSafeQueue();
	virtual ~MMS_ThreadSafeQueue();

	void Dequeue(T* aDest);
	void Enqueue(const T& anItem);

	void Remove(const T& anItem);

	void Count(unsigned int& theCount);



	// ProcessArray provides a means for the caller to operate on the data in the array,
	// while thread-safety is provided by MMS_ThreadSafeQueue. The processor's Process()
	// method is called for each item in the array
	template <typename T>class IArrayProcessor {
	public: 
		// return false if iteration should stop.
		virtual bool Process(T& theItem) = 0;
	};
	// PurgeArray - iterate the PurgeProcessor over the data in the array. If it returns true,
	// the item is removed from the array.
	template <typename T>class IArrayPurger {
	public:
		// return true if the item should be removed
		virtual bool Purge(T& theItem) = 0;
	};
	void ProcessArray(IArrayProcessor<T>& theProcessor);
	void PurgeArray(IArrayPurger<T>& thePurger);

private:
	MC_GrowingArray<T> myArray;
//	MT_Mutex myReaderMutex;
//	MT_Mutex myWriterMutex;
	MT_Mutex myAccessMutex;
	HANDLE myAlerter;
	MT_Semaphore myFullSemaphore;
	MT_Semaphore myEmptySemaphore;
};


/* Implementation follows */


template<typename T>
MMS_ThreadSafeQueue<T>::MMS_ThreadSafeQueue()
: myFullSemaphore(0, 256), myEmptySemaphore(256,256)
{
	myAlerter = CreateEvent(NULL, TRUE, FALSE, NULL);
	myArray.Init(256,256,true);
	// myReaderMutex.Lock();
}

template<typename T>
MMS_ThreadSafeQueue<T>::~MMS_ThreadSafeQueue()
{
}

template<typename T>
void  /* Remove void keyword and get C1001 in studio 2003. */
MMS_ThreadSafeQueue<T>::Dequeue(T* aDest)
{
	// myReaderMutex.Lock();
	myFullSemaphore.Acquire();
//	WaitForSingleObject(myAlerter, INFINITE);
	myAccessMutex.Lock();

//	if (myArray.Count())
//	{
		*aDest = myArray[0];
		myArray.RemoveCyclicAtIndex(0);
		myAccessMutex.Unlock();
		myEmptySemaphore.Release();
//		if (myArray.Count())
//		{
			// myReaderMutex.Unlock();
//		}
//		else
//		{
//			ResetEvent(myAlerter);
//		}
//	}
//	else
//	{
//		*aDest = NULL;
//	}
}

template<typename T>
void 
MMS_ThreadSafeQueue<T>::Enqueue(const T& anItem)
{
	myEmptySemaphore.Acquire();
	myAccessMutex.Lock();
	myArray.Add(anItem);
	myAccessMutex.Unlock();
	myFullSemaphore.Release();
	/*	myWriterMutex.Lock();
	myAccessMutex.Lock();
	myArray.Add(anItem);
	if (myArray.Count() == 1)
	{
		// myReaderMutex.Unlock();
		SetEvent(myAlerter);
	}
	myWriterMutex.Unlock();
	myAccessMutex.Unlock();*/
}

template<typename T>
void
MMS_ThreadSafeQueue<T>::Remove(const T& anItem)
{
	// myReaderMutex.Lock();
	myAccessMutex.Lock();
	int previousCount = myArray.Count();

	myArray.RemoveCyclic(anItem);
	if (!(previousCount && (myArray.Count() == 0)))
	{
		ResetEvent(myAlerter);
		// myReaderMutex.Unlock();
	}
	myAccessMutex.Unlock();
}

template<typename T>
void
MMS_ThreadSafeQueue<T>::Count(unsigned int& theCount)
{
	myAccessMutex.Lock();
	theCount = unsigned int(myArray.Count());
	myAccessMutex.Unlock();
}

template<typename T>
void
MMS_ThreadSafeQueue<T>::ProcessArray(IArrayProcessor<T>& theProcessor)
{
	myAccessMutex.Lock();
	for (int i=0; (i < myArray.Count()) && theProcessor.Process(myArray[i]); i++)
		;
	myAccessMutex.Unlock();
}

template<typename T>
void 
MMS_ThreadSafeQueue<T>::PurgeArray(IArrayPurger<T>& thePurger)
{
	// myReaderMutex.Lock();
	myAccessMutex.Lock();
	int previousCount = myArray.Count();
	int i=0;
	for (int i=0; i < myArray.Count(); i++)
	{
		if (thePurger.Purge(myArray[i]))
			myArray.RemoveCyclicAtIndex(i--);
	}
	if (!(previousCount && (myArray.Count() == 0)))
	{
		ResetEvent(myAlerter);
		// myReaderMutex.Unlock();
	}
	myAccessMutex.Unlock();
}


#endif
