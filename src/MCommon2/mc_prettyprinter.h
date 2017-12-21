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

#ifndef __MDB_PRETTYPRINTER_H__
#define __MDB_PRETTYPRINTER_H__

#include "mc_string.h"

class MDB_IPrettyPrintable
{
public:
    virtual MC_String ToString() const = 0;
};

class MC_PrettyPrinter
{
public:
	static MC_String ToString(const MDB_IPrettyPrintable& anObject);
	static void ToDebug(const char* theData, size_t theDatalen, const char *aTag = "");
};

#endif //__MDB_PRETTYPRINTER_H__
