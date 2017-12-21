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
//class MC_ITreePreorderIterator

//implements iterator protocol for itrees (preorder traversal)


#ifndef MC_ITREEPREORDERITERATOR_H
#define MC_ITREEPREORDERITERATOR_H

template <class Type>
class MC_ITreePreorderIterator : public MC_Iterator<Type>
{
public:

	//constructor
	MC_ITreePreorderIterator(MC_ITree<Type>& aTree)
	{
		myTree = &aTree;
	}

	//destructor
	~MC_ITreePreorderIterator()
	{

	}

	//iterator protocol
	bool Start()
	{
		//initialize a preorder traversal of a itree
		//first initialize the stack
		myMC_Stack.Clear();

		//first time out, just push the root
		if(myTree->GetRoot())
			myMC_Stack.Push(myTree->GetRoot());

		return !myMC_Stack.IsEmpty();
	}

	bool AtEnd()
	{
		//we are done when stack is empty
		return myMC_Stack.IsEmpty();
	}

	Type Get()
	{
		//return value of current node
		return GetNode()->myData;
	}

	void Set(const Type& aNewValue)
	{
		MC_ITreeNode<Type>* node;

		if(!myMC_Stack.IsEmpty())
		{
			//change the current value
			node = myMC_Stack.GetTop();
			node->myData = aNewValue;
		}
	}

	bool Next()
	{
		//invariant - for each node on stack except top
		//left children are being investigated
		//for all including top no right children have been visited
		MC_ITreeNode<Type>* current = myMC_Stack.GetTop();
		MC_ITreeNode<Type>* next = current->myLeftChild;

		if(next)
		{
			myMC_Stack.Push(next);
			return true;
		}

		//no more left nodes, 
		//find unexplored right child
		while(!myMC_Stack.IsEmpty())
		{
			current = myMC_Stack.Pop();
			next = current->myRightChild;

			if(next)
			{
				//note parent is not on stack,
				//preserving invariant
				myMC_Stack.Push(next);
				return true;
			}
		}

		return false;
	}

	//access the current node itself
	MC_ITreeNode<Type>* GetNode()
	{
		//return value of current node
		return myMC_Stack.GetTop();
	}

private:

	MC_ITree<Type>* myTree;
	MC_StackList<MC_ITreeNode<Type>*> myMC_Stack;
};


/*
//IMPLEMENTATION
//constructor
template <class Type>
MC_ITreePreorderIterator<Type>::MC_ITreePreorderIterator(MC_ITree<Type>& aTree)
{
	myTree = &aTree;
}


//destructor
template <class Type>
MC_ITreePreorderIterator<Type>::~MC_ITreePreorderIterator()
{

}


//init
template <class Type>
bool MC_ITreePreorderIterator<Type>::Start()
{
	//initialize a preorder traversal of a itree
	//first initialize the stack
	myMC_Stack.Clear();

	//first time out, just push the root
	if(myTree->GetRoot())
		myMC_Stack.Push(myTree->GetRoot());

	return !myMC_Stack.IsEmpty();
}


template <class Type>
bool MC_ITreePreorderIterator<Type>::AtEnd()
{
	//we are done when stack is empty
	return myMC_Stack.IsEmpty();
}


//return copy of current element
template <class Type>
Type MC_ITreePreorderIterator<Type>::Get()
{
	//return value of current node
	return GetNode()->myData;
}


//access the current node itself
template <class Type>
MC_ITreeNode<Type>* MC_ITreePreorderIterator<Type>::GetNode()
{
	//return value of current node
	return myMC_Stack.GetTop();
}


template <class Type>
void MC_ITreePreorderIterator<Type>::Set(const Type& aNewValue)
{
	MC_ITreeNode<Type>* node;

	if(!myMC_Stack.IsEmpty())
	{
		//change the current value
		node = myMC_Stack.GetTop();
		node->myData = aNewValue;
	}
}


template <class Type>
bool MC_ITreePreorderIterator<Type>::Next()
{
	//invariant - for each node on stack except top
	//left children are being investigated
	//for all including top no right children have been visited
	MC_ITreeNode<Type>* current = myMC_Stack.GetTop();
	MC_ITreeNode<Type>* next = current->myLeftChild;

	if(next)
	{
		myMC_Stack.Push(next);
		return true;
	}

	//no more left nodes, 
	//find unexplored right child
	while(!myMC_Stack.IsEmpty())
	{
		current = myMC_Stack.Pop();
		next = current->myRightChild;

		if(next)
		{
			//note parent is not on stack,
			//preserving invariant
			myMC_Stack.Push(next);
			return true;
		}
	}

	return false;
}
*/

#endif
