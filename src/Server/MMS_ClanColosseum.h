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
#ifndef __MMS_CLANCOLOSSEUM_H__
#define __MMS_CLANCOLOSSEUM_H__

#include "MC_GrowingArray.h"
#include "MT_Mutex.h"

#include "MMG_ClanColosseumProtocol.h"

class MMS_ClanColosseum 
{
public:
	
	void						Register(
									int				aProfileId,
									int				aClanId,
									MMG_ClanColosseumProtocol::Filter&		aFilter);
	void						Unregister(
									int				aProfileId);
	
	void						GetFilterMaps(
									int				aProfileId,
									int				aClanId,
									MC_HybridArray<uint64, 255>&			aMapList);

	bool						GetOne(
									int				aProfileId,
									int				aClanId,
									MMG_ClanColosseumProtocol::GetRsp&		theRsp);
	void						FillResponse(
									MMG_ClanColosseumProtocol::GetRsp&		theRsp);

	static MMS_ClanColosseum*	GetInstance() { return ourInstance; }
private:

	typedef struct  
	{
		int					clanId;
		int					profileId;
		MMG_ClanColosseumProtocol::Filter	filter;
	}ColosseumEntry;
	
	static MMS_ClanColosseum*		ourInstance;
	MC_GrowingArray<ColosseumEntry>	myEntries;
	MT_Mutex						myMutex;

							MMS_ClanColosseum();
							~MMS_ClanColosseum();
};

#endif