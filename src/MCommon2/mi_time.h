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
#ifndef MI_TIME_H
#define MI_TIME_H


enum MI_TimeIds
{
	MI_TIMEID_0,
	MI_TIMEID_1,
	MI_TIMEID_2,
	MI_TIMEID_3,
	MI_TIMEID_4,
	MI_TIMEID_5,
	MI_TIMEID_6,
	MI_TIMEID_7,

	NUM_MI_TIMEIDS
};


typedef unsigned __int64 MI_TimeUnit;

enum MI_TimerMethod
{
	MI_RDTSC,
	MI_TIMEGETTIME,
	MI_QPC,
};

class MI_Time
{
private:
	MI_Time();
	~MI_Time();

	static MI_Time* ourInstance;

	static MI_TimeUnit ourStartTimes[NUM_MI_TIMEIDS];

	double* ourInternalElapsedTimes;
	double ourInternalCurrentTime;

	int myCurrentTimeIndex;

	void Calibrate();

public:
	static bool Create();
	static void Destroy();

	static MI_Time* GetInstance() { return ourInstance; }

	static unsigned int GetSystemTime();	// Returns system time in ms

	static void GetExactTime(MI_TimeUnit* aReturnTime);
	static MI_TimeUnit GetExactTime() { MI_TimeUnit t; GetExactTime(&t); return t; }
	static float ConvertTimeToSeconds(const MI_TimeUnit& aTime);

	static void StartTimer(MI_TimeIds aTimerId);
	static MI_TimeUnit ReadTimer(MI_TimeIds aTimerId);

	static void GetDateAndTimeString(char* aResult255CharArray, bool aIncludeDate, bool aIncludeHours, bool aIncludeMin, bool aIncludeSecs);	// dev util: returns "20060801_18h17m00s"
	static void GetDateAndTime(int& aResultYear, int& aResultMonth, int& aResultDay, int& aResultHour, int& aResultMin, int& aResultSec, int& aResultMs);

	static float ourElapsedTime;		// Frame delta time, in seconds
	static float ourCurrentTime;		// Time elapsed since program start, in seconds

	static double ourRealElapsedTime;	// Not affected by FPS capping or pausing
	static double ourRealCurrentTime;	// Not affected by FPS capping or pausing

	static unsigned int ourCurrentSystemTime;	// In milliseconds since OS start (Note: don't cast this to a float, precision will be terrible! Subtract ints to get delta value, then cast to float if you must)

	static unsigned int ourFrameCounter;

	static bool ourUseAveragedTimeGetTime;
	static bool ourTimerProblemDetected;

	void ResetCurrentTime();
	void Update(); // call once each frame

	void SetMaxFPS(float aFPS = -1.0f);
	void SetMinFPS(float aFPS = -1.0f);

	void Pause(bool aState);
	bool IsPaused();

	void SetTimeScale(float aScaleFactor = 1.f); // premultiplier for time values (ourElapsedTime/ourCurrentTime)
	float GetTimeScale() const { return myTimeScale; }

	void EnforceMaxFPS();

	static void __stdcall GetRDTSC(MI_TimeUnit* aReturnTime);
	static bool GetQPC(MI_TimeUnit* aReturnTime);	// Returns false if a leap was detected

	float myMinElapsedTime;
	float myMaxElapsedTime;
	bool myPauseFlag;

	float myTimeScale;

	static MI_TimerMethod ourTimerMethod;

	// Global Time Override Callback, use with extreme caution!
	// (currently used by ingame cinematics to compensate for sync problems with sound engine when using Averaged time-get-time)
	static double (__cdecl* ourTimeUpdateOverrideCallback)(double);
};


#endif // MI_TIME_H
