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
#include "MN_WinsockNet.h"
#include "MMG_ReliableUdpSocket.h"
#include "MC_Debug.h"
#include "MC_Profiler.h"
#include "ct_assert.h"
#include "MN_SocketUtils.h"
#include "MMG_Cryptography.h"
#include "MMG_CryptoHash.h"
#include "MMG_Util.h"
//#include "MI_Time.h"
#include <math.h>

#include "mc_prettyprinter.h"

unsigned int rudp_num_bytes_sent = 0;
unsigned int rudp_num_bytes_recv = 0;
unsigned int rudp_num_packets_sent = 0;
unsigned int rudp_num_packets_recv = 0;
unsigned int rudp_num_resends = 0;
unsigned int rudp_srtt_num_measures = 0;
float rudp_srtt_cumulative = 0;

const float			MMG_ReliableUdpSocket::RetransmissionTimer::Initial =							3.0f;
const float			MMG_ReliableUdpSocket::RetransmissionTimer::Min =								1.0f;
const float			MMG_ReliableUdpSocket::RetransmissionTimer::Max =								60.0f;
const float			MMG_ReliableUdpSocket::RetransmissionTimer::InitialAlpha =						0.125f;
const float			MMG_ReliableUdpSocket::RetransmissionTimer::InitialBeta =						0.250f;
const unsigned long	MMG_ReliableUdpSocket::Association_t::MaxResends =								5;
const float			MMG_ReliableUdpSocket::Association_t::HeartbeatInterval =						45.0f;


unsigned int MMG_ReliableUdpSocket::MAX_UDP_PACKET_SIZE = 512; // Todo: have an option to negotiate max packet size during connect - only useful for long lived connections
unsigned int MMG_ReliableUdpSocket::MAX_UDP_DATA_SIZE = MAX_UDP_PACKET_SIZE - sizeof(Footer);
unsigned int MMG_ReliableUdpSocket::MAX_RUDP_PACKET_SIZE = MAX_UDP_PACKET_SIZE*1;
unsigned int MMG_ReliableUdpSocket::MAX_RUDP_DATA_SIZE = MAX_UDP_DATA_SIZE*1;

const unsigned int MMG_ReliableUdpSocket::Handshake::MAGIC = Handshake().myMagic;
const unsigned int MMG_ReliableUdpSocket::Handshake::PROTOCOLVERSION = Handshake().myProtocolVersion;


int MMG_GetLastSocketError(SOCKET theSocketDescriptor)
{
	int res=0;
	int resa = WSAGetLastError();
	int resLen = sizeof(res);
#if IS_PC_BUILD		// SWFM:AW - To get the xb360 to compile
	getsockopt(theSocketDescriptor, SOL_SOCKET, SO_ERROR, (char*)&res, &resLen);
#endif
	if (res==0)	
		res = resa;
	return res;
}


MMG_ReliableUdpSocket::MMG_ReliableUdpSocket(bool aShouldBlock, void* aTagAlongState)
: myIsBlocking(aShouldBlock)
, myTagAlongState(aTagAlongState)
, mySocketStateListener(NULL)
{
	unsigned long yes = 1, no = 0, block=aShouldBlock?0:1;
	memset(&mySocket, 0, sizeof(mySocket));
	memset(&myRecentlyReceivedPackets, 0, sizeof(myRecentlyReceivedPackets));
	myRecentlyReceivedIndex = 0;

	mySocket.mySockDesc = mn_socket(AF_INET, SOCK_DGRAM, 0);
	mySocket.errCode = mn_ioctlsocket(mySocket.mySockDesc, FIONBIO, &block);
	if (0 != mySocket.errCode)
		MC_DEBUG( " could not ioctlsocket(): %d", MMG_GetLastSocketError(mySocket.mySockDesc));

	mySentUnconfirmedPackets.Init(10,5, false);
	myNotYetAckedSequenceNumbers.Init(10,5, false);
	myIncomingDataBuffer.Init(10,10,false);

	mySequenceNumber = SequenceNumber(unsigned long long(this));
	myCurrentPacketBufferIndex = 0;
	memset(myPacketBuffer, 0, sizeof(myPacketBuffer));
	myState = NOT_CONNECTED;
	myStatus = STATUS_OK;

	Association.TimeOfLastActivity = Association.TimeOfLastReceive = myCurrentTime = float(GetTickCount()) / 1000.0f;
	Association.CurrentNumberOfResends = 0;
	Association.NextHeartbeatDeadline = myCurrentTime + Association.HeartbeatInterval;
	Association.myLastRoundtripTime = 0.0f;

	RTO.Current = RTO.Initial;
	RTO.Alpha = RTO.InitialAlpha;
	RTO.Beta = RTO.InitialBeta;
	RTO.SRRT = 0.0f;
	RTO.RTTVar = 0.0f;
	RTO.myRttMeasureState = RetransmissionTimer::NO_MEASURE_PENDING;
	RTO.myCurrentMeasuredRoundtrip = 0;
	RTO.myHasAnyMeasurement = false;

	mySocket.myServerSocketDesc = -1;

	shouldAbort = false;
}

