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
#ifndef MF_FILE_H
#define MF_FILE_H


#include "mc_string.h"
#include "MC_GrowingArray.h"

template <class Type> class MC_Vector2;
template <class Type> class MC_Vector3;
template <class Type> class MC_Vector4;
class MC_Matrix33;
class MC_Matrix44;
class MF_SDF;
struct MF_SDFFileInfo;
struct MF_SDFExtraFileInfo;

struct MF_FileDatTime
{
	unsigned int myYear; // 2000-2133
	unsigned int myMonth; // 1-12
	unsigned int myDay; // 1-31
	unsigned int myHour; // 0-23
	unsigned int myMinute; // 0-59
	unsigned int mySecond; // 0-59
};


class MF_GetDirEntryData
{
public:
	MF_GetDirEntryData() { myName = NULL; }
	~MF_GetDirEntryData() { if(myName) delete [] myName; }

	char* myName;
	unsigned int myFileSize;
	unsigned int myFileDateTime;
	bool myIsDirectoryFlag;

	MF_GetDirEntryData* myNextPtr;
};


class MF_GetDirData
{
public:
	MF_GetDirData() { myFirstEntry = NULL; }
	~MF_GetDirData();

	void AddFile(const char* aName, unsigned int aFileSize, unsigned int aFileDateTime, bool anIsDirectoryFlag);

	MF_GetDirEntryData* myFirstEntry;
};

class MF_File
{
	friend class MF_SDF;
private:
	struct MF_FileMutexLock
	{
		MF_FileMutexLock();
		~MF_FileMutexLock();
		bool myLockFlag;
	};

public:
	typedef MC_StaticString<260> Path;

	enum MF_Flags
	{
		MFILE_NOT_OPEN		= 0,
		OPEN_READ			= (1 << 0),
		OPEN_WRITE			= (1 << 1),
		OPEN_APPEND			= (1 << 2),
		OPEN_KEEPEXISTING	= (1 << 3), // only used when opening a file for writing
		OPEN_STREAMING		= (1 << 4),
		OPEN_COMPRESSED		= (1 << 5),
		OPEN_BIGENDIAN		= (1 << 6), // the file we're reading is big endian. note: can't write big endian right now
		SDF_HANDLES_READS	= (1 << 7), // custom packed format, must call SDF at each read
	};

	enum MF_Position
	{
		POS_BEGINNING,
		POS_CURRENT,
		POS_END
	};

	enum MF_Errors
	{
		ERROR_NO_ERROR,
		ERROR_FILENOTFOUND,
		ERROR_READFAILED,
		ERROR_WRITEFAILED,
		ERROR_SDF_WRONGVERSION,
		ERROR_SDF_WRONGSIZE, // sdf specific to verify correctness
		ERROR_UNABLETOUNPACKDATA,
		ERROR_FILENOTOPEN,
		ERROR_FILETOOBIG,
		NUM_ERRORS
	};

private:
	
	void Constructor();
	
	unsigned int myFlags;
	unsigned char* myBuffer;
	unsigned int myFileSize;
	unsigned int myFilePos;
	unsigned int myFileDate;
	unsigned int myBufferSize;
	void* myFileHand;
	MC_String myName;
	bool myReadLineCutFlag;

	unsigned char* myStreamBuffer;
	int myStreamBufferStart;
	int myStreamBufferEnd;

	MF_SDF* mySDF;											// pointer to parent SDF (currently only set if using CompressMethod::CUSTOM)
	MF_SDFFileInfo* myCustomSDFReadSDFFileInfo;				// pointer to MF_SDFFileInfo
	MF_SDFExtraFileInfo* myCustomSDFReadSDFExtraFileInfo;	// pointer to MF_SDFExtraFileInfo
	unsigned char* myCustomSDFReadHeader;
	unsigned int myCustomSDFReadHeaderSize;
	unsigned char* myCustomSDFReadBuffer;
	unsigned int myCustomSDFReadStart;
	unsigned int myCustomSDFReadBufferLength;

public:

