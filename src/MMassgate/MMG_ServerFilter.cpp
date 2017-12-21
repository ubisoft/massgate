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
#include "MMG_ServerFilter.h"
#include "MMG_BitStream.h"

MMG_ServerFilter::MMG_ServerFilter()
{
}

MMG_ServerFilter& 
MMG_ServerFilter::operator=(const MMG_ServerFilter& aRhs)
{
	if (this != &aRhs)
	{
		Clear();
		if (aRhs.isDedicated.IsSet()) isDedicated = aRhs.isDedicated;
		if (aRhs.isRanked.IsSet()) isRanked = aRhs.isRanked;
		if (aRhs.isRankBalanced.IsSet()) isRankBalanced = aRhs.isRankBalanced;
		if (aRhs.hasDominationMaps.IsSet()) hasDominationMaps = aRhs.hasDominationMaps;
		if (aRhs.hasAssaultMaps.IsSet()) hasAssaultMaps = aRhs.hasAssaultMaps;
		if (aRhs.hasTowMaps.IsSet()) hasTowMaps = aRhs.hasTowMaps;
		if (aRhs.totalSlots.IsSet()) totalSlots = aRhs.totalSlots;
		if (aRhs.emptySlots.IsSet()) emptySlots = aRhs.emptySlots;
		if (aRhs.notFullEmpty.IsSet()) notFullEmpty = aRhs.notFullEmpty;
		if (aRhs.fromPlayNow.IsSet()) fromPlayNow = aRhs.fromPlayNow;
		if (aRhs.partOfMapName.IsSet()) partOfMapName = aRhs.partOfMapName;
		if (aRhs.partOfServerName.IsSet()) partOfServerName = aRhs.partOfServerName;
		if (aRhs.requiresPassword.IsSet()) requiresPassword = aRhs.requiresPassword;
		if (aRhs.noMod.IsSet()) noMod = aRhs.noMod;
		myGameVersion = aRhs.myGameVersion;
		myProtocolVersion = aRhs.myProtocolVersion;
	}
	return *this;
}

bool 
MMG_ServerFilter::operator==(const MMG_ServerFilter& aRhs)
{
	bool equal = ((myGameVersion == aRhs.myGameVersion)&&(myProtocolVersion == aRhs.myProtocolVersion));
	equal = equal && (isRanked.IsSet() && (isRanked == aRhs.isRanked));
	equal = equal && (isRankBalanced.IsSet() && (isRanked == aRhs.isRanked));
	equal = equal && (hasDominationMaps.IsSet() && (hasDominationMaps == aRhs.hasDominationMaps));
	equal = equal && (hasAssaultMaps.IsSet() && (hasAssaultMaps == aRhs.hasAssaultMaps));
	equal = equal && (hasTowMaps.IsSet() && (hasTowMaps == aRhs.hasTowMaps));
	equal = equal && (totalSlots.IsSet() && (totalSlots == aRhs.totalSlots));
	equal = equal && (emptySlots.IsSet() && (emptySlots == aRhs.emptySlots));
	equal = equal && (notFullEmpty.IsSet() && (notFullEmpty == aRhs.notFullEmpty));
	equal = equal && (fromPlayNow.IsSet() && (fromPlayNow == aRhs.fromPlayNow));
	equal = equal && (partOfMapName.IsSet() && (partOfMapName == aRhs.partOfMapName));
	equal = equal && (partOfServerName.IsSet() && (partOfServerName == aRhs.partOfServerName));
	equal = equal && (requiresPassword.IsSet() && (requiresPassword == aRhs.requiresPassword));
	equal = equal && (isDedicated.IsSet() && (isDedicated == aRhs.isDedicated));
	equal = equal && (noMod.IsSet() && (noMod == aRhs.noMod));
	return equal;
}

void
MMG_ServerFilter::Clear()
{
	isDedicated.Clear();
	isRanked.Clear();
	isRankBalanced.Clear();
	hasDominationMaps.Clear(); 
	hasAssaultMaps.Clear();
	hasTowMaps.Clear(); 
	noMod.Clear();
	totalSlots.Clear();
	emptySlots.Clear();
	notFullEmpty.Clear();
	fromPlayNow.Clear();
	partOfMapName.Clear();
	partOfServerName.Clear();
	requiresPassword.Clear();
}

