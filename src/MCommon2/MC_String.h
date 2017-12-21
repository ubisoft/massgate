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
#ifndef MC_STRING_H
#define MC_STRING_H

#include "mc_globaldefines.h"
#include "mc_mem.h"

#ifndef TCHAR
#include <tchar.h>
#endif // TCHAR
#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>

typedef wchar_t MC_LocChar;

int MC_Strlen(const char* aString);
int MC_Strlen(const MC_LocChar* aString);

int MC_MakeUpper(MC_LocChar* aString, int aBufSize);
int MC_MakeLower(MC_LocChar* aString, int aBufSize);

/*
 NOTE: Goto bottom of this file for string utility functions
 NOTE: Goto bottom of this file for string utility functions
 NOTE: Goto bottom of this file for string utility functions
*/

/*****************************************************************************
 MC_String
*****************************************************************************/

extern void* g_static_ref_to_null_data_mc_string2;

template<class C, int S=0>		// C=Character, S=Size
class MC_Str
{
#define MC_STRING_NULL				((C*)g_static_ref_to_null_data_mc_string2)
#define MC_STRING_IS_NOT_NULL(s)	((s)!=g_static_ref_to_null_data_mc_string2)

public:
	typedef C char_type;
	static const int static_size = S;


	MC_Str();
	MC_Str(const MC_Str& aString);
	MC_Str(const TCHAR* aString);
	MC_Str(const wchar_t* aString);
	MC_Str(const C* aString, int aStringLen);
	MC_Str(int aStringLen, char aFillChar);
	~MC_Str();

	operator const C* () const								{ return GetBuffer(); }

	__forceinline C& operator[](int aIndex)
	{
#ifdef MC_HEAVY_DEBUG_MC_STRING_BOUNDSCHECK
		assert(aIndex >= 0 && "String boundscheck");
		assert(aIndex == 0 || aIndex < GetBufferSize() && "String boundscheck");
#endif
		return GetBuffer()[aIndex];
	}
	__forceinline C& operator[](unsigned int aIndex)
	{
#ifdef MC_HEAVY_DEBUG_MC_STRING_BOUNDSCHECK
		assert(aIndex == 0 || int(aIndex) < GetBufferSize() && "String boundscheck");
#endif
		return GetBuffer()[aIndex];
	}

	__forceinline const C& operator[](int aIndex)	const
	{
#ifdef MC_HEAVY_DEBUG_MC_STRING_BOUNDSCHECK
		assert(aIndex >= 0 && "String boundscheck");
		assert(aIndex == 0 || aIndex < GetBufferSize() && "String boundscheck");
#endif
		return GetBuffer()[aIndex];
	}

	__forceinline const C& operator[](unsigned int aIndex) const
	{
#ifdef MC_HEAVY_DEBUG_MC_STRING_BOUNDSCHECK
		assert(aIndex == 0 || int(aIndex) < GetBufferSize() && "String boundscheck");
#endif
		return GetBuffer()[aIndex];
	}
	
	void Assign(const C* aString, int aStringLen);
	void Append(const C* aString, int aStringLen);

	MC_Str& operator=(const MC_Str& aString)				{ if(this != &aString) this->operator=(aString.GetBuffer()); return *this; }
	MC_Str& operator=(const TCHAR* aString);
	MC_Str& operator=(const wchar_t* aString);
	MC_Str& operator=(TCHAR aChar);
	MC_Str& operator=(wchar_t aChar);

	MC_Str& operator+=(const TCHAR* aString);
	MC_Str& operator+=(const wchar_t* aString);
	MC_Str& operator+=(TCHAR aChar);
	MC_Str& operator+=(wchar_t aChar);

	friend MC_Str operator+(const MC_Str& aString, const C* aString2)		{ MC_Str s=aString; s+=aString2; return s; }
	friend MC_Str operator+(const MC_Str& aString, const C aChar)			{ MC_Str s=aString; s+=aChar; return s; }

	friend bool operator==(const MC_Str& aString, const MC_Str& aString2)	{ return InternalCompare( aString, aString2 ) == 0; }
	friend bool operator==(const MC_Str& aString, const C* aString2)		{ return InternalCompare( aString, aString2 ) == 0; }
	friend bool operator==(const C* aString, const MC_Str& aString2)		{ return InternalCompare( aString, aString2 ) == 0; }

	friend bool operator!=(const MC_Str& aString, const MC_Str& aString2)	{ return InternalCompare( aString, aString2 ) != 0; }
	friend bool operator!=(const MC_Str& aString, const C* aString2)		{ return InternalCompare( aString, aString2 ) != 0; }
	friend bool operator!=(const C* aString, const MC_Str& aString2)		{ return InternalCompare( aString, aString2 ) != 0; }

	friend bool operator<(const MC_Str& aString, const MC_Str& aString2)	{ return InternalCompare( aString, aString2 ) < 0; }
	friend bool operator<(const MC_Str& aString, const C* aString2)			{ return InternalCompare( aString, aString2 ) < 0; }
	friend bool operator<(const C* aString, const MC_Str& aString2)			{ return InternalCompare( aString, aString2 ) < 0; }

