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

#include "MC_DebugListenerImplementations.h"
#include "mf_file.h"
#include "MT_Mutex.h"

#pragma push_macro("new")
#undef new

#include <crtdbg.h>
#include <time.h>
#include <sys/timeb.h>
#include <io.h>
#pragma pop_macro("new")


// DO AVOID USE OF CRT IN ANY ::DebugMessage implementations!
// DO AVOID ANY (IMPLICIT) MEMORY ALLOCATIONS!



#ifndef _RELEASE_
MC_MsDevOutputDebugListener::MC_MsDevOutputDebugListener(  ) : MC_DebugListener()
{
}

void MC_MsDevOutputDebugListener::DebugMessage( const char* aMessage )
{
	// Will output aMessage into the visual studio debugger.
	// If the message is longer than 4092b then it is truncated

	char str[4094]; 

	unsigned int strIterator=0;
	while (*aMessage && (strIterator < sizeof(str)-2))
		str[strIterator++]=*aMessage++;
	str[strIterator++]='\n';
	str[strIterator] = 0;
	// Good OutputDebugString docs (http://www.unixwiz.net/techtips/outputdebugstring.html)
	OutputDebugString(str);
}

#endif // _RELEASE_

void MC_MsDevOutputDebugListener::Commit()
{
}

void MC_StderrDebugListener::DebugMessage( const char* aMessage )
{
	// DO AVOID USE OF CRT IN ANY ::DebugMessage implementations!
	fprintf(stderr, "%s\n", aMessage);
}

void MC_StderrDebugListener::Commit()
{
}

MC_FileDebugListener::MC_FileDebugListener( const char* aFilename, bool anAppendFileFlag, bool aFlushFlag) : MC_DebugListener()
{
	myFile = fopen(aFilename, anAppendFileFlag ? "at" : "wt"); // verify name and empty file
	myFlushFlag = aFlushFlag;

	if(myFile)
	{
		if(myFlushFlag)
		{
			// Try to force flush of each write
			setvbuf(myFile, NULL, _IONBF, 0); 
		}

		// Print the local time and UTC time so we can compare debugfiles from different timezones.
		struct __timeb64 timebuffer;
		_ftime64(&timebuffer);
		struct tm* timetm = localtime(&timebuffer.time); // Display time as local-time
		// Cannot queue to calls to asctime without saving intermediate (varargs uses a static buffer internally)
		char asctime_tmp[20];
		memcpy_s(asctime_tmp, sizeof(asctime_tmp), asctime(timetm), 19);

		timetm = _gmtime64(&timebuffer.time); // Display time as GMT (UTC)
		fprintf(myFile, "# Debugfile %s\n# Created at localtime %.19s (%.19s UTC)\n# Comment: \n", aFilename, asctime_tmp, asctime(timetm));
	}
}

void MC_FileDebugListener::Commit()
{
	if (myFile)
		_commit(_fileno(myFile));
}

void MC_FileDebugListener::Destroy( void )
{
	if( myFile )
	{
		fclose( myFile );
		myFile = NULL;
	}
}

//#ifndef	_RELEASE_	
void MC_FileDebugListener::DebugMessage( const char* aMessage )
{
	// DO AVOID USE OF CRT IN ANY ::DebugMessage implementations!
	if( myFile )
	{
		struct __timeb64 timebuffer;
		_ftime64(&timebuffer);
		struct tm* timetm = localtime(&timebuffer.time); // Display time as local-time
		fprintf(myFile, "[%.19s.%.3hu] %s\n", asctime(timetm), timebuffer.millitm, aMessage);	 // Write to crt buffer

		if(myFlushFlag)
		{
			fflush(myFile);	// Flush to system
#ifdef MC_DEBUG_COMMIT_OUTPUT_TO_DISK
			_commit(_fileno(myFile)); // Flush system to disk
#endif
		}
	}
}

MC_XMLFileListener::MC_XMLFileListener( const char* aFilename, bool anAppendFileFlag )
:myAppendFlag( anAppendFileFlag )
{
	bool exist = MF_File::ExistFile( aFilename );
	__time64_t ltime;

   _time64( &ltime );
	char* ct = _ctime64( &ltime );
	ct[24] = 0;
	
	if( anAppendFileFlag && exist )
		myFile = fopen(aFilename, "r+"); 
	else
		myFile = fopen(aFilename, "w"); // verify name and empty file

	myFilename = aFilename;
	if( myFile )
	{
		if( exist && myAppendFlag )
		{
			fseek( myFile, -4, SEEK_END );
			fprintf( myFile, "<run started=\"%s\">\n</run></e>", ct );
		}
		else
		{
			if( myAppendFlag )
				fprintf( myFile, "<e><run started=\"%s\">\n</run></e>", ct );
			else
				fprintf( myFile, "<e started=\"%s\">\n</e>", ct );
		}
	}
}

void MC_XMLFileListener::Commit()
{
	if (myFile)
		_commit(_fileno(myFile));
}

MC_XMLFileListener::~MC_XMLFileListener( void )
{
	if (myFile)
		fclose(myFile); 
}

void MC_XMLFileListener::StartError( const char* aDescription )
{
	int num = -4;  // </e>

	if( !myFile )
		return;

	if( myAppendFlag )  // </run>
		num-=6;

	fseek(myFile, num, SEEK_END);
	fprintf(myFile, "\n<error desc=\"");

	for(int i=0; aDescription[i]; i++)
	{
		char c = aDescription[i];

		// " -> '
		if(c == '\"')
			c = '\'';

		fprintf( myFile, "%c", c );
	}

	fprintf( myFile, "\">", aDescription );
}

void MC_XMLFileListener::CloseError( void )
{
	if( !myFile )
		return;

	fprintf( myFile, "\n</error>\n" );
	if( myAppendFlag )
        fprintf( myFile, "</run>" );
	fprintf( myFile, "</e>" );

	fflush(myFile);
#ifndef MC_DEBUG_COMMIT_OUTPUT_TO_DISK
	_commit(_fileno(myFile));
#endif
}

void MC_XMLFileListener::StartTag( const char* aTag, const char* aDesc )
{
	if( !myFile )
		return;
	if( aDesc )
		fprintf( myFile, "\n\t<%s d=\"%s\">", aTag, aDesc );
	else
		fprintf( myFile, "\n\t<%s>", aTag );
}

void MC_XMLFileListener::WriteData( const char* data )
{
	if( !myFile )
		return;
	fprintf( myFile, "%s", data);
}

void MC_XMLFileListener::EndTag( const char* aTag )
{
	if( !myFile )
		return;
	fprintf( myFile, "\n\t</%s>", aTag );
}

//#endif // _RELEASE_

