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
#include "MMG_StatsTrackingProtocol.h"


void
MMG_StatsTrackingProtocol::HelloReq::ToStream(
	MSB_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_StatsTrackingProtocolDelimiter::MST_HELLO_REQ);
}

bool
MMG_StatsTrackingProtocol::HelloReq::FromStream(
	MSB_ReadMessage& theStream)
{
	return true; 
}

//////////////////////////////////////////////////////////////////////////


void
MMG_StatsTrackingProtocol::HelloRsp::ToStream(
	MSB_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_StatsTrackingProtocolDelimiter::MST_HELLO_RSP);
	theStream.WriteBool(reportStats); 
}

bool
MMG_StatsTrackingProtocol::HelloRsp::FromStream(
	MSB_ReadMessage& theStream)
{
	return theStream.ReadBool(reportStats); 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_StatsTrackingProtocol::GameInstalled::ToStream(
	MSB_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_StatsTrackingProtocolDelimiter::MST_GAME_INSTALLED);
	theStream.WriteUInt64(macAddress);
	theStream.WriteUInt32(cdKeySeqNum);
	theStream.WriteUInt32(cdKeyCheckSum);
	theStream.WriteUInt8(productId);
}

bool
MMG_StatsTrackingProtocol::GameInstalled::FromStream(
	MSB_ReadMessage& theStream)
{
	bool good = true; 

	good = good && theStream.ReadUInt64(macAddress);
	good = good && theStream.ReadUInt32(cdKeySeqNum);
	good = good && theStream.ReadUInt32(cdKeyCheckSum);
	good = good && theStream.ReadUInt8(productId);

	return good; 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_StatsTrackingProtocol::StartedSinglePlayerMission::ToStream(
	MSB_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_StatsTrackingProtocolDelimiter::MST_STARTED_SINGLEPLAYER_MISSION);
	theStream.WriteUInt64(macAddress);
	theStream.WriteUInt32(cdKeySeqNum);
	theStream.WriteUInt32(cdKeyCheckSum);
	theStream.WriteUInt8(singelPlayerLevel);
	theStream.WriteUInt8(productId);
}

bool 
MMG_StatsTrackingProtocol::StartedSinglePlayerMission::FromStream(
	MSB_ReadMessage& theStream)
{
	bool good = true; 

	good = good && theStream.ReadUInt64(macAddress);
	good = good && theStream.ReadUInt32(cdKeySeqNum);
	good = good && theStream.ReadUInt32(cdKeyCheckSum);
	good = good && theStream.ReadUInt8(singelPlayerLevel);
	good = good && theStream.ReadUInt8(productId);

	return good; 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_StatsTrackingProtocol::StartedMultiPlayer::ToStream(
	MSB_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_StatsTrackingProtocolDelimiter::MST_STARTED_MULTIPLAYER);
	theStream.WriteUInt64(macAddress);
	theStream.WriteUInt32(cdKeySeqNum);
	theStream.WriteUInt32(cdKeyCheckSum);
	theStream.WriteUInt8(productId);
}

bool 
MMG_StatsTrackingProtocol::StartedMultiPlayer::FromStream(
	MSB_ReadMessage& theStream)
{
	bool good = true; 

	good = good && theStream.ReadUInt64(macAddress);
	good = good && theStream.ReadUInt32(cdKeySeqNum);
	good = good && theStream.ReadUInt32(cdKeyCheckSum);
	good = good && theStream.ReadUInt8(productId);

	return good; 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_StatsTrackingProtocol::StartedLAN::ToStream(
	MSB_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_StatsTrackingProtocolDelimiter::MST_STARTED_LAN);
	theStream.WriteUInt64(macAddress);
	theStream.WriteUInt32(cdKeySeqNum);
	theStream.WriteUInt32(cdKeyCheckSum);
	theStream.WriteUInt8(productId);
}

bool 
MMG_StatsTrackingProtocol::StartedLAN::FromStream(
	MSB_ReadMessage& theStream)
{
	bool good = true; 

	good = good && theStream.ReadUInt64(macAddress);
	good = good && theStream.ReadUInt32(cdKeySeqNum);
	good = good && theStream.ReadUInt32(cdKeyCheckSum);
	good = good && theStream.ReadUInt8(productId);

	return good; 
}

