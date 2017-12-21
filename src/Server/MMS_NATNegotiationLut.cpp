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
#include "StdAfx.h"
#include "MMS_NATNegotiationLut.h"
#include "MC_Debug.h"
#include "ML_Logger.h"

MMS_NATNegotiationLutContainer::Lut::Lut(MT_Mutex* aMutex, 
										 MMS_IocpServer::SocketContext* aSocketContext, 
										 unsigned int aRequestId, 
										 unsigned int aReservedDummy)
: myMutex(aMutex)
, mySocketContext(aSocketContext)
, myRequestId(aRequestId)
, myReservedDummy(aReservedDummy)
{
	LOG_DEBUGPOS(); 
	myLastUse = GetTickCount(); 
}

MMS_NATNegotiationLutContainer::Lut::~Lut()
{
	LOG_DEBUGPOS(); 
}

MMS_NATNegotiationLutContainer::LutRef::LutRef()
: myNATNegLut(NULL)
{
	assert(false && "Don't call this code"); 
}
	
MMS_NATNegotiationLutContainer::LutRef::LutRef(Lut* aLut)
: myNATNegLut(aLut)
{
}

MMS_NATNegotiationLutContainer::LutRef::~LutRef()
{
	LOG_DEBUGPOS(); 
	Release(); 
}

bool
MMS_NATNegotiationLutContainer::LutRef::IsGood()
{
	return (myNATNegLut != NULL); 
}

void 
MMS_NATNegotiationLutContainer::LutRef::Release()
{
	if(myNATNegLut != NULL)
	{
		myNATNegLut->myMutex->Unlock(); 	
	}
}

MMS_NATNegotiationLutContainer::MMS_NATNegotiationLutContainer()
: myNextMutexIndex(0)
{	
	LOG_DEBUGPOS(); 
}

MMS_NATNegotiationLutContainer::~MMS_NATNegotiationLutContainer()
{	
	LOG_DEBUGPOS(); 
}

bool MMS_NATNegotiationLutContainer::AddPeer(unsigned int aSessionCookie, 
											 unsigned int aPeerProfileId, 
											 MMS_IocpServer::SocketContext* aSocketContext, 
											 unsigned int aRequestId, 
											 unsigned int aReservedDummy)
{
	Key key; 
	
	key.mySessionCookie = aSessionCookie; 
	key.myPeerProfileId = aPeerProfileId; 

	MMS_NATNegotiationLutContainer::Lut* lut = new MMS_NATNegotiationLutContainer::Lut(&myMutexes[myNextMutexIndex], aSocketContext, aRequestId, aReservedDummy); 

	myNextMutexIndex++; 
	if(myNextMutexIndex >= MAX_NUM_MUTEXES)
	{
		myNextMutexIndex = 0; 
	}

	MT_MutexLock lock(myTableLock); 
	if(myPeers.Exists(key))
	{
		lock.Unlock(); 
		LOG_ERROR("this session is already added, %d, %d", aSessionCookie, aPeerProfileId); 
		delete lut; 
		return false; 
	}

	myPeers[key] = lut;  

	return true; 
}

void 
MMS_NATNegotiationLutContainer::RemovePeer(unsigned int aSessionCookie, 
										   unsigned int aPeerProfileId)
{
	Key key; 
	
	key.mySessionCookie = aSessionCookie; 
	key.myPeerProfileId = aPeerProfileId; 

	MMS_NATNegotiationLutContainer::Lut* lut; 
	myTableLock.Lock(); 
	if(myPeers.GetIfExists(key, lut))
	{
		myPeers.RemoveByKey(key);
		myTableLock.Unlock(); 

		MT_Mutex* lutLock = lut->myMutex; 
		lutLock->Lock(); 
		delete lut; 
		lutLock->Unlock(); 
	}
	else
	{
		myTableLock.Unlock(); 
	}
}

void 
MMS_NATNegotiationLutContainer::RemovePeer(MMS_NATNegotiationLutContainer::LutRef& aLutRef)
{
	myTableLock.Lock(); 
	myPeers.RemoveByItem(aLutRef.myNATNegLut);
	myTableLock.Unlock(); 

	MT_Mutex* lutLock = aLutRef.myNATNegLut->myMutex; 
	lutLock->Lock(); 

	delete aLutRef.myNATNegLut; 
	aLutRef.myNATNegLut = NULL; 
	lutLock->Unlock(); 
}

MMS_NATNegotiationLutContainer::Lut*
MMS_NATNegotiationLutContainer::GetPeerLut(unsigned int aSessionCookie, 
										   unsigned int aPeerProfileId)
{
	Key key; 

	key.mySessionCookie = aSessionCookie; 
	key.myPeerProfileId = aPeerProfileId; 

	MT_MutexLock lock(myTableLock);
	MMS_NATNegotiationLutContainer::Lut* lut; 
	if(myPeers.GetIfExists(key, lut))
	{
		MT_Mutex* lutLock = lut->myMutex; 
		lutLock->Lock(); 
		lut->myLastUse = GetTickCount(); 
		return lut; 
	}

	return NULL; 
}

void 
MMS_NATNegotiationLutContainer::PurgeOldLuts()
{
	unsigned int currentTime = GetTickCount(); 
	
	return;

	MT_MutexLock lock(myTableLock); 
	MC_HashMap<Key, Lut*>::Iterator iter(myPeers); 
	for(; iter; iter++)
	{
		MMS_NATNegotiationLutContainer::Lut* lut = iter.Item(); 
		MT_Mutex* lutLock = lut->myMutex; 

		if(lutLock->TryLock())
		{
			if((currentTime - lut->myLastUse) > MMS_NATNegotiationLutContainer::Lut::MY_MAX_IDLE_TIME)
			{
				myPeers.RemoveByItem(lut);
				delete lut; 
			}
			lutLock->Unlock(); 				
		}
	}
}
