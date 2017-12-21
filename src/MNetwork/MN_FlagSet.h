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
#ifndef MN_FLAGSET_H
#define MN_FLAGSET_H

#include "mc_bitvector.h"

class MN_FlagSet
{
public:

	//constructor
	MN_FlagSet(unsigned int aSize);

	//destructor
	~MN_FlagSet();

	//operations
	unsigned int MaxSize();
	unsigned int Occupance();
	void Add(unsigned int anIndex);
	void Remove(unsigned int anIndex);
	void Clear();
	bool Includes(unsigned int anIndex);
	bool IsEmpty();

	//modifying operations
	MN_FlagSet& Union(const MN_FlagSet& aSet);
	MN_FlagSet& Intersection(const MN_FlagSet& aSet);
	MN_FlagSet& Difference(const MN_FlagSet& aSet);

	bool Equal(const MN_FlagSet& aSet);

	//access
	MC_BitVector& GetBits()	{return *myBits;}

private:

	MC_BitVector* myBits;
};

#endif