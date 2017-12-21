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
#include "mdb_timeable.h"
#include "mmsystem.h"

void
MDB_Timeable::BeginTimer(TimerIndex theTimer)
{
	assert(theTimer < NUM_TIMERINDEXES);
	myLastBeginTime[theTimer] = timeGetTime();
}

void
MDB_Timeable::EndTimer(TimerIndex theTimer)
{
	assert(theTimer < NUM_TIMERINDEXES);
	myNumberOfTimings[theTimer]++;
	myTotalTime[theTimer] += (timeGetTime() - myLastBeginTime[theTimer]);
}

double
MDB_Timeable::GetTotalTime(TimerIndex theTimer)
{
	assert(theTimer < NUM_TIMERINDEXES);
	return (myTotalTime[theTimer]) / 1000.0;
}

double
MDB_Timeable::GetAverageTime(TimerIndex theTimer)
{
	assert(theTimer < NUM_TIMERINDEXES);
	return (myTotalTime[theTimer] / 1000.0) / double(myNumberOfTimings[theTimer]);
}

MDB_Timeable::MDB_Timeable()
{
	memset(myLastBeginTime, 0, sizeof(myLastBeginTime));
	memset(myNumberOfTimings, 0, sizeof(myNumberOfTimings));
	for (int i=0; i<NUM_TIMERINDEXES; i++)
		myTotalTime[i] = 0.0;
}

MDB_Timeable::MDB_Timeable(const MDB_Timeable& aRhs)
{
	*this = aRhs;
}

MDB_Timeable& 
MDB_Timeable::operator=(const MDB_Timeable& aRhs)
{
	memcpy(myLastBeginTime, aRhs.myLastBeginTime, sizeof(myLastBeginTime));
	memcpy(myNumberOfTimings, aRhs.myNumberOfTimings, sizeof(myNumberOfTimings));
	memcpy(myTotalTime, aRhs.myTotalTime, sizeof(myTotalTime));
	return *this;
}


MDB_Timeable::~MDB_Timeable()
{
}
