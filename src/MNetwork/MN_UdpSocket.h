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
//mn_udpsocket.h
//basic UDP based network communications

#ifndef MN_UDPSOCKET_H
#define MN_UDPSOCKET_H

#include "mn_platform.h"

class MN_UdpSocket
{
public:

	//UTILITY
	//make a broadcast address
	static void MakeBroadcastAddress(SOCKADDR_IN& anAddress, const unsigned short aPortNumber);
	//UTILITY
	
	//creation
	static MN_UdpSocket* Create(const bool anEnableBroadcastFlag);
	
	//destructor
	virtual ~MN_UdpSocket();
	
	//bind the socket (name it with a port number)
	//only used for servers
	bool Bind(const unsigned short aPortNumber);

	//send a raw data buffer
	//(to send to the INADDR_BROADCAST address, this options must have been specified on construct.)
	//(use MakeBroadcastAddress() to send to INADDR_BROADCAST.)
	virtual bool SendBuffer(const SOCKADDR_IN& aTargetAddress, const void* aBuffer, int aBufferLength);
	
	// Send raw data to myDestinationAddress
	bool SendBuffer( const void* aBuffer, int aBufferLength);

	//recieve to a raw data buffer
	//(will receive packets directed to the INADDR_BROADCAST address is this options was specified on construct)
	bool RecvBuffer(SOCKADDR_IN& aSourceAddress, char* aBuffer, int aBufferLength, int& aNumReadBytes);

	//return local host name string
	char* GetLocalHostName();

	//set to true if the operations on this socket should block
	bool setBlocking(bool);

	void SetSocketInteralRecvBufferSize(unsigned int aBufferSize);
	void SetSocketInteralSendBufferSize(unsigned int aBufferSize);

	static int ourLastError;

	// Destination address used with SendBuffer( buffer, len );
	SOCKADDR_IN myDestinationAddress;

	//winsock socket
	SOCKET myWinsock;


protected:

	//constructor (null)
	MN_UdpSocket();

	//init
	bool Init(const bool anEnableBroadcastFlag);

private:

	//debug
	void EchoWSAError(const char* aFunctionCall);

	//local host name string
	char myLocalHostName[MAX_PATH];

	//has sent something
	bool myHasSentBufferFlag;
};

#endif


/*
 * All Windows Sockets error constants are biased by WSABASEERR from
 * the "normal"

#define WSABASEERR              10000
/*
 * Windows Sockets definitions of regular Microsoft C error constants

#define WSAEINTR                (WSABASEERR+4)
#define WSAEBADF                (WSABASEERR+9)
#define WSAEACCES               (WSABASEERR+13)
#define WSAEFAULT               (WSABASEERR+14)
#define WSAEINVAL               (WSABASEERR+22)
#define WSAEMFILE               (WSABASEERR+24)

/*
 * Windows Sockets definitions of regular Berkeley error constants

#define WSAEWOULDBLOCK          (WSABASEERR+35)
#define WSAEINPROGRESS          (WSABASEERR+36)
#define WSAEALREADY             (WSABASEERR+37)
#define WSAENOTSOCK             (WSABASEERR+38)
#define WSAEDESTADDRREQ         (WSABASEERR+39)
#define WSAEMSGSIZE             (WSABASEERR+40)
#define WSAEPROTOTYPE           (WSABASEERR+41)
#define WSAENOPROTOOPT          (WSABASEERR+42)
#define WSAEPROTONOSUPPORT      (WSABASEERR+43)
#define WSAESOCKTNOSUPPORT      (WSABASEERR+44)
#define WSAEOPNOTSUPP           (WSABASEERR+45)
#define WSAEPFNOSUPPORT         (WSABASEERR+46)
#define WSAEAFNOSUPPORT         (WSABASEERR+47)
#define WSAEADDRINUSE           (WSABASEERR+48)
#define WSAEADDRNOTAVAIL        (WSABASEERR+49)
#define WSAENETDOWN             (WSABASEERR+50)
#define WSAENETUNREACH          (WSABASEERR+51)
#define WSAENETRESET            (WSABASEERR+52)
#define WSAECONNABORTED         (WSABASEERR+53)
#define WSAECONNRESET           (WSABASEERR+54)
#define WSAENOBUFS              (WSABASEERR+55)
#define WSAEISCONN              (WSABASEERR+56)
#define WSAENOTCONN             (WSABASEERR+57)
#define WSAESHUTDOWN            (WSABASEERR+58)
#define WSAETOOMANYREFS         (WSABASEERR+59)
#define WSAETIMEDOUT            (WSABASEERR+60)
#define WSAECONNREFUSED         (WSABASEERR+61)
#define WSAELOOP                (WSABASEERR+62)
#define WSAENAMETOOLONG         (WSABASEERR+63)
#define WSAEHOSTDOWN            (WSABASEERR+64)
#define WSAEHOSTUNREACH         (WSABASEERR+65)
#define WSAENOTEMPTY            (WSABASEERR+66)
#define WSAEPROCLIM             (WSABASEERR+67)
#define WSAEUSERS               (WSABASEERR+68)
#define WSAEDQUOT               (WSABASEERR+69)
#define WSAESTALE               (WSABASEERR+70)
#define WSAEREMOTE              (WSABASEERR+71)

/*
 * Extended Windows Sockets error constant definitions

#define WSASYSNOTREADY          (WSABASEERR+91)
#define WSAVERNOTSUPPORTED      (WSABASEERR+92)
#define WSANOTINITIALISED       (WSABASEERR+93)
#define WSAEDISCON              (WSABASEERR+101)
#define WSAENOMORE              (WSABASEERR+102)
#define WSAECANCELLED           (WSABASEERR+103)
#define WSAEINVALIDPROCTABLE    (WSABASEERR+104)
#define WSAEINVALIDPROVIDER     (WSABASEERR+105)
#define WSAEPROVIDERFAILEDINIT  (WSABASEERR+106)
#define WSASYSCALLFAILURE       (WSABASEERR+107)
#define WSASERVICE_NOT_FOUND    (WSABASEERR+108)
#define WSATYPE_NOT_FOUND       (WSABASEERR+109)
#define WSA_E_NO_MORE           (WSABASEERR+110)
#define WSA_E_CANCELLED         (WSABASEERR+111)
#define WSAEREFUSED             (WSABASEERR+112)
*/