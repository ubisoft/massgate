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
//template based vector classes

//NOTE: Default constructors DO NOT initialize members (this is a speed optimization)

#ifndef MC_VECTOR_H
#define MC_VECTOR_H

#include "MC_Math.h"

//FORWARD DECLARATIONS
template <class Type> class MC_Vector3;
template <class Type> class MC_Vector4;


//2D VECTOR
template <class Type>
class MC_Vector2
{
public:

	//CONSTRUCTORS
	MC_Vector2() {
#ifdef MC_HEAVY_DEBUG_MC_VECTOR_NAN_INIT
		// Set to -NaN so we can easier detect usage of un-inited data (through float exceptions)
		memset(this, 0xff, sizeof(*this));
#endif
	}
	MC_Vector2(Type aX, Type aZ)											{myX = aX; myZ = aZ;}
	MC_Vector2(const MC_Vector3<Type>& aMC_Vector)							{myX = aMC_Vector.myX; myZ = aMC_Vector.myZ;}
	
	template <class AnotherType> 
	MC_Vector2(const MC_Vector2<AnotherType>& aMC_Vector)					{myX = (Type)aMC_Vector.myX; myZ = (Type)aMC_Vector.myZ;}

	//DESTRUCTOR
	~MC_Vector2()															{}

	//ASSIGNMENT
	void Set(Type aX, Type aZ)												{myX = aX; myZ = aZ;}
	template <class AnotherType> 
	MC_Vector2<Type>& operator=(const MC_Vector2<AnotherType>& aMC_Vector)	{myX = (Type)aMC_Vector.myX; myZ = (Type)aMC_Vector.myZ; return *this;}
	MC_Vector2<Type>& operator=(const MC_Vector2<Type>& aMC_Vector)			{myX = aMC_Vector.myX; myZ = aMC_Vector.myZ; return *this;}
	MC_Vector2<Type>& operator=(const MC_Vector3<Type>& aMC_Vector)			{myX = aMC_Vector.myX; myZ = aMC_Vector.myZ; return *this;}

	//MODIFYING OPERATIONS
	MC_Vector2<Type>& operator+=(const MC_Vector2<Type>& aMC_Vector)		{myX += aMC_Vector.myX; myZ += aMC_Vector.myZ; return *this;}
	MC_Vector2<Type>& operator-=(const MC_Vector2<Type>& aMC_Vector)		{myX -= aMC_Vector.myX; myZ -= aMC_Vector.myZ; return *this;}
    
	MC_Vector2<Type>& operator*=(Type aValue)								{myX *= aValue; myZ *= aValue; return *this;}
	MC_Vector2<Type>& operator/=(Type aValue)								{myX /= aValue; myZ /= aValue; return *this;}

	//CREATIVE OPERATORS WITH VECTORS
	MC_Vector2<Type> operator+(const MC_Vector2<Type>& aMC_Vector) const	{return MC_Vector2<Type>(myX+aMC_Vector.myX, myZ+aMC_Vector.myZ);}
	MC_Vector2<Type> operator-(const MC_Vector2<Type>& aMC_Vector) const	{return MC_Vector2<Type>(myX-aMC_Vector.myX, myZ-aMC_Vector.myZ);}
	MC_Vector2<Type> operator*(const MC_Vector2<Type>& aMC_Vector) const	{return MC_Vector2<Type>(myX*aMC_Vector.myX, myZ*aMC_Vector.myZ);}
	MC_Vector2<Type> operator/(const MC_Vector2<Type>& aMC_Vector) const	{return MC_Vector2<Type>(myX/aMC_Vector.myX, myZ/aMC_Vector.myZ);}

	//CREATIVE OPERATORS WITH SCALARS
	MC_Vector2<Type> operator*(Type aValue) const							{return MC_Vector2<Type>(myX * aValue, myZ * aValue);}
	MC_Vector2<Type> operator/(Type aValue) const							{return MC_Vector2<Type>(myX / aValue, myZ / aValue);}

    //COMPARISON
	int operator==(const MC_Vector2<Type>& aMC_Vector) const				{return (myX == aMC_Vector.myX && myZ == aMC_Vector.myZ);} 
	int operator!=(const MC_Vector2<Type>& aMC_Vector) const				{return !(*this == aMC_Vector);} 
	
	//NEGATION
	MC_Vector2<Type> operator-() const										{return MC_Vector2<Type>(-myX, -myZ);}

