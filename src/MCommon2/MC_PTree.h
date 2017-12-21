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
//binary tree class
//for storing pointers to instances

#ifndef MC_PTREE_H
#define MC_PTREE_H

#include "mc_ptreenode.h"


//ptree class
template <class Type>
class MC_PTree
{
public:

	//constructor
	MC_PTree()
	{
		myRoot = NULL;
		myCount = 0;
	}

	//destructor
	virtual ~MC_PTree()
	{
		RemoveAll();
	}

	//uses overloaded operators for size comparison...
	bool Add(Type* aPointer)
	{
		const unsigned char LEFT_CHILD = 1;
		const unsigned char RIGHT_CHILD = 2;
		MC_PTreeNode<Type>* current = NULL;
		MC_PTreeNode<Type>* parent = NULL;
		unsigned char whichChild = 0;

		if(!myRoot)
		{
			//new node at root
			myRoot = new MC_PTreeNode<Type>(aPointer);
			assert(myRoot);

			//increase count
			assert(myCount == 0);
			myCount++;
			
			return (myRoot != NULL);
		}
		else
		{
			current = myRoot;

			//insert...
			while(current)
			{
				if(*aPointer < *(current->myData))
				{
					parent = current;
					current = current->myLeftChild;
					whichChild = LEFT_CHILD;
				}
				else if(*aPointer > *(current->myData))
				{
					parent = current;
					current = current->myRightChild;
					whichChild = RIGHT_CHILD;
				}
				else
				{
					assert(*aPointer == *(current->myData));
					//doubles are not allowed
					return false;
				}
			}

			assert(whichChild);
			switch(whichChild)
			{
			case LEFT_CHILD:
				{
					parent->myLeftChild = new MC_PTreeNode<Type>(aPointer);
					assert(parent->myLeftChild);

					//increase count
					myCount++;
					
					return (parent->myLeftChild != NULL);
				}
				break;

			case RIGHT_CHILD:
				{
					parent->myRightChild = new MC_PTreeNode<Type>(aPointer);
					assert(parent->myRightChild);

					//increase count
					myCount++;

					return (parent->myRightChild != NULL);
				}
				break;
			}
		}

		//shouldn't reach this point
		assert(0);
		return false;
	}

	//root node access
	MC_PTreeNode<Type>* GetRoot()
	{
		return myRoot;
	}

/*	//empty check
	bool IsEmpty() const
	{
		return (myRoot == NULL);
	}*/

	//tree contents count
	int Count() const
	{
		return myCount;
	}

	//uses overloaded operators for size comparison...
	bool Find(Type* aPointer, Type** aFoundPointer = NULL) const
	{
		MC_PTreeNode<Type>* current = myRoot;
		bool found = false;

		while(!found && current)
		{
			if(*aPointer < *(current->myData))
			{
				current = current->myLeftChild;
			}
			else if(*aPointer > *(current->myData))
			{
				current = current->myRightChild;
			}
			else
			{
				assert(*aPointer == *(current->myData));
				if(aFoundPointer)
				{
					*aFoundPointer = current->myData;
				}
				found = true;
			}
		}

		return found;
	}

	//remove
	bool Remove(Type* aPointer)
	{
		MC_PTreeNode<Type>* newRoot = NULL;
		MC_PTreeNode<Type>* parent = NULL;
		MC_PTreeNode<Type>* child = NULL;

		//if no root, just quit
		if(myRoot) 
		{
			//deleting root
			if(myRoot->myData == aPointer)
			{
				newRoot = RemoveTop(myRoot);
				delete myRoot;
				myRoot = newRoot;
				myCount--;	
				return true;
			}
			else
			{
				//otherwise some other node 
				parent = myRoot;

				while(parent)
				{
					if(parent->myData < aPointer)
					{
						//search right subtree
						child = parent->myRightChild;

						if(child && (child->myData == aPointer))
						{
							parent->myRightChild = RemoveTop(child);
							delete child;
							child = NULL;
							myCount--;	
						}
					}
					else
					{
						//search left subtree
						child = parent->myLeftChild;

						if(child && (child->myData == aPointer))
						{
							parent->myLeftChild = RemoveTop(child);
							delete child;
							child = NULL;
							myCount--;	
						}
					}

					parent = child;
				}

				return true;
			}
		}
		else
		{
			//no root
			return false;
		}
	}

	//clear tree (don't delete data in each node)
	void RemoveAll()
	{
		if(myRoot)
		{
			PostorderDelete(myRoot, false);
			myRoot = NULL;
			myCount = 0;
		}
	}