	friend bool operator>(const MC_Str& aString, const MC_Str& aString2)	{ return InternalCompare( aString, aString2 ) > 0; }
	friend bool operator>(const MC_Str& aString, const C* aString2)			{ return InternalCompare( aString, aString2 ) > 0; }
	friend bool operator>(const C* aString, const MC_Str& aString2)			{ return InternalCompare( aString, aString2 ) > 0; }

	friend bool operator<=(const MC_Str& aString, const MC_Str& aString2)	{ return InternalCompare( aString, aString2 ) <= 0; }
	friend bool operator<=(const MC_Str& aString, const C* aString2)		{ return InternalCompare( aString, aString2 ) <= 0; }
	friend bool operator<=(const C* aString, const MC_Str& aString2)		{ return InternalCompare( aString, aString2 ) <= 0; }

	friend bool operator>=(const MC_Str& aString, const MC_Str& aString2)	{ return InternalCompare( aString, aString2 ) >= 0; }
	friend bool operator>=(const MC_Str& aString, const C* aString2)		{ return InternalCompare( aString, aString2 ) >= 0; }
	friend bool operator>=(const C* aString, const MC_Str& aString2)		{ return InternalCompare( aString, aString2 ) >= 0; }


	void SetLength(int aLength)					{ Reserve(aLength); (*this)[aLength] = 0; }
	int GetLength() const						{ return MC_Strlen( GetBuffer() ); }
	bool IsEmpty() const						{ return GetLength() == 0; }

	int Compare(const C* aString) const			{ return InternalCompare(GetBuffer(), aString); }
	int CompareNoCase(const C* aString) const	{ return InternalCompareNoCase(GetBuffer(), aString); }
	bool EndsWith(const C* aString) const		{ return InternalEndsWith(GetBuffer(), aString); }
	bool EndsWithNoCase(const C* aString) const	{ return InternalEndsWithNoCase(GetBuffer(), aString); }
	bool BeginsWith(const C* aString) const		{ return InternalBeginsWith(GetBuffer(), aString); }
	bool BeginsWithNoCase(const C* aString) const { return InternalBeginsWithNoCase(GetBuffer(), aString); }

	MC_Str& MakeUpper()							{ if (sizeof(C)>1) Reserve(GetLength()*2); InternalMakeUpper( GetBuffer(), GetBufferSize() ); return *this; }
	MC_Str& MakeLower()							{ if (sizeof(C)>1) Reserve(GetLength()*2); InternalMakeLower( GetBuffer(), GetBufferSize() ); return *this; }

	MC_Str& __cdecl Format(const C* aFormatString, ...);

	MC_Str& TrimRight();
	MC_Str& TrimLeft();

	MC_Str& Delete(int nIndex, int nCount = 1);

	int Find(const C aChar, int aStartPosition = 0) const;
	int Find(const C* aString) const			{ return InternalFind( GetBuffer(), aString ); }
	int ReverseFind(C aChar) const;
	int ReverseFind(C aChar, int aStartPosition) const;

	int Replace(const C* anOldString, const C* aNewString);
	int Replace(C aChOld, C aChNew);							// Replaces single chars only

	MC_Str SubStr(int nFirst, int nCount = -1) const;
	MC_Str Mid(int nFirst, int nCount) const;
	MC_Str Left(int nCount) const;
	MC_Str Right(int nCount) const;
	void Insert(int anIndex, const C* aString);

	int			GetBufferSize() const			{ return InternalGetBufferSize( *this ); }
	C*			GetBuffer()						{ return InternalGetBuffer( *this ); }
	const C*	GetBuffer() const				{ return InternalGetBuffer( *this ); }
	bool		IsDynamic() const				{ return (S < 1); }

	void		Reserve(int aLength)			{ InternalReserve(*this, aLength); }
	void		Clear()							{ if(GetBufferSize() > 0) GetBuffer()[0] = 0; }

	static inline MC_Str<wchar_t> TEMPConvertToLoc(const char* aString) // REMOVE LATER!!!
	{
		return MC_Str<wchar_t>(aString);
	}

protected:
	void SetBuffer( C* aPointer )				{ InternalSetBuffer( *this, aPointer ); }	// Note, sets the pointer only (not allowed on static strings)
	void SetBufferSize( int aSize )				{ InternalSetBufferSize( *this, aSize ); }

	void ReallocData(int aSize);	// Resize buffer and preserve contents. (makes room for aSize (+ null termination) characters)
	void AllocData(int aSize);		// Allocate buffer that has room for aSize characters (+ null termination). (Old contents are destroyed)
	void Dealloc();					// Deallocate string buffer and set length to 0
	void ZeroMembers();

	// Data
	union
	{
		C*	myDynamicPointer;			// Pointer to dynamically allocated string
		int* myLength;					// This variable is just here to help the debugger show the string length
		C myBuffer[ (S > 0) ? S : 1 ];	// Set up buffer size for static string
	};