MMG_ReliableUdpSocket::~MMG_ReliableUdpSocket()
{
	if (mySocketStateListener)
		mySocketStateListener->OnDelete(this);

	if (myState != CLOSED)
		MC_DEBUG("Hard abortive shutdown. Pending acks and whatnot discarded.");
	Abort();
	myNotYetAckedSequenceNumbers.RemoveAll();
	mySentUnconfirmedPackets.DeleteAll();
	myIncomingDataBuffer.DeleteAll();
}

void
MMG_ReliableUdpSocket::SetTagAlongData(void* aTagAlongData)
{
	assert((myTagAlongState == NULL)||(aTagAlongData == NULL));
	myTagAlongState = aTagAlongData;
}

void*
MMG_ReliableUdpSocket::GetTagAlongData()
{
	return myTagAlongState;
}

void
MMG_ReliableUdpSocket::SetStateListener(ISocketStateListener* aListener)
{
	mySocketStateListener = aListener;
}


unsigned long 
MMG_ReliableUdpSocket::GetPeerIp() const
{
	return mySocket.myPeerAddr.sin_addr.s_addr;
}

const char*	
MMG_ReliableUdpSocket::GetPeerIpString() const
{
	return inet_ntoa(mySocket.myPeerAddr.sin_addr);
}


unsigned short 
MMG_ReliableUdpSocket::GetPeerPort() const
{
	return ntohs(mySocket.myPeerAddr.sin_port);
}

float
MMG_ReliableUdpSocket::GetRoundtripTime() const
{
	return RTO.SRRT;
}

float
MMG_ReliableUdpSocket::GetLastRoundtripTime() const
{
	return Association.myLastRoundtripTime;
}

float
MMG_ReliableUdpSocket::GetTimeSinceLastReceive() const
{
	return Association.TimeOfLastReceive - myCurrentTime;
}

float
MMG_ReliableUdpSocket::GetIdleTime() const
{
	return myCurrentTime - Association.TimeOfLastActivity;
}

float
MMG_ReliableUdpSocket::GetTimeUntilNextScheduledSend() const
{
	float t = 99999.0f;
	const unsigned int count = mySentUnconfirmedPackets.Count();
	for (unsigned int i = 0; i < count; i++)
		t = __min(t, mySentUnconfirmedPackets[i]->myNextSendTime - myCurrentTime);
	return t;
}


void 
MMG_ReliableUdpSocket::Connect(const char* aDestination, unsigned short thePort)
{
	assert(myState == NOT_CONNECTED);
	Association.TimeOfLastActivity = myCurrentTime;
	Association.CurrentNumberOfResends = 0;

	mySocket.myAddr.sin_family = AF_INET;
	mySocket.myAddr.sin_addr.s_addr = INADDR_ANY;
	mySocket.myAddr.sin_port = 0;

	mySocket.errCode = mn_bind(mySocket.mySockDesc, (const SOCKADDR*)&mySocket.myAddr, sizeof(mySocket.myAddr));
	if (0 != mySocket.errCode)
		MC_DEBUG( " could not bind(): %d", MMG_GetLastSocketError(mySocket.mySockDesc));

	int len = sizeof(mySocket.myAddr);
	int errc = getsockname(mySocket.mySockDesc, (SOCKADDR*)&mySocket.myAddr, &len);
	if (errc != 0)
	{
		assert(false);
	}
	mySocket.myPeerAddr.sin_family = PF_INET;
	mySocket.myPeerAddr.sin_port = htons(thePort);
	if (!MN_SocketUtils::MakeAddress(mySocket.myPeerAddr, aDestination, thePort))
	{
		MC_DEBUG( " could not find server %s:%u.", aDestination, thePort);
		mySocket.errCode = WSAEADDRNOTAVAIL;
	}

	// Reliably send a connection request
	myState = HANDSHAKE;
	myStatus = STATUS_OK;
	Handshake h;
	h.myType = Handshake::CONNECT;
	mySocket.myServerSocketDesc = -1;
	Send((const char*)&h, sizeof(Handshake));
}

