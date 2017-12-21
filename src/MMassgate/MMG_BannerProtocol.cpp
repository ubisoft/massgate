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
#include "StdAfx.h"
#include "MMG_BannerProtocol.h"
#include "MMG_Messaging.h"
#include "MMG_ProtocolDelimiters.h"


MMG_BannerProtocol::ClientToMassgateGetBannersBySupplierId::ClientToMassgateGetBannersBySupplierId()
: mySupplierId(0)
, myReservedDummy(0)
{
}

void 
MMG_BannerProtocol::ClientToMassgateGetBannersBySupplierId::ToStream(MN_WriteMessage& aWm)
{
	aWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_REQ_GET_BANNERS_BY_SUPPLIER_ID); 
	aWm.WriteUInt(mySupplierId);
	aWm.WriteUInt(myReservedDummy); 	
}

bool 
MMG_BannerProtocol::ClientToMassgateGetBannersBySupplierId::FromStream(MN_ReadMessage& aRm)
{
	bool good = true; 

	good = good && aRm.ReadUInt(mySupplierId);
	good = good && aRm.ReadUInt(myReservedDummy); 

	return good; 
}

bool 
MMG_BannerProtocol::ClientToMassgateGetBannersBySupplierId::IsSane()
{
	return true; 
} 

//////////////////////////////////////////////////////////////////////////

MMG_BannerProtocol::MassgateToClientGetBannersBySupplierId::MassgateToClientGetBannersBySupplierId()
: mySupplierId(0)
, myReservedDummy(0) 
{	
	myBannerInfo.Init(1, 1); 
}

void 
MMG_BannerProtocol::MassgateToClientGetBannersBySupplierId::ToStream(MN_WriteMessage& aWm)
{
	aWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RSP_GET_BANNERS_BY_SUPPLIER_ID); 

	aWm.WriteUInt((unsigned int)myBannerInfo.Count()); 
	for(int i = 0; i < myBannerInfo.Count(); i++)
	{
		aWm.WriteString(myBannerInfo[i].myURL.GetBuffer());
		aWm.WriteUInt64(myBannerInfo[i].myHash); 
		aWm.WriteUChar(myBannerInfo[i].myType); 
	}
	aWm.WriteUInt(mySupplierId); 
	aWm.WriteUInt(myReservedDummy); 
}

bool 
MMG_BannerProtocol::MassgateToClientGetBannersBySupplierId::FromStream(MN_ReadMessage& aRm)
{
	bool good = true; 
	unsigned int count; 

	good = good && aRm.ReadUInt(count); 
	for(unsigned int i = 0; i < count && good; i++)
	{
		BannerInfo bannerInfo; 
		good = good && aRm.ReadString(bannerInfo.myURL.GetBuffer(), bannerInfo.myURL.GetBufferSize());
		unsigned __int64 bannerHash; 
		good = good && aRm.ReadUInt64(bannerHash);
		unsigned char bannerType; 
		good = good && aRm.ReadUChar(bannerType); 
		if(good)
		{
			bannerInfo.myHash = bannerHash; 
			bannerInfo.myType = bannerType; 
			myBannerInfo.Add(bannerInfo); 
		}
	}
	good = good && aRm.ReadUInt(mySupplierId); 
	good = good && aRm.ReadUInt(myReservedDummy);

	return good; 
}

void 
MMG_BannerProtocol::MassgateToClientGetBannersBySupplierId::AddBanner(const char* anURL, 
																	  const unsigned __int64 aHash, 
																	  const unsigned char aType)
{
	BannerInfo bannerInfo(anURL, aHash, aType);
	myBannerInfo.Add(bannerInfo); 
}

bool 
MMG_BannerProtocol::MassgateToClientGetBannersBySupplierId::IsSane()
{
	return true; 
}

//////////////////////////////////////////////////////////////////////////

MMG_BannerProtocol::ClientToMassgateAckBanner::ClientToMassgateAckBanner()
: myBannerHash(0)
, myReserveredDummy(0)
, myType(-1)
{
}

void 
MMG_BannerProtocol::ClientToMassgateAckBanner::ToStream(MN_WriteMessage& aWm)
{
	assert(myType != 255 && "Uninitialized data"); 
	assert(myBannerHash != 0 && "Uninitialized data"); 
	aWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_REQ_ACK_BANNER); 
	aWm.WriteUInt64(myBannerHash); 
	aWm.WriteUInt(myReserveredDummy); 
	aWm.WriteUChar(myType); 
} 
	
