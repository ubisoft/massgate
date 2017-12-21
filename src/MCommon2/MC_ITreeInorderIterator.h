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
//class MC_ITreeInorderIterator

//implements iterator protocol for itrees (inorder traversal)


#ifndef MC_ITREEINORDERITERATOR_H
#define MC_ITREEINORDERITERATOR_H

#include "mc_iterator.h"
#include "mc_itree.h"
#include "mc_stacklist.h"


template <class Type>
class MC_ITreeInorderIterator : public MC_Iterator<Type>
{
public:

	//constructor
	MC_ITreeInorderIterator()
	{
		myTree = NULL;
	}

	//constructor, set tree explicitly
	MC_ITreeInorderIterator(MC_ITree<Type>& aTree)
	{
		SetTree(aTree);
	}

	//destructor
	~MC_ITreeInorderIterator()
	{

	}

	//set tree
	bool SetTree(MC_ITree<Type>& aTree)
	{
		myTree = &aTree;
		assert(myTree);

		return (myTree != NULL);
	}

	//init
	bool Start()
	{
		//initialize inorder itree traversal
		//clear out the stack
		myStack.RemoveAll();

		//then reinitialize it
		SlideLeft(myTree->GetRoot()); 

		return !myStack.IsEmpty(); 
	}

	//test for end of list
	bool AtEnd()
	{
		//we are done when stack is empty
		return myStack.IsEmpty();
	}

	//return copy of current element
	Type Get()
	{
		//return value of current node
		return GetNode()->myData;
	}

	//access the current node itself
	MC_ITreeNode<Type>* GetNode()
	{
		//return value of current node
		return myStack.GetTop();
	}

	//advance to next element
	bool Next()
	{
		MC_ITreeNode<Type>* node;
		MC_ITreeNode<Type>* next;

		//inv - for each node on stack, left children
		//have been explored, right children have not
		if(!myStack.IsEmpty())
		{
			node = myStack.Pop();
			next = node->myRightChild;

			if(next) 
				SlideLeft(next);
		}

		//if stack isn't empty we have nodes
		return !myStack.IsEmpty();
	}

	//change current element
	void Set(const Type& aNewValue)
	{
		MC_ITreeNode<Type>* node;

		if(!myStack.IsEmpty())
		{
			//change the current value
			node = myStack.GetTop();
			node->myData = aNewValue;
		}
	}

protected:

	//utility
    void SlideLeft(MC_ITreeNode<Type>* aNode)
	{
		//slide left from the current node
		while(aNode)
		{
			myStack.Push(aNode);
			aNode = aNode->myLeftChild;
		}
	}

	//members
	MC_ITree<Type>* myTree;
	MC_StackList<MC_ITreeNode<Type>*> myStack;
};

#endif
