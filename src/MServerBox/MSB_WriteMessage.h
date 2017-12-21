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
#ifndef MSB_WRITEMESSAGE_H
#define MSB_WRITEMESSAGE_H

#include "MC_BitVector.h"
#include "MC_String.h"
#include "MC_Vector.h"

#include "MSB_Types.h"
#include "MSB_WriteableMemoryStream.h"
#include "MSB_WriteBuffer.h"
#include "MSB_Xtea.h"

class MSB_WriteMessage
{
public:
						MSB_WriteMessage(
							MSB_TriState aUseTypeChecking = MSB_TS_UNKNOWN);
						~MSB_WriteMessage();

	static void			GlobSetTypeChecking(
							bool aUseTypeChecking);

	static bool			GlobGetTypeChecking();


	void				Clone(
							MSB_WriteMessage&	aMessage);

	void				SetSystemMessage();
	void				SetNamespace(
							uint32				aNamespace);
	uint32				GetNamespace(); 

	uint32				GetNumBytesOnCurrentDelim() const { return myNumBytesOnCurrentDelim; }
	uint32				GetRemainingByteCount() const { return MSB_MAX_DELIMITER_DATA_SIZE - myNumDataBytes; }
	uint32				GetMaxBytes() const { return MSB_MAX_DELIMITER_DATA_SIZE; }

	bool				IsEmpty();
	uint32				GetSize();
	void				Clear();

	void				Append(
							MSB_WriteMessage&	aMsg); 

	MSB_WriteBuffer*	GetHeadBuffer();
	void				AttachBuffers(
							MSB_WriteBuffer*	aBuffer);

	void				WriteDelimiter(
							MSB_DelimiterType	aDelimiter);

	void				WriteBool(
							bool			aBool);

	void				WriteInt8(
							int8			anInt8);

	void				WriteUInt8(
							uint8			anUInt8);
	
	void				WriteInt16(
							int16			anInt16);
	
	void				WriteUInt16(
							uint16			anUInt16);
	
	void				WriteInt32(
							int32			anInt32);
	
	void				WriteUInt32(
							uint32			anUInt32);
	
	void				WriteInt64(
							int64			anInt64);

	void				WriteUInt64(
							uint64			anUInt64);

	void				WriteFloat32(
							float32			aFloat32); 
	
	void				WriteFloat64(
							float64			aFloat64);
	
	void				WriteString(
							const char*		aString);

	void				WriteLocString(
							const MC_LocString& aString)
						{
							WriteLocString(aString.GetBuffer(), aString.GetLength());
						}

	template<int S>
	void				WriteLocString(
							const MC_StaticLocString<S>& aString) 
						{ 
							WriteLocString(aString.GetBuffer(), aString.GetLength()); 
						}

	void				WriteLocString(
							const wchar_t*	aString, 
							uint16			aStringLen);

	void				WriteRawData(
							const void*		someData, 
							uint16			aSize);

	void				WriteVector3f32(
							const MC_Vector3f& aVector);

	void				WriteVector2f32(
							const MC_Vector2f& aVector);

	void				WriteAnglef32(
							float32			anAngle);

	void				WriteBitVector(
							const MC_BitVector& aBitVector);

	void				WriteCryptData(
							const void*		aSourceData, 
							uint16			aSize, 
							const MSB_Xtea&	aChiper);

	void				WriteCryptString(
							const char*		aSourceString, 
							uint16			aSize, 
							const MSB_Xtea&	aChiper);

	void				WriteCryptLocString(
							const wchar_t*	aSourceString, 
							uint16			aSize, 
							const MSB_Xtea& aChiper); 

private:
	static bool			ourUseTypeChecking;
	bool				myUseTypeChecking;

	//Valid for the entire WriteMessage
	MSB_WriteableMemoryStream	myStream;
	uint32				myNumOfDelimiters;
	bool				myIsSystemMessage;
	uint32				myNamespace; 
	uint32				myNumOfShouldEncrypt;
	uint16				myNumBytesOnCurrentDelim;
	uint32				myNumDataBytes;

	void				PrivHeaderOpen();
	void				PrivHeaderClose();

	void				PrivWriteData(
							const uint8*	aDataBuffer, 
							uint32			aDataBufferLength);
	void				PrivWriteTypeCheck(
							MSB_TypecheckType	aType);
};

#endif // MSB_WRITEMESSAGE_H
