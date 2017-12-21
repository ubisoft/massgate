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
#ifndef MN_IWRITEABLEDATASTREAM___H_
#define MN_IWRITEABLEDATASTREAM___H_

#include "mn_connectionerrortype.h"


class MN_IWriteableDataStream
{
public:
	virtual MN_ConnectionErrorType Send(const void* theData, unsigned int theDatalen) = 0;
	virtual unsigned int GetRecommendedBufferSize() = 0;
};

// These should really be in mc_ instead?
class MN_FileStream : public MN_IWriteableDataStream
{
public:
	MN_FileStream(FILE* theFile) : myFile(theFile) { }
	virtual MN_ConnectionErrorType Send(const void* theData, unsigned int theDatalen) { fwrite(theData, theDatalen, 1, myFile); return MN_CONN_OK; }
	virtual unsigned int GetRecommendedBufferSize() { return 8192; }
	~MN_FileStream() { fclose(myFile); }
private:
	FILE* myFile;
};

#endif