//////////////////////////////////////////////////////////////////////////

void
MMG_StatsTrackingProtocol::KeyValue::ToStream(
	MSB_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_StatsTrackingProtocolDelimiter::MST_KEY_VALUE);
	theStream.WriteUInt64(macAddress);
	theStream.WriteUInt32(cdKeySeqNum);
	theStream.WriteUInt32(cdKeyCheckSum);
	theStream.WriteUInt8(productId);
	theStream.WriteUInt32(key); 
	theStream.WriteUInt32(value1); 
	theStream.WriteUInt32(value2); 
	theStream.WriteUInt32(value3); 
}

bool
MMG_StatsTrackingProtocol::KeyValue::FromStream(
	MSB_ReadMessage& theStream)
{
	bool good = true; 

	good = good && theStream.ReadUInt64(macAddress);
	good = good && theStream.ReadUInt32(cdKeySeqNum);
	good = good && theStream.ReadUInt32(cdKeyCheckSum);
	good = good && theStream.ReadUInt8(productId);
	good = good && theStream.ReadUInt32(key);
	good = good && theStream.ReadUInt32(value1);
	good = good && theStream.ReadUInt32(value2);
	good = good && theStream.ReadUInt32(value3);

	return good; 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_StatsTrackingProtocol::DSUnitUtil::ToStream(
	MSB_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_StatsTrackingProtocolDelimiter::MST_DS_UNIT_UTIL);
	theStream.WriteUInt64(mapHash);
	uint32 numItemsInThisBatch = min((uint32)unitUtils.Count(), (uint32)(theStream.GetRemainingByteCount() / 4));
	theStream.WriteUInt32(numItemsInThisBatch);

	for(int i = 0; i < unitUtils.Count(); i++)
	{
		if(theStream.GetRemainingByteCount() < 4)
		{
			theStream.WriteDelimiter(MMG_StatsTrackingProtocolDelimiter::MST_DS_UNIT_UTIL);
			theStream.WriteUInt64(mapHash);
			numItemsInThisBatch = min((uint32)(unitUtils.Count() - i), (uint32)(theStream.GetRemainingByteCount() / 4));
			theStream.WriteUInt32(numItemsInThisBatch);
		}

		theStream.WriteUInt16(unitUtils[i].unitId);
		theStream.WriteUInt16(unitUtils[i].count);
	}
}

bool 
MMG_StatsTrackingProtocol::DSUnitUtil::FromStream(
	MSB_ReadMessage& theStream)
{
	bool good = true; 
	uint32 numUnits;

	good = good && theStream.ReadUInt64(mapHash);
	good = good && theStream.ReadUInt32(numUnits);

	for(uint32 i = 0; good && i < numUnits; i++)
	{
		DSUnitUtil::Unit newUnit; 
		good = good && theStream.ReadUInt16(newUnit.unitId);
		good = good && theStream.ReadUInt16(newUnit.count);
		if(good)
			unitUtils.Add(newUnit);
	}

	return good; 
}

//////////////////////////////////////////////////////////////////////////


void 
MMG_StatsTrackingProtocol::DSTAUtil::ToStream(
	MSB_WriteMessage& theStream) const
{
	theStream.WriteDelimiter(MMG_StatsTrackingProtocolDelimiter::MST_DS_TA_UTIL);
	theStream.WriteUInt64(mapHash);
	uint32 numItemsInThisBatch = min((uint32)taUtils.Count(), (uint32)(theStream.GetRemainingByteCount() / 4));
	theStream.WriteUInt32(numItemsInThisBatch);

	for(int i = 0; i < taUtils.Count(); i++)
	{
		if(theStream.GetRemainingByteCount() < 4)
		{
			theStream.WriteDelimiter(MMG_StatsTrackingProtocolDelimiter::MST_DS_TA_UTIL);
			theStream.WriteUInt64(mapHash);
			numItemsInThisBatch = min((uint32)(taUtils.Count() - i), (uint32)(theStream.GetRemainingByteCount() / 4));
			theStream.WriteUInt32(numItemsInThisBatch);
		}

		theStream.WriteUInt16(taUtils[i].taId);
		theStream.WriteUInt16(taUtils[i].count);
	}
}

