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
#ifndef MMS_TIMEOUTTIMER___H_
#define MMS_TIMEOUTTIMER___H_

#include "MMS_ThreadSafeQueue.h"
#include "MT_Mutex.h"
#include "MT_Thread.h"
#include <time.h>


class MMS_TimeoutTimer
{
public:

	class MMS_ITimeoutCallback
	{
	public: 
		virtual void TimerTimedOut(MMS_TimeoutTimer* theTimer) = 0;
	};

	// No timers will be triggered if the TimeoutTimer isn't initialized
	static void Init();
	static void AddTimer(MMS_TimeoutTimer* theTimer);
	static void RemoveTimer(MMS_TimeoutTimer* theTimer);
	static MMS_TimeoutTimer* Allocate();
	static MMS_TimeoutTimer* Allocate(unsigned int aTimeoutInSeconds, MMS_ITimeoutCallback* theCallbackClass);





	MMS_TimeoutTimer& operator=(const MMS_TimeoutTimer& aRhs);
	bool operator==(const MMS_TimeoutTimer& aRhs);
	bool operator<(const MMS_TimeoutTimer& aRhs);

	virtual ~MMS_TimeoutTimer();



protected:
	MMS_TimeoutTimer();
	MMS_TimeoutTimer(unsigned int aTimeoutInSeconds, MMS_ITimeoutCallback* theCallbackClass);
	MMS_TimeoutTimer(const MMS_TimeoutTimer& aRhs);
	void Set(unsigned int aTimeoutInSeconds, MMS_ITimeoutCallback* theCallbackClass);

	void myTriggerTimer(MMS_TimeoutTimer* aTimer);
	time_t myTimeoutTime;
	MMS_ITimeoutCallback* myCallback;

private:
	static MMS_ThreadSafeQueue<MMS_TimeoutTimer*> myTimers;
	static time_t myClosestTimeout;
	static time_t myCurrentTime;
	static MT_Mutex myAccessMutex;
	class TimerTicker : public MT_Thread, MMS_ThreadSafeQueue<MMS_TimeoutTimer*>::IArrayPurger<MMS_TimeoutTimer*>
	{
	public:
		TimerTicker();
		virtual void Run();
		virtual bool Purge(MMS_TimeoutTimer*& theItem);
	};
	friend class TimerTicker;
	static TimerTicker* myTimerTicker;
	bool myIsActive;
};

#endif
