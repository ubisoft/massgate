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
#include "mc_bitvector.h"
#include "mc_mem.h"

void TestBitVectors();

void MC_BitVector::Init(unsigned int aNumIndices)
{
	TestBitVectors();

	assert(aNumIndices);
	unsigned int newDataLength = (aNumIndices + 7) / 8;

	if ((myData == 0) || (myData && newDataLength > myDataLength))
	{
		if (myData)
			MC_TempFree(myData);
		myDataLength = newDataLength;
		myData = (unsigned char*)MC_TempAllocIfOwnerOnStack(this, newDataLength, __FILE__, __LINE__);
	}

	myNumIndices = aNumIndices;
	Clear();
}

void MC_BitVector::Resize(unsigned int aNumIndices)
{
	assert(aNumIndices);
	unsigned int newDataLength = (aNumIndices + 7) / 8;

	if ((myData == 0) || (myData && newDataLength > myDataLength))
	{
		unsigned char* prevData = myData;
		unsigned int prevDataLength = myDataLength;
		myDataLength = newDataLength;
		myData = (unsigned char*)MC_TempAllocIfOwnerOnStack(this, newDataLength, __FILE__, __LINE__);
		myNumIndices = aNumIndices;
		Clear();

		if (prevData)
		{
			memcpy(myData, prevData, prevDataLength);
			MC_TempFree(prevData);
		}
	}
	else
	{	
		myNumIndices = aNumIndices;
		Clear();
	}
}

MC_BitVector::~MC_BitVector()
{
	if(myData)
	{
		MC_TempFree(myData);
		myData = NULL;
	}
}


void MC_BitVector::operator = (const MC_BitVector& aBitVector)
{
	if(this == &aBitVector)
		return;

	assert(aBitVector.myNumIndices);

	//only do reallocation if sizes are different
	if(myNumIndices != aBitVector.myNumIndices)
	{
		myNumIndices = aBitVector.myNumIndices;
		myDataLength = aBitVector.myDataLength;

		MC_TempFree(myData);
		myData = (unsigned char*)MC_TempAllocIfOwnerOnStack(this, myDataLength, __FILE__, __LINE__);
		assert(myData);
	}

	//copy data
	memcpy(myData, aBitVector.myData, myDataLength);
}


void MC_BitVector::Clear()
{
	memset( myData, 0, myDataLength );  // myDataLength is bytes
}

void MC_BitVector::SetAll()
{
	memset( myData, 255, myDataLength );  // myDataLength is bytes
}

// unit tests for bitvectors, run once at first use
void TestBitVectors()
{
	static bool called = false;
	if (called)
		return;
	called = true;

	for (int i = 0; i < 5; ++i)
	{
		MC_BitVector bv(256*256);
		bool bv2[256*256];
		memset(bv2, 0, sizeof(bv2));

		for (int y = 0; y < 256; ++y)
		{
			for (int x = 0; x < 256; ++x)
			{
				bool randval = (rand() % 2) != 0;
				bv[x + y*256] = randval;
				bv2[x + y*256] = randval;
			}
		}
		for (int y = 0; y < 256; ++y)
		{
			for (int x = 0; x < 256; ++x)
			{
				assert (bv[x + y*256] == bv2[x + y*256]);
			}
		}
	}
}