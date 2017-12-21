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
#include "MC_Endian.h"

// #include "MC_Logger.h"

#include "MSB_MemoryStream.h"
#include "MSB_ReadMessage.h"
#include "MSB_WriteBuffer.h"
#include "MSB_WriteableMemoryStream.h"

MSB_ReadMessage::MSB_ReadMessage()
	: myStream(NULL)
	, myPacketSize(0)
	, myIsTypeChecked(false)
	, myIsSystemMessage(false)
	, myNamespace(0)
	, myOwnsStream(false)
	, myDelimiter(0)
{
}

/**
 * 
 */
MSB_ReadMessage::MSB_ReadMessage(
	MSB_IStream*		aStream)
	: myStream(aStream)
	, myPacketSize(0)
	, myIsTypeChecked(false)
	, myIsSystemMessage(false)
	, myNamespace(0)
	, myOwnsStream(false)
	, myDelimiter(0)
{
}

MSB_ReadMessage::MSB_ReadMessage( 
	MSB_ReadMessage&	aReadMessage, 
	MSB_IStream*		aStream )
	: myStream(aStream)
	, myPacketSize(aStream->GetUsed())
	, myIsTypeChecked(aReadMessage.myIsTypeChecked)
	, myIsSystemMessage(aReadMessage.myIsSystemMessage)
	, myNamespace(aReadMessage.myNamespace)
	, myOwnsStream(false)
	, myDelimiter(aReadMessage.myDelimiter)
{
}

MSB_ReadMessage::~MSB_ReadMessage()
{
	if(myOwnsStream)
		delete myStream;
}

/**
 * Checks if enough data has arrived to decode the packet.
 *
 * @returns True if the packet is complete, false otherwise.
 */
bool
MSB_ReadMessage::IsMessageComplete() const
{
	if(myStream->GetUsed() < MSB_MIN_VALID_PACKET_SIZE)
		return false;

	uint16			size;
	myStream->Peek((uint8*) &size, MSB_PACKET_HEADER_LENGTH);
	size = MSB_SWAP_TO_NATIVE(size);

	// We need data for length + flags + actual packet 
	if(myStream->GetUsed() - MSB_PACKET_HEADER_TOTSIZE < size)
		return false;
	
	return true;
}

/**
 * Checks if the system is a system-message. This function assumes the packet
 * is complete and has been decoded.
 *
 * @return True if the message is a system message, false otherwise.
 */
bool
MSB_ReadMessage::IsSystemMessage() const
{
	return myIsSystemMessage;
}

uint32
MSB_ReadMessage::GetNamespace()
{
	return myNamespace; 
}

/**
 * Writes data needed for delayed processing to the stream.
 *
 * Call AttachToStream to re-read the message.
 *
 */
void
MSB_ReadMessage::DetachToStream(
	MSB_IStream&			aStream)
{
	assert(myPacketSize >= sizeof(myDelimiter));

	DetachedMessage			msg;
	msg.myPacketSize = myPacketSize;
	msg.myDelimiter = myDelimiter;
	msg.myIsTypeChecked = myIsTypeChecked;
	msg.myIsSystemMessage = myIsSystemMessage;
	msg.myNamespace = myNamespace; 

	aStream.Write(&msg, sizeof(msg));
	
	uint32			size = myPacketSize - sizeof(myDelimiter);
	uint8			buffer[1024];
	while(size)
	{
		uint32		blockSize = __min(size, sizeof(buffer));
		myStream->Read(buffer, blockSize);
		aStream.Write(buffer, blockSize);
		size -= blockSize;
	}
}

/**
 * Reads a previously detached message. Note that the data is copied to
 * aTempStorage so aDataStream is freed.
 */
