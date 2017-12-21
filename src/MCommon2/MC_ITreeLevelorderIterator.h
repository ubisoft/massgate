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
//class MC_ITreeLevelorderIterator

//implements iterator protocol for itrees (level order traversal)


#ifndef MC_ITREELEVELORDERITERATOR_H
#define MC_ITREELEVELORDERITERATOR_H

#include "mc_iterator.h"

template <class Type>
class MC_ITreeLevelorderIterator : public MC_Iterator<Type>
{
public:

	//constructor
	MC_ITreeLevelorderIterator(MC_ITree<Type>& aTree)
	{
		myTree = &aTree;
	}

	//destructor
	~MC_ITreeLevelorderIterator()
	{

	}

	//iterator protocol
	bool Start()
	{
		MC_ITreeNode<Type>* root = myTree->GetRoot();
		
		//initialize a levelorder traversal of a itree
		//first initialize the list
		myQueue.Clear();

		//first time out, just enqueue the root
		if(root)
			myQueue.AddLast(root);

		return !myQueue.IsEmpty();
	}

	bool AtEnd()
	{
		//we are done when queue is empty
		return myQueue.IsEmpty();
	}

	Type Get()
	{
		//return value of current node
		return GetNode()->myData;
	}

	bool Next()
	{
		MC_ITreeNode<Type>* node;
		MC_ITreeNode<Type>* next;

		//queue children of current node
		if(!myQueue.IsEmpty())
		{
			node = myQueue.GetFirst();
			myQueue.RemoveFirst();
			
			next = node->myLeftChild;

			if(next) 
				myQueue.AddLast(next);

			next = node->myRightChild;

			if(next) 
				myQueue.AddLast(next);
		}

		return !myQueue.IsEmpty();
	}

	void Set(const Type& aNewValue)
	{
		MC_ITreeNode<Type>* node;

		if(!myQueue.IsEmpty())
		{
			//change the current value
			node = myQueue.GetFirst();
			node->myData = aNewValue;
		}
	}

	//access to the current node itself
	MC_ITreeNode<Type>* GetNode()
	{
		//return value of current node
		return myQueue.GetFirst();
	}

protected:

	//members
	MC_ITree<Type>* myTree;
	MC_List<MC_ITreeNode<Type>*> myQueue;
};


/*
//IMPLEMENTATION
//constructor
template <class Type>
MC_ITreeLevelorderIterator<Type>::MC_ITreeLevelorderIterator(MC_ITree<Type>& aTree)
{
	myTree = &aTree;
}


//destructor
template <class Type>
MC_ITreeLevelorderIterator<Type>::~MC_ITreeLevelorderIterator()
{

}


//init
template <class Type>
bool MC_ITreeLevelorderIterator<Type>::Start()
{
	MC_ITreeNode<Type>* root = myTree->GetRoot();
	
	//initialize a levelorder traversal of a itree
	//first initialize the list
	myQueue.Clear();

	//first time out, just enqueue the root
	if(root)
		myQueue.AddLast(root);

	return !myQueue.IsEmpty();
}


//test for end of list
template <class Type>
bool MC_ITreeLevelorderIterator<Type>::AtEnd()
{
	//we are done when queue is empty
	return myQueue.IsEmpty();
}


//return copy of current element
template <class Type>
Type MC_ITreeLevelorderIterator<Type>::Get()
{
	//return value of current node
	return GetNode()->myData;
}


//access the current node itself
template <class Type>
MC_ITreeNode<Type>* MC_ITreeLevelorderIterator<Type>::GetNode()
{
	//return value of current node
	return myQueue.GetFirst();
}


//advance to next element
template <class Type>
bool MC_ITreeLevelorderIterator<Type>::Next()
{
	MC_ITreeNode<Type>* node;
	MC_ITreeNode<Type>* next;

	//queue children of current node
	if(!myQueue.IsEmpty())
	{
		node = myQueue.GetFirst();
		myQueue.RemoveFirst();
		
		next = node->myLeftChild;

		if(next) 
			myQueue.AddLast(next);

		next = node->myRightChild;

		if(next) 
			myQueue.AddLast(next);
	}

	return !myQueue.IsEmpty();
}


//change current element
template <class Type>
void MC_ITreeLevelorderIterator<Type>::Set(const Type& aNewValue)
{
	MC_ITreeNode<Type>* node;

	if(!myQueue.IsEmpty())
	{
		//change the current value
		node = myQueue.GetFirst();
		node->myData = aNewValue;
	}
}
*/

#endif
