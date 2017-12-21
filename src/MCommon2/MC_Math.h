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
#ifndef INCGUARD_MC_MATH_H
#define INCGUARD_MC_MATH_H

#include "mc_globaldefines.h"
#include <math.h>
#include <xmmintrin.h>
#pragma intrinsic( cos, sin, sqrt )

// Fast with decent accuracy.
MC_FORCEINLINE float MC_InvSqrtFastSafe(float x)
{
#if IS_PC_BUILD

	// Error around 0.0004%
	float xhalf = 0.5f * x;
	int i = *(int*)&x;
	i = 0x5f3759df - (i>>1);
	x = *(float*)&i;
	x = x*(1.5f - xhalf*x*x);
	x = x*(1.5f - xhalf*x*x);	// Extra Newton-Raphson step to improve precision
	return x;

#else

	// Error around 0.00001%
	// We add the tiny value to avoid exceptions on zero input (this does not affect precision measurably).
	return 1.0f / (1e-15f + sqrtf(x));

#endif
}

// Very fast, but very inaccurate.
MC_FORCEINLINE float MC_InvSqrtFastSafe_Estimate(float x)
{
#if IS_PC_BUILD

	// Error around 0.15%
	float xhalf = 0.5f * x;
	int i = *(int*)&x;
	i = 0x5f3759df - (i>>1);
	x = *(float*)&i;
	x = x*(1.5f - xhalf*x*x);
	return x;

#else

	// Error around 0.00001%
	// We add the tiny value to avoid exceptions on zero input (this does not affect precision measurably).
	return 1.0f / (1e-15f + sqrtf(x));

#endif
}

MC_FORCEINLINE void __fastcall MC_SinCos(float angle, float* s, float* c)
{
	__asm
	{
		mov      ecx, c
		mov      edx, s

		fld      dword ptr [angle]
		fsincos
		fstp     dword ptr [ecx]
		fstp     dword ptr [edx]
	}
	*s = (float)sin(angle);
	*c = (float)cos(angle);
}

MC_FORCEINLINE void __fastcall MC_SinCosVec(float angleA, float angleB, float angleC, float* sinvals, float* cosvals)
{
	__asm
	{
		mov      ecx, cosvals
		mov      edx, sinvals

		fld      dword ptr [angleA]
		fsincos
		fstp     dword ptr [ecx]
		fstp     dword ptr [edx]

		fld      dword ptr [angleB]
		fsincos
		fstp     dword ptr [ecx+4]
		fstp     dword ptr [edx+4]

		fld      dword ptr [angleC]
		fsincos
		fstp     dword ptr [ecx+8]
		fstp     dword ptr [edx+8]
	}
}

#endif//INCGUARD_MC_MATH_H

