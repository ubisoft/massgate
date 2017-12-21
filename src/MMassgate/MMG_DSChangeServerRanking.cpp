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
#include "MMG_DSChangeServerRanking.h"
#include "MMG_ProtocolDelimiters.h"

void
MMG_DSChangeServerRanking::ChangeRankingReq::ToStream(
	MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_CHANGE_RANKING_REQ);
	theStream.WriteBool(isRanked); 
}

bool
MMG_DSChangeServerRanking::ChangeRankingReq::FromStream(
	MN_ReadMessage& theStream)
{
	return theStream.ReadBool(isRanked);
}

//////////////////////////////////////////////////////////////////////////

void
MMG_DSChangeServerRanking::ChangeRankingRsp::ToStream(
	MN_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_CHANGE_RANKING_RSP);
}

bool
MMG_DSChangeServerRanking::ChangeRankingRsp::FromStream(
	MN_ReadMessage& theStream)
{
	return true; 
}
