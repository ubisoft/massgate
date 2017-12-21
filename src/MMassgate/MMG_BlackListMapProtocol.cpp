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

#include "MMG_BlackListMapProtocol.h"
#include "MMG_ProtocolDelimiters.h"

#include "MN_ReadMessage.h"
#include "MN_WriteMessage.h"

void
MMG_BlackListMapProtocol::BlackListReq::ToStream(
	MN_WriteMessage& theStream)
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_BLACKLIST_MAP_REQ); 
	theStream.WriteUInt64(myMapHash); 
}

bool
MMG_BlackListMapProtocol::BlackListReq::FromStream(
	MN_ReadMessage& theStream)
{
	bool good = true; 

	good = good && theStream.ReadUInt64(myMapHash); 

	return good; 
}

//////////////////////////////////////////////////////////////////////////

void
MMG_BlackListMapProtocol::RemoveMapReq::ToStream(
	MN_WriteMessage& theStream)
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_DS_REMOVE_MAP_REQ); 
	unsigned int count = myMapHashes.Count(); 
	
	theStream.WriteUInt(count); 
	for(unsigned int i = 0; i < count; i++)
		theStream.WriteUInt64(myMapHashes[i]); 
}

bool
MMG_BlackListMapProtocol::RemoveMapReq::FromStream(
	MN_ReadMessage& theStream)
{
	bool good = true; 

	unsigned int count; 
	good = good && theStream.ReadUInt(count);

	for(unsigned int i = 0; good && i < count; i++)
	{
		unsigned __int64 mapHash; 
		good = good && theStream.ReadUInt64(mapHash); 
		if(good)
			myMapHashes.Add(mapHash); 
	}

	return good; 
}
