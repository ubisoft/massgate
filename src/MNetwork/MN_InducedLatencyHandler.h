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
#ifndef MN_INDUCED_LATENCY_HANDLER___H___
#define MN_INDUCED_LATENCY_HANDLER___H___


#include "MT_Thread.h"
#include "MT_Mutex.h"

#include "MC_SortedGrowingArray.h"


class MN_InducedLatencyHandler : public MT_Thread
{
public:
	MN_InducedLatencyHandler(int theInducedLatency);
	virtual ~MN_InducedLatencyHandler();

	virtual void Run();

	// Non-delayed calls
	SOCKET socket(int af, int type, int protocol);
	int bind(SOCKET s, const struct sockaddr* name, int namelen);
	int ioctlsocket(SOCKET s, long cmd, u_long* argp);
	int closesocket(SOCKET s);
	int shutdown(SOCKET s, int how);

	// Latency-inducing calls
	int send(SOCKET s, const char* buf, int len, int flags);
	int sendto(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen);
	int recv(SOCKET s, char* buf, int len, int flags);
	int recvfrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen);

protected:

	struct IO_item {

		unsigned long issueTime;

		SOCKET s;
		char buffer[65536];
		int len;
		int flags;
		char data[32];
		int datalen;
		int opResult;
		int opErrorCode;
	};

	struct SocketContext {
		SocketContext() : mySocket(INVALID_SOCKET), myLastError(0), myIsBlocking(true), myType(-1), myHasEncounteredError(false) { }
		SocketContext(const SocketContext& aRhs) : mySocket(aRhs.mySocket), myLastError(aRhs.myLastError), myIsBlocking(aRhs.myIsBlocking), myType(aRhs.myType), myHasEncounteredError(aRhs.myHasEncounteredError) { }
		SocketContext& operator=(const SocketContext& aRhs) { if (this != &aRhs) { mySocket = aRhs.mySocket, myLastError = aRhs.myLastError; myIsBlocking = aRhs.myIsBlocking; myType = aRhs.myType; myHasEncounteredError = aRhs.myHasEncounteredError; } return *this; }
		bool operator==(const SocketContext& aRhs) const { return mySocket == aRhs.mySocket; }
		bool operator<(const SocketContext& aRhs) const { return mySocket < aRhs.mySocket; }
		bool operator>(const SocketContext& aRhs) const { return mySocket > aRhs.mySocket; }

		SOCKET	mySocket;
		int		myLastError;
		int		myIsBlocking;
		int		myType;
		bool	myHasEncounteredError;
		bool	myIsBound;
	};

	

private:
	SocketContext* GetSocketContext(SOCKET s);

	MC_SortedGrowingArray<SocketContext> mySockets;
	MC_GrowingArray<IO_item*> myOutputBuffer;
	MC_GrowingArray<IO_item*> myInputBuffer;

	MT_Mutex myMutex;
	int myInputLatency;
	int myOutputLatency;
};


#endif