	// --------- Internal helpers ---------

	// Static string functions
	template<class C, int S>	static void	InternalSetBuffer( MC_Str<C,S>& aStr, C* aPointer )		{ assert( 0 && "not allowed on static strings!" ); }
	template<class C, int S>	static C*	InternalGetBuffer( MC_Str<C,S>& aStr )					{ return aStr.myBuffer; }
	template<class C, int S>	static const C*	InternalGetBuffer( const MC_Str<C,S>& aStr )		{ return aStr.myBuffer; }
	template<class C, int S>	static int	InternalGetBufferSize( const MC_Str<C,S>& aStr )		{ return S; }
	template<class C, int S>	static void	InternalSetBufferSize( MC_Str<C,S>& aStr, int aSize )	{ assert( 0 && "not allowed on static strings!" ); }
	template<class C, int S>	static void	InternalReserve(MC_Str<C,S>& aStr, int aLength)			{ assert(aLength < S); }

	// Specialization for dynamic string
	template<class C>			static void	InternalSetBuffer( MC_Str<C,0>& aStr, C* aPointer )		{ aStr.myDynamicPointer = aPointer; }
	template<class C>			static C*	InternalGetBuffer( MC_Str<C,0>& aStr )					{ return aStr.myDynamicPointer; }
	template<class C>			static const C*	InternalGetBuffer( const MC_Str<C,0>& aStr )		{ return aStr.myDynamicPointer; }
	template<class C>			static int	InternalGetBufferSize( const MC_Str<C,0>& aStr )		{ return (MC_STRING_IS_NOT_NULL(aStr.myDynamicPointer)) ? *(int*)(aStr.myDynamicPointer-sizeof(int)/sizeof(C)) : 0; }
	template<class C>			static void	InternalSetBufferSize( MC_Str<C,0>& aStr, int aSize )	{ if(MC_STRING_IS_NOT_NULL(aStr.myDynamicPointer)) *(int*)(aStr.myDynamicPointer-sizeof(int)/sizeof(C)) = aSize; }
	template<class C>			static void	InternalReserve(MC_Str<C,0>& aStr, int aLength)			{ if (InternalGetBufferSize(aStr) <= aLength) aStr.ReallocData(aLength); }

	// Conversion functions
	template<class C>		C InternalConvertChar( TCHAR aChar ) const;
	template<class C>		C InternalConvertChar( wchar_t aChar ) const;

	template<>				TCHAR	InternalConvertChar<TCHAR>( TCHAR aChar ) const			{ return aChar; }
	template<>				TCHAR	InternalConvertChar<TCHAR>( wchar_t aChar ) const		{ return TCHAR(aChar); }

	template<>				wchar_t	InternalConvertChar<wchar_t>( TCHAR aChar ) const		{ return wchar_t(aChar); }
	template<>				wchar_t	InternalConvertChar<wchar_t>( wchar_t aChar ) const		{ return aChar; }

	// 8-bit (TCHAR) functions
	static void	InternalMakeUpper( TCHAR* aPointer, int aBufferSize )						{ if(aPointer && aBufferSize) _tcsupr_s(aPointer, aBufferSize); }
	static void	InternalMakeLower( TCHAR* aPointer, int aBufferSize )						{ if(aPointer && aBufferSize) _tcslwr_s(aPointer, aBufferSize); }
	static int	InternalCompare( const TCHAR* aString1, const TCHAR* aString2 )				{ return _tcscmp(aString1, aString2); }
	static int	InternalCompareNoCase( const TCHAR* aString1, const TCHAR* aString2 )		{ return _tcsicmp(aString1, aString2); }
	static bool	InternalEndsWith( const TCHAR* aString1, const TCHAR* aString2 )			{ size_t len1=_tcslen(aString1); size_t len2=_tcslen(aString2); return len1>=len2 ? (_tcscmp(aString1+len1-len2, aString2) == 0) : false; }
	static bool	InternalEndsWithNoCase( const TCHAR* aString1, const TCHAR* aString2 )		{ size_t len1=_tcslen(aString1); size_t len2=_tcslen(aString2); return len1>=len2 ? (_tcsicmp(aString1+len1-len2, aString2) == 0) : false; }
	static bool	InternalBeginsWith( const TCHAR* aString1, const TCHAR* aString2 )			{ size_t len1=_tcslen(aString1); size_t len2=_tcslen(aString2); return len1>=len2 ? (_tcsncmp(aString1, aString2, len2) == 0) : false; }
	static bool	InternalBeginsWithNoCase( const TCHAR* aString1, const TCHAR* aString2 )	{ size_t len1=_tcslen(aString1); size_t len2=_tcslen(aString2); return len1>=len2 ? (_tcsncicmp(aString1, aString2, len2) == 0) : false; }
	static int	InternalFind( const TCHAR* aString1, const TCHAR* aString2 )
	{
		const TCHAR* ret = _tcsstr(aString1, aString2);
		return ret ? int(ret - aString1) : -1;
	}

