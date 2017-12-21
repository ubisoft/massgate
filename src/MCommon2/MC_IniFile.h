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
#ifndef _MC_INIFILE_H_
#define _MC_INIFILE_H_

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Header files
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "MC_ArrayMap.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INIFILE utility class...
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

class MC_IniFile
{
public:
						MC_IniFile(
							const char*		aFilename);
						~MC_IniFile();

	bool				Process();

	f32					GetFloat(
							const char*		aKey);
	s32					GetInt(
							const char*		aKey);
	u32					GetUInt(
							const char*		aKey);
	bool				GetBool(
							const char*		aKey);
	const char*			GetString(
							const char*		aKey);
	bool				HasKey(
							const char*		aKey);

private:
	u8*					myFileBuffer;
	s32					mySize;

	MC_ArrayMap<MC_String, MC_String> myValues;

	static const char*	PrivSkipWhitespace(
							const char*		aBegin,
							const char*		aEnd);

	static const char*	PrivLineEnd(
							const char*		aBegin,
							const char*		aEnd);

	static const char*	PrivValueDelimeter(
							const char*		aBegin,
							const char*		aEnd);

	static const char*	PrivSectionNameEnd(
							const char*		aBegin,
							const char*		aEnd,
							bool&			aOutSectionClosed);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif _MC_INIFILE_H_



/* end of file */