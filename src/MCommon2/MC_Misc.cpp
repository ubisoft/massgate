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
#include "MC_Misc.h"

#include "ct_assert.h"
#include "MC_SortedGrowingArray.h"
#include "MF_File.h"
#include <float.h>

#include "MC_Vector.h"
#include "MC_CommandLine.h"

#if IS_PC_BUILD
	#include <xmmintrin.h>
	// #include <WinSafer.h> // Needed for DropAdministratorRights
#endif

// Ref: AMD Software optimization guide.pdf
//  - "Efficient Implementation of Population-Count Function in 32-Bit Mode", page 179
// Returns the number of bits set in a bit field
unsigned int MC_Misc::CountBits(const unsigned int v)
{
	const unsigned int w = v - ((v >> 1) & 0x55555555);
	const unsigned int x = (w & 0x33333333) + ((w >> 2) & 0x33333333);
	const unsigned int y = (x + (x >> 4) & 0x0F0F0F0F);
	return (y * 0x01010101) >> 24;
}

bool MC_Misc::IsPowerOf2(const unsigned int aValue)
{
	return aValue && (aValue & (aValue - 1)) == 0;
}

unsigned int 
MC_Misc::GetLowestHigherOrEqualPowerOfTwo(unsigned int x)
{
	ct_assert<sizeof(int) == 4>();
	x--;
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return(x+1);
}


void MC_Misc::ToggleFpExceptions(bool anEnableFlag)
{
	if (anEnableFlag)
	{
		#ifdef MC_ENABLE_FLOAT_EXCEPTIONS
		_clearfp();
		_controlfp(~(_EM_OVERFLOW | _EM_ZERODIVIDE | _EM_INVALID), MCW_EM);
		#endif // MC_ENABLE_FLOAT_EXCEPTIONS
	}
	else
	{
		#ifdef MC_ENABLE_FLOAT_EXCEPTIONS
		_controlfp(~0, MCW_EM);		// Disable all fp
		#endif // MC_ENABLE_FLOAT_EXCEPTIONS
	}
}

void MC_Misc::MemSwap(void* aBufA, void* aBufB, unsigned int aByteCount)
{
	char* a = (char*)aBufA;
	char* b = (char*)aBufB;

	for(unsigned int i=0; i<aByteCount; i++)
	{
		char t = a[i];
		a[i] = b[i];
		b[i] = t;
	}
}

MC_String MC_Misc::GetExecutableFolder()
{
#if IS_PC_BUILD
	MC_String folder = _pgmptr;
	while(folder.GetLength() > 0)
	{
		const char c = folder[folder.GetLength()-1];

		if(c == '\\' || c == '/' || c == ':')
			break;
	 
		folder.SetLength(folder.GetLength() - 1);
	}
#else
	MC_String folder = "d:\\";
#endif

	return folder;
}

MC_String MC_Misc::Format(const char* aFormatString, ...)
{
	va_list paramList;
	va_start(paramList, aFormatString);

	int b = _vscprintf(aFormatString, paramList);
	MC_String str;
	str.Reserve(b);
	int stringLen =_vstprintf((TCHAR*)str.GetBuffer(), aFormatString, paramList);
	assert(b == stringLen);

	va_end(paramList);

	return str;
}

static inline int SingleHash( int seed, char input )
{
	return (seed^(unsigned int)input) * 214013 + 2531011;
}

static inline void HashToAnySize(const void* aData, int aDataSize, int aHashSizeInInts, int* aHash)
{
	const char* p = (const char*)aData;

	for(int iHash=0; aDataSize!=0; p++, aDataSize--, iHash=(iHash+1)%aHashSizeInInts)
		aHash[iHash] = SingleHash(aHash[iHash], *p);
}

unsigned int MC_Misc::MakeHash32(const void* aData, unsigned int aDataSize, unsigned int aSeed)
{
	const char* p = (const char*)aData;
	int hash = aSeed;
	for(unsigned int i=0; i<aDataSize; i++)
		hash = SingleHash(hash, p[i]);
	return hash;
}

