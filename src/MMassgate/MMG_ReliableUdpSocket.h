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
#ifndef MMG_RELIABLE_UDP_SOCKET___H_
#define MMG_RELIABLE_UDP_SOCKET___H_

#include "MC_GrowingArray.h"
#include "MN_UdpSocket.h"
#include "MN_IWriteableDataStream.h"

#include "MC_Debug.h"

/* This class provides reliable UDP packet sending and receiving.
 *
 * Features:
 * - Automatic acknowledgement and resends of data
 * - Duplicate message detection, i.e. you will receive a single message only once.
 * - A successfull call to Receive() always matches what the peer sent with Send(). You will never get partial packets.
 * - Detects if the peer is or goes down
 *
 * Usage:
 * - If you allocated the MMG_ReliableUdpSocket (i.e. use it as a client towards a server) then you must call Update()
 *   as often as you can, typically once every frame. Never delete the socket before Update() returns false.
 * - If you got the socket from MMG_ReliableUdpSocketServer, then you do not have to call Update() and you should not delete it.
 *   See MMG_ReliableUdpSocketServer for more information.
 * - This class does not provide ordered delivery! All packets (regardless of size) are quaranteed to be delivered in full, 
 *   but not necessarily in order.
 * - Any writable buffers you pass on to functions will most probably be modified EVEN IF THE CALL FAILS.
 * - Always provide a larger buffer for Receive() than the data you expect to receive. Preferrably use MAX_UDP_PACKET_SIZE (~0.5-2Kb)
 *   if you know that the data will be less than a packet, or use as much as MAX_RUDP_PACKET_SIZE.


 * Todo:
 * - Rip out MC_GrowingArray for a priority queue so as to not actually worsen unordered delivery
 * - Negotiate largest MTU packet size during handshake (by first trying a large handshake - will work even with lousy routers)
 */






// if you want to display all raw data for debugging, enable the following line
//#define MMG_RELIABLEUDPSOCKET_PRETTYPRINT_ALL_SOCKET_DATA

// if you want to simulate packet loss, put the lossrate here (0.0 zero loss to 1.0 full loss)
//#define MMG_RELIABLEUDPSOCKET_SIMULATE_PACKET_LOSS_ON_SEND	(0.10f)


// Retrieve socket-specific errorcode - per socket. Better than WSAGetLastError() since it operates per thread.
int MMG_GetLastSocketError(SOCKET theSocketDescriptor);


#define MMG_RELIABLE_UDP_SOCKET_PROTOCOL_VERSION	( 10 )


#define LOC_NUM_PACKETS_IN_BUFFER (128)

class MMG_ReliableUdpSocket : public MN_IWriteableDataStream
{
public:
	typedef enum ConnectionState_t {	NOT_CONNECTED = 'con1', 
										HANDSHAKE, 
										ACTIVE, 
										CLOSE_WAIT, 
										CLOSED } ConnectionState;

	typedef enum ErrorStatus_t {		STATUS_OK = 'err0',
										PROTOCOL_NOT_COMPATIBLE,
										PEER_DOWN } ErrorStatus;

	typedef enum PacketTypes_t {		NORMAL_PACKET = 0x00,
										HANDSHAKE_PACKET = 0x10,
										HEARTBEAT_PACKET = 0x20,
										HEARTBEAT_ACK_PACKET = 0x30 } PacketType;


	class ISocketStateListener
	{
	public:
		virtual void OnDelete(MMG_ReliableUdpSocket* theSocket) = 0;
	};



	MMG_ReliableUdpSocket(bool aShouldBlock, void* aTagAlongState=NULL);
	~MMG_ReliableUdpSocket();

	// Connect to aDestination on thePort. You can send data immediately, but you should preferably wait until IsConnected()
	void			Connect(const char* aDestination, unsigned short thePort);

