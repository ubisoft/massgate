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
#ifndef MSB_WRITEBUFFERPOOL_H
#define MSB_WRITEBUFFERPOOL_H

#include "MT_Mutex.h"

class MSB_WriteBuffer;

class MSB_WriteBufferPool
{
public: 
	static MSB_WriteBufferPool*		GetInstance();
	
	MSB_WriteBuffer*				GetWriteBuffer();

	void							ReleaseWriteBuffer(
										MSB_WriteBuffer* aWriteBuffer); 

	int32							FreeUnusedWriteBuffers();

private:
	MT_Mutex						myGlobalLock;
	MSB_WriteBuffer*				myHeadBuffer;

									MSB_WriteBufferPool();
									~MSB_WriteBufferPool();			
};

#endif // MSB_WRITEBUFFERPOOL_H
