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
#include <stdafx.h>

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_HashTable.h"

/**
* Standard helper
*/

// Integers
MSB_Hash
MSB_StandardHashHelper::Hash1(
	int32			aValue)
{
	aValue = (aValue ^ 61) ^ (aValue >> 16);
	aValue = aValue + (aValue << 3);
	aValue = aValue ^ (aValue >> 4);
	aValue = aValue * 0x27d4eb2d;
	aValue = aValue ^ (aValue >> 15);
	return aValue;	
}

MSB_Hash
MSB_StandardHashHelper::Hash2(
	int32			aValue)
{
	aValue = (aValue+0x7ed55d16) + (aValue<<12);
	aValue = (aValue^0xc761c23c) ^ (aValue>>19);
	aValue = (aValue+0x165667b1) + (aValue<<5);
	aValue = (aValue+0xd3a2646c) ^ (aValue<<9);
	aValue = (aValue+0xfd7046c5) + (aValue<<3);
	aValue = (aValue^0xb55a4f09) ^ (aValue>>16);

	return aValue;
}

int32
MSB_StandardHashHelper::Retain(
   int32			aValue)
{
	return aValue;
}

void
MSB_StandardHashHelper::Release(
	int32			aValue)
{
}

bool
MSB_StandardHashHelper::Equal(
	int32			aValue1, 
	int32			aValue2)
{
	return aValue1 == aValue2;
}

// c-strings

// Dan Bernstein
MSB_Hash
MSB_StandardHashHelper::Hash1(
	const char*		aString)
{
	MSB_Hash		hash = 5381;
	char			c;

	while((c = *aString++) != 0)
		hash = hash * 33 ^ c;

	return hash;
}

// SDBM
MSB_Hash
MSB_StandardHashHelper::Hash2(
	const char*		aString)
{
	MSB_Hash		hash = 0;
	char			c;

	while((c = *aString++) != 0)
		hash = c + (hash<<6) + (hash << 16) - hash;

	return hash;
}

const char*
MSB_StandardHashHelper::Retain(
	const char*		aString)
{
	size_t		len = strlen(aString);
	char*		data = new char[len+1];
	memcpy(data, aString, len+1);
	return data;
}

void
MSB_StandardHashHelper::Release(
	const char*		aString)
{
	delete [] aString;
}

bool
MSB_StandardHashHelper::Equal(
	const char*		aString1, 
	const char*		aString2)
{
	return strcmp(aString1, aString2) == 0;
}

// Wide-strings

// Dan Bernstein
MSB_Hash		
MSB_StandardHashHelper::Hash1(const wchar_t* aString)
{
	MSB_Hash		hash = 5381;
	wchar_t			c;

	while((c = *aString++) != 0)
		hash = hash * 33 ^ c;

	return hash;
}

// SDBM
MSB_Hash
MSB_StandardHashHelper::Hash2(const wchar_t* aString)
{
	MSB_Hash		hash = 0;
	wchar_t			c;

	while((c = *aString++) != 0)
		hash = c + (hash<<6) + (hash << 16) - hash;

	return hash;
}

const wchar_t*
MSB_StandardHashHelper::Retain(const wchar_t* aString)
{
	size_t		len = wcslen(aString);
	wchar_t*	data = new wchar_t[len+1];
	memcpy(data, aString, (len+1) * sizeof(wchar_t));
	return data;
}

void
MSB_StandardHashHelper::Release(const wchar_t* aValue)
{
	delete [] aValue;
}

bool
MSB_StandardHashHelper::Equal(const wchar_t* aString1, const wchar_t* aString2)
{
	return wcscmp(aString1, aString2) == 0;
}

#endif // IS_PC_BUILD
