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
#ifndef MSB_ISTREAM_H
#define MSB_ISTREAM_H

class MSB_IStream 
{
public:
	virtual				~MSB_IStream() {}
		
	virtual uint32		GetUsed() { assert(false && "GetUsed() on the this MSB_IStream is not possible."); return 0; }

	virtual uint32		Read(
							void*			aBuffer,
							uint32			aSize) { assert(false && "Read() on the this stream is not possible."); return 0; }
	virtual uint32		ReadSkip(
							uint32			aSize) { assert(false && "ReadSkip() on the this stream is not possible."); return 0; }
	virtual uint32		Peek(
							void*			aBuffer,
							uint32			aSize)  { assert(false && "Peek() on the this stream is not possible."); return 0; }
	virtual uint32		Write(
							const void*		aBuffer,
							uint32			aSize)  { assert(false && "Write() on the this stream is not possible."); return 0; }
	virtual uint32		WriteSkip(
							uint32			aSize)  { assert(false && "WriteSkip() on the this stream is not possible."); return 0; }
};

#endif /* MSB_ISTREAM_H */
