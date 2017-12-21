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
#ifndef MT_THREAD__H_
#define MT_THREAD__H_

#include "MT_Mutex.h"

class MT_Thread
{
public:
	MT_Thread();
	// Create the thread and invoke Run() from it
	bool			Start();
	void			Stop();		// Non-blocking, just sets stop requested flag
	void			StopAndDelete();

	void			Suspend();
	unsigned int	Resume();

	bool			StopRequested() const { UpdateHardwareThread(); return myStopRequested; };
	bool			IsRunning()		const { return myThreadHandle != -1; };

	uintptr_t		GetHandle()		const { return myThreadHandle; }
	DWORD			GetThreadId()	const { return myThreadId; }

	void			ChangeHardwareThread(const u32 aHardwareThread);

	static void		SuspendAllThreads(DWORD anExceptionThread);

protected:
	// Your thread implementation should be in Run. model like this: void Run() { while (!StopRequested()) { ... } }
	virtual void Run() = 0;

	// You can not place a thread on the stack! Threads must be allocated on the heap.
	virtual ~MT_Thread();

private:
	volatile bool		myStopRequested;
	uintptr_t volatile	myThreadHandle;
	UINT				myThreadId;
	friend void MT_Thread_thread_starter(void* aThread);


	void				UpdateHardwareThread() const {};

	static MT_Thread*			 ourThreads[256];
	static volatile unsigned int ourNumThreads;
};



#endif
