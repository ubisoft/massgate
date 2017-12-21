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
#include "stdafx.h"
#include "MMG_NATNegotiationProtocol.h"
#include "MMG_Messaging.h"
#include "MMG_ProtocolDelimiters.h"

// MEDIATING 

//////////////////////////////////////////////////////////////////////////

MMG_NATNegotiationProtocol::ClientToMediatingRequestConnect::ClientToMediatingRequestConnect()
: myRequestId(0)
, myPassiveProfileId(0)
, myReservedDummy(0)
, myUseRelayingServer(2)
{
}

void
MMG_NATNegotiationProtocol::ClientToMediatingRequestConnect::ToStream(MN_WriteMessage& aWm)
{
	assert(myRequestId != 0 && "myRequestId not valid"); 
	assert(myPassiveProfileId != 0 && "myPassiveProfileId not valid"); 
	assert(myUseRelayingServer >= 0 && myUseRelayingServer <= 1 && "myUseRelayingServer not valid"); 

	aWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_NATNEG_CLIENT_TO_MEDIATING_REQUEST_CONNECT); 
	aWm.WriteUInt(myRequestId); 
	aWm.WriteUInt(myPassiveProfileId); 
	aWm.WriteUInt(myPrivateEndpoint.myAddr);
	aWm.WriteUShort(myPrivateEndpoint.myPort); 
	aWm.WriteUInt(myPublicEndpoint.myAddr);
	aWm.WriteUShort(myPublicEndpoint.myPort); 
	aWm.WriteUChar(myUseRelayingServer); 
	aWm.WriteUInt(myReservedDummy); 
}

bool
MMG_NATNegotiationProtocol::ClientToMediatingRequestConnect::FromStream(MN_ReadMessage& aRm)
{
	bool good = true; 

	good = good && aRm.ReadUInt(myRequestId); 
	good = good && aRm.ReadUInt(myPassiveProfileId); 
	good = good && aRm.ReadUInt(myPrivateEndpoint.myAddr);
	good = good && aRm.ReadUShort(myPrivateEndpoint.myPort);
	good = good && aRm.ReadUInt(myPublicEndpoint.myAddr);
	good = good && aRm.ReadUShort(myPublicEndpoint.myPort);
	good = good && aRm.ReadUChar(myUseRelayingServer); 
	good = good && aRm.ReadUInt(myReservedDummy); 

	return good; 
}

bool
MMG_NATNegotiationProtocol::ClientToMediatingRequestConnect::IsSane()
{
	bool sane = true; 

	sane = sane && (myRequestId != 0); 
	sane = sane && (myPassiveProfileId != 0); 
	sane = sane && (myUseRelayingServer >= 0 && myUseRelayingServer <= 1); 

	return sane; 
}

//////////////////////////////////////////////////////////////////////////

MMG_NATNegotiationProtocol::MediatingToClientRequestConnect::MediatingToClientRequestConnect()
: myRequestId(0)
, myActiveProfileId(0)
, mySessionCookie(0)
, myReservedDummy(0)
, myUseRelayingServer(2)
{
}


void 
MMG_NATNegotiationProtocol::MediatingToClientRequestConnect::ToStream(MN_WriteMessage& aWm)
{
	assert(myRequestId != 0 && "myRequestId not valid"); 
	assert(myActiveProfileId != 0 && "myActiveProfileId not valid"); 
	assert(mySessionCookie != 0 && "mySessionCookie not valid"); 
	assert(myUseRelayingServer >= 0 && myUseRelayingServer <= 1 && "myUseRelayingServer not valid"); 

	aWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_NATNEG_MEDIATING_TO_CLIENT_REQUEST_CONNECT); 
	aWm.WriteUInt(myRequestId);
	aWm.WriteUInt(myActiveProfileId);
	aWm.WriteUInt(mySessionCookie);
	aWm.WriteUInt(myActivePeersPrivateEndpoint.myAddr);
	aWm.WriteUShort(myActivePeersPrivateEndpoint.myPort);
	aWm.WriteUInt(myActivePeersPublicEndpoint.myAddr);
	aWm.WriteUShort(myActivePeersPublicEndpoint.myPort);
	aWm.WriteUChar(myUseRelayingServer); 
	aWm.WriteUInt(myReservedDummy); 
}