void
MMG_ReliableUdpSocket::Connect(Socket* aServersocket, SOCKADDR_IN* aDestination)
{
	assert(myState == NOT_CONNECTED);
	assert(aDestination);
	Association.TimeOfLastActivity = myCurrentTime;
	Association.CurrentNumberOfResends = 0;

	mySocket.myServerSocketDesc = aServersocket->mySockDesc;
	memcpy(&mySocket.myServerSocket, aServersocket, sizeof(SOCKADDR_IN));

	mySocket.myAddr.sin_family = AF_INET;
	mySocket.myAddr.sin_addr.s_addr = INADDR_ANY;
	mySocket.myAddr.sin_port = 0;


	mySocket.errCode = mn_bind(mySocket.mySockDesc, (const SOCKADDR*)&mySocket.myAddr, sizeof(mySocket.myAddr));
	if (0 != mySocket.errCode)
		MC_DEBUG( " could not bind(): %d", MMG_GetLastSocketError(mySocket.mySockDesc));

	connect(mySocket.mySockDesc, (SOCKADDR*)aDestination, sizeof(SOCKADDR_IN));

	myState = HANDSHAKE;
	myStatus = STATUS_OK;
	memcpy(&mySocket.myPeerAddr, aDestination, sizeof(SOCKADDR_IN));


//	if (0 != (mySocket.errCode = connect(mySocket.mySockDesc, (const SOCKADDR*)&mySocket.myPeerAddr, sizeof(mySocket.myPeerAddr))))
//		MC_DEBUG( " could not connect to server. Errorcode: %d", MMG_GetLastSocketError(mySocket.mySockDesc));

}

unsigned int 
MMG_ReliableUdpSocket::Receive(void* theData, unsigned int theMaxDatalen)
{
	int numBytes = 0;
	if (myState == ACTIVE)
		numBytes = myReceive(theData, theMaxDatalen);
	assert(numBytes != 1);
	return numBytes;
}

unsigned int
MMG_ReliableUdpSocket::myReceive(void* const theData, unsigned int theMaxDatalen)
{
	ct_assert<sizeof(SequenceNumber) == 2>();

	if ( (myState == NOT_CONNECTED) || (myState == CLOSED) )
		return 0;

	if (shouldAbort)
	{
		Abort();
		myStatus = PEER_DOWN;
		mySocket.errCode = WSAECONNRESET;
		return 0;
	}

	if (mySocket.errCode != 0) 
		return 0;
	int addrSize = sizeof(mySocket.myPeerAddr);
	int numBytes = 0;
	int errc=0;
	if (myIncomingDataBuffer.Count())
	{
		Data_t* dataInBuffer = myIncomingDataBuffer[0];
		numBytes = myIncomingDataBuffer[0]->myDatalen;
		errc = myIncomingDataBuffer[0]->myErrorCode;
		if (numBytes != -1)
			memcpy(theData, dataInBuffer->myData, numBytes);
		delete dataInBuffer;
		myIncomingDataBuffer.RemoveAtIndex(0);
	}
	else if (mySocket.myServerSocketDesc == -1)
	{
		numBytes = mn_recvfrom(mySocket.mySockDesc, (char*)theData, theMaxDatalen, 0, (SOCKADDR*)&mySocket.myPeerAddr, &addrSize);
		if (numBytes == 0)
		{
			assert(false); // peer issued a graceful down. Should not happen on message-oriented sockets!
			return 0;
		}
		else if (numBytes == SOCKET_ERROR)
		{
			errc = MMG_GetLastSocketError(mySocket.mySockDesc);
		}
		else
		{
			rudp_num_bytes_recv += numBytes;
			rudp_num_packets_recv++;
		}
	}
	if (numBytes == SOCKET_ERROR)
	{
		switch(errc)
		{
		case WSAEMSGSIZE:
			MC_DEBUG("Received more than theMaxDatalen - DROPPING IT. Increase your buffers if you wanted it.");
		case WSAEWOULDBLOCK:// nothing to receive
			return 0;
		case WSAECONNRESET: // previous ::send() resulted in ICMP port unreachable. Peer is not up.
			Abort();
			myStatus = PEER_DOWN;
			mySocket.errCode = errc;
			return 0;
		default:
			mySocket.errCode = errc;
		}
		MC_DEBUG( " error receiving: %u", mySocket.errCode);
		return 0;
	}
	else if (numBytes >= sizeof(Footer))
	{
		Association.TimeOfLastActivity = myCurrentTime;
		Association.TimeOfLastReceive = myCurrentTime;
#ifdef MMG_RELIABLEUDPSOCKET_PRETTYPRINT_ALL_SOCKET_DATA
		MC_DEBUG("Received from %s:%u on port %u", inet_ntoa(mySocket.myPeerAddr.sin_addr), int(ntohs(mySocket.myPeerAddr.sin_port)), int(ntohs(mySocket.myAddr.sin_port)));
		MDB_PrettyPrinter::ToDebug((const char*)theData, numBytes);
#endif
		Footer* footer = (Footer*)(((char*)theData)+numBytes-sizeof(Footer));

		// the packet contains an ok footer. process any acks in it.
		for (unsigned int nack = 0; nack < unsigned int(footer->myFeatures & 0x0f); nack++)
			myReceivedAck(footer->myAcks[nack]);

		// did we get a partial packet? Should not happen with udp, but you never know..
		if (unsigned int(numBytes) < sizeof(Footer) + footer->myDatalen)
			return 0;

		if (footer->myDatalen > 0)
			myNotYetAckedSequenceNumbers.AddUnique(footer->myMsgId);

		if ((myState == ACTIVE) && ((footer->myFeatures & 0xf0) == HEARTBEAT_PACKET))
		{
//			MC_DEBUG("responding to heartbeat.");
			Send(NULL, 0, false, HEARTBEAT_ACK_PACKET);
			return 0;
		}
		if ((myState == ACTIVE) && ((footer->myFeatures & 0xf0) == HEARTBEAT_ACK_PACKET))
		{
			// We got a HEARTBEAT ACK
			Association.CurrentNumberOfResends = 0;
			Association.NextHeartbeatDeadline = GetNextHeartbeatDeadline();
//			MC_DEBUG("Got HEARTBEAT ACK");
			return 0;
		}
		if ((myState != HANDSHAKE) && ((footer->myFeatures & 0xf0) == HANDSHAKE_PACKET))
			return 0; // we got a leftover from the handshake process
		if ((myState != HANDSHAKE) && (*((unsigned int*)theData) == 'rudp'))
		{
			//assert(false); 
			return 0;
		}

		// Have we recently received the packet already? (Sender may have missed our ack)
		if (myHaveRecentlyReceived(footer->myMsgId))
			return 0;


		if (footer->myDatalen > 0)
		{
			// got an ok message
		}

		return footer->myDatalen;

	}
	return 0;
}

