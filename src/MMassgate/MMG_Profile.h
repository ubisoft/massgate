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
#ifndef MMG_PROFILE_H_
#define MMG_PROFILE_H_

#include "MC_String.h"
#include "MC_GrowingArray.h"

#include "MMG_Constants.h"

#include "MN_WriteMessage.h"
#include "MN_ReadMessage.h"


/* NOTE! The stats and medals defined here should match those in MMG_Stats, but this format is used to send
 * complete info on a player to a client, the other format is a tighter format for DS stats reporting.
 */


class MMG_Profile
{
public:
	MMG_Profile();

	void CreateTaggedName(const MC_LocChar* aTagFormat, const MC_LocChar* aClanShortName)
	{
		MC_LocChar newName[64];
		MC_LocChar* newNamePtr = &newName[0];
		const MC_LocChar* tagPtr = aTagFormat;
		int tmp;
		MC_LocChar val;
		do
		{
			val = *tagPtr++;
			switch(val)
			{
			case L'P':
				tmp = myName.GetLength();
				memcpy(newNamePtr, myName.GetBuffer(), tmp*2);
				newNamePtr += tmp;
				break;
			case L'C':
				tmp = (int)wcslen(aClanShortName);
				memcpy(newNamePtr, aClanShortName, tmp*2);
				newNamePtr += tmp;
				break;
			default:
				*newNamePtr++ = val;
				break;
			};
		} while (val);
		myName = newName;
	}

	void ToStream(MN_WriteMessage& aWm) const;
	bool FromStream(MN_ReadMessage& aRm);

	bool operator == (const MMG_Profile& aRhs) const
	{
		if(myProfileId == aRhs.myProfileId)
			return true; 
		return false; 
	}

	MMG_ProfilenameString	myName;
	unsigned int	myProfileId;
	unsigned int	myClanId;			// 0=no clan, -1 = unknown, n=clanid
	unsigned int	myOnlineStatus;		// 0=offline, 1=online, x=playing on server id x
	unsigned char	myRank;				// 0=private, 1=sergeant, etc
	unsigned char	myRankInClan;		// 0=not in clan, 1=leader, 2=officer, 3=grunt
};

#endif
