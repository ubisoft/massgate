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

// #include "MC_Logger.h"
#include "MSB_Huffman.h"

// 
// MSB_Huffman
//

MSB_Huffman::MSB_Huffman()
	: myDecodePtr(HUFF_ROOT) //Start up first decode
{
}

MSB_Huffman::~MSB_Huffman()
{
}


/*
int
MSB_Huffman::EncodeStream( 
	MSB_IStream&		aReadStream, 
	int					aNumSymbols, 
	MSB_WriteBitStream& aWriteBitStream,
	MSB_HuffmanBuilder*	aSymbolCounter)
{
	int					res = 0;
	uint8				buf[16];

	do
	{
		//Fill our buff
		int readLen = __min(aNumSymbols, sizeof(buf));
		if ( aReadStream.Read(buf, readLen) != readLen )
			return -1; //Error, or short read

		if ( aSymbolCounter != NULL )
			aSymbolCounter->CountSymbols( buf, readLen );

		for ( int i = 0; i < readLen; i++ )
		{
			if ( EncodeSymbol( buf[i], aWriteBitStream ) )
				res++;
			else
				return -2;
		}

		aNumSymbols -= readLen;

	} while (aNumSymbols > 0);

	return res;
}

int 
MSB_Huffman::DecodeStream( 
	MSB_ReadBitStream&	aReadBitStream, 
	int					aNumSymbols, 
	MSB_IStream&		aWriteStream,
	MSB_HuffmanBuilder*	aSymbolCounter)
{
	int					res = 0;
	uint8				buf[16];

	do 
	{
		int writeLen = __min(aNumSymbols, sizeof(buf));
		for ( int i = 0; i < writeLen; i++ )
		{
			if ( DecodeSymbol(aReadBitStream, buf[i]) )
				res++;
			else
				return -2;
		}
		if ( aWriteStream.Write(buf, writeLen) != writeLen )
			return -1; //Error, or short read

		if ( aSymbolCounter != NULL )
			aSymbolCounter->CountSymbols( buf, writeLen );

		aNumSymbols -= writeLen;
	} while (aNumSymbols > 0);

	return res;
}
*/

bool
MSB_Huffman::EncodeSymbol( 
	uint8				aSymbol,
	MSB_WriteBitBuffer&	aWriteBitBuffer )
{
	//Need to empty anything in our bit stack first, and it has to fit this time
	bool res = PrivWriteBits(aWriteBitBuffer);
	assert( res && "Provided buffer too short. Must be atleast 10 bytes." );

	/* Algorithm:
	* Start at the symbol and go up the tree, recording the path using a bits buffer
	* by adding the bits to front, those giving a a path read from left to right.
	*/
	do //A symbol is coded with at least one bit
	{
		myWriteBuf1 |= ( ((uint64)myNodeLeafs[aSymbol].myCode) << myNumBits1 );
		myNumBits1++;

		if ( myNumBits1 == sizeof(myWriteBuf1)*8 )
		{
			myWriteBuf2 = myWriteBuf1;
			myNumBits2 = sizeof(myWriteBuf1)*8;

			myWriteBuf1 = 0;
			myNumBits1 = 0;
		}

		aSymbol = myNodeLeafs[aSymbol].myParent;
	} while ( aSymbol !=  HUFF_ROOT ); //while not reached root

	//Need to empty anything in our bit stack first
	if ( PrivWriteBits(aWriteBitBuffer) == false )
		return false; //Buffer is full, encoding not complete

	return true; //The symbol is now encoded
}

bool
MSB_Huffman::PrivWriteBits(
	MSB_WriteBitBuffer&	aWriteBitBuffer)
{
	while ( myNumBits2 > 0)
	{
		if ( aWriteBitBuffer.WriteBit(  (uint8) (myWriteBuf2 >> myNumBits2)&1 ) == -1 )
			return false;
		else
			myNumBits2--;
	}
	myWriteBuf2 = 0;

	while ( myNumBits1 > 0)
	{
		if ( aWriteBitBuffer.WriteBit(  (uint8) (myWriteBuf1 >> myNumBits1)&1 ) == -1 )
			return false;
		else
			myNumBits1--;
	}
	myWriteBuf1 = 0;

	return true;
}

bool
MSB_Huffman::DecodeSymbol(
	MSB_ReadBitBuffer&	aReadBitBuffer,
	uint8&				aSymbol)
{
	do 
	{
		int				bit = aReadBitBuffer.ReadBit();
		if ( bit == -1 )
			return false;

		myDecodePtr = bit ? myNodeLeafs[myDecodePtr].myRight : myNodeLeafs[myDecodePtr].myLeft;
	} while (myDecodePtr >= HUFF_LEAFS);

	aSymbol = (uint8) myDecodePtr;
	myDecodePtr = HUFF_ROOT; //Start up next decode
	return true;
}

