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

#include "MSB_MemoryStream.h"

MSB_MemoryStream::MSB_MemoryStream(
	void*		aBuffer, 
	uint32		aLength,
	bool		anOwnsStreamFlag)
	: myBuffer( (uint8*) aBuffer)
	, myLength(aLength)
	, myOffset(0)
	, myOwnsStream(anOwnsStreamFlag)
{
}

MSB_MemoryStream::~MSB_MemoryStream()
{
	if(myOwnsStream)
		delete [] myBuffer;
}

uint32
MSB_MemoryStream::GetUsed()
{
	return myLength - myOffset;
}

uint32
MSB_MemoryStream::Read(
	void*			aBuffer,
	uint32			aSize)
{
	uint32 s = __min(aSize, myLength - myOffset);
	
	memmove(aBuffer, &myBuffer[myOffset], s);
	myOffset += s;

	return s;
}

uint32		
MSB_MemoryStream::Peek(
	void*			aBuffer,
	uint32			aSize)
{
	uint32 s = __min(aSize, myLength - myOffset);

	memmove(aBuffer, &myBuffer[myOffset], s);

	return s;
}

uint32
MSB_MemoryStream::Write(
	const void*		aBuffer,
	uint32			aSize)
{
	assert(false && "Should never be called.");

	return 0;
}
