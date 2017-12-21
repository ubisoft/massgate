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
#ifndef __MDB_TIMEABLE_H__
#define __MDB_TIMEABLE_H__

class MDB_Timeable
{
public:
	typedef enum { TIMER_ONE, TIMER_TWO, TIMER_THREE, TIMER_FOUR, NUM_TIMERINDEXES } TimerIndex;
	void BeginTimer(TimerIndex theTimer);
	void EndTimer(TimerIndex theTimer);
	double GetTotalTime(TimerIndex theTimer);
	double GetAverageTime(TimerIndex theTimer);

	MDB_Timeable();
	MDB_Timeable(const MDB_Timeable& aRhs);
	MDB_Timeable& operator=(const MDB_Timeable& aRhs);
	virtual ~MDB_Timeable();
private:
	unsigned long myLastBeginTime[NUM_TIMERINDEXES];
	unsigned long myNumberOfTimings[NUM_TIMERINDEXES];
	double myTotalTime[NUM_TIMERINDEXES];
};

#endif //__MDB_TIMEABLE_H__