bool
MSB_ReadMessage::AttachToStream(
	MSB_IStream&		aDataStream,
	MSB_IStream&		aTempStream)
{
	DetachedMessage		msg;
	if(aDataStream.GetUsed() < sizeof(msg))
		return false;

	aDataStream.Peek((uint8*)&msg, sizeof(msg));
	
	if(aDataStream.GetUsed() < sizeof(msg) + msg.myPacketSize - sizeof(myDelimiter))
		return false;

	// First time we just peeked the data
	aDataStream.Read((uint8*)&msg, sizeof(msg));

	myPacketSize = msg.myPacketSize;
	myDelimiter = msg.myDelimiter;
	myIsSystemMessage = msg.myIsSystemMessage;
	myNamespace = msg.myNamespace; 
	myIsTypeChecked = msg.myIsTypeChecked;

	// Delimiter has already been read
	uint32				size = myPacketSize - sizeof(MSB_DelimiterType);
	uint8				buffer[1024];
	while(size > 0)
	{
		uint32			blockSize = __min(size, aDataStream.GetUsed());
		blockSize = __min(blockSize, sizeof(buffer));

		aDataStream.Read(buffer, blockSize);
		aTempStream.Write(buffer, blockSize);
		size -= blockSize;
	}

	myOwnsStream = false;
	myStream = &aTempStream;

	return true;
}

void
MSB_ReadMessage::SetStream(
	MSB_IStream*		aStream)
{
	myPacketSize = 0;
	myIsTypeChecked = false;
	myIsSystemMessage = false;
	myNamespace = 0; 
	myOwnsStream = false;

	myStream = aStream;
}

/**
 * Starts the decoding of the packets.
 *
 * Decoding a packet means reading the header and delimiter. This function 
 * assumes that all data has arrived. Call IsMessageComplete() to ensure
 * that the packet is complete.
 *
 * @returns true if the headers was valid, false otherwise.
 */
bool
MSB_ReadMessage::DecodeHeader()
{
	myStream->Read((uint8*) &myPacketSize, MSB_PACKET_HEADER_LENGTH);
	myPacketSize = MSB_SWAP_TO_NATIVE(myPacketSize);
	
	if ( myPacketSize < sizeof(myDelimiter) )
		return false;

	uint8		flags;
	myStream->Read(&flags, MSB_PACKET_HEADER_FLAGS);

	myNamespace = (flags & 0xFC) >> 2; 

	myIsTypeChecked = (flags & 0x03 & MSB_PACKET_FLAGS_TYPECHECKED) != 0;
	myIsSystemMessage = (flags & 0x03 & MSB_PACKET_FLAGS_SYSTEM) != 0;

	if(!PrivValidateType(MSB_TT_DELIMITER))
		return false;

	return true;
}


bool
MSB_ReadMessage::DecodeDelimiter()
{
	if(myStream->Read((uint8*) &myDelimiter, sizeof(MSB_DelimiterType)) != sizeof(MSB_DelimiterType))
		return false;

	myDelimiter = MSB_SWAP_TO_NATIVE(myDelimiter);
	return true;
}

bool
MSB_ReadMessage::ReadBool(
	bool&			aBool)
{
	uint8			v = aBool;
	bool			valid = PrivReadData<uint8>(v, MSB_TT_BOOL);
	aBool = v == 1;
	return valid;
}

bool
MSB_ReadMessage::ReadInt8(
	int8&			anInt8)
{
	return PrivReadData<int8>(anInt8, MSB_TT_INT8);
}

bool
MSB_ReadMessage::ReadUInt8(
	uint8&			anUInt8)
{
	return PrivReadData<uint8>(anUInt8, MSB_TT_UINT8);
}

bool
MSB_ReadMessage::ReadInt16(
	int16&			anInt16)
{
	return PrivReadData<int16>(anInt16, MSB_TT_INT16);
}

bool
MSB_ReadMessage::ReadUInt16(
	uint16&			anUInt16)
{
	return PrivReadData<uint16>(anUInt16, MSB_TT_UINT16);
}

bool
MSB_ReadMessage::ReadInt32(
	int32&			anInt32)
{
	return PrivReadData<int32>(anInt32, MSB_TT_INT32);
}

bool
MSB_ReadMessage::ReadUInt32(
	uint32&			anUInt32)
{
	return PrivReadData<uint32>(anUInt32, MSB_TT_UINT32);
}

bool
MSB_ReadMessage::ReadInt64(
	int64&			anInt64)
{
	return PrivReadData<int64>(anInt64, MSB_TT_INT64);
}

