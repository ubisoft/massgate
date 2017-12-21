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
#include "MC_HashMap.h"

//###########################################################################
inline int MC_HashMap_SingleHash( unsigned int aSeed, char aChar )
{
	return (aSeed^(unsigned int)aChar) * 214013 + 2531011;
}

//###########################################################################
void MC_HashMap_HashData( const void* aData, int aByteCount, int aHashSize, unsigned int* aDest, unsigned int* aSeed )
{
	const char* p = (const char*)aData;

	if( aSeed )
		for( int i=0; i<aHashSize; i++ )
			aDest[i] = aSeed[i];
	else
		for( int i=0; i<aHashSize; i++ )
			aDest[i] = 0;

	for( int iHash=0 ; aByteCount!=0; p++, aByteCount--, iHash=(iHash+1)%aHashSize )
		aDest[iHash] = MC_HashMap_SingleHash( aDest[iHash], *p );
}
/*
//###########################################################################
unsigned int MC_HashMap_HashData( const void* aData, int aByteCount, unsigned int aSeed )
{
	const char* p = (const char*)aData;
	int hash = aSeed;
	for( ; aByteCount!=0; p++, aByteCount-- )
		hash = MC_HashMap_SingleHash( hash, *p );
	return hash;
}

//###########################################################################
unsigned int MC_HashMap_HashString( const void* aData, unsigned int aSeed )
{
	const char* p = (const char*)aData;
	int hash = aSeed;
	for( ; *p; p++ )
		hash = MC_HashMap_SingleHash( hash, *p );
	return hash;
}*/

// the new order!
typedef unsigned int uint;
typedef unsigned short ushort;
#define GET16BITS(d) (*((const ushort *) (d)))

// fast hash function, from 
// http://www.azillionmonkeys.com/qed/hash.html
static uint SuperFastHash (const char * data, int len, uint hash);

unsigned int MC_HashMap_HashData( const void* aData, int aByteCount, unsigned int aSeed )
{
	return SuperFastHash((const char*)aData, aByteCount, aSeed);
}

unsigned int MC_HashMap_HashString( const void* aData, unsigned int aSeed )
{
	return SuperFastHash((const char*)aData, strlen((const char*)aData), aSeed);
}

static uint SuperFastHash (const char * data, int len, uint hash)
{
	if (len <= 0 || data == 0)
		return 0;

	int rem = len & 3;

	len >>= 2;

	// Main loop
	for (;len > 0; len--)
	{
		hash  += GET16BITS (data);
		const uint tmp    = (GET16BITS (data+2) << 11) ^ hash;
		hash   = (hash << 16) ^ tmp;
		data  += 2*sizeof (ushort);
		hash  += hash >> 11;
	}

	// Handle end cases
	switch (rem)
	{
	case 3:
		hash += GET16BITS (data);
		hash ^= hash << 16;
		hash ^= data[sizeof (ushort)] << 18;
		hash += hash >> 11;
		break;
	case 2:
		hash += GET16BITS (data);
		hash ^= hash << 11;
		hash += hash >> 17;
		break;
	case 1:
		hash += *data;
		hash ^= hash << 10;
		hash += hash >> 1;
	}

	// Force "avalanching" of final 127 bits
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;

	return hash;
}