	MF_File();
	MF_File(const char* aFileName, unsigned int aMode = OPEN_READ);
	~MF_File();

	bool IsOpen() { return myFlags != MFILE_NOT_OPEN; }
	bool IsEOF() { return myFilePos >= myFileSize; }
	bool IsBigEndian() const { return (myFlags & OPEN_BIGENDIAN) != 0; }

#if IS_ENDIAN(ENDIAN_BIG)
	bool IsOtherEndian() const { return (myFlags & OPEN_BIGENDIAN) == 0; }
#else
	bool IsOtherEndian() const { return (myFlags & OPEN_BIGENDIAN) != 0; }
#endif

	bool Open(const char* aFileName, unsigned int aMode = OPEN_READ);
	bool Close();

	void* GetBuffer() { return myBuffer; }
	void* GetFileHand() { return myFileHand; }
	const MC_String& GetName() const { return myName; }

	bool Read(void* someData, unsigned int* aLength); // read a block of someData
	bool Read(void* someData, unsigned int aLength); // read a block of someData

	template <class T>
	bool Read(T& aT)
	{
		bool retval = Read(&aT, sizeof(aT));
		return retval;
	}

	// Use overloading to help with the big endian compatibility
	template <class T>
	bool ReadAndFixEndianess(T& aT)
	{
		bool retval = Read(&aT, sizeof(aT));
	
		if(IsOtherEndian())
			aT = MC_Endian::Swap(aT);

		return retval;
	}
	bool Read(double& aT)
	{
		u64 t;
		bool retval = Read(t);	// Read (and swap if other endian) as uint64, to avoid float exceptions.

		aT = *(double*)&t;
		return retval;
	}
	bool Read(float& aT)
	{
		u32 t;
		bool retval = Read(t);	// Read (and swap if other endian) as uint32, to avoid float exceptions.

		aT = *(float*)&t;
		return retval;
	}
	bool Read(s64& aT)
	{
		return ReadAndFixEndianess(*(u64*)&aT);
	}
	bool Read(u64& aT)
	{
		return ReadAndFixEndianess(aT);
	}
	bool Read(int& aT)
	{
		return ReadAndFixEndianess(*(unsigned int*)&aT);
	}
	bool Read(unsigned int& aT)
	{
		return ReadAndFixEndianess(aT);
	}
	bool Read(short& aT)
	{
		return ReadAndFixEndianess(*(unsigned short*)&aT);
	}
	bool Read(unsigned short& aT)
	{
		return ReadAndFixEndianess(aT);
	}
	bool Read(long& aT)
	{
		return ReadAndFixEndianess(*(u32*)&aT);
	}
	bool Read(unsigned long& aT)
	{
		return ReadAndFixEndianess(*(u32*)&aT);
	}

