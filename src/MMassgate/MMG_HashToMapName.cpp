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
#include "MMG_HashToMapName.h"

MMG_HashToMapName::MMG_HashToMapName()
{
}

MMG_HashToMapName::~MMG_HashToMapName()
{
}

MMG_HashToMapName&	
MMG_HashToMapName::GetInstance()
{
	static MMG_HashToMapName* instance = new MMG_HashToMapName();
	return *instance; 
}

void
MMG_HashToMapName::Add(
	unsigned __int64		aMapHash, 
	MC_StaticLocString<64>&	aMapName)
{
	if(myMaps.Exists(aMapHash))
		return; 

	myMaps[aMapHash] = aMapName; 
}

bool
MMG_HashToMapName::GetName(
	unsigned __int64		aMapHash, 
	MC_StaticLocString<64>&	aMapName)
{
	if(!myMaps.Exists(aMapHash))
		return false; 

	aMapName = myMaps[aMapHash]; 

	return true; 
}

