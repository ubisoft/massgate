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
#ifndef MSB_EXCEPTION_H
#define MSB_EXCEPTION_H

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MC_String.h"

class MSB_StackTraceException 
{
	public:
								MSB_StackTraceException();

		const char*				GetStackTrace() const;

	private:
		MC_StaticString<8192>	stackTrace;

		void					PrivMakeStackTrace();
};

#endif // IS_PC_BUILD

#endif /* MSB_EXCEPTION_H */