bool 
MSB_Huffman::GetHuffmanTree( 
	MSB_WriteBitStream& aWriteBitStream )
{
	return PrivEncodeHuffmanTree(HUFF_ROOT, aWriteBitStream);
}

bool 
MSB_Huffman::PrivEncodeHuffmanTree( 
	int					aPtr,
	MSB_WriteBitStream& aWriteBitStream )
{
	bool res;
	if ( aPtr < HUFF_LEAFS )
	{
		res = aWriteBitStream.WriteBit(0) != -1; //I am a leaf, symbol follows
		res = res && aWriteBitStream.WriteBits(aPtr, 8) != -1;
			
	}
	else
	{
		res = aWriteBitStream.WriteBit(1) != -1; //I am a node, two children follows
		res = res && PrivEncodeHuffmanTree(myNodeLeafs[aPtr].myLeft, aWriteBitStream);
		res = res && PrivEncodeHuffmanTree(myNodeLeafs[aPtr].myRight, aWriteBitStream);
	}

	return res;
}

bool
MSB_Huffman::SetHuffmanTree(
	MSB_ReadBitStream&	aReadBitStream )
{
	int					node = HUFF_ROOT;
	int					res = PrivDecodeHuffmanTree(node, 0, node, aReadBitStream);
	assert( node == (HUFF_LEAFS - 1) ); //All nodes should have been used
	return res != -1;
}


int
MSB_Huffman::PrivDecodeHuffmanTree(
	int					aParent,
	int					aCode,
	int&				aNextNode,
	MSB_ReadBitStream&	aReadBitStream )
{
	int					bit = aReadBitStream.ReadBit();
	if ( bit == -1 )
		return -1;
	else if ( bit == 0 )
	{	//leaf
		uint64			symbol = 0;
		if ( aReadBitStream.ReadBits(symbol, 8) == -1)
			return -1;
		assert(symbol >= 0 && symbol <= 255);
		myNodeLeafs[symbol].myParent = aParent;
		myNodeLeafs[symbol].myCode = aCode;
		return (uint8) symbol;
	}
	else //bit == 1
	{	//node
		int				node = aNextNode;
		aNextNode--;
		
		myNodeLeafs[node].myParent = aParent;
		myNodeLeafs[node].myCode = aCode;
		
		int				left = PrivDecodeHuffmanTree(node, 0, aNextNode, aReadBitStream);
		if (left == -1)
			return -1;
		myNodeLeafs[node].myLeft = left;
		
		int				right = PrivDecodeHuffmanTree(node, 1, aNextNode, aReadBitStream);
		if (right == -1)
			return -1;
		myNodeLeafs[node].myRight = right;

		return node;
	}
}

// MSB_HuffmanBuilder
/////////////////////////////

MSB_HuffmanBuilder::MSB_HuffmanBuilder()
{
	Clear();

	//Initing the priority queue
	myPQLast = -1;
}

MSB_HuffmanBuilder::~MSB_HuffmanBuilder()
{
}

void 
MSB_HuffmanBuilder::Clear()
{
	//Init all the leafs
	for (int i = 0; i < HUFF_LEAFS; i++)
	{
		myNodeLeafs[i].myFreq = 0; 
		myNodeLeafs[i].myDepth = 0;
	}
}

void 
MSB_HuffmanBuilder::CountSymbol( 
	uint8				aSymbol )
{
	myNodeLeafs[aSymbol].myFreq++;
}

void
MSB_HuffmanBuilder::CountSymbols( 
	const uint8*		aBuffer, 
	int					aBufLen )
{
	for (int i = 0; i < aBufLen; i++)
		myNodeLeafs[aBuffer[i]].myFreq++;
}

void 
MSB_HuffmanBuilder::SetSymbolCount(
	uint8				aSymbol, 
	uint64				aCount )
{
	myNodeLeafs[aSymbol].myFreq = aCount;
}

