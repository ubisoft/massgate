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

#if IS_PC_BUILD
	#include <mmsystem.h>
	#pragma comment(lib, "winmm.lib")
#endif

#include "mi_time.h"
#include "mt_mutex.h"

#define NUM_TIME_AVERAGEFRAMES 3

MI_Time* MI_Time::ourInstance;
MI_TimeUnit MI_Time::ourStartTimes[NUM_MI_TIMEIDS];
float MI_Time::ourElapsedTime;
float MI_Time::ourCurrentTime;
double MI_Time::ourRealElapsedTime;
double MI_Time::ourRealCurrentTime;
unsigned int MI_Time::ourFrameCounter;
MI_TimerMethod MI_Time::ourTimerMethod =  MI_QPC;
unsigned int MI_Time::ourCurrentSystemTime = 0;

bool MI_Time::ourUseAveragedTimeGetTime = false;
bool MI_Time::ourTimerProblemDetected = false;

static MI_TimeUnit locPrevDeltaTime=0;
static MI_TimeUnit locFrequency = 0;
static double      locInvFrequency = 0;
static MI_TimeUnit locPrevTime;
static MI_TimeUnit locQPCFreq;
static MI_TimeUnit locQPCFreqDiv1000;
static MI_TimeUnit locOurQPC = 0;
static MI_TimeUnit locLastTimeForMaxFPS = 0;
static unsigned int locQPCLeapCounter = 0;
static bool			locQPCCheckForErrors = false;

#if IS_PC_BUILD
	static unsigned int locMinWinTimerRes;
#endif

static unsigned int locWinBeginTime;

static const int TIME_HISTORY_SIZE = 1024;
static unsigned int locTimeHistory[TIME_HISTORY_SIZE] = {0};
static unsigned int locTimeHistoryWriteIndex = 0;

static MT_Mutex& GetTimeMutex()
{
	static MT_Mutex timeMutex;
	return timeMutex;
}

double (__cdecl* MI_Time::ourTimeUpdateOverrideCallback)(double) = 0;


MI_Time::MI_Time()
{
	MT_MutexLock lock(GetTimeMutex());

	ourElapsedTime = 0.0f;
	ourCurrentTime = 0.0f;
	ourRealElapsedTime = 0.0;
	ourRealCurrentTime = 0.0;
	ourFrameCounter = 0;

	myTimeScale = 1.f;
	myMinElapsedTime = -1.0f;
	myMaxElapsedTime = -1.0f;
	myPauseFlag = false;

	myCurrentTimeIndex = 0;

	ourInternalElapsedTimes = new double[NUM_TIME_AVERAGEFRAMES];
	ourInternalCurrentTime = 0.0;

	int i;

	for(i = 0; i < NUM_TIME_AVERAGEFRAMES; i++)
		ourInternalElapsedTimes[i] = 0.001;

#if IS_PC_BUILD
	TIMECAPS tc;
	if(timeGetDevCaps(&tc, sizeof(tc)) == TIMERR_NOERROR)
	{
		locMinWinTimerRes = tc.wPeriodMin;
		timeBeginPeriod(locMinWinTimerRes);
	}
#endif

	locWinBeginTime = MI_Time::GetSystemTime();

	locTimeHistory[0] = locWinBeginTime;
	locTimeHistoryWriteIndex = 1;
	locQPCLeapCounter = 0;
	ourTimerProblemDetected = false;

	if(ourTimerMethod == MI_RDTSC)
		GetExactTime(&locPrevTime);
	else if(ourTimerMethod == MI_TIMEGETTIME)
	{
		locPrevTime = locWinBeginTime;
		locFrequency = 1000;
		locInvFrequency = 1.0 / double(locFrequency);
	}
	else if(ourTimerMethod == MI_QPC)
	{
		QueryPerformanceFrequency((LARGE_INTEGER*)&locQPCFreq);

		if(locQPCFreq != 0)
		{
			GetQPC(&locPrevTime);

			locQPCFreqDiv1000 = locQPCFreq / 1000;
			locFrequency = locQPCFreq;
			locInvFrequency = 1.0 / double(locFrequency);
			locQPCCheckForErrors = true;
		}
		else
		{
			// Switch timer method to TimeGetTime
			ourTimerMethod	= MI_TIMEGETTIME;
			locPrevTime = locWinBeginTime;
			locFrequency = 1000;
			locInvFrequency = 1.0 / double(locFrequency);

			ourTimerProblemDetected = true;
			locQPCCheckForErrors	= false;
			
			MC_DEBUG("Timer problem detected in QueryPerformanceFrequency, switching to TimeGetTime");
		}
	}
	else
	{
		assert(0);
	}

	Calibrate();
}