	// 16-bit (wchar_t) functions
	static void	InternalMakeUpper( wchar_t* aPointer, int aBufferSize )						{ if(aPointer && aBufferSize) MC_MakeUpper(aPointer, aBufferSize); }
	static void	InternalMakeLower( wchar_t* aPointer, int aBufferSize )						{ if(aPointer && aBufferSize) MC_MakeLower(aPointer, aBufferSize); }
	static int	InternalCompare( const wchar_t* aString1, const wchar_t* aString2 )			{ return wcscmp(aString1, aString2); }
	static int	InternalCompareNoCase( const wchar_t* aString1, const wchar_t* aString2 )	{ return _wcsicmp(aString1, aString2); }
	static bool	InternalEndsWith( const wchar_t* aString1, const wchar_t* aString2 )		{ size_t len1=wcslen(aString1); size_t len2=wcslen(aString2); return len1>=len2 ? (wcscmp(aString1+len1-len2, aString2) == 0) : false; }
	static bool	InternalEndsWithNoCase( const wchar_t* aString1, const wchar_t* aString2 )	{ size_t len1=wcslen(aString1); size_t len2=wcslen(aString2); return len1>=len2 ? (wcsicmp(aString1+len1-len2, aString2) == 0) : false; }
	static bool	InternalBeginsWith( const wchar_t* aString1, const wchar_t* aString2 )		{ size_t len1=wcslen(aString1); size_t len2=wcslen(aString2); return len1>=len2 ? (wcsncmp(aString1, aString2, len2) == 0) : false; }
	static bool	InternalBeginsWithNoCase( const wchar_t* aString1, const wchar_t* aString2 ){ size_t len1=wcslen(aString1); size_t len2=wcslen(aString2); return len1>=len2 ? (wcsnicmp(aString1, aString2, len2) == 0) : false; }
	static int	InternalFind( const wchar_t* aString1, const wchar_t* aString2 )
	{
		const wchar_t* ret = wcsstr(aString1, aString2);
		return ret ? int(ret - aString1) : -1;
	}
};

// ------------------------ specializations -------------------------

template<int S>
class MC_StaticString : public MC_Str<TCHAR, S>
{
public:
	MC_StaticString() {}
	MC_StaticString(const MC_StaticString& aString) :					MC_Str<TCHAR, S>(aString) {}
	MC_StaticString(const TCHAR* aString) :								MC_Str<TCHAR, S>(aString) {}
	MC_StaticString(const wchar_t* aString) :							MC_Str<TCHAR, S>(aString) {}
	MC_StaticString(const TCHAR* aString, int aStringLen) :				MC_Str<TCHAR, S>(aString, aStringLen) {}

	MC_StaticString& operator=(const MC_StaticString& aString)			{ if(this != &aString) this->operator=(aString.GetBuffer()); return *this; }
	MC_StaticString& operator=(const TCHAR* aString)					{ MC_Str<TCHAR, S>::operator=(aString); return *this; }
	MC_StaticString& operator=(const wchar_t* aString)					{ MC_Str<TCHAR, S>::operator=(aString); return *this; }
	MC_StaticString& operator=(TCHAR aChar)								{ MC_Str<TCHAR, S>::operator=(aChar); return *this; }
	MC_StaticString& operator=(wchar_t aChar)							{ MC_Str<TCHAR, S>::operator=(aChar); return *this; }
};


template<int S>
class MC_StaticLocString : public MC_Str<wchar_t, S>
{
public:
	MC_StaticLocString() {}
	MC_StaticLocString(const MC_StaticLocString& aString) :				MC_Str<wchar_t, S>(aString) {}
	MC_StaticLocString(const TCHAR* aString) :							MC_Str<wchar_t, S>(aString) {}
	MC_StaticLocString(const wchar_t* aString) :						MC_Str<wchar_t, S>(aString) {}
	MC_StaticLocString(const wchar_t* aString, int aStringLen) :		MC_Str<wchar_t, S>(aString, aStringLen) {}

	MC_StaticLocString& operator=(const MC_StaticLocString& aString)	{ if(this != &aString) this->operator=(aString.GetBuffer()); return *this; }
	MC_StaticLocString& operator=(const TCHAR* aString)					{ MC_Str<wchar_t, S>::operator=(aString); return *this; }
	MC_StaticLocString& operator=(const wchar_t* aString)				{ MC_Str<wchar_t, S>::operator=(aString); return *this; }
	MC_StaticLocString& operator=(TCHAR aChar)							{ MC_Str<wchar_t, S>::operator=(aChar); return *this; }
	MC_StaticLocString& operator=(wchar_t aChar)						{ MC_Str<wchar_t, S>::operator=(aChar); return *this; }
};

typedef MC_Str<TCHAR>	MC_String;
typedef MC_Str<wchar_t>	MC_LocString;


// --------------------- implementations ----------------------

