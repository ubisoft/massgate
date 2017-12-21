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
#ifndef MMG_LADDERPROTOCOL_H
#define MMG_LADDERPROTOCOL_H

#include "MC_HybridArray.h"
#include "MN_WriteMessage.h"
#include "MN_ReadMessage.h"
#include "MMG_Profile.h"

class MMG_LadderProtocol
{
public:
	class LadderReq
	{
	public: 
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		unsigned int startPos; 
		unsigned int profileId; 
		unsigned int numItems; 
		unsigned int requestId; 
	};

	class LadderRsp
	{
	public: 
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		void Add(const MMG_Profile& aProfile, unsigned int aScore); 

		bool GotFullResponse() { return myResponseIsFull; }

		class LadderItem
		{
		public: 
			LadderItem()
			: score(0)
			{
			}

			LadderItem(const MMG_Profile& aProfile, 
					   unsigned int aScore)
			: profile(aProfile)
			, score(aScore)
			{
			}

			MMG_Profile profile;
			unsigned int score; 
		};

		MC_HybridArray<LadderItem, 100> ladderItems; 

		unsigned int startPos; 
		unsigned int requestId; 
		unsigned int ladderSize; 
		bool myResponseIsFull;
	};
};

#endif