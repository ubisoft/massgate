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
#ifndef ___MC_HASH_MAP_INCLUDED___
#define ___MC_HASH_MAP_INCLUDED___

/* ----------[Example]---------- 

#include "mc_hashmap.h"
#include "mc_image.h"

// Example implementation of an image manager using MC_HashMap
class ExampleImageManager
{
public:
	ExampleImageManager() {}
	~ExampleImageManager()
	{
		// Clean up by deleting all images.
		// The HashMap itself does not need this, this is so we won't leak the MC_Images we loaded.
		myImageMap.DeleteAll();
	}

	MC_Image* Load(const char* aFilename)
	{
		MC_Image* img;

		// Get the image if it exists, and if so - just return it.
		if(myImageMap.GetIfExists(aFilename, img))
			return img;
		// Note, we need to use GetIfExists to check for the file, if we just did:
		// img = myImageMap[aFilename];
		// we would not know what we got, because the [] operator of the hash map
		// would have created a slot for aFilename if it didn't exist before!

		// The image didn't exist before, so create it.
		img = MC_Image::CreateImage(aFilename);
		if(!img)
			return NULL;

		// Add image to map, with the filename as it's key.
		// The HashMap has special handling for const char*'s, so you don't need to
		// worry about keeping track of the string yourself (aFilename can be a temporary).
		myImageMap[aFilename] = img;
		return img;
	}

	void DebugPrintAllImages()
	{
		// You can iterate over the items in a HashMap (this is not as fast as iterating over a regular array!)
		for(MC_HashMap<const char*, MC_Image*>::Iterator iter(myImageMap); iter; ++iter)
		{
			MC_Image* img = iter.Item();
			MC_DEBUG("Image: %dx%d", img->GetWidth(), img->GetHeight());
		}
	}

private:
	MC_HashMap<const char*, MC_Image*> myImageMap;
};

void ExampleUsageOfImageManager()
{
	ExampleImageManager mgr;

	// Load imageA.tga
	MC_Image* image1 = mgr.Load("imageA.tga");

	// Load imageB.tga
	MC_Image* image2 = mgr.Load("imageB.tga");

	// Load imageA.tga again (finds it in the hashmap - fast!)
	MC_Image* image3 = mgr.Load("imageA.tga");
}

----------[Example ends]---------- */


#include <string.h>

// Hash functions
void			MC_HashMap_HashData( const void* aData, int aByteCount, int aHashSize, unsigned int* aDest, unsigned int* aSeed=0 );
unsigned int	MC_HashMap_HashData( const void* aData, int aByteCount, unsigned int aSeed=0 );
unsigned int	MC_HashMap_HashString( const void* aData, unsigned int aSeed=0 );

// Type helpers
template<class T>	inline unsigned int MC_HashMap_GetHash(const T& anItem)							{ return MC_HashMap_HashData( (const char*)&anItem, sizeof(anItem) ); }
template<>			inline unsigned int MC_HashMap_GetHash(const char* const &anItem)				{ return MC_HashMap_HashString( anItem ); }

template<class T>	inline bool MC_HashMap_Compare(const T& aA, const T& aB)						{ return (aA == aB); }
template<>			inline bool MC_HashMap_Compare(const char* const &aA, const char* const &aB)	{ return (strcmp( aA, aB ) == 0); }

template<class T>	inline void MC_HashMap_Set( T& aDest, const T& aSrc )							{ aDest = aSrc; }
template<>			inline void MC_HashMap_Set( const char* &aDest, const char* const &aSrc )		{
																										delete [] aDest;
																										if( aSrc )
																										{
																											const size_t bufSize = strlen(aSrc) + 1;
																											aDest = new char[ bufSize ];
																											for( size_t i=0; i<bufSize; i++ )
																												((char*)aDest)[i] = aSrc[i];
																										}
																										else
																											aDest = 0;
																									}

