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
#ifndef MMG_Optional___H__
#define MMG_Optional___H__


// Keeps track if a variable T has been set with a value or not.

template<typename T>
class Optional
{
public:
	Optional() { myIsObserving = false; }
	explicit Optional(const T& aValue) { myValue = aValue; myIsObserving = true; }
	/*explicit*/ Optional(const Optional<T>& aValue) : myIsObserving(aValue.myIsObserving) { if (myIsObserving) myValue = aValue.myValue; }
	Optional<T>& operator=(const Optional<T>& aRhs) { myIsObserving = aRhs.myIsObserving; if (myIsObserving) myValue = aRhs.myValue; return *this; }
	Optional<T>& operator=(const T& aValue) { myValue = aValue; myIsObserving = true; return *this; }
	bool operator==(const T& aValue) const { if (myIsObserving && (myValue == aValue)) return true; return false; }
	operator T () const { assert(myIsObserving); return myValue; }
	operator T& ()  { assert(myIsObserving); return myValue; }
	bool IsSet() const { return myIsObserving; }
	void Clear() { myIsObserving = false; }
	~Optional() { }
private:
	T myValue;
	bool myIsObserving;
};


#endif
