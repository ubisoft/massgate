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
#ifndef MMG_MISCCLANPROTOCOLS_H
#define MMG_MISCCLANPROTOCOLS_H

#include "MC_HybridArray.h"
#include "MN_WriteMessage.h"
#include "MN_ReadMessage.h"

class MMG_MiscClanProtocols
{
public:
	class TopTenWinningStreakReq 
	{
	public:
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);
	};

	class TopTenWinningStreakRsp
	{
	public: 
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		class StreakData
		{
		public: 
			unsigned int myClanId; 
			MMG_ClanFullNameString myClanName; 
			unsigned int myStreak; 
		};

		void Add(unsigned int aClanId, MMG_ClanFullNameString aClanName, unsigned int aStreak); 

		MC_HybridArray<StreakData, 10> myStreakData; 
	};
};

#endif