template<class T>	inline void MC_HashMap_Clear(T& anItem)						{ }
template<>			inline void MC_HashMap_Clear(const char* &anItem)			{ anItem = 0; }

template<class T>	inline void MC_HashMap_Destroy(T& anItem)					{ }
template<>			inline void MC_HashMap_Destroy(const char* &anItem)			{ delete[] anItem; anItem = 0; }

// The hash map
template<class KEY, class ITEM, class DERIVED>
class MC_HashBase
{
	struct Entry
	{
		Entry()		{ myValidFlag = false; MC_HashMap_Clear(myKey); }
		~Entry()	{ MC_HashMap_Destroy(myKey); }

		ITEM	myItem;
		KEY		myKey;
		bool	myValidFlag;
	};

public:
	typedef KEY key_type;
	typedef ITEM item_type;

	class Iterator
	{
	public:
		Iterator() { myMap = 0; }
		Iterator(const Iterator& anOther) : myMap(anOther.myMap), myIndex(anOther.myIndex) {}
		Iterator( MC_HashBase& aMap ) : myMap( &aMap )	{ for( myIndex=0; myIndex<myMap->myArraySize && !myMap->myEntries[myIndex].myValidFlag; ++myIndex ) {} }
		Iterator( MC_HashBase* aMap ) : myMap( aMap )	{ for( myIndex=0; myIndex<myMap->myArraySize && !myMap->myEntries[myIndex].myValidFlag; ++myIndex ) {} }

		Iterator& operator=(const Iterator& anOther)
		{ myMap = anOther.myMap; myIndex = anOther.myIndex; return *this; }

		operator bool() const							{ return myMap && (myIndex < myMap->myArraySize); }
		const KEY&		Key() const						{ return myMap->myEntries[myIndex].myKey; }
		ITEM&			Item()							{ return myMap->myEntries[myIndex].myItem; }
		const ITEM&		Item() const					{ return myMap->myEntries[myIndex].myItem; }
		ITEM*			operator ->()					{ return &myMap->myEntries[myIndex].myItem; }
		const ITEM*		operator ->() const				{ return &myMap->myEntries[myIndex].myItem; }

		// ++ and -- operators returns void to avoid prefix/postfix problems.
		void	operator++()						{ for( ++myIndex; myIndex<myMap->myArraySize && !myMap->myEntries[myIndex].myValidFlag; ++myIndex ) {} }	// prefix
		void	operator++( int dummy )				{ ++*this; }		// postfix
		void	operator--()						{ for( --myIndex; myIndex<myMap->myArraySize && !myMap->myEntries[myIndex].myValidFlag; --myIndex ) {} }	// prefix
		void	operator--( int dummy )				{ --*this }			// postfix

	private:
		MC_HashBase*		myMap;
		unsigned int	myIndex;
	};

	Iterator Begin()
	{
		return Iterator(*this);
	}

	MC_HashBase( const int aStartSize)
	{
		myArraySize = aStartSize > 3 ? aStartSize : 3;
		myEntries = new Entry[myArraySize];
		myNumEntries = 0;
	}

	~MC_HashBase()
	{
		delete [] myEntries;
	}

	inline int Count() const
	{
		return int(myNumEntries);
	}

	ITEM& operator[] (const KEY& aKey)
	{
		unsigned int i;
		if( !GetIndex( aKey, i, myArraySize, myEntries ) )
		{
			// Grow if necessary
			if( myNumEntries > myArraySize/2 )
			{
				Resize( (myArraySize+1) * 2 - 1 );
				GetIndex( aKey, i, myArraySize, myEntries );
			}

			myEntries[i].myValidFlag = true;

			MC_HashMap_Set( myEntries[i].myKey, aKey );
			myNumEntries++;
		}

		return myEntries[i].myItem;
	}

