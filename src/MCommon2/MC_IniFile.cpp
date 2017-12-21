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
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Header files
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "mf_file.h"

#include "MC_IniFile.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// Internal macros
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////////////////////
//// MC_IniLine::MC_IniLine()
////
//// Parameters:
////  num_sections			number of items to allow per line
////  line_buffer_size		maximum size of an ASCII line
////
//// Explanation:
////  Constructor for MC_IniLine class. Internally the object will allocate
////  workspace for processing ASCII lines into separate components, the size of the 
////  buffer will be:
////      (num_sections*4)+line_buffer_size
////
////////////////////////////////////////////////////////////////////////////////////
//
//MC_IniLine::MC_IniLine(s32 aSectionLimit, s32 aLineLimit) :
//	myNumSections(0),
//	myMaxSections(aSectionLimit),
//	myLineSize(aLineLimit)
//{
//	// create workspace
//	mySections = (char**)MC_TempAlloc((sizeof(char*)*aSectionLimit)+aLineLimit);//MEM_alloc("Workspace", (sizeof(char*)*num_sections)+line_buffer_size, MEM_FROMTOP);
////	DEBUG_assertPrintf(m_sections, ("INIFILE_LINE_DATA -> Cannot allocate buffer"));
//	assert(mySections);
//	myBuffer = (char*)&mySections[aSectionLimit];
//}
//
////////////////////////////////////////////////////////////////////////////////////
//// MC_IniLine::~MC_IniLine()
////
//// Explanation:
////  Destructor for MC_IniLine class. 
////
////////////////////////////////////////////////////////////////////////////////////
//
//MC_IniLine::~MC_IniLine()
//{
//	if (mySections)
//		MC_TempFree(mySections);
//}
//
////////////////////////////////////////////////////////////////////////////////////
//// MC_IniLine::SkipSpaces()
////
//// Parameters:
////  line			Address of an ASCII string
////  line_end		Address of the end of an ASCII string
////
//// Explanation:
////  This function simply skips any space or tab characters in an ASCII string
////  without going beyond the end of the string.
////
//// Returns:
////  Updated string pointer
////
////////////////////////////////////////////////////////////////////////////////////
//
//const char* MC_IniLine::SkipSpaces(const char* aLineStart, const char* aLineEnd)
//{
//	while (aLineStart<aLineEnd && (*aLineStart==' ' || *aLineStart=='\t'))
//		aLineStart++;
//	return (aLineStart);
//}
//
////////////////////////////////////////////////////////////////////////////////////
//// MC_IniLine::Parse()
////
//// Parameters:
////  line			Address of ASCII data
////  line_end		Address of the end of the ASCII string
////
//// Explanation:
////  This is the main function in the INIFILE_LINE_DATA class. This function
////  takes the string passed in and parses a single line from the data into
////  the INIFILE_LINE_DATA object. Once parsed the components of the line
////  can be access via the utility functions.
////
//// Returns:
////  Updated string pointer, points to next line in the ASCII data or the end of
////  the ASCII data.
////
////////////////////////////////////////////////////////////////////////////////////
//
//const char* MC_IniLine::Parse(const char* aLineStart, const char* aLineEnd)
//{
//	char *dest;
//
//	// reset current status of line
//	myNumSections = 0;
//	dest = myBuffer;
//	// process until line terminator found
//	aLineStart = SkipSpaces(aLineStart, aLineEnd);
//	while (aLineStart<aLineEnd && *aLineStart!=';' && (*aLineStart=='\t' || *aLineStart>=' '))
//	{
//		// skip leading '[' as line can be a section from a .INI file
//		if (!myNumSections && *aLineStart=='[')
//			aLineStart++;
//		// get the current string as a new entry
//		mySections[myNumSections] = dest;
//
//		bool quotes=false;
//		if (*aLineStart==',')
//		{
//			*dest++ = 0;
//		}
//		else
//		{
//			while (aLineStart<aLineEnd && ((*aLineStart!=';' && *aLineStart!='=' && *aLineStart!=',' && *aLineStart!=']' && (*aLineStart>=' ' || *aLineStart=='\t')) || (quotes)))
//			{
//				if (*aLineStart == '\"')
//				{
//					quotes = quotes ? false : true;
//					aLineStart++;
//				}
//				else
//				{
//					if ((*aLineStart==' ' || *aLineStart=='\t') && !quotes )
//						break;
//					*dest++ = *aLineStart++;
//				}
//			}
//		}
//		// move to next section
//		if (dest!=mySections[myNumSections])
//		{
//			// store terminator and increment count
//			*dest++ = 0;
//			myNumSections++;
////			DEBUG_assertPrintf(dest<m_buffer+m_line_size, ("INIFILE_LINE_DATA -> Buffer overrun copying line from source"));
////			DEBUG_assertPrintf(m_num_sections<=m_max_sections, ("INIFILE_LINE_DATA -> Too many items on line"));
//			assert(dest<myBuffer+myLineSize);
//			assert(myNumSections<=myMaxSections);
//
//			// move to next data
//			aLineStart = SkipSpaces(aLineStart, aLineEnd);
//			if (aLineStart<aLineEnd && (*aLineStart==',' || *aLineStart=='=' || *aLineStart==']'))
//				aLineStart++;
//		}
//		aLineStart = SkipSpaces(aLineStart, aLineEnd);
//	}
//	// skip anything else on this line
//	while (aLineStart<aLineEnd && *aLineStart=='\t' || *aLineStart>=' ')
//		aLineStart++;
//	// skip line terminator
//	while (aLineStart<aLineEnd && *aLineStart!='\t' && *aLineStart<' ')
//		aLineStart++;
//	return (aLineStart);
//}
//
////////////////////////////////////////////////////////////////////////////////////
//// MC_IniLine::GetFloat()
////
//// Parameters:
////  aDataIndex		Data item index
////
//// Explanation:
////  This utility function returns a data item from the currently parsed line
////  and converts the ASCIIZ string to a floating point value.
////
//// Returns:
////  Floating point line data item.
////
////////////////////////////////////////////////////////////////////////////////////
//
//f32 MC_IniLine::GetFloat(s32 aDataIndex)
//{
//	const char *string = GetData(aDataIndex);
//#ifndef RELEASE
//	if (!string)
//	{
//		char buffer[1024];
//
//		buffer[0] = 0;
//		for (int32 i=0; i<myNumSections; i++)
//		{
//			strcat(buffer, GetSection(i));
//			strcat(buffer, " ");
//		}
////		DEBUG_abortPrintf(("INIFILE_LINE_DATA -> Getting float past end of line\n     line=%s", buffer));
//		assert(0);
//	}
//#endif
//
//	// get sign of string (most atof's don't like signs!)
//	if (string)
//	{
//		float32 result = 1.0f;
//		if (*string=='-')
//		{
//			result = -1.0f;
//			string++;
//		}
//		result *= (float32)atof(string);
//		return (result);
//	}
//	return 0.0f;
//}
//
////////////////////////////////////////////////////////////////////////////////////
//// MC_IniLine::GetBool()
////
//// Parameters:
////  aDataIndex		Data item index
////
//// Explanation:
////	TBD
////
//// Returns:
////	A bool32 value specified by the relevant line data item.
////
////////////////////////////////////////////////////////////////////////////////////
//
//bool MC_IniLine::GetBool(s32 aDataIndex)
//{
//	return GetTable(aDataIndex, lookup_bool)!=0;
//}
//
////////////////////////////////////////////////////////////////////////////////////
//// MC_IniLine::GetInt()
////
//// Parameters:
////  aDataIndex		Data item index
////
//// Explanation:
////  This utility function returns a data item from the currently parsed line
////  and converts the ASCIIZ string to an integer value.
////
//// Returns:
////  Integer line data item.
////
////////////////////////////////////////////////////////////////////////////////////
//
//s32 MC_IniLine::GetInt(s32 aDataIndex)
//{
//	const char *string = GetData(aDataIndex);
//
//#ifndef RELEASE
//	if (!string)
//	{
//		char buffer[1024];
//
//		buffer[0] = 0;
//		for (int32 i=0; i<myNumSections; i++)
//		{
//			strcat(buffer, GetSection(i));
//			strcat(buffer, " ");
//		}
////		DEBUG_abortPrintf(("INIFILE_LINE_DATA -> Getting int past end of line\n     line=%s", buffer));
//		assert(0);
//	}
//#endif
//
//	// get sign of string (most atof's don't like signs!)
//	if (string)
//	{
//		int32 result = 1;
//		if (*string=='-')
//		{
//			result = -1;
//			string++;
//		}
//		result *= (int32)atof(string);
//		return (result);
//	}
//	return 0;
//}
//
////////////////////////////////////////////////////////////////////////////////////
//// MC_IniLine::GetString()
////
//// Parameters:
////  aDataIndex		Data item index
////
//// Explanation:
////  This utility function returns a data item from the currently parsed line. This
////  function returns the address of an ASCIIZ string.
////
//// Returns:
////  Address of an ASCIIZ string.
////
////////////////////////////////////////////////////////////////////////////////////
//
//const char* MC_IniLine::GetString(s32 aDataIndex)
//{
//	const char* string = GetData(aDataIndex);
//#ifndef RELEASE
//	if (!string)
//	{
//		char buffer[1024];
//
//		buffer[0] = 0;
//		for (int32 i=0; i<myNumSections; i++)
//		{
//			strcat(buffer, GetSection(i));
//			strcat(buffer, " ");
//		}
////		DEBUG_abortPrintf(("INIFILE_LINE_DATA -> Getting string past end of line\n     line=%s", buffer));
//		assert(0);
//	}
//#endif
//
//	return (string);
//}
//
////////////////////////////////////////////////////////////////////////////////////
//// MC_IniLine::GetTable()
////
//// Parameters:
////  aDataIndex		Data item index
////	table			Address of label and value table to lookup values
////
//// Explanation:
////	This function will search the specified table for the specified data item. The
////	table contains a list of strings and matching values, the required data item
////	will be compared with each entry in turn and if it matches will return the
////	matching data value.
////  
//// Returns:
////	Value from table with matching label or 0 if not found. 
////
////////////////////////////////////////////////////////////////////////////////////
//
//// standard bool lookup table
//MC_IniLine::Lookup MC_IniLine::lookup_bool[]=
//{
//	// bool equivalents
//	{ "1",				1 },
//	{ "0",				0 },
//	{ "on",				1 },
//	{ "off",			0 },
//	{ "yes",			1 },
//	{ "no",				0 },
//	{ "true",			1 },
//	{ "false",			0 },
//	// terminator
//	{ 0 }
//};
//
//u32 MC_IniLine::GetTable(s32 aDataIndex, MC_IniLine::Lookup* aLookupTable)
//{
//	uint32 value = 0;
//	const char *string = GetString(aDataIndex);
//
//	while (aLookupTable->name)
//	{
//		if (!stricmp(string, aLookupTable->name))
//		{
//			value = aLookupTable->value;
//			break;
//		}
//		aLookupTable++;
//	}
//	return (value);
//}
//
////////////////////////////////////////////////////////////////////////////////////
//// MC_IniLine::Scroll()
////
//// Parameters:
////	aCount		Number of times to scroll
////
//// Explanation:
////	Scroll the sections of a parsed line and decrement the total count sections
////	available. Effectively this will scroll "data[0]" into "name", "data[1]" into
////	"data[0]" and so on.
////  
//// Returns:
////	None.
////
////////////////////////////////////////////////////////////////////////////////////
//
//void MC_IniLine::Scroll(s32 aCount)
//{
//	while (aCount>0 && myNumSections)
//	{
//		// move all sections back one
//		for (s32 i=1; i<myNumSections; i++)
//			mySections[i-1]=mySections[i];
//		// decrement number of sections
//		myNumSections--;
//		aCount--;
//	}
//}

