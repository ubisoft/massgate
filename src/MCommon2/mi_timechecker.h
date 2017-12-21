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
#ifndef MI_TIMECHECKER_H
#define MI_TIMECHECKER_H


#include "mc_growingarray.h"
#include "mi_time.h"


struct MI_TimeCheckerIdFramePartData
{
	MI_TimeUnit myStartTime;
	MI_TimeUnit myEndTime;
};


struct MI_TimeCheckerIdFrameData
{
	unsigned int myFrameNum;

	MC_GrowingArray<MI_TimeCheckerIdFramePartData> myElapsedTimes;
};


struct MI_TimeCheckerIdData
{
	unsigned int myId;

	unsigned int myLastFrameNum;

	MC_GrowingArray<MI_TimeCheckerIdFrameData*> myFrames;
};


class MI_TimeChecker
{
private:
	static MC_GrowingArray<MI_TimeCheckerIdData*> ourIds;
	static MC_GrowingArray<MI_TimeCheckerIdFramePartData> ourFrameTimes;

	static unsigned int ourFrameCount;

	static bool ourIsInitedFlag;

	static bool ourIsEnabledFlag;

	static bool ourShouldEnableFlag;
	static bool ourShouldDisableFlag;

public:
	static void Enable() { ourShouldEnableFlag = true; }
	static void Disable() { ourShouldDisableFlag = true; }

	static void BeginFrame();
	static void EndFrame();

	static void BeginCheck(unsigned int anId);
	static void EndCheck(unsigned int anId);

	static void Clear();
};


#endif // MI_TIMECHECKER_H
