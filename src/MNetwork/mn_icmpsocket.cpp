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
//mn_tcpsocket.cpp
//basic TCP/IP based network communications

#include "stdafx.h"
#include "mn_icmpsocket.h"
#include "MN_SocketUtils.h"
#include "MN_WinsockNet.h"

struct ICMP_HEADER {	/* ICMP as per RFC 792 */
	u_char	icmp_type;		/* type of message */
	u_char	icmp_code;		/* type sub code */
	u_short icmp_cksum;		/* ones complement cksum */
	u_short	icmp_id;		/* identifier */
	u_short	icmp_seq;		/* sequence number */
	char	icmp_data[1];	/* data */
};

struct IP_HEADER {	/* IP version 4 as per RFC 791 */
	u_char	ip_hl;		/* header length */
	u_char	ip_v;		/* version */
	u_char	ip_tos;		/* type of service */
	short	ip_len;		/* total length */
	u_short	ip_id;		/* identification */
	short	ip_off;		/* fragment offset field */
	u_char	ip_ttl;		/* time to live */
	u_char	ip_p;		/* protocol */
	u_short	ip_cksum;		/* checksum */
	struct	in_addr ip_src;	/* source address */
	struct	in_addr ip_dst;	/* destination address */
};

#define PINGBUFSIZE 2048 + sizeof(ICMP_HEADER) + sizeof(IP_HEADER)

//constructor (NULL)
MN_IcmpSocket::MN_IcmpSocket()
{
	//init socket as nothing
	myWinsock = INVALID_SOCKET;
}


//destructor
MN_IcmpSocket::~MN_IcmpSocket()
{
	int err;
	const int BUFSIZE = 512;
	char buf[BUFSIZE];

	if(myWinsock != INVALID_SOCKET)
	{
		//done sending
		mn_shutdown(myWinsock, 1);

		//recieve and discard all pending data
		//until error occurs or nothing left to recv
		do
		{
			err = mn_recv(myWinsock, buf, BUFSIZE, 0);
		}
		while(err != SOCKET_ERROR && err != 0);

		//close socket
		mn_closesocket(myWinsock);
		myWinsock = INVALID_SOCKET;
	}
}
	
//creation
MN_IcmpSocket* MN_IcmpSocket::Create()
{
	MN_IcmpSocket* sock = NULL;

	sock = new MN_IcmpSocket();
	assert(sock);
	if(sock)
	{
		if(!sock->Init())
		{
			delete sock;
			sock = NULL;
		}
	}

	return sock;
}

//default init
bool MN_IcmpSocket::Init()
{

#if IS_PC_BUILD == 0		// SWFM:AW - To get the xb360 to compile
	return false;
#else

	unsigned long nonBlocking = 1;

	//open socket
	//udp socket
	myWinsock = mn_socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

	if(myWinsock == SOCKET_ERROR)
	{
		EchoWSAError("icmp_socket()");
		return false;
	}

	//make socket nonblocking (nonzero "1" enables nonblocking)
	if(mn_ioctlsocket(myWinsock, FIONBIO, &nonBlocking) == SOCKET_ERROR)
	{
		EchoWSAError("ioctlsocket()");
		return false;
	}

	return true;
#endif		//SWFM:AW
}

bool MN_IcmpSocket::SendPing( const in_addr* anAddress, const int anID, const int aSeqNum, const int aBufferLength, float aTime )
{
	int nRet;
	u_short i;
	char c;

	SOCKADDR_IN destAddr;

	char pingBuffer[PINGBUFSIZE];
	memset( pingBuffer, 0, PINGBUFSIZE );

	destAddr.sin_family = PF_INET;
	destAddr.sin_port = htons((u_short)4242);  // TODO: Dummy port 4242 ok?

	destAddr.sin_addr = *anAddress;

	
	ICMP_HEADER* icmpHeader;

	/*--------------------- init ICMP header -----------------------*/
	icmpHeader = (ICMP_HEADER FAR *)pingBuffer;
	icmpHeader->icmp_type  = ICMP_ECHOREQ;
	icmpHeader->icmp_code  = 0;
	icmpHeader->icmp_cksum = 0;
	icmpHeader->icmp_id	  = anID;
	icmpHeader->icmp_seq   = aSeqNum;
	*icmpHeader->icmp_data = 0;

	memcpy (&(pingBuffer[sizeof(ICMP_HEADER)]),&aTime,sizeof(float));

	c = 32;  // Start data after time with space
	for (i=sizeof(ICMP_HEADER)+sizeof(long); ((i < (aBufferLength+sizeof(ICMP_HEADER))) && (i < PINGBUFSIZE)); i++) 
	{
		pingBuffer[i] = c;
		c++;
		if (c > 126)	/* go up to ASCII 126, then back to 32/space */
			c = 32;
	}

   /* ----------------------assign ICMP checksum ----------------------   *
	*  ICMP checksum includes ICMP header and data, and assumes current   *
	*   checksum value of zero in header                                  */
	icmpHeader->icmp_cksum = CalcChecksum((u_short FAR *)icmpHeader, aBufferLength+sizeof(ICMP_HEADER));

				  // socket			   buffer	          length							  flags			destination		address length 
	nRet = mn_sendto(myWinsock, (LPSTR)icmpHeader, aBufferLength+sizeof(ICMP_HEADER)+sizeof(long), 0, (SOCKADDR*)&destAddr, sizeof(SOCKADDR_IN));   

	if (nRet == SOCKET_ERROR) 
	{
		EchoWSAError("icmp_sendbuffer()");
	}	

	return (nRet != SOCKET_ERROR);
}