	//MATH
	MC_Vector2<Type> Normal() const											{return MC_Vector2<Type>(myZ, -myX);}
	Type Length() const														{return (Type)(sqrt((Type)(myX * myX + myZ * myZ)));}
	Type Length2() const													{return (myX * myX + myZ * myZ);}
	MC_Vector2<Type>& Scale(Type aMC_Vector)								{Normalize(); operator *= (aMC_Vector); return *this;}
	MC_Vector2<Type>& Normalize()											{operator /= (Length()); return *this;}
	MC_Vector2<Type>& NormalizeSafe()										{if(Length2() > (Type)0.00001)  Normalize() ; else myX = myZ = 0; return *this;}

	MC_Vector2<Type> GetNormalized() const									{return *this / Length();}
	MC_Vector2<Type> GetNormalizedSafe() const								{MC_Vector2<Type>(*this).NormalizeSafe();}

	MC_Vector2<Type>& MulElements( const MC_Vector2<Type>& aVect )			{x*=aVect.x; y*=aVect.y; return *this;}

	//UTILITY
	//true if the vector is right of aVect
	bool RightOf(const MC_Vector2<Type>& aVect) const						{return ((myX * aVect.myZ) < (myZ * aVect.myX));}

	//true if the vector is left of aVect
	bool LeftOf(const MC_Vector2<Type>& aVect) const						{return ((myX * aVect.myZ) > (myZ * aVect.myX));}

	//if deviation > 0, the vector is right of aVect
	//if deviation < 0, the vector is left of aVect
	Type Deviation(const MC_Vector2<Type>& aVect) const						{return ((myX * aVect.myZ) - (myZ * aVect.myX));}

	//create aMC_Vector vector rotated 90 degrees right
	MC_Vector2<Type> CreateRightNormal() const								{return MC_Vector2<Type>(-myZ, myX);}

	//create aMC_Vector vector rotated 90 degrees left
	MC_Vector2<Type> CreateLeftNormal()	const								{return MC_Vector2<Type>(myZ, -myX);}

	//Rotate the vector
	MC_Vector2<Type>& Rotate(float anAngle)			
	{ 
		float sinVal, cosVal;
		MC_SinCos(anAngle, &sinVal, &cosVal);
		Type temp;

		temp = (Type) (myX * cosVal + myY * sinVal);
		myY = (Type) (myY * cosVal - myX * sinVal);
		myX = temp;
		return *this;
	}

	//dot product
	Type Dot(const MC_Vector2<Type>& aMC_Vector) const						{return aMC_Vector.myX*myX + aMC_Vector.myZ*myZ;}

	//members
	union	{Type myX; Type myU; Type myWidth; Type x;};
	union	{Type myZ; Type myY; Type myV; Type myHeight; Type y; Type z; };
};


//3D VECTOR
template <class Type>
class MC_Vector3
{
public:
	//CONSTRUCTORS
	MC_Vector3()
	{
#ifdef MC_HEAVY_DEBUG_MC_VECTOR_NAN_INIT
		// Set to -NaN so we can easier detect usage of un-inited data (through float exceptions)
		memset(this, 0xff, sizeof(*this));
#endif
	}
	MC_Vector3(Type aX, Type aY, Type aZ)									{myX = aX; myY = aY; myZ = aZ;}

	template <class AnotherType> 
	MC_Vector3(const MC_Vector2<AnotherType>& aMC_Vector)					{myX = (Type)aMC_Vector.myX; myY = 0; myZ = (Type)aMC_Vector.myZ;}

	template <class AnotherType> 
	MC_Vector3(const MC_Vector3<AnotherType>& aMC_Vector)					{myX = (Type)aMC_Vector.myX; myY = (Type)aMC_Vector.myY; myZ = (Type)aMC_Vector.myZ;}

	template <class AnotherType> 
	MC_Vector3(const MC_Vector4<AnotherType>& aMC_Vector)					{myX = (Type)aMC_Vector.myX; myY = (Type)aMC_Vector.myY; myZ = (Type)aMC_Vector.myZ;}

	//DESTRUCTOR
	~MC_Vector3()															{}
    
	//ASSIGNMENT
	void Set(Type aX, Type aY, Type aZ)										{myX = aX; myY = aY; myZ = aZ;}

