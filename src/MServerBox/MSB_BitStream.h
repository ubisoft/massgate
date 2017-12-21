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
#ifndef MSB_BITSTREAM_
#define MSB_BITSTREAM_

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_IStream.h"
#include "MSB_Types.h"

/** Base class for ReadBitStream and WriteBitStream. Never used outside of this file.
 *
 */
class MSB_BitStream
{
public:
	uint8			GetNumBits() { return myNumBits; }

protected:
	MSB_IStream&	myStream;
	uint64			myBitBuf;
	uint8			myNumBits;
	
					MSB_BitStream(
						MSB_IStream&		aStream)
						: myStream(aStream)
						, myBitBuf(0)
						, myNumBits(0) {}
};

class MSB_ReadBitStream : public MSB_BitStream
{
public:
					MSB_ReadBitStream(
						MSB_IStream&		aReadStream);
					~MSB_ReadBitStream();
						

	FORCEINLINE int	ReadBit()
	{
		if ( myNumBits == 0 && FillBuffer() == false )
			return -1;
		
		return (int) (myBitBuf >> --myNumBits) & 1;
	}

	int				ReadBits(
						uint64&			aBitsBuf,
						unsigned		aNumBits);

	//Commits the peek of data in aReadStream.
	//Returns the number of bits never read on the current byte
	int				FlushBuffer(); 

private:
	bool			FillBuffer();

	uint8			myPeekBytes; //Number of bytes we have peeked in the IStream
};

class MSB_WriteBitStream : public MSB_BitStream
{
public:
					MSB_WriteBitStream(
						MSB_IStream&		aWriteStream);
					~MSB_WriteBitStream();

	FORCEINLINE int	WriteBit(
						int	aBit)
	{
		if ( myNumBits == sizeof(myBitBuf) * 8 && FlushFullBuffer() == false )
			return -1;
		
		myBitBuf <<= 1;
		myBitBuf |= aBit;
		myNumBits++;

		return 0;
	}
	
	int				WriteBits( 
						uint64			aBitsBuf,
						unsigned		aNumBits);

					//Writes cached data to aWriteStream
					//Returns the number of bits needed to be paded
	int				FlushBuffer();
private:
	bool			FlushFullBuffer();
};

#endif // IS_PC_BUILD

#endif // MSB_BITSTREAM_
