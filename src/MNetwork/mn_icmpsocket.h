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
#ifndef __MN_ICMPSOCKET_H__
#define __MN_ICMPSOCKET_H__

#include "mn_platform.h"
#include "mc_string.h"

/*	
 * ICMP Socket.	
 *	
 *	At the moment it only supports ICMP_ECHOREQUEST (ping) and it's reply. 
 *	
 *	TODO: 
 *	Support at least traceroute
 *	
 */

#define ICMP_ECHOREPLY	0		// Echo reply: ping reply
#define ICMP_DESTUNREACH	3	// Destination unreachable
#define ICMP_ECHOREQ	8		// Echo request: ping request

class MN_IcmpSocket
{
public:

	// Call create to get an instance of MN_IcmpSocket
	static MN_IcmpSocket* Create();

	// Call Sendbuffer to send a ping request to anAddress
	// anID, aSeqNum is returned from the pingee. aSendLength is the number of bytes data to send in one ping (excl headers).
	bool SendPing( const char* aAddress, int anID = 0, int aSeqNum = 0, int aSendLength = 64, float aTime = 0 );
	bool SendPing( const in_addr* aAddress, int anID = 0, int aSeqNum = 0, int aSendLength = 64, float aTime = 0 );

	// Call RecvAnswer to check if a ping reply exist.
	// anID, aSeqNum are the same as used in SendPing(). Returns true if a reply was received
	bool RecvReply(unsigned short* anID, unsigned short* aSeqNum, SOCKADDR_IN* aFromAddr, float* aReplyTime );
	
	//destructor
	~MN_IcmpSocket();
	
	//debug
	static void EchoWSAError(const char* aFunctionCall);
private:

	// Function to calculate 16bit Checksum in ICMP Header
	static unsigned short CalcChecksum(unsigned short FAR*aBuffer, int aLength);

	//constructor (NULL)
	MN_IcmpSocket();

	//default init
	bool Init();
	
	//winsock socket
	SOCKET myWinsock;
};

#endif __MN_ICMPSOCKET_H__
