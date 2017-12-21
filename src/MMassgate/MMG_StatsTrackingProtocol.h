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
#ifndef MMG_STATSTRACKING_PROTOCOL_H
#define MMG_STATSTRACKING_PROTOCOL_H

#include "MC_HybridArray.h"

#include "MSB_WriteMessage.h"
#include "MSB_ReadMessage.h"

#define MST_SERVER_HOSTNAME		"stats.massgate.net"
#define MST_SERVER_PORT			3006

typedef enum 
{
	MST_HELLO_REQ,
	MST_HELLO_RSP,
	MST_GAME_INSTALLED, 
	MST_STARTED_SINGLEPLAYER_MISSION,
	MST_STARTED_MULTIPLAYER, 
	MST_STARTED_LAN, 
	MST_KEY_VALUE,
	MST_DS_UNIT_UTIL,
	MST_DS_TA_UTIL, 
	MST_DS_DEATH_MAP,
	MST_GET_CDKEY_VALIDATION_COOKIE_REQ,
	MST_GET_CDKEY_VALIDATION_COOKIE_RSP,
	MST_VALIDATE_CDKEY_REQ,
	MST_VALIDATE_CDKEY_RSP,
	MST_PING_REQ,
} MMG_StatsTrackingProtocolDelimiter;

class MMG_StatsTrackingProtocol
{
public:
	class HelloReq
	{
	public: 
		void			ToStream(
							MSB_WriteMessage& theStream) const;

		bool			FromStream(
							MSB_ReadMessage& theStream);
	};

	class HelloRsp
	{
	public: 
		void			ToStream(
							MSB_WriteMessage& theStream) const;

		bool			FromStream(
							MSB_ReadMessage& theStream);

		bool			reportStats; 
	};

	class GameInstalled
	{
	public:
		void			ToStream(
							MSB_WriteMessage& theStream) const;
		
		bool			FromStream(
							MSB_ReadMessage& theStream);

		uint64			macAddress;
		uint32			cdKeySeqNum;
		uint32			cdKeyCheckSum;
		uint8			productId; 
	};

	class StartedSinglePlayerMission
	{
	public:
		void			ToStream(
							MSB_WriteMessage& theStream) const;

		bool			FromStream(
							MSB_ReadMessage& theStream);

		uint64			macAddress;
		uint32			cdKeySeqNum;
		uint32			cdKeyCheckSum;
		uint8			productId; 
		uint8			singelPlayerLevel;
	}; 

	class StartedMultiPlayer
	{
	public:
		void			ToStream(
							MSB_WriteMessage& theStream) const;
		
		bool			FromStream(
							MSB_ReadMessage& theStream);

		uint64			macAddress;
		uint32			cdKeySeqNum;
		uint32			cdKeyCheckSum;	
		uint8			productId; 
	};

	class StartedLAN
	{
	public:
		void			ToStream(
							MSB_WriteMessage& theStream) const;
		
		bool			FromStream(
							MSB_ReadMessage& theStream);

		uint64			macAddress;
		uint32			cdKeySeqNum;
		uint32			cdKeyCheckSum;	
		uint8			productId; 
	};

	class KeyValue
	{
	public: 
		void			ToStream(
							MSB_WriteMessage& theStream) const;

		bool			FromStream(
							MSB_ReadMessage& theStream);

		uint64			macAddress;
		uint32			cdKeySeqNum;
		uint32			cdKeyCheckSum;	
		uint8			productId; 
		uint32			key; 
		uint32			value1; 
		uint32			value2; 
		uint32			value3; 
	};

	class DSUnitUtil
	{
	public:
		DSUnitUtil()
		{
		}

		DSUnitUtil(
			uint64 aMapHash)
			: mapHash(aMapHash)
		{
		}

		void			ToStream(
							MSB_WriteMessage& theStream) const;
		
		bool			FromStream(
							MSB_ReadMessage& theStream);

		class Unit
		{
		public:
			Unit()
				: unitId(0)
				, count(0)
			{
			}