void MI_Time::Calibrate()
{
	unsigned int startTim;
	unsigned int endTim;
	MI_TimeUnit start=0, end=0;
	int i;

	locFrequency = 0;

	if(ourTimerMethod == MI_RDTSC)
	{
		for(i=0;i<10;i++)
		{
			startTim = MI_Time::GetSystemTime();
			if(ourTimerMethod == MI_RDTSC)
				GetExactTime(&start);
			else if(ourTimerMethod == MI_Time::GetSystemTime())
				start = MI_Time::GetSystemTime();

			Sleep(100);

			if(ourTimerMethod == MI_RDTSC)
				GetExactTime(&end);
			else if(ourTimerMethod == MI_Time::GetSystemTime())
				end = MI_Time::GetSystemTime();
			endTim = MI_Time::GetSystemTime();

			endTim = (startTim > endTim ? startTim - endTim : endTim - startTim);

			if(i>0)		//  First timing is skewed so ignore
				locFrequency+= (((end > start ? end - start : start - end)) * 1000) / endTim;
		}
		locFrequency/=9;
		locInvFrequency = 1.0 / double(locFrequency);
		GetExactTime(&locPrevTime);
	}
}

MI_Time::~MI_Time()
{
	MT_MutexLock lock(GetTimeMutex());

	delete [] ourInternalElapsedTimes;

#if IS_PC_BUILD
	if(locMinWinTimerRes < 0xFFFFFFFF)
		timeEndPeriod(locMinWinTimerRes);
#endif
}


bool MI_Time::Create()
{
	MT_MutexLock lock(GetTimeMutex());

	if(!ourInstance)
		ourInstance = new MI_Time();

	return true;
}


void MI_Time::Destroy()
{
	MT_MutexLock lock(GetTimeMutex());

	if(ourInstance)
	{
		delete ourInstance;
		ourInstance = NULL;
	}
}

unsigned int MI_Time::GetSystemTime()
{
	return timeGetTime();
}

void MI_Time::GetExactTime(MI_TimeUnit* aReturnTime)
{
	if(ourTimerMethod == MI_RDTSC)
		GetRDTSC(aReturnTime);
	else if(ourTimerMethod == MI_QPC)
		GetQPC(aReturnTime);
	else if (ourTimerMethod == MI_TIMEGETTIME)
		*aReturnTime = MI_Time::GetSystemTime() - locWinBeginTime;
	else
		assert(0);
}

bool MI_Time::GetQPC(MI_TimeUnit* aReturnTime)
{
	MT_MutexLock lock(GetTimeMutex());

	static MI_TimeUnit prevQpc;
	static DWORD prevTime;
	DWORD currTime;
	DWORD deltaTime;
	MI_TimeUnit currQpc;
	MI_TimeUnit deltaQpc;

	// Set init times
	if(locOurQPC == 0)
	{
		if(!QueryPerformanceCounter((LARGE_INTEGER*)&prevQpc))
			assert(0);

		prevTime = MI_Time::GetSystemTime();

		locOurQPC = 1;
	}

	// Read current times
	if(!QueryPerformanceCounter((LARGE_INTEGER*)&currQpc))
		assert(0);

	currTime = MI_Time::GetSystemTime();

	deltaQpc = currQpc - prevQpc;
	deltaTime = currTime - prevTime;

	// Detect leaps. Ref: http://support.microsoft.com/default.aspx?scid=KB;EN-US;Q274323&
	if((deltaQpc) <= 0 || (deltaQpc > (deltaTime+50) * locQPCFreqDiv1000))
	{
		// Re-init timers
		if(!QueryPerformanceCounter((LARGE_INTEGER*)&prevQpc))
			assert(0);
		prevTime = MI_Time::GetSystemTime();

		// Step our counter and return
		locOurQPC += 1;
		*aReturnTime = locOurQPC;
		
		locQPCLeapCounter++;

		return false;
	}
	else
	{
		prevQpc = currQpc;
		prevTime = currTime;

		// Step our counter and return
		locOurQPC += deltaQpc;
		*aReturnTime = locOurQPC;

		return true;
	}
}