MN_ConnectionErrorType 
MMG_ReliableUdpSocket::Send(const void* theData, unsigned int theDatalen)
{
	if (myState == CLOSE_WAIT)
		return MN_CONN_BROKEN;
	if (Send((const char*)theData, theDatalen, true))
		return MN_CONN_OK;
	return MN_CONN_BROKEN;
}

bool 
MMG_ReliableUdpSocket::Send(const char* const theData, unsigned int theDatalen, bool aIsReliableFlag, PacketType thePacketType)
{
	if ((myState == CLOSED) || (myState == NOT_CONNECTED) )
		return false;

	bool status = true;

	ct_assert<sizeof(SequenceNumber) == 2>();

	const char* srcData = theData;

	unsigned int packlen = min(theDatalen, MAX_UDP_DATA_SIZE);
	theDatalen -= packlen;

	assert(theDatalen == 0);

	Packet* packet = myAllocateNewPacket(packlen);

	memcpy(packet->myData, srcData, packlen);
	srcData += packlen;
	packet->myFooter->myDatalen = packlen;
	packet->myFooter->myMsgId = myGetNextSequenceNumber();
	if ((myState == HANDSHAKE) && (packlen == sizeof(Handshake)) && (((unsigned int*)theData)[0]==Handshake::MAGIC))
	{
		packet->myFooter->myFeatures = HANDSHAKE_PACKET;
	}
	else
		packet->myFooter->myFeatures = thePacketType;

	packet->mySentCounter = 0;
	status = SendPacket(packet, aIsReliableFlag);

	return status;
}

void
MMG_ReliableUdpSocket::SendHeartbeat()
{
	Send(NULL, 0, false, HEARTBEAT_PACKET);
}