bool 
MMG_NATNegotiationProtocol::MediatingToClientRequestConnect::FromStream(MN_ReadMessage& aRm)
{
	bool good = true; 

	good = good && aRm.ReadUInt(myRequestId); 
	good = good && aRm.ReadUInt(myActiveProfileId); 
	good = good && aRm.ReadUInt(mySessionCookie);	
	good = good && aRm.ReadUInt(myActivePeersPrivateEndpoint.myAddr);
	good = good && aRm.ReadUShort(myActivePeersPrivateEndpoint.myPort);
	good = good && aRm.ReadUInt(myActivePeersPublicEndpoint.myAddr); 
	good = good && aRm.ReadUShort(myActivePeersPublicEndpoint.myPort);
	good = good && aRm.ReadUChar(myUseRelayingServer); 
	good = good && aRm.ReadUInt(myReservedDummy); 

	return good; 
}

bool 
MMG_NATNegotiationProtocol::MediatingToClientRequestConnect::IsSane()
{
	bool sane = true; 

	sane = sane && (myRequestId != 0); 
	sane = sane && (myActiveProfileId != 0); 
	sane = sane && (mySessionCookie != 0); 
	sane = sane && (myUseRelayingServer >= 0 && myUseRelayingServer <= 1); 

	return sane; 
}

//////////////////////////////////////////////////////////////////////////

MMG_NATNegotiationProtocol::ClientToMediatingResponseConnect::ClientToMediatingResponseConnect()
: myRequestId(0)
, myActiveProfileId(0)
, myAcceptRequest(2)
, mySessionCookie(0)
, myReservedDummy(0)
, myUseRelayingServer(2)
{
}

void
MMG_NATNegotiationProtocol::ClientToMediatingResponseConnect::ToStream(MN_WriteMessage& aWm)
{
	assert(myRequestId != 0 && "myRequestId not valid"); 
	assert(myActiveProfileId != 0 && "myActiveProfileId not valid"); 
	assert(myAcceptRequest >= 0 && myAcceptRequest <= 1 && "myAcceptRequest not valid");

	aWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_NATNEG_CLIENT_TO_MEDIATING_RESPONSE_CONNECT); 
	aWm.WriteUInt(myRequestId);
	aWm.WriteUInt(myActiveProfileId); 
	aWm.WriteUChar(myAcceptRequest); 
	if(myAcceptRequest == 1)
	{
		assert(mySessionCookie != 0 && "mySessionCookie not valid"); 
		assert(myUseRelayingServer >= 0 && myUseRelayingServer <= 1 && "myUseRelayingServer not valid"); 

		aWm.WriteUInt(mySessionCookie); 
		aWm.WriteUInt(myPrivateEndpoint.myAddr); 
		aWm.WriteUShort(myPrivateEndpoint.myPort); 
		aWm.WriteUInt(myPublicEndpoint.myAddr); 
		aWm.WriteUShort(myPublicEndpoint.myPort); 
		aWm.WriteUChar(myUseRelayingServer); 
	}
	aWm.WriteUInt(myReservedDummy); 
}

bool 
MMG_NATNegotiationProtocol::ClientToMediatingResponseConnect::FromStream(MN_ReadMessage& aRm)
{
	bool good = true; 

	good = good && aRm.ReadUInt(myRequestId); 
	good = good && aRm.ReadUInt(myActiveProfileId); 
	good = good && aRm.ReadUChar(myAcceptRequest); 
	if(good == false)
	{
		MC_DEBUG("Protocol error"); 
		return false; 
	}
	if(myAcceptRequest == 1)
	{
		good = good && aRm.ReadUInt(mySessionCookie);
		good = good && aRm.ReadUInt(myPrivateEndpoint.myAddr);
		good = good && aRm.ReadUShort(myPrivateEndpoint.myPort);
		good = good && aRm.ReadUInt(myPublicEndpoint.myAddr);
		good = good && aRm.ReadUShort(myPublicEndpoint.myPort);
		good = good && aRm.ReadUChar(myUseRelayingServer); 
	}
	good = good && aRm.ReadUInt(myReservedDummy); 

	return good; 
}

bool 
MMG_NATNegotiationProtocol::ClientToMediatingResponseConnect::IsSane()
{
	bool sane = true; 

	sane = sane && (myRequestId != 0); 
	sane = sane && (myActiveProfileId != 0); 
	sane = sane && (myAcceptRequest >= 0 && myAcceptRequest <= 1);
	if(myAcceptRequest == 1)
	{
		sane = sane && (mySessionCookie != 0); 	
		sane = sane && (myUseRelayingServer >= 0 && myUseRelayingServer <= 1); 
	}
	
	return sane; 
}

//////////////////////////////////////////////////////////////////////////

