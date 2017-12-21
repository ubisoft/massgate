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
#ifndef MMG_FRIENDSLADDERPROTOCOL_H
#define MMG_FRIENDSLADDERPROTOCOL_H

#include "MC_GrowingArray.h"
#include "MN_WriteMessage.h"
#include "MN_ReadMessage.h"

class MMG_FriendsLadderProtocol
{
public:
	class FriendsLadderReq
	{
	public: 
		FriendsLadderReq(); 

		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);
	
		void Add(unsigned int aProfileId); 

		unsigned int requestId; 
		MC_GrowingArray<unsigned int> myFriendsProfileIds; 
	};
	
	class FriendsLadderRsp
	{
	public: 
		FriendsLadderRsp(); 

		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		class LadderEntry 
		{
		public: 
			unsigned int myProfileId; 
			unsigned int myScore; 
		};

		void Add(unsigned int aProfileId, 
				 unsigned int aScore); 

		static void Sort(
			MC_GrowingArray<LadderEntry>& aFriendsLadderData); 

		unsigned int requestId; 
		MC_GrowingArray<LadderEntry> myFriendsLadderData; 
	};

};

#endif