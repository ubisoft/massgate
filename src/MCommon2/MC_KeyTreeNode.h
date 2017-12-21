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
//mc_keytreenode.h
//tree node class for storing instances


#ifndef MC_KEYTREENODE_H
#define MC_KEYTREENODE_H

//instance tree node class
template <class Type, class Key>
class MC_KeyTreeNode
{
public:

	//constructor
	MC_KeyTreeNode(const Type& anInstance, const Key& aKey)
	{
		myData = anInstance;
		myKey = aKey;
		myLeftChild = NULL;
		myRightChild = NULL;
	}

	//destructor
	~MC_KeyTreeNode()
	{

	}

	//members
	Type myData;
	Key myKey;
	MC_KeyTreeNode<Type, Key>* myLeftChild;
	MC_KeyTreeNode<Type, Key>* myRightChild;

private:

	//illegal constructor
	MC_KeyTreeNode()
	{
		assert(0);
	}
};



/*
//IMPLEMENTATION
//constructor
template <class Type, class Key>
MC_KeyTreeNode<Type, Key>::MC_KeyTreeNode(const Type& anInstance, Key aKey)
{
	myData = anInstance;
	myKey = aKey;
	myLeftChild = NULL;
	myRightChild = NULL;
}


//destructor
template <class Type, class Key>
MC_KeyTreeNode<Type, Key>::~MC_KeyTreeNode()
{

}


//illegal constructor
template <class Type, class Key>
MC_KeyTreeNode<Type, Key>::MC_KeyTreeNode()
{
	assert(0);
}
*/




#endif