MMG_NATNegotiationProtocol::MediatingToClientResponseConnect::MediatingToClientResponseConnect()
: myRequestId(0)
, myPassiveProfileId(0)
, myAcceptRequest(2)
, mySessionCookie(0)
, myReservedDummy(0)
, myUseRelayingServer(2)
{
}

void
MMG_NATNegotiationProtocol::MediatingToClientResponseConnect::ToStream(MN_WriteMessage& aWm)
{
	assert(myRequestId != 0 && "myRequestId not valid"); 
	assert(myPassiveProfileId != 0 && "myPassiveProfileId not valid"); 
	assert(myAcceptRequest >= 0 && myAcceptRequest <= 1 && "myAcceptRequest not valid"); 	

	aWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_NATNEG_MEDIATING_TO_CLIENT_RESPONSE_CONNECT); 
	aWm.WriteUInt(myRequestId); 
	aWm.WriteUInt(myPassiveProfileId); 
	aWm.WriteUChar(myAcceptRequest); 
	if(myAcceptRequest == 1)
	{
		assert(mySessionCookie != 0 && "mySessionCooki not valid"); 
		assert(myUseRelayingServer >= 0 && myUseRelayingServer <= 1 && "myUseRelayingServer not valid"); 

		aWm.WriteUInt(mySessionCookie); 
		aWm.WriteUInt(myPassivePeersPrivateEndpoint.myAddr); 
		aWm.WriteUShort(myPassivePeersPrivateEndpoint.myPort); 
		aWm.WriteUInt(myPassivePeersPublicEndpoint.myAddr); 
		aWm.WriteUShort(myPassivePeersPublicEndpoint.myPort); 
		aWm.WriteUChar(myUseRelayingServer); 
	}
	aWm.WriteUInt(myReservedDummy); 
}

bool 
MMG_NATNegotiationProtocol::MediatingToClientResponseConnect::FromStream(MN_ReadMessage& aRm)
{
	bool good = true; 

	good = good && aRm.ReadUInt(myRequestId); 
	good = good && aRm.ReadUInt(myPassiveProfileId); 
	good = good && aRm.ReadUChar(myAcceptRequest);
	if(good == false)
	{
		MC_DEBUG("Protocol error");
		return false; 
	}
	if(myAcceptRequest == 1)
	{
		good = good && aRm.ReadUInt(mySessionCookie); 
		good = good && aRm.ReadUInt(myPassivePeersPrivateEndpoint.myAddr);
		good = good && aRm.ReadUShort(myPassivePeersPrivateEndpoint.myPort); 
		good = good && aRm.ReadUInt(myPassivePeersPublicEndpoint.myAddr); 
		good = good && aRm.ReadUShort(myPassivePeersPublicEndpoint.myPort); 
		good = good && aRm.ReadUChar(myUseRelayingServer); 
	}
	good = good && aRm.ReadUInt(myReservedDummy); 

	return good; 
}

bool 
MMG_NATNegotiationProtocol::MediatingToClientResponseConnect::IsSane()
{
	bool sane = true; 

	sane = sane && (myRequestId != 0); 
	sane = sane && (myPassiveProfileId != 0);
	sane = sane && (myAcceptRequest >= 0 && myAcceptRequest <= 1);
	if(myAcceptRequest == 1)
	{
		sane = sane && (mySessionCookie != 0); 	
		sane = sane && (myUseRelayingServer >= 0 && myUseRelayingServer <= 1); 
	}

	return sane; 
}

// RELAYING 

MMG_NATNegotiationProtocol::ClientToRelayingRequestRegister::ClientToRelayingRequestRegister()
: myRequestId(0)
, myReservedDummy(0)
{
}

void 
MMG_NATNegotiationProtocol::ClientToRelayingRequestRegister::ToStream(MN_WriteMessage& aWm)
{
	assert(myRequestId != 0 && "myRequestId not valid"); 

	aWm.WriteDelimiter(MMG_ProtocolDelimiters::NATNEG_CLIENT_TO_RELAYING_REQUEST_REGISTER); 	
	aWm.WriteUInt(myRequestId); 
	myAuthToken.ToStream(aWm); 
	aWm.WriteUInt(myReservedDummy); 
}

bool 
MMG_NATNegotiationProtocol::ClientToRelayingRequestRegister::FromStream(MN_ReadMessage& aRm)
{
	bool good = true; 

	good = good && aRm.ReadUInt(myRequestId);
	good = good && myAuthToken.FromStream(aRm); 
	good = good && aRm.ReadUInt(myReservedDummy);

	return good; 
}

