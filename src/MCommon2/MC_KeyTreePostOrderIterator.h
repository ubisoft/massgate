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
//class MC_KeyTreePostorderIterator

//implements iterator protocol for keytrees (postorder traversal)


#ifndef MC_KEYTREEPOSTORDERITERATOR_H
#define MC_KEYTREEPOSTORDERITERATOR_H

#include "mc_iterator.h"
#include "mc_itree.h"
#include "mc_stacklist.h"


template <class Type, class Key>
class MC_KeyTreePostorderIterator : public MC_Iterator<Type>
{
public:

	//constructor
	MC_KeyTreePostorderIterator(MC_KeyTree<Type, Key>& aTree)
	{
		myTree = &aTree;
	}

	//destructor
	~MC_KeyTreePostorderIterator()
	{

	}

	//iterator protocol
	bool Start()
	{
		//initialize inorder itree traversal
		//clear out the stack
		myStack.RemoveAll();

		//then reinitialize it if the tree isn't empty
		if(myTree->Count())
		{
			StackChildren(myTree->GetRoot()); 

			return !myStack.IsEmpty(); 
		}
		else
		{
			return true;
		}
	}

	bool AtEnd()
	{
		//we are done when stack is empty
		return myStack.IsEmpty();
	}

	Type Get()
	{
		//return value of current node
		return GetNode()->myData;
	}

	bool Next()
	{
		//move to the next item in sequence
		//pop current node from stack
		myStack.Pop();

		//return false if stack is empty, and hence there are no more items
		return !myStack.IsEmpty();
	}

	//access the current node itself
	MC_KeyTreeNode<Type, Key>* GetNode()
	{
		//return value of current node
		return myStack.GetTop();
	}

protected:

    //internal method used to stack children of a node
    void StackChildren(MC_KeyTreeNode<Type, Key>* aNode)
	{
		MC_KeyTreeNode<Type, Key>* next;

		//stack all the children of the current node
		myStack.Push(aNode);

		next = aNode->myRightChild;
		if(next) 
			StackChildren(next);

		next = aNode->myLeftChild;
		if(next) 
			StackChildren(next);
	}

	//members
	MC_KeyTree<Type, Key>* myTree;
	MC_StackList<MC_KeyTreeNode<Type, Key>*> myStack;
};

#endif
