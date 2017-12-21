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
#include "mn_writemessage.h"
#include "MN_LoopbackConnection.h"
#include "mn_connection.h"

#include "mc_bitvector.h"
#include "mc_debug.h"

#include "mc_commandline.h"

#include "MP_Pack.h"
#include "zlib.h"
#include "MT_ThreadingTools.h"
#include "MT_Mutex.h"

#include "MC_Misc.h"

static const unsigned int LOC_BUFFER_MULTIPLIER = 2;


//__declspec(thread) static unsigned int LocZippedDataBufferSize = 0;
//__declspec(thread) static unsigned char* LocZippedDataBuffer = NULL;

__declspec(thread) static unsigned char LocZippedDataBuffer[1<<16];
__declspec(thread) static unsigned int LocZippedDataBufferSize = sizeof(LocZippedDataBuffer);


// yeah, only delete the first thread. This is so wic.exe memleak detection won't register this as a leak
//static struct locDataBufferRemover { locDataBufferRemover() { } ~locDataBufferRemover() { delete [] LocZippedDataBuffer; } } dataRemover;


#ifdef _DEBUG

static MT_Mutex locUnitTestMutex;

#include "MN_ReadMessage.h"
// unit test code

static volatile bool hasDoneUnitTest = false;

static bool MN_MessageTest()
{
	static bool hasTested = false;
	if (hasTested)
		return true;
	hasTested = true;
	// Construct a message
	bool defaultZipped = MN_Message::ShouldZip();
	bool defaultTypeChecked = MN_Message::IsTypeChecked();

	int defaultrand = rand();

	for (int i = 0; i < 4; i++)
	{
		switch(i)
		{
		case 0:
			MN_Message::EnableZipCompression(false);
			MN_Message::EnableTypeChecking(false);
			break;
		case 1:
			MN_Message::EnableZipCompression(true);
			MN_Message::EnableTypeChecking(false);
			break;
		case 2:
			MN_Message::EnableZipCompression(true);
			MN_Message::EnableTypeChecking(true);
			break;
		case 3:
			MN_Message::EnableZipCompression(false);
			MN_Message::EnableTypeChecking(true);
			break;
		}
		MN_WriteMessage wm;
		const int NUM_LOOPS = 50;
		for (int i = 0; i < NUM_LOOPS; i++)
		{
			srand(i);
			switch(rand()%5)
			{
			case 0:
				wm.WriteString("Testar lite hejsan på dejsan!");
			case 1:
				wm.WriteDelimiter(0xff);
				wm.WriteUInt(99);
			case 2:
				wm.WriteShort(23);
			case 3:
				wm.WriteUChar(200);
			case 4:
				wm.WriteUInt(987);
			}
		}
		wm.WriteInt('end ');

		MN_LoopbackConnection<20480> stream;
		if(wm.SendMe(&stream) != MN_CONN_OK)
			assert(false);

		for (int i = 0; i < 4; i++)
		{
			switch(i)
			{
			case 0:
				MN_Message::EnableZipCompression(false);
				MN_Message::EnableTypeChecking(false);
				break;
			case 1:
				MN_Message::EnableZipCompression(true);
				MN_Message::EnableTypeChecking(false);
				break;
			case 2:
				MN_Message::EnableZipCompression(true);
				MN_Message::EnableTypeChecking(true);
				break;
			case 3:
				MN_Message::EnableZipCompression(false);
				MN_Message::EnableTypeChecking(true);
				break;
			}

			MN_ReadMessage rm;
			assert(rm.BuildMe((const unsigned char*)stream.myData, stream.myDatalen) == stream.myDatalen);

			MC_String str;
			MN_DelimiterType del;
			signed int si;
			unsigned int ui;
			short sh;
			unsigned char uc;

			// make sure we got back what we put in
			for (int i = 0; i < NUM_LOOPS; i++)
			{
				srand(i);
				switch(rand() % 5)
				{
				case 0:
					rm.ReadString(str);
				case 1:
					rm.ReadDelimiter(del);
					rm.ReadUInt(ui);
				case 2:
					rm.ReadShort(sh);
				case 3:
					rm.ReadUChar(uc);
				case 4:
					rm.ReadUInt(ui);
				};
			}
			rm.ReadInt(si);
			assert(si == 'end ');
		}
	}
	MN_Message::EnableZipCompression(defaultZipped);
	MN_Message::EnableTypeChecking(defaultTypeChecked);
	srand(defaultrand);
	return true;
}

