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

// #include "MC_Logger.h"

#include "MSB_WriteableMemoryStream.h"
#include "MSB_WriteBuffer.h"

MSB_WriteableMemoryStream::MSB_WriteableMemoryStream()
	: myRead(0)
	, myReadBuffer(NULL)
	, myReadOffset(0)
	, myWMarkBuffer(NULL)
	, myWrite(0)
	, myWriteBuffer(NULL)
{
}

MSB_WriteableMemoryStream::~MSB_WriteableMemoryStream()
{
	PrivReleaseAll();
}

uint32
MSB_WriteableMemoryStream::Read(
	void*			aBuffer,
	uint32			aSize)
{
	return PrivRead(aBuffer, aSize, false);
}

uint32
MSB_WriteableMemoryStream::Peek(
	void*			aBuffer,
	uint32			aSize)
{
	return PrivRead(aBuffer, aSize, true);
}

uint32
MSB_WriteableMemoryStream::ReadSkip(
	uint32			aSize)
{
	return PrivRead(NULL, aSize, false);
}

uint32
MSB_WriteableMemoryStream::Write(
	const void*		aBuffer,
	uint32			aSize)
{
	return PrivWrite(aBuffer, aSize);
}

uint32 
MSB_WriteableMemoryStream::Write( 
	MSB_IStream&	aReadStream, 
	uint32			aSize )
{
	uint32				size = aSize;
	while ( size > 0 )
	{
		int32			blockSize = __min(size, myWriteBuffer->myBufferSize - myWriteBuffer->myWriteOffset);
		int32			readLen = aReadStream.Read( &myWriteBuffer->myBuffer[myWriteBuffer->myWriteOffset], blockSize );
		assert( readLen == blockSize && "Short read in MSB_WriteableMemoryStream::Write()");

		myWriteBuffer->myWriteOffset += blockSize;
		size -= readLen;

		if(myWriteBuffer->myWriteOffset == myWriteBuffer->myBufferSize)
		{
			myWriteBuffer->myNextBuffer = MSB_WriteBuffer::Allocate();
			myWriteBuffer = myWriteBuffer->myNextBuffer;
		}
	}

	myWrite += aSize;
	return aSize;
}

uint32		
MSB_WriteableMemoryStream::WriteSkip(
	uint32			aSize)
{
	return PrivWrite(NULL, aSize);
}

void 
MSB_WriteableMemoryStream::WriteMark()
{
	if ( myWriteBuffer == NULL )
		myReadBuffer = myWriteBuffer = MSB_WriteBuffer::Allocate();

	myWMark = myWrite;
	myWMarkBuffer = myWriteBuffer;
	myWMarkOffset = myWriteBuffer->myWriteOffset;
}


void 
MSB_WriteableMemoryStream::WriteRewind()
{
	assert(myWMarkBuffer != NULL && "Use WriteMark() before WriteRewind()" );

	MSB_WriteBuffer*		remove = myWMarkBuffer->myNextBuffer;
	if ( remove != NULL )
		remove->ReleaseAll();

	myWrite = myWMark;
	myWriteBuffer = myWMarkBuffer;
	myWMarkBuffer->myWriteOffset = myWMarkOffset;
}

void 
MSB_WriteableMemoryStream::WriteUnmark()
{
	myWMarkBuffer = NULL;
}

void
MSB_WriteableMemoryStream::OverwriteAtWriteMark( 
	const void*			aBuffer, 
	uint32				aSize,
	bool				aMoveMarker)
{
	assert( myWMarkBuffer != NULL && "Use WriteMark() before OverwriteAtWriteMark().");

	//Laziness, We don't want to handle this case, and it shouldn't ever happen
	assert( myWMark + aSize <= myWrite && "Can't overwrite data not yet written."); 

	uint32				size = aSize;
	const uint8*		buffer = (const uint8*) aBuffer;
	MSB_WriteBuffer*	writeBuffer = myWMarkBuffer;
	uint32				writeOffset = myWMarkOffset;

	while (size > 0)
	{
		uint32			blockSize = __min(size, writeBuffer->myBufferSize - writeOffset);
		memcpy(&writeBuffer->myBuffer[writeOffset], buffer, blockSize);

		size -= blockSize;
		buffer += blockSize;

		if(writeOffset == writeBuffer->myBufferSize)
		{
			writeBuffer = writeBuffer->myNextBuffer;
			writeOffset = 0;
		}
	}

	if ( aMoveMarker )
	{
		myWMark += aSize;
		myWMarkBuffer = writeBuffer;
		myWMarkOffset = writeOffset;
	}
}

