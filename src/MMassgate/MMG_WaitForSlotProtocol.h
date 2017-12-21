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
#ifndef MMG_WAITFORSLOTPROTOCOL_H
#define MMG_WAITFORSLOTPROTOCOL_H

#include "MN_WriteMessage.h"
#include "MN_ReadMessage.h"

class MMG_WaitForSlotProtocol
{
public:

	class ClientToMassgateGetDSQueueSpotReq
	{
	public: 
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		unsigned int serverId; 
	};

	class MassgateToDSGetDSQueueSpotReq
	{
	public: 
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);
	
		unsigned int profileId; 
	};

	class DSToMassgateGetDSQueueSpotRsp
	{
	public: 
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);
	
		unsigned int profileId; 
		unsigned int cookie; 
	};

	class MassgateToClientGetDSQueueSpotRsp
	{
	public: 
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);
	
		unsigned int serverId; 
		unsigned int cookie; 
	};

	class ClientToMassgateRemoveDSQueueSpotReq
	{
	public: 
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);
	
		unsigned int serverId; 
	};

	class MassgateToDSRemoveDSQueueSpotReq
	{
	public: 
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);
	
		unsigned int profileId; 
	};
};

class MMG_IWaitForSlotListener
{
public: 
	virtual void OnSlotReceived(MMG_WaitForSlotProtocol::MassgateToClientGetDSQueueSpotRsp& aRsp) = 0; 
};

#endif