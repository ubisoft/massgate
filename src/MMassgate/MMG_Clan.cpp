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
#include "MMG_Clan.h"

void MMG_Clan::Description::ToStream(MN_WriteMessage& aWm) const
{
	aWm.WriteLocString(myFullName.GetBuffer(), myFullName.GetLength());
	aWm.WriteLocString(myClanTag.GetBuffer(), myClanTag.GetLength());
	aWm.WriteUInt(myClanId);
}

bool MMG_Clan::Description::FromStream(MN_ReadMessage& aRm)
{
	bool good = true;
	good = good && aRm.ReadLocString(myFullName.GetBuffer(), myFullName.GetBufferSize());
	good = good && aRm.ReadLocString(myClanTag.GetBuffer(), myClanTag.GetBufferSize());
	good = good && aRm.ReadUInt(myClanId);
	return good;
}

void 
MMG_Clan::FullInfo::ToStream(MN_WriteMessage& aWm) const
{
	aWm.WriteLocString(fullClanName.GetBuffer(), fullClanName.GetLength());
	aWm.WriteLocString(shortClanName.GetBuffer(), shortClanName.GetLength());
	aWm.WriteLocString(motto.GetBuffer(), motto.GetLength());
	aWm.WriteLocString(leaderSays.GetBuffer(), leaderSays.GetLength());
	aWm.WriteLocString(homepage.GetBuffer(), homepage.GetLength());

	aWm.WriteUInt(clanMembers.Count());
	for (int i = 0; i < clanMembers.Count() && i < MMG_Clan::MAX_MEMBERS; i++)
		aWm.WriteUInt(clanMembers[i]);

	aWm.WriteUInt(clanId);
	aWm.WriteUInt(playerOfWeek);
}

bool 
MMG_Clan::FullInfo::FromStream(MN_ReadMessage& aRm)
{
	bool good = true;

	good = good && aRm.ReadLocString(fullClanName.GetBuffer(), fullClanName.GetBufferSize());
	good = good && aRm.ReadLocString(shortClanName.GetBuffer(), shortClanName.GetBufferSize());
	good = good && aRm.ReadLocString(motto.GetBuffer(), motto.GetBufferSize());
	good = good && aRm.ReadLocString(leaderSays.GetBuffer(), leaderSays.GetBufferSize());
	good = good && aRm.ReadLocString(homepage.GetBuffer(), homepage.GetBufferSize());

	unsigned int numMembers = 0;
	unsigned int memberId;
	good = good && aRm.ReadUInt(numMembers);
	clanMembers.RemoveAll();
	numMembers = __min(numMembers, MAX_MEMBERS);
	for (unsigned int i = 0; i < numMembers; i++)
	{
		good = good && aRm.ReadUInt(memberId);
		if (good)
			clanMembers.Add(memberId);
	}

	good = good && aRm.ReadUInt(clanId) && aRm.ReadUInt(playerOfWeek);

	return good;
}