//////////////////////////////////////////////////////////////////////////////////
// MC_IniFile::MC_IniFile()
//
// Parameters:
//  aFilename		Name of .INI file
//
// Explanation:
//  Constructor for the MC_IniFile class. This constructor loads in the specified
//  file and prepares for the processing of the file.
//  
//////////////////////////////////////////////////////////////////////////////////

MC_IniFile::MC_IniFile(
	const char* aFilename)
	: myFileBuffer(NULL)
	, mySize(0)
{
	// load .INI file
	MF_File file(aFilename);
	mySize = file.GetSize();
	if (mySize)
	{
		myFileBuffer = (u8*)MC_TempAlloc(mySize);
		assert(myFileBuffer);
		file.Read(myFileBuffer, mySize);
		file.Close();
	}
}

//////////////////////////////////////////////////////////////////////////////////
// MC_IniFile::~MC_IniFile()
//
// Explanation:
//  Destructor for the MC_IniFile class. This frees any allocated resources in 
//  preparation for deletion.
//  
//////////////////////////////////////////////////////////////////////////////////

MC_IniFile::~MC_IniFile()
{
	if (myFileBuffer)
	{
		MC_TempFree(myFileBuffer);
	}
}

//////////////////////////////////////////////////////////////////////////////////
// MC_IniFile::GetFloat()
//
// Parameters:
//  aKey		Key name
//
// Explanation:
//  This utility function returns a data item associated with key
//  and converts the ASCIIZ string to a floating point value.
//
// Returns:
//  Floating point line data item.
//
//////////////////////////////////////////////////////////////////////////////////

