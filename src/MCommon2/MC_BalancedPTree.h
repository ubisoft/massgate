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
//Balanced binary tree, subclass of ptree (stores pointerse to instance)
//This tree does not balance automatically, an explicit call to Balance() must be made.
//Uses a vector object to balance itself.

#ifndef MC_BALANCEDPTREE_H
#define MC_BALANCEDPTREE_H

#include "mc_ptree.h"
#include "mc_array.h"

template <class Type>
class MC_Array;


//balanced ptree
template <class Type>
class MC_BalancedPTree : public MC_PTree<Type>
{
public:

	//constructor
	MC_BalancedPTree()
	{

	}

	//destructor
	~MC_BalancedPTree()
	{

	}

	//copies the ptree to a vector (does not sort)
	//uses a divide and conquer method to insert the objects into the ptree to retain a balance
	bool Balance()
	{
		MC_Array<Type*>* sortArray = NULL;
		int count;
		int copyLength;

		//count
		count = Count();
		if(count)
		{
			//alloc sortvector
			sortArray = new MC_Array<Type*>(count);
			assert(sortArray);
			if(sortArray)
			{
				//inorder traversal, extracting to the vector
				InorderExtract(sortArray, myRoot);

				//delete the old ptree
				RemoveAll();

				//stuff the words back into the empty ptree
				//using a recursive divide and conquer method
				copyLength = sortArray->Count();
				RecursiveInsert(sortArray, 0, copyLength - 1);

				//delete vector
				delete sortArray;
				sortArray = NULL;

				return true;
			}
		}

		return false;
	}

private:

	//insert into vector by inorder walk   
	void InorderExtract(MC_Array<Type*>* aSortArray, MC_PTreeNode<Type>* aNode)
	{
		if(aNode->myLeftChild)
			InorderExtract(aSortArray, aNode->myLeftChild);

		aSortArray->Add(aNode->myData);

		if(aNode->myRightChild)
			InorderExtract(aSortArray, aNode->myRightChild);
	}

	//a divide and conquer style thing, low is here the first vector index, high the last
	void RecursiveInsert(MC_Array<Type*>* aSortArray, int aLowIndex, int aHighIndex)
	{
		int mid;

		if(aLowIndex <= aHighIndex)
		{
			//insert current middle
			mid = (aLowIndex + aHighIndex) / 2;
			Add(aSortArray->operator[](mid));

			//do aLowIndexer half
			RecursiveInsert(aSortArray, aLowIndex, mid - 1);

			//do upper half
			RecursiveInsert(aSortArray, mid + 1, aHighIndex);
		}
	}
};


/*
//IMPLEMENTATION
//constructor
template <class Type>
MC_BalancedPTree<Type>::MC_BalancedPTree()
{

}


//destructor
template <class Type>
MC_BalancedPTree<Type>::~MC_BalancedPTree()
{

}


//copies the ptree to a vector but does NOT quicksort.
//Instead uses a divide and conquer method to insert the objects into the ptree to retain a balance
template <class Type>
bool MC_BalancedPTree<Type>::Balance()
{
	MC_Array<Type*>* sortArray = NULL;;
	int count;
	int copyLength;

	//count
	count = Count();
	if(count)
	{
		//alloc sortvector
		sortArray = new MC_Array<Type*>(count);
		assert(sortArray);
		if(sortArray)
		{
			//inorder traversal, extracting to the vector
			InorderExtract(sortArray, myRoot);

			//delete the old ptree
			Clear();

			//stuff the words back into the empty ptree
			//using a recursive divide and conquer method
			copyLength = sortArray->Length();
			RecursiveInsert(sortArray, 0, copyLength - 1);

			//delete vector
			delete sortArray;
			sortArray = NULL;

			return true;
		}
	}

	return false;
}


//insert into vector by inorder walk   
template <class Type>
void MC_BalancedPTree<Type>::InorderExtract(MC_Array<Type*>* aSortArray, MC_PTreeNode<Type>* aNode)
{
	if(aNode->myLeftChild)
		InorderExtract(aSortArray, aNode->myLeftChild);

	aSortArray->Insert(aNode->myData);

	if(aNode->myRightChild)
		InorderExtract(aSortArray, aNode->myRightChild);
}


//a divide and conquer style thing, low is here the first vector index, high the last
template <class Type>
void MC_BalancedPTree<Type>::RecursiveInsert(MC_Array<Type*>* aSortArray, int aLowIndex, int aHighIndex)
{
	int mid;

	if(aLowIndex <= aHighIndex)
	{
		//insert current middle
		mid = (aLowIndex + aHighIndex) / 2;
		Insert(aSortArray->operator[](mid));

		//do aLowIndexer half
		RecursiveInsert(aSortArray, aLowIndex, mid - 1);

		//do upper half
		RecursiveInsert(aSortArray, mid + 1, aHighIndex);
	}
}
*/

#endif