void 
MSB_WriteableMemoryStream::Clear()
{
	PrivReleaseAll();
	
	myRead = 0;
	myReadBuffer = NULL;
	myReadOffset = 0;
	
	myWMarkBuffer = NULL; //Not marked

	myWrite = 0;
	myWriteBuffer = NULL;
}

/**
 * Copies all of my data between myRead and myWrite to aTarget
 */
void 
MSB_WriteableMemoryStream::Clone( 
	MSB_WriteableMemoryStream&	aTarget )
{
	MSB_WriteBuffer*	currWriteBuffer = myReadBuffer;
	uint32				currReadOffset = myReadOffset;

	while ( currWriteBuffer != NULL )
	{
		uint32			blockSize = currWriteBuffer->myWriteOffset - currReadOffset;
		uint32			wroteLen = aTarget.Write(currWriteBuffer->myBuffer + currReadOffset, blockSize);
		assert( wroteLen == blockSize && "Short write in MSB_WriteableMemoryStream::Clone()" );

		currWriteBuffer = currWriteBuffer->myNextBuffer;
		currReadOffset = 0;
	}	
}


MSB_WriteBuffer* 
MSB_WriteableMemoryStream::GetBuffers() const
{
	return myReadBuffer;
}


MSB_WriteBuffer* 
MSB_WriteableMemoryStream::DetachBuffers()
{
	MSB_WriteBuffer*	buffers = myReadBuffer;
	myReadBuffer = NULL; //So Clear() wont release the buffers
	Clear();  

	return buffers;
}

uint32 
MSB_WriteableMemoryStream::PrivRead( 
	const void*					aBuffer, //Is NULL if we can't to Skip()
	uint32						aSize, 
	bool						aPeek ) 
{
	assert(myReadBuffer != NULL && "Don't Read() on an empty stream.");
	
	uint32			readSize = __min(aSize, GetUsed());
	assert( myWMarkBuffer == NULL || myRead + readSize <= myWMark ); //Don't read past WriteMark
	
	uint8*			buffer = (uint8*) aBuffer;
	uint32			size = readSize;

	uint32				readOffset = myReadOffset;
	MSB_WriteBuffer*	readBuffer = myReadBuffer;

	while(size > 0)
	{
		uint32		blockSize = __min(size, readBuffer->myWriteOffset - readOffset);

		if ( aBuffer != NULL ) //Skip only?
		{
			memcpy(buffer, &readBuffer->myBuffer[readOffset], blockSize);
			buffer += blockSize;
		}
		readOffset += blockSize;
		size -= blockSize;

		if(readOffset == readBuffer->myBufferSize)
		{
			MSB_WriteBuffer*		last = readBuffer;
			readBuffer = readBuffer->myNextBuffer;
			if ( aPeek == false ) //if Read()
			{
				last->Release();
				myReadBuffer = readBuffer;
			}
			readOffset = 0;
		}
	}

	if ( aPeek == false ) //This was a true Read()? update the pointer
	{
		myRead += readSize;
		myReadBuffer = readBuffer;
		myReadOffset = readOffset;
	}

	return readSize;	
}

uint32
MSB_WriteableMemoryStream::PrivWrite(
	 const void*		aBuffer, //Is NULL if we want to WriteSkip()
	 uint32				aSize)
{
	uint32			size = aSize;
	const uint8*	buffer = (const uint8*) aBuffer;

	if (myWriteBuffer == NULL)
		myReadBuffer = myWriteBuffer = MSB_WriteBuffer::Allocate();

	while(size > 0)
	{
		uint32	blockSize = __min(size, myWriteBuffer->myBufferSize - myWriteBuffer->myWriteOffset);
		if ( aBuffer ) // If this is null we're just skipping forward.
			memcpy(&myWriteBuffer->myBuffer[myWriteBuffer->myWriteOffset], buffer, blockSize);

		myWriteBuffer->myWriteOffset += blockSize;
		size -= blockSize;
		buffer += blockSize;
		
		if(myWriteBuffer->myWriteOffset == myWriteBuffer->myBufferSize)
		{
			myWriteBuffer->myNextBuffer = MSB_WriteBuffer::Allocate();
			myWriteBuffer = myWriteBuffer->myNextBuffer;
		}
	}

	myWrite += aSize;
	return aSize;
}

void 
MSB_WriteableMemoryStream::PrivReleaseAll()
{
	if (myReadBuffer)
		myReadBuffer->ReleaseAll();
}

