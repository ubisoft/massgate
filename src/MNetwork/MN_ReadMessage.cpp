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
#include "mn_readmessage.h"

#include "mc_bitvector.h"

#include "MC_Debug.h"
#include "mc_commandline.h"

#include "MP_Pack.h"
#include "MT_ThreadingTools.h"

#include "MC_Misc.h"

static const unsigned int LOC_BUFFER_MULTIPLIER = 10;

__declspec(thread) static unsigned int LocUncompressedDataBufferSize = 0;
__declspec(thread) static unsigned char* LocUncompressedDataBuffer = NULL;

// yeah, only delete the first thread. This is so wic.exe memleak detection won't register this as a leak
static struct locDataBufferRemover { locDataBufferRemover() { } ~locDataBufferRemover() { delete [] LocUncompressedDataBuffer; } } dataRemover;

template<typename T>
static inline void CopyDataFromAnyAddress(T& aDest, const void* aSource)
{
	aDest = *(const T*)aSource;
}

unsigned char*
MN_ReadMessage::GetDataBuffer(unsigned int theDataSize)
{
	if (theDataSize <= LocUncompressedDataBufferSize)
		return LocUncompressedDataBuffer;

	if (LocUncompressedDataBuffer == NULL)
	{
		LocUncompressedDataBuffer = new unsigned char[theDataSize];
		LocUncompressedDataBufferSize = theDataSize;
	}
	else if (theDataSize > LocUncompressedDataBufferSize)
	{
		unsigned char* tmp = new unsigned char[theDataSize];
		memcpy(tmp, LocUncompressedDataBuffer, LocUncompressedDataBufferSize);
		delete [] LocUncompressedDataBuffer;
		LocUncompressedDataBuffer = tmp;
		LocUncompressedDataBufferSize = theDataSize;
	}
	return LocUncompressedDataBuffer;
}


//constructor
MN_ReadMessage::MN_ReadMessage(unsigned int theTransportSize)
:MN_Message()
{
	myReadOffset = 0;
	myPendingStringSize = 0;
	ourUncompressedData = GetDataBuffer(theTransportSize * LOC_BUFFER_MULTIPLIER);
	myDataPacketSize = theTransportSize;
	myLightTypecheckFlag = false;
}


//destructor
MN_ReadMessage::~MN_ReadMessage()
{
	const unsigned int count = myPendingPackets.Count();
	for (unsigned int i=0; i < count; i++)
		MN_Packet::Deallocate(myPendingPackets[i]);
	myPendingPackets.RemoveAll();
}

