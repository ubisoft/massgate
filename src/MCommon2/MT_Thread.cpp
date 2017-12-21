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
#include "MT_Thread.h"
#include "MT_ThreadingTools.h"
#include "mc_misc.h"
#include <process.h>
#include <float.h>
#include "mc_commandline.h"

static void MT_Thread_thread_starter(void* aThread);

MT_Thread*	MT_Thread::ourThreads[256] = {NULL};
volatile unsigned int MT_Thread::ourNumThreads = 0;

MT_Thread::MT_Thread()
:myThreadHandle(-1)
,myThreadId(0)
,myStopRequested(true)
{
}

bool
MT_Thread::Start()
{
	myStopRequested = false;
	myThreadHandle = _beginthread(MT_Thread_thread_starter, 0, static_cast<void*>(this));

	//fix for hyperthreaded cpus making the main thread run slower
#if IS_PC_BUILD
	if(myThreadHandle != -1)
	{
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		if(!MC_CommandLine::GetInstance()->IsPresent("noaffinitymasking") && sysInfo.dwNumberOfProcessors >= 8)
			SetThreadAffinityMask((HANDLE)myThreadHandle, ~3);
	}
#endif


	return myThreadHandle != -1;
}

void
MT_Thread::Stop()
{
	myStopRequested = true;
}

void
MT_Thread::StopAndDelete()
{
	myStopRequested = true;

	int retryCount = 0;
	while(myThreadHandle != -1)
	{
		// Yield
		if(retryCount++ < 100)
			MC_Sleep(0);
		else
			MC_Sleep(1);

#ifndef _RELEASE_
		if(retryCount > 10000)
		{
			MC_ERROR("Deadlock in StopAndDelete");
			MT_ThreadingTools::PrintThreadingStatus(true);
			assert(0 && "Stuck in MC_Thread::StopAndDelete");
		}
#endif
	}

	delete this;
}

void
MT_Thread::Suspend()
{
	// If the thread is making a kernel call, SuspendThread fails.
	// We might need to repeat the SuspendThread call several times for it to succeed.
	while(SuspendThread((HANDLE)myThreadHandle) == 0xffffffff)
		MC_Sleep(0);
}

void
MT_Thread::SuspendAllThreads(DWORD anExceptionThread)
{
	// This whole suspend threads thingy is, ironically, NOT THREADSAFE! If some threads are left hanging at app exit,
	// a static ourMutex is destroyed before the thread gets killed.
	// 1. This function should only be called due to a crash
	// 2. A thread should not spawn a thread, but rather, all threads should be spawned from within the same thread - then
	//    there is no race condition.
	for (unsigned int i = 0; i < ourNumThreads; i++)
	{
		if (ourThreads[i]->GetThreadId() != anExceptionThread)
			ourThreads[i]->Suspend();
	}
}

unsigned int
MT_Thread::Resume()
{
	return ResumeThread((HANDLE)myThreadHandle);
}

MT_Thread::~MT_Thread()
{
	assert(myThreadHandle == -1);
}

void
MT_Thread_thread_starter(void* aThread)
{
	MT_Thread* thr;

	// Init stack top pointer for this thread
	MC_IsProbablyOnStack(0);

	MT_ThreadingTools::SetCurrentThreadName("Unnamed MT_Thread");

	#ifdef MC_SET_FLOAT_ROUNDING_CHOP
	_clearfp();
	_controlfp(_RC_CHOP, _MCW_RC); // make sure rounding is set to truncation
	#endif

	#ifdef MC_ENABLE_FLOAT_EXCEPTIONS
	_clearfp();
	_controlfp(~(_EM_OVERFLOW | _EM_ZERODIVIDE | _EM_INVALID), MCW_EM);
	#endif //MC_ENABLE_FLOAT_EXCEPTIONS

	thr = static_cast<MT_Thread*>(aThread);

	assert(MT_Thread::ourNumThreads < 256);
	MT_Thread::ourThreads[MT_Thread::ourNumThreads++] = thr;

	thr->myThreadId = GetCurrentThreadId();

	thr->Run();

	for (unsigned int i = 0; i < MT_Thread::ourNumThreads; i++)
	{
		if (MT_Thread::ourThreads[i] == thr)
		{
			MT_Thread::ourThreads[i] = MT_Thread::ourThreads[--MT_Thread::ourNumThreads];
			break;
		}
	}
	thr->myThreadHandle = -1;

	_endthread();
}

// SWFM:SWR - allow hardware thread to be changed
void MT_Thread::ChangeHardwareThread(const u32 aHardwareThread)
{

}
