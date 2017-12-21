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
#ifndef MC_F2I_H
#define MC_F2I_H


/*__forceinline int TruncToInt(double aDouble) // only works for double precision, and D3D always puts FPU in single precision mode
{
	static const double double2fixmagic = 68719476736.0 * 1.5;

	aDouble += double2fixmagic;
	return ((int*) &aDouble)[1] >> 16;
}*/


__forceinline int TruncToInt(const float aFloat)
{
/*#ifdef _MSC_VER
	int i;
	aFloat -= 0.5f;
	__asm
	{
		fld aFloat
		fistp i
	}
	return i;
#else // not _MSC_VER*/
	return (int) aFloat;
//#endif // not _MSC_VER
}


__forceinline static void TruncToInt(int* anInt, const float aFloat)
{
/*#ifdef _MSC_VER
	aFloat -= 0.5f;
	__asm
	{
		fld aFloat
		mov eax, anInt
		fistp dword ptr [eax]
	}
#else // not _MSC_VER*/
	*anInt = (int) aFloat;
//#endif // not _MSC_VER
}


__forceinline static void TruncToUnsignedInt(unsigned int* anInt, const float aFloat)
{
/*#ifdef _MSC_VER
	aFloat -= 0.5f;
	__asm
	{
		fld aFloat
		mov eax, anInt
		fistp dword ptr [eax]
	}
#else // not _MSC_VER*/
	*anInt = (int) aFloat;
//#endif // not _MSC_VER
}


__forceinline static void RoundToInt(int* anInt, const float aFloat)
{
/*#ifdef _MSC_VER
	__asm
	{
		fld aFloat
		mov eax, anInt
		fistp dword ptr [eax]
	}
	assert(*anInt == (int) (aFloat + 0.5f));
#else // not _MSC_VER*/
	*anInt = (int) (aFloat + 0.5f);
//#endif // not _MSC_VER
}


__forceinline static void RoundToUnsignedInt(unsigned int* anInt, const float aFloat)
{
/*#ifdef _MSC_VER
	__asm
	{
		fld aFloat
		mov eax, anInt
		fistp dword ptr [eax]
	}
	assert(*anInt == (unsigned int) (aFloat + 0.5f));
#else // not _MSC_VER*/
	*anInt = (unsigned int) (aFloat + 0.5f);
//#endif // not _MSC_VER
}


__forceinline static int RoundToInt(const float aFloat)
{
/*#ifdef _MSC_VER
	int i;
	__asm
	{
		fld aFloat
		fistp i
	}
	assert(i == (int) (aFloat + 0.5f));
	return i;
#else // not _MSC_VER*/
	return (int) (aFloat + 0.5f);
//#endif // not _MSC_VER
}


__forceinline static unsigned int RoundToUnsignedInt(const float aFloat)
{
/*#ifdef _MSC_VER
	int i;
	__asm
	{
		fld aFloat
		fistp i
	}
	assert(i == (unsigned int) (aFloat + 0.5f));
	return i;
#else // not _MSC_VER*/
	return (unsigned int) (aFloat + 0.5f);
//#endif // not _MSC_VER
}


#endif // MC_F2I_H
