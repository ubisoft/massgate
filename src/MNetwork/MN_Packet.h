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
//MN_Packet
//atomic network packet


#ifndef MN_PACKET_H
#define MN_PACKET_H

#include "MC_GrowingArray.h"

const int MN_PACKET_DATA_SIZE = 1364;


// Note! This class is very tied to Message, in that the upper two bits of Size is protocol flags. Keep them in
// transmission but do not get fooled by "large" sizes.

class MN_Packet
{
public:
	// Creation with UDP as default MTU
	static MN_Packet*		Create(unsigned int theTransportSize=512);

	// Creation from existing packet. Will also get the same mtu
	static MN_Packet*		Create(const MN_Packet* aPacket);

	// Delete the packet. The calling pointer should be invalidated immediately after calling this.
	static void				Deallocate(MN_Packet*& aPacket);

	
	// Call this to deallocate the statically allocated packets;
	static void				CleanUp();
	
	// Size of packet including headers and data, i.e. all that is necessary for sending on the network
	inline unsigned short	GetBinarySize()	const	{ return (sizeof(myWriteOffset) + GetWriteOffset() * sizeof(unsigned char)); }

	// Returns the full data (including any headers) which is very suitable for sending on the network
	const unsigned char*	GetBinaryData() const	{ return myRawDataBuffer; }

	// Geez, I wonder what this does
	void					operator=( const MN_Packet& aPacket );

	// 
	const unsigned short	GetWriteOffset() const	{ return myWriteOffset&0x3fff; }

	//
	void					SetWriteOffset(unsigned short aWo) { assert((aWo < myRawDataBufferLen)&&(aWo >= 0)); myWriteOffset = aWo; }

	// Gets the data in the packet - EXCLUDING any headers. Don't send this over the network
	const unsigned char*	GetData() const			{ assert(myRawDataBuffer); return myRawDataBuffer + sizeof(short);	}

	// Append data to the packet
	void					AppendData(const void* someData, unsigned int someDatalen);

	// Clear the contents of the packet
	void					Clear();

	const unsigned short	GetPacketDataCapacity() { return myTransportSize - sizeof(short); }

private:
	//constructors
	MN_Packet();
	MN_Packet(unsigned char* anAllocatedBuffPtr, unsigned int theTransportSize);
	explicit MN_Packet(const MN_Packet& aPacket);

	//destructor
	virtual ~MN_Packet();

	
	unsigned char*	myRawDataBuffer;
	unsigned int	myRawDataBufferLen;

	//offset for current write position in data
	unsigned short& myWriteOffset;
	unsigned short myTransportSize;


	static MN_Packet* GetNewPacket(unsigned int theTransportSize);
};



#endif
