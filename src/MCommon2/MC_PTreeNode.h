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
//mc_itreenode.h
//tree node class for storing pointers to instances


#ifndef MC_PTREENODE_H
#define MC_PTREENODE_H


//tree node class
template <class Type>
class MC_PTreeNode
{
public:

	//constructor
	MC_PTreeNode(Type* aPointer)
	{
		myData = aPointer;
		myLeftChild = NULL;
		myRightChild = NULL;
	}

	//destructor
	~MC_PTreeNode()
	{
		//shouldn't delete myData
	}

	//members
	Type* myData;
	MC_PTreeNode<Type>* myLeftChild;
	MC_PTreeNode<Type>* myRightChild;

private:

	//illegal constructor
	MC_PTreeNode()
	{
		assert(0);
	}
};



/*
//IMPLEMENTATION
//constructor
template <class Type>
MC_PTreeNode<Type>::MC_PTreeNode(Type* aPointer)
{
	myData = aPointer;
	myLeftChild = NULL;
	myRightChild = NULL;
}


//destructor
template <class Type>
MC_PTreeNode<Type>::~MC_PTreeNode()
{
	//shouldn't delete myData by default
}


//illegal constructor
template <class Type>
MC_PTreeNode<Type>::MC_PTreeNode()
{
	assert(0);
}
*/


#endif
