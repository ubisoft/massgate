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
//bitvector.h
//vector of boolean values (bits), courtesy of Timothy Budd

#ifndef MC_BITVECTOR_H
#define MC_BITVECTOR_H

class MC_BitVector
{
public:

	MC_BitVector();
	MC_BitVector(unsigned int aNumIndices);
	MC_BitVector(const MC_BitVector& aBitVector);
	~MC_BitVector();
	void operator = (const MC_BitVector& aBitVector);


	void Init(unsigned int aNumIndices);
	void Resize(unsigned int aNumIndices);



	const unsigned int GetNumIndices()	const;		

	void Set(unsigned int anIndex);
	void Clear(unsigned int anIndex);
	bool Test(unsigned int anIndex) const;
	bool Test8(unsigned int anIndex) const; // tests the 8 bits adjacent to this index and returns true if any is set
	void Flip(unsigned int anIndex);

	void Clear();
	void SetAll();

	unsigned char* GetData() const		{return myData;}
	unsigned int GetDataLength() const	{return myDataLength;}

	struct Proxy
	{
		Proxy(unsigned char& aValue, unsigned int aMask);
		Proxy& operator=(bool aValue);
		
		bool operator==(bool aValue) const;
		bool operator!=(bool aValue) const;
		operator const bool () const;

	private:
		unsigned char& myValue;
		unsigned int myMask;
	};

	Proxy operator[](unsigned int anIndex);
	const bool operator[](unsigned int anIndex) const;

private:
	unsigned char* myData;
	unsigned int myDataLength;		//real data length (bytes)
	unsigned int myNumIndices;		//valid bit indices
};

class MC_BitVector2D : private MC_BitVector
{
public:
	typedef MC_BitVector::Proxy Proxy;

	MC_BitVector2D();
	MC_BitVector2D(unsigned int aWidth, unsigned int aHeight);

	void Init(unsigned int aWidth, unsigned int aHeight);

	Proxy operator()(unsigned int aX, unsigned int aY);
	const bool operator()(unsigned int aX, unsigned int aY) const;

	unsigned int GetWidth() const;
	unsigned int GetHeight() const;

private:
	unsigned int myWidth;
	unsigned int myHeight;
};




inline MC_BitVector::MC_BitVector()
	: myData(0), myDataLength(0), myNumIndices(0)
{
}

inline MC_BitVector::MC_BitVector(unsigned int aNumIndices)
	: myData(0), myDataLength(0), myNumIndices(0)
{
	Init(aNumIndices);
}

inline const unsigned int MC_BitVector::GetNumIndices() const
{
	return(myNumIndices);
}

inline MC_BitVector::MC_BitVector(const MC_BitVector& aBitVector)
{
	myData = NULL;

	*this = aBitVector;
}

inline MC_BitVector::Proxy MC_BitVector::operator[](unsigned int anIndex)
{
	return Proxy(myData[anIndex>>3], 1 << (anIndex & 7));
}

inline const bool MC_BitVector::operator[](unsigned int anIndex) const
{
	return Test(anIndex);
}


inline void MC_BitVector::Set(unsigned int anIndex)
{
	myData[(anIndex >> 3)] |= (1 << (anIndex & 7));
}

inline void MC_BitVector::Clear(unsigned int anIndex)
{
	myData[(anIndex >> 3)] &= ~(1 << (anIndex & 7));
}

inline bool MC_BitVector::Test(unsigned int anIndex) const
{
	return 0 != (myData[(anIndex >> 3)] & (1 << (anIndex & 7)));
}

inline bool MC_BitVector::Test8(unsigned int anIndex) const
{
	return 0 != myData[anIndex >> 3];
}

inline void MC_BitVector::Flip(unsigned int anIndex)
{
	myData[(anIndex >> 3)] ^= (1 << (anIndex & 7));
}

inline MC_BitVector::Proxy::Proxy(unsigned char& aValue, unsigned int aMask)
: myValue(aValue), myMask(aMask)
{
}

inline MC_BitVector::Proxy& MC_BitVector::Proxy::operator=(bool aValue)
{
	if (aValue)
		myValue |= myMask;
	else
		myValue &= ~myMask;
	return *this;
}

inline bool MC_BitVector::Proxy::operator==(bool aValue) const
{
	return aValue == (0 != (myValue & myMask));
}

inline bool MC_BitVector::Proxy::operator!=(bool aValue) const
{
	return aValue != (0 != (myValue & myMask));
}

inline MC_BitVector::Proxy::operator const bool () const
{
	return (0 != (myValue & myMask));
}


inline MC_BitVector2D::MC_BitVector2D()
: MC_BitVector()
{
}

inline MC_BitVector2D::MC_BitVector2D(unsigned int aWidth, unsigned int aHeight)
: MC_BitVector(aWidth * aHeight)
, myWidth(aWidth)
, myHeight(aHeight)
{
}

inline void MC_BitVector2D::Init(unsigned int aWidth, unsigned int aHeight)
{
	MC_BitVector::Init(aWidth * aHeight);
	myWidth = aWidth;
	myHeight = aHeight;
}

inline MC_BitVector2D::Proxy MC_BitVector2D::operator()(unsigned int aX, unsigned int aY)
{
	assert ((aX < myWidth) && (aY < myHeight));
	return (*this)[aY*myWidth + aX];
}

inline const bool MC_BitVector2D::operator()(unsigned int aX, unsigned int aY) const
{
	assert ((aX < myWidth) && (aY < myHeight));
	return (*this)[aY*myWidth + aX];
}

inline unsigned int MC_BitVector2D::GetWidth() const
{
	return myWidth;
}

inline unsigned int MC_BitVector2D::GetHeight() const
{
	return myHeight;
}

#endif
