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
#include "mi_timechecker.h"
#include "mf_file.h"


unsigned int MI_TimeChecker::ourFrameCount;
MC_GrowingArray<MI_TimeCheckerIdData*> MI_TimeChecker::ourIds;
MC_GrowingArray<MI_TimeCheckerIdFramePartData> MI_TimeChecker::ourFrameTimes;
bool MI_TimeChecker::ourIsInitedFlag;
bool MI_TimeChecker::ourIsEnabledFlag = true;
bool MI_TimeChecker::ourShouldEnableFlag;
bool MI_TimeChecker::ourShouldDisableFlag;


void MI_TimeChecker::BeginFrame()
{
	MI_TimeCheckerIdFramePartData tim;

	if(ourShouldEnableFlag)
	{
		ourIsEnabledFlag = true;
		ourShouldEnableFlag = false;
	}

	if(ourShouldDisableFlag)
	{
		ourIsEnabledFlag = false;
		ourShouldDisableFlag = false;
	}

	if(!ourIsEnabledFlag)
		return;

	if(!ourIsInitedFlag)
	{
		ourIds.Init(32, 32, false);
		ourFrameTimes.Init(4096, 4096, false);
		ourIsInitedFlag = true;
	}
	else
		ourFrameCount++;

	MI_Time::GetExactTime(&tim.myStartTime);
	ourFrameTimes.Add(tim);
}


void MI_TimeChecker::EndFrame()
{
	if(!ourIsEnabledFlag)
		return;

	MI_Time::GetExactTime(&ourFrameTimes[ourFrameTimes.Count() - 1].myEndTime);
}


void MI_TimeChecker::BeginCheck(unsigned int anId)
{
	MI_TimeCheckerIdData* id;
	MI_TimeCheckerIdFrameData* frame;
	MI_TimeCheckerIdFramePartData tim;
	MI_TimeCheckerIdFramePartData* timPtr;
	int i;

	if(!ourIsEnabledFlag)
		return;

	for(i = 0; i < ourIds.Count(); i++)
		if(ourIds[i]->myId == anId)
		{
			id = ourIds[i];
			break;
		}

	if(i == ourIds.Count())
	{
		id = new MI_TimeCheckerIdData;
		id->myId = anId;
		id->myLastFrameNum = -1;
		id->myFrames.Init(4096, 4096, false);
		ourIds.Add(id);
	}

	if(id->myLastFrameNum == ourFrameCount)
	{
		frame = id->myFrames[id->myFrames.Count() - 1];
	}
	else
	{
		frame = new MI_TimeCheckerIdFrameData;
		frame->myElapsedTimes.Init(1, 1, false);
		id->myFrames.Add(frame);
	}

	if(frame->myElapsedTimes.Count() > 0)
	{
		timPtr = &frame->myElapsedTimes[frame->myElapsedTimes.Count() - 1];
		assert(timPtr->myEndTime != timPtr->myStartTime - 1); // make sure EndCheck() has been issued before new StartCheck()
	}

	MI_Time::GetExactTime(&tim.myStartTime);
	tim.myEndTime = tim.myStartTime - 1;
	frame->myElapsedTimes.Add(tim);
	frame->myFrameNum = ourFrameCount;

	id->myLastFrameNum = ourFrameCount;
}


void MI_TimeChecker::EndCheck(unsigned int anId)
{
	MI_TimeCheckerIdData* id;
	MI_TimeCheckerIdFrameData* frame;
	MI_TimeCheckerIdFramePartData* tim;
	int i;

	if(!ourIsEnabledFlag)
		return;

	for(i = 0; i < ourIds.Count(); i++)
		if(ourIds[i]->myId == anId)
		{
			id = ourIds[i];
			break;
		}

	assert(i < ourIds.Count()); // id must exist
	assert(ourFrameCount == id->myLastFrameNum); // make sure a StartCheck() has been issued this frame
	frame = id->myFrames[id->myFrames.Count() - 1];
	tim = &frame->myElapsedTimes[frame->myElapsedTimes.Count() - 1];
	assert(tim->myEndTime == tim->myStartTime - 1); // make sure EndCheck() has been issued only once for each StartCheck()

	MI_Time::GetExactTime(&tim->myEndTime);
}


void MI_TimeChecker::Clear()
{
	int i;

	for(i = 0; i < ourIds.Count(); i++)
	{
		ourIds[i]->myFrames.DeleteAll();
	}

	ourIds.DeleteAll();
}