	template<typename Type>
	bool Read(MC_Vector2<Type>& aVec)
	{
		bool retval = Read(&aVec, sizeof(Type)*2);
		if(IsOtherEndian())
			MC_Endian::Swap((Type*)&aVec, 2);
		return retval;
	}
	template<typename Type>
	bool Read(MC_Vector3<Type>& aVec)
	{
		bool retval = Read(&aVec, sizeof(Type)*3);
		if(IsOtherEndian())
			MC_Endian::Swap((Type*)&aVec, 3);
		return retval;
	}
	template<typename Type>
	bool Read(MC_Vector4<Type>& aVec)
	{
		bool retval = Read(&aVec, sizeof(Type)*4);
		if(IsOtherEndian())
			MC_Endian::Swap((Type*)&aVec, 4);
		return retval;
	}
	bool Read(MC_Matrix33& aMat)
	{
		bool retval = Read(&aMat, sizeof(float)*3*3);
		if(IsOtherEndian())
			MC_Endian::Swap((float*)&aMat, 3*3);
		return retval;
	}
	bool Read(MC_Matrix44& aMat)
	{
		bool retval = Read(&aMat, sizeof(float)*4*4);
		if(IsOtherEndian())
			MC_Endian::Swap((float*)&aMat, 4*4);
		return retval;
	}
	template<typename T>
	bool ReadArray(T* aPointer, int aNumElements)
	{
		bool retval = Read((void*)aPointer, aNumElements * sizeof(T));

		if(IsOtherEndian())
			MC_Endian::Swap(aPointer, aNumElements);

		return retval;
	}
	template<typename Type>
	bool ReadArray(MC_Vector2<Type>* aPointer, int aNumElements)
	{
		bool retval = Read((void*)aPointer, aNumElements * sizeof(MC_Vector2<Type>));

		if(IsOtherEndian())
			MC_Endian::Swap((Type*)aPointer, aNumElements*2);

		return retval;
	}
	template<typename Type>
	bool ReadArray(MC_Vector3<Type>* aPointer, int aNumElements)
	{
		bool retval = Read((void*)aPointer, aNumElements * sizeof(MC_Vector3<Type>));

		if(IsOtherEndian())
			MC_Endian::Swap((Type*)aPointer, aNumElements*3);

		return retval;
	}
	template<typename Type>
	bool ReadArray(MC_Vector4<Type>* aPointer, int aNumElements)
	{
		bool retval = Read((void*)aPointer, aNumElements * sizeof(MC_Vector4<Type>));

		if(IsOtherEndian())
			MC_Endian::Swap((Type*)aPointer, aNumElements*4);

		return retval;
	}

	static const int TEMP_BUFFER_SIZE = 1024;

	void ReadASCIIZ(char* someData, unsigned int aMaxLength = 256); // reads a NULL-terminated string
template <unsigned int StringLength>
	void ReadASCIIZ(MC_StaticString<StringLength>& aString) { ReadASCIIZ(aString.GetBuffer(), StringLength); } // reads a NULL-terminated string
	void ReadASCIIZ(MC_String& aString); // reads a NULL-terminated string

	void ReadString(char* someData, unsigned int aMaxLength = 256)
	{
		unsigned short len;
		Read(len);
		assert(len < aMaxLength);
		Read(someData, len);
		someData[len] = 0;
	} // reads a string
template <unsigned int StringLength>
	void ReadString(MC_StaticString<StringLength>& aString) { ReadString(aString.GetBuffer(), StringLength); } // reads a string
	void ReadString(MC_String& aString)
	{
		unsigned short len;
		Read( len );
		aString.Reserve( len );
		Read( aString.GetBuffer(), len );
		aString[(int)len] = 0;
	} // reads a string

	void ReadStringLong( MC_String& aString )
	{
		unsigned int len;
		Read( len );
		aString.Reserve( len );
		Read( aString.GetBuffer(), len );
		aString[(int)len] = 0;
	} // reads a string

	void ReadLocString(MC_LocString& aString) { unsigned short len; Read(len); aString.Reserve(len+1); Read((void*) (const MC_LocChar*) aString, len * sizeof(MC_LocChar)); } // reads a string
template <unsigned int StringLength>
	void ReadLocString(MC_StaticLocString<StringLength>& aString) { ReadLocString(aString.GetBuffer(), StringLength); } // reads a string
	void ReadLocString(MC_LocChar* aString, unsigned int aMaxLength = 256) { unsigned short len; Read(len); assert(len < aMaxLength); Read(aString, len * sizeof(MC_LocChar)); aString[len] = 0; } // reads a string

	void ReadLine(char* someData, unsigned int aMaxLength = 256); // reads a line and removes beginning and trailing whitespaces
template <unsigned int StringLength>
	void ReadLine(MC_StaticString<StringLength>& aString) { ReadLine(aString.GetBuffer(), StringLength); } // reads a line and removes beginning and trailing whitespaces
	void ReadLine(MC_String& aString); // reads a line and removes beginning and trailing whitespaces

	void ReadLineTrue(char* someData, unsigned int aMaxLength = 256); // reads a line, without formating it