template<class C, int S>
void MC_Str<C,S>::ZeroMembers()
{
	if(S < 1)	// if dynamic
		myDynamicPointer = MC_STRING_NULL;
	else		// else static
		GetBuffer()[0]=0;
}

template<class C, int S>
MC_Str<C,S>::MC_Str()
{
	ZeroMembers();
}

template<class C, int S>
MC_Str<C,S>::MC_Str(const MC_Str& aString)
{
	ZeroMembers();
	*this = aString;
}

template<class C, int S>
MC_Str<C,S>::MC_Str(const TCHAR* aString)
{
	ZeroMembers();
	*this = aString;
}

template<class C, int S>
MC_Str<C,S>::MC_Str(const wchar_t* aString)
{
	ZeroMembers();
	*this = aString;
}

template<class C, int S>
MC_Str<C,S>::MC_Str(const C* aString, int aStringLen)
{
	ZeroMembers();

	if(aString && aStringLen > 0)
	{
		AllocData(aStringLen);
		memcpy(GetBuffer(), aString, aStringLen * sizeof(C));
	}
}

template <class C, int S>
MC_Str<C,S>::MC_Str(int aStringLen, char aFillChar)
{
	ZeroMembers();
	if (aStringLen > 0)
	{
		AllocData(aStringLen);
		memset(GetBuffer(), aFillChar, aStringLen * sizeof(C));
	}
}

template<class C, int S>
MC_Str<C,S>::~MC_Str()
{
	Dealloc();
}

template <class C, int S>
void MC_Str<C,S>::Assign(const C* aString, int aStringLen)
{
	if(aString && aStringLen > 0)
	{
		Reserve(aStringLen);
		int toCopy = MC_Min(aStringLen, GetBufferSize()-1);
		memcpy(GetBuffer(), aString, toCopy * sizeof(C));
		GetBuffer()[toCopy] = 0;
	}
}

template <class C, int S>
void MC_Str<C,S>::Append(const C* aString, int aStringLen)
{
	if (aString && aStringLen > 0)
	{
		const int len = GetLength();
		if ((len + aStringLen + 1) > GetBufferSize())
			Reserve(len + aStringLen);
		const int toCopy = MC_Min(aStringLen, GetBufferSize()-1-len);
		assert (toCopy >= 0);
		if (toCopy)
			memcpy(GetBuffer()+len, aString, toCopy * sizeof(C));
		GetBuffer()[len + toCopy] = 0;
	}
}

template <class C, int S>
void MC_Str<C,S>::Insert(int anIndex, const C* aString)
{
	int newStrLen = MC_Strlen(aString);
	const int len = GetLength();

	Reserve(len + newStrLen);

	if (GetBufferSize() < len + newStrLen + 1) // can't deal with cramped static strings yet
		return;

	if (anIndex < 0)
		anIndex = len;

	if (anIndex >= len)
	{
		Append(aString, newStrLen);
	}
	else
	{
		memmove(GetBuffer() + anIndex + newStrLen, GetBuffer() + anIndex, (len - anIndex + 1) * sizeof(C));
		memcpy(GetBuffer() + anIndex, aString, newStrLen * sizeof(C));
	}
}


template<class C, int S>
MC_Str<C,S>& MC_Str<C,S>::TrimRight()
{
	int len = GetLength();
	if(len > 0)
	{
		C* pBuf = GetBuffer();
		for(C* p=pBuf + len - 1; p>=pBuf && *p && *p<=32; --p)
			*p = 0;
	}
	return *this;
}

template<class C, int S>
MC_Str<C,S>& MC_Str<C,S>::TrimLeft()
{
	int len = GetLength();
	if(len > 0)
	{
		C* pRead = GetBuffer();
		C* pWrite = pRead;

		// Find first non-white
		while(*pRead && *pRead<=32)
			pRead++;

		// Move string
		while(*pRead)
			*pWrite++ = *pRead++;

		// null termination
		*pWrite = 0;
	}
	return *this;
}

template<class C, int S>
MC_Str<C,S>& MC_Str<C,S>::Delete(int nIndex, int nCount = 1)
{
	if(nIndex < 0)
		return *this;

	int len = GetLength();
	if(len < 1)
		return *this;

	if(nIndex + nCount > len)
		nCount = len - nIndex;

	if(nCount < 1)
		return *this;

	C* pBuf = GetBuffer();
	C* pRead = pBuf + nIndex + nCount;
	C* pWrite = pBuf + nIndex;

	// Move string
	while( *pRead )
		*pWrite++ = *pRead++;

	// null termination
	*pWrite = 0;

	return *this;
}

template<class C, int S>
int MC_Str<C,S>::Find(const C aChar, int aStartPosition) const
{
	if (aStartPosition < 0)
		aStartPosition = 0;
	if (aStartPosition > GetLength())
		return -1;

	const C* pBuf = GetBuffer();

	if (pBuf)
	{
		const C* p = pBuf + aStartPosition;
		while(*p)
		{
			if(*p == aChar)
				return int(p - pBuf);
			++p;
		}
	}

	return -1;
}

