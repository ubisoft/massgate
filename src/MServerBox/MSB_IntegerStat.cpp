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
#include "StdAfx.h"

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_IntegerStat.h"

MSB_IntegerStat::MSB_IntegerStat()
	: myIsValueSet(false)
	, myCurrent(0)
	, myMax(0x8000000000000000L)
	, myMin(0x07FFFFFFFFFFFFFFL)
	, mySum(0)
	, myCount(0)
{
}

MSB_IntegerStat::~MSB_IntegerStat()
{

}

void
MSB_IntegerStat::Add(
	int64			aDelta)
{
	MT_MutexLock	lock(myLock);

	int64			value = myCurrent + aDelta;

	if(value < myMin)
		myMin = value;

	if(value > myMax)
		myMax = value;

	myCurrent = value;

	mySum += value;
	myCount ++;

	myIsValueSet = true;
}

void
MSB_IntegerStat::Set(
	int64			aValue)
{
	MT_MutexLock	lock(myLock);

	if(aValue < myMin)
		myMin = aValue;

	if(aValue > myMax)
		myMax = aValue;

	myCurrent = aValue;

	mySum += aValue;
	myCount ++;
}

int64
MSB_IntegerStat::Get() const
{
	return myCurrent;
}

void
MSB_IntegerStat::Marshal(
	MSB_WriteMessage&	aMessage)
{
	MT_MutexLock		lock(myLock);

	if(myIsValueSet)
	{
		aMessage.WriteInt64(myCurrent);
		aMessage.WriteInt64(myMin);
		aMessage.WriteInt64(myMax);
		aMessage.WriteInt64(myCount);
		aMessage.WriteInt64(mySum);
	}
	else
	{
		aMessage.WriteInt64(0);
		aMessage.WriteInt64(0);
		aMessage.WriteInt64(0);
		aMessage.WriteInt64(0);
		aMessage.WriteInt64(0);
	}
}

void
MSB_IntegerStat::ToString(
	MSB_WriteMessage&	aMessage)
{
	MT_MutexLock		lock(myLock);

	MC_String t;

	t.Format("Current: %I64d, Min: %I64d, Max: %I64d, count: %I64d, Sum: %I64d", 
		myCurrent, myMin, myMax, myCount, mySum);

	aMessage.WriteString(t.GetBuffer());
}

#endif // IS_PC_BUILD
