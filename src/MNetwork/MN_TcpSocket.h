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
//mn_tcpsocket.h
//basic TCP/IPbased network communications

#ifndef MN_TCPSOCKET_H
#define MN_TCPSOCKET_H

#include "mn_platform.h"
#include "mc_string.h"
#include "mc_circulararray.h"
#include "mc_growingarray.h"

#define MAX_INTERNAL_PACKETS 500

class MN_TcpSocket
{
public:
	enum SendResponse
	{
		FAILED = 0,
		WOULDBLOCK = 1,
		OK = 2
	};

	//creation
	static MN_TcpSocket* Create(bool aShouldBlock=false);
	static MN_TcpSocket* Create(const SOCKET& aConnectedWinsock, const SOCKADDR_IN& aRemoteAddress, bool aShouldBlock=false);
	static void Update();

	//destructor
	~MN_TcpSocket();
	
	//bind the socket (name it with a port number), and listen for clients
	//only used for servers
	bool BindAndListen(const unsigned short aPortNumber);

	// bind the socket to some local address and port 
	bool Bind(const unsigned int anAddress, 
			  const unsigned short aPortNumber, 
			  const bool aReuseAddress); 

	bool Listen(); 

	//accept a possible pending connection
	MN_TcpSocket* AcceptConnection();

	//connect to an address
	bool Connect(const SOCKADDR_IN& anAddress);
	
	//connect to an address
	bool Connect(const unsigned int anAddress, 
				 const unsigned short aPortNumber);

	//disconnect the socket
	void Disconnect(bool theDisconnectIsGraceful=true);

	//send a raw data buffer
	bool SendBuffer(char* aBuffer, int aBufferLength);
	
	//recieve to a raw data buffer
	bool RecvBuffer(char* aBuffer, int aBufferLength, int& aNumBytesRead);

	//get bound address of this socket
	bool GetLocalAddress(SOCKADDR_IN& anAddress);
	bool GetPeerAddress(SOCKADDR_IN& anAddress);

	//get remote address that this socket is connected to
	SOCKADDR_IN GetRemoteAddress()	{return myRemoteAddress;}

	//return local host name string
	const char* GetLocalHostName();

	//check if connected
	bool IsConnected();

	//set to true if the operations on this socket should block
	bool SetNonBlocking();
	bool IsBlocking();

	bool DisableNagle( bool aFlag );

	//debug
	static void EchoWSAError(const char* aFunctionCall);

	void SetSocketInteralRecvBufferSize(unsigned int aBufferSize);
	void SetSocketInteralSendBufferSize(unsigned int aBufferSize);

	void Print(); 

	unsigned int GetLastActivityTimestamp() { return myLastActivity; }
	SOCKET GetSocket() { return myWinsock; }

private:

	//constructor (NULL)
	MN_TcpSocket();

	//default init
	bool Init(bool aShouldBlock=false);
	
	//make tcpsocket from a connected winsock (used by server)
	bool Init(const SOCKET& aConnectedWinsock, const SOCKADDR_IN& aRemoteAddress, bool aShouldBlock=false);
	
	bool StoreBufferInternal(const char* aBuffer, int aBufferLength);
	SendResponse InternalSend(char* aBuffer, int aBufferLength);

	//winsock socket
	SOCKET myWinsock;
	
	//remote address (connected to, only valid if actually connected)
	SOCKADDR_IN myRemoteAddress;

	//local host name string
	MC_String myLocalHostName;

	//is this socket blocking or not?
	bool myIsBlocking;

	// Are we currently connecting? If so, we must use another connect() pipe
	bool myIsConnecting;

	char* myInternalBuffer;
	int myInternalReadPointer;
	int myInternalWritePointer;

	MC_CircularArrayT<int, MAX_INTERNAL_PACKETS>* myInternalPackets;
	static long volatile ourNumBuffersAllocated;

	unsigned int myLastActivity;

	static MC_GrowingArray<MN_TcpSocket*> ourActiveSockets;
	static unsigned int ourLastActiveSocketCheck;
	static const unsigned int GetTimeDiff( const unsigned int aStartTime, const unsigned int aEndTime );
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
