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
#ifndef MMG_CONSTANTS__H_
#define MMG_CONSTANTS__H_

// Bring in Massgate protocol version numbers etc
#include "MMG_Protocols.h"


// Set some common things
#define MMG_ClanShortTagStringSize		5
#define MMG_ClanTagExtraStringSize		4 // -==-
#define MMG_ClanTagStringSize			(MMG_ClanShortTagStringSize+MMG_ClanTagExtraStringSize+1)
#define MMG_ClanFullNameStringSize		31

#define MMG_ProfilenameStringSize		(14+MMG_ClanTagStringSize)
#define MMG_EmailStringSize				63
#define MMG_PasswordStringSize			15
#define MMG_InstantMessageStringSize	255
#define MMG_GameNameStringSize			63
#define MMG_ClanMottoStringSize			255
#define MMG_ClanMotdStringSize			255
#define MMG_ClanHomepageStringSize		255
#define MMG_ProfileMottoStringSize			255
#define MMG_ProfileHomepageStringSize		255

// these are to make the clan edit UI behave 
#define MMG_ClanMottoMaxInputSize		120
#define MMG_ClanMotdMaxInputSize		120

typedef MC_StaticLocString	<32>								MMG_TournamentFullNameString;
typedef MC_StaticLocString	<MMG_GameNameStringSize+1>			MMG_GameNameString;
typedef MC_StaticLocString	<MMG_ProfilenameStringSize+1>		MMG_ProfilenameString;
typedef MC_StaticLocString	<MMG_ClanFullNameStringSize+1>		MMG_ClanFullNameString;
typedef MC_StaticLocString	<MMG_ClanTagStringSize+1>			MMG_ClanTagString;
typedef MC_StaticLocString  <MMG_ClanMottoStringSize+1>			MMG_ClanMottoString; 
typedef MC_StaticLocString  <MMG_ClanMotdStringSize+1>			MMG_ClanMotdString; 
typedef MC_StaticLocString	<MMG_ClanHomepageStringSize+1>		MMG_ClanHomepageString; 
typedef MC_StaticString		<MMG_EmailStringSize+1>				MMG_EmailString;
typedef MC_StaticLocString	<MMG_PasswordStringSize+1>			MMG_PasswordString;
typedef MC_StaticLocString	<MMG_InstantMessageStringSize+1>	MMG_InstantMessageString;
typedef MC_StaticLocString	<MMG_ProfileHomepageStringSize+1>	MMG_ProfileHomepageString;
typedef MC_StaticLocString  <MMG_ProfileMottoStringSize+1>		MMG_ProfileMottoString; 
typedef MC_StaticLocString	<MMG_InstantMessageStringSize+1>	MMG_ClanMessageString;

class MMG_ClanName
{
public:
	MMG_ClanName()
		: clanId(0)
	{
	}

	MMG_ClanName(
		unsigned int aClanId, 
		MMG_ClanFullNameString& aClanName)
		: clanId(aClanId)
		, clanName(aClanName)
	{
	}

	bool operator==(const MMG_ClanName& aRhs) const
	{
		return this->clanId == aRhs.clanId;
	}

	unsigned int clanId;
	MMG_ClanFullNameString clanName;
};

#define MMG_MAX_BUFFER_SIZE				4096

#define MASSGATE_DISTRIBUTION_ID_FOR_WORLD_IN_CONFLICT		(1)
#define MASSGATE_PRODUCT_ID_FOR_WORLD_IN_CONFLICT			(1)

// for the love of god don't change the values the ProductId enum, 
// we have no idea at how many places these values are hard coded 
typedef enum 
{
	PRODUCT_ID_WIC07_STANDARD_KEY = 1,
	PRODUCT_ID_WIC07_TIMELIMITED_KEY,
	PRODUCT_ID_WIC08_STANDARD_KEY,
	PRODUCT_ID_WIC08_TIMELIMITED_KEY
} ProductId; 

#endif