	void ReadLineLocalized(MC_LocChar* someData, unsigned int aMaxLength = 256); // reads a line and removes beginning and trailing whitespaces
	void ReadLineTrueLocalized(MC_LocChar* someData, unsigned int aMaxLength = 256); // reads a line, without formating it

	void ReadWord(char* someData, unsigned int aMaxLength = 256); // reads a part of a line (a word)

	bool Write(const void* someData, unsigned int aLength); // write a block of someData

template <class T>
	const bool Write(T& aT) { return Write(&aT, sizeof(aT)); }

	// Use overloading to help with the big endian compatibility
	template <class T>
	bool WriteAndFixEndianess(T& aT)
	{
		if(IsOtherEndian())
			aT = MC_Endian::Swap(aT);

		bool retval = Write(&aT, sizeof(aT));
	
		if(IsOtherEndian())
			aT = MC_Endian::Swap(aT);

		return retval;
	}
	bool Write(double& aT)
	{
		u64 t = *(u64*)&aT;
		return Write(t);	// Read (and swap if other endian) as uint64, to avoid float exceptions.
	}
	bool Write(float& aT)
	{
		u32 t = *(u32*)&aT;
		return Write(t);	// Read (and swap if other endian) as uint32, to avoid float exceptions.
	}
	bool Write(s64& aT)
	{
		return WriteAndFixEndianess(*(u64*)&aT);
	}
	bool Write(u64& aT)
	{
		return WriteAndFixEndianess(aT);
	}
	bool Write(int& aT)
	{
		return WriteAndFixEndianess(*(unsigned int*)&aT);
	}
	bool Write(unsigned int& aT)
	{
		return WriteAndFixEndianess(aT);
	}
	bool Write(short& aT)
	{
		return WriteAndFixEndianess(*(unsigned short*)&aT);
	}
	bool Write(unsigned short& aT)
	{
		return WriteAndFixEndianess(aT);
	}
	bool Write(long& aT)
	{
		return WriteAndFixEndianess(*(u32*)&aT);
	}
	bool Write(unsigned long& aT)
	{
		return WriteAndFixEndianess(*(u32*)&aT);
	}

	// SWFM:SWR - start adding Write types
	template<typename Type>
	bool Write(MC_Vector3<Type>& aVec)
	{
		if(IsOtherEndian())
			MC_Endian::Swap((Type*)&aVec, 3);
		bool retval = Write(&aVec, sizeof(Type)*3);
		if(IsOtherEndian())
			MC_Endian::Swap((Type*)&aVec, 3);
		return retval;
	}

	// SWFM:SWR - added WriteArray
	template<typename T>
	bool WriteArray(T* aPointer, int aNumElements)
	{
		if(IsOtherEndian())
			MC_Endian::Swap(aPointer, aNumElements);

		bool retval = Write((void*)aPointer, aNumElements * sizeof(T));

		// wasteful to flip twice, but shouldn't be used in proper release builds
		if(IsOtherEndian())
			MC_Endian::Swap(aPointer, aNumElements);

		return retval;
	}

	void WriteLine(const char* aString) { Write(aString, (unsigned int) strlen(aString)); Write("\r\n", 2); } // write a string and adds \n\r
	void WriteASCIIZ(const char* aString) { Write(aString, (unsigned int) strlen(aString) + 1); } // write a null-terminated string
	void WriteString(const char* aString) { unsigned short len; len = (unsigned short) strlen(aString); Write(len); Write(aString, len); } // write a string

	void WriteStringLong( const char* aString )
	{
		unsigned int len;
		len = (unsigned int) strlen( aString );
		Write( len );
		Write( aString, len );
	} // write a string

	void WriteLocString(MC_LocString& aString) { unsigned short len; len = (unsigned short) aString.GetLength(); Write(len); Write((void*) (const MC_LocChar*) aString, len * sizeof(MC_LocChar)); } // write a string
	void WriteLocString(const MC_LocChar* aString) { unsigned short len; len = (unsigned short) wcslen(aString); Write(len); Write(aString, len * sizeof(MC_LocChar)); } // write a string