bool
MMG_ReliableUdpSocket::SendPacket(Packet* thePacket, bool aIsReliableFlag)
{
	int sendstatus=0;

	if ((myState == CLOSED) || (myState == NOT_CONNECTED))
		return false;

	if (thePacket->myFooter->myDatalen)
	{
		switch(RTO.myRttMeasureState)
		{
		case RetransmissionTimer::MEASURE_PENDING:
			// Check if the measurement timed out
			if (RTO.myStartOfRttMeasure + RTO.Max < myCurrentTime)
			{
				RTO.myRttMeasureState = RetransmissionTimer::NO_MEASURE_PENDING;
//				MC_DEBUG("RTO measurement for %X timed out", thePacket->myFooter->myMsgId);
			}
			break;
		case RetransmissionTimer::NO_MEASURE_PENDING:
			// Begin a first measure, according to Karn's algorithm
			if (thePacket->mySentCounter == 0)
			{
				BeginRTOMeasurement(thePacket->myFooter->myMsgId);
			}
			else
			{
//				MC_DEBUG("%X Will not RTO on resends.", this);
			}
			break;
		default:
			assert(false);
		};
	}


	thePacket->myNextSendTime = RTO.Current + myCurrentTime;
	thePacket->mySentCounter++;

	ct_assert<sizeof(SequenceNumber) == 2>(); // the two switch()'s below uses byteshuffling.


	const unsigned int currNumAcks = thePacket->myFooter->myFeatures & 0x0f;
	const unsigned int maxAdditionalAcks = 3-currNumAcks;
	const unsigned int additionalAcks = min(maxAdditionalAcks, unsigned int(myNotYetAckedSequenceNumbers.Count()));
	thePacket->myFooter->myFeatures &= 0xf0;
	thePacket->myFooter->myFeatures |= additionalAcks + currNumAcks;
	for (unsigned int nack = currNumAcks; nack < currNumAcks + additionalAcks; nack++)
	{
		thePacket->myFooter->myAcks[nack] = myNotYetAckedSequenceNumbers[0];
		myNotYetAckedSequenceNumbers.RemoveCyclicAtIndex(0);
	}

	if ((myState == HANDSHAKE) && ((thePacket->myFooter->myFeatures&0x0f)==thePacket->myFooter->myFeatures))
	{
		// hey, we won't actually send another packet while we are handshaking
		// but we'll try to send it asap after handshake is complete.
		thePacket->myNextSendTime = 0;
		thePacket->mySentCounter = 0;
//		MC_DEBUG("ignored send during handshake");
		
	}
	else
	{
#ifdef MMG_RELIABLEUDPSOCKET_PRETTYPRINT_ALL_SOCKET_DATA
		if (mySocket.myServerSocketDesc != -1)
			MC_DEBUG("Sending to %s:%u from port %u", inet_ntoa(mySocket.myPeerAddr.sin_addr), ntohs(mySocket.myPeerAddr.sin_port), ntohs(mySocket.myServerSocket.sin_port));
		else
			MC_DEBUG("Sending to %s:%u from port %u", inet_ntoa(mySocket.myPeerAddr.sin_addr), ntohs(mySocket.myPeerAddr.sin_port), ntohs(mySocket.myAddr.sin_port));
		MDB_PrettyPrinter::ToDebug(thePacket->myData, thePacket->myFooter->myDatalen + sizeof(Footer));
#endif

#ifdef MMG_RELIABLEUDPSOCKET_SIMULATE_PACKET_LOSS_ON_SEND
		// MMG_RELIABLEUDPSOCKET_SIMULATE_PACKET_LOSS_ON_SEND must be a float between 0.0 and 1.0!
		ct_assert<int(MMG_RELIABLEUDPSOCKET_SIMULATE_PACKET_LOSS_ON_SEND) != MMG_RELIABLEUDPSOCKET_SIMULATE_PACKET_LOSS_ON_SEND>();
		if (rand() > MMG_RELIABLEUDPSOCKET_SIMULATE_PACKET_LOSS_ON_SEND * float(RAND_MAX))
		{
			if (mySocket.myServerSocketDesc != -1)
			{
				sendstatus = mn_sendto(mySocket.myServerSocketDesc, thePacket->myData, thePacket->myFooter->myDatalen+sizeof(Footer), 0, (const SOCKADDR*)&mySocket.myPeerAddr, sizeof(mySocket.myPeerAddr));
			}
			else
			{
				sendstatus = mn_sendto(mySocket.mySockDesc, thePacket->myData, thePacket->myFooter->myDatalen+sizeof(Footer), 0, (const SOCKADDR*)&mySocket.myPeerAddr, sizeof(mySocket.myPeerAddr));
			}
//			MC_DEBUG("%X SENDING", this);
		}
		else
		{
			sendstatus = thePacket->myFooter->myDatalen + sizeof(Footer);
//			MC_DEBUG("%X DROPPING PACKET", this);
		}
#else
		if (mySocket.myServerSocketDesc != -1)
		{
			sendstatus = mn_sendto(mySocket.myServerSocketDesc, thePacket->myData, thePacket->myFooter->myDatalen+sizeof(Footer), 0, (const SOCKADDR*)&mySocket.myPeerAddr, sizeof(mySocket.myPeerAddr));
		}
		else
		{
			sendstatus = mn_sendto(mySocket.mySockDesc, thePacket->myData, thePacket->myFooter->myDatalen+sizeof(Footer), 0, (const SOCKADDR*)&mySocket.myPeerAddr, sizeof(mySocket.myPeerAddr));
		}
#endif

		if (sendstatus == SOCKET_ERROR)
		{
			int errc = MMG_GetLastSocketError(mySocket.mySockDesc);
			switch(errc)
			{
			case WSAEWOULDBLOCK:
				break;
			case WSAEMSGSIZE:
				MC_DEBUG("Trying to send more than MTU - DROPPING IT.");
				assert(false);
			case WSAENETDOWN:
			case WSAEHOSTUNREACH:
			case WSAECONNRESET: // previous ::send() resulted in ICMP port unreachable. Peer is not up.
				MC_DEBUG("Closing connection");
				
				Abort();
				myStatus = PEER_DOWN;
			default:
				MC_DEBUG("Send error: %u", errc);
				
				mySocket.errCode = errc;
				return false;
			}
		}
		else
		{
			rudp_num_bytes_sent += sendstatus;
			rudp_num_packets_sent++;
		}
	}

	if (aIsReliableFlag)
	{
		// removed addunique failed check - if this happens, there is no point in returning false
		mySentUnconfirmedPackets.AddUnique(thePacket);
		return true;
	}
	else
	{
		delete thePacket;
		return true;
	}
	assert(thePacket);
	return sendstatus == thePacket->myFooter->myDatalen+sizeof(Footer);
}

