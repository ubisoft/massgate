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
#include "mn_flagset.h"


//constructor
MN_FlagSet::MN_FlagSet(unsigned int aSize)
{
	assert(aSize);
	if(aSize)
	{
		myBits = new MC_BitVector(aSize);
		assert(myBits);

		Clear();
	}
	else
	{
		myBits = NULL;
	}
}


//destructor
MN_FlagSet::~MN_FlagSet()
{
	if(myBits)
	{
		delete myBits;
		myBits = 0;
	}
}


//operations
unsigned int MN_FlagSet::MaxSize()
{
	if(myBits)
		return myBits->GetNumIndices();
	else
		return 0;
}


unsigned int MN_FlagSet::Occupance()
{
	unsigned int i;
	unsigned int occupance = 0;
	
	assert(myBits);
	for(i=0;i<myBits->GetNumIndices();i++)
	{
		if(myBits->Test(i))
			occupance++;
	}

	return occupance;
}


void MN_FlagSet::Add(unsigned int anIndex)
{
	assert(myBits);
	if(anIndex < myBits->GetNumIndices())
	{
		myBits->Set(anIndex);
	}
}


void MN_FlagSet::Remove(unsigned int anIndex)
{
	assert(myBits);
	if(anIndex < myBits->GetNumIndices())
	{
		myBits->Clear(anIndex);
	}
}


void MN_FlagSet::Clear()
{
	myBits->Clear();
}


bool MN_FlagSet::Includes(unsigned int anIndex)
{
	assert(myBits);
	if(anIndex < myBits->GetNumIndices())
	{
		return myBits->Test(anIndex);
	}
	else
		return false;
}


bool MN_FlagSet::IsEmpty()
{
	unsigned int i;
	
	assert(myBits);
	for(i=0;i<myBits->GetNumIndices();i++)
	{
		if(myBits->Test(i))
			return false;
	}

	return true;
}


//modifying operations
MN_FlagSet& MN_FlagSet::Union(const MN_FlagSet& aSet)
{
	unsigned int i;
	
	assert(myBits && aSet.myBits);
	for(i=0;i<aSet.myBits->GetNumIndices();i++)
	{
		//merge sets
		if(myBits->Test(i) || aSet.myBits->Test(i))
			myBits->Set(i);
	}

	return *this;
}


MN_FlagSet& MN_FlagSet::Intersection(const MN_FlagSet& aSet)
{
	unsigned int i;
	
	assert(myBits && aSet.myBits);
	for(i=0;i<aSet.myBits->GetNumIndices();i++)
	{
		//intersect sets
		if(myBits->Test(i) && !aSet.myBits->Test(i))
			myBits->Clear(i);
	}

	return *this;
}


MN_FlagSet& MN_FlagSet::Difference(const MN_FlagSet& aSet)
{
	unsigned int i;
	
	assert(myBits && aSet.myBits);
	for(i=0;i<aSet.myBits->GetNumIndices();i++)
	{
		//differ sets
		if(myBits->Test(i) && !aSet.myBits->Test(i))
			myBits->Set(i);
		else
			myBits->Clear(i);
	}

	return *this;
}


bool MN_FlagSet::Equal(const MN_FlagSet& aSet)
{
	unsigned int i;
	
	assert(myBits && aSet.myBits);
	for(i=0;i<aSet.myBits->GetNumIndices();i++)
	{
		//test equality
		if(myBits->Test(i) != aSet.myBits->Test(i))
			return false;
	}

	return true;
}