bool 
MMG_NATNegotiationProtocol::ClientToRelayingRequestRegister::IsSane()
{
	return myRequestId != 0; 
}

//////////////////////////////////////////////////////////////////////////

MMG_NATNegotiationProtocol::RelayingToClientResponseRegister::RelayingToClientResponseRegister()
: myRequestId(0)
, myReservedDummy(0)
{
}

void 
MMG_NATNegotiationProtocol::RelayingToClientResponseRegister::ToStream(MN_WriteMessage& aWm)
{
	assert(myRequestId != 0 && "myRequestId not valid"); 

	aWm.WriteDelimiter(MMG_ProtocolDelimiters::NATNEG_RELAYING_TO_SERVER_RESPONSE_REGISTER);
	aWm.WriteUInt(myRequestId); 
	aWm.WriteUInt(myPublicEndpoint.myAddr); 
	aWm.WriteUShort(myPublicEndpoint.myPort); 
	aWm.WriteUInt(myReservedDummy); 
}

bool 
MMG_NATNegotiationProtocol::RelayingToClientResponseRegister::FromStream(MN_ReadMessage& aRm)
{
	bool good = true; 

	good = good && aRm.ReadUInt(myRequestId); 
	good = good && aRm.ReadUInt(myPublicEndpoint.myAddr); 
	good = good && aRm.ReadUShort(myPublicEndpoint.myPort); 
	good = good && aRm.ReadUInt(myReservedDummy); 

	return good; 
}

bool 
MMG_NATNegotiationProtocol::RelayingToClientResponseRegister::IsSane()
{
	return myRequestId != 0; 
}

//////////////////////////////////////////////////////////////////////////

MMG_NATNegotiationProtocol::ClientToRelayingRequestConnectToPeer::ClientToRelayingRequestConnectToPeer()
: myRequestId(0)
, mySessionCookie(0)
, myProfileId(0)
, myPeersProfileId(0)
, myReservedDummy(0)
{
}

void 
MMG_NATNegotiationProtocol::ClientToRelayingRequestConnectToPeer::ToStream(MN_WriteMessage& aWm)
{
	assert(myRequestId != 0 && "myRequestId not valid"); 
	assert(mySessionCookie != 0 && "mySessionCookie not valid"); 
	assert(myProfileId != 0 && "myProfileId not valid"); 
	assert(myPeersProfileId != 0 && "myPeersProfileId not valid"); 

	aWm.WriteDelimiter(MMG_ProtocolDelimiters::NATNEG_CLIENT_TO_RELAYING_REQUEST_CONNECT_TO_PEER); 
	aWm.WriteUInt(myRequestId); 
	aWm.WriteUInt(mySessionCookie); 
	aWm.WriteUInt(myProfileId); 
	aWm.WriteUInt(myPeersProfileId); 
	aWm.WriteUInt(myReservedDummy); 
}

bool 
MMG_NATNegotiationProtocol::ClientToRelayingRequestConnectToPeer::FromStream(MN_ReadMessage& aRm)
{
	bool good = true; 

	good = good && aRm.ReadUInt(myRequestId); 
	good = good && aRm.ReadUInt(mySessionCookie); 
	good = good && aRm.ReadUInt(myProfileId); 
	good = good && aRm.ReadUInt(myPeersProfileId); 
	good = good && aRm.ReadUInt(myReservedDummy); 

	return good; 
}

bool 
MMG_NATNegotiationProtocol::ClientToRelayingRequestConnectToPeer::IsSane()
{
	bool sane = true; 

	sane = sane && (myRequestId != 0); 
	sane = sane && (mySessionCookie != 0); 
	sane = sane && (myProfileId != 0); 
	sane = sane && (myPeersProfileId != 0); 

	return sane; 
}


//////////////////////////////////////////////////////////////////////////

MMG_NATNegotiationProtocol::RelayingToClientResponseConnectToPeer::RelayingToClientResponseConnectToPeer()
: myRequestId(0)
, myRequestOk(2)
, myReservedDummy(0)
{
}

void 
MMG_NATNegotiationProtocol::RelayingToClientResponseConnectToPeer::ToStream(MN_WriteMessage& aWm)
{
	assert(myRequestId != 0 && "myRequestId not valid"); 
	assert(myRequestOk >= 0 && myRequestOk <= 1 && "myRequestOk not valid"); 

	aWm.WriteDelimiter(MMG_ProtocolDelimiters::NATNEG_RELAYING_TO_CLIENT_RESPONSE_CONNECT_TO_PEER);
	aWm.WriteUInt(myRequestId); 
	aWm.WriteUInt(myRequestOk); 
	aWm.WriteUInt(myReservedDummy); 
}

