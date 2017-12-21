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
#ifndef MMG_SERVERFILTER____H__
#define MMG_SERVERFILTER____H__

#include "MMG_Optional.h"
#include "MMG_IStreamable.h"
#include "MC_String.h"
#include "MN_ReadMessage.h"
#include "MN_WriteMessage.h"

class MMG_ServerFilter: public MMG_IStreamable
{
public:
	MMG_ServerFilter();
	MMG_ServerFilter& operator=(const MMG_ServerFilter& aRhs);
	
	// only compares if self matches with rhs - for the variables that are set locally.
	bool operator==(const MMG_ServerFilter& aRhs);
	void Clear();

	Optional<MC_LocString>	partOfServerName;
	Optional<MC_LocString>	partOfMapName;
	Optional<bool>			requiresPassword;
	Optional<bool>			isDedicated;
	Optional<bool>			isRanked;
	Optional<bool>			isRankBalanced;
	Optional<bool>			hasDominationMaps;
	Optional<bool>			hasAssaultMaps;
	Optional<bool>			hasTowMaps;
	Optional<bool>			noMod;
	Optional<char>			totalSlots;
	Optional<char>			emptySlots;
	Optional<char>			notFullEmpty;
	Optional<bool>			fromPlayNow;

	unsigned int			myGameVersion;
	unsigned int			myProtocolVersion;

	void ToStream(MN_WriteMessage& theStream) const;
	bool FromStream(MN_ReadMessage& theStream);
};


#endif
