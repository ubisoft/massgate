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
#ifndef MN_WINSOCKNET_H
#define MN_WINSOCKNET_H

//#define MN_WINSOCKNET_PRETTY_PRINT_DATA

// include normal winsock 2 definitions
#include "mn_platform.h"

// Also include newer definition overrides!
#if IS_PC_BUILD
	#include <Ws2tcpip.h>
#endif

#include "MT_Thread.h"
#include "MT_Mutex.h"
#include "MT_Semaphore.h"
#include "MC_GrowingArray.h"
#include "mc_string.h"

SOCKET mn_socket(int af, int type, int protocol);
int mn_ioctlsocket(SOCKET s, long cmd, u_long* argp);
int mn_closesocket(SOCKET s);
int mn_shutdown(SOCKET s, int how);
int mn_send(SOCKET s, const char* buf, int len, int flags);
int mn_sendto(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen);
int mn_recv(SOCKET s, char* buf, int len, int flags);
int mn_recvfrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen);
int mn_bind(SOCKET s, const struct sockaddr* name, int namelen);

class MN_WinsockNet
{
public:

	//start/stop network
	static bool Create(unsigned short theWinsockMajorVersion = 2, unsigned short theWinsockMinorVersion = 0); // Default to winsock 2.0 (MMS uses 0x22, maybe we should on client too?)
	static bool Destroy();

	static bool Initialized()	{return ourInitializedFlag;}

	static void InitiateGracefulSocketTeardown(SOCKET aSocket);	

	static const char* DumpNetworkInformation();
	static void PostInitialize();

	static const char* GetErrorDescription(int errorCode);


	// Wireless support functions
	static bool IsConnectionWireless(); // Returns true if we have a wireless network card that is up
	static float GetWirelessSignalStrength(); // Returns signal-strength from 0.0f (none) to 1.0f (full) if IsConnectionWireless(), else undefined.
	
	// Move these somewhere else! MC_Platform?
	// Returns false if we are not running on a laptop
	// sets batteryPercent (0.0-1.0), batterytimeLeft (seconds, or INT_MAX if running on AC power), isUsingWireless, and if so sets wirelessStrength to 0.0-1.0	
	static bool GetIntelLaptopInfo(float& batteryPercent, unsigned int& batteryTimeLeft, bool &isUsingWireless, float& wirelessStrength);
	static bool IsLaptop();

	static unsigned __int64 GetMacAddress() { return ourMacAddress; }; 

	class SocketTeardownThread : public MT_Thread
	{
	public:
		SocketTeardownThread();
		void QueueSocketForDeletion(SOCKET aSocket);
		void Run();
	private:
		MT_Mutex myMutex;
		MC_GrowingArray<unsigned int> myDeletionSockets;
	};

	static bool ourDoLogBandwidthFlag;

private:

	//constructor
	MN_WinsockNet();

	//destructor
	~MN_WinsockNet();

	static bool ourInitializedFlag;
	static SocketTeardownThread* ourSocketTeardownThread;

	static unsigned __int64 ourMacAddress; 

};

struct MN_WinsockNetStats 
{
	MN_WinsockNetStats() { memset(this, 0, sizeof(*this)); }
	char		 firstUpAdapterName[96];
	char		 firstUpMedium[96];
	unsigned int numInterfaces;
	unsigned int numUpInterfaces;
	unsigned int numDownInterfaces;
	unsigned int numDnsServers;
	unsigned int maxMtu;
	unsigned int dhcpEnabled;
};

#endif
