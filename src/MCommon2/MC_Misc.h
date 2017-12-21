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
#ifndef MC_MISC_H
#define MC_MISC_H

#include "mc_string.h"
#include "mc_vector.h"

class MC_Matrix33;

namespace MC_Misc
{
	// Returns the number of bits set in a bit field
	unsigned int CountBits(const unsigned int aBitField);

	// Returns true if the value is a non-zero and a power of 2.
	bool IsPowerOf2(const unsigned int aValue);

	// Get the lowest power of two that is higher than or equal to x
	unsigned int GetLowestHigherOrEqualPowerOfTwo(unsigned int x);

	void ToggleFpExceptions(bool anEnableFlag);

	void MemSwap(void* aBufA, void* aBufB, unsigned int aByteCount);

	// Gets the full path of the directory which contains the executable (including the last backslash)
	MC_String GetExecutableFolder();

	MC_String Format(const char*, ...);

	// Works for any vector class that defines x, y, z, ... members
	template <class T> MC_String ToStringV2(const T& aV) { return Format("(%.2f, %.2f)", aV.x, aV.y); }
	template <class T> MC_String ToStringV3(const T& aV) { return Format("(%.2f, %.2f, %.2f)", aV.x, aV.y, aV.z); }
	template <class T> MC_String ToStringV4(const T& aV) { return Format("(%.2f, %.2f, %.2f, %.2f)", aV.x, aV.y, aV.z, aV.w); }

	unsigned int MakeHash32(const void* aData, unsigned int aDataSize, unsigned int aSeed = 0);
	unsigned _int64 MakeHash64(const void* aData, unsigned int aDataSize, _int64 aSeed = 0);

	void  SRand(int aSeed);		// Set seed for the random functions below
	float Rand01();				// Returns 0..1
	float Rand12();				// Returns 1..2	(Random between 1 and 2 is the very fastest)
	float RandMinus1To1();		// Returns -1..1
	MC_Vector2f RandVec2();		// Returns Normalized(-1..1, -1..1)
	MC_Vector3f RandVec3();		// Returns Normalized(-1..1, -1..1, -1..1)
	MC_Vector2f RandVec2_01();	// Returns (0..1, 0..1)
	MC_Vector3f RandVec3_01();	// Returns (0..1, 0..1, 0..1)

	bool CompareFilenames(const char* aFilename1, const char* aFilename2);
	bool SanityCheckFloat(float aFloat);
	bool SanityCheck(const MC_Vector3f& aVec3);
	bool SanityCheck(const MC_Matrix33& aMatrix);
	bool SanityCheckAndZeroDenormal(float &aFloat);
	bool SanityCheckAndZeroDenormals(MC_Vector3f& aVec3);
	bool SanityCheckAndZeroDenormals(MC_Matrix33& aMatrix);
	bool DropAdministratorRights();
	bool IsSingleCore();
};

template <typename T> class MC_StoreAndReset
{
public:
	T myValue;
	T* myPtr;
	explicit MC_StoreAndReset(T* aValue) { myValue = *aValue; myPtr = aValue; }
	MC_StoreAndReset(T* aValue, const T &aNewValue) { myValue = *aValue; myPtr = aValue; *myPtr = aNewValue; }
	~MC_StoreAndReset() { *myPtr = myValue; }
};

typedef MC_StoreAndReset<unsigned int> MC_StoreAndResetDWORD;
typedef MC_StoreAndReset<bool> MC_StoreAndResetBOOL;

#endif