	bool SetFilePos(int aPosition, MF_Position aReference = POS_BEGINNING);
	unsigned int GetPos() const;
	unsigned int GetSize() const { return myFileSize; }
	unsigned int GetDate() const { return myFileDate; }

	static bool ConvertExtendedDateTimeToCompressedForm(const MF_FileDatTime& anExtendedDateTime, unsigned int& aReturnDateTime);
	static void ConvertCompressedDateTimeToExtendedForm(unsigned int aDateTime, MF_FileDatTime& aReturnExtendedDateTime);

	static bool ExistFile(const char* aFileName) { unsigned int date, size; return GetFileInfo(aFileName, date, size); }
	static bool ExistDir(const char* aDirName);

	static bool DelTree(const char* aDir);
	static bool DelFile(const char* aFile);

	static bool GetFileInfo(const char* aFileName, unsigned int& aReturnDateTime, unsigned int& aReturnSize, char* aSDFName = NULL);
	static bool GetFileDate(const char* aFileName, unsigned int& aReturnDateTime) { unsigned int siz; return GetFileInfo(aFileName, aReturnDateTime, siz); }

	static bool RenameFile(const char* anOldFileName, const char* aNewFileName);

	static bool CreatePath(const char* aFileName);

	static bool GetDir(const char* aDir, MF_GetDirData* someOutData);

	static bool CopyFile(const char* aSourceFileName, const char* aDestinationFileName);
	static bool CreatePathValidated(const char* aPath);

	// file name convenience functions

	// returns the extension in the specified path.
	// works with ".format", "C:\\path\\to\\file.format" etc. do not delete
	// the returned array as it points to the argument array. does not require
	// the file to exist
	static const char* ExtractExtension(const char* aPath);

	// as above but returns file name in path.
	// example: "C:\\path\\to\\file.format" returns "file.format"
	static const char* ExtractFileName(const char* aPath);

	// extracts the directory from a full path. example: "C:\\path\\to\\file.format"
	// returns "C:\\path\\to". does not require the directory to exist
	static char* ExtractDirectory(char* aDirectoryDest, const char* aFullPath);

	// removes the extension from a full path
	static char* RemoveExtensionFromPath(char* aDest, const char* aFullPath);

	// replaces the extension with the given: ReplaceExtension(buf, "myfile.tga", "jpg") returns "myfile.jpg"
	// can handle extensions both with or without a dot prepended.
	static char* ReplaceExtension(char* aDest, const char* aFullPath, const char* aNewExtension);

	static char* ExtractUpperDirectory(char* aDirectoryDest, const char* aFullPath);

	// generates a unique filename (not file). if aBaseDir is NULL, then the user's temp
	// directory will be used, else the specified directory.
	// example: MakeTempFileName(strbuf, "txt"); can generate
	// "C:\\Documents and Settings\\User Name\\Local Settings\\Temp\\fe3sr000.txt" etc...
	static void MakeTempFileName(char* aPathDestination, const char* anExtension = NULL, const char* aBaseDir = NULL);

	// case insensitive strcmp()
	static bool CompareStrings(const char* aString1, const char* aString2);

	// checks the whether the path is equal to the specified extension
	static bool ExtensionEquals(const char* aPath, const char* anExtension)
	{
		return CompareStrings(ExtractExtension(aPath), anExtension);
	}
	//returns files matching search string, only supports strings on form dir/*.ext
	static MC_GrowingArray<MC_String> FindFiles(MC_String aSearchString, bool aRecursive);

	static void SetLogMissingFiles(bool anEnableFlag, const char* aLogFilename);
	static void LogMissingFile(const char* aFilename);

	static bool ourDebugDisableZipDds;			// For measurements
	static bool ourURLCommentFix;				// fix for url's
	static MC_String ourMissingFileLog;
	static MC_GrowingArray<MC_String> ourMissingFiles;
};

#endif // not MF_FILE_H
