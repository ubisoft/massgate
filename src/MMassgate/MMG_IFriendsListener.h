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
#ifndef MMG_IFRIENDSLISTENER___H___
#define MMG_IFRIENDSLISTENER___H___

#include "MMG_Constants.h"
#include "MMG_Profile.h"
#include "MN_WriteMessage.h"
#include "MN_ReadMessage.h"

class MMG_IFriendsListener
{
public:
	struct Friend
	{
		MMG_Profile profile;
		void ToStream(MN_WriteMessage& aWm) const;
		bool FromStream(MN_ReadMessage& aRm);
		Friend();
		Friend(const Friend& aRhs);
		Friend& operator=(const Friend& aRhs);
		bool operator<(const Friend& aRhs) const;
		bool operator>(const Friend& aRhs) const;
		bool operator==(const Friend& aRhs) const;
	};
	struct Acquaintance
	{
		unsigned int profileId;
		unsigned int numTimesPlayed;
		void ToStream(MN_WriteMessage& aWm) const;
		bool FromStream(MN_ReadMessage& aRm);
		Acquaintance();
		Acquaintance(const Acquaintance& aRhs);
		Acquaintance& operator=(const Acquaintance& aRhs);
		bool operator<(const Acquaintance& aRhs) const;
		bool operator>(const Acquaintance& aRhs) const;
		bool operator==(const Acquaintance& aRhs) const;
		bool operator<(const Friend& aRhs) const;
		bool operator>(const Friend& aRhs) const;
		bool operator==(const Friend& aRhs) const;
		bool operator<(const unsigned int aRhs) const;
		bool operator>(const unsigned int aRhs) const;
		bool operator==(const unsigned int aRhs) const;
	};

	virtual void ReceiveFriend(const unsigned int aProfileId) = 0;
	virtual void ReceiveAcquaintance(const unsigned int aProfileId) = 0;
	virtual void RemoveAcquaintance(const unsigned int aProfileId) = 0;
	virtual void RemoveFriend(const unsigned int aProfileId) = 0;
	virtual void INTERFACE_IS_DEPRECATED() = 0;

};

#endif
