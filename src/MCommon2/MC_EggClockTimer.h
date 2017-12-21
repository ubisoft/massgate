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
#ifndef MC_EGGCLOCKTIMER_H
#define MC_EGGCLOCKTIMER_H

#include "mi_time.h"

class MC_EggClockTimer
{
public: 
	MC_EggClockTimer(unsigned int aTimeoutMillis)
	: myTimeoutMillis(aTimeoutMillis)
	, myStartTimeMillis(MI_Time::GetSystemTime())
	{	
	}

	MC_EggClockTimer()
	: myTimeoutMillis(0)
	, myStartTimeMillis(0)
	{
	}

	~MC_EggClockTimer()
	{
	}

	bool HasExpired()
	{
		unsigned int currentTimeMillis = MI_Time::GetSystemTime();
		if((currentTimeMillis - myStartTimeMillis) >= myTimeoutMillis)
		{
			myStartTimeMillis = currentTimeMillis; 
			return true; 
		}
		return false; 
	}

	void Reset()
	{
		myStartTimeMillis = MI_Time::GetSystemTime(); 
	}

	void SetTimeout(unsigned int aTimeoutMillis)
	{
		myStartTimeMillis = MI_Time::GetSystemTime(); 
		myTimeoutMillis = aTimeoutMillis;
	}

	unsigned int TimeLeft()
	{
		return myTimeoutMillis - (MI_Time::GetSystemTime() - myStartTimeMillis); 
	}

private: 
	unsigned int myTimeoutMillis; 
	unsigned int myStartTimeMillis; 
};

#endif