void
MMG_ReliableUdpSocket::Close(bool aForceImmediateCloseFlag)
{
	if ((myState == ACTIVE) || (myState == HANDSHAKE))
		myState = CLOSE_WAIT;
	else if (myState != CLOSE_WAIT)
		myState = CLOSED;
	if ((myState == CLOSE_WAIT) && aForceImmediateCloseFlag)
		Abort();
}

bool
MMG_ReliableUdpSocket::IsConnected() const
{ 
	return (myState == ACTIVE) && (myStatus == STATUS_OK); 
}

void
MMG_ReliableUdpSocket::Abort()
{
	if (myState == CLOSED)
		return;
	if (mySocket.mySockDesc > 0)
	{
//		char buff[512];
		mn_shutdown(mySocket.mySockDesc, SD_BOTH);
//		if (!myIsBlocking)
//			while(recv(mySocket.mySockDesc, buff, sizeof(buff), 0) > 0)
//				;
		mn_closesocket(mySocket.mySockDesc);
		mySocket.mySockDesc = 0;
	}

	myNotYetAckedSequenceNumbers.RemoveAll();
	mySentUnconfirmedPackets.DeleteAll();
	myIncomingDataBuffer.DeleteAll();
	myState = CLOSED;
	myStatus = STATUS_OK;
}

MMG_ReliableUdpSocket::Packet*
MMG_ReliableUdpSocket::myAllocateNewPacket(unsigned int theRequiredBufferlen)
{
	assert(myState != CLOSED);
	Packet* p = new Packet();
	p->myDataBufferlen = theRequiredBufferlen + sizeof(Footer);
	p->myData = new char [p->myDataBufferlen];

//	memset(p->myData, 0x11, p->myDataBufferlen);

	p->myFooter = (Footer*)(p->myData + theRequiredBufferlen);
	return p;
//	ct_assert<LOC_NUM_PACKETS_IN_BUFFER == 0x7f+1>();
//	myCurrentPacketBufferIndex &= 0x7f;
//	if (myPacketBuffer[myCurrentPacketBufferIndex] == NULL)
//	{
//		Packet* p = new Packet();
//		if (p == NULL)
//		{
//			MC_DEBUG(" out of memory while trying to allocate %u bytes.", sizeof(Packet));
//			assert(false && "out of memory");
//			return NULL;
//		}
//		p->myData = NULL;
//		p->myDataBufferlen = 0;
//		myPacketBuffer[myCurrentPacketBufferIndex] = p;
//	}
//
//	Packet* retPacket = myPacketBuffer[myCurrentPacketBufferIndex++];
//
//	if (retPacket->myDataBufferlen < theRequiredBufferlen + sizeof(Footer))
//	{
//		delete [] retPacket->myData;
//		retPacket->myDataBufferlen = theRequiredBufferlen + sizeof(Footer);
//		retPacket->myData = new char [retPacket->myDataBufferlen];
//		if (retPacket->myData == 0)
//		{
//			MC_DEBUG(" out of memory while trying to allocate %u bytes.", retPacket->myDataBufferlen);
//			assert(false && "out of memory");
//			return NULL;
//		}
//	}
//	retPacket->myFooter = (Footer*)(retPacket->myData + theRequiredBufferlen /*- sizeof(Footer)*/);
//	return retPacket;
}

void 
MMG_ReliableUdpSocket::myReceivedAck(const SequenceNumber theAckNum)
{
	Association.TimeOfLastActivity = myCurrentTime;
	if (RTO.myRttMeasureState == RetransmissionTimer::MEASURE_PENDING)
	{
		if (theAckNum == RTO.myCurrentMeasuredRoundtrip)
		{
			// Ok, calculate the round-trip time
			const float R = fabsf(myCurrentTime - RTO.myStartOfRttMeasure);
			Association.myLastRoundtripTime = __max(0.0001f, R);
			if (RTO.myHasAnyMeasurement)
			{
				// RTTVAR <- (1 - RTO.Beta) * RTTVAR + RTO.Beta * |SRTT - R'|
				RTO.RTTVar = (1.0f - RTO.Beta) * RTO.RTTVar + RTO.Beta * fabsf(RTO.SRRT - Association.myLastRoundtripTime);
				// SRTT   <- (1 - RTO.Alpha) * SRTT + RTO.Alpha * R'
				RTO.SRRT   = (1.0f - RTO.Alpha) * RTO.SRRT + RTO.Alpha * Association.myLastRoundtripTime;
			}
			else // initial measurement
			{
				RTO.SRRT = Association.myLastRoundtripTime;
				RTO.RTTVar = Association.myLastRoundtripTime/2.0f;
				RTO.myHasAnyMeasurement = true;
			}
			// Adjust for clock granularity
			RTO.RTTVar = MMG_Util::Clamp<float>(RTO.RTTVar, 0.05f, RTO.RTTVar);
			// Set the retransmissiontimer
			RTO.Current = MMG_Util::Clamp<float>(RTO.SRRT + 4.0f * RTO.RTTVar, RTO.Min, RTO.Max);
//			MC_DEBUG("%X new srrt: %f", this, RTO.SRRT);

			rudp_srtt_num_measures++;
			rudp_srtt_cumulative += R;
		}
		else
		{
//			MC_DEBUG("Got out of sync (%X) measurement response. Clearing RTO.", theAckNum);
		}
		RTO.myRttMeasureState = RetransmissionTimer::NO_MEASURE_PENDING;
	}

	const unsigned int count = mySentUnconfirmedPackets.Count();
	for (unsigned int i = 0; i < count; i++)
	{
		if (mySentUnconfirmedPackets[i]->myFooter->myMsgId == theAckNum)
		{
			// Found the message we waited for an ack on. Reset heartbeat deadline since we know the association is fine
			Association.NextHeartbeatDeadline = GetNextHeartbeatDeadline();
			// We know the connection works
			Association.CurrentNumberOfResends = 0;
			// Remove packet from pending ack buffer
			mySentUnconfirmedPackets.DeleteCyclicAtIndex(i--);
			return;
		}
	}
}

