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
#ifndef MSB_WRITELIST_H
#define MSB_WRITELIST_H

class MSB_WriteBuffer;

class MSB_WriteList
{
public:
	class Entry 
	{
	public:
		Entry*				myNext;
		MSB_WriteBuffer*	myBuffers;
	};

							MSB_WriteList();
							~MSB_WriteList();

	MSB_WriteBuffer*		Pop();
	void					Push(
								MSB_WriteBuffer*		aBuffer);

private:
	Entry*					myHead;
	Entry*					myTail;
};

#endif /* MSB_WRITELIST_H */
