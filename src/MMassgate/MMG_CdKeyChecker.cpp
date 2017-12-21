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

#include "MS_Aes.h"

#include "MMG_CdKeyValidator.h"
#include "MMG_CdKeyChecker.h"
#include "MMG_StatsTrackingProtocol.h"

#include "MSB_ReadMessage.h"
#include "MSB_TCPConnection.h"
#include "MSB_WriteMessage.h"

bool MMG_CdKeyChecker::ValidateProductId(MMG_CdKey::Validator& aCdKey, bool aHasSovietAssault)
{
	if (!aCdKey.IsKeyValid())
		return false;

	if(aCdKey.GetProductIdentifier() == PRODUCT_ID_WIC07_STANDARD_KEY || 
		aCdKey.GetProductIdentifier() == PRODUCT_ID_WIC07_TIMELIMITED_KEY)
	{
		if(!aHasSovietAssault)
			return true; 
	}

	if(aCdKey.GetProductIdentifier() == PRODUCT_ID_WIC08_STANDARD_KEY || 
		aCdKey.GetProductIdentifier() == PRODUCT_ID_WIC08_TIMELIMITED_KEY)
	{
		if(aHasSovietAssault)
			return true; 
	}

	return false;
}

MMG_CdKeyChecker::MMG_CdKeyChecker(
	const char*					aCdKey,
	bool						aHasSovietAssault)
	: myCdKeySeqNum()
	, myIsValid(true)
	, myIsComplete(false)
	, myCookie(0)
	, myIsBrokenConnection(false)
	, myTcpConnection(NULL)
{
	MMG_CdKey::Validator	key;
	key.SetKey(aCdKey);

	if(key.IsKeyValid() && ValidateProductId(key, aHasSovietAssault))
	{
		MMG_CdKey::Validator::EncryptionKey		encKey;
		key.GetEncryptionKey(encKey);

		uint32					cdKeyLength = encKey.GetLength();
		memcpy(myEncryptionKey, encKey.GetBuffer(), cdKeyLength);

		// Pad key to make it 128 bits
		for(uint32 i = 0; i + cdKeyLength < 16; i++)
			myEncryptionKey[cdKeyLength + i] = encKey[i];
		myEncryptionKey[16] = 0;

		myCdKeySeqNum = key.GetSequenceNumber();

		myTcpConnection = new MSB_TCPConnection(MST_SERVER_HOSTNAME, MST_SERVER_PORT);

		MSB_WriteMessage		wmsg;
		MMG_StatsTrackingProtocol::GetKeyValidationChallengeReq	req;
		req.ToStream(wmsg);

		myTcpConnection->Send(wmsg);
	}
	else
	{
		myIsValid = false;
		myIsComplete = true;
	}
}

MMG_CdKeyChecker::~MMG_CdKeyChecker()
{
	if(myTcpConnection)
		myTcpConnection->Delete();
}

bool
MMG_CdKeyChecker::IsCheckComplete()
{
	if(myIsBrokenConnection || myIsComplete)
		return true;

	// If we're not complete there must be a packet for us, at some time
	MSB_ReadMessage		rmsg;
	while(myTcpConnection->GetNext(rmsg))
	{
		if(myCookie == 0)
			PrivHandleChallengeRsp(rmsg);
		else
			PrivHandleValidationRsp(rmsg);
	}

	if(!myIsComplete && myTcpConnection->IsClosed())
		myIsBrokenConnection = true;

	return myIsComplete;
}

bool
MMG_CdKeyChecker::IsKeyValid()
{
	return myIsValid;
}

void
MMG_CdKeyChecker::PrivHandleChallengeRsp(
	MSB_ReadMessage&	aReadMessage)
{
	MMG_StatsTrackingProtocol::GetKeyValidationChallengeRsp	rsp;
	if(!rsp.FromStream(aReadMessage))
	{
		MC_ERROR("Got invalid response to cdkey-challenge");
		myIsBrokenConnection = true;
		return;
	}

	myCookie = rsp.myCookie;

	uint8		rawData[16];
	memcpy(rawData, &myCookie, sizeof(myCookie));

	MMG_StatsTrackingProtocol::ValidateCdKeyReq		req;
	AES			aes;
	aes.SetParameters(128);
	aes.StartEncryption((unsigned char*)myEncryptionKey);
	aes.Encrypt(rawData, req.myEncryptedData, 1);

	req.myCdKeySeqNum = myCdKeySeqNum;

	MSB_WriteMessage			wmsg;
	req.ToStream(wmsg);
	myTcpConnection->Send(wmsg);
}

void
MMG_CdKeyChecker::PrivHandleValidationRsp(
	MSB_ReadMessage&	aReadMessage)
{
	MMG_StatsTrackingProtocol::ValidateCdKeyRsp	rsp;
	if(!rsp.FromStream(aReadMessage))
	{
		MC_ERROR("Got invalid response to validation request");
		myIsBrokenConnection = true;
		return;
	}
	
	myIsComplete = true;
	myIsValid = rsp.myIsValidFlag;
}