	template <class AnotherType> 
	MC_Vector3<Type>& operator=(const MC_Vector2<AnotherType>& aMC_Vector) 	{myX = (Type)aMC_Vector.myX; myY = 0; myZ = (Type)aMC_Vector.myZ; return *this;}

	template <class AnotherType> 
	MC_Vector3<Type>& operator=(const MC_Vector3<AnotherType>& aMC_Vector) 	{myX = (Type)aMC_Vector.myX; myY = (Type)aMC_Vector.myY; myZ = (Type)aMC_Vector.myZ; return *this;}

	template <class AnotherType> 
	MC_Vector3<Type>& operator=(const MC_Vector4<AnotherType>& aMC_Vector) 	{myX = (Type)aMC_Vector.myX; myY = (Type)aMC_Vector.myY; myZ = (Type)aMC_Vector.myZ; return *this;}

	//MODIFYING OPERATIONS
	MC_Vector3<Type>& operator+=(const MC_Vector3<Type>& aMC_Vector)		{myX += aMC_Vector.myX; myY += aMC_Vector.myY; myZ+= aMC_Vector.myZ; return *this;}
	MC_Vector3<Type>& operator-=(const MC_Vector3<Type>& aMC_Vector)		{myX -= aMC_Vector.myX; myY -= aMC_Vector.myY; myZ-= aMC_Vector.myZ; return *this;}

	MC_Vector3<Type>& operator*=(Type aValue)								{myX *= aValue; myY *= aValue; myZ *= aValue; return *this;}
	MC_Vector3<Type>& operator/=(Type aValue)								{Type inv = (Type) 1.0 / aValue; myX *= inv; myY *= inv; myZ *= inv; return *this;}

	//CREATIVE OPERATORS
	MC_Vector3<Type> operator+(const MC_Vector3<Type>& aMC_Vector) const	{return MC_Vector3<Type>(myX + aMC_Vector.myX, myY + aMC_Vector.myY, myZ + aMC_Vector.myZ);}
	MC_Vector3<Type> operator-(const MC_Vector3<Type>& aMC_Vector) const	{return MC_Vector3<Type>(myX - aMC_Vector.myX, myY - aMC_Vector.myY, myZ - aMC_Vector.myZ);}

	MC_Vector3<Type>  operator*(Type aValue) const							{return MC_Vector3<Type>(myX * aValue, myY * aValue,  myZ * aValue);}
	MC_Vector3<Type>  operator/(Type aValue) const							{return MC_Vector3<Type>(myX / aValue, myY / aValue,  myZ / aValue);}

    //COMPARISON
	bool operator==(const MC_Vector3<Type>& aMC_Vector) const				{return (myX == aMC_Vector.myX) && (myY == aMC_Vector.myY) && (myZ == aMC_Vector.myZ);}
	bool operator!=(const MC_Vector3<Type>& aMC_Vector) const				{return (myX != aMC_Vector.myX) || (myY != aMC_Vector.myY) || (myZ != aMC_Vector.myZ);}

	//NEGATION
	MC_Vector3<Type> operator-() const										{return MC_Vector3<Type>(-myX, -myY, -myZ);}

    //MATH
	Type Length() const														{return (Type)(sqrt(myX * myX + myY * myY + myZ * myZ));}
	Type Length2() const													{return (myX * myX + myY * myY + myZ * myZ);}
	Type Length2D() const														{return (Type)(sqrt(myX * myX + myZ * myZ));}
	Type Length2D2() const													{return (myX * myX + myZ * myZ);}
 	MC_Vector3<Type>& Scale(Type aValue)									{Normalize(); operator *= (aValue); return *this;}

	MC_Vector3<Type>& Normalize()											{operator /= (Length()); return *this;}
	MC_Vector3<Type>& NormalizeSafe();

	MC_Vector3<Type> GetNormalized() const									{return *this / Length();}
	MC_Vector3<Type> GetNormalizedSafe() const;

	MC_Vector3<Type>& NormalizeFast();
	MC_Vector3<Type>& NormalizeFastSafe();

	MC_Vector3<Type>& MulElements( const MC_Vector3<Type>& aMC_Vector )		{x*=aMC_Vector.x; y*=aMC_Vector.y; z*=aMC_Vector.z; return *this;}
	MC_Vector3<Type> Unit() const											{return MC_Vector3<Type>(*this).Normalize();}
	MC_Vector3<Type> UnitSafe() const										{return MC_Vector3<Type>(*this).NormalizeSafe();}

