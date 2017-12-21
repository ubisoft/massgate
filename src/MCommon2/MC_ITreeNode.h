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
//tree node class for storing instances


#ifndef MC_ITREENODE_H
#define MC_ITREENODE_H

//instance tree node class
template <class Type>
class MC_ITreeNode
{
public:

	//constructor
	MC_ITreeNode(Type anInstance)
	{
		myData = anInstance;
		myLeftChild = NULL;
		myRightChild = NULL;
	}

	//destructor
	~MC_ITreeNode()
	{

	}

	//members
	Type myData;
	MC_ITreeNode<Type>* myLeftChild;
	MC_ITreeNode<Type>* myRightChild;

private:

	//illegal constructor
	MC_ITreeNode()
	{
		assert(0);
	}
};



/*
//IMPLEMENTATION
//constructor
template <class Type>
MC_ITreeNode<Type>::MC_ITreeNode(Type anInstance)
{
	myData = anInstance;
	myLeftChild = NULL;
	myRightChild = NULL;
}


//destructor
template <class Type>
MC_ITreeNode<Type>::~MC_ITreeNode()
{

}


//illegal constructor
template <class Type>
MC_ITreeNode<Type>::MC_ITreeNode()
{
	assert(0);
}
*/




#endif
