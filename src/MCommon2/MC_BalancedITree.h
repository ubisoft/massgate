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
//Balanced binary itree, subclass of itree (stores instances)
//This tree does not balance automatically, an explicit call to Balance() must be made.
//Uses a vector object to balance itself.

#ifndef MC_BALANCEDITREE_H
#define MC_BALANCEDITREE_H

#include "mc_itree.h"
#include "mc_array.h"

template <class Type>
class MC_Array;


//balanced itree
template <class Type>
class MC_BalancedITree : public MC_ITree<Type>
{
public:

	//constructor
	MC_BalancedITree()
	{

	}

	//destructor
	~MC_BalancedITree()
	{

	}

	//copies the itree to a vector (does not sort)
	//uses a divide and conquer method to insert the objects into the itree to retain a balance
	bool Balance()
	{
		MC_Array<Type>* sortArray = NULL;;
		int count;
		int copyLength;

		//count
		count = Count();
		if(count)
		{
			//alloc sortvector
			sortArray = new MC_Array<Type>(count);
			assert(sortArray);
			if(sortArray)
			{
				//inorder traversal, extracting to the vector
				InorderExtract(sortArray, myRoot);

				//delete the old itree
				RemoveAll();

				//stuff the words back into the empty itree
				//using a recursive divide and conquer method
				copyLength = sortArray->Count();
				if(!RecursiveInsert(*sortArray, 0, copyLength - 1))
				{
					delete sortArray;
					return false;
				}

				delete sortArray;
				return true;
			}
		}

		return false;
	}

protected:

	//a divide and conquer style thing, low is here the first vector index, high the last
	virtual bool RecursiveInsert(MC_Array<Type>& aSortArray, const int aLowIndex, const int aHighIndex)
	{
		int mid;

		if(aLowIndex <= aHighIndex)
		{
			//insert current middle
			mid = (aLowIndex + aHighIndex) / 2;
			if(!Add(aSortArray[mid]))
				return false;

			//do aLowIndexer half
			if(!RecursiveInsert(aSortArray, aLowIndex, mid - 1))
				return false;

			//do upper half
			if(!RecursiveInsert(aSortArray, mid + 1, aHighIndex))
				return false;
		}

		return true;
	}

private:

	//insert into vector by inorder walk   
	void InorderExtract(MC_Array<Type>* aSortArray, MC_ITreeNode<Type>* aNode)
	{
		if(aNode->myLeftChild)
			InorderExtract(aSortArray, aNode->myLeftChild);

		aSortArray->Add(aNode->myData);

		if(aNode->myRightChild)
			InorderExtract(aSortArray, aNode->myRightChild);
	}
};

#endif
