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
#include "MMG_ProfileEditableVariablesProtocol.h"

using namespace MMG_ProtocolDelimiters;

namespace MMG_ProfileEditableVariablesProtocol
{
	void
	SetReq::ToStream(MN_WriteMessage& theStream) const
	{
		theStream.WriteDelimiter(MESSAGING_PROFILE_SET_EDITABLES_REQ);
		theStream.WriteLocString(motto.GetBuffer(), motto.GetLength());
		theStream.WriteLocString(homepage.GetBuffer(), homepage.GetLength());
	}

	bool
	SetReq::FromStream(MN_ReadMessage& theStream)
	{
		bool		good = theStream.ReadLocString(motto.GetBuffer(), motto.GetBufferSize());
		good = good && theStream.ReadLocString(homepage.GetBuffer(), homepage.GetBufferSize());

		return good;
	}

	void
	GetReq::ToStream(MN_WriteMessage& theStream) const
	{
		theStream.WriteDelimiter(MESSAGING_PROFILE_GET_EDITABLES_REQ);
		theStream.WriteUInt(profileId);
	}

	bool
	GetReq::FromStream(MN_ReadMessage& theStream)
	{
		return theStream.ReadUInt(profileId);
	}

	void
	GetRsp::ToStream(MN_WriteMessage& theStream) const
	{
		theStream.WriteDelimiter(MESSAGING_PROFILE_GET_EDITABLES_RSP);
		theStream.WriteLocString(motto.GetBuffer(), motto.GetLength());
		theStream.WriteLocString(homepage.GetBuffer(), homepage.GetLength());
	}

	bool
	GetRsp::FromStream(MN_ReadMessage& theStream)
	{
		bool		good = theStream.ReadLocString(motto.GetBuffer(), motto.GetBufferSize());
		good = good && theStream.ReadLocString(homepage.GetBuffer(), homepage.GetBufferSize());

		return good;
	}
};