static bool MN_MessageTest2()
{
	// NOT COMPATIBLE WITH NEW TYPECHECKING

	const unsigned char buff1[269] = {

		0xB,0xC1,0x78,0xDA,0x5D,0x90,0x41,0x4E,0xC2,0x40,0x18,0x85,0x3F,0x6D,0x8,0x9A,
			0x28,0x31,0x81,0x85,0xC4,0x33,0x98,0xD8,0x8,0x2,0x4B,0xC5,0xD,0x89,0x6C,0x34,
			0xC2,0x82,0x10,0xD2,0xDA,0x6A,0x4D,0xA,0x43,0xAC,0x21,0x70,0x14,0xAE,0xE0,0x91,
			0x3C,0x83,0x4B,0xF,0xC0,0x9B,0x59,0x10,0x3A,0x69,0x3A,0x99,0x4E,0xDF,0xF7,0xDE,
			0xFB,0x27,0xE0,0xF1,0x89,0x4A,0x60,0xD7,0x6A,0xC0,0xEB,0x80,0xFF,0x2B,0xB8,0x60,
			0xCA,0x84,0x88,0x18,0xC3,0x8A,0x54,0xBB,0x82,0x84,0x77,0x42,0xBD,0x6D,0xBA,0xC0,
			0x1,0x30,0x3D,0x87,0x3A,0x63,0x6,0xF4,0xB9,0xA6,0x43,0x26,0x20,0xD7,0x63,0x58,
			0xF0,0x29,0xB0,0xD0,0x77,0x2A,0xF4,0xAD,0x8C,0x9D,0x1D,0xC3,0xA9,0xFC,0x52,0xE6,
			0x92,0x5A,0xFF,0x96,0x9E,0x92,0x64,0x7B,0x64,0x25,0xB3,0x7D,0x7C,0xA2,0x68,0xCF,
			0xE5,0x59,0x2E,0xCD,0x7D,0x78,0x97,0x5B,0x89,0x17,0xF2,0xFB,0x76,0x35,0xBE,0xB8,
			0x17,0x90,0xA9,0xC6,0x4A,0xE7,0xB9,0x4A,0xB4,0x54,0xD0,0xEB,0xDF,0xAB,0x40,0x43,
			0xE3,0xE,0x79,0x61,0xA4,0xC1,0x43,0xEE,0x58,0xCA,0x22,0x67,0xC3,0x83,0xC3,0x2C,
			0xD8,0x56,0xBE,0x57,0xEF,0xB7,0x61,0x6F,0x2A,0xE2,0xC3,0x8D,0xBA,0x51,0xB,0xA3,
			0x44,0x9B,0x6C,0x74,0x16,0x2B,0xD9,0x3,0x7E,0x2,0x38,0x71,0xCE,0xB9,0xCA,0x25,
			0x42,0xBD,0x2A,0x7F,0xF5,0x72,0x95,0xE,0x3D,0x39,0x1A,0xD6,0x9A,0x23,0xD6,0x2E,
			0xD2,0x5D,0x15,0x82,0x42,0x6E,0xCA,0x60,0xED,0x12,0x6A,0xFA,0x99,0x2A,0x38,0x52,
			0xFB,0x4C,0xAB,0x95,0x1B,0x97,0x63,0xC5,0x3B,0x2F,0x3B,0x47,0x1F };

		const unsigned char buff2[] = {

			0x7B,0xC0,0x78,0xDA,0x63,0x66,0x70,0xF1,0x61,0x60,0x65,0x66,0x8,0xF5,0x64,0x48,
				0x61,0x60,0x60,0x0,0x32,0x82,0x19,0x14,0x41,0x94,0x33,0x3,0x23,0x58,0x34,0x59,
				0x18,0x2C,0xEA,0xC9,0x70,0x61,0x59,0x18,0x3B,0x98,0xC1,0x0,0x53,0xF7,0x71,0x3D,
				0x98,0xEA,0xDC,0xA,0xA6,0x2E,0x46,0x72,0x30,0x14,0x33,0xA4,0x32,0x14,0x31,0x94,
				0x81,0x49,0x4B,0x98,0x32,0x6,0x84,0x71,0xCE,0x50,0xB6,0x0,0x27,0x43,0x62,0x4E,
				0x4E,0x7E,0xB9,0x95,0x82,0x16,0x83,0x3C,0x43,0x6E,0x62,0x41,0xB1,0x7E,0x4A,0x6A,
				0x4E,0x66,0x59,0x6A,0x51,0x65,0x7C,0x49,0x6A,0x71,0x89,0xA1,0x7E,0x6A,0x45,0x41,
				0x7E,0x51,0x89,0x5E,0x66,0x72,0x2A,0x3,0x0,0xB3,0x25,0x21,0x2F
		};

		const unsigned char buff3[] = {
			0x5B,0xC0,0x78,0xDA,0x63,0x66,0x8,0xD,0x66,0x60,0x6,0x42,0x4F,0x3F,0x6,0x46,
				0x6,0x6,0x20,0x23,0xD4,0x13,0xB,0xC3,0x99,0x13,0xCA,0x68,0xFE,0x92,0xF3,0x4B,
				0x92,0x21,0x9D,0x21,0x91,0xA1,0x84,0x21,0x95,0xA1,0x18,0x48,0xE6,0x33,0xE4,0x1,
				0x59,0x86,0x40,0x68,0xC0,0x60,0xC6,0xE0,0xC0,0x90,0xB,0x94,0x2B,0x6,0xC2,0x4C,
				0x86,0x32,0xA0,0xB8,0x1E,0x90,0x95,0xA,0x34,0x85,0x9D,0x21,0x3,0x48,0x67,0x1,
				0x79,0x89,0x40,0xF5,0x50,0xC3,0x40,0x0,0x0,0x97,0xAC,0x13,0x3
		};

		const unsigned char buff4[] = {
			0x42,0x0,0x9,0x23,0x0,0x42,0x48,0x6B,0x63,0x44,0x5A,0x6C,0x68,0x39,0x63,0x31,
				0x66,0x62,0x61,0x54,0x63,0x68,0x6C,0x68,0x6C,0x52,0x4A,0x58,0x46,0x70,0x75,0x53,
				0x56,0x6E,0x33,0x78,0x58,0x45,0x53,0x0,0x85,0x6,0x0,0x0,0x86,0x4,0x0,0x0,
				0xE0,0x5A,0x0,0x0,0x7,0x0,0x68,0x0,0x65,0x0,0x6A,0x0,0x73,0x0,0x61,0x0,
				0x6E,0x0,0x0,0x0,0xC0,0xC0
		};

		int i = 1;
		bool defaultTypecheck = MN_Message::IsTypeChecked();
		MN_Message::EnableTypeChecking(false);

		while (i--)
		{
			MN_ReadMessage rm1, rm2, rm3, rm4;
			assert(rm1.BuildMe(buff1, sizeof(buff1)) == sizeof(buff1));
			assert(rm2.BuildMe(buff2, sizeof(buff2)) == sizeof(buff2));
			assert(rm3.BuildMe(buff3, sizeof(buff3)) == sizeof(buff3));
			assert(rm4.BuildMe(buff3, sizeof(buff3)) == sizeof(buff3));
			MN_Message::EnableTypeChecking(true);
		}
		MN_Message::EnableTypeChecking(defaultTypecheck);

		return true;
}
#endif // _DEBUG