f32
MC_IniFile::GetFloat(
	const char*		aKey)
{
	MC_String* value = myValues.Find(aKey);
	if (!value)
	{
		return 0.0f;
	}

	const char* string = value->GetBuffer();
	
	// get sign of string (most atof's don't like signs!)
	if (string)
	{
		float32 result = 1.0f;
		if (*string=='-')
		{
			result = -1.0f;
			string++;
		}
		result *= (float32)atof(string);
		return (result);
	}
	return 0.0f;
}

s32
MC_IniFile::GetInt(
	const char*		aKey)
{
	MC_String* value = myValues.Find(aKey);
	if (!value)
	{
		return 0;
	}

	const char* string = value->GetBuffer();

	// get sign of string (most atof's don't like signs!)
	if (string)
	{
		int32 result = 1;
		if (*string=='-')
		{
			result = -1;
			string++;
		}
		result *= (int32)atoi(string);
		return (result);
	}
	return 0;
}

u32
MC_IniFile::GetUInt(
	const char*		aKey)
{
	MC_String* value = myValues.Find(aKey);
	if (!value)
	{
		return 0;
	}

	const char* string = value->GetBuffer();

	// get sign of string (most atof's don't like signs!)
	if (string)
	{
		int32 result = 1;
		if (*string == '-')
		{
			result = -1;
			string++;
		}
		result *= (int32)strtoul(string, NULL, 0);
		return (result);
	}
	return 0;
}