template<class C, int S>
int MC_Str<C,S>::ReverseFind(C aChar) const
{
	const C* pBuf = GetBuffer();

	if(pBuf)
	{
		const C* p = pBuf + GetLength() - 1;
		while(p >= pBuf)
		{
			if(*p == aChar)
				return int(p - pBuf);
			--p;
		}
	}

	return -1;
}

template<class C, int S>
int MC_Str<C,S>::ReverseFind(C aChar, int aStartPosition) const
{
	const C* pBuf = GetBuffer();

	if(pBuf)
	{
		int len = GetLength();
		const C* p = pBuf + (aStartPosition > 0 && aStartPosition < len ? aStartPosition : len - 1);
		while(p >= pBuf)
		{
			if(*p == aChar)
				return int(p - pBuf);
			--p;
		}
	}

	return -1;
}

template<class C, int S>
void MC_Str<C,S>::ReallocData(int aSize)
{
	if( S > 0 )	// If static buffer
	{
		if( aSize >= S )
		{
			assert( 0 && "The static string buffer is too small" );
			aSize = S - 1;	// safety net
		}
	}
	else		// If dynamic buffer
	{
		if( aSize+1 > GetBufferSize() )	// only allocate more if we have to
		{
			C* pNew = (C*)MC_TempAllocIfOwnerOnStack(this, sizeof(int) + sizeof(C)*(aSize+1), __FILE__, __LINE__); //  new C[aSize + 1 + sizeof(int) / sizeof(C)];
			pNew += sizeof(int) / sizeof(C);
			memset((void*)pNew, 0, sizeof(C)*(aSize+1));

			C* pOld = GetBuffer();
			if (pOld)
			{
				const int oldLen = GetLength();
				memcpy( pNew, pOld, sizeof(C) * (aSize<oldLen?aSize:oldLen) );
				Dealloc();
			}
			SetBuffer(pNew);
			SetBufferSize(aSize+1);
		}
	}

	GetBuffer() [aSize] = 0;
}

template<class C, int S>
void MC_Str<C,S>::AllocData(int aSize)
{
	if( S > 0 )	// If static buffer
	{
		if( aSize >= S )
		{
			assert( 0 && "The static string buffer is too small" );
			aSize = S - 1;	// safety net
		}
	}
	else		// If dynamic buffer
	{
		if( aSize+1 > GetBufferSize() )	// Only allocate more if we have to
		{
			Dealloc();
			C* pNew = (C*)MC_TempAllocIfOwnerOnStack(this, sizeof(int) + sizeof(C)*(aSize+1), __FILE__, __LINE__); //  new C[aSize + 1 + sizeof(int) / sizeof(C)];
			pNew += sizeof(int) / sizeof(C);
			SetBuffer(pNew);
			SetBufferSize(aSize+1);
		}
	}

	//GetBuffer() [aSize] = 0;
	memset((void*)GetBuffer(), 0, sizeof(C)*(aSize+1));
}

template<class C, int S>
void MC_Str<C,S>::Dealloc()
{
	if( S > 0 )	// If static buffer
	{
		GetBuffer()[0] = 0;
	}
	else if (MC_STRING_IS_NOT_NULL(myDynamicPointer)) // If dynamic buffer and not empty
	{
		C* pOld = myDynamicPointer;
		pOld -= sizeof(int) / sizeof(C);
		MC_TempFree(pOld);
		SetBuffer(MC_STRING_NULL);
		SetBufferSize(0);
	}
}

template<class C, int S>
int MC_Str<C,S>::Replace(const C* anOldString, const C* aNewString)
{
	int numChanged = 0;
	int newstringLength = MC_Strlen(aNewString);
	int oldstringLength = MC_Strlen(anOldString);

	C* ptrData = GetBuffer();
	while( true )
	{
		int findIndex = InternalFind( ptrData, anOldString );
		if( findIndex >= 0 )
		{
			ptrData += findIndex + oldstringLength;
			numChanged++;
		}
		else
			break;
	}

	if(numChanged)
	{
		// Create copy of source string
		MC_Str<C,S> srcs(*this);

		// resize buffer to fit new string
		int newlen = GetLength() + (newstringLength-oldstringLength) * numChanged;
		AllocData(newlen);

		C* src = srcs.GetBuffer();
		C* dst = GetBuffer();
		for (int i=0; i<numChanged; i++)
		{
			int findIndex = InternalFind( src, anOldString );
			assert( findIndex >= 0 );

			// Copy to dest
			memcpy(dst, src, findIndex * sizeof(C));
			dst += findIndex;
			memcpy(dst, aNewString, newstringLength * sizeof(C));
			dst += newstringLength;

			src += findIndex + oldstringLength;
		}
		memcpy(dst, src, (newlen-(dst-GetBuffer())) * sizeof(C) );
	}

	return numChanged;
}