unsigned char* MN_WriteMessage::GetDataBuffer(unsigned int theDataSize)
{
	// SWFM:SWR - allocate buffer if not yet present
	assert(theDataSize < LocZippedDataBufferSize);
	return LocZippedDataBuffer;
}



//constructor
MN_WriteMessage::MN_WriteMessage(unsigned int theTransportSize)
:MN_Message()
, myDataPacketSize(theTransportSize)
{
	myPendingPacketMessageSize = 0;
#ifdef _DEBUG
	MT_MutexLock lock(locUnitTestMutex);
	if (!hasDoneUnitTest)
	{
		hasDoneUnitTest = true;
//		MC_DEBUG("Testing MN_ReadMessage and MN_WriteMessage");
//		assert(MN_MessageTest());
//		assert(MN_MessageTest2());
//		MC_DEBUG("Test complete.");
	}
	ourTypeCheckFlag = ourDefaultTypeCheckFlag;
#endif // _DEBUG
}


//destructor
MN_WriteMessage::~MN_WriteMessage()
{
	myPendingPacketMessageSize = 0;
}



//write routines for basic data types (can be extended)
void MN_WriteMessage::WriteBool(bool aBool)
{
	char		tmp = aBool ? 1 : 0;
	PrepareWrite(sizeof(tmp), 'BL');
	myCurrentPacket->AppendData(&tmp, sizeof(tmp));
}

void MN_WriteMessage::WriteChar(char aChar)
{
	PrepareWrite(sizeof(aChar), 'CH');
	myCurrentPacket->AppendData(&aChar, sizeof(aChar));
}


void MN_WriteMessage::WriteUChar(unsigned char aUChar)
{
	PrepareWrite(sizeof(aUChar), 'UC');
	myCurrentPacket->AppendData(&aUChar, sizeof(aUChar));
}


void MN_WriteMessage::WriteShort(short aShort)
{
	PrepareWrite(sizeof(aShort), 'SH');
	myCurrentPacket->AppendData(&aShort, sizeof(aShort));
}


void MN_WriteMessage::WriteUShort(unsigned short anUShort)
{
	PrepareWrite(sizeof(anUShort), 'US');
	myCurrentPacket->AppendData(&anUShort, sizeof(anUShort));
}