	// Receive data. NOTE! theMaxDatalen is the maximum size in theData buffer - not how many bytes you want to receive!
	// theMaxDatalen should be at least 8 bytes larger than real length of the data you expect.
	unsigned int	Receive(void* theData, unsigned int theMaxDatalen);

	// Send data, to be used by MN_WriteMessage. Same characteristics as bool Send(..., ..., true);
	virtual MN_ConnectionErrorType Send(const void* theData, unsigned int theDatalen);

	// Call update as often as you can (i.e. once every frame). If it returns false, the connection is broken.
	bool			Update();

	// The reliable udp socket employs a connection abstraction. The only way to know if the peer is running OK is to check IsConnected()
	bool			IsConnected() const;

	// Call this to close the socket. Unconfirmed packets will still be sent as long as Update is called.
	// Do not set aForceImmediateCloseFlag unless you really truly and utterly understand the consequences of doing so.
	void			Close(bool aForceImmediateCloseFlag=false);

	// Get the state of the socket.
	ConnectionState	GetState() const;

	// Get the last error
	ErrorStatus		GetStatus() const;

	float			GetIdleTime() const;
	float			GetTimeSinceLastReceive() const;

	float			GetRoundtripTime() const;
	float			GetLastRoundtripTime() const;

	float			GetTimeUntilNextScheduledSend() const;

	void			SetStateListener(ISocketStateListener* aListener);

	void			SetTagAlongData(void* aTagAlongData);
	void*			GetTagAlongData();

	// get the IP of the peer
	unsigned long	GetPeerIp() const;
	const char*		GetPeerIpString() const;

	// get the port of the peer
	unsigned short	GetPeerPort() const;

	// This is how much data you can receive on a single packet
	static unsigned int MAX_UDP_DATA_SIZE;

	// This is the minimum buffer size you must specify in calls to Receive in order to actually receive MAD_UDP_DATA_SIZE
	static unsigned int MAX_UDP_PACKET_SIZE;

	// This is the absolute largest data you can request to send in a single Send() call.
	static unsigned int MAX_RUDP_DATA_SIZE;

	// This is the absolute largest buffer you must provide in calls to Receive(). Unnecessary if you know you will get less than 500b.
	static  unsigned int MAX_RUDP_PACKET_SIZE;

	virtual unsigned int GetRecommendedBufferSize() { return MAX_RUDP_DATA_SIZE; }

	// some operators
	bool operator==(const MMG_ReliableUdpSocket& aRhs);
	bool operator!=(const MMG_ReliableUdpSocket& aRhs);
	bool operator<(const MMG_ReliableUdpSocket& aRhs);
	bool operator>(const MMG_ReliableUdpSocket& aRhs);

	void AddDataToInputBuffer(const void* theData, const unsigned long theDatalen, int theErrorCode);

protected:

	ISocketStateListener* mySocketStateListener;

	typedef unsigned short SequenceNumber;

	struct Handshake
	{
		typedef enum HandshakeType_t { NOTHING='bozo', CONNECT='conn', CONNECTION_ESTABLISHED='cack', PROTOCOL_MISMATCH='prot' } HandshakeType;

		const unsigned int myMagic;
		const unsigned int myProtocolVersion;
		HandshakeType myType;

		static const unsigned int MAGIC;
		static const unsigned int PROTOCOLVERSION;
		Handshake() : myMagic('rudp'), myProtocolVersion(MMG_RELIABLE_UDP_SOCKET_PROTOCOL_VERSION), myType(NOTHING) { }
	};

	struct Footer
	{
		SequenceNumber	myMsgId;
		unsigned short	myDatalen;
		unsigned char	myFeatures; //  0xAB where:
									//  if A = 1 this message is protocol control (HANDSHAKE)
									//  A=0 normal data, B=numAcks
									//  A=2 HEARTBEAT, B=numAcks (can add acks to heartbeat if desired)
									//  A=3 HEARTBEAT ACK, B=numAcks (can add acks to heartbeat ack if desired)
		SequenceNumber	myAcks[3];
	};

