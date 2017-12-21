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
#ifndef MSB_TCPWRITELIST_H
#define MSB_TCPWRITELIST_H

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MT_Mutex.h"

#include "MSB_IoCore.h"
#include "MSB_LinkedList.h"

class MSB_TCPSocket;
class MSB_WriteBuffer;

class MSB_TCPWriteList
{
public:
							MSB_TCPWriteList(
								uint32					aMaxOutgoingLength = -1);
							~MSB_TCPWriteList();

	void					SetMaxOutgoingLength(
								uint32					aMaxOutgoingLength);

	void					SetSocket(
								MSB_TCPSocket*			aSocket);

	int32					SendComplete();
	bool					IsEmpty() const { return myWriteList.IsEmpty(); }
	bool					IsIdle() const { return myWriteList.IsEmpty() && myTransitBuffers[0].myEntry == NULL; }
	
	int32					Add(
								MSB_WriteBuffer*		aBufferList);
	
private:
	class Entry;

	typedef struct {
		Entry*				myEntry;
		MSB_WriteBuffer*	myBuffer;
	}TransitBuffer;

	class Entry 
	{
	public:
		Entry*				myNext;
		Entry*				myPrev;

							Entry(
								MSB_WriteBuffer*		aBufferList);
		void				Release();

		bool				IsEmpty();
		void				BufferSent(
								MSB_WriteBuffer*		aBuffer);
		uint32				StartNextSend(
								TransitBuffer*			aTransitList,
								WSABUF*					aWsaList,
								uint32					aMaxCount);

		void				SetBufferList(
								MSB_WriteBuffer*		aBufferList);

	private:
		MSB_WriteBuffer*	myBufferList;
		MSB_WriteBuffer*	myTransitHead;
		MSB_WriteBuffer*	myTransitTail;
	};
	
	class EntryPool
	{
	public:
		static EntryPool&	GetInstance() { return ourInstance; }	
		Entry*				GetEntry(
								MSB_WriteBuffer*		aBufferList);
		void				PutEntry(
								Entry*		anEntry);
	private:
		static EntryPool	ourInstance;
		MT_Mutex			myGlobalLock;
		Entry*				myStack;
		EntryPool() 
			: myStack(NULL) {};
		~EntryPool() {};
	};

	static const uint32		NUM_TRANSIT_BUFFERS = 10;

	MT_Mutex				myLock;
	MSB_IoCore::OverlappedHeader	myOverlapped;
	TransitBuffer			myTransitBuffers[NUM_TRANSIT_BUFFERS];
	WSABUF					myWsaBuffs[NUM_TRANSIT_BUFFERS];
	MSB_LinkedList<Entry>	myWriteList;
	MSB_TCPSocket*			myTcpSocket;
	uint32					myMaxOutgoingLength;
	uint32					myCurrentOutgoingLength;

	int32					PrivStartNextSend();
};

#endif // MSB_TCPWRITELIST_H

#endif // IS_PC_BUILD
