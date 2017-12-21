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
#ifndef MSB_WRITEABLEMEMORYSTREAM_H
#define MSB_WRITEABLEMEMORYSTREAM_H

#include "MSB_IStream.h"

class MSB_WriteBuffer;

class MSB_WriteableMemoryStream : public MSB_IStream
{
public:
						MSB_WriteableMemoryStream();
	virtual				~MSB_WriteableMemoryStream();

	virtual uint32		GetUsed() { return (uint32) (myWrite - myRead); }

	//Read
	virtual uint32		Read(
							void*			aBuffer,
							uint32			aSize);
	virtual uint32		Peek(
							void*			aBuffer,
							uint32			aSize);
	virtual uint32		ReadSkip(
							uint32			aSize);
	//Write
	virtual uint32		Write(
							const void*		aBuffer,
							uint32			aSize);
	uint32				Write(
							MSB_IStream&	aReadStream,
							uint32			aSize);
	virtual uint32		WriteSkip(
							uint32			aSize);
	void				WriteMark();
	void				WriteRewind();
	void				WriteUnmark(); //Allow reader to read past write mark
	void				OverwriteAtWriteMark(
							const void*		aBuffer,
							uint32			aSize,
							bool			aMoveMarker = false);

	void				Clear();
	void				Clone(
							MSB_WriteableMemoryStream&	aTarget);

	MSB_WriteBuffer*	DetachBuffers();
	MSB_WriteBuffer*	GetBuffers() const;

	

	
private:
	/*
	The 64bits are absolute byte pointers into the stream.
	The rule on the absolute values is:
		Read <= WMark <= Write
	Current size (used) is (Write - Read)

	if WMarkBuffer is NULL, then the WMark counter is invalid.
	*/

	uint64				myRead;
	MSB_WriteBuffer*	myReadBuffer;
	uint32				myReadOffset;

	uint64				myWMark;
	MSB_WriteBuffer*	myWMarkBuffer;
	uint32				myWMarkOffset;
	
	uint64				myWrite;
	MSB_WriteBuffer*	myWriteBuffer;
	
	uint32				PrivRead(
							const void*		aBuffer, //Is NULL if we want to Skip()
							uint32			aSize,
							bool			aPeek);
	uint32				PrivWrite(
							const void*		aBuffer, //Is NULL if we want to Skip()
							uint32			aSize);

	void				PrivReleaseAll();
};

#endif /* MSB_WRITEABLEMEMORYSTREAM_H */