	struct Packet
	{
		Packet() { myNextSendTime = 1e37f; myDataBufferlen = 0; myFooter = NULL; myData = NULL;  }
		~Packet() { delete [] myData;	 }
		bool operator==(const Packet& aRhs) { return myFooter->myMsgId == aRhs.myFooter->myMsgId; }
		bool operator<(const Packet& aRhs) { return myFooter->myMsgId < aRhs.myFooter->myMsgId; }
		bool operator>(const Packet& aRhs) { return myFooter->myMsgId > aRhs.myFooter->myMsgId; }
		Footer*			myFooter;
		char*			myData;
		float			myNextSendTime;
		unsigned int	myDataBufferlen;
		unsigned long	mySentCounter;

	private:
		bool operator=(const Packet& aPacket) { }
	};

	struct Socket
	{
		SOCKET			mySockDesc;
		SOCKADDR_IN		myAddr;
		SOCKADDR_IN		myServerSocket;
		SOCKADDR_IN		myPeerAddr;
		SOCKET			myServerSocketDesc;
		int errCode;
	};

	// Force immediate close of the socket. Any pending data will be discarded.
	void			Abort();
	void			Connect(Socket* aServersocket, SOCKADDR_IN* aDestination); // used for a socket that is already connected
	void			SendHeartbeat();
	bool			Send(const char* const theData, unsigned int theDatalen, bool aIsReliableFlag=true, PacketType thePacketType=NORMAL_PACKET);
	bool			SendPacket(Packet* thePacket, bool aIsReliableFlag);
	void			myReceivedAck(const SequenceNumber theAckNum);
	unsigned int	myReceive(void* const theData, unsigned int theMaxDatalen);
	Packet*			myAllocateNewPacket(unsigned int theRequiredBufferlen);
	SequenceNumber	myGetNextSequenceNumber();
	bool			myHaveRecentlyReceived(const SequenceNumber theId);


	ConnectionState myState;
	ErrorStatus		myStatus;

	// only called by MMG_ReliableUdpSocketServer
	bool			ServerQuestion_HasPendingData();

private:

	struct RetransmissionTimer{
		/* 14. Suggested SCTP Protocol Parameter Values

		The following protocol parameters are RECOMMENDED:

		RTO.Initial              - 3  seconds
		RTO.Min                  - 1  second
		RTO.Max                 -  60 seconds
		RTO.Alpha                - 1/8
		RTO.Beta                 - 1/4
		Valid.Cookie.Life        - 60  seconds
		Association.Max.Retrans  - 10 attempts
		Path.Max.Retrans         - 5  attempts (per destination address)
		Max.Init.Retransmits     - 8  attempts
		HB.interval              - 30 seconds

		IMPLEMENTATION NOTE: The SCTP implementation may */
		static const float Initial;
		static const float Min;
		static const float Max;
		static const float InitialAlpha;
		static const float InitialBeta;

		float Current;
		float SRRT; // smoothed roundtrip time
		float RTTVar; // roundtrip variance
		float Alpha;
		float Beta;

		SequenceNumber	myCurrentMeasuredRoundtrip;
		float			myStartOfRttMeasure;
		bool			myHasAnyMeasurement;
		enum rtt_measurestate_t{ NO_MEASURE_PENDING, MEASURE_PENDING } myRttMeasureState ;

	}RTO;

	struct Association_t
	{
		static const unsigned long	MaxResends;
		static const float			HeartbeatInterval;

		float				NextHeartbeatDeadline;
		float				TimeOfLastReceive;
		float				TimeOfLastActivity;
		unsigned int		CurrentNumberOfResends;
		float				myLastRoundtripTime;
	}Association;



	MMG_ReliableUdpSocket() { assert(false); };
	MMG_ReliableUdpSocket(const MMG_ReliableUdpSocket& aRhs) { assert(false); };
	MMG_ReliableUdpSocket& operator=(const MMG_ReliableUdpSocket& aRhs) { assert(false); };



