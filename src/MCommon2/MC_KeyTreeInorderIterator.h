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
//class MC_KeyTreeInorderIterator

//implements iterator protocol for keytrees (inorder traversal)


#ifndef MC_KEYTREEINORDERITERATOR_H
#define MC_KEYTREEINORDERITERATOR_H

#include "mc_iterator.h"
#include "mc_keytree.h"
#include "mc_stacklist.h"


template <class Type, class Key>
class MC_KeyTreeInorderIterator : public MC_Iterator<Type>
{
public:

	//constructor
	MC_KeyTreeInorderIterator(MC_KeyTree<Type, Key>& aTree)
	{
		myTree = &aTree;
	}

	//destructor
	~MC_KeyTreeInorderIterator()
	{

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
	MC_KeyTreeNode<Type, Key>* GetNode()
	{
		//return value of current node
		return myStack.GetTop();
	}

	//advance to next element
	bool Next()
	{
		MC_KeyTreeNode<Type, Key>* node;
		MC_KeyTreeNode<Type, Key>* next;

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
	void Set(Type aNewValue)
	{
		MC_KeyTreeNode<Type, Key>* node;

		if(!myStack.IsEmpty())
		{
			//change the current value
			node = myStack.GetTop();
			node->myData = aNewValue;
		}
	}

protected:

	//utility
    void SlideLeft(MC_KeyTreeNode<Type, Key>* aNode)
	{
		//slide left from the current node
		while(aNode)
		{
			myStack.Push(aNode);
			aNode = aNode->myLeftChild;
		}
	}

	//members
	MC_KeyTree<Type, Key>* myTree;
	MC_StackList<MC_KeyTreeNode<Type, Key>*> myStack;
};

#endif
