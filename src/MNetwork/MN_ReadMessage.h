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
#ifndef MN_READMESSAGE_H
#define MN_READMESSAGE_H

#include "mn_format.h"
#include "mn_message.h"
#include "mc_string.h"
#include "mc_vector.h"

class MC_BitVector;

class MN_ReadMessage : public MN_Message, public MN_ReceiveFormat
{
public:

	//constructor
	MN_ReadMessage(unsigned int theTransportSize = MN_PACKET_DATA_SIZE);

	//destructor
	~MN_ReadMessage();

	bool SetLightTypecheckFlag(bool aFlag) { bool oldState = myLightTypecheckFlag; myLightTypecheckFlag = aFlag; return oldState; }

	bool ReadBool(bool& aBool);
	bool ReadChar(char& aChar);
	bool ReadUChar(unsigned char& anUChar);
	bool ReadShort(short& aShort);
	bool ReadUShort(unsigned short& anUShort);
	bool ReadInt(int& anInt);
	bool ReadUInt(unsigned int& anUInt);
	bool ReadInt64(__int64& anInt);
	bool ReadUInt64(unsigned __int64& anUInt);
	bool ReadFloat(float& aFloat);
	bool ReadString(MC_String& aString);
	bool ReadString(char* aBuffer, unsigned int aBufferSize);
	bool operator>>(MC_String& aString) { return ReadString(aString); }
	bool ReadLocString(MC_LocString& aString);
	bool ReadLocString(wchar_t* aBuffer, unsigned int aMaxBufferLen);
	
	// MAKE SURE YOU HAVE ROOM FOR THE DATA!
	bool ReadRawData( void* someData, int theMaxLength=-1,int* outNumReadBytes=NULL ); // -1 means we can take anything in our buffer

	//optimized reads
	bool ReadPosition2f(MC_Vector2f& aVector);	// Valid range [0 .. MN_POSITION_RANGE]
	bool ReadPosition3f(MC_Vector3f& aVector);	// Valid range [0 .. MN_POSITION_RANGE]
	bool ReadAngle(float& aFloat);				// [-MN_ANGLE_RANGE .. MN_ANGLE_RANGE]
	bool ReadUnitFloat(float& aFloat);			// Valid range [0.0 .. 1.0]

	// Reads a string from the network packet into aString. If aString is NULL or anAllocatememFlag is set,
	// memory is allocated to the string as well. Returns false if the network packet is invalid.
	bool ReadString(char*& aString, bool anAllocateMemFlag = false);
	
	// Returns the size of the string to be read. Not needed to read string, but
	// useful if you want to allocate memory yourself or want to check your current structure.
	// NOTE: After a call to ReadStringSize(), you HAVE to call ReadString() as well (unless
	// ReadStringSize returns 0).
	unsigned short ReadStringSize( void );

	// Reads vecotors from the network packet.
	bool ReadVector2f(MC_Vector2f& aVector);
	bool ReadVector3f(MC_Vector3f& aVector);

	//special, for debugging and network integrity
	//otherwise equivalent to ReadUChar()
	bool ReadDelimiter(MN_DelimiterType& aDelimiter);

	//bit vector
	bool ReadBitVector(MC_BitVector& aBitVector);

	bool AtEnd();

	//MN_ReceiveFormat implementation
	//receive
	//should return the number of bytes used by the subclass implementation, 0 if none used
	unsigned int BuildMe(const void* aBuffer, const unsigned int aBufferLength);

private:
	MN_ReadMessage& operator=(const MN_ReadMessage&) { return *this; }
	//packet operations
	unsigned int ConstructPackets(const unsigned char* someData, const unsigned int aDataLength);
	void AddPacket(MN_Packet* aPacket);

	//get next packet for reading
	//if fails, no more pending packets
	bool NextReadPacket();

	//returns true if typecheck succeeds or if typecheck is disabled
	bool TypeCheck(short aTypeCheckValue);

	//read offset in current packet
	unsigned short myReadOffset;

	// Size of the string being read from the packet
	unsigned short myPendingStringSize;

	unsigned char*	ourUncompressedData;
	unsigned int	myDataPacketSize;
	bool			myLightTypecheckFlag;

/*	//Per thread/stack compressionbuffercaches
	struct locDataPerThreadIdentifier {
		locDataPerThreadIdentifier()
		{
			myData = NULL;
			myDatalen = 0;
		}
		~locDataPerThreadIdentifier()
		{
			free(myData);
		}
		unsigned char*	myData;
		unsigned int	myDatalen;
	};
	*/
	unsigned char* GetDataBuffer(unsigned int theDataSize);

};

#endif