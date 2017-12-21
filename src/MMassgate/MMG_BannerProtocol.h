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
#ifndef MMG_BANNER_PROTOCOL_H
#define MMG_BANNER_PROTOCOL_H

#include "MN_WriteMessage.h"
#include "MN_ReadMessage.h"

class MMG_BannerProtocol
{
public:
	class ClientToMassgateGetBannersBySupplierId
	{
	public: 
		ClientToMassgateGetBannersBySupplierId(); 

		void ToStream(MN_WriteMessage& aWm); 
		bool FromStream(MN_ReadMessage& aRm); 
		bool IsSane(); 

		unsigned int mySupplierId; 
		unsigned int myReservedDummy; 
	};
	
	class MassgateToClientGetBannersBySupplierId
	{
	public: 
		MassgateToClientGetBannersBySupplierId(); 

		void ToStream(MN_WriteMessage& aWm); 
		bool FromStream(MN_ReadMessage& aRm); 
		bool IsSane(); 

		void AddBanner(const char *anURL, 
					   const unsigned __int64 aHash, 
					   unsigned char aType); 

		struct BannerInfo 
		{
			BannerInfo()
			: myHash(0)
			, myType(0)
			{
			}

			BannerInfo(const char* aBannerURL, 
					   unsigned __int64 aBannerHash, 
					   unsigned char aType)
			: myURL(aBannerURL)
			, myHash(aBannerHash)
			, myType(aType)
			{
			}

			MC_StaticString<256> myURL; 
			unsigned __int64 myHash; 
			unsigned char myType; 
		};

		MC_GrowingArray<BannerInfo> myBannerInfo; 

		unsigned int mySupplierId; 
		unsigned int myReservedDummy; 
	};

	class ClientToMassgateAckBanner
	{
	public: 
		ClientToMassgateAckBanner(); 

		void ToStream(MN_WriteMessage& aWm); 
		bool FromStream(MN_ReadMessage& aRm); 
		bool IsSane(); 
		
		unsigned __int64 myBannerHash; 
		unsigned int myReserveredDummy; 
		unsigned char myType; 
	};

	class MassgateToClientAckBanner
	{
	public: 
		MassgateToClientAckBanner(); 

		void ToStream(MN_WriteMessage& aWm); 
		bool FromStream(MN_ReadMessage& aRm); 
		bool IsSane(); 

		MC_StaticString<256> myNewBannerUrl;
		unsigned __int64 myBannerHash;
		unsigned __int64 myNewBannerHash;
		unsigned int myReservedDummy; 
		unsigned char myBannerRotationTime:7; 
		unsigned char myBannerIsValid:1;
	};

	class ClientToMassgateGetBannerByHash
	{
	public: 
		ClientToMassgateGetBannerByHash(); 

		void ToStream(MN_WriteMessage& aWm); 
		bool FromStream(MN_ReadMessage& aRm); 
		bool IsSane(); 

		unsigned __int64 myBannerHash; 
		unsigned int myReservedDummy; 
	};

	class MassgateToClientGetBannerByHash
	{
	public: 
		MassgateToClientGetBannerByHash(); 

		void ToStream(MN_WriteMessage& aWm); 
		bool FromStream(MN_ReadMessage& aRm); 
		bool IsSane(); 

		MC_StaticString<256> myBannerURL; 
		unsigned __int64 myBannerHash; 
		unsigned int mySupplierId; 
		unsigned int myReservedDummy; 
		unsigned char myType; 
	};
};

class MMG_IBannerMessagingListener
{
public:
	virtual void HandleGetBannersBySupplierId(const MMG_BannerProtocol::MassgateToClientGetBannersBySupplierId& someBanners) = 0;
	virtual void HandleGetBannerAck(const MMG_BannerProtocol::MassgateToClientAckBanner& aBannerAck) = 0; 
	virtual void HandleGetBannerByHash(const MMG_BannerProtocol::MassgateToClientGetBannerByHash& aBanner) = 0; 
};

#endif
