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
#ifndef MSB_READMESSAGE_H
#define MSB_READMESSAGE_H

#include "MC_BitVector.h"
#include "MC_String.h"
#include "MC_Vector.h"

#include "MSB_IStream.h"
#include "MSB_Types.h"
#include "MSB_Xtea.h"

class MSB_WriteBuffer;

class MSB_ReadMessage {
	public:
							MSB_ReadMessage();
							MSB_ReadMessage(
								MSB_IStream*		aStream);		
							MSB_ReadMessage(
								MSB_ReadMessage&	aReadMessage,
								MSB_IStream*		aStream);		
							~MSB_ReadMessage();

		bool				IsMessageComplete() const;
		bool				DecodeHeader();
		bool				IsSystemMessage() const;
		uint32				GetNamespace(); 
		bool				DecodeDelimiter();
		MSB_DelimiterType	GetDelimiter() const { return myDelimiter; }

		bool				IsTypeChecked() const { return myIsTypeChecked; }

		uint16				GetPacketSize() const { return myPacketSize; }

		MSB_IStream*		GetStream() { return myStream; }
		void				SetStream(
								MSB_IStream*		aStream);

		void				DetachToStream(
								MSB_IStream&		aStream);
		bool				AttachToStream(
								MSB_IStream&		aStream,
								MSB_IStream&		aTempStorage);
		
								

		bool				ReadBool(
								bool&			aBool);

		bool				ReadInt8(
								int8&			anInt8);

		bool				ReadUInt8(
								uint8&			anUInt8);

		bool				ReadInt16(
								int16&			anInt16);

		bool				ReadUInt16(
								uint16&			anUInt16);

		bool				ReadInt32(
								int32&			anInt32);

		bool				ReadUInt32(
								uint32&			anUInt32);

		bool				ReadInt64(
								int64&			anInt64);

		bool				ReadUInt64(
								uint64&			anUInt64);

		bool				ReadFloat32(
								float32&		aFloat32); 

		bool				ReadFloat64(
								float64&		aFloat64);

		bool				ReadString(
								char*			aBuffer, 
								uint16			aBufferSize);

		bool				ReadString(
								MC_String&		aString);

		bool				ReadLocString(
								wchar_t*		aBuffer, 
								uint32			aMaxLength);

		bool				ReadLocString(
								MC_LocString&	aString);

		bool				ReadRawData(
								void*			aData, 
								uint16&			aMaxSize);

		bool				ReadRawDataMaxLength(
								void*			aData, 
								uint16			aMaxSize);

		bool				ReadVector3f32(
								MC_Vector3f&	aVector);

		bool				ReadVector2f32(
								MC_Vector2f&	aVector);

		bool				ReadAnglef32(
								float32&		anAngle);

		bool				ReadBitVector(
								MC_BitVector& 	aBitVector);

		bool				ReadCryptData(
								void*			aTargetData, 
								uint16&			aMaxSize, 
								const MSB_Xtea&	aChiper);

		bool				ReadCryptString(
								char*			aTargetString, 
								uint16			aMaxSize, 
								const MSB_Xtea&	aChiper);

		bool				ReadCryptLocString(
								wchar_t*		aTargetString, 
								uint16			aMaxSize, 
								const MSB_Xtea& aChiper); 

	private:
		class DetachedMessage 
		{
			public:
				uint16				myPacketSize;
				MSB_DelimiterType	myDelimiter;
				bool				myIsTypeChecked;
				bool				myIsSystemMessage;
				uint32				myNamespace;
		};

		MSB_IStream*		myStream;
		uint16				myPacketSize;
		bool				myIsTypeChecked;
		bool				myIsSystemMessage;
		uint32				myNamespace;
		bool				myOwnsStream;
		MSB_DelimiterType	myDelimiter;

		bool				PrivValidateType(
								MSB_TypecheckType	aExpectedType);

		template <typename T>
		bool				PrivReadData(
								T& 					aValue, 
								MSB_TypecheckType 	aTT);
};

#endif /* MSB_READMESSAGE_H */
