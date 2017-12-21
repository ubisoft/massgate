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
#ifndef MSB_UDPWRITELIST_H_
#define MSB_UDPWRITELIST_H_

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MSB_Types.h"
#include "MSB_LinkedList.h"
#include "MSB_WriteBuffer.h"

#include "MT_Mutex.h"

class MSB_UDPSocket;

class MSB_UDPWriteList
{
public:
				MSB_UDPWriteList();
				~MSB_UDPWriteList();	

	void		Add(
					const sockaddr_in&		anAddr, 
					MSB_WriteBuffer*		aHeadBuffer);
	void		Add(
					const MSB_UDPClientID	aClientID,
					const sockaddr_in&		anAddr,
					MSB_WriteBuffer*		aHeadBuffer);
	void		RemoveFromInTransit(
					OVERLAPPED*				anOverlapped,
					MSB_Socket_Win*				anUDPSocket);

	bool		IsEmpty() { return myWriteList.IsEmpty(); }
	bool		IsIdle() { return myWriteList.IsEmpty() && myInTransit.IsEmpty(); }

	int			Send(
					MSB_Socket_Win*				anUDPSocket);

	bool		myWritePending;

private:
	class Entry 
	{
		typedef struct {
			MSB_IoCore::OverlappedHeader	myOverlapped;	
			Entry*						mySelf;
		} OverlappedEntry; 

		/* FIXME: Make a pool of Entries to speed up allocate/deallocate of them */
	public:
		Entry();
		~Entry();

		static Entry*		Allocate();
		void				Deallocate();

		static Entry*		GetFromOverlapped(
								OVERLAPPED*	anOverlapped);

		MSB_UDPClientID		myClientID;  //In network byte order!!!
		struct sockaddr_in	myDest;
		MSB_WriteBuffer*	myWriteBuffer;
		WSABUF				myWsaBuf[3];
		OverlappedEntry		myOverlappedEntry;
		bool				myIncludeHeaderFlag;

		Entry*				myNext;
		Entry*				myPrev;
	};

	class EntryPool
	{
	public:
		static EntryPool&	GetInstance() { return ourInstance; }	
		Entry*				GetEntry();
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


	MT_Mutex				myLock;

	MSB_LinkedList<Entry>	myWriteList;
	MSB_LinkedList<Entry>	myInTransit;
};

#endif // IS_PC_BUILD

#endif // MSB_UDPWRITELIST_H_
