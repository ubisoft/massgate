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

#include "MSB_WriteMessage.h"

#include "MC_Hybridarray.h"
#include "MC_Endian.h"

#include "MSB_WriteBuffer.h"
 
bool MSB_WriteMessage::ourUseTypeChecking = false; 

void
MSB_WriteMessage::GlobSetTypeChecking(
	bool aUsetypeChecking)
{
	ourUseTypeChecking = aUsetypeChecking; 	
}

bool
MSB_WriteMessage::GlobGetTypeChecking()
{
	return ourUseTypeChecking; 
}



MSB_WriteMessage::MSB_WriteMessage(
	MSB_TriState aUseTypeChecking)
	: myNumOfDelimiters(0)
	, myIsSystemMessage(false)
	, myNamespace(0)
	, myNumOfShouldEncrypt(0)
	, myNumBytesOnCurrentDelim(0)
	, myNumDataBytes(0)
{
	if(aUseTypeChecking == MSB_TS_UNKNOWN)
		myUseTypeChecking = ourUseTypeChecking;
	else if(aUseTypeChecking == TRUE)
		myUseTypeChecking = true; 
	else 
		myUseTypeChecking = false;
}

MSB_WriteMessage::~MSB_WriteMessage()
{
}

/**
 * Makes a clone of the current WriteMessage into the argument.
 * Side effect is that the delimiter will be closed
 */
void
MSB_WriteMessage::Clone(
	MSB_WriteMessage&	aTarget)
{
	assert( myNumOfDelimiters > 0 && "Don't Clone() an empty WriteMessage");

	//We must close and write the header as we can't read the entire stream unless
	// its WriteMark() is removed
	PrivHeaderClose();

	aTarget.myUseTypeChecking = myUseTypeChecking;
	aTarget.myIsSystemMessage = myIsSystemMessage;
	aTarget.myNamespace = myNamespace;
	aTarget.myNumOfDelimiters = myNumOfDelimiters;
	aTarget.myNumBytesOnCurrentDelim = myNumBytesOnCurrentDelim;

	myStream.Clone(aTarget.myStream);

// 	myStream.ReadMark();
// 	uint32 readLen = myStream.GetUsed();
// 	uint32 copyLen = aTarget.myStream.Copy(myStream, readLen);
// 	assert(readLen == copyLen && "Short Copy() in MSB_WriteMessage::Clone()");
// 	myStream.ReadRewind();
}

MSB_WriteBuffer*
MSB_WriteMessage::GetHeadBuffer()
{
	if ( myNumOfDelimiters != 0)
		PrivHeaderClose();

	return myStream.GetBuffers(); 
}

bool
MSB_WriteMessage::IsEmpty()
{
	return myStream.GetUsed() == 0;
}

uint32 
MSB_WriteMessage::GetSize()
{
	return myStream.GetUsed();
}


/**
 * Sets the system flag for _all_ delimiters in this write message
 */
void
MSB_WriteMessage::SetSystemMessage()
{
	myIsSystemMessage = true;
}

void
MSB_WriteMessage::SetNamespace(
	uint32	aNamespace)
{
	myNamespace = aNamespace; 
}

uint32
MSB_WriteMessage::GetNamespace()
{
	return myNamespace; 
} 

void
MSB_WriteMessage::Append(
	MSB_WriteMessage&	aMsg)
{
	aMsg.PrivHeaderClose(); 
	MSB_WriteBuffer* current = aMsg.GetHeadBuffer(); 
	while(current)
	{
		myStream.Write(current->myBuffer, current->myWriteOffset); 
		current = current->myNextBuffer; 
	}
}

void
MSB_WriteMessage::Clear()
{
	myStream.Clear();
	myNumOfDelimiters = 0;
	myNumBytesOnCurrentDelim = 0;
	myNumDataBytes = 0;
	myNumOfShouldEncrypt = 0;

	myIsSystemMessage = false;
	myNamespace = 0; 
}

void 
MSB_WriteMessage::WriteDelimiter(
	MSB_DelimiterType aDelimiter)
{
	if ( myNumOfDelimiters != 0)
		PrivHeaderClose(); //Close last header
	PrivHeaderOpen(); //And start a new one	
	PrivWriteTypeCheck(MSB_TT_DELIMITER);

	myNumBytesOnCurrentDelim = 0;
	myNumOfDelimiters++;

	uint16		del = MSB_SWAP_TO_BIG_ENDIAN(aDelimiter); 
	PrivWriteData((uint8*)&del, sizeof(MSB_DelimiterType));
}

void
MSB_WriteMessage::WriteBool(
	bool aBool)
{
	PrivWriteTypeCheck(MSB_TT_BOOL);

	uint8 t = aBool ? 1 : 0;
	PrivWriteData((uint8*)&t, sizeof(uint8));
}

void
MSB_WriteMessage::WriteInt8(
	int8 anInt8)
{
	PrivWriteTypeCheck(MSB_TT_INT8);
	PrivWriteData((uint8*)&anInt8, sizeof(anInt8));
}

