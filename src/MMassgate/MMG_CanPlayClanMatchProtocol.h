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
#ifndef MMG_CANPLAYCLANMATCHPROTOCOL_H
#define MMG_CANPLAYCLANMATCHPROTOCOL_H

#include "MN_WriteMessage.h"
#include "MN_ReadMessage.h"
#include "MC_HybridArray.h"

class MMG_CanPlayClanMatchProtocol
{
public:
	class CanPlayClanMatchReq
	{
	public: 
		void			ToStream(
							MN_WriteMessage&	theStream);
		bool			FromStream(
							MN_ReadMessage&		theStream);

		void 
		Add(
			unsigned int aProfileId)
		{
			profileIds.Add(aProfileId); 
		}

		MC_HybridArray<unsigned int, 32> profileIds; 
		unsigned int requestId; 
	}; 

	class CanPlayClanMatchRsp
	{
	public: 
		void			ToStream(
							MN_WriteMessage&	theStream);
		bool			FromStream(
							MN_ReadMessage&		theStream);
		
		class CanPlay
		{
		public: 
			CanPlay()
				: profileId(0)
				, canPlay(false)
			{
			}

			CanPlay(
				unsigned int	aProfileId, 
				bool			aCanPlay)
				: profileId(aProfileId)
				, canPlay(aCanPlay)
			{
			}

			bool operator == (const CanPlay& aRhs) const 
			{
				return profileId == aRhs.profileId && canPlay == aRhs.canPlay;
			}

			unsigned int profileId; 
			bool canPlay; 
		}; 

		void 
			Add(
			unsigned int	aProfileId, 
			bool			aCanPlay)
		{
			canPlayInformation.AddUnique(CanPlay(aProfileId, aCanPlay));
		}

		MC_HybridArray<CanPlay, 32> canPlayInformation;
		unsigned int requestId; 
		unsigned int timeLimit; 
	}; 
};

class MMG_ICanPlayClanMatchListener
{
public: 
	virtual void ReceiveCanPlayClanMatchRsp(
		MMG_CanPlayClanMatchProtocol::CanPlayClanMatchRsp& aRsp) = 0;
};

#endif