	Type Psi() const														{return atan2(myX, myZ);}
	Type Theta() const														{return atan2(myY, sqrt(myX*myX +myZ*myZ));}

	MC_Vector3<Type> GetMin(const MC_Vector3<Type>& aMC_Vector) const
	{
		return MC_Vector3<Type>(__min(x, aMC_Vector.x), __min(y, aMC_Vector.y), __min(z, aMC_Vector.z));
	}

	MC_Vector3<Type> GetMax(const MC_Vector3<Type>& aMC_Vector) const
	{
		return MC_Vector3<Type>(__max(x, aMC_Vector.x), __max(y, aMC_Vector.y), __max(z, aMC_Vector.z));
	}

	MC_Vector3<Type> Cross(const MC_Vector3<Type>& aMC_Vector) const
	{
		return MC_Vector3<Type>(
			myY * aMC_Vector.myZ - myZ * aMC_Vector.myY,
			myZ * aMC_Vector.myX - myX * aMC_Vector.myZ,
			myX * aMC_Vector.myY - myY * aMC_Vector.myX);
	}

	Type Dot(const MC_Vector3<Type>& aMC_Vector) const						{return aMC_Vector.myX * myX + aMC_Vector.myY * myY + aMC_Vector.myZ * myZ;}

	//Rotate the vector around x
	MC_Vector3<Type>& RotateAroundX(float anAngle)			
	{ 
		float sinVal, cosVal;
		MC_SinCos(anAngle, &sinVal, &cosVal);
		Type temp;

		temp = (Type) (myY * cosVal + myZ * sinVal);
		myZ = (Type) (myZ * cosVal - myY * sinVal);
		myY = temp;
		return *this;
	}

	//Rotate the vector around y
	MC_Vector3<Type>& RotateAroundY(float anAngle)			
	{ 
		float sinVal, cosVal;
		MC_SinCos(anAngle, &sinVal, &cosVal);
		Type temp;

		temp = (Type) (myX * cosVal + myZ * sinVal);
		myZ = (Type) (myZ * cosVal - myX * sinVal);
		myX = temp;
		return *this;
	}

	//Rotate the vector around z
	MC_Vector3<Type>& RotateAroundZ(float anAngle)			
	{ 
		float sinVal, cosVal;
		MC_SinCos(anAngle, &sinVal, &cosVal);
		Type temp;

		temp = (Type) (myX * cosVal + myY * sinVal);
		myY = (Type) (myY * cosVal - myX * sinVal);
		myX = temp;
		return *this;
	}

	// ACCESS
	__forceinline const Type operator[](int index) const { return (&x)[index]; }
	__forceinline Type& operator[](int index) { return (&x)[index]; }

	//MEMBERS
	union	{Type myX; Type myR; Type x; Type r; };
	union	{Type myY; Type myG; Type y; Type g; };
	union	{Type myZ; Type myB; Type z; Type b; };
};



//4d vector
template <class Type>
class MC_Vector4 
{
public:
	//CONSTRUCTORS
	MC_Vector4() {
#ifdef MC_HEAVY_DEBUG_MC_VECTOR_NAN_INIT
		// Set to -NaN so we can easier detect usage of un-inited data (through float exceptions)
		memset(this, 0xff, sizeof(*this));
#endif
	}
	MC_Vector4(Type aX, Type aY, Type aZ, Type aW = Type(1))				{myX = aX; myY = aY; myZ = aZ; myW = aW;}

	template <class AnotherType> 
	MC_Vector4(const MC_Vector4<AnotherType>& aMC_Vector)					{myX = (Type)aMC_Vector.myX; myY = (Type)aMC_Vector.myY; myZ = (Type)aMC_Vector.myZ; myW = (Type)aMC_Vector.myW;}
    
	template <class AnotherType> 
	MC_Vector4(const MC_Vector3<AnotherType>& aMC_Vector)					{myX = (Type)aMC_Vector.myX; myY = (Type)aMC_Vector.myY; myZ = (Type)aMC_Vector.myZ; myW = (Type)1;}

	//DESTRUCTOR
	~MC_Vector4(){};

	//ASSIGNMENT
	void Set(Type aX, Type aY, Type aZ, Type aW)							{myX = aX; myY = aY; myZ = aZ; myW = aW;}