unsigned _int64 MC_Misc::MakeHash64(const void* aData, unsigned int aDataSize, _int64 aSeed)
{
	_int64 hash = aSeed;
	HashToAnySize(aData, aDataSize, 2, (int*)&hash);
	return hash;
}

static int locHoldRand = 0;
__forceinline static int MiscRand()
{
	// Duplicated implementation of the standard rand() function (to get it inlined, and 
	// to be sure that the result is always in the range 0 .. 0x7fff even if the standard
	// implementation should change).
	return(((locHoldRand = locHoldRand * 214013 + 2531011) >> 16) & 0x7fff);
}

void MC_Misc::SRand(int aSeed)
{
	locHoldRand = aSeed;
}

float MC_Misc::Rand01()
{
	const unsigned int rnd = 0x3F800000 | (MiscRand() << 8);				// 1.0 .. 1.9999 (with 15 random bits)
	return *(float*)&rnd - 1.0f;
}

float MC_Misc::Rand12()
{
	const unsigned int rnd = 0x3F800000 | (MiscRand() << 8);				// 1.0 .. 1.9999 (with 15 random bits)
	return *(float*)&rnd;
}

float MC_Misc::RandMinus1To1()
{
	const unsigned int rnd = 0x40000000 | (MiscRand() << 8);				// 2.0 .. 3.9999 (with 15 random bits)
	return *(float*)&rnd - 3.0f;
}

// Returns random unit vector
MC_Vector2f MC_Misc::RandVec2()
{
	const unsigned int rnd = MiscRand();
	MC_Vector2f res;
	*(int*)&res.x = 0x40000000 | ((rnd << 8) & 0x007f0000);		// 2.0 .. 3.9999 (with 7 random bits)
	*(int*)&res.y = 0x40000000 | ((rnd << 16) & 0x007f0000);	// 2.0 .. 3.9999 (with 7 random bits)
	res.x -= 3.0f;
	res.y -= 3.0f;
	const float length = res.x * res.x + res.y * res.y;
	if(length != 0.0f)
		res /= sqrtf(length);
	else
		res.x = 1.0f;
	return res;
}

// Returns random unit vector
MC_Vector3f MC_Misc::RandVec3()
{
	const unsigned int rnd = MiscRand();
	MC_Vector3f res;
	*(int*)&res.x = 0x40000000 | ((rnd << 8) & 0x007C0000);		// 2.0 .. 3.9999 (with 5 random bits)
	*(int*)&res.y = 0x40000000 | ((rnd << 13) & 0x007C0000);	// 2.0 .. 3.9999 (with 5 random bits)
	*(int*)&res.z = 0x40000000 | ((rnd << 18) & 0x007C0000);	// 2.0 .. 3.9999 (with 5 random bits)
	res.x -= 3.0f;
	res.y -= 3.0f;
	res.z -= 3.0f;
	const float length = res.x * res.x + res.y * res.y + res.z * res.z;
	if(length != 0.0f)
		res /= sqrtf(length);
	else
		res.x = 1.0f;
	return res;
}

MC_Vector2f MC_Misc::RandVec2_01()
{
	const unsigned int rnd = MiscRand();
	MC_Vector2f res;
	*(int*)&res.x = 0x3F800000 | ((rnd << 8) & 0x007f0000);		// 1.0 .. 2.9999 (with 7 random bits)
	*(int*)&res.y = 0x3F800000 | ((rnd << 16) & 0x007f0000);	// 1.0 .. 2.9999 (with 7 random bits)
	res.x -= 1.0f;
	res.y -= 1.0f;
	return res;
}

MC_Vector3f MC_Misc::RandVec3_01()
{
	const unsigned int rnd = MiscRand();
	MC_Vector3f res;
	*(int*)&res.x = 0x3F800000 | ((rnd << 8) & 0x007C0000);		// 1.0 .. 2.9999 (with 5 random bits)
	*(int*)&res.y = 0x3F800000 | ((rnd << 13) & 0x007C0000);	// 1.0 .. 2.9999 (with 5 random bits)
	*(int*)&res.z = 0x3F800000 | ((rnd << 18) & 0x007C0000);	// 1.0 .. 2.9999 (with 5 random bits)
	res.x -= 1.0f;
	res.y -= 1.0f;
	res.z -= 1.0f;
	return res;
}

