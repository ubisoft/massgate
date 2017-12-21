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
//mn_message.h
//compressed data for network transport

#ifndef MN_MESSAGE_H
#define MN_MESSAGE_H

#include "MC_HybridArray.h"
#include "mn_packet.h"

typedef unsigned short MN_DelimiterType;

// Effective range for positions is [0 .. MN_POSITION_RANGE]
#define MN_POSITION_RANGE (1536.0f)

// Effective range for angles is [-MN_ANGLE_RANGE .. MN_ANGLE_RANGE]
#define MN_ANGLE_RANGE (8.0f)

//message class
class MN_Message
{
public:
	static void EnableZipCompression(bool aState)	{ourZipFlag = aState;}
	static void EnableCompression(bool aState)		{ourCompressionFlag = aState;}
	static void EnableTypeChecking(bool aState)		{ourDefaultTypeCheckFlag = aState;}
	static bool ShouldZip( void )					{return ourZipFlag;}
	static bool IsCompressed( void )				{return ourCompressionFlag;}
	static bool IsTypeChecked( void )				{return ourDefaultTypeCheckFlag;}

	//destructor
	virtual ~MN_Message();

	//clear the message, and reset the data pointers
	void Clear();

	bool Empty();

	// Returns total size of all packets in message
	unsigned int GetMessageSize( void );

	MN_DelimiterType GetLastDelimiter() { return myLastDelimiter; }
protected:

	MN_Message& operator =(const MN_Message&) { return *this; }
	//constructor
	MN_Message();

	//static members
	static volatile bool ourZipFlag;
	static volatile bool ourCompressionFlag;
	static volatile bool ourDefaultTypeCheckFlag;

	bool ourTypeCheckFlag;


	//current packet
	MN_Packet* myCurrentPacket;

	//pending message packets
	MC_HybridArray<MN_Packet*,8> myPendingPackets;

	//position of last delimiter
	MN_DelimiterType myLastDelimiter;
	unsigned short myLastDelimiterWriteOffset;

	// 
	unsigned int myPendingPacketMessageSize;
};

#endif