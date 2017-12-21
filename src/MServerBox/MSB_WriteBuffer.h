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
#ifndef MSB_WRITEBUFFER_H
#define MSB_WRITEBUFFER_H

#include "MSB_WriteBufferPool.h"
#include "MSB_Types.h"

class MSB_WriteBuffer
{
public: 
	static MSB_WriteBuffer*		Allocate();

	void						Retain();
	void						Release();

	//Retains/Releases all my following WriteBuffers
	void						RetainAll();
	void						ReleaseAll();

	uint8						myBuffer[MSB_WRITEBUFFER_DEFAULT_SIZE];
	uint32						myBufferSize;
	uint32						myWriteOffset;

	MSB_WriteBuffer*			myNextBuffer;
private:
	volatile LONG				myRefCount;
	MSB_WriteBufferPool*		myAllocationPool; 

								MSB_WriteBuffer();
								~MSB_WriteBuffer();

	void						Deallocate();
	static MSB_WriteBuffer*		PrivAllocateWin();
	static MSB_WriteBuffer*		PrivAllocateConsole();

	friend class				MSB_WriteBufferPool;
}; 

#endif // MSB_WRITEBUFFER_H