void __stdcall MI_Time::GetRDTSC(MI_TimeUnit* aReturnTime)
{
	__asm
	{
		push edx
		push ecx
		push eax
		mov ecx, aReturnTime
		_emit 0x0f
		_emit 0x31
		mov [ecx], eax
		mov [ecx+4], edx
		pop eax
		pop ecx
		pop edx
	}
}

float MI_Time::ConvertTimeToSeconds(const MI_TimeUnit& aTime)
{
	return (float) (((double) aTime) * locInvFrequency);
}


void MI_Time::StartTimer(MI_TimeIds aTimerId)
{
	GetExactTime(&ourStartTimes[aTimerId]);
}


MI_TimeUnit MI_Time::ReadTimer(MI_TimeIds aTimerId)
{
	MI_TimeUnit tim;

	GetExactTime(&tim);

	return (tim > ourStartTimes[aTimerId] ? tim - ourStartTimes[aTimerId] : ourStartTimes[aTimerId] - tim);
}


void MI_Time::ResetCurrentTime()
{
	ourInternalCurrentTime = 0.0;
	ourCurrentTime = 0.0f;
}

void MI_Time::EnforceMaxFPS()
{
	MI_TimeUnit currTime;
	GetExactTime(&currTime);

	if(myMinElapsedTime >= 0)
	{
		MC_PROFILER_BEGIN(delay, "Delay to cap FPS");
		while(true)
		{
			MI_TimeUnit delta = currTime - locLastTimeForMaxFPS;
			float fDelta = MI_Time::ConvertTimeToSeconds(delta);
			if(fDelta >= myMinElapsedTime)
				break;
			else if(myMinElapsedTime - fDelta >= 0.001f)
				MC_Sleep(1);
			else
				MC_Sleep(0);
			GetExactTime(&currTime);
		}
	}

	locLastTimeForMaxFPS = currTime;
}

void MI_Time::Update()
{
	MT_MutexLock lock(GetTimeMutex());

	MI_TimeUnit iTim, iDeltaTim;
	double tim;
	int i;

	EnforceMaxFPS();

	const unsigned int currTime = MI_Time::GetSystemTime();

	if(ourUseAveragedTimeGetTime)
	{
		locTimeHistory[locTimeHistoryWriteIndex] = currTime;

		int count = 0;
		unsigned int totalTime = 0;
		for(i=1; i<TIME_HISTORY_SIZE; i++)
		{
			const int idx = (locTimeHistoryWriteIndex + TIME_HISTORY_SIZE - i) % TIME_HISTORY_SIZE;
			const unsigned int delta = currTime - locTimeHistory[idx];

			if(locTimeHistory[idx] == 0)
			{
				if(count > 0)
					break;
				else
				{
					totalTime = 50;
					count = 1;
				}
			}

			if(delta > 300 && count > 0 && totalTime > 0)
				break;

			totalTime = delta;
			count++;

			if(totalTime > 150)
				break;
		}

		locTimeHistoryWriteIndex = (locTimeHistoryWriteIndex + 1) % TIME_HISTORY_SIZE;

		tim = 0.001 * double(totalTime) / double(count);
	}
	else
	{
		if(ourTimerMethod == MI_RDTSC)
		{
			GetRDTSC(&iTim);
			iDeltaTim = (iTim > locPrevTime ? iTim - locPrevTime : locPrevTime - iTim);
		}
		else if(ourTimerMethod == MI_TIMEGETTIME)
		{
			iTim = MI_Time::GetSystemTime();
			iDeltaTim = iTim - locPrevTime;
		}
		else if(ourTimerMethod == MI_QPC)
		{
			if(GetQPC(&iTim))
			{
				iDeltaTim = iTim - locPrevTime;
			}
			else
			{
				// Bad QPC value, use previous delta.
				iDeltaTim = locPrevDeltaTime;
			}
		}
		else
			assert(0);

		ourInternalElapsedTimes[myCurrentTimeIndex] = ((double) iDeltaTim) * locInvFrequency;
		myCurrentTimeIndex = (myCurrentTimeIndex + 1) % NUM_TIME_AVERAGEFRAMES;

		locPrevTime = iTim;
		locPrevDeltaTime = iDeltaTim;

		tim = ourInternalElapsedTimes[0];
		for(i = 1; i < NUM_TIME_AVERAGEFRAMES; i++)
			tim += ourInternalElapsedTimes[i];

		if(NUM_TIME_AVERAGEFRAMES > 1)
			tim /= NUM_TIME_AVERAGEFRAMES;
	}

	ourRealElapsedTime = tim;
	ourRealCurrentTime += tim;

	assert(ourRealElapsedTime >= 0.0);

	if(myMinElapsedTime >= 0 && tim < myMinElapsedTime)
		tim = myMinElapsedTime;

	if(myMaxElapsedTime >= 0 && tim > myMaxElapsedTime)
		tim = myMaxElapsedTime;

	if(ourTimeUpdateOverrideCallback)
		tim = ourTimeUpdateOverrideCallback(tim);

	tim *= myTimeScale; // premultiplier for time scaling, moved to after fps limiting for better video recording support

	if(myPauseFlag)
		tim = 0.0f;

	ourInternalCurrentTime += tim;

	assert(ourInternalCurrentTime >= 0.0);

	ourElapsedTime = (float) tim;

	if(ourElapsedTime == 0.0f)
		ourElapsedTime = 0.00001f;
	ourCurrentTime = (float) ourInternalCurrentTime;

	assert(ourCurrentTime >= 0.0f);

	ourFrameCounter++;

	ourCurrentSystemTime = currTime;

	// QPC error detection
	if( locQPCCheckForErrors && (ourFrameCounter > 500))
	{
		// Give up on QPC time if the amount leaps are greater than 2%
		float precentageWrong = (float) locQPCLeapCounter / (float) ourFrameCounter;
		if( precentageWrong > 0.02f )
		{
			// Switch timer method to TimeGetTime
			ourTimerMethod	= MI_TIMEGETTIME;
			locPrevTime		= GetSystemTime();
			locFrequency	= 1000;
			locInvFrequency = 1.0 / double(locFrequency);

			ourTimerProblemDetected = true;
			locQPCCheckForErrors	= false;
			
			MC_DEBUG("Timer problem detected, switching to TimeGetTime");
		}
	}
}


