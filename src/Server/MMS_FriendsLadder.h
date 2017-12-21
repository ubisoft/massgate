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
#ifndef MMS_FRIENDSLADDER_H
#define MMS_FRIENDSLADDER_H

#include "MMG_FriendsLadderProtocol.h"
#include "MC_GrowingArray.h"
#include "MT_Mutex.h"

class MMS_FriendsLadder
{
public:
	MMS_FriendsLadder();
	~MMS_FriendsLadder();

	void Add(const unsigned int aProfileId, 
			 const unsigned int aScore);

	void Remove(const unsigned int aProfileId); 

	void AddOrUpdate(const unsigned int aProfileId, 
					const unsigned int aScore);

	void GetFriendsLadder(MMG_FriendsLadderProtocol::FriendsLadderReq& aLadderReq, 
						  MMG_FriendsLadderProtocol::FriendsLadderRsp& aLadderRsp); 

	void Purge();


	static MT_Mutex myMutex;

private: 

	class LadderItem 
	{
	public: 
		LadderItem(); 
		LadderItem(const unsigned int aProfileId, 
				   const unsigned int aScore); 

		bool operator>(const LadderItem& aRhs) const; 
		bool operator<(const LadderItem& aRhs) const; 
		bool operator==(const LadderItem& aRhs) const; 

		unsigned int myProfileId; 
		unsigned int myScore;
	};

	struct LadderItemComparer
	{
		static bool Equals(const LadderItem& aLadderItem, const unsigned int aProfileId)
		{ 
			return aLadderItem.myProfileId == aProfileId; 
		} 
		static bool LessThan(const LadderItem& aLadderItem, const unsigned int aProfileId)
		{ 
			return aLadderItem.myProfileId < aProfileId; 
		} 
	};

	MC_GrowingArray<LadderItem> myItems; 
	bool myItemsAreDirty; 

	void PrivSort(); 
	LadderItem* PrivFind(unsigned int aProfileId); 
	void PrivRemove(unsigned int aProfileId); 

};

#endif