bool 
MMG_NATNegotiationProtocol::RelayingToClientResponseConnectToPeer::FromStream(MN_ReadMessage& aRm)
{
	bool good = true; 

	good = good && aRm.ReadUInt(myRequestId); 
	good = good && aRm.ReadUInt(myRequestOk); 
	good = good && aRm.ReadUInt(myReservedDummy); 

	return good; 
}

bool 
MMG_NATNegotiationProtocol::RelayingToClientResponseConnectToPeer::IsSane()
{
	bool sane = true; 
	
	sane = sane && (myRequestId != 0);
	sane = sane && (myRequestOk >= 0 && myRequestOk <= 1);

	return sane; 
}

//////////////////////////////////////////////////////////////////////////

MMG_NATNegotiationProtocol::Cookie::Cookie()
: mySessionCookie(0)
, myReservedDummy(0)
{
}

void 
MMG_NATNegotiationProtocol::Cookie::ToStream(MN_WriteMessage& aWm)
{
	assert(mySessionCookie != 0 && "mySessionCookie"); 

	aWm.WriteDelimiter(MMG_ProtocolDelimiters::NATNEG_COOKIE); 
	aWm.WriteUInt(mySessionCookie); 
	aWm.WriteUInt(myReservedDummy); 
}

bool 
MMG_NATNegotiationProtocol::Cookie::FromStream(MN_ReadMessage& aRm)
{
	bool good = true; 

	good = good && aRm.ReadUInt(mySessionCookie); 
	good = good && aRm.ReadUInt(myReservedDummy);

	return good; 
}

bool 
MMG_NATNegotiationProtocol::Cookie::IsSane()
{
	return mySessionCookie != 0; 
}

//////////////////////////////////////////////////////////////////////////

MMG_NATNegotiationProtocol::Ping::Ping()
: myReservedDummy(0)
{
}

void 
MMG_NATNegotiationProtocol::Ping::ToStream(MN_WriteMessage& aWm)
{
	aWm.WriteDelimiter(MMG_ProtocolDelimiters::NATNEG_PING); 
	aWm.WriteUInt(myReservedDummy); 
}

bool 
MMG_NATNegotiationProtocol::Ping::FromStream(MN_ReadMessage& aRm)
{
	return aRm.ReadUInt(myReservedDummy); 
}

bool 
MMG_NATNegotiationProtocol::Ping::IsSane()
{
	return true; 
}

//////////////////////////////////////////////////////////////////////////

MMG_NATNegotiationProtocol::Pong::Pong()
: myReservedDummy(0)
{
}

void 
MMG_NATNegotiationProtocol::Pong::ToStream(MN_WriteMessage& aWm)
{
	aWm.WriteDelimiter(MMG_ProtocolDelimiters::NATNEG_PONG); 
	aWm.WriteUInt(myReservedDummy); 
}

bool 
MMG_NATNegotiationProtocol::Pong::FromStream(MN_ReadMessage& aRm)
{
	return aRm.ReadUInt(myReservedDummy); 
}

bool 
MMG_NATNegotiationProtocol::Pong::IsSane()
{
	return true; 
}

//////////////////////////////////////////////////////////////////////////

MMG_NATNegotiationProtocol::Data::Data()
: myData(NULL)
, myDataLength(0)
, myReservedDummy(0)
{
}

void 
MMG_NATNegotiationProtocol::Data::ToStream(MN_WriteMessage& aWm)
{
	assert(myData != NULL && "myData not valid"); 
	assert(myDataLength != 0 && "myDataLength not valid"); 

	aWm.WriteDelimiter(MMG_ProtocolDelimiters::NATNEG_DATA); 
	aWm.WriteRawData((const char*)myData, myDataLength);
	aWm.WriteUInt(myReservedDummy); 
}

bool 
MMG_NATNegotiationProtocol::Data::FromStream(MN_ReadMessage& aRm)
{
	bool good = true; 

	good = good && aRm.ReadRawData((unsigned char*)myData, myDataLength, &myDataLength);
	good = good && aRm.ReadUInt(myReservedDummy); 

	return good; 
}

bool 
MMG_NATNegotiationProtocol::Data::IsSane()
{
	bool sane = true; 

	sane = sane && (myData != NULL);
	sane = sane && (myDataLength != 0); 

	return sane; 
}