bool
MSB_ReadMessage::ReadUInt64(
	uint64&			anUInt64)
{
	return PrivReadData<uint64>(anUInt64, MSB_TT_UINT64);
}

bool
MSB_ReadMessage::ReadFloat32(
	float32&		aFloat32)
{
	uint32			value;
	if(!PrivReadData<uint32>(value, MSB_TT_FLOAT32))
		return false;

	*(float32*) &aFloat32 = *(float32*) &value;
	return true;
}

bool
MSB_ReadMessage::ReadFloat64(
	float64&		aFloat64)
{
	return PrivReadData<float64>(aFloat64, MSB_TT_FLOAT64);
}

bool
MSB_ReadMessage::ReadString(
	 char*			aBuffer, 
	 uint16			aBufferSize)
{
	uint16			len;
	if(!PrivReadData<uint16>(len, MSB_TT_STRING))
		return false;

	if(len > aBufferSize)
		return false;

	if(myStream->Read(aBuffer, len) != len)
		return false;

	aBuffer[len] = 0;

	return true;
}

bool				
MSB_ReadMessage::ReadString(
	MC_String&		aString)
{
	uint16			len;
	if(!PrivReadData<uint16>(len, MSB_TT_STRING))
		return false;

	aString.Reserve(len);
	if(myStream->Read(aString.GetBuffer(), len) != len)
		return false;

	aString.GetBuffer()[len] = 0;

	return true;
}

bool
MSB_ReadMessage::ReadLocString(
	wchar_t*		aBuffer, 
	uint32			aMaxBufferLen)
{
	uint16			len;
	if(!PrivReadData<uint16>(len, MSB_TT_LOCSTRING))
		return false;

	if(len > aMaxBufferLen)
		return false;

	if(myStream->Read(aBuffer, len * sizeof(wchar_t)) != (len * sizeof(wchar_t)))
		return false;

	aBuffer[len] = 0;

	return true;
}

bool				
MSB_ReadMessage::ReadLocString(
	MC_LocString&		aString)
{
	uint16			len;
	if(!PrivReadData<uint16>(len, MSB_TT_LOCSTRING))
		return false;

	aString.Reserve(len+1);
	if(myStream->Read(aString.GetBuffer(), len * sizeof(wchar_t)) != (len * sizeof(wchar_t)))
		return false;

	aString.GetBuffer()[len] = 0;

	return true;
}

bool
MSB_ReadMessage::ReadRawData(
	 void*			aData, 
	 uint16&		aMaxSize)
{
	uint16			size;
	if(!PrivReadData<uint16>(size, MSB_TT_RAWDATA))
		return false;
	
	if ( aMaxSize < size )
		return false;
	
	if( myStream->Read(aData, size) != size)
		return false;

	aMaxSize = size;
	return true;
}

bool
MSB_ReadMessage::ReadRawDataMaxLength(
	 void*			aData, 
	 uint16			aMaxSize)
{
	uint16			size;
	if(!ReadUInt16(size))
		return false;

	if ( aMaxSize < size )
		return false;
	
	if( myStream->Read(aData, size) != size)
		return false;

	return true;
}

bool
MSB_ReadMessage::ReadVector3f32(
	MC_Vector3f&	aVector)
{
	if(!PrivValidateType(MSB_TT_VECTOR3F32))
		return false;

	if(!ReadFloat32(aVector.x))
		return false;
	if(!ReadFloat32(aVector.y))
		return false;
	if(!ReadFloat32(aVector.z))
		return false;

	return true;
}

bool
MSB_ReadMessage::ReadVector2f32(
	MC_Vector2f&	aVector)
{
	if(!PrivValidateType(MSB_TT_VECTOR2F32))
		return false;

	if(!ReadFloat32(aVector.x))
		return false;
	if(!ReadFloat32(aVector.y))
		return false;
	
	return true;
}

bool
MSB_ReadMessage::ReadAnglef32(
	float32&		anAngle)
{
	if(!PrivValidateType(MSB_TT_ANGLE32))
		return false;

	if(!ReadFloat32(anAngle))
		return false;

	return true;
}

