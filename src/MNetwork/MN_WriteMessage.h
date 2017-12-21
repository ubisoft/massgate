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
#ifndef MN_WRITEMESSAGE_H
#define MN_WRITEMESSAGE_H

#include "MC_HybridArray.h"
#include "mn_format.h"
#include "mn_message.h"
#include "mc_vector.h"
#include "mc_string.h"
#include "MN_IWriteableDataStream.h"

class MC_BitVector;

class MN_WriteMessage : public MN_Message, public MN_SendFormat
{
public:

	//constructor
	MN_WriteMessage(unsigned int theTransportSize = MN_PACKET_DATA_SIZE);

	//destructor
	~MN_WriteMessage();

	//write routines for basic data types (can be extended)
	void WriteBool(bool aBool);
	void WriteChar(char aChar);
	void WriteUChar(unsigned char anUChar);
	void WriteShort(short aShort);
	void WriteUShort(unsigned short anUShort);
	void WriteInt(int anInt);
	void WriteUInt(unsigned int anUInt);
	void WriteInt64( const __int64 anInt);
	void WriteUInt64( const unsigned __int64 anUInt);
	void WriteFloat(float aFloat);
	void WriteString(const char* aString);
	void WriteLocString(const MC_LocString& aString);
	template<int S>
	void WriteLocString(const MC_StaticLocString<S>& aString) { WriteLocString(aString.GetBuffer(), aString.GetLength()); }
	void WriteLocString(const wchar_t* aString, unsigned int aStringLen);
	void WriteVector2f(const MC_Vector2f& aVector);
	void WriteVector3f(const MC_Vector3f& aVector);
	void WriteRawData( const void* someData, unsigned short aSize );

	//optimized writes
	void WritePosition2f(const MC_Vector2f& aVector);	// Valid range [0 .. MN_POSITION_RANGE]
	void WritePosition3f(const MC_Vector3f& aVector);	// Valid range [0 .. MN_POSITION_RANGE]
	void WriteAngle(float aFloat);						// Valid range [-MN_ANGLE_RANGE .. MN_ANGLE_RANGE]
	void WriteUnitFloat(float aFloat);					// Valid range [0.0 .. 1.0]

	//special, for debugging and network integrity
	void WriteDelimiter(const MN_DelimiterType& aCmd);

	//bit vector
	void WriteBitVector(const MC_BitVector& aBitVector);

	//append an existing MN_WriteMessage
	void Append(MN_WriteMessage& aMessage);

	//MN_SendFormat implementation
	//send
	MN_ConnectionErrorType SendMe(MN_IWriteableDataStream* aConnection, 
								  bool aDisableCompression = false);

private:

	// MN_WriteMessage& operator=(const MN_WriteMessage&) { return *this; }

	//append support
	void AppendEmpty(MN_WriteMessage& aMessage);
	void AppendNotEmpty(MN_WriteMessage& aMessage);
	bool PacketFitsInCurrentPacket(MN_Packet& aPacket);
	void CopyPacketToCurrentPacket(MN_Packet& aPacket);
	void AppendPacket(MN_Packet& aPacket);

	//create a first packet for writing
	void CreateFirstWritePacket();

	//shorten the current packet at the last NetCmd, stack it, and create a new packet for writing
	void WriteOverflow(unsigned short aLengthToWrite);

	//creates first packet if needed and writes overflow if needed, writes typecheck identifier if ourTypeCheckFlag==true
	void PrepareWrite(unsigned short aLengthToWrite, short aTypeCheckValue);

	// Set internal flags like is compressed or is typechecked
	__forceinline void SetTypecheckFlag(unsigned short& theField, bool aBoolean);
	__forceinline void SetCompressedFlag(unsigned short& theField, bool aBoolean);

	unsigned int myDataPacketSize;

	unsigned char* GetDataBuffer(unsigned int theDataSize);




	class RemoveLastPacketFromPending
	{
	public:
		RemoveLastPacketFromPending(MC_HybridArray<MN_Packet*,8>& whereFrom) : myArray(whereFrom) { };
		~RemoveLastPacketFromPending() { myArray.RemoveLast(); }
	private:
		MC_HybridArray<MN_Packet*,8>& myArray;
	};

};

#endif