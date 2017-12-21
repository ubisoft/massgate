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

#ifndef __MC_COMMANDLINE_H__
#define __MC_COMMANDLINE_H__

#include "mc_growingarray.h"

/* 
	EXCO_CommandLine: Singleton class that parses a command line (either set when calling Create() or the Unicoded commandline returned by system call GetCommandLine()).
*/

class MC_CommandLine
{
public:
	// Call to create.
	static bool Create( const char* aCommandLine );
	
	// Call to destroy.
	static void Destroy( void );

	// returns true if anArguemnt was present on commandline (with value or not)
	// anArgument should be lowercase as any commands are forced to lowercase on creation.
	bool IsPresent( const char* anArgument );

	// Gets the integer value of argument anArgument. If the value of the argument wasn't an int, result is undefined.
	// anArgument should be lowercase as any commands are forced to lowercase on creation.
	bool GetIntValue( const char* anArgument, int& aReturnValue );

	// Gets the float value of argument anArgument. If the value of the argument wasn't a float, result is undefined.
	// anArgument should be lowercase as any commands are forced to lowercase on creation.
	bool GetFloatValue( const char* anArgument, float& aReturnValue );

	// Gets the string value of argument anArgument. 
	// anArgument should be lowercase as any commands are forced to lowercase on creation.
	bool GetStringValue( const char* anArgument, const char*& aReturnValue );

	// Gets the source command line
	const char* GetSourceCommandLine() { return myCommandLineBuf; }

	// Get instance of class.
	static MC_CommandLine* GetInstance( void ) { return ourInstance; }

	// Used to programatically add commandlines argument (mostly used in debugging purposes).
	bool Add( const char* anArgument, const char* aValue );

private:
	// Struct for storing arguments and it's corresponding values.
	struct ArgumentStruct
	{
		char* myArgument;
		char* myValue;
	};	

	// Private construct, destruct. Use Create() and Destroy() instead.
	MC_CommandLine( void );
	~MC_CommandLine( void );

	// myinstance
	static MC_CommandLine* ourInstance;

	// My arguments
	MC_GrowingArray<ArgumentStruct*> myArguments;

	// Copy of command line
	char myCommandLineBuf[256];
};

#endif //__MC_COMMANDLINE_H__