bool 
MMG_BannerProtocol::ClientToMassgateAckBanner::FromStream(MN_ReadMessage& aRm)
{
	bool good = true; 

	good = good && aRm.ReadUInt64(myBannerHash); 
	good = good && aRm.ReadUInt(myReserveredDummy); 
	good = good && aRm.ReadUChar(myType); 

	return good; 
}

bool 
MMG_BannerProtocol::ClientToMassgateAckBanner::IsSane()
{
	return true; 
}

//////////////////////////////////////////////////////////////////////////

MMG_BannerProtocol::MassgateToClientAckBanner::MassgateToClientAckBanner()
: myBannerHash(0)
, myBannerRotationTime(0)
, myBannerIsValid(0)
, myNewBannerHash(0)
, myReservedDummy(0)
{
}

void 
MMG_BannerProtocol::MassgateToClientAckBanner::ToStream(MN_WriteMessage& aWm)
{
	aWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RSP_ACK_BANNER); 
	aWm.WriteUInt64(myBannerHash); 
	unsigned char packed = (myBannerRotationTime<<1) | myBannerIsValid;
	aWm.WriteUChar(packed); 
	if (!myBannerIsValid)
	{
		aWm.WriteUInt64(myNewBannerHash);
		aWm.WriteString(myNewBannerUrl.GetBuffer());
	}
	aWm.WriteUInt(myReservedDummy); 
}
	
bool
MMG_BannerProtocol::MassgateToClientAckBanner::FromStream(MN_ReadMessage& aRm)
{
	bool good = true; 

	good = good && aRm.ReadUInt64(myBannerHash); 
	unsigned char packed; 
	good = good && aRm.ReadUChar(packed);
	myBannerRotationTime = packed >> 1; 
	myBannerIsValid = packed & 0x01; 
	if(!myBannerIsValid)
	{
		aRm.ReadUInt64(myNewBannerHash); 
		aRm.ReadString(myNewBannerUrl.GetBuffer(), myNewBannerUrl.GetBufferSize());
	}
	good = good && aRm.ReadUInt(myReservedDummy); 

	return good; 
}
	
bool 
MMG_BannerProtocol::MassgateToClientAckBanner::IsSane()
{
	return true; 
}

//////////////////////////////////////////////////////////////////////////

MMG_BannerProtocol::ClientToMassgateGetBannerByHash::ClientToMassgateGetBannerByHash()
: myBannerHash(0)
, myReservedDummy(0)
{
}

void 
MMG_BannerProtocol::ClientToMassgateGetBannerByHash::ToStream(MN_WriteMessage& aWm)
{
	aWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_REQ_GET_BANNER_BY_HASH); 
	aWm.WriteUInt64(myBannerHash); 
	aWm.WriteUInt(myReservedDummy); 
}
	
bool 
MMG_BannerProtocol::ClientToMassgateGetBannerByHash::FromStream(MN_ReadMessage& aRm)
{
	bool good = true; 

	good = good && aRm.ReadUInt64(myBannerHash); 
	good = good && aRm.ReadUInt(myReservedDummy); 

	return good; 
}
	
bool 
MMG_BannerProtocol::ClientToMassgateGetBannerByHash::IsSane()
{
	return true; 
}

//////////////////////////////////////////////////////////////////////////

MMG_BannerProtocol::MassgateToClientGetBannerByHash::MassgateToClientGetBannerByHash()
: myBannerHash(0)
, mySupplierId(0)
, myType(0)
, myReservedDummy(0)
{
}

void 
MMG_BannerProtocol::MassgateToClientGetBannerByHash::ToStream(MN_WriteMessage& aWm)
{
	aWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RSP_GET_BANNER_BY_HASH); 
	aWm.WriteUInt64(myBannerHash); 
	aWm.WriteString(myBannerURL.GetBuffer()); 
	aWm.WriteUInt(mySupplierId); 
	aWm.WriteUChar(myType); 
	aWm.WriteUInt(myReservedDummy); 
}

bool 
MMG_BannerProtocol::MassgateToClientGetBannerByHash::FromStream(MN_ReadMessage& aRm)
{
	bool good = true;

	good = good && aRm.ReadUInt64(myBannerHash); 
	good = good && aRm.ReadString(myBannerURL.GetBuffer(), myBannerURL.GetBufferSize()); 
	good = good && aRm.ReadUInt(mySupplierId); 
	good = good && aRm.ReadUChar(myType); 
	good = good && aRm.ReadUInt(myReservedDummy); 

	return good; 
}
	
bool 
MMG_BannerProtocol::MassgateToClientGetBannerByHash::IsSane()
{
	return true; 
}
