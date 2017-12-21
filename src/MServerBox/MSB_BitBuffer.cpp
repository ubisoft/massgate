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
#include "StdAfx.h"

#if IS_PC_BUILD
#include "MSB_BitBuffer.h"

/*
static const uint8 locMask[] = 
{
	0xff, //0 bits (wont happen really)
	0x01, //1 bit
	0x03, //2 bits
	0x07, //3
	0x0f, //4
	0x1f, //5
	0x3f, //6
	0x7f, //7
	0xff, //8 bits, a full byte
};
*/

MSB_BitBuffer::MSB_BitBuffer(
	uint8*		aBuffer,
	uint16		aBufLen)
	: myBuffer(aBuffer)
	, myBufLen(aBufLen)
	, myWorkByte(0)
	, myWorkBit(8) //The most significant bit of a byte, plus 1
{
}

MSB_BitBuffer::~MSB_BitBuffer()
{
}


/*
int 
MSB_BitBuffer::WriteLowBits( 
	uint64		aBits, 
	uint8		aNumBits )
{
	int bitsWritten = 0;
	for (uint8 bit = aNumBits - 1; bit >= 0; bit--)
	{	
		if ( WriteBit( (aBits >> bit)&1 ) == -1 )
			return bitsWritten;
		bitsWritten++;
	}

	return 0;
}
*/

int 
MSB_BitBuffer::AlignByte()
{
	if ( myWorkBit != 8 )
	{
		int padding = myWorkBit;
		myWorkByte++;
		myWorkBit = 0;
		return myWorkBit;
	}
	else
	{
		return 0;
	}
}

#endif // IS_PC_BUILD