void
MSB_WriteMessage::WriteUInt8(
	uint8 anUInt8)
{
	PrivWriteTypeCheck(MSB_TT_UINT8);
	PrivWriteData((uint8*)&anUInt8, sizeof(uint8));
}

void
MSB_WriteMessage::WriteInt16(
	int16 anInt16)
{
	PrivWriteTypeCheck(MSB_TT_INT16);
	anInt16 = MSB_SWAP_TO_BIG_ENDIAN(anInt16);
	PrivWriteData((uint8*)&anInt16, sizeof(int16));
}

void
MSB_WriteMessage::WriteUInt16(
	uint16 anUInt16)
{
	PrivWriteTypeCheck(MSB_TT_UINT16);
	anUInt16 = MSB_SWAP_TO_BIG_ENDIAN(anUInt16);
	PrivWriteData((uint8*)&anUInt16, sizeof(uint16));
}

void
MSB_WriteMessage::WriteInt32(
	int32 anInt32)
{
	PrivWriteTypeCheck(MSB_TT_INT32);
	anInt32 = MSB_SWAP_TO_BIG_ENDIAN(anInt32);
	PrivWriteData((uint8*)&anInt32, sizeof(int32));
}

void
MSB_WriteMessage::WriteUInt32(
	uint32 anUInt32)
{
	PrivWriteTypeCheck(MSB_TT_UINT32);
	anUInt32 = MSB_SWAP_TO_BIG_ENDIAN(anUInt32);
	PrivWriteData((uint8*)&anUInt32, sizeof(uint32));
}

void
MSB_WriteMessage::WriteInt64(
	int64 anInt64)
{
	PrivWriteTypeCheck(MSB_TT_INT64);
	anInt64 = MSB_SWAP_TO_BIG_ENDIAN(anInt64);
	PrivWriteData((uint8*)&anInt64, sizeof(int64));
}

void
MSB_WriteMessage::WriteUInt64(
	uint64 anUInt64)
{
	PrivWriteTypeCheck(MSB_TT_UINT64);
	anUInt64 = MSB_SWAP_TO_BIG_ENDIAN(anUInt64);
	PrivWriteData((uint8*)&anUInt64, sizeof(uint64));
}

void
MSB_WriteMessage::WriteFloat32(
	float32 aFloat32)
{
	PrivWriteTypeCheck(MSB_TT_FLOAT32);

	uint32			value = *(uint32*) &aFloat32;
	value = MSB_SWAP_TO_BIG_ENDIAN(value);
	PrivWriteData((uint8*)&value, sizeof(value));
}

void
MSB_WriteMessage::WriteFloat64(
	float64 aFloat64)
{
	PrivWriteTypeCheck(MSB_TT_FLOAT64);
	aFloat64 = MSB_SWAP_TO_BIG_ENDIAN(aFloat64);
	PrivWriteData((uint8*)&aFloat64, sizeof(float64));
}

void
MSB_WriteMessage::WriteString(
	const char* aString)
{
	PrivWriteTypeCheck(MSB_TT_STRING);

	uint16 len = (uint16) strlen(aString);
	uint16 t = len; 
	len = MSB_SWAP_TO_BIG_ENDIAN(len);
	PrivWriteData((uint8*)&len, sizeof(uint16));
	PrivWriteData((uint8*)aString, t);
}

void
MSB_WriteMessage::WriteLocString(
	const wchar_t*		aString, 
	uint16				aStringLen)
{
	PrivWriteTypeCheck(MSB_TT_LOCSTRING);

	uint16 len = aStringLen; // * sizeof(MC_LocChar);
	uint16 t = len; 
	len = MSB_SWAP_TO_BIG_ENDIAN(len);
	PrivWriteData((uint8*)&len, sizeof(uint16)); 
	PrivWriteData((uint8*)aString, t * sizeof(wchar_t));
}

void
MSB_WriteMessage::WriteRawData(
	const void*			someData, 
	uint16				aSize)
{
	PrivWriteTypeCheck(MSB_TT_RAWDATA);
	
	uint16 t = aSize;
	aSize = MSB_SWAP_TO_BIG_ENDIAN(aSize);
	PrivWriteData((uint8*)&aSize, sizeof(uint16));
	PrivWriteData((uint8*)someData, t);
}

void
MSB_WriteMessage::WriteVector3f32(
	const MC_Vector3f&	aVector)
{
	PrivWriteTypeCheck(MSB_TT_VECTOR3F32);
	
	WriteFloat32(aVector.x);
	WriteFloat32(aVector.y);
	WriteFloat32(aVector.z);
}

void
MSB_WriteMessage::WriteVector2f32(
	const MC_Vector2f&	aVector)
{
	PrivWriteTypeCheck(MSB_TT_VECTOR2F32);

	WriteFloat32(aVector.x);
	WriteFloat32(aVector.y);
}