void MN_WriteMessage::WriteInt(int anInt)
{
	PrepareWrite(sizeof(anInt), 'IN');
	myCurrentPacket->AppendData(&anInt, sizeof(anInt));
}

void MN_WriteMessage::WriteUInt(unsigned int anUInt)
{
	PrepareWrite(sizeof(anUInt), 'UI');
	myCurrentPacket->AppendData(&anUInt, sizeof(anUInt));
}


void MN_WriteMessage::WriteInt64( const __int64 anInt)
{
	PrepareWrite(sizeof(anInt), 'I6');
	myCurrentPacket->AppendData(&anInt, sizeof(anInt));
}

void MN_WriteMessage::WriteUInt64( const unsigned __int64 anInt)
{
	PrepareWrite(sizeof(anInt), 'U6');
	myCurrentPacket->AppendData(&anInt, sizeof(anInt));
}

void MN_WriteMessage::WriteFloat(float aFloat)
{
	assert(MC_Misc::SanityCheckFloat(aFloat));
	PrepareWrite(sizeof(aFloat), 'FL');
	myCurrentPacket->AppendData(&aFloat, sizeof(aFloat));
}

void MN_WriteMessage::WriteRawData( const void* someData, unsigned short aSize )
{
	PrepareWrite(sizeof(aSize) + aSize, 'RD');
	myCurrentPacket->AppendData((unsigned char*)&aSize, sizeof(aSize));
	myCurrentPacket->AppendData((const unsigned char*)someData, aSize);
}


void MN_WriteMessage::WriteString(const char* aString)
{
	unsigned short length;


	//if first write
	if(!myCurrentPacket)
		CreateFirstWritePacket();
	
	length = strlen(aString) + 1;

	//handle potential overflow (length plus null character)
	if((myCurrentPacket->GetWriteOffset() + sizeof(unsigned short) + length /*+ sizeof(char)*/) >= myCurrentPacket->GetPacketDataCapacity())
		WriteOverflow(sizeof(unsigned short) + length/* + sizeof(char)*/);


	// Write size of string so we don't have to search for it in ReadMessage
	myCurrentPacket->AppendData((const unsigned char*)&length, sizeof(length));

	//copy string
	myCurrentPacket->AppendData((const unsigned char*)aString, length);
}


void MN_WriteMessage::WriteLocString(const MC_LocString& aString)
{
	WriteLocString(aString, aString.GetLength());
}

void MN_WriteMessage::WriteLocString(const wchar_t* aString, unsigned int aStringLen)
{
	//if first write
	if(!myCurrentPacket)
		CreateFirstWritePacket();

	unsigned short length = aStringLen + 1;

	//handle potential overflow (length plus null character)
	if((myCurrentPacket->GetWriteOffset() + sizeof(unsigned short) + length * sizeof(MC_LocChar)) >= myCurrentPacket->GetPacketDataCapacity())
		WriteOverflow(sizeof(unsigned short) + length * sizeof(MC_LocChar));

	// Write size of string so we don't have to search for it in ReadMessage
	myCurrentPacket->AppendData((const unsigned char*)&length, sizeof(length));

	//copy string
	myCurrentPacket->AppendData((const unsigned char*)aString, length*sizeof(MC_LocChar));
}



void MN_WriteMessage::WriteVector2f(const MC_Vector2f& aVector)
{
	PrepareWrite(sizeof(aVector), 'V2');
	myCurrentPacket->AppendData(&aVector, sizeof(aVector));
}


void MN_WriteMessage::WriteVector3f(const MC_Vector3f& aVector)
{
	PrepareWrite(sizeof(aVector), 'V3');
	myCurrentPacket->AppendData(&aVector, sizeof(aVector));
}

// Valid range [0 .. MN_POSITION_RANGE]
void MN_WriteMessage::WritePosition2f(const MC_Vector2f& aVector)
{
	unsigned short s[2];
	PrepareWrite(sizeof(s), 'P2');

	assert(aVector.x >= 0);
	assert(aVector.y >= 0);
	assert(aVector.x <= MN_POSITION_RANGE);
	assert(aVector.y <= MN_POSITION_RANGE);

	s[0] = (unsigned short) (aVector.x * (65535.99f / MN_POSITION_RANGE));
	s[1] = (unsigned short) (aVector.y * (65535.99f / MN_POSITION_RANGE));

	myCurrentPacket->AppendData(s, sizeof(s));
}