bool 
MMG_StatsTrackingProtocol::DSTAUtil::FromStream(
	MSB_ReadMessage& theStream)
{
	bool good = true; 
	uint32 numTAs;

	good = good && theStream.ReadUInt64(mapHash);
	good = good && theStream.ReadUInt32(numTAs);

	for(uint32 i = 0; good && i < numTAs; i++)
	{
		DSTAUtil::TA newTA; 
		good = good && theStream.ReadUInt16(newTA.taId);
		good = good && theStream.ReadUInt16(newTA.count);
		if(good)
			taUtils.Add(newTA);
	}

	return good; 
}

//////////////////////////////////////////////////////////////////////////

MMG_StatsTrackingProtocol::DSDeathMap::DSDeathMap()
	: mapHash(0)
{
}

MMG_StatsTrackingProtocol::DSDeathMap::DSDeathMap(
	uint64 aMapHash)
	: mapHash(aMapHash)
{
}

MMG_StatsTrackingProtocol::DSDeathMap::~DSDeathMap()
{
	for(int i = 0; i < deathsPerUnit.Count(); i++)
		delete deathsPerUnit[i];
}

void			
MMG_StatsTrackingProtocol::DSDeathMap::Add(
	uint16 aUnitId, 
	uint32 aTileId, 
	uint16 aCount)
{
	for(int i = 0; i < deathsPerUnit.Count(); i++)
	{
		if(deathsPerUnit[i]->unitId == aUnitId)
		{
			deathsPerUnit[i]->tiles.Add(DeathPerUnit::TileCount(aTileId, aCount));
			return; 
		}
	}

	DeathPerUnit* newUnitDeath = new DeathPerUnit(aUnitId);
	newUnitDeath->tiles.Add(DeathPerUnit::TileCount(aTileId, aCount));
	deathsPerUnit.Add(newUnitDeath);
}

void
MMG_StatsTrackingProtocol::DSDeathMap::RemoveAll()
{
	for(int i = 0; i < deathsPerUnit.Count(); i++)
		delete deathsPerUnit[i];
	deathsPerUnit.RemoveAll();
}

void
MMG_StatsTrackingProtocol::DSDeathMap::ToStream(
	MSB_WriteMessage& theStream) const
{
	for(int i = 0; i < deathsPerUnit.Count(); i++)
	{
		theStream.WriteDelimiter(MMG_StatsTrackingProtocolDelimiter::MST_DS_DEATH_MAP);
		theStream.WriteUInt64(mapHash);

		DSDeathMap::DeathPerUnit* unitDeaths = deathsPerUnit[i];
		theStream.WriteUInt16(unitDeaths->unitId);

		uint32 numDeathsInThisBatch = min((uint32)unitDeaths->tiles.Count(), (theStream.GetRemainingByteCount() - 14) / 6); 
		theStream.WriteUInt32(numDeathsInThisBatch);

		for(int j = 0; j < unitDeaths->tiles.Count(); j++)
		{
			if(theStream.GetRemainingByteCount() < 6)
			{
				theStream.WriteDelimiter(MMG_StatsTrackingProtocolDelimiter::MST_DS_DEATH_MAP);
				theStream.WriteUInt64(mapHash);
				theStream.WriteUInt16(unitDeaths->unitId);
				numDeathsInThisBatch = min((uint32)(unitDeaths->tiles.Count() - j), (theStream.GetRemainingByteCount() - 14) / 6); 
				theStream.WriteUInt32(numDeathsInThisBatch);
			}

			theStream.WriteUInt32(unitDeaths->tiles[j].tileId);
			theStream.WriteUInt16(unitDeaths->tiles[j].count);
		}
	}
}

