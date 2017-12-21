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
//mn_message.cpp
//compressed data for network transport

#include "stdafx.h"
#include "mn_message.h"
#include "mc_bitvector.h"

#ifndef _RELEASE_
#include "mc_debug.h"
#include "mc_commandline.h"
#endif
//local constants
volatile bool MN_Message::ourZipFlag = false;
volatile bool MN_Message::ourCompressionFlag = false;
volatile bool MN_Message::ourDefaultTypeCheckFlag = true;


//constructor
MN_Message::MN_Message()
: ourTypeCheckFlag(ourDefaultTypeCheckFlag)
{
	//nothing allced from beginning
	myCurrentPacket = NULL;
	myLastDelimiter = 0;
	myLastDelimiterWriteOffset = 0;
	myPendingPacketMessageSize = 0;
}


//destructor
MN_Message::~MN_Message()
{
	//delete
	Clear();
}


//clear the message, and reset the data pointer
void MN_Message::Clear()
{
	//delete ALL packets
	if(myCurrentPacket)
	{
		MN_Packet::Deallocate(myCurrentPacket);
	}
	
	for (int i = 0; i < myPendingPackets.Count(); i++)
		MN_Packet::Deallocate(myPendingPackets[i]);
	myPendingPackets.RemoveAll();

	myLastDelimiterWriteOffset = 0;
	myPendingPacketMessageSize = 0;
}

//empty
bool MN_Message::Empty()
{
	if(!myCurrentPacket)
		return true;
	else
		return false;
}

unsigned int MN_Message::GetMessageSize( void )
{
	if( myCurrentPacket )
		return myPendingPacketMessageSize + myCurrentPacket->GetBinarySize(); 
	else
		return myPendingPacketMessageSize;
}
