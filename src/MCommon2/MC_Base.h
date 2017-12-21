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
// This file gets included by MC_GlobalDefines.h

#ifndef _MC_BASE_H_
#define _MC_BASE_H_

// define target platform...

// set WIN32 platforms
#ifdef _WIN32
	#define IS_PC_BUILD		(1)
#endif

// make sure all target platforms can be tested
#ifndef IS_PC_BUILD
	#define IS_PC_BUILD		(0)
#endif

#if IS_PC_BUILD
	#ifdef _GAMEPAD
		#define IS_PAD_BUILD 		(1)
		#define IS_MOUSE_BUILD		(0)
		#define IS_CONSOLE_UI_BUILD	(1)
		#define IS_PC_UI_BUILD		(0)
		#define DISABLE_PLAYER_FOG_OF_WAR (1)
	#else
		#define IS_PAD_BUILD		(0)
		#define IS_MOUSE_BUILD		(1)
		#define IS_CONSOLE_UI_BUILD	(0)
		#define IS_PC_UI_BUILD		(1)
		#define DISABLE_PLAYER_FOG_OF_WAR (0)
	#endif
#else
	#define IS_PAD_BUILD		(1)
	#define IS_MOUSE_BUILD		(0)
	#define IS_CONSOLE_UI_BUILD	(1)
	#define IS_PC_UI_BUILD		(0)
	#define DISABLE_PLAYER_FOG_OF_WAR (0)
#endif

// support for modules
#if IS_PC_BUILD
	#define SUPPORT_VOIP
#endif

// support for emulating mouse with control pad
#define EMULATE_MOUSE (1)

// Defines for platform specific keywords
#if IS_PC_BUILD
	#define MC_FORCEINLINE __forceinline
	#define MC_THREADLOCAL __declspec(thread)
	#define MC_RESTRICT __restrict
#else
	#error Need to define MC_FORCEINLINE, MC_THREADLOCAL and MC_RESTRICT for this platform
#endif

// Commonly used functions
template<typename T>
MC_FORCEINLINE void MC_Swap(T& aA, T& aB)
{
	T temp(aA);
	aA = aB;
	aB = temp;
}

template<typename T>
MC_FORCEINLINE T MC_Clamp(const T& aValue, const T& aMin, const T& aMax)
{
	if(aValue < aMin)
		return aMin;
	else if(aMax < aValue)
		return aMax;
	else
		return aValue;
}

template<typename T>
MC_FORCEINLINE T MC_Min(const T& aA, const T& aB)
{
	if(aA < aB)
		return aA;
	else
		return aB;
}

template<typename T>
MC_FORCEINLINE T MC_Min(const T& aA, const T& aB, const T& aC)
{
	T temp;

	if(aA < aB)
		temp = aA;
	else
		temp = aB;

	if(temp < aC)
		return temp;
	else
		return aC;
}

template<typename T>
MC_FORCEINLINE T MC_Min(const T& aA, const T& aB, const T& aC, const T& aD)
{
	T temp1;
	T temp2;
	
	if(aA < aB)
		temp1 = aA;
	else
		temp1 = aB;

	if(aC < aD)
		temp2 = aC;
	else
		temp2 = aD;
	
	if(temp1 < temp2)
		return temp1;
	else
		return temp2;
}

template<typename T>
MC_FORCEINLINE T MC_Max(const T& aA, const T& aB)
{
	if(aA < aB)
		return aB;
	else
		return aA;
}

template<typename T>
MC_FORCEINLINE T MC_Max(const T& aA, const T& aB, const T& aC)
{
	T temp;

	if(aA < aB)
		temp = aB;
	else
		temp = aA;

	if(temp < aC)
		return aC;
	else
		return temp;
}

template<typename T>
MC_FORCEINLINE T MC_Max(const T& aA, const T& aB, const T& aC, const T& aD)
{
	T temp1;
	T temp2;
	
	if(aA < aB)
		temp1 = aB;
	else
		temp1 = aA;

	if(aC < aD)
		temp2 = aD;
	else
		temp2 = aC;
	
	if(temp1 < temp2)
		return temp2;
	else
		return temp1;
}


	#include "xmmintrin.h"
	inline void MC_CachePrefetch(const void *Pointer, const int Offset)
	{
		_mm_prefetch(((char*)Pointer) + Offset, _MM_HINT_T1);
	}

	inline void MC_CacheFlush(const void *Pointer, const int Offset)
	{
	}

	inline void MC_CacheLineZeroInit128(const void *Pointer, const int Offset)
	{
	}
/// SWFM:TH End

// library includes
#include "MC_DataTypes.h"
#include "MC_Endian.h"

#endif // _MC_BASE_H_

