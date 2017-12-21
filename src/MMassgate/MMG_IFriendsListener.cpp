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

#include "MMG_IFriendsListener.h"


MMG_IFriendsListener::Acquaintance::Acquaintance()
: numTimesPlayed(0)
, profileId(0)
{
}

MMG_IFriendsListener::Acquaintance::Acquaintance(const Acquaintance& aRhs)
: numTimesPlayed(aRhs.numTimesPlayed)
, profileId(aRhs.profileId)
{
}

MMG_IFriendsListener::Acquaintance& 
MMG_IFriendsListener::Acquaintance::operator=(const Acquaintance& aRhs)
{
	if (this != &aRhs)
	{
		numTimesPlayed = aRhs.numTimesPlayed;
		profileId = aRhs.profileId;
	}
	return *this;
}

bool 
MMG_IFriendsListener::Acquaintance::operator<(const Acquaintance& aRhs) const
{
	return profileId < aRhs.profileId;
}

bool 
MMG_IFriendsListener::Acquaintance::operator>(const Acquaintance& aRhs) const
{
	return profileId > aRhs.profileId;
}

bool 
MMG_IFriendsListener::Acquaintance::operator==(const Acquaintance& aRhs) const
{
	return (profileId == aRhs.profileId) && (numTimesPlayed == aRhs.numTimesPlayed);
}

bool 
MMG_IFriendsListener::Acquaintance::operator<(const Friend& aRhs) const
{
	return profileId < aRhs.profile.myProfileId;
}

bool 
MMG_IFriendsListener::Acquaintance::operator>(const Friend& aRhs) const
{
	return profileId > aRhs.profile.myProfileId;
}

bool 
MMG_IFriendsListener::Acquaintance::operator==(const Friend& aRhs) const
{
	return profileId == aRhs.profile.myProfileId;
}

bool 
MMG_IFriendsListener::Acquaintance::operator<(const unsigned int aRhs) const
{
	return profileId < aRhs;
}

bool 
MMG_IFriendsListener::Acquaintance::operator>(const unsigned int aRhs) const
{
	return profileId > aRhs;
}

bool 
MMG_IFriendsListener::Acquaintance::operator==(const unsigned int aRhs) const
{
	return profileId == aRhs;
}


void
MMG_IFriendsListener::Acquaintance::ToStream(MN_WriteMessage& aWm) const
{
	assert(false && "deprecated, why send this?");
	aWm.WriteUInt(profileId);
	aWm.WriteUInt(numTimesPlayed);
}

bool
MMG_IFriendsListener::Acquaintance::FromStream(MN_ReadMessage& aRm)
{
	bool good = true;
	good = good && aRm.ReadUInt(profileId);
	good = good && aRm.ReadUInt(numTimesPlayed);
	return good;
}


MMG_IFriendsListener::Friend::Friend()
{
}

MMG_IFriendsListener::Friend::Friend(const Friend& aRhs)
: profile(aRhs.profile)
{
}

MMG_IFriendsListener::Friend& 
MMG_IFriendsListener::Friend::operator=(const Friend& aRhs)
{
	if (this != &aRhs)
		profile = aRhs.profile;

	return *this;
}

bool 
MMG_IFriendsListener::Friend::operator<(const Friend& aRhs) const
{
	return profile.myName < aRhs.profile.myName;
}

bool 
MMG_IFriendsListener::Friend::operator>(const Friend& aRhs) const
{
	return profile.myName > aRhs.profile.myName;
}

bool 
MMG_IFriendsListener::Friend::operator==(const Friend& aRhs) const
{
	return profile.myProfileId == aRhs.profile.myProfileId;
}

void
MMG_IFriendsListener::Friend::ToStream(MN_WriteMessage& aWm) const
{
	profile.ToStream(aWm);
}

bool
MMG_IFriendsListener::Friend::FromStream(MN_ReadMessage& aRm)
{
	return profile.FromStream(aRm);
}

