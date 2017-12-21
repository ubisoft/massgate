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
#include "MSB_BitStream.h"

MSB_ReadBitStream::MSB_ReadBitStream( 
	MSB_IStream&	aReadStream )
	: MSB_BitStream( aReadStream )
	, myPeekBytes(0)
{
	FillBuffer();
}

MSB_ReadBitStream::~MSB_ReadBitStream()
{
	FlushBuffer();	
}

bool
MSB_ReadBitStream::FillBuffer()
{
	myStream.ReadSkip(myPeekBytes);

	myPeekBytes = myStream.Peek(&myBitBuf, sizeof(myBitBuf)); //Only peek, since we might not use it all
	myNumBits = myPeekBytes * 8;
	if ( myNumBits == 0 )
		return false; //No more data to read
	myBitBuf = MSB_SWAP_TO_NATIVE(myBitBuf); //The buffer is in big/network endian
	myBitBuf >>= sizeof(myBitBuf) * 8 - myNumBits; //Shift down and only use those bytes we read

	return true;
}

int
MSB_ReadBitStream::FlushBuffer()
{
	int		bitsUsed = myPeekBytes * 8 - myNumBits;
	int		bytesUsed = (bitsUsed + 7) / 8;
	myStream.ReadSkip(bytesUsed);
	myPeekBytes = 0;
	myNumBits = 0;

	return bytesUsed * 8 - bitsUsed; //Returns the number of bits never read due to 8bit alignment
}

int 
MSB_ReadBitStream::ReadBits( 
	uint64&			aBitsBuf, //Must be = 0
	unsigned		aNumBits )
{
	if ( aNumBits <= myNumBits )
	{	//We have enough bits in buffer
		myNumBits -= aNumBits;
		uint64		mask = ~0; //All ones
		mask = ~(mask << aNumBits); //Shift in zeros and invert
		aBitsBuf |= (myBitBuf >> myNumBits) & mask;
		return 0;
	}
	else
	{
		aNumBits -= myNumBits; //No need to set myNumBits = 0. FillBuffer() does that for us
		uint64		mask = ~0; //All ones
		mask = ~(mask << myNumBits); //shift in zeros and invert
		aBitsBuf |= (myBitBuf & mask) << aNumBits; //Copy those bits we have into the right spot
		if ( FillBuffer() == false )
			return -1;
		return ReadBits(aBitsBuf, aNumBits); //There should now be enough bits in buffer for the rest
	}
}	

// MSB_WriteBitStream
/////////////////////////

MSB_WriteBitStream::MSB_WriteBitStream( 
	MSB_IStream&	aWriteStream )
	: MSB_BitStream( aWriteStream )
{
}

MSB_WriteBitStream::~MSB_WriteBitStream()
{
	FlushBuffer();
}


int
MSB_WriteBitStream::WriteBits( 
	uint64			aBitsBuf, //The bits not used must be zero
	unsigned		aNumBits)
{
	//How many bits can fit before we need to flush the buffer?
	int				firstNumBits = __min( aNumBits, sizeof(myBitBuf) * 8 - myNumBits );
	int				bitsOverFlow = aNumBits - firstNumBits;
	myBitBuf <<= firstNumBits;
	myBitBuf |= (aBitsBuf >> bitsOverFlow);
	myNumBits += firstNumBits;

	if ( myNumBits < sizeof(myBitBuf) * 8 )
		return 0; //All bits fitted on first attempt
	
	if ( FlushFullBuffer() == false )
			return -1;
		
	//Fill up the rest
	myBitBuf = aBitsBuf;
	myNumBits = bitsOverFlow;
	
	return 0;
}

int 
MSB_WriteBitStream::FlushBuffer()
{
	int pad = 0;
	if ( myNumBits % 8 != 0 )
	{
		pad = 8 - (myNumBits % 8);
		WriteBits(0, pad);
	}
	
	while( myNumBits > 0)
	{
		myNumBits -= 8;
		uint8 byte = (uint8) (myBitBuf >> myNumBits);
		if ( myStream.Write(&byte, 1) != 1 )
			return -1;
		
	}
	return pad;
}

bool 
MSB_WriteBitStream::FlushFullBuffer()
{
	myBitBuf = MSB_SWAP_TO_BIG_ENDIAN(myBitBuf);
	if ( myStream.Write(&myBitBuf, sizeof(myBitBuf)) != sizeof(myBitBuf) )
		return false;
	myBitBuf = myNumBits = 0;
	return true;
}

#endif // IS_PC_BUILD
