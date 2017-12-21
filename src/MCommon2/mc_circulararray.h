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
//NOTE: All template classes have code inlined in their class declarations to reduce code expansion where not required
#ifndef MC_CIRCULARARRAY_H
#define MC_CIRCULARARRAY_H


//MC_CircularArrayT class
template <class Type, unsigned int MAX_NODES>
class MC_CircularArrayT
{
public:
	typedef Type value_t;
	const static unsigned int MAX = MAX_NODES;

	//constructor
	MC_CircularArrayT()
	{
		assert(MAX_NODES > 0); // must have at least one element in list
		myNextFreeNode = 0;
		myFirstNode = 0;
		myCurrentNumNodes = 0;
	}

	//destructor
	~MC_CircularArrayT()
	{
		RemoveAll();
	}

	bool RemoveFirst()
	{
		if (myCurrentNumNodes == 0)
			return false;
		myCurrentNumNodes--;
		myFirstNode++;
		if (myFirstNode == MAX_NODES)
			myFirstNode = 0;
		return true;
	}

	//insertion
	void Add(const Type& aType)
	{
		if(myCurrentNumNodes < MAX_NODES)
		{
			myCurrentNumNodes++;
		}
		else
		{
			if(myNextFreeNode == myFirstNode)
			{
				myFirstNode++;
				if(myFirstNode == MAX_NODES)
					myFirstNode = 0;
			}
		}

		myNodes[myNextFreeNode] = aType;
		myNextFreeNode++;  
		if(myNextFreeNode == MAX_NODES)
			myNextFreeNode = 0;
	}

	Type& First()
	{
		return (*this)[0];
	}

	Type& Last()
	{
		return (*this)[myCurrentNumNodes-1];
	}

	bool RemoveLast()
	{
		if(myCurrentNumNodes == 0)
			return false; // no stuff in list

		myCurrentNumNodes--;

		if(myNextFreeNode == 0)
			myNextFreeNode = MAX_NODES - 1;
		else
			myNextFreeNode--;

		return true;
	}

	//remove all
	void RemoveAll()
	{
		myNextFreeNode = 0;
		myFirstNode = 0;
		myCurrentNumNodes = 0;
	}

	int Find(const Type& anItem)
	{
		if (Count() == 0)
			return -1;

		for (unsigned int i = 0; i < myCurrentNumNodes; ++i)
		{
			if ((*this)[i] == anItem)
				return i;
		}
		return -1;
	}

	int ReverseFind(const Type& anItem)
	{
		for (int i = ((int)myCurrentNumNodes-1); i >= 0; --i)
		{
			if ((*this)[i] == anItem)
				return i;
		}
		return -1;
	}

	void RemoveAtIndex(unsigned int anIndex)
	{
		assert(anIndex < myCurrentNumNodes);
		if (Count() > 1)
		{
			for (int i = ((int)myCurrentNumNodes-1); i > (int)anIndex; --i)
			{
				(*this)[i-1] = (*this)[i];
			}
		}
		RemoveLast();
	}

	bool Full() const
	{
		return myCurrentNumNodes == MAX_NODES;
	}

	//used length
	int Count() const
	{
		return myCurrentNumNodes;
	}

	//element access
	Type& operator[](unsigned int anIndex)
	{
		assert(anIndex < (unsigned int)Count());
		return myNodes[(myFirstNode + anIndex) >= MAX_NODES ? myFirstNode + anIndex - MAX_NODES : myFirstNode + anIndex]; // CHANGE LATER??? (myFirstNode + anIndex) % MAX_NODES
	}

	const Type& operator[](unsigned int anIndex) const
	{
		assert(anIndex < (unsigned int)Count());
		return myNodes[(myFirstNode + anIndex) >= MAX_NODES ? myFirstNode + anIndex - MAX_NODES : myFirstNode + anIndex]; // CHANGE LATER??? (myFirstNode + anIndex) % MAX_NODES
	}

private:
	//members
	Type myNodes[MAX_NODES];
	unsigned int myCurrentNumNodes;
	unsigned int myNextFreeNode;
	unsigned int myFirstNode;
};

//MC_CircularArray class
template <class Type>
class MC_CircularArray
{
public:

	//constructor
	// Call this constructor in your own constructor code like this:
	// YourClass::YourClass( ) : yourArray(100) {  }
	MC_CircularArray(unsigned int aNumNodes)
	{
		assert(aNumNodes > 0); // must have at least one element in list

		myMaxNumNodes = aNumNodes;
		myNodes = new Type[myMaxNumNodes];
		assert(myNodes);
		myNextFreeNode = 0;
		myFirstNode = 0;
		myCurrentNumNodes = 0;
	}

	//destructor
	~MC_CircularArray()
	{
		RemoveAll();

		if(myNodes)
		{
			delete [] myNodes;
			myNodes = NULL;
		}
	}

	//insertion
	void Add(const Type& aType)
	{
		if(myCurrentNumNodes < myMaxNumNodes)
		{
			myCurrentNumNodes++;
		}
		else
		{
			if(myNextFreeNode == myFirstNode)
			{
				myFirstNode++;
				if(myFirstNode == myMaxNumNodes)
					myFirstNode = 0;
			}
		}

		myNodes[myNextFreeNode] = aType;
		myNextFreeNode++;  
		if(myNextFreeNode == myMaxNumNodes)
			myNextFreeNode = 0;
	}

	bool RemoveLast()
	{
		if(myCurrentNumNodes == 0)
			return false; // no stuff in list

		myCurrentNumNodes--;

		if(myNextFreeNode == 0)
			myNextFreeNode = myMaxNumNodes - 1;
		else
			myNextFreeNode--;

		return true;
	}

	//remove all
	void RemoveAll()
	{
		myNextFreeNode = 0;
		myFirstNode = 0;
		myCurrentNumNodes = 0;
	}

	//used length
	int Count() const
	{
		return myCurrentNumNodes;
	}

	//element access
	Type& operator[](unsigned int anIndex) const
	{
		assert(anIndex < (unsigned) Count());
		return myNodes[(myFirstNode + anIndex) >= myMaxNumNodes ? myFirstNode + anIndex - myMaxNumNodes : myFirstNode + anIndex]; // CHANGE LATER??? (myFirstNode + anIndex) % myMaxNumNodes
	}

//private:

	//illegal constructor
	MC_CircularArray()
	{
		assert(0);
	}

	//members
	Type* myNodes;
	unsigned int myMaxNumNodes;
	unsigned int myCurrentNumNodes;
	unsigned int myNextFreeNode;
	unsigned int myFirstNode;
};


#endif // MC_CIRCULARARRAY_H