	void Set(const MC_Vector3<Type>& aVec3, Type aW = Type(1))				{myX = aVec3.myX; myY = aVec3.myY; myZ = aVec3.myZ; myW = aW;}

	void SetVec3(const MC_Vector3<Type>& aVec3)								{ myX = aVec3.myX; myY = aVec3.myY; myZ = aVec3.myZ; }

	template <class AnotherType> 
	MC_Vector4<Type>& operator=(const MC_Vector3<AnotherType>& aMC_Vector) 	{myX = (Type)aMC_Vector.myX; myY = (Type)aMC_Vector.myY; myZ = (Type)aMC_Vector.myZ; myW =Type(1); return *this;}

	template <class AnotherType> 
	MC_Vector4<Type>& operator=(const MC_Vector4<AnotherType>& aMC_Vector) 	{myX = (Type)aMC_Vector.myX; myY = (Type)aMC_Vector.myY; myZ = (Type)aMC_Vector.myZ; myW = (Type)aMC_Vector.myW; return *this;}

    //MODIFYING OPERATORS
	MC_Vector4<Type>& operator+=(const MC_Vector4<Type>& aMC_Vector)		{myX += aMC_Vector.myX; myY += aMC_Vector.myY; myZ += aMC_Vector.myZ; myW += aMC_Vector.myW; return *this;}
	MC_Vector4<Type>& operator-=(const MC_Vector4<Type>& aMC_Vector)		{myX -= aMC_Vector.myX; myY += aMC_Vector.myY; myZ -= aMC_Vector.myZ; myW -= aMC_Vector.myW; return *this;}

	MC_Vector4<Type>& operator*=(Type aMC_Vector)							{myX *= aMC_Vector; myY *= aMC_Vector; myZ *= aMC_Vector; myW *= aMC_Vector; return *this;}
	MC_Vector4<Type>& operator/=(Type aMC_Vector)							{myX /= aMC_Vector; myY /= aMC_Vector; myZ /= aMC_Vector; myW /= aMC_Vector; return *this;}

	//CREATIVE OPERATORS
	MC_Vector4<Type> operator+(const MC_Vector4<Type>& aMC_Vector) const	{return MC_Vector4<Type>(myX + aMC_Vector.myX, myY + aMC_Vector.myY, myZ + aMC_Vector.myZ, myW + aMC_Vector.myW);}
	MC_Vector4<Type> operator-(const MC_Vector4<Type>& aMC_Vector) const	{return MC_Vector4<Type>(myX - aMC_Vector.myX, myY - aMC_Vector.myY, myZ - aMC_Vector.myZ, myW - aMC_Vector.myW);}

	MC_Vector4<Type>  operator*(Type aMC_Vector) const						{return MC_Vector4<Type>(myX * aMC_Vector, myY * aMC_Vector, myZ * aMC_Vector, myW * aMC_Vector);}
	MC_Vector4<Type>  operator/(Type aMC_Vector) const						{return MC_Vector4<Type>(myX / aMC_Vector, myY / aMC_Vector, myZ / aMC_Vector, myW / aMC_Vector);}

    //COMPARISON
	int operator==(const MC_Vector4<Type>& aMC_Vector) const				{return (myX == aMC_Vector.myX && myY == aMC_Vector.myY && myZ == aMC_Vector.myZ && myW == aMC_Vector.myW);}
	int operator!=(const MC_Vector4<Type>& aMC_Vector) const				{return !(*this == aMC_Vector);}

	//NEGATION
	MC_Vector4<Type> operator-() const										{return MC_Vector4<Type>(-myX, -myY, -myZ, -myW);}
	