MMG_ReliableUdpSocket::SequenceNumber
MMG_ReliableUdpSocket::myGetNextSequenceNumber()
{
	if (myNotYetAckedSequenceNumbers.Count() >= 1024)
	{
		MC_DEBUG( " BREAKING REALIABILITY DUE TO TOO SMALL SEQUENCESPACE");
		return mySequenceNumber-1;
	}
	SequenceNumber res;
	bool pending;
	do 
	{
		pending = false;
		res = mySequenceNumber++;
		const unsigned int count = myNotYetAckedSequenceNumbers.Count();
		for (unsigned int i=0; i<count; i++)
			if (myNotYetAckedSequenceNumbers[i] == res)
				pending = true;
	}
	while (pending);
	return res;
}

bool
MMG_ReliableUdpSocket::Update()
{
//	MC_DEBUGPOS();
	if (myState == CLOSED)
		return false;

	DWORD ticktime = GetTickCount();
	float newTime = float(GetTickCount() / 1000.0f);
	myCurrentTime = __max(newTime, myCurrentTime);

	if (Association.CurrentNumberOfResends > Association.MaxResends)
	{
		MC_DEBUG("%X RESEND TRIES EXCEEDED Association.MaxResends", this);

		shouldAbort = true;

//		Close();
		Abort();
		myStatus = PEER_DOWN;
		mySocket.errCode = WSAECONNRESET;
		return false;
	}


	if (myState == CLOSE_WAIT)
	{
		// our user has Close()'d us, but I still need to Receive() in order to detect ack's.
		// However, make sure that we only stay in CLOSED_WAIT for the duration of the RTO
		char buff[1024];
		while (myReceive(buff, sizeof(buff)))
			;
		if (Association.TimeOfLastReceive + RTO.Current*2 < myCurrentTime)
		{
			Abort();
			return false;
		}
	}
	else if (myState == HANDSHAKE)
	{
		// we have sent a reliable connect, but have we got a reply?
		char buff[512];
		if (myReceive(buff, sizeof(buff)) >= sizeof(Handshake))
		{
			Handshake* r = (Handshake*)buff;
			if ((r->myMagic == Handshake::MAGIC)&&(r->myType==Handshake::CONNECTION_ESTABLISHED)&&(r->myProtocolVersion == Handshake::PROTOCOLVERSION))
			{
				if((mySocket.errCode = connect(mySocket.mySockDesc, (SOCKADDR*)&mySocket.myPeerAddr, sizeof(mySocket.myPeerAddr))) != 0)
					MC_DEBUG(" Fatal trouble while establishing connection. Errcode: %d", MMG_GetLastSocketError(mySocket.mySockDesc));
				if (mySocket.errCode == 0)
				{
					// We are connected! Set states and initiate heartbeats
					myState = ACTIVE;
					myStatus = STATUS_OK;
					Association.CurrentNumberOfResends = 0;
					Association.NextHeartbeatDeadline = GetNextHeartbeatDeadline();
				}
			}
			else if (r->myProtocolVersion != Handshake::PROTOCOLVERSION)
			{
				MC_DEBUG("Protocol versions don't match.");
				Abort();
				myStatus = PROTOCOL_NOT_COMPATIBLE;
			}
			else if (r->myMagic != Handshake::MAGIC)
			{
				MC_DEBUG("Got totally unknown handshake");
				Abort();
				myStatus = PROTOCOL_NOT_COMPATIBLE;
			}
			else if (r->myType == Handshake::PROTOCOL_MISMATCH)
			{
				MC_DEBUG("Server kindly told us that our protocol version is wrong.");
				Abort();
				myStatus = PROTOCOL_NOT_COMPATIBLE;
			}
			else
			{
				assert(false && "fix the first if()");
			}
		}
	}
	if (myState == CLOSED)
		return false;

	// resend any sent messages that have not been acked within <duration> seconds
	bool hasNotSent = true;
	const unsigned int count = mySentUnconfirmedPackets.Count();
	for (unsigned int i=0; hasNotSent && (i < count); i++)
	{
		Packet* p = mySentUnconfirmedPackets[i];
		if ((myState == HANDSHAKE) && ((p->myFooter->myFeatures & 0xf0) != HANDSHAKE_PACKET))
			continue;
		if (p->myNextSendTime <= myCurrentTime)
		{
			float delta = 0.0f;
			if (myState != HANDSHAKE)
			{
				delta = RTO.Current;
				RTO.Current = MMG_Util::Clamp<float>(RTO.Current * 2.0f, RTO.Min, RTO.Max);
				delta = RTO.Current - delta;
			}
			rudp_num_resends++;
			Association.CurrentNumberOfResends++;
			// Just remove it - don't delete it! It will be added in SendPacket below
			mySentUnconfirmedPackets.RemoveCyclicAtIndex(i--);
			// Update the RTO for all pending packets
			for (int i = count-1-1; i>=0; i--) // -1 -1 is intentional.
				mySentUnconfirmedPackets[i]->myNextSendTime += delta;

			SendPacket(p, true);

			break;
		}
	}
	// send at most one empty packet with an ack. (in case client receives but never Send() again)
	if (myNotYetAckedSequenceNumbers.Count() != 0)
	{
		Send(NULL, 0, false);
	}

	// Send a heartbeat if it is due
	if (myState == ACTIVE)
	{
		if (myCurrentTime > Association.NextHeartbeatDeadline)
		{
			// Time to send a heartbeat
			Association.CurrentNumberOfResends++; // SCTP 8.3 Path Heartbeat: alternative algorithm
			// Set timer for when we expect the next heartbeat
			Association.NextHeartbeatDeadline = GetNextHeartbeatDeadline();
			// Send the heartbeat
			SendHeartbeat();
		}
	}

	return (mySocket.errCode == 0) && (myState != CLOSED);
}