void
MSB_HuffmanBuilder::SetHuffmanTree(
	MSB_Huffman&		aHuffman)
{
	int i; //Loop continues on the next for-loop
	for (i = 0; i < HUFF_LEAFS; i++)
		PriorityQueue_Insert(i); 	//insert all leafs in priority queue

	/* Algorithm:
	* While ( at least 2 elements in myPQ )
	*   take out 2 smallest, a and b (a < b)
	*   use a new node with a as left and b as right
	*   insert this node into myPQ
	* Final root of tree will be in myNodeLeafs[HUFF_ROOT]
	*/
	for ( ; i < HUFF_SIZE; i++) 
	{
		NodeLeafPtr	a = PriorityQueue_GetMin();
		NodeLeafPtr	b = PriorityQueue_GetMin();

		assert( a != HUFF_INVALID && b != HUFF_INVALID );

		myNodeLeafs[a].myParent = i;
		myNodeLeafs[a].myCode = 0;
		myNodeLeafs[b].myParent = i;
		myNodeLeafs[b].myCode = 1;

		myNodeLeafs[i].myLeft = a;
		myNodeLeafs[i].myRight = b;
		myNodeLeafs[i].myFreq = myNodeLeafs[a].myFreq + myNodeLeafs[b].myFreq;
		myNodeLeafs[i].myDepth = __max(myNodeLeafs[a].myDepth, myNodeLeafs[b].myDepth) + 1;

		PriorityQueue_Insert(i);
	}
	assert( PriorityQueue_GetMin() == HUFF_ROOT ); //The only element leaf in the priority queue should be the root
	assert( PriorityQueue_GetMin() == HUFF_INVALID ); //Should now be empty


	//Now copy the tree to the MSB_Huffman
	for (int k = 0; k < HUFF_SIZE; k++)
	{
		aHuffman.myNodeLeafs[k].myParent	= myNodeLeafs[k].myParent;
		aHuffman.myNodeLeafs[k].myCode		= myNodeLeafs[k].myCode;
		aHuffman.myNodeLeafs[k].myLeft		= myNodeLeafs[k].myLeft;
		aHuffman.myNodeLeafs[k].myRight		= myNodeLeafs[k].myRight;
	}
}


MSB_HuffmanBuilder::NodeLeafPtr 
MSB_HuffmanBuilder::PriorityQueue_GetMin()
{
	if ( myPQLast == -1 )
		return HUFF_INVALID;

	NodeLeafPtr	ret = myPQHeap[0];

	myPQHeap[0] = myPQHeap[myPQLast]; //Move last item to front
	myPQLast--;

	int parent = 0;
	for (;;)
	{
		int child;
		int child1 = (parent + 1) * 2 - 1;
		int child2 = (parent + 1) * 2;
		if ( child1 > myPQLast ) 
			break; //We are done sifting, parent is a leaf
		else if ( child2 > myPQLast )
			child = child1; //only one child to select from
		else if ( IsBigger(myPQHeap[child2], myPQHeap[child1]) ) 
			child = child1; //Child1 was smaller
		else 
			child = child2; //Child2 was smaller

		//sift towards back
		if ( IsBigger(myPQHeap[parent], myPQHeap[child]) )
		{
			//swap
			NodeLeafPtr tmp = myPQHeap[parent];
			myPQHeap[parent] = myPQHeap[child];
			myPQHeap[child] = tmp;

			parent = child;
		}
		else 
		{
			//stop when parent <= child, parent is the smaller
			break;
		}
	}

	return ret;
}

void 
MSB_HuffmanBuilder::PriorityQueue_Insert( 
	NodeLeafPtr		aNodeLeaf )
{
	assert( myPQLast < HUFF_LEAFS - 1); //Don't overflow

	myPQLast++;
	myPQHeap[myPQLast] = aNodeLeaf;  //Insert last

	//Start sifting towards front
	int child = myPQLast;
	while ( child > 0 ) 
	{
		int parent = (child - 1) / 2;

		if ( IsBigger(myPQHeap[parent], myPQHeap[child]) )
		{
			//swap
			NodeLeafPtr tmp = myPQHeap[parent];
			myPQHeap[parent] = myPQHeap[child];
			myPQHeap[child] = tmp;

			child = parent;
		}
		else 
		{
			//stop when parent <= child, parent is the smaller
			break;
		}
	}

}

bool 
MSB_HuffmanBuilder::IsBigger( 
	NodeLeafPtr		aOne, 
	NodeLeafPtr		aTwo )
{
	assert( aOne < HUFF_SIZE && aTwo < HUFF_SIZE );

	if ( myNodeLeafs[aOne].myFreq == myNodeLeafs[aTwo].myFreq )
		return myNodeLeafs[aOne].myDepth > myNodeLeafs[aTwo].myDepth;	//Try to keep the tree as shallow as possible, 
																		//Larger depth (of children) means the node is bigger
	else
		return myNodeLeafs[aOne].myFreq > myNodeLeafs[aTwo].myFreq;
}

#endif // IS_PC_BUILD