	//MATH
	MC_Vector4<Type>& MulElements( const MC_Vector4<Type>& aVect )			{x*=aVect.x; y*=aVect.y; z*=aVect.z; w*=aVect.w; return *this;}
	Type Dot(const MC_Vector4<Type>& aMC_Vector) const						{return aMC_Vector.myX * myX + aMC_Vector.myY * myY + aMC_Vector.myZ * myZ + aMC_Vector.myW * myW;}
	Type Length() const														{return (Type) sqrt(myX * myX + myY * myY + myZ * myZ + myW * myW);}
	Type Length2() const													{return (myX * myX + myY * myY + myZ * myZ + myW * myW);}
	MC_Vector4<Type>& Scale(Type aMC_Vector)								{Normalize(); operator *= (aMC_Vector); return *this;}
	MC_Vector4<Type>& Normalize()											{operator /= (Length()); return *this;}
	MC_Vector4<Type> GetNormalized() const									{return *this / Length();}
	MC_Vector4<Type>& NormalizeSafe()										{if(Length2() > 0) Normalize(); return *this;}
	MC_Vector4<Type>& NormalizeSafe2()										{if(Length2() > 0) Normalize(); else {x=y=z=0;w=1;} return *this;}
	MC_Vector4<Type> Unit() const											{return MC_Vector4<Type>(*this).Normalize();}
	MC_Vector4<Type> UnitSafe() const										{return MC_Vector4<Type>(*this).NormalizeSafe();}

	//VEC3 ACCESS
	MC_Vector3<Type>& Vec3(){ return *(MC_Vector3<Type>*)this; }
	const MC_Vector3<Type>& Vec3() const { return *(const MC_Vector3<Type>*)this; }

	// ACCESS
	__forceinline Type operator[](int index) const { return (&x)[index]; }
	__forceinline Type& operator[](int index) { return (&x)[index]; }

	//MEMBERS
	union {Type myX; Type myR; Type myLeft; Type x; Type r; };
	union {Type myY; Type myG; Type myTop; Type y; Type g; };
	union {Type myZ; Type myB; Type myRight; Type z; Type b; };
	union {Type myW; Type myA; Type myBottom; Type w; Type a; };
};

template <class Type>
__forceinline MC_Vector3<Type> MC_Vector3<Type>::GetNormalizedSafe() const
{
	Type length = Length();

	if(length > 0)
		return *this / length;

	return *this;
}

template <class Type>
__forceinline MC_Vector3<Type>& MC_Vector3<Type>::NormalizeSafe()
{
	const Type length = Length();

	if(length > 0)
		*this /= length;

	return *this;								
}

template <class Type>
__forceinline MC_Vector3<Type>& MC_Vector3<Type>::NormalizeFast()
{
	const float invlen = MC_InvSqrtFastSafe(Length2());

	x *= invlen;
	y *= invlen;
	z *= invlen;

	return *this;
}

template <class Type>
__forceinline MC_Vector3<Type>& MC_Vector3<Type>::NormalizeFastSafe()
{
	const float invlen = MC_InvSqrtFastSafe(Length2());

	x *= invlen;
	y *= invlen;
	z *= invlen;

	return *this;
}



//TYPEDEFS
//vector2
typedef MC_Vector2<int> MC_Vector2i;
typedef MC_Vector2<unsigned int> MC_Vector2ui;
typedef MC_Vector2<double> MC_Vector2d;
typedef MC_Vector2<float> MC_Vector2f;
typedef MC_Vector2<short> MC_Vector2s;

typedef MC_Vector2i Point2i;
typedef MC_Vector2ui Point2ui;
typedef MC_Vector2d Point2d;
typedef MC_Vector2f Point2f;

typedef MC_Vector2f TexCoord;

//vector3
typedef MC_Vector3<int> MC_Vector3i;
typedef MC_Vector3<double> MC_Vector3d;
typedef MC_Vector3<float> MC_Vector3f;
typedef MC_Vector3<unsigned char> MC_Vector3uc;
typedef MC_Vector3<short> MC_Vector3s;

typedef MC_Vector3i Point3i;
typedef MC_Vector3d Point3d;
typedef MC_Vector3f Point3f;

typedef MC_Vector3f Vertex;
typedef MC_Vector3f Normal;
typedef MC_Vector3f Color;
typedef MC_Vector3uc MC_Rgb;

//vector4
typedef MC_Vector4<int> MC_Vector4i;
typedef MC_Vector4<double> MC_Vector4d;
typedef MC_Vector4<float> MC_Vector4f;
typedef __declspec(align(16)) MC_Vector4<float> MC_Vector4fA;	// Aligned to 16-byte boundry for SSE optimizations
typedef MC_Vector4<unsigned char> MC_Vector4uc;
typedef MC_Vector4<short> MC_Vector4s;

typedef MC_Vector4i Point4i;
typedef MC_Vector4d Point4d;
typedef MC_Vector4f Point4f;

typedef MC_Vector4f ColorRGBA;
typedef MC_Vector4uc MC_Rgba;


#endif
