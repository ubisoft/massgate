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
#ifndef MMG_RELIBABLEUDPSOCKETSERVERFACTORY___H_
#define MMG_RELIBABLEUDPSOCKETSERVERFACTORY___H_

#include "MMG_ReliableUdpSocket.h"
#include "MC_SortedGrowingArray.h"
#include "mc_platform.h"

class MMG_ReliableUdpSocketServerFactory
{
public:
	MMG_ReliableUdpSocketServerFactory();
	~MMG_ReliableUdpSocketServerFactory();


	bool Init(unsigned short theListenPort);

	void Abort();

	// Call this once every frame. Returns false if the server is shutdown due to network errors (or not initialized correctly)
	// Delayed update -- after update() you have one frame on you to remove your own references to a closed socket as
	// it will be deleted on the next call to update
	bool Update(bool updateAllSockets=true);

	unsigned short GetListeningPort();

	// Accept a connection. Pass true if you want your operations on the returned socket to be blocking.
	// you must call Dispose() with the socket after you are finished with it
	void Dispose(MMG_ReliableUdpSocket* theSocket);

	bool GetReadableSockets(MC_GrowingArray<MMG_ReliableUdpSocket*>& anArray) const;
	unsigned long GetNumberOfActiveConnections() const;
	MMG_ReliableUdpSocket* GetConnection(const unsigned long theIndex);

protected:
	MMG_ReliableUdpSocket* AcceptConnection(bool aShouldBlock=false); 
	MMG_ReliableUdpSocket::Socket myServerSocket;
	CRITICAL_SECTION myThreadingLock;
	MC_SortedGrowingArray<MMG_ReliableUdpSocket*> myActiveConnections;

private:
	unsigned long myConnectionOffset;
};

#endif
