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
#ifndef MMG_PROFILE_EDITABLE_VARIABLES_PROTOCOL_H
#define MMG_PROFILE_EDITABLE_VARIABLES_PROTOCOL_H

#include "MMG_Constants.h"
#include "MN_ReadMessage.h"
#include "MN_WriteMessage.h"

namespace MMG_ProfileEditableVariablesProtocol
{
	class SetReq
	{
	public: 
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		MMG_ProfileHomepageString	homepage;
		MMG_ProfileMottoString		motto;
	};

	class GetReq
	{
	public:
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		unsigned int		profileId;
	};

	class GetRsp
	{
	public: 
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		MMG_ProfileHomepageString	homepage;
		MMG_ProfileMottoString		motto;
	};

	class MMG_IProfileEditablesListener {
	public:
		virtual void HandleProfileEditablesGetRsp(MMG_ProfileEditableVariablesProtocol::GetRsp& aGetRsp) = 0;
	};
};

#endif /* MMG_PROFILE_EDITABLE_VARIABLES_PROTOCOL_H */