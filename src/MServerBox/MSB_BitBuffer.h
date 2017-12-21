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
#ifndef MSB_BITBUFFER_H_
#define MSB_BITBUFFER_H_

#include "MC_Base.h"
#if IS_PC_BUILD

class MSB_BitBuffer
{
public:
	//Moves bit pointer to an even byte, returning the number of bits moved
	// This can be used to pad to a full byte, in both read and write
	int				AlignByte();

protected:
	uint8*			myBuffer;
	uint16			myBufLen;
	uint16			myWorkByte;
	uint8			myWorkBit;

	//Can't make instances of this class
					MSB_BitBuffer(
						uint8*		aBuffer,
						uint16		aBufLen);
					~MSB_BitBuffer();
};

class MSB_ReadBitBuffer : public MSB_BitBuffer
{
public:
					MSB_ReadBitBuffer(
						uint8*		aBuffer,
						uint16		aBufLen)
						: MSB_BitBuffer(aBuffer, aBufLen) {}
					~MSB_ReadBitBuffer() {}

	// Returns 0 or 1, -1 if the buffer spent and it needs a new one
	inline int	ReadBit()
	{
		if ( myWorkBit == 0 )
		{
			myWorkByte++;
			myWorkBit = 8;

			if ( myWorkByte == myBufLen )
				return -1; //Buffer empty
		}

		return (myBuffer[myWorkByte] >> --myWorkBit) & 1;
	}
};

class MSB_WriteBitBuffer : public MSB_BitBuffer
{
public:
					MSB_WriteBitBuffer(
						uint8*		aBuffer,
						uint16		aBufLen)
						: MSB_BitBuffer(aBuffer, aBufLen) {}
					~MSB_WriteBitBuffer() {}

	inline int	WriteBit(
						uint8		aBit)
	{
		if ( myWorkBit == 0 )
		{
			myWorkByte++;
			if ( myWorkByte == myBufLen )
				return -1; //Buffer full

			myBuffer[myWorkByte] = 0;
			myWorkBit = 8;
		}

		myBuffer[myWorkByte] |= aBit <<= --myWorkBit;

		return 1; //One bit written
	}
	
	uint16			GetWrittenBytes() { return myWorkByte; }
};

#endif // IS_PC_BUILD

#endif // MSB_BITBUFFER_H_