	//delete tree (delete data in each node)
	void DeleteAll()
	{
		if(myRoot)
		{
			PostorderDelete(myRoot, true);
			myRoot = NULL;
			myCount = 0;
		}
	}

protected:

	//postorder traversal, used to delete all nodes (Clear() and Delete())
	void PostorderDelete(MC_PTreeNode<Type>* aNode, bool aDeleteDataFlag) const
	{
		if(aNode->myLeftChild)
			PostorderDelete(aNode->myLeftChild, aDeleteDataFlag);

		if(aNode->myRightChild)
			PostorderDelete(aNode->myRightChild, aDeleteDataFlag);

		if(aNode)
		{
			//NULL child pointers
			aNode->myLeftChild = NULL;
			aNode->myRightChild = NULL;

			//delete contained instance?
			if(aDeleteDataFlag)
			{
				delete aNode->myData;
				aNode->myData = NULL;
			}

			//delete node
			delete aNode;
		}
	}

	//utility, remove the top node in a tree (used by Remove())
	MC_PTreeNode<Type>* RemoveTop(MC_PTreeNode<Type>* aRoot)
	{
		MC_PTreeNode<Type>* left  = aRoot->myLeftChild;
		MC_PTreeNode<Type>* right = aRoot->myRightChild;
		MC_PTreeNode<Type>* node = NULL;
		MC_PTreeNode<Type>* parent = NULL;

		//case 1, no left node
		if(!left)
			return right;

		//case 2, no right node
		if(!right)
			return left;

		//case 3, right node has no left node
		node = right->myLeftChild;
		if(!node)
		{
			right->myLeftChild = left;
			return right;
		}

		//case 4, slide down left tree
		parent = right;
		while(node->myLeftChild)
		{
			parent = node;
			node = node->myLeftChild;
		}

		//now parent points to node, node has no left child
		//disconnect and move to top
		parent->myLeftChild = node->myRightChild;
		node->myLeftChild = left;
		node->myRightChild = right;

		return node;
	}

	//members
	MC_PTreeNode<Type>* myRoot;
	int myCount;
};


