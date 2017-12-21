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
#ifndef MSB_HUFFCODE_
#define MSB_HUFFCODE_

#include "MC_Base.h"

#if IS_PC_BUILD
#include "MSB_Types.h"

/*
In worst case we need 356 bits to code a character. This happens if all the nodes has exactly one leaf, 
but the last node which has two. This will give us a tree that more or less looks like a list. 
A tree/list that has all 255 nodes (there are 256-1=255 nodes for 256 leafs). So the path from 
the root to the last node is 255+1=256 steps. Meaning 256 bits. 

We there for need a 256 bit type, that can build its self one bit at time.
*/

class MSB_256Bits
{
public:
				MSB_256Bits() { Clear(); }	
				~MSB_256Bits() {}

	// Used in huffman coding when going from root to leaf to get the code
	void		ShiftInFromRight(
					uint8	aValue)
	{
		assert( (aValue & 1) == aValue );

		myData[3] <<= 1;
		myData[3] |= (myData[2] >> 63);

		myData[2] <<= 1;
		myData[2] |= (myData[1] >> 63);

		myData[1] <<= 1;
		myData[1] |= (myData[0] >> 63);

		myData[0] <<= 1;
		myData[0] |= aValue;

		myBits++;
	}

	// Used in huffman code when going from leaf to root to get the code
	void		AddToLeft(
					uint64	aValue)
	{
		assert( (aValue & 1) == aValue );

		if ( myBits < 64 )
			myData[0] |= (aValue << myBits);
		else if ( myBits < 128 )
			myData[1] |= (aValue << (myBits - 64));
		else if ( myBits < 192 )
			myData[2] |= (aValue << (myBits - 128));
		else
			myData[3] |= (aValue << (myBits - 192));

		myBits++;
	}

	//Functions to iterate over the bits
	uint8		GetBit(
					uint8 aPos)
	{
		assert( aPos <= myBits );

		if ( aPos < 64 )
			return (uint8) (myData[0] >> aPos) & 1;
		else if ( aPos < 128 )
			return (uint8) (myData[1] >> (aPos - 64)) & 1;
		else if ( aPos < 192 )
			return (uint8) (myData[2] >> (aPos - 128)) & 1;
		else
			return (uint8) (myData[3] >> (aPos - 192)) & 1;
	}

	uint8		Size() { return myBits; }
	
	void		Clear()
	{
		myData[0] = myData[1] = myData[2] = myData[3] = 0;
		myBits = 0;
	}

	
private:
	uint64		myData[4];
	uint8		myBits; //Number of bits used
};

#endif // IS_PC_BUILD

#endif // MSB_256Bits

