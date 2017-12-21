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
#pragma once

#include "MN_WriteMessage.h"
#include "MN_ReadMessage.h"

namespace MMG_ClanColosseumProtocol
{
	class Filter
	{
	public:
		void			ToStream(
							MN_WriteMessage&	theStream);
		bool			FromStream(
							MN_ReadMessage&		theStream);

		char			useEsl;
		int				fromRanking;
		int				toRanking;
		unsigned char	playerMask;
		MC_HybridArray<uint64, 5>	maps;
	};

	class FilterWeights 
	{
	public: 
		FilterWeights()
		: myClanWarsHaveMapWeight(1.0f)
		, myClanWarsDontHaveMapWeight(1.0f)
		, myClanWarsPlayersWeight(1.0f)
		, myClanWarsWrongOrderWeight(1.0f)
		, myClanWarsRatingDiffWeight(1.0f)
		{
		}

		float			myClanWarsHaveMapWeight; 
		float			myClanWarsDontHaveMapWeight; 
		float			myClanWarsPlayersWeight; 
		float			myClanWarsWrongOrderWeight;
		float			myClanWarsRatingDiffWeight;
		float			myClanWarsMaxRatingDiff; 
	};

	class FilterWeightsReq
	{
	public: 
		void			ToStream(
							MN_WriteMessage&	theStream);
		bool			FromStream(
							MN_ReadMessage&		theStream);
	};

	class FilterWeightsRsp
	{
	public:
		void			ToStream(
							MN_WriteMessage&	theStream);
		bool			FromStream(
							MN_ReadMessage&		theStream);	

		FilterWeights		myFilterWeights; 
	};

	class RegisterReq
	{
	public:
		void ToStream(
				MN_WriteMessage&	theStream);
		bool FromStream(
				MN_ReadMessage&		theStream);
		
		Filter		filter;
	};

	class UnregisterReq
	{
	public:
		void ToStream(
				MN_WriteMessage&	theStream);
		bool FromStream(
				MN_ReadMessage&		theStream);
	};

	class GetReq
	{
	public:
		void ToStream(
				MN_WriteMessage&	theStream);
		bool FromStream(
				MN_ReadMessage&		theStream);

		int			reqId;
	};

	class GetRsp
	{
	public:
		void ToStream(
				MN_WriteMessage&	theStream);
		bool FromStream(
				MN_ReadMessage&		theStream);

		int			reqId;			// -1 = pushed from server, just update
		float		myRating;

		class Entry
		{
		public:
			Entry()
			: clanId(0)
			, profileId(0)
			, ladderPos(0)
			, rating(1500.0f)
			{
			}

			int		clanId;
			int		profileId;
			int		ladderPos;
			float	rating;
			Filter	filter;
		};
		MC_HybridArray<Entry, 100>	entries;
	};
};

class MMG_IClanColosseumListener
{
public:
	virtual void RecieveClanInColosseum(
		MMG_ClanColosseumProtocol::GetRsp& aGetRsp) = 0;
};
