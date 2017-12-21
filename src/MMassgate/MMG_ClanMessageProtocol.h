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
#ifndef MMG_CLANMESSAGEALLPROTOCOL_H
#define MMG_CLANMESSAGEALLPROTOCOL_H

#include "mmg_istreamable.h"

namespace MMG_ClanMessageProtocol
{
	class SendReq {
	public:

		void					ToStream(
									MN_WriteMessage&		theStream) const;
		bool					FromStream(
									MN_ReadMessage&			theStream);
		
		MMG_ClanMessageString	message;
	};

	class IClanMessageListener {
	public:
		virtual void			RecieveClanMessage(
									const MMG_ClanMessageString&	aMessage,
									unsigned int					aTimestamp) = 0;
	};
}

#endif /* MMG_CLANMESSAGEALLPROTOCOL_H */