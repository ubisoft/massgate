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
// Standard cross platform data types and support functions
// This file gets included by MC_Base.h (which in turn gets included by MC_GlobalDefines.h)

#ifndef _MC_DATA_TYPES_H_
#define _MC_DATA_TYPES_H_

typedef unsigned __int64	u64;
typedef signed __int64		s64;
typedef unsigned int		u32;
typedef signed int			s32;
typedef unsigned short		u16;
typedef signed short		s16;
typedef unsigned char		u8;
typedef signed char			s8;
typedef float				f32;
typedef double				f64;

// alternate long data types
typedef u64					uint64;
typedef s64					int64;
typedef u32					uint32;
typedef s32					int32;
typedef u16					uint16;
typedef s16					int16;
typedef u8					uint8;
typedef s8					int8;
typedef f32					float32;
typedef f64					float64;

// F32 defines
#define F32_PI				(3.141592653589793f)
#define F32_HALF_PI			(F32_PI*0.5f)
#define F32_TWO_PI			(F32_PI*2.0f)
#define F32_DEGTORAD(ang)	((f32)(ang)*(F32_TWO_PI*(1.0f/360.0f)))
#define	F32_RADTODEG(ang)	((f32)(ang)*(360.0f/F32_TWO_PI))

#define F64_PI				(3.141592653589793)
#define F64_HALF_PI			(F64_PI*0.5)
#define F64_TWO_PI			(F64_PI*2.0)
#define F64_DEGTORAD(ang)	((f64)(ang)*(F64_TWO_PI*(1.0/360.0)))
#define	F64_RADTODEG(ang)	((f64)(ang)*(360.0/F64_TWO_PI))

enum McNoInitType
{
	MC_NO_INIT
};

#endif // _MC_DATA_TYPES_H_

