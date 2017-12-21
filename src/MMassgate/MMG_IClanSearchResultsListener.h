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
#ifndef MMG_ICLANSEARCHNOTIFICATIONS___H_
#define MMG_ICLANSEARCHNOTIFICATIONS___H_

#include "MMG_Clan.h"

class MMG_IClanSearchResultsListener
{
public:
	virtual void ReceiveMatchingClan(const MMG_Clan::Description& theClan) = 0;
	virtual void NoMoreMatchingClansFound(bool didGetAnyResults) = 0;
};

#endif
