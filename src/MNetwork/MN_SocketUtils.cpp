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
//mn_socketutils.h
//utilities for socket classes

#include "stdafx.h"

#include "mc_debug.h"
#include "mn_socketutils.h"
#include "mn_dnscache.h"
#include "MN_WinsockNet.h"

//constructor
MN_SocketUtils::MN_SocketUtils()
{

}

//destructor
MN_SocketUtils::~MN_SocketUtils()
{

}

/*
 * MakeAddress()
 *	Make address from dotted ip or hostname and port
 *	
 *	Uses MN_DNSCache if it's initialized. Otherwise, not.
 *	
 *	in_addr - resulting address will be stored here. Use inet_ntoa(anAddr) to convert to decimal IP
 *	anIpAddress - IP address / domain name to lookup
 *	return - false if lookup failed.
 */
bool MN_SocketUtils::GetInetAddress(in_addr& anAddr, const char* anIpAddress)
{
	anAddr.S_un.S_addr = inet_addr(anIpAddress);

	//if invalid address, try as hostname
	if(anAddr.S_un.S_addr == INADDR_NONE)
		if( MN_DNSCache::GetInstance() )
		{
			MC_StaticString<64> ip = MN_DNSCache::GetIP(anIpAddress);
			anAddr.S_un.S_addr = inet_addr(ip);
		}
		else
		{
			anAddr = MN_DNSCache::GetIPAddr( anIpAddress );  // GetIPAddr works even if DNSCache isn't 'used'
		}

	return (anAddr.S_un.S_addr != INADDR_NONE);
}

//make address from dotted ip or hostname and port
bool MN_SocketUtils::MakeAddress(SOCKADDR_IN& anAddress, const char* anIpAddress, unsigned short aPort)
{
	//init structure (assume dotted address)
	anAddress.sin_family = PF_INET;
	anAddress.sin_port = htons((u_short)aPort);

	return GetInetAddress( anAddress.sin_addr, anIpAddress );
}


//local host, specify port
bool MN_SocketUtils::MakeLocalHostAddress(SOCKADDR_IN& anAddress, unsigned short aPort)
{

	char hostName[MAX_PATH];
	
	//get local host name
	if(gethostname(hostName, MAX_PATH) != SOCKET_ERROR)
	{
		//try to resolve hostname
		if(MakeAddress(anAddress, hostName, aPort))
			return true;
	}

	//try quick and dirty UDP method
	return DirtyUDP_GetLocalHostAddress(anAddress, aPort);
}


//get local host address with specified port
//NOTE: This trick is nasty
bool MN_SocketUtils::DirtyUDP_GetLocalHostAddress(SOCKADDR_IN& anAddress, unsigned short aPort)
{
	SOCKET udpSocket;
	SOCKADDR_IN arbAddress;
	int SOCKADDRSIZE = sizeof(SOCKADDR);

	//get an udp socket
	udpSocket = mn_socket(PF_INET, SOCK_DGRAM, 0);
	if(udpSocket != INVALID_SOCKET)
	{
		//connect to arbitrary port and address (NOT loopback)
		arbAddress.sin_family = PF_INET;
		arbAddress.sin_port = htons(29999);
		arbAddress.sin_addr.s_addr = inet_addr("128.127.50.1");

		if(connect(udpSocket, (LPSOCKADDR)&arbAddress, sizeof(SOCKADDR)) != SOCKET_ERROR)
		{
			//get local address
			getsockname(udpSocket, (LPSOCKADDR)&anAddress, &SOCKADDRSIZE);

			//close socket
			mn_closesocket(udpSocket);

			//set our desired port
			anAddress.sin_port = htons(aPort);

			//success
			return true;
		}

		//close socket
		mn_closesocket(udpSocket);
	}

	return false;
}


//compare addresses (return true if same)
bool MN_SocketUtils::IsSameAddress(const SOCKADDR_IN& anAddress, const SOCKADDR_IN& anotherAddress)
{
	//compare ip (as unsigned long) and port (unsigned short)
	return (anAddress.sin_addr.S_un.S_addr == anotherAddress.sin_addr.S_un.S_addr &&
			anAddress.sin_port == anotherAddress.sin_port);
}

int MN_SocketUtils::CreateNetmask( int aNumBits )
{
	int netmask, i;

	// Shift each bit and set to 1 as needed.
	for( i=0,netmask=0; i<32; netmask <<= 1,i++ )  
		if( i<=aNumBits ) 
			netmask |= 1;
	return netmask;
}

int MN_SocketUtils::CreateIP( const char* anIP )
{
	int tmp1, tmp2, tmp3, tmp4;
	int numParsed = sscanf( anIP, "%d.%d.%d.%d", &tmp1, &tmp2, &tmp3, &tmp4 );	// Read ip from string (format 192.168.0.1)
	assert(numParsed == 4);
	return ( tmp1 << 24 | tmp2 << 16 | tmp3 << 8 | tmp4 );		// Shift read chars and OR them together
}

void MN_SocketUtils::CreateIP( int anIP, char*& aString )
{
	char* ip = (char*)&anIP;
	sprintf( aString, "%d.%d.%d.%d", (unsigned char)ip[3], (unsigned char)ip[2], (unsigned char)ip[1], (unsigned char)ip[0] );
}
