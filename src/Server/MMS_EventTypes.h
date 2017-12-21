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
#ifndef MMS_EVENTTYPOES___H__
#define MMS_EVENTTYPOES___H__

namespace MMS_EventTypes
{
	struct TO_ACCOUNT
	{
		enum { NONE=1000 };
	};
	struct TO_MESSAGING
	{
		enum { NONE=2000, RESERVED, 
			INFORM_ACCEPTED_IN_TOURNAMENT, 
			INFORM_REJECTED_FROM_TOURNAMENT, 
			INFORM_TOURNAMENT_MATCH_READY,
			INFORM_TOURNAMENT_MATCH_LOST,
			INFORM_TOURNAMENT_MATCH_WON
		};
	};
	struct TO_SERVERTRACKER
	{
		enum { NONE=3000 };
	};
	struct TO_CHAT
	{
		enum { NONE=4000 };
	};
};

#endif