void 
MMG_ServerFilter::ToStream(MN_WriteMessage& theStream) const
{
	theStream.WriteUInt(myGameVersion);
	theStream.WriteUInt(myProtocolVersion);

	// Build bitvalue of which fields are set (and sent)
	unsigned short bitfield;
	MMG_BitWriter<unsigned short> bw(&bitfield, 16);
	bw.WriteBit(isDedicated.IsSet());
	bw.WriteBit(isRanked.IsSet());
	bw.WriteBit(isRankBalanced.IsSet());
	bw.WriteBit(hasDominationMaps.IsSet());
	bw.WriteBit(hasAssaultMaps.IsSet());
	bw.WriteBit(hasTowMaps.IsSet());
	bw.WriteBit(totalSlots.IsSet());
	bw.WriteBit(emptySlots.IsSet());
	bw.WriteBit(notFullEmpty.IsSet()); 
	bw.WriteBit(fromPlayNow.IsSet()); 
	bw.WriteBit(partOfMapName.IsSet());
	bw.WriteBit(partOfServerName.IsSet());
	bw.WriteBit(requiresPassword.IsSet());
	bw.WriteBit(noMod.IsSet());

	theStream.WriteUShort(bitfield);

	if (isDedicated.IsSet()) 
		theStream.WriteUChar(isDedicated == true ? 1 : 0);
	
	if (isRanked.IsSet()) 
		theStream.WriteUChar(isRanked == true ? 1 : 0);
	
	if (isRankBalanced.IsSet())
		theStream.WriteUChar(isRankBalanced == true ? 1 : 0);

	if (hasDominationMaps.IsSet())
		theStream.WriteUChar(hasDominationMaps == true ? 1 : 0);

	if (hasAssaultMaps.IsSet())
		theStream.WriteUChar(hasAssaultMaps == true ? 1 : 0);

	if (hasTowMaps.IsSet())
		theStream.WriteUChar(hasTowMaps == true ? 1 : 0);

	if (totalSlots.IsSet()) 
		theStream.WriteChar((unsigned char)totalSlots);
	
	if (emptySlots.IsSet()) 
		theStream.WriteChar(emptySlots);
	
	if (notFullEmpty.IsSet()) 
		theStream.WriteChar(notFullEmpty);
	
	if (fromPlayNow.IsSet()) 
		theStream.WriteChar(fromPlayNow);

	if (partOfMapName.IsSet()) 
		theStream.WriteLocString(partOfMapName);
	
	if (partOfServerName.IsSet()) 
	{
		MC_LocString t = partOfServerName; 
		t.TrimLeft().TrimRight();  
		theStream.WriteLocString(t.MakeLower());
	}
	if (requiresPassword.IsSet()) 
		theStream.WriteUChar(requiresPassword == true ? 1 : 0);

	if (noMod.IsSet()) 
		theStream.WriteUChar(noMod == true ? 1 : 0);
}

bool 
MMG_ServerFilter::FromStream(MN_ReadMessage& theStream)
{
	unsigned char val;
	char sval;
	MC_LocString strval;

	Clear();

	bool status = true;

	status = status && theStream.ReadUInt(myGameVersion);
	status = status && theStream.ReadUInt(myProtocolVersion);

	unsigned short bitfield;
	status = status && theStream.ReadUShort(bitfield);
	
	MMG_BitReader<unsigned short> br(&bitfield, 16);

	if (status && br.ReadBit())
	{
		status = status && theStream.ReadUChar(val);
		isDedicated = val>0;
	}
	if (status && br.ReadBit())
	{
		status = status && theStream.ReadUChar(val);
		isRanked = val>0;
	}
	if (status && br.ReadBit())
	{
		status = status && theStream.ReadUChar(val);
		isRankBalanced = val>0;
	}
	if (status && br.ReadBit())
	{
		status = status && theStream.ReadUChar(val);
		hasDominationMaps = val>0;
	}
	if (status && br.ReadBit())
	{
		status = status && theStream.ReadUChar(val);
		hasAssaultMaps = val>0;
	}
	if (status && br.ReadBit())
	{
		status = status && theStream.ReadUChar(val);
		hasTowMaps = val>0;
	}
	if (status && br.ReadBit())
	{
		status = status && theStream.ReadChar(sval);
		totalSlots = sval;
	}
	if (status && br.ReadBit())
	{
		status = status && theStream.ReadChar(sval);
		emptySlots = sval;
	}
	if (status && br.ReadBit())
	{
		status = status && theStream.ReadChar(sval);
		notFullEmpty = sval;
	}
	if (status && br.ReadBit())
	{
		status = status && theStream.ReadChar(sval);
		fromPlayNow = sval > 0;
	}
	if (status && br.ReadBit())
	{
		status = status && theStream.ReadLocString(strval);
		partOfMapName = strval;
	}
	if (status && br.ReadBit())
	{
		status = status && theStream.ReadLocString(strval);
		partOfServerName = strval;
	}
	if (status && br.ReadBit())
	{
		status = status && theStream.ReadUChar(val);
		requiresPassword = val>0;
	}
	if (status && br.ReadBit())
	{
		status = status && theStream.ReadUChar(val);
		noMod = val>0;
	}
	return status;
}