template<class C, int S>
int MC_Str<C,S>::Replace(C aChOld, C aChNew)
{
	int count = 0;
	C* p = GetBuffer();
	if(p)
	{
		while(*p)
		{
			if(*p == aChOld)
			{
				*p = aChNew;
				count++;
			}
			++p;
		}
	}
	return count;
}

template<class C, int S>
MC_Str<C,S> MC_Str<C,S>::SubStr(int nFirst, int nCount) const
{
	return Mid(nFirst, nCount);
}

template<class C, int S>
MC_Str<C,S> MC_Str<C,S>::Mid(int nFirst, int nCount) const
{
	if (nCount == 0)
		return MC_Str<C,S>();

	MC_Str<C,S> st;
	int i;

	i = GetLength();

	if (nFirst >= i)
		return st;

	if (nCount < 0)
		nCount = i - nFirst;

	if(nFirst + nCount > i)
		nCount = i - nFirst;

	st.AllocData(nCount);
	const C* pSrc = GetBuffer();
	C* pDest = st.GetBuffer();
	memcpy(pDest, pSrc + nFirst, nCount * sizeof(C));
	return st;
}

template<class C, int S>
MC_Str<C,S> MC_Str<C,S>::Left(int nCount) const
{
	if (nCount <= 0)
		return MC_Str<C,S>();

	MC_Str<C,S> st;
	int i;

	i = GetLength();

	if(nCount > i)
		nCount = i;

	st.AllocData(nCount);
	const C* pSrc = GetBuffer();
	C* pDest = st.GetBuffer();
	memcpy(pDest, pSrc, nCount * sizeof(C));
	return st;
}

template<class C, int S>
MC_Str<C,S> MC_Str<C,S>::Right(int nCount) const
{
	if (nCount <= 0)
		return MC_Str<C,S>();

	MC_Str<C,S> st;
	int i;

	i = GetLength();

	if(nCount > i)
		nCount = i;

	st.AllocData(nCount);
	const C* pSrc = GetBuffer();
	C* pDest = st.GetBuffer();
	memcpy(pDest, pSrc + i - nCount, nCount * sizeof(C));
	return st;
}

template<class C, int S>
MC_Str<C,S>& MC_Str<C,S>::operator=(const TCHAR* aString)
{
	if(GetBuffer() == (const C*)aString)
		return *this;

	int newlen = aString ? MC_Strlen( aString ) : 0;
	if( newlen )
	{
		if( (void*)aString != (void*)GetBuffer() )
		{
			AllocData(newlen);
			newlen = MC_Min(newlen, GetBufferSize() - 1);
			C* dest = GetBuffer();
			for( int i=0; i<newlen; i++ )
				dest[i] = InternalConvertChar<C> (aString[i]);
			dest[newlen] = 0;
		}
	}
	else
	{
		Dealloc();
	}
	return *this;
}

template<class C, int S>
MC_Str<C,S>& MC_Str<C,S>::operator=(const wchar_t* aString)
{
	if(GetBuffer() == (const C*)aString)
		return *this;

	int newlen = aString ? MC_Strlen( aString ) : 0;
	if( newlen )
	{
		if( (void*)aString != (void*)GetBuffer() )
		{
			AllocData(newlen);
			newlen = MC_Min(newlen, GetBufferSize() - 1);
			C* dest = GetBuffer();
			for( int i=0; i<newlen; i++ )
				dest[i] = InternalConvertChar<C> (aString[i]);
			dest[newlen] = 0;
		}
	}
	else
	{
		Dealloc();
	}
	return *this;
}

template<class C, int S>
MC_Str<C,S>& MC_Str<C,S>::operator=(TCHAR aChar)
{
	AllocData(1);
	GetBuffer()[0] = InternalConvertChar<C> (aChar);
	GetBuffer()[1] = 0;
	return *this;
}

template<class C, int S>
MC_Str<C,S>& MC_Str<C,S>::operator=(wchar_t aChar)
{
	AllocData(1);
	GetBuffer()[0] = InternalConvertChar<C> (aChar);
	GetBuffer()[1] = 0;
	return *this;
}