bool				
MSB_ReadMessage::ReadBitVector(
	MC_BitVector& aBitVector)
{
	uint16		len;
	if(!PrivReadData<uint16>(len, MSB_TT_BITVECTOR))
		return false;
	
	aBitVector.Init(len * 8); // ??
	if(myStream->Read(aBitVector.GetData(), len) != len)
		return false;

	return true;
}

bool
MSB_ReadMessage::ReadCryptData(
	void*			aTargetData, 
	uint16&			aMaxSize, 
	const MSB_Xtea&	aChiper)
{
	if(!PrivValidateType(MSB_TT_CRYPTDATA))
		return false;
	
	MSB_Xtea::ClearText clearText;
	MSB_Xtea::CryptText cryptText; 

	// cookie 
	uint64 cookie = 0; 
	uint64 cryptCookie = 0; 

	if(!ReadUInt64(cryptCookie))
		return false; 

	cryptText.myCryptLen = sizeof(uint64);
	cryptText.myPadLen = 0; 
	cryptText.myCryptText = (uint8*) &cryptCookie; 

	clearText.myClearLen = sizeof(uint64); 
	clearText.myClearText = (uint8*) &cookie; 

	aChiper.Decrypt(cryptText, clearText);

	if(cookie != 0x1EE7DEADBEEFBABELL)
		return false; 

	// actual message 
	uint16 t; 
	if(!ReadUInt16(t))
		return false; 
	cryptText.myCryptLen = t;

	if(!ReadUInt16(t))
		return false; 
	cryptText.myPadLen = (uint8) t; 

	if((cryptText.myCryptLen - cryptText.myPadLen) > aMaxSize)
		return false; 

	uint16 cryptTextLen = cryptText.myCryptLen; // - cryptText.myPadLen; 

	cryptText.myCryptText = (uint8*) alloca(cryptTextLen); 

	if(!ReadRawData(cryptText.myCryptText, cryptTextLen))
		return false; 

	clearText.myClearLen = cryptText.myCryptLen; 
	clearText.myClearText = (uint8*) alloca(cryptText.myCryptLen); // temp storage

	aChiper.Decrypt(cryptText, clearText);

	memcpy(aTargetData, clearText.myClearText, clearText.myClearLen);

	aMaxSize = clearText.myClearLen; 

	return true; 
}

bool
MSB_ReadMessage::ReadCryptString(
	char*			aTargetString, 
	uint16			aMaxSize, 
	const MSB_Xtea&	aChiper)
{
	if(!PrivValidateType(MSB_TT_CRYPTSTRING))
		return false;

	uint16 len = aMaxSize - 1; 
	if(!ReadCryptData((void*)aTargetString, len, aChiper))
		return false; 
	aTargetString[len] = 0;
	return true; 
}

bool
MSB_ReadMessage::ReadCryptLocString(
	wchar_t*		aTargetString, 
	uint16			aMaxSize, 
	const MSB_Xtea& aChiper)
{
	if(!PrivValidateType(MSB_TT_CRYPTLOCSTRING))
		return false;

	uint16 len = (aMaxSize - 1) * sizeof(wchar_t); 
	if(!ReadCryptData((void*)aTargetString, len, aChiper))
		return false; 
	len /= sizeof(wchar_t); 
	aTargetString[len] = 0;
	return true; 
}

/**
* Validates the current type. If type checking is turned off it does
* nothing.
*
* @returns true if the expected type matches the actual type, false otherwise.
*/		
bool
MSB_ReadMessage::PrivValidateType(
	MSB_TypecheckType	aExpectedType)
{
	if(!myIsTypeChecked)
		return true;

	uint8		tt;
	if(myStream->Read(&tt, 1) != 1)
		return false;

	return tt == aExpectedType;
}

template <typename T>
bool
MSB_ReadMessage::PrivReadData(T& aValue, MSB_TypecheckType aTT)
{
	if(!PrivValidateType(aTT))
		return false;

	T	temp;
	if(myStream->Read((uint8*)&temp, sizeof(temp)) != sizeof(temp))
		return false;

	aValue = MSB_SWAP_TO_NATIVE(temp);

	return true;
}

