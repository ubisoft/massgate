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
#include "mn_winsocknet.h"
#include "MC_Debug.h"
#include "MN_InducedLatencyHandler.h"
#include "mn_netstats.h"
#include "MC_Commandline.h"
#include "MN_UdpSocket.h"
#include "MT_ThreadingTools.h"
#include "MC_PrettyPrinter.h"
#include "MN_Resolver.h"
#include "MC_Profiler.h"
#include "MI_Time.h"

#include <Iphlpapi.h>
#include <NtDDNdis.h>
#include <winioctl.h>
#include <time.h>


#ifdef MN_WINSOCKNET_PRETTY_PRINT_DATA

#ifndef __FUNCTION__
#define __FUNCTION__ ""
#endif

#define PRETTY_PRINT_IT(DATA, LEN) \
	((LEN) > 0 ? (MC_DEBUGPOS(), MC_PrettyPrinter::ToDebug((DATA), (LEN)), true) : (true))

#else /* MN_WINSOCKNET_PRETTY_PRINT_DATA */ 

#define  PRETTY_PRINT_IT(DATA, LEN) \
	((void)0)

#endif /* MN_WINSOCKNET_PRETTY_PRINT_DATA */ 

typedef IP_ADAPTER_DNS_SERVER_ADDRESS_XP IP_ADAPTER_DNS_SERVER_ADDRESS;
typedef IP_ADAPTER_DNS_SERVER_ADDRESS_XP *PIP_ADAPTER_DNS_SERVER_ADDRESS;

bool MN_WinsockNet::ourInitializedFlag = false;
MN_WinsockNet::SocketTeardownThread* MN_WinsockNet::ourSocketTeardownThread = NULL;
unsigned __int64 MN_WinsockNet::ourMacAddress = 0; 

MN_WinsockNetStats winSockNetStats;

static MN_InducedLatencyHandler* locInducer = NULL;

static HANDLE locHandleToWirelessCard = NULL;

bool MN_WinsockNet::ourDoLogBandwidthFlag = false;

static void LogOutgoingBandwidth(int);
static void LogIncomingBandwidth(int);


//constructor
MN_WinsockNet::MN_WinsockNet()
{

}


//destructor
MN_WinsockNet::~MN_WinsockNet()
{

}


//start/stop network
bool MN_WinsockNet::Create(unsigned short theWinsockMajorVersion, unsigned short theWinsockMinorVersion)
{
	MC_PROFILER_BEGIN_ONE_SHOT(a, __FUNCTION__);

	MC_DEBUGPOS();
	WSADATA wsaData;
	int error;

	if (ourInitializedFlag)
		return true;

	//wsa startup

	error = WSAStartup(MAKEWORD(theWinsockMajorVersion, theWinsockMinorVersion), &wsaData);
	if(error != 0)
	{
		//no usable dll available
		MC_Debug::DebugMessage("WSAStartup failure, no usable WinSock DLL available");
		return false;
	}

	//confirm Winsock support
	if (wsaData.wVersion != MAKEWORD(theWinsockMajorVersion, theWinsockMinorVersion))
	{
		//no usable dll available
		MC_Debug::DebugMessage("DLL confirmation failure, no usable WinSockDLL available");
		WSACleanup( );
		return false;
	}


	//all is green, proceed
	ourInitializedFlag = true;

	MN_NetStats::Create();

	// Send a udp packet to ourselves to REALLY init the stack (will lockup loading dll's on first use otherwise)
	MN_UdpSocket* sock = MN_UdpSocket::Create(true);
	if (sock)
	{
		const char str[] = "\0\0force full winsock init";
		SOCKADDR_IN targetAdress;
		MN_UdpSocket::MakeBroadcastAddress(targetAdress, 3001);
		sock->Bind(INADDR_ANY);
		sock->SendBuffer(targetAdress, str, sizeof(str));
		delete sock;
		MC_Sleep(100);
	}

	return true;
}

void MN_WinsockNet::PostInitialize()
{
	MC_PROFILER_BEGIN_ONE_SHOT(a, __FUNCTION__);
	MC_DEBUGPOS();
	int induceLatency = 0;
	if (MC_CommandLine::GetInstance()->GetIntValue("ping", induceLatency))
	{
		induceLatency = __max(0, induceLatency); // no back to the future in mn_network. yet.
		locInducer = new MN_InducedLatencyHandler(induceLatency);
		locInducer->Start();
	}

	if (MC_CommandLine::GetInstance()->IsPresent("shownetusage"))
	{
		ourDoLogBandwidthFlag = true;
	}

	assert(ourSocketTeardownThread == NULL);
	ourSocketTeardownThread = new SocketTeardownThread();
	ourSocketTeardownThread->Start();
	MC_Debug::DebugMessage("Winsock initialization complete.");
}


bool MN_WinsockNet::Destroy()
{
	MC_DEBUGPOS();
	ourInitializedFlag = false;

	MN_Resolver::Destroy();

	if (locInducer)
	{
		locInducer->StopAndDelete();
		locInducer = NULL;
	}

	ourSocketTeardownThread->Stop();
	ourSocketTeardownThread->QueueSocketForDeletion(INVALID_SOCKET);
	ourSocketTeardownThread->StopAndDelete();

	MN_NetStats::Destroy();

	if(WSACleanup() == SOCKET_ERROR)
	{
		/*		if(Console::Instance())
		Console::Instance()->Out("MN_WinsockNet::StopNetwork: WSACleanup failure");*/
		return false;
	}

	return true;
}


SOCKET mn_socket(int af, int type, int protocol)
{
	SOCKET ret;
	if (locInducer == NULL)
		ret = ::socket(af, type, protocol);
	else 
		ret = locInducer->socket(af, type, protocol);
	return ret;
}

int mn_bind(SOCKET s, const struct sockaddr* name, int namelen)
{
	if (locInducer == NULL)
		return ::bind(s, name, namelen);
	else
		return locInducer->bind(s, name, namelen);
}

int mn_closesocket(SOCKET s)
{
	MC_DEBUG(" %u", s);
	if (locInducer == NULL)
		return ::closesocket(s);
	else
		return locInducer->closesocket(s);
}

int mn_shutdown(SOCKET s, int how)
{
	MC_DEBUG(" %u", s);
	if (locInducer == NULL)
		return ::shutdown(s, how);
	else 
		return locInducer->shutdown(s, how);
}

int mn_send(SOCKET s, const char* buf, int len, int flags)
{
	int bytesSent; 

	LogOutgoingBandwidth(len);

	if (locInducer == NULL)
		bytesSent = ::send(s, buf, len, flags);
	else
		bytesSent = locInducer->send(s, buf, len, flags);

	PRETTY_PRINT_IT(buf, bytesSent); 
	return bytesSent; 
}

int mn_sendto(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen)
{
	int bytesSent; 

	LogOutgoingBandwidth(len);

	if (locInducer == NULL)
		bytesSent = ::sendto(s, buf, len, flags, to, tolen);
	else
		bytesSent = locInducer->sendto(s, buf, len, flags, to, tolen);

	PRETTY_PRINT_IT(buf, bytesSent); 
	return bytesSent; 
}

int mn_recv(SOCKET s, char* buf, int len, int flags)
{
	int bytesReceived; 

	if (locInducer == NULL)
		bytesReceived = ::recv(s, buf, len, flags);
	else
		bytesReceived = locInducer->recv(s, buf, len, flags);

	LogIncomingBandwidth(bytesReceived);

	PRETTY_PRINT_IT(buf, bytesReceived); 
	return bytesReceived; 
}

int mn_recvfrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen)
{
	int bytesReceived; 

	if (locInducer == NULL)
		bytesReceived = ::recvfrom(s, buf, len, flags, from, fromlen);
	else
		bytesReceived = locInducer->recvfrom(s, buf, len, flags, from, fromlen);

	LogIncomingBandwidth(bytesReceived);

	PRETTY_PRINT_IT(buf, bytesReceived);
	return bytesReceived; 
}

int mn_ioctlsocket(SOCKET s, long cmd, u_long* argp)
{
	if (locInducer == NULL)
		return ::ioctlsocket(s, cmd, argp);
	else
		return locInducer->ioctlsocket(s, cmd, argp);
}

void MN_WinsockNet::InitiateGracefulSocketTeardown(SOCKET aSocket)
{
	MC_DEBUG(" %u", aSocket);

	if (aSocket != INVALID_SOCKET)
		ourSocketTeardownThread->QueueSocketForDeletion(aSocket);
}

MN_WinsockNet::SocketTeardownThread::SocketTeardownThread()
{
	myDeletionSockets.Init(4,4,true);
}

