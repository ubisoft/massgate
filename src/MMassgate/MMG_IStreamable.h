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
#ifndef MMG_ISTREAMABLE__H_
#define MMG_ISTREAMABLE__H_

#include "MN_WriteMessage.h"
#include "MN_ReadMessage.h"

class MMG_IStreamable
{
	// Write the contents to the stream
	virtual void ToStream(MN_WriteMessage& theStream) const = 0;
	// Read contents from stream. Returns false if stream is corrupt.
	virtual bool FromStream(MN_ReadMessage& theStream) = 0;
};

#endif