// Valid range [0 .. MN_POSITION_RANGE]
void MN_WriteMessage::WritePosition3f(const MC_Vector3f& aVector)
{
	unsigned short s[3];
	PrepareWrite(sizeof(s), 'P3');

	assert(aVector.x >= 0);
	assert(aVector.y >= 0);
	assert(aVector.z >= 0);
	assert(aVector.x <= MN_POSITION_RANGE);
	assert(aVector.y <= MN_POSITION_RANGE);
	assert(aVector.z <= MN_POSITION_RANGE);

	s[0] = (unsigned short) (aVector.x * (65535.99f / MN_POSITION_RANGE));
	s[1] = (unsigned short) (aVector.y * (65535.99f / MN_POSITION_RANGE));
	s[2] = (unsigned short) (aVector.z * (65535.99f / MN_POSITION_RANGE));

	myCurrentPacket->AppendData(s, sizeof(s));
}

// Valid range [-MN_ANGLE_RANGE .. MN_ANGLE_RANGE]
void MN_WriteMessage::WriteAngle(float aFloat)
{
	unsigned short s;
	PrepareWrite(sizeof(s), 'AN');

	assert(aFloat > -MN_ANGLE_RANGE);
	assert(aFloat < MN_ANGLE_RANGE);

	s = (unsigned short) ((aFloat + MN_ANGLE_RANGE) * (65535.99f / (MN_ANGLE_RANGE*2)));

	myCurrentPacket->AppendData(&s, sizeof(s));
}

// Valid range [0.0 .. 1.0]
void MN_WriteMessage::WriteUnitFloat(float aFloat)
{
	unsigned short s;
	PrepareWrite(sizeof(s), 'UF');

	int t = int(aFloat * 65535.99f);
	if(t < 0)
		t = 0;
	else if(t > 65535)
		t = 65535;

	s = (unsigned short) t;

	myCurrentPacket->AppendData(&s, sizeof(s));
}

//special, for debugging and network integrity
void MN_WriteMessage::WriteDelimiter(const MN_DelimiterType& aDelimiter)
{
	//if first write
	if(!myCurrentPacket)
		CreateFirstWritePacket();
	
	//store command and write offset
//	assert(aDelimiter >= 0 && aDelimiter < NETCMD_NUMCOMMANDS && aDelimiter < 256);
	myLastDelimiter = aDelimiter;
	myLastDelimiterWriteOffset = myCurrentPacket->GetWriteOffset();

#ifndef _RELEASE_
	if( MC_CommandLine::GetInstance()->IsPresent( "mn_showlight" ) && aDelimiter > 1 )
		MC_DEBUG( "MC_WriteMessage::WriteDelimiter - Adding delimiter : %d", aDelimiter );
#endif
	//handle potential overflow
	if((myCurrentPacket->GetWriteOffset() + sizeof(MN_DelimiterType)+ (ourTypeCheckFlag ? sizeof(short) : 0)) >= myCurrentPacket->GetPacketDataCapacity())
		WriteOverflow(sizeof(MN_DelimiterType)+ (ourTypeCheckFlag ? sizeof(short) : 0));

	if(ourTypeCheckFlag)
	{
		short typecheckId = 'DL';
		myCurrentPacket->AppendData(&typecheckId, sizeof(typecheckId));
	}

	myCurrentPacket->AppendData(&aDelimiter, sizeof(MN_DelimiterType));
}


//bit vector
void MN_WriteMessage::WriteBitVector(const MC_BitVector& aBitVector)
{
	assert(aBitVector.GetDataLength() < MN_PACKET_DATA_SIZE-32);	// Sanity check

	const unsigned short length = aBitVector.GetDataLength();
	PrepareWrite(length + sizeof(length), 'BV');
	myCurrentPacket->AppendData(&length, sizeof(length));
	myCurrentPacket->AppendData(aBitVector.GetData(), length);
}

//append an existing MN_WriteMessage
void MN_WriteMessage::Append(MN_WriteMessage& aMessage)
{
	//don't support empty messages
	assert(!aMessage.Empty());

	//if I am empty, then can 
	if(Empty())
	{
		AppendEmpty(aMessage);
	}
	else
	{
		AppendNotEmpty(aMessage);
	}

	//at this point, we MUST have a valid myCurrentPacket, as we don't support appending empty messages
	//(first assert in this function)
	assert(myCurrentPacket);
}


//append a message to an empty message
void MN_WriteMessage::AppendEmpty(MN_WriteMessage& aMessage)
{
	MN_Packet* src;
	MN_Packet* copy;
	
	//self must be empty, incoming may not be empty!
	assert(Empty() && !aMessage.Empty());
	
	//copy pending packets (may be empty)
	for (int i = 0; i < aMessage.myPendingPackets.Count(); i++)
	{
		src = aMessage.myPendingPackets[i];
		assert(src);

		copy = MN_Packet::Create(src);
		assert(copy);

		myPendingPackets.Add(copy);
		myPendingPacketMessageSize += copy->GetBinarySize();
	}

	//copy current packet (may NOT be empty, as we don't support appending empty messages!)
	assert(!myCurrentPacket && aMessage.myCurrentPacket);
	myCurrentPacket = MN_Packet::Create(aMessage.myCurrentPacket);
	assert(myCurrentPacket);
}