void 
MN_WinsockNet::SocketTeardownThread::QueueSocketForDeletion(SOCKET aSocket)
{
	if (aSocket != INVALID_SOCKET)
	{
		MC_DEBUG("Queuing %u for deletion", aSocket);

		mn_shutdown(aSocket, SD_BOTH);
	}
	else
	{
		MC_DEBUG("Shutting down MN_WinsockNet::SocketTeardownThread.");
	}

	MT_MutexLock locker(myMutex);
	myDeletionSockets.Add(aSocket);
}

void 
MN_WinsockNet::SocketTeardownThread::Run()
{
	char buff[1<<16];

	MT_ThreadingTools::SetCurrentThreadName("MN_WinsockNet::SocketTeardownThread");

	while (!StopRequested())
	{
		SOCKET sock;
		myMutex.Lock();
		if (myDeletionSockets.Count() == 0)
		{
			myMutex.Unlock();
			MC_Sleep(100);
			continue;
		}
		sock=myDeletionSockets[0];
		myMutex.Unlock();

		unsigned int numTries = 0;
		while (sock != INVALID_SOCKET)
		{
			bool closeNow = false;
			int status = mn_recv(sock, buff, sizeof(buff), 0);
			numTries++;
			if (status == 0)
				closeNow = true;
			else if (status == SOCKET_ERROR)
				if (WSAGetLastError() != WSAEWOULDBLOCK)
					closeNow = true;
			if (closeNow || (numTries == 10))
			{
				// Disabling lingering for abortive close
				LINGER  lingerStruct;
				lingerStruct.l_onoff = 1;
				lingerStruct.l_linger = 0;
				setsockopt(sock, SOL_SOCKET, SO_LINGER,  (char *)&lingerStruct, sizeof(lingerStruct));

				MC_DEBUG("Deleting socket %u", sock);
				mn_closesocket(sock);
				sock = INVALID_SOCKET;
			}
			MC_Sleep(100);
		}
		myMutex.Lock();
		myDeletionSockets.RemoveItemConserveOrder(0);
		myMutex.Unlock();
	}
	MC_DEBUG("Shutting down thread.");
}



///////////////////////////////////////////////////////////////////////////
///////////HELPER FUNCTION(S) FOR NETWORK INFORMATION//////////////////////
///////////////////////////////////////////////////////////////////////////

#if IS_PC_BUILD	// SWFM:SWR - pc only inclusion
#include <WS2spi.h>
#include <Iphlpapi.h>
#include <iptypes.h>
#include <Ipifcons.h>
#endif


//#include <Routprot.h> DO NOT INCLUDE THIS
// These are copied from it.
#define PROTO_IP_OTHER      1
#define PROTO_IP_LOCAL      2
#define PROTO_IP_NETMGMT    3
#define PROTO_IP_ICMP       4
#define PROTO_IP_EGP        5
#define PROTO_IP_GGP        6
#define PROTO_IP_HELLO      7
#define PROTO_IP_RIP        8
#define PROTO_IP_IS_IS      9
#define PROTO_IP_ES_IS      10
#define PROTO_IP_CISCO      11
#define PROTO_IP_BBN        12
#define PROTO_IP_OSPF       13
#define PROTO_IP_BGP        14

//
// The multicast protocol IDs
//

#define PROTO_IP_MSDP        9
#define PROTO_IP_IGMP       10
#define PROTO_IP_BGMP       11

//
// The IPRTRMGR_PID is 10000 // 0x00002710
//

#define PROTO_IP_VRRP               112
#define PROTO_IP_BOOTP              9999    // 0x0000270F
#define PROTO_IP_NT_AUTOSTATIC      10002   // 0x00002712
#define PROTO_IP_DNS_PROXY          10003   // 0x00002713
#define PROTO_IP_DHCP_ALLOCATOR     10004   // 0x00002714
#define PROTO_IP_NAT                10005   // 0x00002715
#define PROTO_IP_NT_STATIC          10006   // 0x00002716
#define PROTO_IP_NT_STATIC_NON_DOD  10007   // 0x00002717
#define PROTO_IP_DIFFSERV           10008   // 0x00002718
#define PROTO_IP_MGM                10009   // 0x00002719
#define PROTO_IP_ALG                10010   // 0x0000271A
#define PROTO_IP_H323               10011   // 0x0000271B
#define PROTO_IP_FTP                10012   // 0x0000271C
#define PROTO_IP_DTP                10013   // 0x0000271D
//////////////////////////////////////////////////////////////////////////

