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
#include "MMS_FriendsLadder.h"

MT_Mutex MMS_FriendsLadder::myMutex; 

MMS_FriendsLadder::MMS_FriendsLadder()
: myItemsAreDirty(true)
{
	myItems.Init(128000, 128000, false); 
}

MMS_FriendsLadder::~MMS_FriendsLadder()
{
}

void 
MMS_FriendsLadder::Add(const unsigned int aProfileId, 
					   const unsigned int aScore)
{
	MT_MutexLock lock(myMutex); 

	// we simply assume that the we don't have the entry before, 
	// if you can't do that use UpdateItem instead
	myItems.Add(LadderItem(aProfileId, aScore)); 
	myItemsAreDirty = true; 	
}

void 
MMS_FriendsLadder::Remove(const unsigned int aProfileId)
{
	MT_MutexLock lock(myMutex); 
	PrivRemove(aProfileId); 
}

void 
MMS_FriendsLadder::PrivSort()
{
	if(myItemsAreDirty)
	{
		myItems.Sort(-1, -1, false); 
		myItemsAreDirty = false; 
	}
}

MMS_FriendsLadder::LadderItem* 
MMS_FriendsLadder::PrivFind(unsigned int aProfileId)
{
	PrivSort();
	unsigned int index = myItems.FindInSortedArray2<LadderItemComparer>(aProfileId);  
	if(index != -1)
		return &(myItems[index]);

	return NULL; 
}

void 
MMS_FriendsLadder::PrivRemove(unsigned int aProfileId)
{
	PrivSort();
	unsigned int index = myItems.FindInSortedArray2<LadderItemComparer>(aProfileId);  
	if(index != -1)
	{
		myItems.RemoveCyclicAtIndex(index); 
		myItemsAreDirty = true;
	}
}

void 
MMS_FriendsLadder::AddOrUpdate(const unsigned int aProfileId, 
							  const unsigned int aScore)
{
	MT_MutexLock lock(myMutex); 

	if(aScore == 0)
	{
		PrivRemove(aProfileId); 
	}
	else 
	{
		LadderItem* item = PrivFind(aProfileId); 
		if(item)
			item->myScore = aScore; 
		else 
			myItems.Add(LadderItem(aProfileId, aScore)); 
		myItemsAreDirty = true; 
	}
}

void 
MMS_FriendsLadder::GetFriendsLadder(MMG_FriendsLadderProtocol::FriendsLadderReq& aLadderReq, 
									MMG_FriendsLadderProtocol::FriendsLadderRsp& aLadderRsp)
{
	MT_MutexLock lock(myMutex); 
	PrivSort();
	for(int i = 0; i < aLadderReq.myFriendsProfileIds.Count(); i++)
	{
		unsigned int index = myItems.FindInSortedArray2<LadderItemComparer>(aLadderReq.myFriendsProfileIds[i]);  
		if(index != -1)
			aLadderRsp.Add(myItems[index].myProfileId, myItems[index].myScore); 
	}
}

void
MMS_FriendsLadder::Purge()
{
	myItems.RemoveAll();
}


MMS_FriendsLadder::LadderItem::LadderItem()
: myProfileId(0)
, myScore(0)
{
}

MMS_FriendsLadder::LadderItem::LadderItem(const unsigned int aProfileId, 
										  const unsigned int aScore)
: myProfileId(aProfileId)
, myScore(aScore)
{
}

bool 
MMS_FriendsLadder::LadderItem::operator>(const LadderItem& aRhs) const
{
	return myProfileId > aRhs.myProfileId; 
}

bool 
MMS_FriendsLadder::LadderItem::operator<(const LadderItem& aRhs) const
{
	return myProfileId < aRhs.myProfileId; 
}

bool 
MMS_FriendsLadder::LadderItem::operator==(const LadderItem& aRhs) const
{
	return myProfileId == aRhs.myProfileId; 
}