//append a message to a non-empty message
void MN_WriteMessage::AppendNotEmpty(MN_WriteMessage& aMessage)
{
	MN_Packet* appPacket;

	//self and incoming may not be empty!
	assert(!Empty() && !aMessage.Empty());

	//while aMessage.myPendingPackets fit in myCurrentPacket
	int i=0;
	while(i < aMessage.myPendingPackets.Count())
	{
		appPacket = aMessage.myPendingPackets[i];
		assert(appPacket);

		if(PacketFitsInCurrentPacket(*appPacket))
		{
			//copy to myCurrentPacket
			CopyPacketToCurrentPacket(*appPacket);
		}
		else
		{
			//didn't fit!
			break;
		}
		//next incoming packet
		i++;
	}

	//if all aMessage.myPendingPackets fit in myCurrentPacket, try the aMessage.myCurrentPacket as well
	if(i == aMessage.myPendingPackets.Count())
	{
		//is it there?
		if(aMessage.myCurrentPacket)
		{
			//if it exists, it should be written to, and if it doesn't, the message was empty,
			//and the first assert in this function handles that!
			assert(aMessage.myCurrentPacket->GetWriteOffset());

			//if fits...
			if(PacketFitsInCurrentPacket(*aMessage.myCurrentPacket))
			{
				//copy
				CopyPacketToCurrentPacket(*aMessage.myCurrentPacket);
			}
			//if not...
			else
			{
				//add current to pending
				myPendingPackets.Add(myCurrentPacket);
				myPendingPacketMessageSize += myCurrentPacket->GetBinarySize();
				myCurrentPacket = NULL;

				//a copy of aMessage.myCurrentPacket will now become our current packet
				myCurrentPacket = MN_Packet::Create(aMessage.myCurrentPacket);
				assert(myCurrentPacket);
			}
		}
	}
	//if all aMessage.myPendingPackets DIDN'T fit in myCurrentPacket...
	else
	{
		//myCurrentPacket is as full as it will get, so move to pending
		myPendingPackets.Add(myCurrentPacket);
		myPendingPacketMessageSize += myCurrentPacket->GetBinarySize();
		myCurrentPacket = NULL;
		
		//if there are incoming packets remaining, append them as well
		while(i != aMessage.myPendingPackets.Count())
		{
			appPacket = aMessage.myPendingPackets[i];
			assert(appPacket);

			AppendPacket(*appPacket);

			i++;
		}

		//if aMessage.myCurrentPacket is there, we should copy it to become our myCurrentPacket
		if(aMessage.myCurrentPacket)
		{
			//if it exists, it should be written to, and if it doesn't, the message was empty,
			//and the first assert in this function handles that!
			assert(aMessage.myCurrentPacket->GetWriteOffset());

			//a copy of aMessage.myCurrentPacket will now become our current packet
			myCurrentPacket = MN_Packet::Create(aMessage.myCurrentPacket);
			assert(myCurrentPacket);
		}
	}
}


bool MN_WriteMessage::PacketFitsInCurrentPacket(MN_Packet& aPacket)
{
	assert(myCurrentPacket);
	unsigned short freeSpace = myCurrentPacket->GetPacketDataCapacity() - myCurrentPacket->GetWriteOffset();

	return (freeSpace >= aPacket.GetWriteOffset());
}


void MN_WriteMessage::CopyPacketToCurrentPacket(MN_Packet& aPacket)
{
	assert(myCurrentPacket && PacketFitsInCurrentPacket(aPacket));
	myCurrentPacket->AppendData(aPacket.GetData(), aPacket.GetWriteOffset());
}

//append an existing MN_Packet
void MN_WriteMessage::AppendPacket(MN_Packet& aPacket)
{
	MN_Packet* copy;
	
	//don't support empty packets
	assert(aPacket.GetWriteOffset());

	//copy construct the packet
	copy = MN_Packet::Create(&aPacket);
	assert(copy);
	myPendingPackets.Add(copy);
	myPendingPacketMessageSize += copy->GetBinarySize();
}


//create a first chunk for writing
void MN_WriteMessage::CreateFirstWritePacket()
{
	//NOTE: After the addition of Append(),
	//you will have cases with a NULL current packet and pending packets in the list
//	assert(!myCurrentPacket && !myPendingPackets.Count());
	assert(!myCurrentPacket);
	
	myCurrentPacket = MN_Packet::Create(myDataPacketSize);
	assert(myCurrentPacket);
}


