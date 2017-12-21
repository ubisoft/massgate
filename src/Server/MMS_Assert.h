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
#ifndef MMS_ASSERT_H
#define MMS_ASSERT_H

void 
MMS_Assert(
	const char*  aExpression, 
	const char*	 aFile, 
	int			 aLine); 

#ifdef assert
#undef assert
#endif

#ifdef _DEBUG

#define assert(EXPRESSION) \
	if(!(EXPRESSION)) \
	{ \
		MMS_Assert(#EXPRESSION, __FILE__, __LINE__); \
	}

#else
#define assert(EXPRESSION) while(false){}
#endif

#endif // MMS_ASSERT_H