//read routines for basic data types (can be extended)
bool MN_ReadMessage::ReadBool(bool& aBool)
{
	if(!myCurrentPacket)
		if(!NextReadPacket())
			return false;

	if(!TypeCheck('BL'))
		return false;

	bool retVal = false;
	if((myReadOffset + sizeof(char)) <= myCurrentPacket->GetWriteOffset())
	{
		//read
		char		tmp = *((char*)(myCurrentPacket->GetData() + myReadOffset));
		myReadOffset += sizeof(char);

		if(tmp == 0)
		{
			aBool = false;
			retVal = true;
		}
		else if(tmp == 1)
		{
			aBool = true;
			retVal = true;
		}
	}

	//handle potential overflow
	if(myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();

	return retVal;
}

bool MN_ReadMessage::ReadChar(char& aChar)
{
	bool retVal = false;
	
	//if first read
	if(!myCurrentPacket)
		if(!NextReadPacket())
			return false;
	
	if(!TypeCheck('CH'))
		return false;
	
	if((myReadOffset + sizeof(char)) <= myCurrentPacket->GetWriteOffset())
	{
		//read
		aChar = *((char*)(myCurrentPacket->GetData() + myReadOffset));
		myReadOffset += sizeof(char);

		retVal = true;
	}

	//handle potential overflow
	if(myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();

	return retVal;
}


bool MN_ReadMessage::ReadUChar(unsigned char& anUChar)
{
	bool retVal = false;
	
	//if first read
	if(!myCurrentPacket)
		if(!NextReadPacket())
			return false;
	
	if(!TypeCheck('UC'))
		return false;

	if((myReadOffset + sizeof(unsigned char)) <= myCurrentPacket->GetWriteOffset())
	{
		//read
		anUChar = *((unsigned char*)(myCurrentPacket->GetData() + myReadOffset));
		myReadOffset += sizeof(unsigned char);

		retVal = true;
	}

	//handle potential overflow
	if(myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();

	return retVal;
}


bool MN_ReadMessage::ReadShort(short& aShort)
{
	bool retVal = false;
	
	//if first read
	if(!myCurrentPacket)
		if(!NextReadPacket())
			return false;

	if(!TypeCheck('SH'))
		return false;

	if((myReadOffset + sizeof(short)) <= myCurrentPacket->GetWriteOffset())
	{
		//read
		aShort = *((short*)(myCurrentPacket->GetData() + myReadOffset));
		myReadOffset += sizeof(short);

		retVal = true;
	}

	//handle potential overflow
	if(myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();

	return retVal;
}


bool MN_ReadMessage::ReadUShort(unsigned short& anUShort)
{
	bool retVal = false;
	
	//if first read
	if(!myCurrentPacket)
		if(!NextReadPacket())
			return false;
	
	if(!TypeCheck('US'))
		return false;
	
	if((myReadOffset + sizeof(unsigned short)) <= myCurrentPacket->GetWriteOffset())
	{
		//read
		anUShort = *((unsigned short*)(myCurrentPacket->GetData() + myReadOffset));
		myReadOffset += sizeof(unsigned short);

		retVal = true;
	}

	//handle potential overflow
	if(myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();

	return retVal;
}


bool MN_ReadMessage::ReadInt(int& anInt)
{
	bool retVal = false;
	
	//if first read
	if(!myCurrentPacket)
		if(!NextReadPacket())
			return false;
	
	if(!TypeCheck('IN'))
		return false;
	
	if((myReadOffset + sizeof(int)) <= myCurrentPacket->GetWriteOffset())
	{
		//read
		anInt = *((int*)(myCurrentPacket->GetData() + myReadOffset));
		myReadOffset += sizeof(int);

		retVal = true;
	}

	//handle potential overflow
	if(myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();

	return retVal;
}


bool MN_ReadMessage::ReadUInt(unsigned int& anUInt)
{
	bool retVal = false;
	
	//if first read
	if(!myCurrentPacket)
		if(!NextReadPacket())
			return false;
	
	if(!TypeCheck('UI'))
		return false;
	
	if((myReadOffset + sizeof(unsigned int)) <= myCurrentPacket->GetWriteOffset())
	{
		//read
		anUInt = *((unsigned int*)(myCurrentPacket->GetData() + myReadOffset));
		myReadOffset += sizeof(unsigned int);

		retVal = true;
	}

	//handle potential overflow
	if(myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();

	return retVal;
}

bool MN_ReadMessage::ReadInt64(__int64& anInt)
{
	bool retVal = false;
	
	//if first read
	if(!myCurrentPacket)
		if(!NextReadPacket())
			return false;
	
	if(!TypeCheck('I6'))
		return false;
	
	if((myReadOffset + sizeof(__int64)) <= myCurrentPacket->GetWriteOffset())
	{
		//read
		anInt = *((__int64*)(myCurrentPacket->GetData() + myReadOffset));
		myReadOffset += sizeof(__int64);

		retVal = true;
	}

	//handle potential overflow
	if(myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();

	return retVal;
}

bool MN_ReadMessage::ReadUInt64( unsigned __int64& anInt)
{
	bool retVal = false;
	
	//if first read
	if(!myCurrentPacket)
		if(!NextReadPacket())
			return false;
	
	if(!TypeCheck('U6'))
		return false;
	
	if((myReadOffset + sizeof(unsigned __int64)) <= myCurrentPacket->GetWriteOffset())
	{
		//read
		anInt = *((unsigned __int64*)(myCurrentPacket->GetData() + myReadOffset));
		myReadOffset += sizeof(unsigned __int64);

		retVal = true;
	}

	//handle potential overflow
	if(myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();

	return retVal;
}

bool MN_ReadMessage::ReadFloat(float& aFloat)
{
	bool retVal = false;
	
	//if first read
	if(!myCurrentPacket)
		if(!NextReadPacket())
			return false;
	
	if(!TypeCheck('FL'))
		return false;
	
	if((myReadOffset + sizeof(float)) <= myCurrentPacket->GetWriteOffset())
	{
		CopyDataFromAnyAddress(aFloat, myCurrentPacket->GetData() + myReadOffset);
		myReadOffset += sizeof(float);

		retVal = true;
	}

	//handle potential overflow
	if(myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();

	if (!MC_Misc::SanityCheckFloat(aFloat))
		return false;

	return retVal;
}

unsigned short MN_ReadMessage::ReadStringSize( void )
{
	//if first read
	if(!myCurrentPacket)
	{
		if(!NextReadPacket())
		{
			myPendingStringSize = 0;
			return 0;
		}
	}

	assert( myCurrentPacket );

	if (myReadOffset + sizeof(myPendingStringSize) <= myCurrentPacket->GetWriteOffset())
		memcpy( &myPendingStringSize, &myCurrentPacket->GetData()[myReadOffset], sizeof(unsigned short) );
	else
		return 0;
	myReadOffset += sizeof(unsigned short);

	return myPendingStringSize;
}

bool MN_ReadMessage::ReadString(char*& aString, bool anAllocateMemFlag)
{
	if( myPendingStringSize == 0 )
		ReadStringSize();
	if (myPendingStringSize == 0) // ReadStringSize() failed
		return false;
	if( anAllocateMemFlag || !aString )
	{
		if( aString )
			delete [] aString;
		aString = new char[myPendingStringSize];
	}

#ifdef _DEBUG
	assert(_msize(aString) >= myPendingStringSize);
#endif // _DEBUG

	if (myReadOffset + myPendingStringSize > myCurrentPacket->GetWriteOffset())
		return false;

	if( aString )
	{
		memcpy( aString, &myCurrentPacket->GetData()[myReadOffset], myPendingStringSize);
		aString[myPendingStringSize-1] = 0;
	}
	myReadOffset += myPendingStringSize;
	myPendingStringSize = 0;

	// Read next chunk of current packet at end.
	if(myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();
	return true;
}


//if this function fails, the string is empty
bool MN_ReadMessage::ReadString(MC_String& aString)
{
	if( myPendingStringSize == 0 )
		ReadStringSize();
	if (myPendingStringSize == 0) // ReadStringSize() failed
	{
		aString = "";
		return false;
	}

	if (myReadOffset + myPendingStringSize > myCurrentPacket->GetWriteOffset())
		return false;
	
	aString = (const TCHAR*)(&myCurrentPacket->GetData()[myReadOffset]);
	myReadOffset += myPendingStringSize;
	myPendingStringSize = 0;

	// Read next chunk of current packet at end.
	if(myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();
	return true;
}

bool MN_ReadMessage::ReadString(char* aBuffer, unsigned int aBufferSize)
{
	if( myPendingStringSize == 0 )
		ReadStringSize();
	if ((myPendingStringSize == 0) || (myPendingStringSize > aBufferSize))
	{
		*aBuffer = 0;
		return false;
	}

	if (myReadOffset + myPendingStringSize > myCurrentPacket->GetWriteOffset())
		return false;

	memcpy(aBuffer, &myCurrentPacket->GetData()[myReadOffset], myPendingStringSize);
	aBuffer[myPendingStringSize-1] = 0;
	myReadOffset += myPendingStringSize;
	myPendingStringSize = 0;

	// Read next chunk of current packet at end.
	if(myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();
	return true;
}


//if this function fails, the string is empty
bool MN_ReadMessage::ReadLocString(MC_LocString& aString)
{
	if( myPendingStringSize == 0 )
		ReadStringSize();
	if (myPendingStringSize == 0) // ReadStringSize() failed
	{
		aString = L"";
		return false;
	}	
	if (myReadOffset + myPendingStringSize * sizeof(MC_LocChar) > myCurrentPacket->GetWriteOffset())
		return false;

	aString = (MC_LocChar*) &myCurrentPacket->GetData()[myReadOffset];
	myReadOffset += myPendingStringSize * sizeof(MC_LocChar);
	myPendingStringSize = 0;

	// Read next chunk of current packet at end.
	if(myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();
	return true;
}

bool MN_ReadMessage::ReadLocString(wchar_t* aBuffer, unsigned int aMaxBufferLen)
{
	assert(aMaxBufferLen);
	if (myPendingStringSize == 0)
		ReadStringSize();
	if (myPendingStringSize == 0)
		return false;

	if (myReadOffset + myPendingStringSize * sizeof(MC_LocChar) > myCurrentPacket->GetWriteOffset())
		return false;

	if (aMaxBufferLen < myPendingStringSize)
		return false;

	memcpy(aBuffer, &myCurrentPacket->GetData()[myReadOffset], myPendingStringSize * sizeof(MC_LocChar));
	aBuffer[myPendingStringSize-1] = 0;
	myReadOffset += myPendingStringSize * sizeof(MC_LocChar);
	myPendingStringSize = 0;

	if (myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();
	return true;
}


//if this function fails, the string is empty
bool MN_ReadMessage::ReadRawData( void* someData, int theMaxLen, int* outNumReadBytes)
{
	unsigned short dataSize;
	//if first read
	if(!myCurrentPacket)
		NextReadPacket();

	assert( someData );
	assert( myCurrentPacket );

	if(!TypeCheck('RD'))
		return false;

	if (myReadOffset + sizeof(unsigned short) > myCurrentPacket->GetWriteOffset())
		return false;

	memcpy( &dataSize, &myCurrentPacket->GetData()[myReadOffset], sizeof(unsigned short) );
	myReadOffset += sizeof(unsigned short);

	if ((theMaxLen != -1) && (dataSize > theMaxLen))
		return false;

	if (myReadOffset + dataSize > myCurrentPacket->GetWriteOffset())
		return false;

	if( someData )
		memcpy( someData, &myCurrentPacket->GetData()[myReadOffset], dataSize);

	if (outNumReadBytes)
		*outNumReadBytes = dataSize;

	myReadOffset += dataSize;

	// Read next chunk of current packet at end.
	if(myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();

	return true;
}

bool MN_ReadMessage::ReadVector2f(MC_Vector2f& aVector)
{
	bool retVal = false;

	//if first read
	if(!myCurrentPacket)
		NextReadPacket();

	if(!TypeCheck('V2'))
		return false;
	
	if((myReadOffset + sizeof(MC_Vector2f)) <= myCurrentPacket->GetWriteOffset())
	{
		CopyDataFromAnyAddress(aVector, myCurrentPacket->GetData() + myReadOffset);
		myReadOffset += sizeof(MC_Vector2f);

		retVal = true;
	}

	//handle potential overflow
	if(myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();

	return retVal;
}

bool MN_ReadMessage::ReadVector3f(MC_Vector3f& aVector)
{
	bool retVal = false;

	//if first read
	if(!myCurrentPacket)
		NextReadPacket();

	if(!TypeCheck('V3'))
		return false;
	
	if((myReadOffset + sizeof(MC_Vector3f)) <= myCurrentPacket->GetWriteOffset())
	{
		CopyDataFromAnyAddress(aVector, myCurrentPacket->GetData() + myReadOffset);
		myReadOffset += sizeof(MC_Vector3f);

		retVal = true;
	}

	//next chunk
	if(myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();

	return retVal;
}

// Valid range [0 .. MN_POSITION_RANGE]
bool MN_ReadMessage::ReadPosition2f(MC_Vector2f& aVector)
{
	bool retVal = false;

	//if first read
	if(!myCurrentPacket)
		NextReadPacket();

	if(!TypeCheck('P2'))
		return false;
	
	if((myReadOffset + sizeof(unsigned short) * 2) <= myCurrentPacket->GetWriteOffset())
	{
		unsigned short* s = (unsigned short*)(myCurrentPacket->GetData() + myReadOffset);

		//read
		aVector.x = float(s[0]) * (MN_POSITION_RANGE / 65535.0f);
		aVector.y = float(s[1]) * (MN_POSITION_RANGE / 65535.0f);
		myReadOffset += sizeof(unsigned short) * 2;

		retVal = true;
	}

	//next chunk
	if(myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();

	return retVal;
}

// Valid range [0 .. MN_POSITION_RANGE]
bool MN_ReadMessage::ReadPosition3f(MC_Vector3f& aVector)
{
	bool retVal = false;

	//if first read
	if(!myCurrentPacket)
		NextReadPacket();

	if(!TypeCheck('P3'))
		return false;
	
	if((myReadOffset + sizeof(unsigned short) * 3) <= myCurrentPacket->GetWriteOffset())
	{
		unsigned short* s = (unsigned short*)(myCurrentPacket->GetData() + myReadOffset);

		//read
		aVector.x = float(s[0]) * (MN_POSITION_RANGE / 65535.0f);
		aVector.y = float(s[1]) * (MN_POSITION_RANGE / 65535.0f);
		aVector.z = float(s[2]) * (MN_POSITION_RANGE / 65535.0f);
		myReadOffset += sizeof(unsigned short) * 3;

		retVal = true;
	}

	//next chunk
	if(myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();

	return retVal;
}

// [-MN_ANGLE_RANGE .. MN_ANGLE_RANGE]
bool MN_ReadMessage::ReadAngle(float& aFloat)
{
	bool retVal = false;

	//if first read
	if(!myCurrentPacket)
		NextReadPacket();

	if(!TypeCheck('AN'))
		return false;
	
	if((myReadOffset + sizeof(unsigned short)) <= myCurrentPacket->GetWriteOffset())
	{
		unsigned short* s = (unsigned short*)(myCurrentPacket->GetData() + myReadOffset);

		//read
		aFloat = float(s[0]) * ((MN_ANGLE_RANGE*2) / 65535.0f) - MN_ANGLE_RANGE;
		myReadOffset += sizeof(unsigned short);

		retVal = true;
	}

	//next chunk
	if(myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();

	return retVal;
}

// Valid range [0.0 .. 1.0]
bool MN_ReadMessage::ReadUnitFloat(float& aFloat)
{
	bool retVal = false;

	//if first read
	if(!myCurrentPacket)
		NextReadPacket();

	if(!TypeCheck('UF'))
		return false;
	
	if((myReadOffset + sizeof(unsigned short)) <= myCurrentPacket->GetWriteOffset())
	{
		unsigned short* s = (unsigned short*)(myCurrentPacket->GetData() + myReadOffset);

		//read
		aFloat = float(s[0]) * (1.0f / 65535.0f);
		myReadOffset += sizeof(unsigned short);

		retVal = true;
	}

	//next chunk
	if(myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();

	return retVal;
}

//special, for debugging and network integrity
//otherwise equivalent to ReadUChar()
bool MN_ReadMessage::ReadDelimiter(MN_DelimiterType& aDelimiter)
{
	bool retVal = false;

	//if first read
	if(!myCurrentPacket)
		NextReadPacket();

	if(!TypeCheck('DL'))
		return false;
	
	if((myReadOffset + sizeof(MN_DelimiterType)) <= myCurrentPacket->GetWriteOffset())
	{
		//read
		myLastDelimiter = aDelimiter = *((MN_DelimiterType*)(myCurrentPacket->GetData() + myReadOffset));
//		assert(aDelimiter >= 0 && aNetCmd < NETCMD_NUMCOMMANDS);
		myReadOffset += sizeof(MN_DelimiterType);

		retVal = true;
	}

	//next chunk
	if(myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();

	return retVal;
}


//bit vector
bool MN_ReadMessage::ReadBitVector(MC_BitVector& aBitVector)
{
	bool retVal = false;
	unsigned short dataLength;
	
	//if first read
	if(!myCurrentPacket)
		NextReadPacket();

	if(!TypeCheck('BV'))
		return false;

	if((myReadOffset + sizeof(unsigned short)) <= myCurrentPacket->GetWriteOffset())
	{
		//read length
		dataLength = *((unsigned short*)(myCurrentPacket->GetData() + myReadOffset));
		myReadOffset += sizeof(unsigned short);

		if((myReadOffset + dataLength) <= myCurrentPacket->GetWriteOffset())
		{
			memcpy( aBitVector.GetData(), myCurrentPacket->GetData() + myReadOffset, dataLength );
			myReadOffset += dataLength;
			retVal = true;
		}
	}

	//next chunk
	if(myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();

	return retVal;
}


//get next chunk for reading
//if fails, no more pending chunks
bool MN_ReadMessage::NextReadPacket()
{
	if(myPendingPackets.Count())
	{
		//delete possible current chunk
		if(myCurrentPacket)
		{
			assert(myReadOffset >= myCurrentPacket->GetWriteOffset());
			MN_Packet::Deallocate(myCurrentPacket);
			myCurrentPacket = NULL;
		}
		
		//set new current chunk
		myCurrentPacket = myPendingPackets[0]; // Todo - ReadMessage maybe should have a reverse sorted pendingpackets instead

		//check stuff
		assert(myCurrentPacket->GetWriteOffset()); // EXAMINE BUG: This assertion was triggered. once.

		myPendingPackets.RemoveAtIndex(0);

		//zero readoffset
		myReadOffset = 0;

		return true;
	}
	else
		return false;
}

bool MN_ReadMessage::TypeCheck(short aTypeCheckValue)
{
	if(ourTypeCheckFlag)
	{
		//if first read
		if(!myCurrentPacket)
			if(!NextReadPacket())
				return true;

		short readValue;
		if((myReadOffset + sizeof(short)) <= myCurrentPacket->GetWriteOffset())
		{
			//read
			readValue = *((short*)(myCurrentPacket->GetData() + myReadOffset));
			myReadOffset += sizeof(short);
		}
		else
		{
			MC_DEBUG("Typecheck failed. Data type should have been %c%c but there was no data available. Last delimiter was %d",
				char(aTypeCheckValue>>8), char(aTypeCheckValue),
				int(myLastDelimiter));

			if(!myLightTypecheckFlag)
				assert(0 && "Typecheck failed, no data");

			return false;
		}

		//handle potential overflow
		if(myReadOffset >= myCurrentPacket->GetWriteOffset())
			NextReadPacket();

		if(readValue != aTypeCheckValue)
		{
			MC_DEBUG("Typecheck failed. Data type was %c%c, should have been %c%c. Last delimiter was %d",
				char(readValue>>8), char(readValue),
				char(aTypeCheckValue>>8), char(aTypeCheckValue),
				int(myLastDelimiter));

			if(!myLightTypecheckFlag)
				assert(0 && "Typecheck failed, wrong type");

			return false;
		}
	}

	return true;
}

bool MN_ReadMessage::AtEnd()
{
	//next chunk
	if(myCurrentPacket && myPendingPackets.Count() && myReadOffset >= myCurrentPacket->GetWriteOffset())
		NextReadPacket();

	if( myCurrentPacket )
	{
		if(myCurrentPacket && myReadOffset >= myCurrentPacket->GetWriteOffset())
			return true;
		else
			return false;
	}
	else if( myPendingPackets.Count() )
	{
		return false;
	}
	else
		return true;
}


//MN_ReceiveFormat implementation
//receive
//should return the number of bytes used by the subclass implementation, 0 if none used
unsigned int MN_ReadMessage::BuildMe(const void* aBuffer, const unsigned int aBufferLength)
{
	unsigned int usedData = 0;

	//clear message (can NOT be junk in the message)
	Clear();

	usedData = ConstructPackets((const unsigned char*)aBuffer, aBufferLength);

	return usedData;
}

#include <time.h>
#include <stdio.h>
#include "MC_Platform.h"
//construct message packets from buffer
unsigned int MN_ReadMessage::ConstructPackets(const unsigned char* someData, const unsigned int aDataLength)
{
	unsigned int usedBytes = 0;
	const unsigned char* data = someData;
	unsigned int dataLength = aDataLength;
	MN_Packet* newPacket = MN_Packet::Create(myDataPacketSize);
	unsigned short zippedSize;
	unsigned short unzippedSize;
	unsigned int numBytesConsumed = 0;

	//must have at least sizeof(myWriteOffset) in buffer to be able to construct packets
	while(dataLength > sizeof(short))
	{
		bool isZipped = false;
		ourTypeCheckFlag = IsTypeChecked(); // Reset incase it has been adapted to fit the last incoming (wrong type) message.
		numBytesConsumed = 0;
		unzippedSize = *(unsigned short*)data;
		zippedSize = unzippedSize;

		// Check if the message was sent with the same typechecking type as we use.
		if (((unzippedSize & 0x4000)>0) ^ IsTypeChecked())
		{
			// Message was sent %stypechecked but we are %stypechecked. Adapting!", unzippedSize & 0x4000?"":"not ", IsTypeChecked()?"":"not ");
			ourTypeCheckFlag = !ourTypeCheckFlag;
		}
		if( unzippedSize & 0x8000 )
		{
			zippedSize = *(const unsigned short*)data;
			isZipped = true;
			zippedSize &= 0x3fff;

			// Not the whole packet in buffer. Return for now and let game receive again.
			if( dataLength - sizeof(newPacket->GetWriteOffset()) < zippedSize )
			{
				MN_Packet::Deallocate(newPacket);
				return usedBytes;
			}

			unzippedSize = MP_Pack::UnpackZip(data+sizeof(unsigned short), zippedSize, ourUncompressedData, myDataPacketSize*LOC_BUFFER_MULTIPLIER, &numBytesConsumed);
			if ((zippedSize != numBytesConsumed) || (unzippedSize == 0))
			{
#ifdef _DEBUG
				MC_DEBUG("misformed data. Dropping.");
#endif
				MN_Packet::Deallocate(newPacket);
				return 0;
			}
			assert(unzippedSize <= myDataPacketSize*LOC_BUFFER_MULTIPLIER);

		}
		else
		{
			unzippedSize &= 0x3fff;
			zippedSize &= 0x3fff;
			// Not the whole packet in buffer. Return for now and let game receive again.
			if( dataLength - sizeof(newPacket->GetWriteOffset()) < unzippedSize)
			{
				MN_Packet::Deallocate(newPacket);
				return usedBytes;
			}
			numBytesConsumed = unzippedSize;
		}
		//set number of bytes in message data
//		newPacket->GetWriteOffset() = unzippedSize;

		//bytes used
		usedBytes += sizeof(short);
		dataLength -= sizeof(short);
		data += sizeof(short);

		//copy number of bytes that should be in this message
		if (isZipped)
			newPacket->AppendData(ourUncompressedData, unzippedSize);
		else
			newPacket->AppendData(data, unzippedSize);

		assert( newPacket->GetWriteOffset() <= newPacket->GetPacketDataCapacity());
		assert( newPacket->GetWriteOffset() <= myDataPacketSize );

		if (!newPacket->GetWriteOffset())
		{
#ifdef _DEBUG
			MC_DEBUG("Malformed packet. Dropping.");
#endif // _DEBUG
			break;
		}
		//add to message (copy)
		AddPacket(newPacket);

		//more bytes used
		usedBytes += numBytesConsumed;
		dataLength -= numBytesConsumed;
		data += numBytesConsumed;

		//clear for next read
		newPacket->Clear();
	}
	
	MN_Packet::Deallocate(newPacket);

	return usedBytes;
}


//copy the packet and add to list
void MN_ReadMessage::AddPacket(MN_Packet* aPacket)
{
	MN_Packet* newPacket;
	assert(aPacket->GetWriteOffset());
	newPacket = MN_Packet::Create( aPacket );
	assert(newPacket);
	assert(newPacket->GetWriteOffset());
	myPendingPackets.Add(newPacket);
	myPendingPacketMessageSize += newPacket->GetBinarySize();
//	myPendingPacketMessageSize += newPacket->GetWriteOffset();
}

