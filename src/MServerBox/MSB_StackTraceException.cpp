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
#include "stdafx.h"

#include "MC_Base.h"
#if IS_PC_BUILD

#include "MC_StackWalker.h"

#include "MSB_StackTraceException.h"

MSB_StackTraceException::MSB_StackTraceException()
{
	PrivMakeStackTrace();
}

const char*
MSB_StackTraceException::GetStackTrace() const
{
	return stackTrace.GetBuffer();
}

void
MSB_StackTraceException::PrivMakeStackTrace()
{
// 	MC_StackWalker::GetInstance()->TraceAndResolveCallstackToString(stackTrace);
}

#endif // IS_PC_BUILD
