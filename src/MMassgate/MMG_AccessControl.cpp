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

#include "MC_Debug.h"

#include "MMG_AccessControl.h"


void testAccessControl()
{
/*	static bool hasTestedAccessControl=false;
	if (hasTestedAccessControl)
		return;
	hasTestedAccessControl = true;

	MM_G_Pr__ofile prof1, prof2, prof3, prof4, prof5;
	prof1.myGroups.Add("wic_player");

	prof2.myGroups.Add("wic_player");
	prof2.myGroups.Add("sysadmin");

	prof3.myGroups.Add("wic_player");
	prof3.myGroups.Add("banned");
	prof3.myGroups.Add("sysadmin");

	prof4.myGroups.Add("massgate");
	prof4.myGroups.Add("sysadmin");
	prof4.myGroups.Add("banned");
	prof4.myGroups.Add("wic_player");

	prof5.myGroups.Add("wic_player");
	prof5.myGroups.Add("wic_admin");
	prof5.myGroups.Add("massgate");
	prof5.myGroups.Add("banned");
	prof5.myGroups.Add("clan_ROCCKS");

	MMG_AccessControl access1("allow: *");
	MMG_AccessControl access2("allow: * except: banned");
	MMG_AccessControl access3("allow: wic_player except: banned");
	MMG_AccessControl access4("allow: clan_ROCCKS sysadmin");
	MMG_AccessControl access5("allow: wic_player except: *");

	MM_G_P__rofi-le* profs[5];
	MMG_AccessControl* accesses[5];
	profs[0] = &prof1;
	profs[1] = &prof2;
	profs[2] = &prof3;
	profs[3] = &prof4;
	profs[4] = &prof5;
	accesses[0] = &access1;
	accesses[1] = &access2;
	accesses[2] = &access3;
	accesses[3] = &access4;
	accesses[4] = &access5;

	bool truthtable[5][5] = {	{ true, true, true, false, false } ,
								{ true, true, true, true, false } ,
								{ true, false, false, true, false } ,
								{ true, false, false, true, false } ,
								{ true, false, false, true, false } 
							};

	for (int x = 0; x < 5; x++)
		for (int y = 0; y < 5; y++)
			assert(truthtable[y][x] == accesses[x]->IsAccessibleFor(*profs[y]));
*/
}


MMG_AccessControl::MMG_AccessControl(const MC_String& theAccessControlString)
{
#ifdef _DEBUG
	testAccessControl();
#endif // _DEBUG
	assert(false && "support removed. fix later");
/*
	myAllowed.Init(4,4);
	myDenied.Init(4,4);

	if(!BuildAccessLists(theAccessControlString))
	{
		MC_DEBUG("Invalid format of access control string: '%.512s'", (const char*)theAccessControlString);
	}
	*/
}

MMG_AccessControl::~MMG_AccessControl()
{
}

/*
bool
MMG_AccessControl::IsAccessibleFor(const M_MG_Pr_ofile& aProfile)
{
	bool explicitAllow = false;

	// Check for match in allow-list
	for (int i=0; i < aProfile.myGroups.Count(); i++)
	{
		for (int j = 0; j < myAllowed.Count(); j++)
		{
			if ((myAllowed[j] == "*") || (myAllowed[j] == aProfile.myGroups[i]))
				explicitAllow = true;
		}
	}

	// bail out if not allowed
	if (!explicitAllow)
		return false;

	// Check for overrides in except-list
	for (int i=0; i < aProfile.myGroups.Count(); i++)
	{
		for (int j = 0; j < myDenied.Count(); j++)
		{
			if ((myDenied[j] == "*") || (myDenied[j] == aProfile.myGroups[i]))
				return false;
		}
	}

	return explicitAllow;
}
*/

bool
MMG_AccessControl::BuildAccessLists(MC_String theAccessControlString)
{
	assert(false && "support removed. fix later");
	return false;
/*
	theAccessControlString.TrimLeft();
	theAccessControlString.TrimRight();
	
	if(theAccessControlString.Left(7) != "allow: ")
	{
		MC_DEBUG("Illegal format of accessspecifier. Ignoring.");
		return false;
	}

	MC_String exceptions;
	const int exceptstart = theAccessControlString.Find("except: ");
	if (exceptstart >= 0)
	{
		exceptions = theAccessControlString.Right(theAccessControlString.GetLength() - exceptstart - 8);
		theAccessControlString = theAccessControlString.Left(exceptstart);
	}
	theAccessControlString = theAccessControlString.Right(theAccessControlString.GetLength() - 7);

	char group[64] = {0};
	size_t offset = 0;
	const char* srcstr = (const char*)theAccessControlString;

	while (sscanf(srcstr + offset, "%64s", &group))
	{
		if (group[0] == 0) break;
		offset += strlen(group);
		myAllowed.Add(group);
		group[0] = 0;
	}
	offset = 0;
	while (sscanf((const char*)exceptions + offset, "%64s", &group))
	{
		if (group[0] == 0) break;
		offset += strlen(group);
		myDenied.Add(group);
		group[0] = 0;
	}

	return myAllowed.Count() > 0;
	*/
}

