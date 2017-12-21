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
#ifndef MMG_DSCHANGESERVERRANKING_H
#define MMG_DSCHANGESERVERRANKING_H

#include "MN_ReadMessage.h"
#include "MN_WriteMessage.h"

class MMG_DSChangeServerRanking
{
public:
	class ChangeRankingReq
	{
	public: 
		void	ToStream(
					MN_WriteMessage& theStream) const;

		bool	FromStream(
					MN_ReadMessage& theStream);

		bool	isRanked;
	};

	class ChangeRankingRsp
	{
	public: 
		void	ToStream(
					MN_WriteMessage& theStream) const;

		bool	FromStream(
					MN_ReadMessage& theStream);
	}; 
};

#endif // MMG_DSCHANGESERVERRANKING_H