void MI_Time::SetMaxFPS(float aFPS)
{
	myMinElapsedTime = (aFPS <= 0 ? -1 : 1.0f / aFPS);
}


void MI_Time::SetMinFPS(float aFPS)
{
	myMaxElapsedTime = (aFPS <= 0 ? -1 : 1.0f / aFPS);
}

void MI_Time::SetTimeScale(float aScaleFactor)
{
	myTimeScale = aScaleFactor;
}

void MI_Time::Pause(bool aState)
{
	myPauseFlag = aState;
}

bool MI_Time::IsPaused()
{
	return myPauseFlag;
}


void MI_Time::GetDateAndTimeString(char* aResult255CharArray, bool aIncludeDate, bool aIncludeHours, bool aIncludeMin, bool aIncludeSecs)
{
	SYSTEMTIME stLocalTime;
	GetLocalTime( &stLocalTime );

	aResult255CharArray[0] = 0;

	if (aIncludeDate)
		sprintf(aResult255CharArray, "%04d%02d%02d", stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay);
	if (aIncludeHours || aIncludeMin || aIncludeSecs)
		strcat(aResult255CharArray, "_");
	if (aIncludeHours)
		sprintf(&aResult255CharArray[strlen(aResult255CharArray)], "%02dh", stLocalTime.wHour);
	if (aIncludeMin)
		sprintf(&aResult255CharArray[strlen(aResult255CharArray)], "%02dm", stLocalTime.wMinute);
	if (aIncludeSecs)
		sprintf(&aResult255CharArray[strlen(aResult255CharArray)], "%02ds", stLocalTime.wSecond);
}

void MI_Time::GetDateAndTime(int& aResultYear, int& aResultMonth, int& aResultDay, int& aResultHour, int& aResultMin, int& aResultSec, int& aResultMs)
{
	SYSTEMTIME stLocalTime;
	GetLocalTime( &stLocalTime );

	aResultYear	= stLocalTime.wYear;
	aResultMonth= stLocalTime.wMonth;
	aResultDay	= stLocalTime.wDay;
	aResultHour = stLocalTime.wHour;
	aResultMin	= stLocalTime.wMinute;
	aResultSec	= stLocalTime.wSecond;
	aResultMs	= stLocalTime.wMilliseconds;
}