	bool FindKeyByItem( const ITEM& anItem, KEY& aKey ) const
	{
		for( int i=0; i<myArraySize; i++ )
		{
			if(myEntries[i].myValidFlag)
			{
				if( MC_HashMap_Compare( myEntries[i].myItem, anItem ) )
				{
					aKey = myEntries[i].myKey;
					return true;
				}
			}
		}

		return false;
	}

	bool RemoveByKey( const KEY& aKey, bool aReallocOnShrink=true )
	{
		unsigned int i;
		if( GetIndex( aKey, i, myArraySize, myEntries ) )
		{
			RemoveAtIndex(i, aReallocOnShrink);
			return true;
		}
		return false;
	}

	bool PopByKey( const KEY& aKey, ITEM& anItem, bool aReallocOnShrink=true )
	{
		unsigned int i;
		if( GetIndex( aKey, i, myArraySize, myEntries ) )
		{
			anItem = myEntries[i].myItem;
			RemoveAtIndex(i, aReallocOnShrink);
			return true;
		}
		return false;
	}

	// Removes all entries with the specified item
	int RemoveByItem( const ITEM& anItem, bool aReallocOnShrink=true )
	{
		int count = 0;
		for( unsigned int i=0; i<myArraySize; i++ )
		{
			if(myEntries[i].myValidFlag)
			{
				if( MC_HashMap_Compare( myEntries[i].myItem, anItem ) )
				{
					RemoveAtIndex(i, aReallocOnShrink);
					count++;
					i = -1;		// start over (the for loop will increment this to 0)
				}
			}
		}
		return count;
	}

	void RemoveAll(bool aShrinkBufferFlag = true)
	{
		for(unsigned int i=0; i<myArraySize; i++)
		{
			if(myEntries[i].myValidFlag)
			{
				myEntries[i].myValidFlag = false;
				MC_HashMap_Destroy(myEntries[i].myKey);
			}
		}

		myNumEntries = 0;

		// Only shrink if it's worth while and if we want to
		if(aShrinkBufferFlag && myArraySize > 6)
		{
			delete [] myEntries;
			myArraySize = 3;
			myEntries = new Entry[myArraySize];
		}
	}

	void DeleteAll(bool aShrinkBufferFlag = true)
	{
		for(unsigned int i=0; i<myArraySize; i++)
		{
			if(myEntries[i].myValidFlag)
			{
				myEntries[i].myValidFlag = false;
				delete myEntries[i].myItem;
				myEntries[i].myItem = NULL;
				MC_HashMap_Destroy(myEntries[i].myKey);
			}
		}

		myNumEntries = 0;

		// Only shrink if it's worth while and if we want to
		if(aShrinkBufferFlag && myArraySize > 6)
		{
			delete [] myEntries;
			myArraySize = 3;
			myEntries = new Entry[myArraySize];
		}
	}

	bool Exists(const KEY& aKey) const
	{
		unsigned int i;
		return GetIndex( aKey, i, myArraySize, myEntries );
	}

	bool GetIfExists(const KEY& aKey, ITEM& anItem) const
	{
		unsigned int i;
		if( GetIndex( aKey, i, myArraySize, myEntries ) )
		{
			anItem = myEntries[i].myItem;
			return true;
		}
		else
			return false;
	}

	ITEM* GetIfExists(const KEY& aKey) const
	{
		unsigned int i;
		if( GetIndex( aKey, i, myArraySize, myEntries ) )
			return &(myEntries[i].myItem);
		else
			return 0;
	}

	//this function is slow and should be avoided if possible
	KEY* GetKeyByIndexNumber(int aIndex) const
	{
		int numFound = 0;
		for(unsigned int a = 0; a < myArraySize ; ++a)
		{
			if(myEntries[a].myValidFlag)
			{
				++numFound;
				if(numFound > aIndex)
					return &myEntries[a].myKey;
			}
		}
		return 0;
	}

private:
	bool GetIndex( const KEY& aKey, unsigned int& i, unsigned int maxEntries, Entry* pEntries ) const
	{
		bool bExists = false;

		i = ((DERIVED*)this)->GetHash(aKey) % maxEntries;
		int starti = i;
		
		while( pEntries[i].myValidFlag )
		{
			if( MC_HashMap_Compare(aKey, pEntries[i].myKey) )
				return true;

			i = (i+1) % maxEntries;

			if (i == starti)
				return false;
		}

		return false;
	}

