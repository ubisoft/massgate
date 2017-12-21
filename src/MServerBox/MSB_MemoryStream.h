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
#ifndef MSB_MEMORYSTREAM_H_
#define MSB_MEMORYSTREAM_H_

#include "MSB_IStream.h"

class MSB_MemoryStream : public MSB_IStream
{
public:
						MSB_MemoryStream(
							void*		aBuffer, 
							uint32		aLength,
							bool		anOwnsStreamFlag = false);

	virtual				~MSB_MemoryStream();

	virtual uint32		GetUsed();

	virtual uint32		Read(
							void*			aBuffer,
							uint32			aSize);
	virtual uint32		Peek(
							void*			aBuffer,
							uint32			aSize);
	virtual uint32		Write(
							const void*		aBuffer,
							uint32			aSize);

private:
	uint8*				myBuffer;
	uint32				myLength;
	uint32				myOffset;
	bool				myOwnsStream;
};

#endif // MSB_MEMORYSTREAM_H_
