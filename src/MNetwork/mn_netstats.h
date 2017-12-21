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

#ifndef __MN_NETSTATS_HH__
#define __MN_NETSTATS_HH__

#define MN_NETSTATS_NUM_SECONDS 120

class MN_NetStats
{
public:
	static bool Create( void );
	static void Destroy( void );
	static MN_NetStats* GetInstance( void ) { return ourInstance; }

	static void TCPSent( unsigned int aBytes );
	static void TCPReceived( unsigned int aBytes );

	static void UDPSent( unsigned int aBytes );
	static void UDPReceived( unsigned int aBytes );

	// Copies stats data into data buffer (NEED TO BE ALLOCATED!)
	void GetUDPBytesSent( unsigned int* someData ) ;
	void GetUDPBytesReceived( unsigned int* someData ) ;
	void GetUDPPacketsReceived( unsigned int* someData ) ;
	void GetUDPPacketsSent( unsigned int* someData ) ;

	void GetTCPBytesSent( unsigned int* someData ) ;
	void GetTCPBytesReceived( unsigned int* someData ) ;
	void GetTCPPacketsReceived( unsigned int* someData ) ;
	void GetTCPPacketsSent( unsigned int* someData ) ;

	void GetTotBytesSent( unsigned int* someData ) ;
	void GetTotBytesReceived( unsigned int* someData ) ;
	void GetTotPacketsReceived( unsigned int* someData ) ;
	void GetTotPacketsSent( unsigned int* someData ) ;

private:
	bool Init( void );

	void UpdateData( void );

	MN_NetStats( void );
	~MN_NetStats(void);

	static MN_NetStats* ourInstance;

	unsigned int myTCPSent[MN_NETSTATS_NUM_SECONDS];
	unsigned int myTCPReceived[MN_NETSTATS_NUM_SECONDS];
	unsigned int myTCPPacketsSent[MN_NETSTATS_NUM_SECONDS];
	unsigned int myTCPPacketsReceived[MN_NETSTATS_NUM_SECONDS];

	unsigned int myUDPSent[MN_NETSTATS_NUM_SECONDS];
	unsigned int myUDPReceived[MN_NETSTATS_NUM_SECONDS];
	unsigned int myUDPPacketsSent[MN_NETSTATS_NUM_SECONDS];
	unsigned int myUDPPacketsReceived[MN_NETSTATS_NUM_SECONDS];

	int myLastPos;
	float myLastTime;
};

#endif //__MN_NETSTATS_HH__
