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

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_HashTable.h"
#include "MSB_Stats.h"
#include "MSB_StatsContext.h"

#include "MSB_IntegerStat.h"

/**
 * Never call this directly, not for public use anyway.
 *
 */
MSB_StatsContext::MSB_StatsContext()
{

}

void
MSB_StatsContext::InsertValue(
	const char*			aName,
	IValue*				aValue)
{
	MT_MutexLock		lock(myMutex);

	assert(myValues.HasKey(aName) == false);
	myValues.Add(aName, aValue);
}

bool
MSB_StatsContext::HasStat(
	const char*			aName)
{
	return myValues.HasKey(aName);
}

MSB_StatsContext::IValue&
MSB_StatsContext::operator[](
	const char*			aName)
{
	return *myValues[aName];
}

void
MSB_StatsContext::Marshal(
	MSB_WriteMessage&	aWriteMessage)
{
	ValueTable::Iterator	i(myValues);
	aWriteMessage.WriteUInt16(myValues.Count());
	
	while(i.Next())
		i.GetItem()->Marshal(aWriteMessage);
}

void
MSB_StatsContext::MarshalMetaData(
	MSB_WriteMessage&	aWriteMessage)
{
	ValueTable::Iterator	i(myValues);
	aWriteMessage.WriteUInt16(myValues.Count());

	while(i.Next())
		aWriteMessage.WriteString(i.GetKey());
}

void
MSB_StatsContext::GetNames(
	MSB_IArray<MC_String>& someNames)
{
	ValueTable::Iterator	i(myValues);

	while(i.Next())
		someNames.Add(MC_String((const char*)i.GetKey()));
}

#endif // IS_PC_BUILD
