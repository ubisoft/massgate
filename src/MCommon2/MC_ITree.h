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
//mc_itree.h
//binary tree class
//for storing instances

#ifndef MC_ITREE_H
#define MC_ITREE_H

#include "mc_itreenode.h"


//itree class
template <class Type>
class MC_ITree
{
public:

	//constructor
	MC_ITree()
	{
		myRoot = NULL;
		myCount = 0;
	}

	//destructor
	virtual ~MC_ITree()
	{
		RemoveAll();
	}

	//uses overloaded operators for size comparison...
	virtual bool Add(const Type& anInstance)
	{
		const unsigned char LEFT_CHILD = 1;
		const unsigned char RIGHT_CHILD = 2;
		MC_ITreeNode<Type>* current = NULL;
		MC_ITreeNode<Type>* parent = NULL;
		unsigned char whichChild = 0;

		//if empty
		if(!myRoot)
		{
			//new node at root
			myRoot = new MC_ITreeNode<Type>(anInstance);
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
				switch(Compare(anInstance, current->myData))
				{
				case 0:
					{
						parent = current;
						current = current->myLeftChild;
						whichChild = LEFT_CHILD;
					}
					break;

				case 1:
					{
						parent = current;
						current = current->myRightChild;
						whichChild = RIGHT_CHILD;
					}
					break;

				default:
					{
						//doubles are not allowed
						return false;
					}
					break;
				}
			}

			assert(whichChild);
			switch(whichChild)
			{
			case LEFT_CHILD:
				{
					parent->myLeftChild = new MC_ITreeNode<Type>(anInstance);
					assert(parent->myLeftChild);
					
					//increase count
					myCount++;

					return (parent->myLeftChild != NULL);
				}
				break;

			case RIGHT_CHILD:
				{
					parent->myRightChild = new MC_ITreeNode<Type>(anInstance);
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
	MC_ITreeNode<Type>* GetRoot()
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
	bool Find(const Type& anInstance) const
	{
		MC_ITreeNode<Type>* current = myRoot;
		bool found = false;

		while(!found && current)
		{
			if(anInstance < current->myData)
			{
				current = current->myLeftChild;
			}
			else if(anInstance > current->myData)
			{
				current = current->myRightChild;
			}
			else
			{
				assert(anInstance == current->myData);
				found = true;
			}
		}

		return found;
	}

	//remove
	bool Remove(const Type& anInstance)
	{
		MC_ITreeNode<Type>* newRoot = NULL;
		MC_ITreeNode<Type>* parent = NULL;
		MC_ITreeNode<Type>* child = NULL;

		//if no root, just quit
		if(myRoot) 
		{
			//deleting root
			if(myRoot->myData == anInstance)
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
					if(parent->myData < anInstance)
					{
						//search right subtree
						child = parent->myRightChild;

						if(child && (child->myData == anInstance))
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

						if(child && (child->myData == anInstance))
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

	//used by destructor
	void RemoveAll()
	{
		//if not empty
		if(myRoot)
		{
			PostorderRemove(myRoot);   
			myRoot = NULL;
			myCount = 0;
		}
	}

protected:

	//postorder traversal, used to clear all nodes (used by Clear())
	void PostorderRemove(MC_ITreeNode<Type>* aNode) const
	{
		if(aNode->myLeftChild)
			PostorderRemove(aNode->myLeftChild);

		if(aNode->myRightChild)
			PostorderRemove(aNode->myRightChild);

		if(aNode)
		{
			//precaution...
			aNode->myLeftChild = NULL;
			aNode->myRightChild = NULL;
			delete aNode;
		}
	}

	//utility, remove the top node in a tree (used by Remove())
	MC_ITreeNode<Type>* RemoveTop(MC_ITreeNode<Type>* aRoot)
	{
		MC_ITreeNode<Type>* left  = aRoot->myLeftChild;
		MC_ITreeNode<Type>* right = aRoot->myRightChild;
		MC_ITreeNode<Type>* node = NULL;
		MC_ITreeNode<Type>* parent = NULL;

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

	virtual const unsigned char Compare(const Type& anInstance, const Type& anotherInstance)
	{
		if(anInstance < anotherInstance)
		{
			return 0;
		}
		else if(anInstance > anotherInstance)
		{
			return 1;
		}
		else
		{
			return 2;
		}
	}

	//members
	MC_ITreeNode<Type>* myRoot;
	int myCount;
};

#endif
