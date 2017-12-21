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
#ifndef MMG_AVERAGE___H___
#define MMG_AVERAGE___H___

template<typename T>
class MMG_Average
{
public:
	MMG_Average() { Reset(); }
	MMG_Average(const MMG_Average& aRhs) : myNumValues(aRhs.myNumValues), myAccumulatedValue(aRhs.myAccumulatedValue), myUseCache(false) { }
	MMG_Average& operator=(const MMG_Average& aRhs) { if (this != & aRhs) { myAccumulatedValue = aRhs.myAccumulatedValue; myNumValues = aRhs.myNumValues; myUseCache=false; } return *this; }
	bool operator<(const MMG_Average& aRhs) const { return GetAverage() < aRhs.GetAverage(); }
	bool operator==(const MMG_Average& aRhs) const { return GetAverage() == aRhs.GetAverage(); }
	T GetAverage() const { if (!myUseCache) { myCachedValue = myAccumulatedValue / myNumValues; myUseCache=true; } return myCachedValue; }
	void Reset() { myAccumulatedValue = 0; myNumValues = 0; myUseCache = false; }
	void AddValue(const T& theValue) { myAccumulatedValues += theValue; myNumValues++; myUseCache = false; }
private:
	T myAccumulatedValue;
	unsigned long myNumValues;
	bool myUseCache;
	mutable T myCachedValue;
};

#endif