bool
MC_IniFile::GetBool(
	const char*		aKey)
{
	MC_String* value = myValues.Find(aKey);
	if (!value)
	{
		return false;
	}

	if (*value == "1"
		|| *value == "on"
		|| *value == "yes"
		|| *value == "true")
	{
		return true;
	}
	return false;
}

const char*
MC_IniFile::GetString(
	const char*		aKey)
{
	MC_String* value = myValues.Find(aKey);
	if (!value)
	{
		return NULL;
	}

	return value->GetBuffer();
}

//////////////////////////////////////////////////////////////////////////////////
// MC_IniFile::HasKey()
//
// Parameters:
//  aKey		Key name
//
// Explanation:
//  This utility function checks if INIFILE object contains data
//  associated with key.
//
// Returns:
//  True if key exists.
//
//////////////////////////////////////////////////////////////////////////////////

bool
MC_IniFile::HasKey(
	const char*		aKey)
{
	return myValues.Find(aKey) != 0;
}

//////////////////////////////////////////////////////////////////////////////////
// MC_IniFile::Process()

// Explanation:
//  The .INI file referenced by the INIFILE object is processed.
//  
// Returns:
//  True if INIFILE object processed without errors.
//
//////////////////////////////////////////////////////////////////////////////////

bool
MC_IniFile::Process()
{
	const char *begin = (char*)myFileBuffer;
	const char *end = begin + mySize;

	MC_String currentSection;
	for (const char* lineStart = begin; lineStart != end;)
	{
		lineStart = PrivSkipWhitespace(lineStart, end);
		if (lineStart == end)
		{
			break;
		}

		// Comment
		if (*lineStart == ';'
			|| *lineStart == '#')
		{
			lineStart = PrivLineEnd(lineStart, end);
			continue;
		}

		// Unexpected character
		if (*lineStart == '='
			|| *lineStart == ']')
		{
			return false;
		}

		// Start of section
		if (*lineStart == '[')
		{
			++lineStart;
			bool sectionClosed = false;
			const char* sectionEnd = PrivSectionNameEnd(lineStart, end, sectionClosed);
			if (!sectionClosed)
			{
				return false;
			}

			currentSection = MC_String(lineStart, sectionEnd - lineStart).TrimLeft().TrimRight() + '.';
			lineStart = PrivLineEnd(lineStart, end);
			continue;
		}

		// Value
		const char* equals = PrivValueDelimeter(lineStart, end);
		const char* lineEnd = PrivLineEnd(equals, end);
		if (equals == end || equals + 1 == end)
		{
			break;
		}

		MC_String key = currentSection + MC_String(lineStart, equals - lineStart).TrimLeft().TrimRight();
		MC_String value = MC_String(equals + 1, lineEnd - equals).TrimLeft().TrimRight();
		myValues.Insert(key, value);
		lineStart = (lineEnd == end) ? lineEnd : lineEnd + 1;
	}

	return true;
}

const char*
MC_IniFile::PrivSkipWhitespace(
	const char* aBegin,
	const char* aEnd)
{
	while (aBegin < aEnd
		&& (*aBegin == ' '
			|| *aBegin == '\t'
			|| *aBegin == '\r'
			|| *aBegin == '\n'))
	{
		aBegin++;
	}
	return (aBegin);
}

const char*
MC_IniFile::PrivLineEnd(
	const char* aBegin,
	const char* aEnd)
{
	while (aBegin < aEnd && *aBegin != '\n')
	{
		aBegin++;
	}
	return (aBegin);
}

const char*
MC_IniFile::PrivValueDelimeter(
	const char*		aBegin,
	const char*		aEnd)
{
	while (aBegin < aEnd && *aBegin != '=')
	{
		aBegin++;
	}
	return (aBegin);
}

const char*
MC_IniFile::PrivSectionNameEnd(
	const char*		aBegin,
	const char*		aEnd,
	bool&			aOutSectionClosed)
{
	while (aBegin < aEnd && *aBegin != '\n' && *aBegin != ']')
	{
		aBegin++;
	}
	aOutSectionClosed = (aBegin != aEnd) && *aBegin == ']';
	return (aBegin);
}

/* end of file */
