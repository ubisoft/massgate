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
#include "stdafx.h"
#include "MMG_Profile.h"

MMG_Profile::MMG_Profile()
: myProfileId(0)
, myClanId(0)
, myRankInClan(0)
, myRank(0)
, myOnlineStatus(0)
{
}

void 
MMG_Profile::ToStream(MN_WriteMessage& aWm) const
{
	aWm.WriteLocString(myName.GetBuffer(), myName.GetLength());
	aWm.WriteUInt(myProfileId);
	aWm.WriteUInt(myClanId);
	aWm.WriteUInt(myOnlineStatus);
	aWm.WriteUChar(myRank);
	aWm.WriteUChar(myRankInClan);
}

bool 
MMG_Profile::FromStream(MN_ReadMessage& aRm)
{
	bool good = true;
	good = good && aRm.ReadLocString(myName.GetBuffer(), myName.GetBufferSize());
	good = good && aRm.ReadUInt(myProfileId);
	good = good && aRm.ReadUInt(myClanId);
	good = good && aRm.ReadUInt(myOnlineStatus);
	good = good && aRm.ReadUChar(myRank);
	good = good && aRm.ReadUChar(myRankInClan);
	return good;
}
