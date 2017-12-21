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

#ifndef MN_SOCKETUTILS_H
#define MN_SOCKETUTILS_H

#include "mn_platform.h"

class MN_SocketUtils
{
public:

	//make address from dotted ip or hostname and port
	// Saves nslookup in MN_DNSCache if it's initialized. Works without.
	static bool MakeAddress(SOCKADDR_IN& anAddress, const char* anIpAddress, unsigned short aPort);

	//make address from dotted ip or hostname and port
	// Saves nslookup in MN_DNSCache if it's initialized. Works without.
	static bool GetInetAddress(struct in_addr& anAddress, const char* anIpAddress);

	//local host, specify port
	static bool MakeLocalHostAddress(SOCKADDR_IN& anAddress, unsigned short aPort);

	//compare addresses (return true if same)
	static bool IsSameAddress(const SOCKADDR_IN& anAddress, const SOCKADDR_IN& anotherAddress);

	// Creates a netmask with aNumBits valid bits
	static int CreateNetmask( int aNumBits );

	// Translates a numerical ipstring (format 192.168.0.1) to an int (0xC0A80001)
	static int CreateIP( const char* anIP );

	// Translates an integer IP (0xC0A80001) to a numerical ipstring (format 192.168.0.1)
	// ASSUMES THAT anIP IS ALLOCATED TO AT LEAST 16 bytes
	static void CreateIP( int anIP, char*& aString );

private:

	//dirty last resort way of getting local host address
	static bool DirtyUDP_GetLocalHostAddress(SOCKADDR_IN& aLocalAddress, unsigned short aPort);
	
	//constructor
	MN_SocketUtils();

	//destructor
	~MN_SocketUtils();
};

#endif