			Unit(
				uint16 aUnitId, 
				uint16 aCount)
				: unitId(aUnitId)
				, count(aCount)
			{
			}

			uint16 unitId;
			uint16 count; 
		};

		void 
		Add(
			uint16 aUnitId, 
			uint16 aCount)
		{
			unitUtils.Add(Unit(aUnitId, aCount));
		}

		void RemoveAll()
		{
			unitUtils.RemoveAll();
		}

		uint64 mapHash;
		MC_HybridArray<Unit, 256> unitUtils;
	};

	class DSTAUtil 
	{
	public:
		DSTAUtil()
		{
		}

		DSTAUtil(
			uint64 aMapHash)
			: mapHash(aMapHash)
		{
		}

		void			ToStream(
							MSB_WriteMessage& theStream) const;
		
		bool			FromStream(
							MSB_ReadMessage& theStream);

		class TA 
		{
		public:
			TA()
				: taId(0)
				, count(0)
			{
			}

			TA(
				uint16 aTaId, 
				uint16 aCount)
				: taId(aTaId)
				, count(aCount)
			{
			}

			uint16 taId;
			uint16 count; 
		};

		void 
		Add(
			uint16 aTaId, 
			uint16 aCount)
		{
			taUtils.Add(TA(aTaId, aCount));
		}

		void RemoveAll()
		{
			taUtils.RemoveAll();
		}

		uint64 mapHash;
		MC_HybridArray<TA, 256> taUtils;
	};

	class DSDeathMap
	{
	public: 
						DSDeathMap();

						DSDeathMap(
							uint64 aMapHash);

						~DSDeathMap();

		void			ToStream(
							MSB_WriteMessage& theStream) const;
		
		bool			FromStream(
							MSB_ReadMessage& theStream);

		void			Add(
							uint16 aUnitId, 
							uint32 aTileId, 
							uint16 aCount);

		void			RemoveAll();

		class DeathPerUnit
		{
		public:
			DeathPerUnit()
				: unitId(0)
			{
			}

			DeathPerUnit(
				uint16 aUnitId)
				: unitId(aUnitId)
			{
			}

			class TileCount 
			{
			public: 
				TileCount()
					: tileId(0)
					, count(0)
				{
				}

				TileCount(
					uint32 aTileId, 
					uint16 aCount)
					: tileId(aTileId)
					, count(aCount)
				{
				}

				uint32 tileId;
				uint16 count; 
			};

			uint16							unitId;
			MC_HybridArray<TileCount, 128>	tiles; 
		};

		uint64								mapHash;
		MC_HybridArray<DeathPerUnit*, 256>	deathsPerUnit; 
	};

	class GetKeyValidationChallengeReq 
	{
	public:
		
		void			ToStream(
							MSB_WriteMessage&		aWriteMessage) const;
		bool			FromStream(
							MSB_ReadMessage&		aReadMessage);
	};

	class GetKeyValidationChallengeRsp
	{
	public:

		void			ToStream(
							MSB_WriteMessage&		aWriteMessage) const;
		bool			FromStream(
							MSB_ReadMessage&		aReadMessage);

		uint64			myCookie;
	};
	
	class ValidateCdKeyReq
	{
	public:
		void			ToStream(
							MSB_WriteMessage&		aWriteMessage) const;
		bool			FromStream(
							MSB_ReadMessage&		aReadMessage);

		uint32				myCdKeySeqNum;
		uint8				myEncryptedData[16];	// 16 bytes is one AES-block
	};

	class ValidateCdKeyRsp 
	{
	public:
		void					ToStream(
									MSB_WriteMessage&		aWriteMessage) const;
		bool					FromStream(
									MSB_ReadMessage&		aReadMessage);

		bool					myIsValidFlag;
	};

	class PingReq
	{
	public:
		void					ToStream(
									MSB_WriteMessage&		aWriteMessage) const;
		bool					FromStream(
									MSB_ReadMessage&		aReadMessage);

		uint32					myGameVersion;
		MC_StaticString<3>		myLangCode;
		uint32					myProdId;
	};
	
};

#endif 