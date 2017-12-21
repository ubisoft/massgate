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
#ifndef MMS_DOUBLE_BUFFERED_ARRAY_H
#define MMS_DOUBLE_BUFFERED_ARRAY_H

template <typename T>
class MMS_DoubleBufferedArray 
{
public:
						MMS_DoubleBufferedArray()
							: myContainerA(100, 100)
							, myContainerB(100, 100)
						{
							myFrontArray = &myContainerA;
							myBackArray = &myContainerB;
						}

		T*				GetBackArray()
						{
							return myBackArray;
						}

		T*				GetFrontArray()
						{
							return myFrontArray;
						}

		void			Flip()
						{
							T* temp = myFrontArray;
							myFrontArray = myBackArray;
							myBackArray = temp;
						}

		T*				operator->()
						{
							return myFrontArray;
						}

private:
	T					myContainerA;
	T					myContainerB;
	T*					myFrontArray;
	T*					myBackArray;
};

#endif /* MMS_DOUBLE_BUFFERED_ARRAY_H */