bool
MMG_ReliableUdpSocket::myHaveRecentlyReceived(const SequenceNumber theId)
{
	for (unsigned int i=0; i < sizeof(myRecentlyReceivedPackets)/sizeof(SequenceNumber); i++)
		if (myRecentlyReceivedPackets[i] == theId)
			return true;
	// it wasn't there. add it!
	myRecentlyReceivedPackets[myRecentlyReceivedIndex++] = theId;
	if (myRecentlyReceivedIndex >= sizeof(myRecentlyReceivedPackets)/sizeof(SequenceNumber))
		myRecentlyReceivedIndex = 0;
	return false;
}

MMG_ReliableUdpSocket::ConnectionState
MMG_ReliableUdpSocket::GetState() const
{
	return myState;
}

MMG_ReliableUdpSocket::ErrorStatus
MMG_ReliableUdpSocket::GetStatus() const
{
	return myStatus;
}

float
MMG_ReliableUdpSocket::GetNextHeartbeatDeadline()
{
	// Setup heartbeat in RTO.Curret + 50% jitter on heartbeat interval
	const float jitter = Association.HeartbeatInterval * float(rand() - rand()) / float(unsigned long(RAND_MAX)<<1);
	const float retval = myCurrentTime + RTO.Current + Association.HeartbeatInterval + jitter;
	return retval;
}

void
MMG_ReliableUdpSocket::BeginRTOMeasurement(SequenceNumber thePacket)
{
	RTO.myCurrentMeasuredRoundtrip = thePacket;
	RTO.myStartOfRttMeasure = myCurrentTime;
	RTO.myRttMeasureState = RetransmissionTimer::MEASURE_PENDING;
}

bool 
MMG_ReliableUdpSocket::operator==(const MMG_ReliableUdpSocket& aRhs)
{
	return memcmp(&mySocket, &aRhs.mySocket, sizeof(mySocket)) == 0;
}

bool 
MMG_ReliableUdpSocket::operator!=(const MMG_ReliableUdpSocket& aRhs)
{
	return memcmp(&mySocket, &aRhs.mySocket, sizeof(mySocket)) != 0;
}

bool 
MMG_ReliableUdpSocket::operator<(const MMG_ReliableUdpSocket& aRhs)
{
	return memcmp(&mySocket, &aRhs.mySocket, sizeof(mySocket)) < 0;
}

bool 
MMG_ReliableUdpSocket::operator>(const MMG_ReliableUdpSocket& aRhs)
{
	return memcmp(&mySocket, &aRhs.mySocket, sizeof(mySocket)) > 0;
}

void 
MMG_ReliableUdpSocket::AddDataToInputBuffer(const void* theData, const unsigned long theDatalen, int theErrorCode)
{
	Data_t* data = new Data_t;
	if (theDatalen != -1)
		memmove(data->myData, theData, theDatalen);
	data->myDatalen = theDatalen;
	data->myErrorCode = theErrorCode;
	myIncomingDataBuffer.Add(data);
}

bool
MMG_ReliableUdpSocket::ServerQuestion_HasPendingData()
{
	return myIncomingDataBuffer.Count() > 0;
}

