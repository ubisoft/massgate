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

#ifndef MC_ARRAY_H
#define MC_ARRAY_H


//MC_Array class
template <class Type>
class MC_Array
{
public:

	//constructor
	// Call this constructor in your own constructor code like this:
	// YourClass::YourClass( ) : yourArray(100) {  }
	MC_Array(unsigned int aNumNodes)
	{
		myNumNodes = aNumNodes;
		myNodes = new Type[myNumNodes];
		assert(myNodes);
		myNextFreeNode = 0;
	}

	//destructor
	~MC_Array()
	{
		RemoveAll();

		if(myNodes)
		{
			delete [] myNodes;
			myNodes = NULL;
		}
	}

	//add
	bool Add(const Type& aType)
	{
		if(myNextFreeNode < myNumNodes)
		{
			myNodes[myNextFreeNode] = aType;
			myNextFreeNode++;  

			return true;
		}

		return false;
	}

	bool AddNoDuplicates(const Type& aType)
	{
		unsigned int n;
		
		if(myNextFreeNode < myNumNodes)
		{
			//search for node of same value
			for(n=0;n<myNextFreeNode;n++)
			{
				if(myNodes[n] == aType)
				{
					return false;
				}
			}
			
			//assign
			myNodes[myNextFreeNode] = aType;

			//increase index of next free node
			myNextFreeNode++;  

			return true;
		}

		return false;
	}

	//searching
	//TODO: Change this interface to BinarySearch instead!
	bool BinarySearch(const Type& aType) const
	{
		unsigned int low = 0;
		unsigned int mid;
		unsigned int high = Length();
		bool found = false;

		while((low < high) && (!found))
		{
			mid = (low + high) / 2;
			
			if(myNodes[mid] < aType)
			{
				low = mid + 1;
			}
			else if(myNodes[mid] > aType)
			{
				high = mid;
			}
			else
			{
				assert(myNodes[mid] == aType);
				found = true;
			}
		}

		return found;
	}

	// Finding (returns -1 if not found)
	int Find(const Type& aType) const
	{
		for (int i = 0; i < Count(); ++i)
			if (myNodes[i] == aType)
				return i;
		return -1;
	}

/*	//empty check
	bool IsEmpty() const
	{
		return (myNextFreeNode == 0);
	}*/

	//clear
	void RemoveAll()
	{
		if(Count())
			myNextFreeNode = 0;
	}

	//used length
	int Count() const
	{
		return myNextFreeNode;
	}

	//element access
	Type& operator[](unsigned int anIndex) const
	{
		assert(anIndex < myNumNodes);
		return myNodes[anIndex];
	}

	//sort (using quicksort internally)
	//NOTE: use of this function requires that the operators >, <, and == defined are defined for the stored Type
	//NOTE: be especially careful when storing pointers, as the default operators will compare addresses, not the values of the instances pointed at
	void Sort(bool aSortDescendingFlag = false)
	{
		if( myNextFreeNode != 0 )
			QuickSort(0, myNextFreeNode - 1, aSortDescendingFlag);
	}

	void SetMaxNumUsedItems(unsigned int aCount)
	{
		assert(aCount <= myNumNodes);

		myNextFreeNode = aCount;
	}

	Type* GetBuffer()
	{
		return myNodes;
	}

	const Type* GetBuffer() const
	{
		return myNodes;
	}

private:

	//illegal constructor
	MC_Array()
	{
		assert(0);
	}
	
	//quick sort function, low is the low end index, and high is the high end index to sort between
	void QuickSort(unsigned int aLowIndex, unsigned int aHighIndex, bool aSortDescendingFlag)
	{
		unsigned int pivot;

		if(aLowIndex > aHighIndex)
			return;
		else
		{
			pivot = (aLowIndex + aHighIndex) / 2;
			pivot = Split(aLowIndex, pivot, aHighIndex, aSortDescendingFlag);

			if(aLowIndex < pivot)
				QuickSort(aLowIndex, pivot - 1, aSortDescendingFlag);
			
			if(pivot < aHighIndex) 
				QuickSort(pivot + 1, aHighIndex, aSortDescendingFlag);
		}
	}
	
	//split function, used by quicksort
	//NOTE: use of this function requires that the operators >, <, and == defined are defined for the stored Type
	//NOTE: be especially careful when storing pointers, as the default operators will compare addresses, not the values of the instances pointed at
	unsigned int Split(unsigned int aLowIndex, unsigned int aPivotIndex, unsigned int aHighIndex, bool aSortDescendingFlag)
	{
		Type tempType;

		if(aPivotIndex != aLowIndex)
		{
			//swap
			tempType = myNodes[aLowIndex];
			myNodes[aLowIndex] = myNodes[aPivotIndex];
			myNodes[aPivotIndex] = tempType;
		}

		aPivotIndex = aLowIndex;
		aLowIndex++;

		while(aLowIndex <= aHighIndex)
		{
			if(!aSortDescendingFlag)
			{
				if((myNodes[aLowIndex] < myNodes[aPivotIndex]) || (myNodes[aLowIndex] == myNodes[aPivotIndex]))
				{
					aLowIndex++;
				}
				else if(myNodes[aHighIndex] > myNodes[aPivotIndex])
				{
					aHighIndex--;
				}
				else
				{
					//swap
					tempType = myNodes[aLowIndex];
					myNodes[aLowIndex] = myNodes[aHighIndex];
					myNodes[aHighIndex] = tempType;
				}
			}
			else
			{
				if((myNodes[aLowIndex] > myNodes[aPivotIndex]) || (myNodes[aLowIndex] == myNodes[aPivotIndex]))
				{
					aLowIndex++;
				}
				else if(myNodes[aHighIndex] < myNodes[aPivotIndex])
				{
					aHighIndex--;
				}
				else
				{
					//swap
					tempType = myNodes[aLowIndex];
					myNodes[aLowIndex] = myNodes[aHighIndex];
					myNodes[aHighIndex] = tempType;
				}
			}
		}

		if(aHighIndex != aPivotIndex)
		{
			//swap
			tempType = myNodes[aPivotIndex];
			myNodes[aPivotIndex] = myNodes[aHighIndex];
			myNodes[aHighIndex] = tempType;
		}

		return aHighIndex;
	}

	//members
	Type* myNodes;
	unsigned int myNumNodes;
	unsigned int myNextFreeNode;
};

#endif