bool MC_Misc::CompareFilenames(const char* aFilename1, const char* aFilename2)
{
	int i;

	for(i=0; aFilename1[i] && aFilename2[i]; i++)
	{
		const char c = aFilename1[i];
		const char d = aFilename2[i];

		if(c == d)
			continue;

		if((c == '/' || c == '\\') && (d == '/' || d == '\\'))
			continue;

		if(toupper(c) == toupper(d))
			continue;

		return false;
	}

	return (aFilename1[i] == aFilename2[i]);
}

bool MC_Misc::SanityCheckFloat(float aFloat)
{
	unsigned int i = *(unsigned int*)&aFloat;

	i &= 0x7fffffff;		// Take away sign bit

	if(i == 0)				// OK, Zero float
		return true;

	if(i < 0x00800000)		// Bad, Denormal (less than 1.17e-38)
		return false;

	if(i == 0x7f800000)		// Bad, INF
		return false;

	if(i > 0x7f800000)		// Bad, NaN
		return false;

	if(i > 0x7f000000)		// Bad, Huge (larger than 1.70e+38)
		return false;

	return true;
}

bool MC_Misc::SanityCheck(const MC_Vector3f& aVec3)
{
	return SanityCheckFloat(aVec3.x) && SanityCheckFloat(aVec3.y) && SanityCheckFloat(aVec3.z);
}

bool MC_Misc::SanityCheckAndZeroDenormal(float &aFloat)
{
	unsigned int i = *(unsigned int*)&aFloat;

	i &= 0x7fffffff;		// Take away sign bit

	if(i == 0)				// OK, Zero float
		return true;

	if(i < 0x00800000)		// OKish, Denormal (less than 1.17e-38) - set to zero
	{
		aFloat = 0.f;
		return true;
	}

	if(i == 0x7f800000)		// Bad, INF
		return false;

	if(i > 0x7f800000)		// Bad, NaN
		return false;

	if(i > 0x7f000000)		// Bad, Huge (larger than 1.70e+38)
		return false;

	return true;
}

bool MC_Misc::SanityCheckAndZeroDenormals(MC_Vector3f& aVec3)
{
	return SanityCheckAndZeroDenormal(aVec3.x) && SanityCheckAndZeroDenormal(aVec3.y) && SanityCheckAndZeroDenormal(aVec3.z);
}

/* - Adding this makes us win2k incompatible. Get function by name from DLL?
bool MC_Misc::DropAdministratorRights()
{
	bool res = false;

	SAFER_LEVEL_HANDLE hAuthzLevel = NULL;
	if(SaferCreateLevel(SAFER_SCOPEID_USER, SAFER_LEVELID_NORMALUSER, 0, &hAuthzLevel, NULL))
	{
		HANDLE hToken = NULL;
		if(SaferComputeTokenFromLevel(hAuthzLevel, NULL, &hToken, 0, NULL)) 
		{
			if(ImpersonateLoggedOnUser(hToken))
				res = true;
		}

		SaferCloseLevel(hAuthzLevel);
	}

	return res;
}
*/

bool MC_Misc::IsSingleCore()
{
#if IS_PC_BUILD

	static bool firstRun = true;
	static bool isSingle = false;

	if(firstRun)
	{
		firstRun = false;

		if(MC_CommandLine::GetInstance() && MC_CommandLine::GetInstance()->IsPresent("singlecore"))
		{
			isSingle = true;
		}
		else
		{
			DWORD procAff, sysAff;
			if(GetProcessAffinityMask(GetCurrentProcess(), &procAff, &sysAff))
			if(procAff == 1)
				isSingle = true;
		}
	}

	return isSingle;

#endif

    return false;
}

