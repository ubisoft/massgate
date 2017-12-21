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
#include "MMG_ClanListener.h"

void
MMG_ClanListener::EditableValues::ToStream(MN_WriteMessage& aWm) const
{
	aWm.WriteLocString(motto.GetBuffer(), motto.GetLength());
	aWm.WriteLocString(leaderSays.GetBuffer(), leaderSays.GetLength());
	aWm.WriteLocString(homepage.GetBuffer(), homepage.GetLength());
	aWm.WriteUInt(playerOfWeek);
}

bool 
MMG_ClanListener::EditableValues::FromStream(MN_ReadMessage& aRm)
{
	bool good = true;

	good = good && aRm.ReadLocString(motto.GetBuffer(), motto.GetBufferSize());
	good = good && aRm.ReadLocString(leaderSays.GetBuffer(), leaderSays.GetBufferSize());
	good = good && aRm.ReadLocString(homepage.GetBuffer(), homepage.GetBufferSize());
	good = good && aRm.ReadUInt(playerOfWeek);

	return good;
}

