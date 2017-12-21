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
#include "MMG_ProfileIgnoreProtocol.h"
#include "MMG_ProtocolDelimiters.h"

 
void 
MMG_ProfileIgnoreProtocol::AddRemoveIgnoreReq::ToStream(MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_IGNORELIST_ADD_REMOVE_REQ);
	theStream.WriteUInt(profileId);
	theStream.WriteUChar(ignoreYesNo);
}

bool 
MMG_ProfileIgnoreProtocol::AddRemoveIgnoreReq::FromStream(MN_ReadMessage& theStream)
{
	bool good = true; 

	good = good && theStream.ReadUInt(profileId);
	good = good && theStream.ReadUChar(ignoreYesNo);

	return good; 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_ProfileIgnoreProtocol::IgnoreListGetReq::ToStream(MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_IGNORELIST_GET_REQ);
}

bool 
MMG_ProfileIgnoreProtocol::IgnoreListGetReq::FromStream(MN_ReadMessage& theStream)
{
	return true; 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_ProfileIgnoreProtocol::IgnoreListGetRsp::ToStream(MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_IGNORELIST_GET_RSP);
	theStream.WriteUInt(ignoredProfiles.Count());
	for(int i = 0; i < ignoredProfiles.Count(); i++)
		theStream.WriteUInt(ignoredProfiles[i]);
}

bool 
MMG_ProfileIgnoreProtocol::IgnoreListGetRsp::FromStream(MN_ReadMessage& theStream)
{
	bool good = true; 

	unsigned int numProfiles = 0; 
	good = good && theStream.ReadUInt(numProfiles);
	for(unsigned int i = 0; good && i < numProfiles; i++)
	{
		unsigned int tmp = 0; 
		good = good && theStream.ReadUInt(tmp);
		if(good)
			ignoredProfiles.Add(tmp);
	}

	return good; 
}