//shorten the current chunk at the last MN_Delimiter, stack it, and create a new chunk for writing
void MN_WriteMessage::WriteOverflow(unsigned short aLengthToWrite)
{
	MN_Packet* newPacket;
	unsigned short overFlowLength = myCurrentPacket->GetWriteOffset() - myLastDelimiterWriteOffset;

	//check if chopping won't help
	if(unsigned int(overFlowLength + aLengthToWrite) >= myCurrentPacket->GetPacketDataCapacity())
	{
		MC_DEBUG("MN_WriteMessage::WriteOverflow(): Cannot create new packet, command data is larger than maximum buffer size\n\nDelimiter: %d\nData length: %d\nMaximum buffer size: %d\n\nTHIS IS AN APPLICATION PROTOCOL DESIGN ERROR!",
								myLastDelimiter,
								(overFlowLength + aLengthToWrite),
								myCurrentPacket->GetPacketDataCapacity());
		assert(0 && "MN_WriteMessage::WriteOverflow(): Application data overflows max packet size");
	}
	
	//create new chunk
	newPacket = MN_Packet::Create(myDataPacketSize);
	assert(newPacket);

	//copy overflowed data to new chunk
	newPacket->AppendData(myCurrentPacket->GetData() + myLastDelimiterWriteOffset, overFlowLength);

	//shorten the overflowed chunk
	myCurrentPacket->SetWriteOffset(myLastDelimiterWriteOffset);
	
	//add to list
	myPendingPackets.Add(myCurrentPacket);
	myPendingPacketMessageSize += myCurrentPacket->GetBinarySize();
	
	//set pointer to "new" current chunk
	myCurrentPacket = newPacket;

	//set new net cmd offset (now beginning of current chunk)
	myLastDelimiterWriteOffset = 0;

	//make sure can write now
	assert(unsigned int(myCurrentPacket->GetWriteOffset() + aLengthToWrite) < myCurrentPacket->GetPacketDataCapacity());
}

//creates first packet if needed and writes overflow if needed, writes typecheck identifier if ourTypeCheckFlag==true
void MN_WriteMessage::PrepareWrite(unsigned short aLengthToWrite, short aTypeCheckValue)
{
	//if first write
	if(!myCurrentPacket)
		CreateFirstWritePacket();
	
	//handle potential overflow
	unsigned short numBytesNeeded = aLengthToWrite + (ourTypeCheckFlag ? sizeof(short) : 0);
	if(myCurrentPacket->GetWriteOffset() + numBytesNeeded >= myCurrentPacket->GetPacketDataCapacity())
		WriteOverflow(numBytesNeeded);

	if(ourTypeCheckFlag)
		myCurrentPacket->AppendData(&aTypeCheckValue, sizeof(aTypeCheckValue));
}