bool
MMG_StatsTrackingProtocol::DSDeathMap::FromStream(
	MSB_ReadMessage& theStream)
{
	bool good = true; 
	uint16 unitId; 
	uint32 numDeaths; 

	good = good && theStream.ReadUInt64(mapHash);
	good = good && theStream.ReadUInt16(unitId); 
	good = good && theStream.ReadUInt32(numDeaths);

	for(uint32 i = 0; good && i < numDeaths; i++)
	{
		uint32 tileId; 
		uint16 count; 
	
		good = good && theStream.ReadUInt32(tileId);
		good = good && theStream.ReadUInt16(count);

		if(good)
			Add(unitId, tileId, count); 
	}

	return good; 
}

//////////////////////////////////////////////////////////////////////////

void
MMG_StatsTrackingProtocol::GetKeyValidationChallengeReq::ToStream(
	MSB_WriteMessage&		aWriteMessage) const
{
	aWriteMessage.WriteDelimiter(MST_GET_CDKEY_VALIDATION_COOKIE_REQ);
}

bool
MMG_StatsTrackingProtocol::GetKeyValidationChallengeReq::FromStream(
	MSB_ReadMessage&		aReadMessage)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////

void
MMG_StatsTrackingProtocol::GetKeyValidationChallengeRsp::ToStream(
	MSB_WriteMessage&		aWriteMessage) const
{
	aWriteMessage.WriteDelimiter(MST_GET_CDKEY_VALIDATION_COOKIE_RSP);
	aWriteMessage.WriteUInt64(myCookie);
}

bool
MMG_StatsTrackingProtocol::GetKeyValidationChallengeRsp::FromStream(
	MSB_ReadMessage&		aReadMessage)
{
	return aReadMessage.ReadUInt64(myCookie);
}

//////////////////////////////////////////////////////////////////////////

void
MMG_StatsTrackingProtocol::ValidateCdKeyReq::ToStream(
	MSB_WriteMessage&		aWriteMessage) const
{
	aWriteMessage.WriteDelimiter(MST_VALIDATE_CDKEY_REQ);
	aWriteMessage.WriteUInt32(myCdKeySeqNum);
	aWriteMessage.WriteRawData(myEncryptedData, sizeof(myEncryptedData));
}

bool
MMG_StatsTrackingProtocol::ValidateCdKeyReq::FromStream(
	MSB_ReadMessage&		aReadMessage)
{
	bool			good = aReadMessage.ReadUInt32(myCdKeySeqNum);
	uint16			dsize = sizeof(myEncryptedData);
	good = good && aReadMessage.ReadRawData(myEncryptedData, dsize);

	return good && dsize == 16;
}

//////////////////////////////////////////////////////////////////////////

void
MMG_StatsTrackingProtocol::ValidateCdKeyRsp::ToStream(
	MSB_WriteMessage&		aWriteMessage) const
{
	aWriteMessage.WriteDelimiter(MST_VALIDATE_CDKEY_RSP);
	aWriteMessage.WriteBool(myIsValidFlag);
}

bool
MMG_StatsTrackingProtocol::ValidateCdKeyRsp::FromStream(
	MSB_ReadMessage&		aReadMessage)
{
	bool		good = aReadMessage.ReadBool(myIsValidFlag);

	return good;
}

//////////////////////////////////////////////////////////////////////////

void
MMG_StatsTrackingProtocol::PingReq::ToStream(
	MSB_WriteMessage&		aWriteMessage) const
{
	aWriteMessage.WriteDelimiter(MST_PING_REQ);
	aWriteMessage.WriteUInt32(myGameVersion);
	aWriteMessage.WriteUInt32(myProdId);
	aWriteMessage.WriteString(myLangCode.GetBuffer());
}

bool
MMG_StatsTrackingProtocol::PingReq::FromStream(
	MSB_ReadMessage&		aReadMessage)
{
	bool			good = true;

	good = aReadMessage.ReadUInt32(myGameVersion);
	good = good && aReadMessage.ReadUInt32(myProdId);
	good = good && aReadMessage.ReadString(myLangCode.GetBuffer(), myLangCode.GetBufferSize());

	return good;
}