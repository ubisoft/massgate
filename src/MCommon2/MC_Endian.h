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

#ifndef _MC_ENDIAN_H_
#define _MC_ENDIAN_H_

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Endian types
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define ENDIAN_LITTLE	(0)
#define ENDIAN_BIG		(1)

#if IS_PC_BUILD
	#define SYSTEM_ENDIAN			(ENDIAN_LITTLE)	// little endian on PC
#else
	#error unknown platform endian
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Macro (compile time) support...
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define IS_ENDIAN(x)		(SYSTEM_ENDIAN==(x))

// other bits removed (allow optimiser to bin unused code)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Namespace (run time) support...
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace MC_Endian
{
	// check endianess
	static bool IsLittle()					{ return SYSTEM_ENDIAN==ENDIAN_LITTLE; };
	static bool IsBig	()					{ return SYSTEM_ENDIAN==ENDIAN_BIG; };

	// swap functions

	static u16	Swap	(const u16 data)	{ return ((data&0xff)<<8)|(data>>8); };
	static u32	Swap	(const u32 data)	{ return ((data&0xff)<<24)|((data&0xff00)<<8)|((data>>8)&0xff00)|((data>>24)); };
	static u64	Swap	(const u64 data)	{ return ((data&0xff)<<56)|((data&0xff00)<<40)|((data&0xff0000)<<24)|((data&0xff000000)<<8)|((data>>8)&0xff000000)|((data>>24)&0xff0000)|((data>>40)&0xff00)|((data>>56)); };
	static u8	Swap	(const u8 data)		{ return data; };
	static s8	Swap	(const s8 data)		{ return data; };
	static s16	Swap	(const s16 data)	{ return (s16)Swap((u16)data); };
	static s32	Swap	(const s32& data)	{ return (s32)Swap((u32)data); };
	static s64	Swap	(const s64& data)	{ return (s64)Swap((u64)data); };
	static f32	Swap	(const f32& data)	{ u32 temp=Swap(*(u32*)&data); return *(f32*)&temp; };
	static f64	Swap	(const f64& data)	{ u64 temp=Swap(*(u64*)&data); return *(f64*)&temp; };

	// array swap functions
	static u8*	Swap	(u8* anArray, const s32 aLength)
	{
		return anArray;
	};
	static u16*	Swap	(u16* anArray, const s32 aLength)
	{
		for (s32 i=0; i<aLength; i++)
			anArray[i]=Swap(anArray[i]);
		return anArray;
	};
	static u32*	Swap	(u32* anArray, const s32 aLength)
	{
		for (s32 i=0; i<aLength; i++)
			anArray[i]=Swap(anArray[i]);
		return anArray;
	};
	static u64*	Swap	(u64* anArray, const s32 aLength)
	{
		for (s32 i=0; i<aLength; i++)
			anArray[i]=Swap(anArray[i]);
		return anArray;
	};
	static s8*	Swap	(s8* anArray, const s32 aLength)
	{
		return anArray;
	};
	static s16*	Swap	(s16* anArray, const s32 aLength)
	{
		Swap((u16*)anArray, aLength);
		return anArray;
	};
	static s32*	Swap	(s32* anArray, const s32 aLength)
	{
		Swap((u32*)anArray, aLength);
		return anArray;
	};
	static s64*	Swap	(s64* anArray, const s32 aLength)
	{
		Swap((u64*)anArray, aLength);
		return anArray;
	};
	static f32*	Swap	(f32* anArray, const s32 aLength)
	{
		Swap((u32*)anArray, aLength);
		return anArray;
	};
	static f64*	Swap	(f64* anArray, const s32 aLength)
	{
		Swap((u64*)anArray, aLength);
		return anArray;
	};

	/// This function is needed, because in big-endian, the values aren't where you would expect because of byte ordering
	/// Therefore constants like 0xFFFF0000 cease to be valid
	inline unsigned int HalfwordFlip32BitConstant(const unsigned int Constant)
	{
		#if SYSTEM_ENDIAN == ENDIAN_BIG
			const unsigned int Halfword0 = Constant & 0xFFFF;
			const unsigned int Halfword1 = (Constant >> 16) & 0xFFFF;
			return (Halfword0 << 16) | Halfword1;
		#else
			return Constant;
		#endif
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif // _MC_ENDIAN_H_

// end of file
