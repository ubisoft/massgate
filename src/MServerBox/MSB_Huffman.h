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
#ifndef MSB_HUFFMAN_H
#define MSB_HUFFMAN_H

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_256Bits.h"
#include "MSB_BitBuffer.h"
#include "MSB_BitStream.h"

#define	HUFF_LEAFS 256
#define	HUFF_NODES 255
#define	HUFF_SIZE (HUFF_LEAFS + HUFF_NODES)
#define HUFF_INVALID (-1)
#define	HUFF_ROOT (HUFF_SIZE - 1)

class MSB_HuffmanBuilder;

class MSB_Huffman
{	
public:
								MSB_Huffman();
								~MSB_Huffman();
// 	//Do not forget to run aWriteBitStream.FlushBuffer() afterwards
// 	int							EncodeStream(
// 									MSB_IStream&		aReadStream,
// 									int					aNumSymbols, 
// 									MSB_WriteBitStream&	aWriteBitStream,
// 									MSB_HuffmanBuilder*	aSymbolCounter = NULL);
// 	
// 	//Do not forget to run aReadBitStream.FlushBuffer() afterwards
// 	int							DecodeStream(
// 									MSB_ReadBitStream&	aReadBitStream,
// 									int					aNumSymbols,
// 									MSB_IStream&		aWriteStream,
// 									MSB_HuffmanBuilder*	aSymbolCounter = NULL);

	bool						EncodeSymbol(
									uint8				aSymbol,
									MSB_WriteBitBuffer&	aWriteBitBuffer);

	bool						DecodeSymbol(
									MSB_ReadBitBuffer&	aReadBitBuffer,
									uint8&				aSymbol);

	bool						GetHuffmanTree(
									MSB_WriteBitStream&	aWriteBitStream);

	bool						SetHuffmanTree(
									MSB_ReadBitStream&	aReadBitStream);



private:
	friend class MSB_HuffmanBuilder; //A HuffmanBuilder has access this build myself

	typedef struct 
	{
		uint32					myParent : 9;	//Pointer to parent
		uint32					myCode : 1;		//Am i left or right child of my parent (cache for encoder)
		uint32					myLeft : 9;		
		uint32					myRight : 9;
	} NodeLeaf;

	NodeLeaf					myNodeLeafs[HUFF_SIZE]; //My huffman tree, 256 leafs first, followed 255 nodes
														//Root is the last (HUFF_ROOT) element.

	//With real bad luck, a symbol can be encoded with 256 bits, a fully unbalanced tree
	//However, for this to happen, we have to have a count of somewhere around 2^256 on the most common symbol
	// Every symbol has to have the double count of the last two, beside the first 2 which can have a 
	// count of 0. 
	// This gives us a max depth of 64 and some for the rest of the 192 symbols = 8. So 72.
	// Two 64 bits counters will always be enough
	uint64						myWriteBuf1;
	uint64						myWriteBuf2;
	uint8						myNumBits1;
	uint8						myNumBits2;

	uint16						myDecodePtr;

	bool						PrivEncodeHuffmanTree( 
									int					aPtr,
									MSB_WriteBitStream& aWriteBitStream );
	int							PrivDecodeHuffmanTree(
									int					aParent,
									int					aCode,
									int&				aNextNode,
									MSB_ReadBitStream&	aReadBitStream );

	bool						PrivWriteBits(
									MSB_WriteBitBuffer&	aWriteBitBuffer);
};


class MSB_HuffmanBuilder
{
public:
	MSB_HuffmanBuilder();
	~MSB_HuffmanBuilder();

	void						Clear();

	void						CountSymbol(
									uint8			aSymbol);
	void						CountSymbols(
									const uint8*	aBuffer,
									int				aBufLen);

	void						SetSymbolCount(
									uint8			aSymbol,
									uint64			aCount);

	void						SetHuffmanTree(
									MSB_Huffman&	aHuffman);

private:

	typedef int16 NodeLeafPtr;

	typedef struct 
	{
		uint64					myFreq;
		NodeLeafPtr				myLeft;
		NodeLeafPtr				myRight;
		NodeLeafPtr				myParent;
		uint8					myDepth; //Depth of the tree below this node
		uint8					myCode; //0 if i am a left child of my parent, 1 if right child
	} NodeLeaf;

	NodeLeaf					myNodeLeafs[HUFF_SIZE]; //Leafs followed by Nodes. Root node is the last

	//Functions and variables for Priority Queue handling
	NodeLeafPtr					myPQHeap[HUFF_LEAFS];
	NodeLeafPtr					myPQLast;

	NodeLeafPtr					PriorityQueue_GetMin();
	void						PriorityQueue_Insert(
									NodeLeafPtr		aNodeLeaf);
	bool						IsBigger(
									NodeLeafPtr		aNodeLeaf1,
									NodeLeafPtr		aNodeLeaf2);
};


#endif // IS_PC_BUILD

#endif // MSB_HUFFMAN_H
