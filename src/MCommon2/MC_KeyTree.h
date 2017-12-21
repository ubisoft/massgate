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
//mc_keytree.h
//binary tree class
//for storing instances or pointers to instances along with a key,
//used for comparisons (this is why you can store pointers or instances as you like)

#ifndef MC_KEYTREE_H
#define MC_KEYTREE_H

#include "mc_keytreenode.h"


//keytree class
template <class Type, class Key>
class MC_KeyTree
{
public:

	//constructor
	MC_KeyTree(const bool aDuplicatesAllowedFlag = false)
	:myRoot(NULL)
	,myCount(0)
	,myDuplicatesAllowedFlag(aDuplicatesAllowedFlag)
	{
	}

	//destructor
	~MC_KeyTree()
	{
		RemoveAll();
	}

	//uses overloaded operators for size comparison...
	bool Add(const Type& anInstance, const Key& aKey)
	{
		const unsigned char LEFTCHILD = 1;
		const unsigned char RIGHTCHILD = 2;
		MC_KeyTreeNode<Type, Key>* current = NULL;
		MC_KeyTreeNode<Type, Key>* parent = NULL;
		unsigned char whichChild = 0;

		if(!myRoot)
		{
			//new node at root
			myRoot = new MC_KeyTreeNode<Type, Key>(anInstance, aKey);
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
				if(aKey < current->myKey)
				{
					parent = current;
					current = current->myLeftChild;
					whichChild = LEFTCHILD;
				}
				else if(aKey > current->myKey)
				{
					parent = current;
					current = current->myRightChild;
					whichChild = RIGHTCHILD;
				}
				else
				{
					assert(aKey == current->myKey);

					if(myDuplicatesAllowedFlag)
					{
						//duplicates are stored as greater
						parent = current;
						current = current->myRightChild;
						whichChild = RIGHTCHILD;
					}
					else
					{
						//duplicates are not allowed
						return false;
					}
				}
			}

			assert(whichChild);
			switch(whichChild)
			{
			case LEFTCHILD:
				{
					parent->myLeftChild = new MC_KeyTreeNode<Type, Key>(anInstance, aKey);
					assert(parent->myLeftChild);
					
					//increase count
					myCount++;
					
					return (parent->myLeftChild != NULL);
				}
				break;

			case RIGHTCHILD:
				{
					parent->myRightChild = new MC_KeyTreeNode<Type, Key>(anInstance, aKey);
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

	//root access
	MC_KeyTreeNode<Type, Key>* GetRoot()
	{
		return myRoot;
	}

/*	//empty check
	bool IsEmpty() const
	{
		return (myRoot == NULL);
	}*/

	//count
	int Count() const
	{
		return myCount;
	}

	//uses overloaded operators for size comparison...
	bool Find(const Key& aKey, Type* aTypePointer = NULL, MC_KeyTreeNode<Type, Key>** aNodePointer = NULL) const
	{
		MC_KeyTreeNode<Type, Key>* current = myRoot;

		while(current)
		{
			if(aKey < current->myKey)
			{
				current = current->myLeftChild;
			}
			else if(aKey > current->myKey)
			{
				current = current->myRightChild;
			}
			else
			{
				assert(aKey == current->myKey);

				if(aTypePointer)
				{
					*aTypePointer = current->myData;
				}
				if(aNodePointer)
				{
					*aNodePointer = current;
				}
				return true;
			}
		}

		return false;
	}

	//uses overloaded operators for size comparison of keys...
	//find closest lesser or exact key matching the passed key, and return the assocated data
	//number of comparisons will here by equal to the depth of the tree, if it is balanced.
	void FindClosestLesser(const Key& aKey, Type& aType) const
	{
		MC_KeyTreeNode<Type, Key>* current = myRoot;

		while(current)
		{
			if(aKey < current->myKey)
			{
				//current key is greater than passed key, so it is not the closest value
				//go to lesser keys
				current = current->myLeftChild;
			}
			else if(aKey > current->myKey)
			{
				//current key is less that passed key, so it may be the closest value
				aType = current->myData;
				
				//go to greater keys
				current = current->myRightChild;
			}
			else
			{
				//equal to passed key, return directly
				aType = current->myData;
				return;
			}
		}

		//aType is now the closest value
	}

	//remove item for key
	bool Remove(const Key& aKey, Type* aRemovedItemPointer = NULL)
	{
		MC_KeyTreeNode<Type, Key>* newRoot = NULL;
		MC_KeyTreeNode<Type, Key>* parent = NULL;
		MC_KeyTreeNode<Type, Key>* child = NULL;

		//if no root, just quit
		if(myRoot) 
		{
			//deleting root
			if(myRoot->myKey == aKey)
			{
				//grab the item to be removed
				if(aRemovedItemPointer)
				{
					*aRemovedItemPointer = myRoot->myData;
				}
				
				newRoot = RemoveTop(myRoot);
				delete myRoot;
				myRoot = newRoot;

				myCount--;
				assert(myCount >= 0);
				
				return true;
			}
			else
			{
				//otherwise some other node 
				parent = myRoot;

				while(parent)
				{
					if(parent->myKey < aKey)
					{
						//search right subtree
						child = parent->myRightChild;

						if(child && (child->myKey == aKey))
						{
							//grab the item to be removed
							if(aRemovedItemPointer)
							{
								*aRemovedItemPointer = child->myData;
							}
							
							parent->myRightChild = RemoveTop(child);
							delete child;
							child = NULL;

							myCount--;
							assert(myCount >= 0);

							return true;
						}
					}
					else
					{
						//search left subtree
						child = parent->myLeftChild;

						if(child && (child->myKey == aKey))
						{
							//grab the item to be removed
							if(aRemovedItemPointer)
							{
								*aRemovedItemPointer = child->myData;
							}

							parent->myLeftChild = RemoveTop(child);
							delete child;
							child = NULL;

							myCount--;
							assert(myCount >= 0);

							return true;
						}
					}

					parent = child;
				}
			}
		}

		return false;
	}


	//clear tree
	void RemoveAll()
	{
		if(myRoot)
		{
			PostorderClear(myRoot);
			myRoot = NULL;
			myCount = 0;
		}
	}

	//delete tree
	void DeleteAll()
	{
		if(myRoot)
		{
			PostorderDelete(myRoot);
			myRoot = NULL;
			myCount = 0;
		}
	}

protected:

	//postorder traversal, used to delete all nodes
	void PostorderDelete(MC_KeyTreeNode<Type, Key>* aNode) const
	{
		if(aNode->myLeftChild)
			PostorderDelete(aNode->myLeftChild);

		if(aNode->myRightChild)
			PostorderDelete(aNode->myRightChild);

		if(aNode)
		{
			//NULL child pointers
			aNode->myLeftChild = NULL;
			aNode->myRightChild = NULL;

			//delete contained instance
			delete aNode->myData;
			aNode->myData = NULL;

			//delete node
			delete aNode;
		}
	}

	//postorder traversal, used to clear all nodes
	void PostorderClear(MC_KeyTreeNode<Type, Key>* aNode) const
	{
		if(aNode->myLeftChild)
			PostorderClear(aNode->myLeftChild);

		if(aNode->myRightChild)
			PostorderClear(aNode->myRightChild);

		if(aNode)
		{
			//NULL child pointers
			aNode->myLeftChild = NULL;
			aNode->myRightChild = NULL;

			//delete node
			delete aNode;
		}
	}

	void InorderCount(MC_KeyTreeNode<Type, Key>* aNode, unsigned int& aCount) const
	{
		//left subkeytree
		if(aNode->myLeftChild)
			InorderCount(aNode->myLeftChild, aCount);

		//self
		aCount++;

		//right subkeytree
		if(aNode->myRightChild)
			InorderCount(aNode->myRightChild, aCount);
	}

	//utility, remove the top node in a tree (used by Remove())
	MC_KeyTreeNode<Type, Key>* RemoveTop(MC_KeyTreeNode<Type, Key>* aRoot)
	{
		MC_KeyTreeNode<Type, Key>* left  = aRoot->myLeftChild;
		MC_KeyTreeNode<Type, Key>* right = aRoot->myRightChild;
		MC_KeyTreeNode<Type, Key>* node = NULL;
		MC_KeyTreeNode<Type, Key>* parent = NULL;

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
	MC_KeyTreeNode<Type, Key>* myRoot;
	int myCount;
	const bool myDuplicatesAllowedFlag;
};

#endif