//MN_SendFormat implementation
//send internal data using MN_Connection
//note that myCurrentPacket may now be empty, as a result of the Append() function
//in these cases myCurrentPacket is simply ignored since there is nothing to send
MN_ConnectionErrorType MN_WriteMessage::SendMe(MN_IWriteableDataStream* aConnection, 
											   bool aDisableCompression)
{
	// Check for implementation error -- is your MN_WriteMessage configured for a larger packetsize than this transport can handle?
	assert(aConnection->GetRecommendedBufferSize() >= myDataPacketSize);

	MN_Packet* packet;
	MN_ConnectionErrorType err;
	bool packetSent = false;
	bool fallbackToUncompressed = false;
	bool localZipFlag; 

	localZipFlag = ourZipFlag && (aDisableCompression == false); 

	unsigned short* header = NULL;

	unsigned int currNumberOfCoalescedPackets = 0;

	// Init compression
	int zerr;
	z_stream stream;
	unsigned char* zipBuffer = NULL;
	const unsigned int zipBufferCapacity = myCurrentPacket->GetPacketDataCapacity();
	unsigned int zipBufferSize = 0;

	if (localZipFlag)
	{
		stream.zalloc = (alloc_func)0;
		stream.zfree = (free_func)0;
		stream.opaque = (voidpf)0;

		zipBuffer = GetDataBuffer(zipBufferCapacity+2);
		zipBuffer += sizeof(unsigned short);
		zipBufferSize = 0;
		currNumberOfCoalescedPackets = 0;
		zerr = deflateInit(&stream, Z_BEST_COMPRESSION);
		if (zerr != Z_OK) 
		{
			MC_DEBUG("Could not init compression subsystem (%u).", zerr);
			assert(false);
			return MN_CONN_BROKEN;
		}
	}

	// Temporarily add myCurrentPacket to our pending packets for a happy loop life
	myPendingPackets.Add(myCurrentPacket);
	RemoveLastPacketFromPending autoRemover(myPendingPackets); // so we can return; whenever we want and maintain state

	// Iterate over all packets and send them

	for (int i = 0; i < myPendingPackets.Count(); i++)
	{
		packet = myPendingPackets[i];
		if (packet->GetWriteOffset() == 0)
			continue;
		if (packet->GetBinarySize() >= 0x4000)
		{
			MC_DEBUG("SIZE OF MESSAGE TOO LARGE. YOU HAVE A PROTOCOL ERROR.");
			assert(packet->GetBinarySize() < 0x4000);
			return MN_CONN_BROKEN;
		}


		if (localZipFlag)
		{
			// Incrementally zip the packet and only send if the accumulated zip is larger than a single MN_Packet.
			fallbackToUncompressed = false;
			zerr = deflateReset(&stream);
			assert(zerr == Z_OK);
			stream.next_in = (Bytef*) packet->GetData();
			stream.avail_in = packet->GetWriteOffset();
			stream.next_out = (Bytef*) zipBuffer + zipBufferSize;
			stream.avail_out = __max(0, int(zipBufferCapacity) - int(zipBufferSize) - int(sizeof(unsigned short)));

			zerr = deflate(&stream, Z_FINISH);

			if (zerr == Z_STREAM_END)
			{
				// All good - the packet was added to the outgoing zipbuffer
				header = (unsigned short*)(zipBuffer+zipBufferSize - sizeof(unsigned short));
				if (stream.total_out >= packet->GetBinarySize())
				{
					// The compressed packet is larger than the uncompressed one. Add the uncompressed one instead
					memcpy(header, packet->GetBinaryData(), packet->GetBinarySize());
					zipBufferSize += packet->GetBinarySize();
					assert(zipBufferSize <= zipBufferCapacity);
					SetCompressedFlag(*header, false);
				}
				else
				{
					*header = (unsigned short)stream.total_out;
					// Tag packet as ok
					SetCompressedFlag(*header, true);
					zipBufferSize += stream.total_out;
					zipBufferSize += sizeof(unsigned short);
					assert(zipBufferSize <= zipBufferCapacity);
				}
				SetTypecheckFlag(*header, ourTypeCheckFlag);
				// Be ready to process next packet
			}
			else
			{
				// The current packet could not fit in the output buffer.
				// if we have accumulated zippackets from before, send them and retry current
				// else the current cannot fit in a single packet (compressed > uncompressed) so fall back
				// to uncompressed.
				if (zipBufferSize)
				{
					err = aConnection->Send(zipBuffer - sizeof(unsigned short), zipBufferSize);
					if (err != MN_CONN_OK)
					{
						deflateEnd(&stream);
						return err;
					}
					zipBufferSize = 0;
					packetSent = true;
					// Retry from scratch with current packet
					i--;
				}
				else
				{
					// a single packet compressed to larger than allowed size. Send it uncompressed.
					fallbackToUncompressed = true;
					MC_DEBUG("fallback to uncompressed");
				}
			}
		}
		if ((!localZipFlag) || fallbackToUncompressed)// Send the packet in buffer as-is without compression.
		{
			header = (unsigned short*)packet->GetBinaryData();
			SetCompressedFlag(*header, false);
			SetTypecheckFlag(*header, ourTypeCheckFlag);
			err = aConnection->Send(packet->GetBinaryData(), packet->GetBinarySize());
			if (err != MN_CONN_OK)
			{
				// deallocate zip structures
				if (localZipFlag)
					deflateEnd(&stream);
				return err;
			}
			packetSent = true;
		}
	}

	if (localZipFlag && zipBufferSize)
	{
		// We still have some zipped data in our outputbuffer - send it!
		assert(zipBufferSize <= zipBufferCapacity);

		err = aConnection->Send(zipBuffer - sizeof(unsigned short), zipBufferSize);

		if (err != MN_CONN_OK)
		{
			// deallocate zip structures
			deflateEnd(&stream);
			return err;
		}
		packetSent = true;
	}

	// deallocate zip structures
	if (localZipFlag)
	{
		assert(zipBufferSize <= zipBufferCapacity);
		deflateEnd(&stream);
//		delete [] (zipBuffer-sizeof(unsigned short));
	}

	//success
	if(packetSent)
		return MN_CONN_OK;
	else
		return MN_CONN_NODATA;
}

void MN_WriteMessage::SetTypecheckFlag(unsigned short& theField, bool aBoolean)
{
	if (aBoolean)
		theField |= 0x4000;
	else
		theField &= 0xBFFF;
}

void MN_WriteMessage::SetCompressedFlag(unsigned short& theField, bool aBoolean)
{
	if (aBoolean)
		theField |= 0x8000;
	else
		theField &= 0x7fff;
}