/*
//IMPLEMENTATION
//constructor
template <class Type>
MC_PTree<Type>::MC_PTree()
{
	myRoot = NULL;
}


//destructor
template <class Type>
MC_PTree<Type>::~MC_PTree()
{
	Clear();
}


//uses overloaded operators for size comparison...
template <class Type>
bool MC_PTree<Type>::Insert(Type* aPointer)
{
	const unsigned char LEFT_CHILD = 1;
	const unsigned char RIGHT_CHILD = 2;
	MC_PTreeNode<Type>* current = NULL;
	MC_PTreeNode<Type>* parent = NULL;
	unsigned char whichChild = 0;

	if(IsEmpty())
	{
		//new node at root
		myRoot = new MC_PTreeNode<Type>(aPointer);
		assert(myRoot);
		
		return (myRoot != NULL);
	}
	else
	{
		current = myRoot;

		//insert...
		while(current)
		{
			if(*aPointer < *(current->myData))
			{
				parent = current;
				current = current->myLeftChild;
				whichChild = LEFTCHILD;
			}
			else if(*aPointer > *(current->myData))
			{
				parent = current;
				current = current->myRightChild;
				whichChild = RIGHTCHILD;
			}
			else if(*aPointer == *(current->myData))
			{
				//doubles are not allowed
				return false;
			}
		}

		assert(whichChild);
		if(whichChild == LEFTCHILD)
		{
			parent->myLeftChild = new MC_PTreeNode<Type>(aPointer);
			assert(parent->myLeftChild);
			
			return (parent->myLeftChild != NULL);
		}
		else if(whichChild == RIGHTCHILD)
		{
			parent->myRightChild = new MC_PTreeNode<Type>(aPointer);
			assert(parent->myRightChild);

			return (parent->myRightChild != NULL);
		}
	}

	//shouldn't reach this point
	return false;
}


//get root
template <class Type>
MC_PTreeNode<Type>* MC_PTree<Type>::GetRoot()
{
	return myRoot;
}


template <class Type>
bool MC_PTree<Type>::IsEmpty() const
{
	return (myRoot == NULL);
}


template <class Type>
int MC_PTree<Type>::Count() const
{
	unsigned int count = 0;

	if(!IsEmpty())
	{
		InorderCount(myRoot, count);
	}

	return count;
}


//uses overloaded operators for size comparison...
template <class Type>
bool MC_PTree<Type>::Find(Type* aPointer, Type** aFoundPointer) const
{
	MC_PTreeNode<Type>* current = myRoot;
	bool found = false;

	while(!found && current)
	{
		if(*aPointer < *(current->myData))
		{
			current = current->myLeftChild;
		}
		else if(*aPointer > *(current->myData))
		{
			current = current->myRightChild;
		}
		else
		{
			assert(*aPointer == *(current->myData));
			if(aFoundPointer)
			{
				*aFoundPointer = current->myData;
			}
			found = true;
		}
	}

	return found;
}


//remove
template <class Type>
bool MC_PTree<Type>::Remove(Type* aPointer)
{
	MC_PTreeNode<Type>* newRoot = NULL;
	MC_PTreeNode<Type>* parent = NULL;
	MC_PTreeNode<Type>* child = NULL;

	//if no root, just quit
	if(myRoot) 
	{
		//deleting root
		if(myRoot->myData == aPointer)
		{
			newRoot = RemoveTop(myRoot);
			delete myRoot;
			myRoot = newRoot;
			
			return true;
		}
		else
		{
			//otherwise some other node 
			parent = myRoot;

			while(parent)
			{
				if(parent->myData < aPointer)
				{
					//search right subtree
					child = parent->myRightChild;

					if(child && (child->myData == aPointer))
					{
						parent->myRightChild = RemoveTop(child);
						delete child;
						child = NULL;
					}
				}
				else
				{
					//search left subtree
					child = parent->myLeftChild;

					if(child && (child->myData == aPointer))
					{
						parent->myLeftChild = RemoveTop(child);
						delete child;
						child = NULL;
					}
				}

				parent = child;
			}

			return true;
		}
	}
	else
	{
		//no root
		return false;
	}
}


//utility, remove the root node in the tree
template <class Type>
MC_PTreeNode<Type>* MC_PTree<Type>::RemoveTop(MC_PTreeNode<Type>* aRoot)
{
	MC_PTreeNode<Type>* left  = aRoot->myLeftChild;
	MC_PTreeNode<Type>* right = aRoot->myRightChild;
	MC_PTreeNode<Type>* node = NULL;
	MC_PTreeNode<Type>* parent = NULL;

	//case 1, no left node
	if(!left)
		return right;

	//case 2, no right node
	if(!right)
		return left;

	//case 3, right node has no left node
	node = right->myLeftChild;
	if(!node)
	{
		right->myLeftChild = left;
		return right;
	}

	//case 4, slide down left tree
	parent = right;
	while(node->myLeftChild)
	{
		parent = node;
		node = node->myLeftChild;
	}

	//now parent points to node, node has no left child
	//disconnect and move to top
	parent->myLeftChild = node->myRightChild;
	node->myLeftChild = left;
	node->myRightChild = right;

	return node;
}



//clear tree (don't delete data in each node)
template <class Type>
void MC_PTree<Type>::Clear()
{
	if(!IsEmpty())
	{
		PostorderDelete(myRoot, false);
		myRoot = NULL;
	}
}


//delete tree (delete data in each node)
template <class Type>
void MC_PTree<Type>::Delete()
{
	if(!IsEmpty())
	{
		PostorderDelete(myRoot, true);
		myRoot = NULL;
	}
}


//postorder traversal, used to delete all nodes
//NOTE: This WILL delete the contained instances
template <class Type>
void MC_PTree<Type>::PostorderDelete(MC_PTreeNode<Type>* aNode, bool aDeleteDataFlag) const
{
	if(aNode->myLeftChild)
		PostorderDelete(aNode->myLeftChild, aDeleteDataFlag);

	if(aNode->myRightChild)
		PostorderDelete(aNode->myRightChild, aDeleteDataFlag);

	if(aNode)
	{
		//NULL child pointers
		aNode->myLeftChild = NULL;
		aNode->myRightChild = NULL;

		//delete contained instance?
		if(aDeleteDataFlag)
		{
			delete aNode->myData;
			aNode->myData = NULL;
		}

		//delete node
		delete aNode;
	}
}


template <class Type>
void MC_PTree<Type>::InorderCount(MC_PTreeNode<Type>* aNode, unsigned int& aCount) const
{
	//left subptree
	if(aNode->myLeftChild)
		InorderCount(aNode->myLeftChild, aCount);

	//self
	aCount++;

	//right subptree
	if(aNode->myRightChild)
		InorderCount(aNode->myRightChild, aCount);
}
*/

#endif
