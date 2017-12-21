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
#include "MMG_OptionalContentProtocol.h"
#include "MMG_Messaging.h"
#include "MMG_ProtocolDelimiters.h"

void 
MMG_OptionalContentProtocol::GetReq::ToStream(
	MN_WriteMessage& aWm)
{
	aWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_OPTIONAL_CONTENT_GET_REQ); 
	aWm.WriteString(myLang.GetBuffer()); 
}

bool 
MMG_OptionalContentProtocol::GetReq::FromStream(
	MN_ReadMessage& aRm)
{
	return aRm.ReadString(myLang.GetBuffer(), myLang.GetBufferSize()); 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_OptionalContentProtocol::GetRsp::ToStream(
	MN_WriteMessage& aWm)
{
	aWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_OPTIONAL_CONTENT_GET_RSP); 
	aWm.WriteUInt(items.Count()); 
	for(int i = 0; i < items.Count(); i++)
	{
		aWm.WriteUInt64(items[i].hash);
		aWm.WriteString(items[i].name.GetBuffer()); 
		aWm.WriteString(items[i].url.GetBuffer());
		aWm.WriteUInt(items[i].id);
	}
}

bool 
MMG_OptionalContentProtocol::GetRsp::FromStream(
	MN_ReadMessage& aRm)
{
	unsigned int numItems; 
	bool good = aRm.ReadUInt(numItems);

	for(unsigned int i = 0; good && i < numItems; i++)
	{
		unsigned __int64 hash;
		MC_StaticString<256> url, name;
		unsigned int id; 
		good = good && aRm.ReadUInt64(hash);
		good = good && aRm.ReadString(name.GetBuffer(), name.GetBufferSize()); 
		good = good && aRm.ReadString(url.GetBuffer(), url.GetBufferSize()); 
		good = good && aRm.ReadUInt(id);
		if(good)
			AddItem(hash, name.GetBuffer(), url.GetBuffer(), id);
	}

	return good; 
}

void 
MMG_OptionalContentProtocol::GetRsp::AddItem(
	unsigned __int64 aHash, 
	const char* aName, 
	const char* anUrl, 
	unsigned int aId)
{
	for(int i = 0; i < items.Count(); i++)
		if(items[i].hash == aHash)
			return; 

	items.Add(Item(aHash, aName, anUrl, aId)); 
}

//////////////////////////////////////////////////////////////////////////

void 
MMG_OptionalContentProtocol::RetryItemReq::ToStream(
	MN_WriteMessage& aWm)
{
	aWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_OPTIONAL_CONTENT_RETRY_REQ);
	aWm.WriteString(myLang.GetBuffer()); 
	aWm.WriteUInt64(hash);
	aWm.WriteInt(idsToExclude.Count());
	for(int i = 0; i < idsToExclude.Count(); i++)
		aWm.WriteUInt(idsToExclude[i]);
}

bool 
MMG_OptionalContentProtocol::RetryItemReq::FromStream(
	MN_ReadMessage& aRm)
{
	bool good = true; 

	good = good && aRm.ReadString(myLang.GetBuffer(), myLang.GetBufferSize()); 
	good = good && aRm.ReadUInt64(hash);

	int numIdsToExclude;
	good = good && aRm.ReadInt(numIdsToExclude);

	for(int i = 0; good && i < numIdsToExclude; i++)
	{
		unsigned int id; 
		good = good && aRm.ReadUInt(id);
		if(good)
			idsToExclude.Add(id);
	}

	return good; 
}

//////////////////////////////////////////////////////////////////////////

MMG_OptionalContentProtocol::RetryItemRsp::RetryItemRsp()
: hash(0)
, id(0)
{
}

void 
MMG_OptionalContentProtocol::RetryItemRsp::ToStream(
	MN_WriteMessage& aWm)
{
	aWm.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_OPTIONAL_CONTENT_RETRY_RSP);
	aWm.WriteUInt64(hash);
	aWm.WriteString(name.GetBuffer()); 
	aWm.WriteString(url.GetBuffer());
	aWm.WriteUInt(id);	
}

bool 
MMG_OptionalContentProtocol::RetryItemRsp::FromStream(
	MN_ReadMessage& aRm)
{
	bool good = true; 

	good = good && aRm.ReadUInt64(hash);
	good = good && aRm.ReadString(name.GetBuffer(), name.GetBufferSize()); 
	good = good && aRm.ReadString(url.GetBuffer(), url.GetBufferSize()); 
	good = good && aRm.ReadUInt(id);

	return good; 
}
