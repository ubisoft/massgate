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
#ifndef MSB_VITTER_
#define MSB_VITTER_


typedef	uint16 NodePtr;

class MSB_Vitter
{
public:
					MSB_Vitter();
					~MSB_Vitter();


private:
	class Node
	{
	public:
		uint32		myWeight;	
		NodePtr		myParent;	//Next node up the tree
		NodePtr		myChildren; //Right child, left child = myChildren - 1
		uint8		mySymbol;	//The character/symbol this node represents
	};

	NodePtr			myEscapeNode;
	NodePtr			myRoot;
	NodePtr*		myMap;

};




#endif // MSB_VITTER_