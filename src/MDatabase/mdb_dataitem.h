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
#ifndef MMS_DATAITEM
#define MMS_DATAITEM

#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>

class MDB_DataItem
{
public:
	MDB_DataItem(const void* theData) : myData(theData) { }
	operator const char* () const { return (const char*)myData; }
	operator const MC_String () const { return MC_String((const char*)myData); }

	operator const bool () const { return bool(atoi((const char*)myData) == 0 ? false : true); }

	operator const short () const { return short(atoi((const char*)myData)); }
	operator const unsigned short () const { return unsigned short(atoi((const char*)myData)); }

	operator const int () const { return atoi((const char*)myData); }
	operator const unsigned int () const { return (unsigned int)_tcstoui64((const char*)myData, NULL, 10 ); }

	operator const __int64 () const { return _atoi64((const char*)myData); }
	operator const unsigned __int64 () const { return _tcstoui64((const char*)myData, NULL, 10 ); }  // for some reason _atoi64 didn't work?

	operator const double () const {return atof((const char*)myData); }
	operator const long () const { return atol((const char*)myData); }
	operator const float () const {return float(atof((const char*)myData)); }

private:
	const void* myData;
};

#endif
