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
#include "MT_Event.h"

MT_Event::MT_Event(void) 
{
	// Creates the event object
	// initially set to unsignalled
	// myEvent = CreateEvent(NULL,FALSE,FALSE,NULL);	// SWFM:SWR - changed to auto reset
	// SWFM:SWR - changed back to manual reset (background loader requires this)
	myEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
}

MT_Event::~MT_Event(void)
{
	CloseHandle(myEvent);
}

void MT_Event::ClearSignal()
{
	ResetEvent(myEvent);
}

void MT_Event::Signal()
{
	SetEvent(myEvent);
}

void MT_Event::WaitForSignal()
{
	MC_THREADPROFILER_ENTER_WAIT();

	if (WaitForSingleObject(myEvent, INFINITE) != WAIT_OBJECT_0)
	{
		assert(0);
	}

	MC_THREADPROFILER_LEAVE_WAIT();
}

void MT_Event::TimedWaitForSignal(const u32 aTimeoutPeriod, bool& signalled)
{
	// Performs a timed wait on the semaphore (in ms), returning whether
	// the operation succeeded, and populating the supplied bool reference
	// with a value corresponding to whether the semaphore was signalled
	// - not signalled assumes a timeout occurred.

	MC_THREADPROFILER_ENTER_WAIT();

	signalled = false;

	DWORD result = WaitForSingleObject(myEvent, aTimeoutPeriod);
	if (result == WAIT_OBJECT_0)
		signalled = true;
	else if (result == WAIT_TIMEOUT)
		signalled = false;
	else
		assert(0);

	MC_THREADPROFILER_LEAVE_WAIT();
}
