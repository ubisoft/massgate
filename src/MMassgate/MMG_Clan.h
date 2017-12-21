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
#ifndef MMG_CLAN__H_
#define MMG_CLAN__H_

#include "MMG_Constants.h"

#include "MN_WriteMessage.h"
#include "MN_ReadMessage.h"
#include "MC_HybridArray.h"

namespace MMG_Clan
{
	static const unsigned int MAX_MEMBERS=512;
	class Description
	{
	public:
		void ToStream(MN_WriteMessage& aWm) const;
		bool FromStream(MN_ReadMessage& aRm);

		MMG_ClanFullNameString myFullName;
		MMG_ClanTagString myClanTag;
		unsigned int myClanId;

		bool operator<(const Description& aRhs) const { return myClanId < aRhs.myClanId; }
		bool operator>(const Description& aRhs) const { return myClanId > aRhs.myClanId; }
		bool operator==(const Description& aRhs) const { return myClanId == aRhs.myClanId; }
		Description() : myClanId(0) { }
		Description(const Description& aRhs) : myFullName(aRhs.myFullName), myClanTag(aRhs.myClanTag), myClanId(aRhs.myClanId) { }
		Description& operator=(const Description& aRhs) { if (this != &aRhs) { myFullName = aRhs.myFullName; myClanTag = aRhs.myClanTag; myClanId = aRhs.myClanId; } return *this; }
	};

	struct FullInfo
	{
		void ToStream(MN_WriteMessage& aWm) const;
		bool FromStream(MN_ReadMessage& aRm);


		FullInfo() { clanId = playerOfWeek = 0; }
		MMG_ClanFullNameString	fullClanName;
		MMG_ClanTagString		shortClanName;
		MMG_ClanMottoString		motto;
		MMG_ClanMotdString		leaderSays; // empty if listener is not part of clan
		MMG_ClanHomepageString	homepage;
		MC_HybridArray<unsigned int,32> clanMembers;
		unsigned int			clanId;
		unsigned int			playerOfWeek;
	}; 
}; // namespace MMG_Clan

#endif