	float GetNextHeartbeatDeadline();
	void BeginRTOMeasurement(SequenceNumber thePacket);

	MC_GrowingArray<Packet*>						mySentUnconfirmedPackets;
	MC_GrowingArray<SequenceNumber>					myNotYetAckedSequenceNumbers;
	Packet*											myPacketBuffer[LOC_NUM_PACKETS_IN_BUFFER];
	SequenceNumber									myRecentlyReceivedPackets[32];
	unsigned int									myRecentlyReceivedIndex;
	unsigned int									myCurrentPacketBufferIndex;
	SequenceNumber									mySequenceNumber;
	Socket											mySocket;
	bool											myIsBlocking;
	float											myCurrentTime;
	friend class MMG_ReliableUdpSocketServerFactory;
	friend class SockAddrComparer;
	bool shouldAbort;
	void* myTagAlongState;

	struct Data_t
	{
		unsigned char myData[512];
		unsigned int myDatalen;
		int				myErrorCode;
	};
	MC_GrowingArray<Data_t*> myIncomingDataBuffer;
};

class SockAddrComparer
{
public:
	static bool LessThan(const MMG_ReliableUdpSocket* anObj1, const SOCKADDR_IN& from)
	{ 
		const SOCKADDR_IN& raddr = anObj1->mySocket.myPeerAddr;
		return (raddr.sin_addr.s_addr == from.sin_addr.s_addr) ? (raddr.sin_port < from.sin_port) : (raddr.sin_addr.s_addr < from.sin_addr.s_addr);
	}
	static bool LessThan(const MMG_ReliableUdpSocket* anObj1, const MMG_ReliableUdpSocket* anObj2)
	{ 
		const SOCKADDR_IN& raddr = anObj1->mySocket.myPeerAddr;
		const SOCKADDR_IN& from = anObj2->mySocket.myPeerAddr;
		return (raddr.sin_addr.s_addr == from.sin_addr.s_addr) ? (raddr.sin_port < from.sin_port) : (raddr.sin_addr.s_addr < from.sin_addr.s_addr);
	}

	static bool GreaterThan(const MMG_ReliableUdpSocket* anObj1, const SOCKADDR_IN& from)
	{ 
		const SOCKADDR_IN& raddr = anObj1->mySocket.myPeerAddr;
		return (raddr.sin_addr.s_addr == from.sin_addr.s_addr) ? (raddr.sin_port > from.sin_port) : (raddr.sin_addr.s_addr > from.sin_addr.s_addr);
	}
	static bool GreaterThan(const MMG_ReliableUdpSocket* anObj1, const MMG_ReliableUdpSocket* anObj2)
	{ 
		const SOCKADDR_IN& raddr = anObj1->mySocket.myPeerAddr;
		const SOCKADDR_IN& from = anObj2->mySocket.myPeerAddr;
		return (raddr.sin_addr.s_addr == from.sin_addr.s_addr) ? (raddr.sin_port > from.sin_port) : (raddr.sin_addr.s_addr > from.sin_addr.s_addr);
	}

	static bool Equals(const MMG_ReliableUdpSocket* anObj1, const SOCKADDR_IN& from) 
	{ 
		const SOCKADDR_IN& raddr = anObj1->mySocket.myPeerAddr;
		return (raddr.sin_addr.s_addr == from.sin_addr.s_addr) && (raddr.sin_port == from.sin_port);
	}
	static bool Equals(const MMG_ReliableUdpSocket* anObj1, const MMG_ReliableUdpSocket* anObj2) 
	{ 
		const SOCKADDR_IN& raddr = anObj1->mySocket.myPeerAddr;
		const SOCKADDR_IN& from = anObj2->mySocket.myPeerAddr;
		return (raddr.sin_addr.s_addr == from.sin_addr.s_addr) && (raddr.sin_port == from.sin_port);
	}
};


#endif