void				
MSB_WriteMessage::WriteAnglef32(
	float32				anAngle)
{
	PrivWriteTypeCheck(MSB_TT_ANGLE32);

	WriteFloat32(anAngle);		// Could be better I guess
}

void
MSB_WriteMessage::WriteBitVector(
	const MC_BitVector& aBitVector)
{
	PrivWriteTypeCheck(MSB_TT_BITVECTOR);

	uint16 len = (uint16) aBitVector.GetDataLength();
	len = MSB_SWAP_TO_BIG_ENDIAN(len);
	PrivWriteData((uint8*)&len, sizeof(uint16));
	PrivWriteData(aBitVector.GetData(), aBitVector.GetDataLength());
}

void
MSB_WriteMessage::WriteCryptData(
	const void*		aSourceData, 
	uint16			aSize, 
	const MSB_Xtea&	aChiper)
{
	PrivWriteTypeCheck(MSB_TT_CRYPTDATA); 

	MSB_Xtea::ClearText clearText;
	MSB_Xtea::CryptText cryptText; 

	// cookie 
	uint64 cookie = 0x1EE7DEADBEEFBABELL; 
	uint64 cryptCookie = 0; 
	clearText.myClearText = (uint8*) &cookie; 	
	clearText.myClearLen = sizeof(uint64); 
	cryptText.myCryptText = (uint8*) &cryptCookie; 
	aChiper.Encrypt(clearText, cryptText);
	WriteUInt64(cryptCookie);

	// actual message 
	clearText.myClearText = (uint8*) aSourceData; 
	clearText.myClearLen = aSize;
	cryptText.myCryptText = (uint8*) alloca(aSize + 7);
	aChiper.Encrypt(clearText, cryptText);

	WriteUInt16(cryptText.myCryptLen);
	WriteUInt16(cryptText.myPadLen);
	WriteRawData(cryptText.myCryptText, cryptText.myCryptLen);
}

void
MSB_WriteMessage::WriteCryptString(
	const char*		aSourceString, 
	uint16			aSize, 
	const MSB_Xtea&	aChiper)
{
	PrivWriteTypeCheck(MSB_TT_CRYPTSTRING); 
	WriteCryptData((void*)aSourceString, aSize, aChiper); 
}

void 
MSB_WriteMessage::WriteCryptLocString(
	const wchar_t*	aSourceString, 
	uint16			aSize, 
	const MSB_Xtea& aChiper)
{
	PrivWriteTypeCheck(MSB_TT_CRYPTLOCSTRING); 
	WriteCryptData((void*)aSourceString, aSize * sizeof(wchar_t), aChiper); 
}

void 
MSB_WriteMessage::PrivWriteTypeCheck(
	MSB_TypecheckType	aType)
{
	if(myUseTypeChecking)
	{
		uint8		b = aType;
		PrivWriteData(&b, sizeof(uint8));
		myNumDataBytes --;
	}
}

void					
MSB_WriteMessage::PrivWriteData(
	const uint8*	aDataBuffer, 
	uint32			aDataBufferLength)
{
	assert( myNumBytesOnCurrentDelim + aDataBufferLength <= MSB_MAX_DELIMITER_DATA_SIZE && "An MSB_WriteMessage can't be longer than 32k." );

	myNumDataBytes += aDataBufferLength;
	myNumBytesOnCurrentDelim += aDataBufferLength;
	uint32 len = myStream.Write(aDataBuffer, aDataBufferLength);
	assert(len == aDataBufferLength);
}

void
MSB_WriteMessage::PrivHeaderOpen()
{
	//Remember position for header
	myStream.WriteMark();

	//Reserve room for header
	myStream.WriteSkip(MSB_PACKET_HEADER_TOTSIZE);
}

void
MSB_WriteMessage::PrivHeaderClose()
{
	if ( myNumBytesOnCurrentDelim == 0 )
		return; //Nothing written to next delimiter. Last header is already closed

	assert( myNumOfDelimiters > 0 && "You need to have atleast one delimiter in the message.");

	uint8			header[MSB_PACKET_HEADER_TOTSIZE];
	uint16*			length = (uint16*) header;
	*length = MSB_SWAP_TO_BIG_ENDIAN(myNumBytesOnCurrentDelim);
	
	uint8*			flags = header + MSB_PACKET_HEADER_LENGTH;
	*flags = 0; //Clear all flags
	if(myUseTypeChecking)
		*flags |= MSB_PACKET_FLAGS_TYPECHECKED;
	if(myIsSystemMessage)
		*flags |= MSB_PACKET_FLAGS_SYSTEM;

	*flags |= myNamespace << 2; 

	myStream.OverwriteAtWriteMark(header, MSB_PACKET_HEADER_TOTSIZE);
	myStream.WriteUnmark();

	//Reset for next Delimiter 
	myNumBytesOnCurrentDelim = 0; //So we do not try to close the header twice
	myNumDataBytes = 0;
}