	void RemoveAtIndex(unsigned int anIndex, bool aReallocOnShrink)
	{
		myNumEntries--;
		assert(myEntries[anIndex].myValidFlag);
		myEntries[anIndex].myValidFlag = false;
		MC_HashMap_Destroy(myEntries[anIndex].myKey);

		// Shrink if necessary (and if we want to)
		if( aReallocOnShrink && myNumEntries < myArraySize/5 )
		{
			Resize( ((myArraySize+1) / 2) - 1 );
		}
		else
		{
			// Fill hole that might occur
			unsigned int i = anIndex;
			// Start at the next item in the array, and loop until the first hole
			for( i = (i+1) % myArraySize; myEntries[i].myValidFlag; i = (i+1) % myArraySize )
			{
				// Calc the index that this items wants to reside in
				unsigned int j = ((DERIVED*)this)->GetHash( myEntries[i].myKey ) % myArraySize;

				// Run j util we find the first hole
				while( i != j && myEntries[j].myValidFlag )
				{
					j = (j+1) % myArraySize;
				}

				if( i != j )
				{
					myEntries[j].myKey = myEntries[i].myKey;
					myEntries[j].myItem = myEntries[i].myItem;
					myEntries[j].myValidFlag = true;
					MC_HashMap_Clear(myEntries[i].myKey);
					myEntries[i].myValidFlag = false;
				}
			}
		}
	}

	void Resize( unsigned int aNewMaxSize )
	{
		Entry* pNewEntries = new Entry[aNewMaxSize];

		for( unsigned int readIndex=0; readIndex<myArraySize; readIndex++ )
		{
			if( myEntries[readIndex].myValidFlag )
			{
				unsigned int i;
				GetIndex( myEntries[readIndex].myKey, i, aNewMaxSize, pNewEntries );
				pNewEntries[i].myKey = myEntries[readIndex].myKey;
				pNewEntries[i].myItem = myEntries[readIndex].myItem;
				pNewEntries[i].myValidFlag = true;
				MC_HashMap_Clear(myEntries[readIndex].myKey);
			}
		}

		delete [] myEntries;
		myEntries = pNewEntries;
		myArraySize = aNewMaxSize;
	}

	Entry*			myEntries;
	unsigned int	myArraySize;
	unsigned int	myNumEntries;
};

template<class KEY, class ITEM>
class MC_HashMap : public MC_HashBase<KEY, ITEM, MC_HashMap<KEY, ITEM> >
{
public:
	typedef typename MC_HashBase<KEY, ITEM, MC_HashMap<KEY, ITEM> > BASE;

	friend class BASE;

	explicit MC_HashMap(const int aStartSize = 7 )
		: BASE(aStartSize)
	{}

private:
	// Overriden from MC_HashBase
	unsigned int GetHash(const KEY& aKey)
	{
		return MC_HashMap_GetHash<KEY>( aKey );
	}
};

template<class ITEM>
class MC_HashSet : public MC_HashBase<unsigned int, ITEM, MC_HashSet<ITEM> >
{
public:
	typedef typename MC_HashBase<unsigned int, ITEM, MC_HashSet<ITEM> > BASE;

	friend class BASE;

	explicit MC_HashSet(const int aStartSize = 7 )
		: BASE(aStartSize)
	{}

private:
	// Overriden from MC_HashBase
	unsigned int GetHash(const key_type& aKey)
	{
		return aKey;
	}
};

#endif //___MC_HASH_MAP_INCLUDED___

