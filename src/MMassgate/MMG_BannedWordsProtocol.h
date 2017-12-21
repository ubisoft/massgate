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
#ifndef MMG_BANNEDWORDSPROTOCOL_H
#define MMG_BANNEDWORDSPROTOCOL_H

#include "MN_WriteMessage.h"
#include "MN_ReadMessage.h"
#include "MC_HybridArray.h"
#include "MC_String.h"

class MMG_BannedWordsProtocol
{
public:

	class GetReq
	{
	public: 
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);
	};

	class GetRsp
	{
	public: 
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		void Add(MC_LocString& aString);

		MC_HybridArray<MC_StaticLocString<225>, 256> myBannedStrings; 
	};
};

#endif