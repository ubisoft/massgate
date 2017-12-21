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
#ifndef MMS_NATNEGOTIATIONLUT___H__
#define MMS_NATNEGOTIATIONLUT___H__

#include "MMS_IocpServer.h"
#include "MC_StaticArray.h"
#include "MC_GrowingArray.h"
#include "MC_HashMap.h"
#include "MT_Mutex.h"

class MMS_NATNegotiationLutContainer 
{
public: 
	class Lut
	{
	public:
		Lut(MT_Mutex* aMutex, 
			MMS_IocpServer::SocketContext* mySocketContext, 
			unsigned int aRequestId, 
			unsigned int aReservedDummy); 

		~Lut(); 

		MMS_IocpServer::SocketContext* mySocketContext; 

		MT_Mutex* myMutex; 
		unsigned int myRequestId; 
		unsigned int myReservedDummy; 
		static const unsigned int MY_MAX_IDLE_TIME = 60000; // we keep luts for 60 seconds, then purge 
		unsigned int myLastUse; 

	private: 
		Lut(const Lut& aRhs) { assert(false && "no copyctor for MMS_NATNegotiationLut!"); }
		Lut& operator=(const Lut& aRhs) { assert(false && "no operator= for MMS_NATNegotiationLut!"); }
	};

	class LutRef
	{
	public: 
		LutRef();
		LutRef(Lut* aLut);
		~LutRef();
		void operator=(Lut* aNATNegLut) { assert(myNATNegLut == NULL); myNATNegLut = aNATNegLut; }
		__forceinline Lut* operator->() { assert(myNATNegLut); return myNATNegLut; }
		__forceinline Lut* GetItem()  { assert(myNATNegLut); return myNATNegLut; }
		bool IsGood();
		void Release();

		friend class MMS_NATNegotiationLutContainer; 

	private:
		Lut* myNATNegLut;
	};

	MMS_NATNegotiationLutContainer(); 
	
	~MMS_NATNegotiationLutContainer(); 

	// aSessionCookie and aPeerProfileId makes the unique key
	// aRequestId and aReservedDummy are simply stored for the bounce back message 

	bool AddPeer(unsigned int aSessionCookie, 
				 unsigned int aProfileId, 
				 MMS_IocpServer::SocketContext* aSocketContext, 
				 unsigned int aRequestId, 
				 unsigned int aReservedDummy); 

	void RemovePeer(unsigned int aSessionCookie, 
					unsigned int aProfileId);

	void RemovePeer(LutRef& aLutRef); 

	Lut* GetPeerLut(unsigned int aSessionCookie, 
					unsigned int aProfileId); 

	void PurgeOldLuts(); 

private: 

	struct Key
	{
		unsigned int mySessionCookie; 
		unsigned int myPeerProfileId; 

		bool operator==(const Key& rhs) const
		{
			return (mySessionCookie == rhs.mySessionCookie) && (myPeerProfileId == rhs.myPeerProfileId); 
		}
	};

	MC_HashMap<Key, Lut*> myPeers; 
	static const unsigned int MAX_NUM_MUTEXES = 8; 
	MC_StaticArray<MT_Mutex, MAX_NUM_MUTEXES> myMutexes; 
	volatile unsigned int myNextMutexIndex; 
	MT_Mutex myTableLock; 
};

#endif