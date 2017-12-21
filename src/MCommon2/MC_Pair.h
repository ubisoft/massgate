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
#pragma once 

// an implementation of std::pair. useful in conjunction with MC_Publisher etc.

#ifndef MC_PAIR_H
#define MC_PAIR_H

template<typename T0, typename T1>
class MC_Pair
{
public:
	MC_Pair()
	{
	}
	MC_Pair(const T0& aFirst, const T1& aSecond)
		: myFirst(aFirst), mySecond(aSecond)
	{
	}

	void Set(const T0& aFirst, const T1& aSecond)
	{
		myFirst  = aFirst;
		mySecond = aSecond;
	}

	T0 myFirst;
	T1 mySecond;
};

template<typename T0, typename T1> MC_Pair<T0, T1> MakePair(const T0& aFirst, const T1& aSecond)
{
	return MC_Pair<T0, T1>(aFirst, aSecond);
}

#endif // end #ifndef MC_PAIR_H
