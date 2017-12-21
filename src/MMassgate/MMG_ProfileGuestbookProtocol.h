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
#ifndef MMG_PROFILEGUESTBOOKPROTOCOL_H
#define MMG_PROFILEGUESTBOOKPROTOCOL_H

namespace MMG_ProfileGuestbookProtocol {

	static const unsigned int MSG_MAX_LEN = 128; 
	static const unsigned int MAX_NUM_MSGS = 32; 

	class PostReq
	{
	public: 
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		MC_StaticLocString<MSG_MAX_LEN> msg; 
		unsigned int profileId; 
		unsigned int getGuestbook;  
		unsigned int requestId; 
	};

	class GetReq
	{
	public: 
		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		unsigned int requestId; 
		unsigned int profileId;
	};

	class GetRsp
	{
	public: 
		GetRsp(); 

		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		void AddMessage(MC_LocChar* aMessage, unsigned int aTimestamp, unsigned int aProfileId, unsigned int aMessageId);

		unsigned int requestId; 
		unsigned char ignoresGettingProfile;

		class GuestbookEntry  
		{
		public:  
			GuestbookEntry()
				: timestamp(0)
				, profileId(0)
				, messageId(0)
			{
			}

			GuestbookEntry(MC_LocChar* aMsg, 
				unsigned int aTimestamp, 
				unsigned int aProfileId, 
				unsigned int aMessageId)
				: msg(aMsg)
				, timestamp(aTimestamp)
				, profileId(aProfileId)
				, messageId(aMessageId)
			{
			}

			MC_StaticLocString<MSG_MAX_LEN> msg; 
			unsigned int timestamp; 
			unsigned int profileId; 
			unsigned int messageId; 
		};

		MC_HybridArray<GuestbookEntry, MAX_NUM_MSGS> messages; 
	};

	class DeleteReq
	{
	public: 
		DeleteReq()
		: messageId(-1)
		, deleteAllByThisProfile(false)
		{
		}

		void ToStream(MN_WriteMessage& theStream) const;
		bool FromStream(MN_ReadMessage& theStream);

		unsigned int messageId; 
		bool deleteAllByThisProfile; 
	};

	class MMG_IProfileGuestbookListener
	{
	public: 
		virtual void HandleProfileGuestbookGetRsp(MMG_ProfileGuestbookProtocol::GetRsp& aGetRsp) = 0; 
	};

}

#endif /* MMG_PROFILEGUESTBOOKPROTOCOL_H */