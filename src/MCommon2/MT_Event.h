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
#ifndef MT_EVENT___H_
#define MT_EVENT___H_

#include "MC_Platform.h"
class MT_Event
{
public:
	MT_Event(void);
	~MT_Event(void);

	void	ClearSignal();
	void	Signal();
	void	WaitForSignal();
	void	TimedWaitForSignal(const u32 aTimeoutPeriod, bool& signalled);

private:
	HANDLE	myEvent;
};

#endif
