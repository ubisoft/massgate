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
#ifndef MMG_ACCESSCONTROL___H__
#define MMG_ACCESSCONTROL___H__

//#include "M_MG_Pr__of_ile.h"
#include "MC_GrowingArray.h"



//This class is used to match a profile against an accessstring, such as
//
//
// allow: *
// allow: wic_admins system
// allow: wic_admins
// allow: * except: bannedCdKeys bannedAccounts
// allow: wic_admins except: bannedCdKeys
//
//
// so, syntax uses * as default
//
// default groups are:
//
// wic_admins
// massgate_admins
// wic_players
// See the database for additional groups
//
//
// Additional groups can be created on a title-basis (e.g. one group per clan).
//


class MMG_AccessControl
{
public:
	MMG_AccessControl(const MC_String& theAccessControlString);
	~MMG_AccessControl();

//	bool IsAccessibleFor(const MM_G_Pr_of_ile& aProfile);

protected:

	MC_GrowingArray<MC_String> myAllowed;
	MC_GrowingArray<MC_String> myDenied;

private:
	bool BuildAccessLists(MC_String theAccessControlString);
};

#endif