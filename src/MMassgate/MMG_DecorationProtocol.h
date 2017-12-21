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
#ifndef MMG_DECORATIONPROTOCOL_H
#define MMG_DECORATIONPROTOCOL_H

#include "MC_HybridArray.h"

class MMG_DecorationProtocol
{
public:
	class PlayerMedalsReq
	{
	public:	
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		unsigned int profileId;
	};

	class PlayerMedalsRsp 
	{
	public:
		class Medal 
		{
		public: 
			Medal()
			: medalLevel(0)
			, numStars(0)
			{
			}
			Medal(unsigned short aMedalLevel, unsigned short aNumStars)
			: medalLevel(aMedalLevel)
			, numStars(aNumStars)
			{
			}
			unsigned short medalLevel;
			unsigned short numStars;
		};

		void Add(unsigned short aMedalLevel, unsigned short aNumStars);

		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		MC_HybridArray<Medal, 19> medals;
		unsigned int profileId;
	};

	class PlayerBadgesReq
	{
	public:	
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		unsigned int profileId;
	};

	class PlayerBadgesRsp
	{
	public: 
		class Badge
		{
		public:
			Badge()
			: badgeLevel(0)
			, numStars(0)
			{
			}
			Badge(unsigned short aBadgeLevel, unsigned short aNumStars)
			: badgeLevel(aBadgeLevel)
			, numStars(aNumStars)
			{
			}
			unsigned short badgeLevel;
			unsigned short numStars;
		};

		void Add(unsigned short aBadgeLevel, unsigned short aNumStars);

		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		MC_HybridArray<Badge, 14> badges;

		unsigned int profileId; 
	};

	class ClanMedalsReq
	{
	public: 
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		unsigned int clanId;
	};

	class ClanMedalsRsp
	{
	public: 
		class Medal 
		{
		public: 
			Medal()
				: medalLevel(0)
				, numStars(0)
			{
			}
			Medal(unsigned short aMedalLevel, unsigned short aNumStars)
				: medalLevel(aMedalLevel)
				, numStars(aNumStars)
			{
			}
			unsigned short medalLevel;
			unsigned short numStars;
		};

		void Add(unsigned short aMedalLevel, unsigned short aNumStars);

		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		MC_HybridArray<Medal, 9> medals;
		unsigned int clanId;
	};

private:

};

#endif