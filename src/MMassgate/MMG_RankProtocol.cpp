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
#include "MMG_RankProtocol.h"
#include "MMG_ProtocolDelimiters.h"

void
MMG_RankProtocol::LeftNextRankReq::ToStream(
	MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_LEFT_NEXTRANK_GET_REQ); 
}

bool
MMG_RankProtocol::LeftNextRankReq::FromStream(
	MN_ReadMessage& theStream)
{
	return true; 
}

//////////////////////////////////////////////////////////////////////////

void
MMG_RankProtocol::LeftNextRankRsp::ToStream(
	MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_LEFT_NEXTRANK_GET_RSP); 
	theStream.WriteUInt(nextRank); 
	theStream.WriteUInt(scoreNeeded); 
	theStream.WriteUInt(ladderPercentageNeeded); 
}

bool			
MMG_RankProtocol::LeftNextRankRsp::FromStream(
	MN_ReadMessage& theStream)
{
	bool good = true; 

	good = good && theStream.ReadUInt(nextRank); 
	good = good && theStream.ReadUInt(scoreNeeded); 
	good = good && theStream.ReadUInt(ladderPercentageNeeded); 

	return good; 
}
