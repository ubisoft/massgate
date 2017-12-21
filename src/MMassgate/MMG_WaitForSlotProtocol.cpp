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
#include "MMG_WaitForSlotProtocol.h"

#include "MMG_ProtocolDelimiters.h"

void 
MMG_WaitForSlotProtocol::ClientToMassgateGetDSQueueSpotReq::ToStream(
	MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_QUEUE_SPOT_REQ); 
	theStream.WriteUInt(serverId); 
}

bool 
MMG_WaitForSlotProtocol::ClientToMassgateGetDSQueueSpotReq::FromStream(
	MN_ReadMessage& theStream)
{
	return theStream.ReadUInt(serverId); 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_WaitForSlotProtocol::MassgateToDSGetDSQueueSpotReq::ToStream(
	MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_DS_GET_QUEUE_SPOT_REQ); 
	theStream.WriteUInt(profileId); 
}

bool 
MMG_WaitForSlotProtocol::MassgateToDSGetDSQueueSpotReq::FromStream(
	MN_ReadMessage& theStream)
{
	return theStream.ReadUInt(profileId); 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_WaitForSlotProtocol::DSToMassgateGetDSQueueSpotRsp::ToStream(
	MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_DS_GET_QUEUE_SPOT_RSP); 
	theStream.WriteUInt(profileId); 
	theStream.WriteUInt(cookie); 
}

bool 
MMG_WaitForSlotProtocol::DSToMassgateGetDSQueueSpotRsp::FromStream(
	MN_ReadMessage& theStream)
{
	bool good = true; 

	good = good && theStream.ReadUInt(profileId); 
	good = good && theStream.ReadUInt(cookie);

	return good; 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_WaitForSlotProtocol::MassgateToClientGetDSQueueSpotRsp::ToStream(
	MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_QUEUE_SPOT_RSP); 
	theStream.WriteUInt(serverId); 
	theStream.WriteUInt(cookie); 
}

bool 
MMG_WaitForSlotProtocol::MassgateToClientGetDSQueueSpotRsp::FromStream(
	MN_ReadMessage& theStream)
{
	bool good = true; 

	good = good && theStream.ReadUInt(serverId); 
	good = good && theStream.ReadUInt(cookie);

	return good; 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_WaitForSlotProtocol::ClientToMassgateRemoveDSQueueSpotReq::ToStream(
	MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_REMOVE_QUEUE_SPOT_REQ); 
	theStream.WriteUInt(serverId); 
}

bool 
MMG_WaitForSlotProtocol::ClientToMassgateRemoveDSQueueSpotReq::FromStream(
	MN_ReadMessage& theStream)
{
	return theStream.ReadUInt(serverId); 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_WaitForSlotProtocol::MassgateToDSRemoveDSQueueSpotReq::ToStream(
	MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_DS_REMOVE_QUEUE_SPOT_REQ); 
	theStream.WriteUInt(profileId); 
}

bool 
MMG_WaitForSlotProtocol::MassgateToDSRemoveDSQueueSpotReq::FromStream(
	MN_ReadMessage& theStream)
{
	return theStream.ReadUInt(profileId); 
}