bool MN_IcmpSocket::SendPing( const char* anAddress, const int anID, const int aSeqNum, const int aBufferLength, float aTime )
{
	SOCKADDR_IN destAddr;

	if( MN_SocketUtils::MakeAddress( destAddr, anAddress, 4242) == false )	// TODO: Dummy port ok?
		return false; // Hostname lookup failed or something

	return SendPing( &destAddr.sin_addr, anID, aSeqNum, aBufferLength, aTime );
}

bool MN_IcmpSocket::RecvReply(unsigned short* anID, unsigned short* aSeqNum, SOCKADDR_IN* aFromAddr, float* aReplyTime)
{
	int nAddrLen = sizeof(SOCKADDR_IN);
	int nRet, i;

	char pingBuffer[PINGBUFSIZE];
	ICMP_HEADER* icmpHeader;

	memset( pingBuffer, 0, PINGBUFSIZE );

	SOCKADDR_IN stFromAddr;
	/*-------------------- receive ICMP echo reply ------------------*/
	stFromAddr.sin_family = AF_INET;
	stFromAddr.sin_addr.s_addr = INADDR_ANY;  /* not used on input anyway */
	stFromAddr.sin_port = 0;   /* port not used in ICMP */
					/* socket */  /* buffer */		/* length */										    /* flags */    /* source */       /* addrlen*/
 	nRet = mn_recvfrom (myWinsock, (LPSTR)pingBuffer, PINGBUFSIZE+sizeof(ICMP_HEADER)+sizeof(long)+sizeof(IP_HEADER), 0, (LPSOCKADDR)aFromAddr, &nAddrLen);                 

	if (nRet == SOCKET_ERROR) 
	{
		EchoWSAError("icmp_recvfrom()");
		return false;
	}

 /*------------------------- parse data ---------------------------
  * remove the time from data and display with current time.
  *  NOTE: the data received and sent are asymmetric: we receive 
  *  the IP header, although we didn’t send it. This subtlety is
  *  often missed by WinSocks so we do a quick check of the data
  *  received to see if it includes the IP header (we look for 0x45
  *  value in first byte of buffer to check if IP header present).
  *
  * figure out the offset to data */

	if (pingBuffer[0] == 0x45) 
	{  /* IP header present? */
		i = sizeof(IP_HEADER) + sizeof(ICMP_HEADER);
		icmpHeader = (ICMP_HEADER*)&pingBuffer[sizeof(IP_HEADER)-4];
	}
	else
	{
		i = sizeof(ICMP_HEADER);	
		icmpHeader = (ICMP_HEADER*)pingBuffer;
	}
  
	/* pull out the ICMP ID and Sequence numbers */
	*anID  = icmpHeader->icmp_id;
	*aSeqNum = icmpHeader->icmp_seq;
   		
	/* remove the send time from the ICMP data */
	memcpy (aReplyTime, (&pingBuffer[i-4]), sizeof(float));
   		
	return true;
} /* end icmp_recvfrom() */



void MN_IcmpSocket::EchoWSAError(const char* aFunctionCall)
{
	int error = WSAGetLastError();
	
	//echo to console
	if(error != WSAEWOULDBLOCK)
	{
/*		if(Console::Instance())
			Console::Instance()->Out("WSAError: %d while calling: %s\n", error, aFunctionCall);*/
	}
}

/*-----------------------------------------------------------
 * Description:
 *  Calculate Internet checksum for data buffer and length (one’s 
 *  complement sum of 16-bit words).  Used in IP, ICMP, UDP, IGMP.
 */
unsigned short MN_IcmpSocket::CalcChecksum(unsigned short FAR*aBuffer, int aLength)
{	
  register long cksum = 0L;	/* work variables */
		
  /* note: to handle odd number of bytes, last (even) byte in 
   *  buffer have a value of 0 (we assume that it does) */
  while( aLength > 0 )
  {
    cksum += *(aBuffer++);	/* add word value to sum */
    aLength -= 2;          /* decrement byte count by 2 */
  }
  /* put 32-bit sum into 16-bits */
  cksum = (cksum & 0xffff) + (cksum>>16);
  cksum += (cksum >> 16);

  /* return Internet checksum.  Note:integral type
   * conversion warning is expected here. It's ok. */
  return (unsigned short)(~cksum); 
}  /* end cksum() */