template<class C, int S>
MC_Str<C,S>& MC_Str<C,S>::operator+=(const TCHAR* aString)
{
	int oldLen = GetLength();
	int otherLen = MC_Strlen(aString);
	int newLen = oldLen + otherLen;
	if( newLen > 0 )
	{
		ReallocData( newLen );
		otherLen = MC_Min(otherLen, GetBufferSize() - oldLen - 1);		//make sure we dont write to far on static buffer in release candidate with asserts disabled
		C* pBuf = GetBuffer();
		for( int i=0; i<otherLen; i++ )
			pBuf[oldLen+i] = InternalConvertChar<C>( aString[i] );
		pBuf[oldLen + otherLen] = 0;
	}
	return *this;
}
template<class C, int S>
MC_Str<C,S>& MC_Str<C,S>::operator+=(const wchar_t* aString)
{
	int oldLen = GetLength();
	int otherLen = MC_Strlen(aString);
	int newLen = oldLen + otherLen;
	if( newLen > 0 )
	{
		ReallocData( newLen );
		otherLen = MC_Min(otherLen, GetBufferSize() - oldLen - 1);		//make sure we dont write to far on static buffer in release candidate with asserts disabled
		C* pBuf = GetBuffer();
		for( int i=0; i<otherLen; i++ )
			pBuf[oldLen+i] = InternalConvertChar<C>( aString[i] );
		pBuf[oldLen + otherLen] = 0;
	}
	return *this;
}
template<class C, int S>
MC_Str<C,S>& MC_Str<C,S>::operator+=(TCHAR aChar)
{
	int oldLen = GetLength();
	int newLen = oldLen + 1;
	if( newLen > 0 )
	{
		ReallocData( newLen );
		C* pBuf = GetBuffer();
		if(oldLen < GetBufferSize() - 1)		//make sure we dont write to far on static buffer in release candidate with asserts disabled
			pBuf[oldLen] = InternalConvertChar<C>( aChar );
	}
	return *this;
}
template<class C, int S>
MC_Str<C,S>& MC_Str<C,S>::operator+=(wchar_t aChar)
{
	int oldLen = GetLength();
	int newLen = oldLen + 1;
	if( newLen > 0 )
	{
		ReallocData( newLen );
		C* pBuf = GetBuffer();
		if(oldLen < GetBufferSize() - 1)		//make sure we dont write to far on static buffer in release candidate with asserts disabled
			pBuf[oldLen] = InternalConvertChar<C>( aChar );
	}
	return *this;
}

template<class C, int S>
MC_Str<C,S>& __cdecl MC_Str<C,S>::Format(const C* aFormatString, ...)
{
	va_list paramList;
	va_start(paramList, aFormatString);

	if(sizeof(C) == sizeof(TCHAR))
	{
		int b = _vscprintf((const TCHAR*)aFormatString, paramList);
		AllocData(b);
#if _MSC_VER >= 1400
		int stringLen =_vstprintf_s((TCHAR*)GetBuffer(), GetBufferSize(), (const TCHAR*)aFormatString, paramList);
#else
		int stringLen =_vstprintf((TCHAR*)GetBuffer(), (const TCHAR*)aFormatString, paramList);
#endif
		assert(b == stringLen);
	}
	else
	{
		assert(sizeof(wchar_t) == sizeof(C));

		int b = _vscwprintf((const wchar_t*)aFormatString, paramList);
		AllocData(b);

#if _MSC_VER >= 1400
		// using undocumented _vswprintf_c instead of vswprintf_s to avoid getting a runtime error when
		// buffer is too short (YES it is safe and the string gets truncated when buffer is too short)

		//int stringLen = vswprintf_s((wchar_t*)GetBuffer(), GetBufferSize(), (const wchar_t*)aFormatString, paramList);
		int stringLen = _vswprintf_c((wchar_t*)GetBuffer(), GetBufferSize(), (const wchar_t*)aFormatString, paramList);
#else
		int stringLen = vswprintf((wchar_t*)GetBuffer(), (const wchar_t*)aFormatString, paramList);
#endif
		assert(b == stringLen);
	}

	va_end(paramList);

	return *this;
}


/*****************************************************************************
 String utility functions
*****************************************************************************/

#define s2i(x)		MC_StringToInt(x)

int MC_StringToInt(const char* const aString);
int MC_StringToInt(const MC_LocChar* aString);

bool MC_WildCardCompareNoCase(const char* aString, const char* aWildcard);
bool MC_EndsWith(const char* aString, const char* aEndString);
bool MC_EndsWithNoCase(const char* aString, const char* aEndString);

MC_String MC_Strtok(const char** anInput, const char* aSeparator);

// Returns a pointer to the first occurrence of aSubString in aString, or NULL if aSubString does not appear in aString.
// If aSubString is NULL points to a string of zero length, the function returns aString.
const char* MC_Stristr(const char* aString, const char* aSubString);

// MC_Strfmt - convenience wrapper for sprintf
// use like this:	MC_String str = MC_Strfmt<32>("%f, %f", num1, num2); or just WhateverFunction(MC_Strfmt<>("%n says hi", name));
template <int BUFSIZE> struct MC_Strfmt
{
	MC_Strfmt(const char* aString, ...)
	{
		va_list paramList;
		va_start(paramList, aString);
#if _MSC_VER >= 1400
		vsnprintf_s(myBuffer, BUFSIZE, _TRUNCATE, aString, paramList);
#else
		_vsnprintf(myBuffer, BUFSIZE, aString, paramList);
#endif
		va_end(paramList);
	}
	
	operator const char* () const { return myBuffer; }

	char myBuffer[BUFSIZE];
};

inline MC_Strfmt<32> MC_Itoa(int i) { return MC_Strfmt<32>("%d", i); }
MC_Strfmt<64> MC_Ftoa(float f, bool aTrimFlag=false);


#endif MC_STRING_H
