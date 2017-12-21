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
#ifndef MMG_ReliablePacketHandler___H_
#define MMG_ReliablePacketHandler___H_

/*
#include "MC_GrowingArray.h"
#include "MC_KeyTree.h"
#include <winsock2.h>
#include "MMG_Average.h"

template<class UDPSOCKETTYPE>
class MMG_ReliablePacketHandler
{
public:
	MMG_ReliablePacketHandler();
	virtual ~MMG_ReliablePacketHandler();
	
	virtual void Update();

	virtual bool ReliableSend(const UDPSOCKETTYPE& aDestination, const void* theData, unsigned int theDatalen);
	virtual bool ReliableReceive(const UDPSOCKETTYPE& theServerSocket, const void* dataDest, unsigned int maxDatalen, unsigned int& numReceivedBytes);

protected:
	typedef SOCKADDR_IN IP;
	typedef unsigned short AckIdentifier;

	class ReliableEntity
	{
		UDPSOCKETTYPE* myDestination;
		const void* myData;
		unsigned int myDatalen;
		AckIdentifier myAckId;
		float lastSendTime;
		MC_GrowingArray<AckIdentifier> myContainingAcks;
	};

	MC_GrowingArray<ReliableEntity*> mySentButNotAckedReliableSends; // store in binary tree, on myAckId
	MC_KeyTree<IP, MMG_Average<float> > myLatencyMeasurements;
	MC_KeyTree<MC_GrowingArray<AckIdentifier>*, UDPSOCKETTYPE*> myPendingAcks;

private:
	MC_GrowingArray<AckIdentifier>* myPendingAcks(const UDPSOCKETTYPE& aDestination);
	void addPendingAck(const UDPSOCKETTYPE* aSocket, AckIdentifier ackNum);

	short mySequenceCounter;
	char* myOutgoingPacketBuffer;
	char* myIncomingPacketBuffer;
};

#include "MMG_ReliablePacketHandler.cpp"
*/
#endif
