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

#include "MSB_SimpleCounterStat.h"
#include "MSB_WriteMessage.h"

MSB_SimpleCounterStat::MSB_SimpleCounterStat()
	: MSB_StatsContext::IValue()
	, myValue(0)
{
	
}

MSB_SimpleCounterStat::~MSB_SimpleCounterStat()
{
	
}

void
MSB_SimpleCounterStat::Marshal(
	MSB_WriteMessage&	aWriteMessage)
{
	long		value = myValue;
	aWriteMessage.WriteInt32(value);
}

void
MSB_SimpleCounterStat::ToString(
	MSB_WriteMessage&	aMessage)
{
	MC_String t;

	t.Format("Value: %d", myValue);

	aMessage.WriteString(t.GetBuffer());
}

#endif // IS_PC_BUILD