const char*
MN_WinsockNet::DumpNetworkInformation()
{
#if IS_PC_BUILD == 0		// SWFM:AW - To get the xb360 to compile
	return "network information";
#else

	struct GeneratedWarnings
	{
		GeneratedWarnings() { memset(this, 0, sizeof(*this)); }

		bool	addr169dot254Found;
		bool	noGoodUpCardFound;
		bool	noGoodDnsFound;
	}warning;

	warning.addr169dot254Found = false;
	warning.noGoodUpCardFound = true;
	warning.noGoodDnsFound = true;


	PIP_ADAPTER_ADDRESSES AdapterAddresses = NULL;

	static MC_StaticString<64*1024> outStr;
	MC_StaticString<4096> buff;

	outStr = "\n\n--------------------- Network information ----------------------\n"
		"Operating system:    ";

	outStr += MC_GetOperatingSystemDescription();
	outStr += "\n";

	WSANAMESPACE_INFO namespaceInfo[128];
	DWORD namespaceSize=sizeof(namespaceInfo);
	unsigned int numNamespaces = WSAEnumNameSpaceProviders(&namespaceSize, namespaceInfo);
	if (numNamespaces != SOCKET_ERROR)
	{
		for (unsigned int i = 0; i < numNamespaces; i++)
		{
			MC_StaticString<1024> buff;
			if (i == 0)
				buff.Format("Installed providers: %s (%s)\n", namespaceInfo[i].lpszIdentifier, namespaceInfo[i].fActive?"active":"inactive");
			else
				buff.Format("                     %s (%s)\n", namespaceInfo[i].lpszIdentifier, namespaceInfo[i].fActive?"active":"inactive");
			outStr += buff;
		}
	}


	WSAPROTOCOL_INFOW protocolInfo[128];
	DWORD protocolInfoSize = sizeof(protocolInfo);
	int errorCode;
	unsigned int numProtocols = WSCEnumProtocols(NULL, protocolInfo, &protocolInfoSize, &errorCode);
	if (numProtocols != SOCKET_ERROR)
	{
		bool hasShownHeader = false;
		for (unsigned int i = 0; i < numProtocols; i++)
		{
			if (protocolInfo[i].dwProviderFlags)
			{
				MC_StaticString<1024> buff;
				if (!hasShownHeader)
					buff.Format("Installed protocols: %S (%u)\n", protocolInfo[i].szProtocol, protocolInfo[i].dwCatalogEntryId);
				else
					buff.Format("                     %S (%u)\n", protocolInfo[i].szProtocol, protocolInfo[i].dwCatalogEntryId);
				hasShownHeader = true;
				outStr += buff;
			}
		}
	}


	HMODULE hModule = LoadLibrary("iphlpapi.dll");

	if (hModule == NULL)
	{
		outStr += "ERROR: Unable to locate iphlpapi.dll.";
		return outStr;
	}

	/* This function is not available pre-xp but we want to use it if we can!
	ULONG WINAPI GetAdaptersAddresses(
	ULONG Family,
	ULONG Flags,
	PVOID Reserved,
	PIP_ADAPTER_ADDRESSES AdapterAddresses,
	PULONG SizePointer
	);

	With that function, we can get more stats per interface (up, etc) and more details on the type of interface, etc

	*/
	typedef ULONG (WINAPI *GetAdaptersAddressesInLib_t) 
		(ULONG,ULONG,PVOID,PIP_ADAPTER_ADDRESSES,PULONG);

	GetAdaptersAddressesInLib_t GetAdaptersAddressesInLib = (GetAdaptersAddressesInLib_t) GetProcAddress(hModule, "GetAdaptersAddresses");

	if (GetAdaptersAddressesInLib == NULL)
	{
		outStr += " * UNABLE TO LOAD GetAdaptersAddresses(). This is possibly due to Windows pre-XP.\n";
		outStr += "   Some information will be unavailable.\n";
	}
	else
	{
		ULONG OutBufferLength = 0;
		ULONG RetVal = 0, i;    

		// The size of the buffer can be different 
		// between consecutive API calls.
		// In most cases, i < 2 is sufficient;
		// One call to get the size and one call to get the actual parameters.  [[[[THIS COMMENT IS FROM MSDN]]]]
		// But if one more interface is added or addresses are added, 
		// the call again fails with BUFFER_OVERFLOW. 
		// So the number is picked slightly greater than 2. 
		// We use i <5 in the example
		for (i = 0; i < 5; i++) {
			RetVal = 
				(GetAdaptersAddressesInLib)(
				AF_INET, 
				0,
				NULL, 
				AdapterAddresses, 
				&OutBufferLength);

			if (RetVal != ERROR_BUFFER_OVERFLOW) {
				break;
			}

			if (AdapterAddresses != NULL) {
				free(AdapterAddresses);
			}

			AdapterAddresses = (PIP_ADAPTER_ADDRESSES)malloc(OutBufferLength);
			if (AdapterAddresses == NULL) {
				RetVal = GetLastError();
				break;
			}
		}

		if (RetVal != NO_ERROR) 
		{
			AdapterAddresses = NULL;
			outStr += " * Could not call GetAdaptersAddresses\n";
		}
	}


	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;
	unsigned long ulOutBufLen;

	pAdapterInfo = (IP_ADAPTER_INFO *) malloc( sizeof(IP_ADAPTER_INFO) );
	ulOutBufLen = sizeof(IP_ADAPTER_INFO);

	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the ulOutBufLen variable
	if (GetAdaptersInfo( pAdapterInfo, &ulOutBufLen) != ERROR_SUCCESS)
	{
		free (pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *) malloc (ulOutBufLen);
	}

	if ((dwRetVal = GetAdaptersInfo( pAdapterInfo, &ulOutBufLen)) == NO_ERROR)
	{
		pAdapter = pAdapterInfo;
		_set_printf_count_output(1); // enable %n
		while (pAdapter)
		{
			bool currentAdapterIsFirstUp = false;
			char dnsStrings[256] = "";
			char name[512] = "(query not supported)";
			char* type = "(query not supported)";
			MC_StaticString<64> status = "(query not supported)";
			DWORD mtu = -1;
			DWORD flags = -1;

			PIP_ADAPTER_ADDRESSES AdapterList = AdapterAddresses;
			while (AdapterList) {
				if (memcmp(AdapterList->PhysicalAddress, pAdapter->Address, min(AdapterList->PhysicalAddressLength, pAdapter->AddressLength)) == 0)
					break;
				AdapterList = AdapterList->Next;
			}

			winSockNetStats.numInterfaces++;

			if (AdapterList)
			{
				flags = AdapterList->Flags;
				mtu = AdapterList->Mtu;
				sprintf(name, "%S", AdapterList->FriendlyName); // Convert from unicode to ascii
				switch (AdapterList->IfType)
				{
				case IF_TYPE_OTHER                 : type = "IF_TYPE_OTHER";									break;                  
				case IF_TYPE_REGULAR_1822          : type = "IF_TYPE_REGULAR_1822";								break;           
				case IF_TYPE_HDH_1822              : type = "IF_TYPE_HDH_1822";									break;               
				case IF_TYPE_DDN_X25               : type = "IF_TYPE_DDN_X25";									break;                
				case IF_TYPE_RFC877_X25            : type = "IF_TYPE_RFC877_X25";								break;             
				case IF_TYPE_ETHERNET_CSMACD       : type = "IF_TYPE_ETHERNET_CSMACD";							break;        
				case IF_TYPE_IS088023_CSMACD       : type = "IF_TYPE_IS088023_CSMACD";							break;        
				case IF_TYPE_ISO88024_TOKENBUS     : type = "IF_TYPE_ISO88024_TOKENBUS";						break;      
				case IF_TYPE_ISO88025_TOKENRING    : type = "IF_TYPE_ISO88025_TOKENRING";						break;     
				case IF_TYPE_ISO88026_MAN          : type = "IF_TYPE_ISO88026_MAN";								break;           
				case IF_TYPE_STARLAN               : type = "IF_TYPE_STARLAN";									break;                
				case IF_TYPE_PROTEON_10MBIT        : type = "IF_TYPE_PROTEON_10MBIT";							break;        
				case IF_TYPE_PROTEON_80MBIT        : type = "IF_TYPE_PROTEON_80MBIT";							break;         
				case IF_TYPE_HYPERCHANNEL          : type = "IF_TYPE_HYPERCHANNEL";								break;           
				case IF_TYPE_FDDI                  : type = "IF_TYPE_FDDI";										break;                   
				case IF_TYPE_LAP_B                 : type = "IF_TYPE_LAP_B";									break;                  
				case IF_TYPE_SDLC                  : type = "IF_TYPE_SDLC";										break;                   
				case IF_TYPE_DS1                   : type = "IF_TYPE_DS1";										break;                    
				case IF_TYPE_E1                    : type = "IF_TYPE_E1";										break;                     
				case IF_TYPE_BASIC_ISDN            : type = "IF_TYPE_BASIC_ISDN";								break;             
				case IF_TYPE_PRIMARY_ISDN          : type = "IF_TYPE_PRIMARY_ISDN";								break;           
				case IF_TYPE_PROP_POINT2POINT_SERIAL: type = "IF_TYPE_PROP_POINT2POINT_SERIAL";					break;
				case IF_TYPE_PPP                   : type = "IF_TYPE_PPP";										break;                    
				case IF_TYPE_SOFTWARE_LOOPBACK     : type = "IF_TYPE_SOFTWARE_LOOPBACK";						break;      
				case IF_TYPE_EON                   : type = "IF_TYPE_EON";										break;                    
				case IF_TYPE_ETHERNET_3MBIT        : type = "IF_TYPE_ETHERNET_3MBIT";							break;         
				case IF_TYPE_NSIP                  : type = "IF_TYPE_NSIP";										break;                   
				case IF_TYPE_SLIP                  : type = "IF_TYPE_SLIP";										break;                   
				case IF_TYPE_ULTRA                 : type = "IF_TYPE_ULTRA";									break;                  
				case IF_TYPE_DS3                   : type = "IF_TYPE_DS3";										break;                    
				case IF_TYPE_SIP                   : type = "IF_TYPE_SIP";										break;                    
				case IF_TYPE_FRAMERELAY            : type = "IF_TYPE_FRAMERELAY";								break;             
				case IF_TYPE_RS232                 : type = "IF_TYPE_RS232";									break;                  
				case IF_TYPE_PARA                  : type = "IF_TYPE_PARA";										break;                   
				case IF_TYPE_ARCNET                : type = "IF_TYPE_ARCNET";									break;                 
				case IF_TYPE_ARCNET_PLUS           : type = "IF_TYPE_ARCNET_PLUS";								break;            
				case IF_TYPE_ATM                   : type = "IF_TYPE_ATM";										break;                    
				case IF_TYPE_MIO_X25               : type = "IF_TYPE_MIO_X25";									break;                
				case IF_TYPE_SONET                 : type = "IF_TYPE_SONET";									break;                  
				case IF_TYPE_X25_PLE               : type = "IF_TYPE_X25_PLE";									break;                
				case IF_TYPE_ISO88022_LLC          : type = "IF_TYPE_ISO88022_LLC";								break;           
				case IF_TYPE_LOCALTALK             : type = "IF_TYPE_LOCALTALK";								break;              
				case IF_TYPE_SMDS_DXI              : type = "IF_TYPE_SMDS_DXI";									break;               
				case IF_TYPE_FRAMERELAY_SERVICE    : type = "IF_TYPE_FRAMERELAY_SERVICE";						break;     
				case IF_TYPE_V35                   : type = "IF_TYPE_V35";										break;                    
				case IF_TYPE_HSSI                  : type = "IF_TYPE_HSSI";										break;                   
				case IF_TYPE_HIPPI                 : type = "IF_TYPE_HIPPI";									break;                  
				case IF_TYPE_MODEM                 : type = "IF_TYPE_MODEM";									break;                  
				case IF_TYPE_AAL5                  : type = "IF_TYPE_AAL5";										break;                   
				case IF_TYPE_SONET_PATH            : type = "IF_TYPE_SONET_PATH";								break;             
				case IF_TYPE_SONET_VT              : type = "IF_TYPE_SONET_VT";									break;               
				case IF_TYPE_SMDS_ICIP             : type = "IF_TYPE_SMDS_ICIP";								break;              
				case IF_TYPE_PROP_VIRTUAL          : type = "IF_TYPE_PROP_VIRTUAL";								break;           
				case IF_TYPE_PROP_MULTIPLEXOR      : type = "IF_TYPE_PROP_MULTIPLEXOR";							break;       
				case IF_TYPE_IEEE80212             : type = "IF_TYPE_IEEE80212";								break;              
				case IF_TYPE_FIBRECHANNEL          : type = "IF_TYPE_FIBRECHANNEL";								break;           
				case IF_TYPE_HIPPIINTERFACE        : type = "IF_TYPE_HIPPIINTERFACE";							break;         
				case IF_TYPE_FRAMERELAY_INTERCONNECT: type = "IF_TYPE_FRAMERELAY_INTERCONNECT";					break;
				case IF_TYPE_AFLANE_8023           : type = "IF_TYPE_AFLANE_8023";								break;            
				case IF_TYPE_AFLANE_8025           : type = "IF_TYPE_AFLANE_8025";								break;            
				case IF_TYPE_CCTEMUL               : type = "IF_TYPE_CCTEMUL";									break;                
				case IF_TYPE_FASTETHER             : type = "IF_TYPE_FASTETHER";								break;              
				case IF_TYPE_ISDN                  : type = "IF_TYPE_ISDN";										break;                   
				case IF_TYPE_V11                   : type = "IF_TYPE_V11";										break;                    
				case IF_TYPE_V36                   : type = "IF_TYPE_V36";										break;                    
				case IF_TYPE_G703_64K              : type = "IF_TYPE_G703_64K";									break;               
				case IF_TYPE_G703_2MB              : type = "IF_TYPE_G703_2MB";									break;               
				case IF_TYPE_QLLC                  : type = "IF_TYPE_QLLC";										break;                   
				case IF_TYPE_FASTETHER_FX          : type = "IF_TYPE_FASTETHER_FX";								break;           
				case IF_TYPE_CHANNEL               : type = "IF_TYPE_CHANNEL";									break;                
				case IF_TYPE_IEEE80211             : type = "IF_TYPE_IEEE80211";								break;              
				case IF_TYPE_IBM370PARCHAN         : type = "IF_TYPE_IBM370PARCHAN";							break;          
				case IF_TYPE_ESCON                 : type = "IF_TYPE_ESCON";									break;                  
				case IF_TYPE_DLSW                  : type = "IF_TYPE_DLSW";										break;                   
				case IF_TYPE_ISDN_S                : type = "IF_TYPE_ISDN_S";									break;                 
				case IF_TYPE_ISDN_U                : type = "IF_TYPE_ISDN_U";									break;                 
				case IF_TYPE_LAP_D                 : type = "IF_TYPE_LAP_D";									break;                  
				case IF_TYPE_IPSWITCH              : type = "IF_TYPE_IPSWITCH";									break;               
				case IF_TYPE_RSRB                  : type = "IF_TYPE_RSRB";										break;                   
				case IF_TYPE_ATM_LOGICAL           : type = "IF_TYPE_ATM_LOGICAL";								break;            
				case IF_TYPE_DS0                   : type = "IF_TYPE_DS0";										break;                      
				case IF_TYPE_DS0_BUNDLE            : type = "IF_TYPE_DS0_BUNDLE";								break;               
				case IF_TYPE_BSC                   : type = "IF_TYPE_BSC";										break;                      
				case IF_TYPE_ASYNC                 : type = "IF_TYPE_ASYNC";									break;                    
				case IF_TYPE_CNR                   : type = "IF_TYPE_CNR";										break;                      
				case IF_TYPE_ISO88025R_DTR         : type = "IF_TYPE_ISO88025R_DTR";							break;            
				case IF_TYPE_EPLRS                 : type = "IF_TYPE_EPLRS";									break;                    
				case IF_TYPE_ARAP                  : type = "IF_TYPE_ARAP";										break;                     
				case IF_TYPE_PROP_CNLS             : type = "IF_TYPE_PROP_CNLS";								break;                
				case IF_TYPE_HOSTPAD               : type = "IF_TYPE_HOSTPAD";									break;                  
				case IF_TYPE_TERMPAD               : type = "IF_TYPE_TERMPAD";									break;                  
				case IF_TYPE_FRAMERELAY_MPI        : type = "IF_TYPE_FRAMERELAY_MPI";							break;           
				case IF_TYPE_X213                  : type = "IF_TYPE_X213";										break;                     
				case IF_TYPE_ADSL                  : type = "IF_TYPE_ADSL";										break;                     
				case IF_TYPE_RADSL                 : type = "IF_TYPE_RADSL";									break;                    
				case IF_TYPE_SDSL                  : type = "IF_TYPE_SDSL";										break;                     
				case IF_TYPE_VDSL                  : type = "IF_TYPE_VDSL";										break;                     
				case IF_TYPE_ISO88025_CRFPRINT     : type = "IF_TYPE_ISO88025_CRFPRINT";						break;        
				case IF_TYPE_MYRINET               : type = "IF_TYPE_MYRINET";									break;                  
				case IF_TYPE_VOICE_EM              : type = "IF_TYPE_VOICE_EM";									break;                 
				case IF_TYPE_VOICE_FXO             : type = "IF_TYPE_VOICE_FXO";								break;                
				case IF_TYPE_VOICE_FXS             : type = "IF_TYPE_VOICE_FXS";								break;                
				case IF_TYPE_VOICE_ENCAP           : type = "IF_TYPE_VOICE_ENCAP";								break;              
				case IF_TYPE_VOICE_OVERIP          : type = "IF_TYPE_VOICE_OVERIP";								break;             
				case IF_TYPE_ATM_DXI               : type = "IF_TYPE_ATM_DXI";									break;                  
				case IF_TYPE_ATM_FUNI              : type = "IF_TYPE_ATM_FUNI";									break;                 
				case IF_TYPE_ATM_IMA               : type = "IF_TYPE_ATM_IMA";									break;                  
				case IF_TYPE_PPPMULTILINKBUNDLE    : type = "IF_TYPE_PPPMULTILINKBUNDLE";						break;       
				case IF_TYPE_IPOVER_CDLC           : type = "IF_TYPE_IPOVER_CDLC";								break;              
				case IF_TYPE_IPOVER_CLAW           : type = "IF_TYPE_IPOVER_CLAW";								break;              
				case IF_TYPE_STACKTOSTACK          : type = "IF_TYPE_STACKTOSTACK";								break;             
				case IF_TYPE_VIRTUALIPADDRESS      : type = "IF_TYPE_VIRTUALIPADDRESS";							break;         
				case IF_TYPE_MPC                   : type = "IF_TYPE_MPC";										break;                      
				case IF_TYPE_IPOVER_ATM            : type = "IF_TYPE_IPOVER_ATM";								break;               
				case IF_TYPE_ISO88025_FIBER        : type = "IF_TYPE_ISO88025_FIBER";							break;           
				case IF_TYPE_TDLC                  : type = "IF_TYPE_TDLC";										break;                     
				case IF_TYPE_GIGABITETHERNET       : type = "IF_TYPE_GIGABITETHERNET";							break;          
				case IF_TYPE_HDLC                  : type = "IF_TYPE_HDLC";										break;                     
				case IF_TYPE_LAP_F                 : type = "IF_TYPE_LAP_F";									break;                    
				case IF_TYPE_V37                   : type = "IF_TYPE_V37";										break;                      
				case IF_TYPE_X25_MLP               : type = "IF_TYPE_X25_MLP";									break;                  
				case IF_TYPE_X25_HUNTGROUP         : type = "IF_TYPE_X25_HUNTGROUP";							break;            
				case IF_TYPE_TRANSPHDLC            : type = "IF_TYPE_TRANSPHDLC";								break;               
				case IF_TYPE_INTERLEAVE            : type = "IF_TYPE_INTERLEAVE";								break;               
				case IF_TYPE_FAST                  : type = "IF_TYPE_FAST";										break;                     
				case IF_TYPE_IP                    : type = "IF_TYPE_IP";										break;                       
				case IF_TYPE_DOCSCABLE_MACLAYER    : type = "IF_TYPE_DOCSCABLE_MACLAYER";						break;       
				case IF_TYPE_DOCSCABLE_DOWNSTREAM  : type = "IF_TYPE_DOCSCABLE_DOWNSTREAM";						break;     
				case IF_TYPE_DOCSCABLE_UPSTREAM    : type = "IF_TYPE_DOCSCABLE_UPSTREAM";						break;       
				case IF_TYPE_A12MPPSWITCH          : type = "IF_TYPE_A12MPPSWITCH";								break;             
				case IF_TYPE_TUNNEL                : type = "IF_TYPE_TUNNEL";									break;                   
				case IF_TYPE_COFFEE                : type = "IF_TYPE_COFFEE";									break;                   
				case IF_TYPE_CES                   : type = "IF_TYPE_CES";										break;                      
				case IF_TYPE_ATM_SUBINTERFACE      : type = "IF_TYPE_ATM_SUBINTERFACE";							break;         
				case IF_TYPE_L2_VLAN               : type = "IF_TYPE_L2_VLAN";									break;                  
				case IF_TYPE_L3_IPVLAN             : type = "IF_TYPE_L3_IPVLAN";								break;                
				case IF_TYPE_L3_IPXVLAN            : type = "IF_TYPE_L3_IPXVLAN";								break;               
				case IF_TYPE_DIGITALPOWERLINE      : type = "IF_TYPE_DIGITALPOWERLINE";							break;         
				case IF_TYPE_MEDIAMAILOVERIP       : type = "IF_TYPE_MEDIAMAILOVERIP";							break;          
				case IF_TYPE_DTM                   : type = "IF_TYPE_DTM";										break;                      
				case IF_TYPE_DCN                   : type = "IF_TYPE_DCN";										break;                      
				case IF_TYPE_IPFORWARD             : type = "IF_TYPE_IPFORWARD";								break;                
				case IF_TYPE_MSDSL                 : type = "IF_TYPE_MSDSL";									break;                    
				case IF_TYPE_IEEE1394              : type = "IF_TYPE_IEEE1394";									break;                 
//				case IF_TYPE_RECEIVE_ONLY          : type = "IF_TYPE_RECEIVE_ONLY";								break;
				default							   : type = "UNKNOWN";											break;
				};

				switch (AdapterList->OperStatus)
				{
				case IfOperStatusUp: 
					status = "IfOperStatusUp"; 			
					winSockNetStats.numUpInterfaces++;	 
					if (pAdapter->DhcpEnabled) 
						winSockNetStats.dhcpEnabled |= true;
					if (winSockNetStats.firstUpAdapterName[0] == 0)
					{
						strncpy_s(winSockNetStats.firstUpAdapterName, sizeof(winSockNetStats.firstUpAdapterName), pAdapter->Description, _TRUNCATE);
						currentAdapterIsFirstUp = true;
					}
					break;
				case IfOperStatusDown:				status = "IfOperStatusDown";		winSockNetStats.numDownInterfaces++;	break;
				case IfOperStatusTesting:			status = "IfOperStatusTesting"; break;
				case IfOperStatusUnknown:			status = "IfOperStatusUnknown"; break;
				case IfOperStatusDormant:			status = "IfOperStatusDormant"; break;
				case IfOperStatusNotPresent:		status = "IfOperStatusNotPresent"; break;
				case IfOperStatusLowerLayerDown:	status = "IfOperStatusLowerLayerDown"; break;
				default:							status = "UNKNOWN"; break;
				};

				PIP_ADAPTER_DNS_SERVER_ADDRESS dnsaddr = AdapterList->FirstDnsServerAddress;
				unsigned int numDns = 0;
				while (dnsaddr)
				{
					MC_StaticString<32> dnsIpStr = inet_ntoa(((sockaddr_in*)dnsaddr->Address.lpSockaddr)->sin_addr);
					if (warning.noGoodDnsFound)
						if ((!dnsIpStr.BeginsWith("169.254.")) && (!dnsIpStr.BeginsWith("0.0.")))
							warning.noGoodDnsFound = false;

					numDns++;
					strcat(dnsStrings, dnsIpStr.GetBuffer());
					strcat(dnsStrings, " \t");
					dnsaddr = dnsaddr->Next;
					winSockNetStats.numDnsServers = __max(winSockNetStats.numDnsServers, numDns);
				}
			}

			char* generaltype = "Unknown";
			switch(pAdapter->Type)
			{
			case MIB_IF_TYPE_OTHER:			generaltype = "MIB_IF_TYPE_OTHER";			break;
			case MIB_IF_TYPE_ETHERNET:		generaltype = "MIB_IF_TYPE_ETHERNET";		break;
			case MIB_IF_TYPE_TOKENRING:		generaltype = "MIB_IF_TYPE_TOKENRING";		break;
			case MIB_IF_TYPE_FDDI:			generaltype = "MIB_IF_TYPE_FDDI";			break;
			case MIB_IF_TYPE_PPP:			generaltype = "MIB_IF_TYPE_PPP";			break;
			case MIB_IF_TYPE_LOOPBACK:		generaltype = "MIB_IF_TYPE_LOOPBACK";		break;
			case MIB_IF_TYPE_SLIP:			generaltype = "MIB_IF_TYPE_SLIP";			break;
			};

			char charbuff[256] = ""; char* charbuffp=charbuff;
			int len = 0;
			for (UINT i = 0; i < pAdapter->AddressLength; i++)
			{
				sprintf(charbuffp, "%x%c%n", pAdapter->Address[i], 
					i == pAdapter->AddressLength - 1 ? ' ' : '-', &len);
				charbuffp += len;
			}
			*charbuffp = 0;

			ourMacAddress = 0;
			for(UINT i = 0; i < pAdapter->AddressLength && i < sizeof(ourMacAddress); i++)
			{
				ourMacAddress |= pAdapter->Address[i] << (i*8);
			}

			winSockNetStats.maxMtu = __max(winSockNetStats.maxMtu, mtu);


			// Adapted from Intel Laptop Gaming TDK
			const char* physicalMedium = "unknown";
			char nicHandleName[512] = "\\\\.\\";
			strcat_s(nicHandleName, sizeof(nicHandleName), pAdapter->AdapterName);
			HANDLE hNIC = CreateFile(nicHandleName, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if ( hNIC  != INVALID_HANDLE_VALUE )
			{
				unsigned long uOidCode = OID_GEN_PHYSICAL_MEDIUM;
				unsigned long uPhyMedium = 0;
				unsigned long ReqBuffer = 0;
				if ( DeviceIoControl(hNIC, IOCTL_NDIS_QUERY_GLOBAL_STATS, &uOidCode, sizeof(ULONG), &uPhyMedium, sizeof(ULONG), &ReqBuffer, NULL) != 0 )
				{
					switch(uPhyMedium)
					{
					case NdisPhysicalMediumUnspecified: physicalMedium = "Unspecified"; break;
					case NdisPhysicalMediumWirelessLan: physicalMedium = "WirelessLan"; break;
					case NdisPhysicalMediumCableModem: physicalMedium = "CableModem"; break;
					case NdisPhysicalMediumPhoneLine: physicalMedium = "PhoneLine"; break;
					case NdisPhysicalMediumPowerLine: physicalMedium = "PowerLine"; break;
					case NdisPhysicalMediumDSL: physicalMedium = "DSL"; break;
					case NdisPhysicalMediumFibreChannel: physicalMedium = "FibreChannel"; break;
					case NdisPhysicalMedium1394: physicalMedium = "1394"; break;
					case NdisPhysicalMediumWirelessWan: physicalMedium = "WirelessWan"; break;
					case NdisPhysicalMediumNative802_11: physicalMedium = "Native802_11"; break;
					case NdisPhysicalMediumBluetooth: physicalMedium = "Bluetooth"; break;
					};
					if ( uPhyMedium == NdisPhysicalMediumWirelessLan )
					{
						locHandleToWirelessCard = hNIC;
						if (currentAdapterIsFirstUp)
							strcpy_s(winSockNetStats.firstUpMedium, sizeof(winSockNetStats.firstUpMedium), physicalMedium);
					}
				}
			}


			buff.Format(
				"\nAdapter name:   %s\n"
				"\tAdapter index:  %u (0x%x)\n"
				"\tAdapter id:     %s\n"
				"\tAdapter Desc:   %s\n"
				"\tAdapter medium: %s\n"
				"\tAdapter Flags:  %u\n"
				"\tAdapter MTU:    %u\n"
				"\tAdapter type:   %s\n"
				"\tAdapter IfType: %s\n"
				"\tAdapter Status: %s\n"
				"\tAdapter Addr:   %s\n"
				"\tAdapter DNS:    %s\n"
				, name, pAdapter->Index, pAdapter->Index, pAdapter->AdapterName, pAdapter->Description, physicalMedium, flags, mtu, generaltype, type, status.GetBuffer(), charbuff, dnsStrings[0]?dnsStrings:"None");

			outStr += buff;

			PIP_ADDR_STRING addresses = &pAdapter->IpAddressList;
			while (addresses)
			{
				MC_StaticString<16> ip = addresses->IpAddress.String;

				if (warning.noGoodUpCardFound)
				{
					if (status == "IfOperStatusUp")
					{
						if ((!ip.BeginsWith("169.254.")) && (!ip.BeginsWith("0.0.")))
							warning.noGoodUpCardFound = false;
					}
				}
				if (!warning.addr169dot254Found)
				{
					if (ip.BeginsWith("169.254."))
						warning.addr169dot254Found = true;
				}
				buff.Format("\tIP Address:     %s  \tmask: %s\n", ip.GetBuffer(), addresses->IpMask.String);
				outStr += buff;
				addresses = addresses->Next;
			}

			addresses = &pAdapter->GatewayList;
			while (addresses)
			{
				buff.Format("\tGateway:        %s\n", addresses->IpAddress.String);
				outStr += buff;
				addresses = addresses->Next;
			}


			if (pAdapter->DhcpEnabled)
			{
				buff.Format("\tDHCP Enabled:   Yes\n"
					"\tDHCP Server:    %s\n"
					"\tLease Obtained: %ld\n",pAdapter->DhcpServer.IpAddress.String, pAdapter->LeaseObtained);
				outStr += buff;
			}
			else
			{
				outStr += "\tDHCP Enabled:   No";
			}

			if (pAdapter->HaveWins) 
			{
				buff.Format("\tHave Wins:      Yes\n"
					"\t\tPrimary Wins Server: \t%s\n", pAdapter->PrimaryWinsServer.IpAddress.String);
				outStr += buff;

				addresses = &pAdapter->SecondaryWinsServer;
				while (addresses)
				{
					buff.Format("\t\tSecondary Wins Server: \t%s\n", addresses->IpAddress.String);
					outStr += buff;
					addresses = addresses->Next;
				}
			}
			else
			{
				outStr += "\tHave Wins:      No\n";
			}

			pAdapter = pAdapter->Next;
		}
	}
	else
	{
		outStr += " * Call to GetAdaptersInfo failed.\n";
	}
	free (pAdapterInfo);
	free(AdapterAddresses);

	outStr += "\nRouting table:\n";

	PMIB_IPFORWARDTABLE pIpForwardTable;
	DWORD dwSize = sizeof (MIB_IPFORWARDTABLE);
	dwRetVal = 0;

	pIpForwardTable = (MIB_IPFORWARDTABLE*) malloc(sizeof(MIB_IPFORWARDTABLE));
	if (GetIpForwardTable(pIpForwardTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER) 
	{
		free(pIpForwardTable);
		pIpForwardTable = (MIB_IPFORWARDTABLE*) malloc(dwSize);
	}

	if ((dwRetVal = GetIpForwardTable(pIpForwardTable, &dwSize, 1)) == NO_ERROR) 
	{
		outStr += "    Destination           Netmask           Nexthop  Interface    Metric                 Proto          Type        Age\n";
		outStr += "---------------   ---------------   ---------------  ---------   -------   -------------------  ------------    -------\n";


		for (unsigned int i = 0; i < pIpForwardTable->dwNumEntries; i++) 
		{
			char addrbuff[3][16];
			in_addr ia;
			ia.s_addr = pIpForwardTable->table[i].dwForwardDest;
			memcpy(addrbuff[0], inet_ntoa(ia), 16);
			ia.s_addr = pIpForwardTable->table[i].dwForwardMask;
			memcpy(addrbuff[1], inet_ntoa(ia), 16);
			ia.s_addr = pIpForwardTable->table[i].dwForwardNextHop;
			memcpy(addrbuff[2], inet_ntoa(ia), 16);

			char* proto = "UNKNOWN";
			switch (pIpForwardTable->table[i].dwForwardProto)
			{
				/*
				from http://msdn.microsoft.com/library/default.asp?url=/library/en-us/rras/rras/protocol_identifiers.asp
				Protocol 					Description
				--------					-----------
				PROTO_IP_OTHER 				Protocol not listed here.
				PROTO_IP_LOCAL 				Routes generated by the stack.
				PROTO_IP_NETMGMT			Routes added by "route add" or through SNMP.
				PROTO_IP_ICMP 				Routes from ICMP redirects.
				PROTO_IP_EGP 				Exterior Gateway Protocol.
				PROTO_IP_GGP 				To be determined.
				PROTO_IP_HELLO 				HELLO routing protocol.
				PROTO_IP_RIP 				Routing Information Protocol.
				PROTO_IP_IS_IS 				To be determined.
				PROTO_IP_ES_IS 				To be determined.
				PROTO_IP_CISCO 				To be determined.
				PROTO_IP_BBN 				To be determined.
				PROTO_IP_OSPF 				Open Shortest Path First routing protocol.
				PROTO_IP_BGP 				Border Gateway Protocol.
				PROTO_IP_BOOTP 				Bootstrap Protocol.
				PROTO_IPV6_DHCPRELAY 		DHCPv6 Relay protocol.
				PROTO_IP_NT_AUTOSTATIC 		Routes that were originally generated by a routing protocol, but which are now static.
				PROTO_IP_NT_STATIC 			Routes that were added from the routing user interface, or by "routemon ip add".
				PROTO_IP_NT_STATIC_NON_DOD	Identical to PROTO_IP_NET_STATIC, except these routes do not cause Dial On Demand (DOD).
				*/

			case PROTO_IP_OTHER:				proto = "PROTO_IP_OTHER";				break;
			case PROTO_IP_LOCAL:				proto = "PROTO_IP_LOCAL";				break;
			case PROTO_IP_NETMGMT:				proto = "PROTO_IP_NETMGMT";				break;
			case PROTO_IP_ICMP:					proto = "PROTO_IP_ICMP";				break;
			case PROTO_IP_EGP:					proto = "PROTO_IP_EGP";					break;
			case PROTO_IP_GGP:					proto = "PROTO_IP_GGP";					break;
			case PROTO_IP_HELLO:				proto = "PROTO_IP_HELLO";				break;
			case PROTO_IP_RIP:					proto = "PROTO_IP_RIP";					break;
			case PROTO_IP_IS_IS:				proto = "PROTO_IP_IS_IS";				break;
			case PROTO_IP_ES_IS:				proto = "PROTO_IP_ES_IS";				break;
			case PROTO_IP_CISCO:				proto = "PROTO_IP_CISCO";				break;
			case PROTO_IP_BBN:					proto = "PROTO_IP_BBN";					break;
			case PROTO_IP_OSPF:					proto = "PROTO_IP_OSPF";				break;
			case PROTO_IP_BGP:					proto = "PROTO_IP_BGP";					break;
			case PROTO_IP_BOOTP:				proto = "PROTO_IP_BOOTP";				break;
			case PROTO_IP_NT_AUTOSTATIC:		proto = "PROTO_IP_NT_AUTOSTATIC";		break;
			case PROTO_IP_NT_STATIC:			proto = "PROTO_IP_NT_STATIC";			break;
			case PROTO_IP_NT_STATIC_NON_DOD:	proto = "PROTO_IP_NT_STATIC_NON_DOD";	break;
			};

			char* type = "UNKNOWN";
			switch (pIpForwardTable->table[i].dwForwardType)
			{
			case 4: type = "REMOTE_ROUTE";	break;
			case 3: type = "LOCAL_ROUTE";	break;
			case 2: type = "INVALID";		break;
			case 1: type = "OTHER";			break;
			};

			buff.Format("%15s   %15s   %15s   %8u  %8u  %20s  %12s  %8us\n",
				addrbuff[0], addrbuff[1], addrbuff[2], pIpForwardTable->table[i].dwForwardIfIndex,
				pIpForwardTable->table[i].dwForwardMetric1, proto, type,
				pIpForwardTable->table[i].dwForwardAge);
			outStr += buff;
		}
	}
	else 
	{
		outStr += "\tGetIpForwardTable failed.\n";
	}
	free(pIpForwardTable);

	outStr += "-----------------------------------------------------------------\n\n";
	FreeLibrary(hModule);

	outStr +="Network summary:\n";
	bool issuedWarning = false;

	if (warning.addr169dot254Found)
	{
		outStr += " ! Found a LINK-LOCAL network card. You may have connection and/or DHCP errors.\n";
		issuedWarning = true;
	}
	if (warning.noGoodDnsFound)
	{
		outStr += " ! DNS Server may be unavailable or inaccessible.\n";
		issuedWarning = true;
	}
	if (warning.noGoodUpCardFound)
	{
		outStr += " ! No correctly setup network card was found.\n";
		issuedWarning = true;
	}

	if (!issuedWarning)
		outStr += " * Network appears to function ok.\n";
	else
	{
		outStr += " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
		outStr += " ! Network problems where detected - you may experience problems with online and multiplayer games. !\n";
		outStr += " ! There is no general solution to the problem(s) listed above.                                     !\n";
		outStr += " ! Please contact your internet service provider for help.                                          !\n";
		outStr += " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
	}

	return outStr;
#endif
}

const char*
MN_WinsockNet::GetErrorDescription(int errorCode)
{
#if IS_PC_BUILD == 0		// SWFM:AW - To get the xb360 to compile
	return "network error";
#else
	switch(errorCode)
	{
	case WSAEINTR:
		return "WSAEINTR (10004) : A blocking operation was interrupted by a call to WSACancelBlockingCall.";
		break; case WSAEBADF:
		return "WSAEBADF (10009) : The file handle supplied is not valid.";
		break; case WSAEACCES:
		return "WSAEACCES (10013) : An attempt was made to access a socket in a way forbidden by its access permissions.";
		break; case WSAEFAULT:
		return "WSAEFAULT (10014) : The system detected an invalid pointer address in attempting to use a pointer argument in a call.";
		break; case WSAEINVAL:
		return "WSAEINVAL (10022) : An invalid argument was supplied.";
		break; case WSAEMFILE:
		return "WSAEMFILE (10024) : Too many open sockets.";
		break; case WSAEWOULDBLOCK:
		return "WSAEWOULDBLOCK (10035) : A non-blocking socket operation could not be completed immediately.";
		break; case WSAEINPROGRESS:
		return "WSAEINPROGRESS (10036) : A blocking operation is currently executing.";
		break; case WSAEALREADY:
		return "WSAEALREADY (10037) : An operation was attempted on a non-blocking socket that already had an operation in progress.";
		break; case WSAENOTSOCK:
		return "WSAENOTSOCK (10038L) : An operation was attempted on something that is not a socket.";
		break; case WSAEDESTADDRREQ:
		return "WSAEDESTADDRREQ (10039) : A required address was omitted from an operation on a socket.";
		break; case WSAEMSGSIZE:
		return "WSAEMSGSIZE (10040) : A message sent on a datagram socket was larger than the internal message buffer or some other network limit, or the buffer used to receive a datagram into was smaller than the datagram itself.";
		break; case WSAEPROTOTYPE:
		return "WSAEPROTOTYPE (10041) : A protocol was specified in the socket function call that does not support the semantics of the socket type requested.";
		break; case WSAENOPROTOOPT:
		return "WSAENOPROTOOPT (10042) : An unknown, invalid, or unsupported option or level was specified in a getsockopt or setsockopt call.";
		break; case WSAEPROTONOSUPPORT:
		return "WSAEPROTONOSUPPORT (10043) : The requested protocol has not been configured into the system, or no implementation for it exists.";
		break; case WSAESOCKTNOSUPPORT:
		return "WSAESOCKTNOSUPPORT (10044) : The support for the specified socket type does not exist in this address family.";
		break; case WSAEOPNOTSUPP:
		return "WSAEOPNOTSUPP (10045) : The attempted operation is not supported for the type of object referenced.";
		break; case WSAEPFNOSUPPORT:
		return "WSAEPFNOSUPPORT (10046) : The protocol family has not been configured into the system or no implementation for it exists.";
		break; case WSAEAFNOSUPPORT:
		return "WSAEAFNOSUPPORT (10047) : An address incompatible with the requested protocol was used.";
		break; case WSAEADDRINUSE:
		return "WSAEADDRINUSE (10048) : Only one usage of each socket address (protocol/network address/port) is normally permitted.";
		break; case WSAEADDRNOTAVAIL:
		return "WSAEADDRNOTAVAIL (10049) : The requested address is not valid in its context.";
		break; case WSAENETDOWN:
		return "WSAENETDOWN (10050) : A socket operation encountered a dead network.";
		break; case WSAENETUNREACH:
		return "WSAENETUNREACH (10051) : A socket operation was attempted to an unreachable network.";
		break; case WSAENETRESET:
		return "WSAENETRESET (10052) : The connection has been broken due to keep-alive activity detecting a failure while the operation was in progress.";
		break; case WSAECONNABORTED:
		return "WSAECONNABORTED (10053) : An established connection was aborted by the software in your host machine.";
		break; case WSAECONNRESET:
		return "WSAECONNRESET (10054) : An existing connection was forcibly closed by the remote host.";
		break; case WSAENOBUFS:
		return "WSAENOBUFS (10055) : An operation on a socket could not be performed because the system lacked sufficient buffer space or because a queue was full.";
		break; case WSAEISCONN:
		return "WSAEISCONN (10056) : A connect request was made on an already connected socket.";
		break; case WSAENOTCONN:
		return "WSAENOTCONN (10057) : A request to send or receive data was disallowed because the socket is not connected and (when sending on a datagram socket using a sendto call) no address was supplied.";
		break; case WSAESHUTDOWN:
		return "WSAESHUTDOWN (10058) : A request to send or receive data was disallowed because the socket had already been shut down in that direction with a previous shutdown call.";
		break; case WSAETOOMANYREFS:
		return "WSAETOOMANYREFS (10059) : Too many references to some kernel object.";
		break; case WSAETIMEDOUT:
		return "WSAETIMEDOUT (10060) : A connection attempt failed because the connected party did not properly respond after a period of time, or established connection failed because connected host has failed to respond.";
		break; case WSAECONNREFUSED:
		return "WSAECONNREFUSED (10061) : No connection could be made because the target machine actively refused it.";
		break; case WSAELOOP:
		return "WSAELOOP (10062) : Cannot translate name.";
		break; case WSAENAMETOOLONG:
		return "WSAENAMETOOLONG (10063) : Name component or name was too long.";
		break; case WSAEHOSTDOWN:
		return "WSAEHOSTDOWN (10064) : A socket operation failed because the destination host was down.";
		break; case WSAEHOSTUNREACH:
		return "WSAEHOSTUNREACH (10065) : A socket operation was attempted to an unreachable host.";
		break; case WSAENOTEMPTY:
		return "WSAENOTEMPTY (10066) : Cannot remove a directory that is not empty.";
		break; case WSAEPROCLIM:
		return "WSAEPROCLIM (10067) :  A Windows Sockets implementation may have a limit on the number of applications that may use it simultaneously.";
		break; case WSAEUSERS:
		return "WSAEUSERS (10068) : Ran out of quota.";
		break; case WSAEDQUOT:
		return "WSAEDQUOT (10069) : Ran out of disk quota.";
		break; case WSAESTALE:
		return "WSAESTALE (10070) : File handle reference is no longer available.";
		break; case WSAEREMOTE:
		return "WSAEREMOTE (10071) : Item is not available locally.";
		break; case WSASYSNOTREADY:
		return "WSASYSNOTREADY (10091) : WSAStartup cannot function at this time because the underlying system it uses to provide network services is currently unavailable.";
		break; case WSAVERNOTSUPPORTED:
		return "WSAVERNOTSUPPORTED (10092) : The Windows Sockets version requested is not supported.";
		break; case WSANOTINITIALISED:
		return "WSANOTINITIALISED (10093) : Either the application has not called WSAStartup, or WSAStartup failed.";
		break; case WSAEDISCON:
		return "WSAEDISCON (10101) : Returned by WSARecv or WSARecvFrom to indicate the remote party has initiated a graceful shutdown sequence.";
		break; case WSAENOMORE:
		return "WSAENOMORE (10102) : No more results can be returned by WSALookupServiceNext.";
		break; case WSAECANCELLED:
		return "WSAECANCELLED (10103) : A call to WSALookupServiceEnd was made while this call was still processing. The call has been canceled.";
		break; case WSAEINVALIDPROCTABLE:
		return "WSAEINVALIDPROCTABLE (10104) : The procedure call table is invalid.";
		break; case WSAEINVALIDPROVIDER:
		return "WSAEINVALIDPROVIDER (10105) : The requested service provider is invalid.";
		break; case WSAEPROVIDERFAILEDINIT:
		return "WSAEPROVIDERFAILEDINIT (10106) : The requested service provider could not be loaded or initialized.";
		break; case WSASYSCALLFAILURE:
		return "WSASYSCALLFAILURE (10107) : A system call that should never fail has failed.";
		break; case WSASERVICE_NOT_FOUND:
		return "WSASERVICE_NOT_FOUND (10108) : No such service is known. The service cannot be found in the specified name space.";
		break; case WSATYPE_NOT_FOUND:
		return "WSATYPE_NOT_FOUND (10109) : The specified class was not found.";
		break; case WSA_E_NO_MORE:
		return "WSA_E_NO_MORE (10110) : No more results can be returned by WSALookupServiceNext.";
		break; case WSA_E_CANCELLED:
		return "WSA_E_CANCELLED (10111) : A call to WSALookupServiceEnd was made while this call was still processing. The call has been canceled.";
		break; case WSAEREFUSED:
		return "WSAEREFUSED (10112) : A database query failed because it was actively refused.";
		break; case WSAHOST_NOT_FOUND:
		return "WSAHOST_NOT_FOUND (11001) : No such host is known.";
		break; case WSATRY_AGAIN:
		return "WSATRY_AGAIN (11002) : This is usually a temporary error during hostname resolution and means that the local server did not receive a response from an authoritative server.";
		break; case WSANO_RECOVERY:
		return "WSANO_RECOVERY (11003) : A non-recoverable error occurred during a database lookup.";
		break; case WSANO_DATA:
		return "WSANO_DATA (11004) : The requested name is valid, but no data of the requested type was found.";
		break; case WSA_QOS_RECEIVERS:
		return "WSA_QOS_RECEIVERS (11005) : At least one reserve has arrived.";
		break; case WSA_QOS_SENDERS:
		return "WSA_QOS_SENDERS (11006) : At least one path has arrived.";
		break; case WSA_QOS_NO_SENDERS:
		return "WSA_QOS_NO_SENDERS (11007) : There are no senders.";
		break; case WSA_QOS_NO_RECEIVERS:
		return "WSA_QOS_NO_RECEIVERS (11008) : There are no receivers.";
		break; case WSA_QOS_REQUEST_CONFIRMED:
		return "WSA_QOS_REQUEST_CONFIRMED (11009) : Reserve has been confirmed.";
		break; case WSA_QOS_ADMISSION_FAILURE:
		return "WSA_QOS_ADMISSION_FAILURE (11010) : Error due to lack of resources.";
		break; case WSA_QOS_POLICY_FAILURE:
		return "WSA_QOS_POLICY_FAILURE (11011) : Rejected for administrative reasons - bad credentials.";
		break; case WSA_QOS_BAD_STYLE:
		return "WSA_QOS_BAD_STYLE (11012) : Unknown or conflicting style.";
		break; case WSA_QOS_BAD_OBJECT:
		return "WSA_QOS_BAD_OBJECT (11013) : Problem with some part of the filterspec or providerspecific buffer in general.";
		break; case WSA_QOS_TRAFFIC_CTRL_ERROR:
		return "WSA_QOS_TRAFFIC_CTRL_ERROR (11014) : Problem with some part of the flowspec.";
		break; case WSA_QOS_GENERIC_ERROR:
		return "WSA_QOS_GENERIC_ERROR (11015) : General QOS error.";
		break; case WSA_QOS_ESERVICETYPE:
		return "WSA_QOS_ESERVICETYPE (11016) : An invalid or unrecognized service type was found in the flowspec.";
		break; case WSA_QOS_EFLOWSPEC:
		return "WSA_QOS_EFLOWSPEC (11017) : An invalid or inconsistent flowspec was found in the QOS structure.";
		break; case WSA_QOS_EPROVSPECBUF:
		return "WSA_QOS_EPROVSPECBUF (11018) : Invalid QOS provider-specific buffer.";
		break; case WSA_QOS_EFILTERSTYLE:
		return "WSA_QOS_EFILTERSTYLE (11019) : An invalid QOS filter style was used.";
		break; case WSA_QOS_EFILTERTYPE:
		return "WSA_QOS_EFILTERTYPE (11020) : An invalid QOS filter type was used.";
		break; case WSA_QOS_EFILTERCOUNT:
		return "WSA_QOS_EFILTERCOUNT (11021) : An incorrect number of QOS FILTERSPECs were specified in the FLOWDESCRIPTOR.";
		break; case WSA_QOS_EOBJLENGTH:
		return "WSA_QOS_EOBJLENGTH (11022) : An object with an invalid ObjectLength field was specified in the QOS provider-specific buffer.";
		break; case WSA_QOS_EFLOWCOUNT:
		return "WSA_QOS_EFLOWCOUNT (11023) : An incorrect number of flow descriptors was specified in the QOS structure.";
		break; case WSA_QOS_EUNKOWNPSOBJ:
		return "WSA_QOS_EUNKOWNPSOBJ (11024) : An unrecognized object was found in the QOS provider-specific buffer.";
		break; 
	default:
		{
			static __declspec(thread) char ret[128];
			sprintf(ret, "Unknown code: %d", errorCode);
			return ret;
		}
	};
	return "error";
#endif		//SWFM:AW
}

bool 
MN_WinsockNet::IsConnectionWireless()
{
	return locHandleToWirelessCard != NULL;
}

float
MN_WinsockNet::GetWirelessSignalStrength()
{
	MC_PROFILER_BEGIN(a, "Getting wireless signal strength"); // Too slow in real life? Need separate thread?
	static float lastSigStrength = 0.0f;
	static unsigned int nextUpdateTime = 0;

	if (MI_Time::ourCurrentSystemTime < nextUpdateTime)
		return lastSigStrength;
	nextUpdateTime = MI_Time::ourCurrentSystemTime + 1000; // Update max once per second

	float sigStrength = 0.0f;
	if ( locHandleToWirelessCard != INVALID_HANDLE_VALUE )
	{
		unsigned long uOidCode = OID_802_11_RSSI;
		unsigned long uPhyMedium = 0;
		unsigned long ReqBuffer = 0;
		long rssi = 0;
		if ( DeviceIoControl(locHandleToWirelessCard, IOCTL_NDIS_QUERY_GLOBAL_STATS, &uOidCode, sizeof(ULONG), &rssi, sizeof(ULONG), &ReqBuffer, NULL) != 0 )
		{
			sigStrength = __min(100.0f, float(rssi + 100) * 2.0f);
			if (sigStrength <= 4.0f)
			{
				sigStrength = 0.0f;
			}
		}
		else
		{
			MC_DEBUG("Failed to query wireless strength.");
			locHandleToWirelessCard = NULL; // Intel laptop tdk doesn't close handle either (handle close == driver signal?)
		}
	}
	return lastSigStrength = sigStrength / 100.0f;
}


bool 
MN_WinsockNet::GetIntelLaptopInfo(float& batteryPercent, unsigned int& batteryTimeLeft, bool &isUsingWireless, float& wirelessStrength)
{
	SYSTEM_POWER_STATUS pwrs;
	if (!GetSystemPowerStatus(&pwrs))
		return false; // os error
	if (pwrs.BatteryFlag == 128)
		return false; // no system battery, i.e. no laptop

	if (pwrs.ACLineStatus == 1) // plugged into power outlet
		batteryTimeLeft = -1;
	else
		batteryTimeLeft = pwrs.BatteryLifeTime == -1 ? 60 : pwrs.BatteryLifeTime; // If unknown, return 1h
	batteryPercent = min(100,pwrs.BatteryLifePercent)/100.0f;

	isUsingWireless = IsConnectionWireless();
	if (isUsingWireless)
		wirelessStrength = GetWirelessSignalStrength();
	return true;
}

bool 
MN_WinsockNet::IsLaptop()
{
	SYSTEM_POWER_STATUS pwrs;
	if (!GetSystemPowerStatus(&pwrs))
		return false; // os error
	if (pwrs.BatteryFlag == 128)
		return false; // no system battery, i.e. no laptop

	return true;
}

void
LogOutgoingBandwidth(int len)
{
	if (!MN_WinsockNet::ourDoLogBandwidthFlag)
		return;
	if (len <= 0)
		return;
	static int lastCurrMinute = unsigned int(time(NULL)) / 60;
	static int bytesInMinute = 0;
	const int currMinute = unsigned int(time(NULL)) / 60;
	if (currMinute != lastCurrMinute)
	{
		MC_DEBUG("SENT %uKb (%.2f Kbit/s) LAST MINUTE", bytesInMinute/1024, (bytesInMinute*8)/1024.f/60.f);
		bytesInMinute = 0;
		lastCurrMinute = currMinute;
	}
	bytesInMinute += len;
}
void
LogIncomingBandwidth(int len)
{
	if (!MN_WinsockNet::ourDoLogBandwidthFlag)
		return;
	if (len <= 0)
		return; // broken udp connection, or no data (tcp)
	static int lastCurrMinute = unsigned int(time(NULL)) / 60;
	static int bytesInMinute = 0;
	const int currMinute = unsigned int(time(NULL)) / 60;
	if (currMinute != lastCurrMinute)
	{
		MC_DEBUG("RECEIVED %uKb (%.2f Kbit/s) LAST MINUTE", bytesInMinute/1024, (bytesInMinute*8)/1024.f/60.f);
		bytesInMinute = 0;
		lastCurrMinute = currMinute;
	}
	bytesInMinute += len;
}


