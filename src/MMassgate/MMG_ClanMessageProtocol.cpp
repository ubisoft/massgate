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
#include "MMG_ProtocolDelimiters.h"
#include "MMG_ClanMessageProtocol.h"

namespace MMG_ClanMessageProtocol
{
	void
	SendReq::ToStream(
		MN_WriteMessage&		theStream) const
	{
		theStream.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_MESSAGE_SEND_REQ);
		theStream.WriteLocString(message.GetBuffer(), message.GetLength());
	}

	bool
	SendReq::FromStream(
		MN_ReadMessage&			theStream)
	{
		bool		good = theStream.ReadLocString(message.GetBuffer(), MMG_InstantMessageStringSize);

		return good;
	}
}
