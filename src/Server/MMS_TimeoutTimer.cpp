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
#include "MMS_TimeoutTimer.h"


MMS_TimeoutTimer::MMS_TimeoutTimer(unsigned int aTimeoutInSeconds, MMS_ITimeoutCallback* theCallbackClass)
: myCallback(theCallbackClass)
{
	myTimeoutTime = time(NULL) + aTimeoutInSeconds;
	myIsActive = true;
}

MMS_TimeoutTimer::MMS_TimeoutTimer(const MMS_TimeoutTimer& aRhs)
:myCallback(aRhs.myCallback)
,myTimeoutTime(aRhs.myTimeoutTime)
,myIsActive(aRhs.myIsActive)
{
}

MMS_TimeoutTimer& 
MMS_TimeoutTimer::operator=(const MMS_TimeoutTimer& aRhs)
{
	myCallback = aRhs.myCallback;
	myTimeoutTime = aRhs.myTimeoutTime;
	myIsActive = aRhs.myIsActive;
	return *this;
}

bool
MMS_TimeoutTimer::operator==(const MMS_TimeoutTimer& aRhs)
{
	return ((myTimeoutTime == aRhs.myTimeoutTime) && (myCallback == aRhs.myCallback) && (myIsActive == aRhs.myIsActive));
}

bool 
MMS_TimeoutTimer::operator<(const MMS_TimeoutTimer& aRhs)
{
	return myIsActive && (myTimeoutTime < aRhs.myTimeoutTime);
}


MMS_TimeoutTimer::MMS_TimeoutTimer()
{
	myCallback = NULL;
	myTimeoutTime = 0;
	myIsActive = 0;
}

MMS_TimeoutTimer::~MMS_TimeoutTimer()
{
}

MMS_TimeoutTimer*
MMS_TimeoutTimer::Allocate()
{
	static MMS_TimeoutTimer myTimers[512];
	static unsigned long myCurrentTimer = 0;

	return &myTimers[myCurrentTimer++&0x1ff];
}

MMS_TimeoutTimer*
MMS_TimeoutTimer::Allocate(unsigned int aTimeoutInSeconds, MMS_ITimeoutCallback* theCallbackClass)
{
	MMS_TimeoutTimer* t = Allocate();
	t->Set(aTimeoutInSeconds, theCallbackClass);
	return t;
}

void
MMS_TimeoutTimer::Set(unsigned int aTimeoutInSeconds, MMS_ITimeoutCallback* theCallbackClass)
{
	myTimeoutTime = time(NULL) + aTimeoutInSeconds;
	myCallback = theCallbackClass;
	myIsActive = true;
}

void
MMS_TimeoutTimer::Init()
{
	MMS_TimeoutTimer::myAccessMutex.Lock();
	if (MMS_TimeoutTimer::myTimerTicker == NULL)
	{
		MMS_TimeoutTimer::myTimerTicker = new TimerTicker();
		myTimerTicker->Start();
	}
	MMS_TimeoutTimer::myAccessMutex.Unlock();
}

void 
MMS_TimeoutTimer::AddTimer(MMS_TimeoutTimer* theTimer)
{
/*
	MMS_TimeoutTimer::myAccessMutex.Lock();
	assert(myTimerTicker != NULL);
	if (MMS_TimeoutTimer::myClosestTimeout == -1) MMS_TimeoutTimer::myClosestTimeout = theTimer->myTimeoutTime;
	if (theTimer->myTimeoutTime < MMS_TimeoutTimer::myClosestTimeout)
		MMS_TimeoutTimer::myClosestTimeout = theTimer->myTimeoutTime;
	myTimers.Enqueue(theTimer);

	MMS_TimeoutTimer::myAccessMutex.Unlock();
*/
}

void 
MMS_TimeoutTimer::RemoveTimer(MMS_TimeoutTimer* theTimer)
{
/*	MMS_TimeoutTimer::myAccessMutex.Lock();

	// Just remove the timer --- Does not update myClosestTimeout (unless no timers left)
	myTimers.Remove(theTimer);
	unsigned int c;
	myTimers.Count(c);
	if (c == 0) MMS_TimeoutTimer::myClosestTimeout = -1;

	MMS_TimeoutTimer::myAccessMutex.Unlock();
*/
}


void 
MMS_TimeoutTimer::myTriggerTimer(MMS_TimeoutTimer* aTimer)
{
	MMS_TimeoutTimer::myAccessMutex.Lock();
	RemoveTimer(aTimer);
	if (aTimer->myCallback)
		aTimer->myCallback->TimerTimedOut(this);
	// BUG UNLOCK PROBABLY
}

MMS_TimeoutTimer::TimerTicker::TimerTicker()
: MT_Thread()
{
}

void
MMS_TimeoutTimer::TimerTicker::Run()
{
	while (!this->StopRequested())
	{
		Sleep(1000);
/*		myCurrentTime = time(NULL);
		MMS_TimeoutTimer::myAccessMutex.Lock();
		myTimers.PurgeArray(*myTimerTicker);
		MMS_TimeoutTimer::myAccessMutex.Unlock();
*/
	}
}

bool
MMS_TimeoutTimer::TimerTicker::Purge(MMS_TimeoutTimer*& theItem)
{
	if (theItem->myTimeoutTime <= MMS_TimeoutTimer::myCurrentTime)
	{
		theItem->myCallback->TimerTimedOut(theItem);
		return true;
	}
	return false;
}

MMS_ThreadSafeQueue<MMS_TimeoutTimer*> MMS_TimeoutTimer::myTimers;
MT_Mutex MMS_TimeoutTimer::myAccessMutex;
time_t MMS_TimeoutTimer::myClosestTimeout = -1;
time_t MMS_TimeoutTimer::myCurrentTime;
MMS_TimeoutTimer::TimerTicker* MMS_TimeoutTimer::myTimerTicker = NULL;

/*
unsigned int myTimeoutTime;
static MMS_ThreadSafeQueue<MMS_TimeoutTimer> myTimers;
static unsigned int myClosestTimeout;
static void myTimerFunc();
static MT_Mutex myAccessMutex;
*/