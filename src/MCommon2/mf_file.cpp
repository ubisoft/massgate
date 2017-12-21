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

#include <string.h>
#include <direct.h>
#include "mf_file.h"
#include "mf_file_platform.h"	// external header file for platform specific components
#include "mp_pack.h"
#include "mc_debug.h"
#include "mt_mutex.h"
#include "mi_time.h"

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif // DEBUG_NEW

#define STREAM_BUFFER_SIZE 16384

bool MF_File::ourDebugDisableZipDds = false;		// can be disabled by app to measure load performance gain
bool MF_File::ourURLCommentFix		= false;			// this will enable test for ':' sign before // in ReadLine
MC_String MF_File::ourMissingFileLog;
MC_GrowingArray<MC_String> MF_File::ourMissingFiles(2,64,true);

static unsigned int ReadFileDateTimeFromHandle(FILETIME& aFT)
{
	MF_FileDatTime mffdt;
	SYSTEMTIME st;
	unsigned int dateTime;

	dateTime = 0;
	if(FileTimeToSystemTime(&aFT, &st))
	{
		mffdt.myYear = st.wYear;
		mffdt.myMonth = st.wMonth;
		mffdt.myDay = st.wDay;
		mffdt.myHour = st.wHour;
		mffdt.myMinute = st.wMinute;
		mffdt.mySecond = st.wSecond;

		MF_File::ConvertExtendedDateTimeToCompressedForm(mffdt, dateTime);
	}

	return dateTime;
}


MF_GetDirData::~MF_GetDirData()
{
	MF_GetDirEntryData* ent = myFirstEntry;
	MF_GetDirEntryData* ent2;

	while(ent)
	{
		ent2 = ent->myNextPtr;
		delete ent;
		ent = ent2;
	}
}


void MF_GetDirData::AddFile(const char* aName, unsigned int aFileSize, unsigned int aFileDateTime, bool anIsDirectoryFlag)
{
	MF_GetDirEntryData* ent = myFirstEntry;

	while(ent)
	{
		if(stricmp(aName, ent->myName) == 0)
			return; // already exists - ignore it

		ent = ent->myNextPtr;
	}

	ent = new MF_GetDirEntryData;
	if(!ent)
		return;

	ent->myName = new char[strlen(aName) + 1];
	strcpy(ent->myName, aName);
	ent->myFileSize = aFileSize;
	ent->myFileDateTime = aFileDateTime;
	ent->myIsDirectoryFlag = anIsDirectoryFlag;
	ent->myNextPtr = myFirstEntry;
	myFirstEntry = ent;
}


struct MF_FileMutex
{
	MF_FileMutex()
	{
		ourState = 1;
	}

	~MF_FileMutex()
	{
		ourState = 2;
	}

	MT_Mutex myMutex;
	static volatile int ourState;
};

volatile int MF_FileMutex::ourState = 0;
static MF_FileMutex locFileMutex;

// The lock is needed because SDF if not thread safe
MF_File::MF_FileMutexLock::MF_FileMutexLock()
{
	myLockFlag = (MF_FileMutex::ourState == 1);
	if(myLockFlag)
		locFileMutex.myMutex.Lock();
}

MF_File::MF_FileMutexLock::~MF_FileMutexLock()
{
	if(myLockFlag && MF_FileMutex::ourState == 1)
		locFileMutex.myMutex.Unlock();
}

MF_File::MF_File()
{
	Constructor();
}

MF_File::MF_File(const char* aFileName, unsigned int aMode)
{
	MF_FileMutexLock lock;

	Constructor();

	Open(aFileName, aMode);
}

void MF_File::Constructor()
{
	myFlags = MFILE_NOT_OPEN;
	myBuffer = NULL;
	myFileHand = INVALID_HANDLE_VALUE;
	myFileSize = 0;
	myFilePos = 0;
	myFileDate = 0;
	myBufferSize = 0;
	myReadLineCutFlag = false;
	myStreamBuffer = 0;
	myStreamBufferStart = 0;
	myStreamBufferEnd = 0;
	
	mySDF = NULL;
	myCustomSDFReadSDFFileInfo = NULL;
	myCustomSDFReadSDFExtraFileInfo = NULL;
	myCustomSDFReadHeader = NULL;
	myCustomSDFReadHeaderSize = 0;
	myCustomSDFReadBuffer = NULL;
	myCustomSDFReadStart = 0;
	myCustomSDFReadBufferLength = 0;
}


MF_File::~MF_File()
{
	MF_FileMutexLock lock;

	if(myFlags != MFILE_NOT_OPEN)
		Close();

	delete [] myBuffer;
	myBuffer = NULL;

	delete [] myStreamBuffer;
	myStreamBuffer = NULL;

	delete [] myCustomSDFReadHeader;
	delete [] myCustomSDFReadBuffer;
	myCustomSDFReadHeader = NULL;
	myCustomSDFReadBuffer = NULL;
	myCustomSDFReadSDFFileInfo = NULL;	// pointer only
}


bool MF_File::Open(const char* aFileName, unsigned int aMode)
{
	if (aFileName == 0 || *aFileName == 0)
		return false;

	MF_FileMutexLock lock;

	myName=aFileName;

	assert(!(aMode & OPEN_COMPRESSED) || !(aMode & OPEN_STREAMING)); // can't both stream and compress the data

	Close();

	if(aMode & OPEN_APPEND)
		aMode |= (OPEN_WRITE | OPEN_KEEPEXISTING); // if appending, then writing by definition

	myFlags = aMode;

	BY_HANDLE_FILE_INFORMATION hinf;
	DWORD i;

#ifndef _RELEASE_
	const MI_TimeUnit t1 = MI_Time::GetExactTime();
#endif

	if(aMode & OPEN_WRITE)
	{
		assert(!(aMode & OPEN_STREAMING)); // can't stream right now
		assert(!(aMode & OPEN_COMPRESSED)); // can't compress right now

		CreatePath(aFileName);

		// SWFM:SWR - for PC pad builds we need to write over read only files
		#if IS_PAD_BUILD
			if (!(aMode&OPEN_KEEPEXISTING))
				SetFileAttributes(aFileName, FILE_ATTRIBUTE_NORMAL);
		#endif

		if((myFileHand = PLATFORM::CreateFile(aFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, (aMode & OPEN_KEEPEXISTING ? OPEN_ALWAYS : CREATE_ALWAYS), 0, NULL)) == INVALID_HANDLE_VALUE)
		{
			myFlags = MFILE_NOT_OPEN;
			return false;
		}

		myBufferSize = myFileSize = GetFileSize(myFileHand, NULL);

		if(myFileSize > 0)
		{
			myBuffer = new unsigned char[myFileSize];
			if(!myBuffer)
			{
				myFlags = MFILE_NOT_OPEN;
				return false;
			}

			if(!ReadFile(myFileHand, myBuffer, myFileSize, &i, NULL) || i != (unsigned) myFileSize)
			{
				myFileSize = i;
				myFlags = MFILE_NOT_OPEN;
				return false;
			}
		}

		if (GetFileInformationByHandle(myFileHand, &hinf))
			myFileDate = ReadFileDateTimeFromHandle(hinf.ftLastWriteTime);
		else
			myFileDate = 0;
	}
	else
	{
		
		if((myFileHand = PLATFORM::CreateFile(aFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
		{		
			myFlags = MFILE_NOT_OPEN;
			LogMissingFile(aFileName);	// For debugging, for example making sure DS data has all files it needs
			return false; // no file found			
		}
		else
		{
			myBufferSize = myFileSize = GetFileSize(myFileHand, NULL);

			if (GetFileInformationByHandle(myFileHand, &hinf))
				myFileDate = ReadFileDateTimeFromHandle(hinf.ftLastWriteTime);
			else
				myFileDate = 0;

			if(!(aMode & OPEN_STREAMING))
			{
				if(myFileSize > 0)
				{
					myBuffer = new unsigned char[myFileSize];
					if(!myBuffer)
					{
						myFlags = MFILE_NOT_OPEN;
						return false;
					}

					if(!ReadFile(myFileHand, myBuffer, myFileSize, &i, NULL) || i != (unsigned) myFileSize)
					{
						myFileSize = i;
						myFlags = MFILE_NOT_OPEN;
						return false;
					}
				}

				CloseHandle(myFileHand);
				myFileHand = INVALID_HANDLE_VALUE;
			}
		}
		if(aMode & OPEN_STREAMING)
		{
			assert(!myStreamBuffer);
			myStreamBufferStart = 0;
			myStreamBufferEnd = 0;
		}
	}

	if (!myBuffer && !myFileHand)
		myFlags = MFILE_NOT_OPEN;

	myFilePos = ((myFlags & OPEN_APPEND) ? myFileSize : 0);

	int millis = 0;

#ifndef _RELEASE_
	const MI_TimeUnit t2 = MI_Time::GetExactTime();
	MI_TimeUnit delta = 0;
	if(t2 >= t1)
		delta = t2 - t1;

	const float seconds = MI_Time::ConvertTimeToSeconds(delta);
	if(seconds < 100000.0f)
		millis = int(0.5f + seconds*1000.0f);
#endif

	MC_Debug::SetLastOpenedFile(myName, myFileSize, (myFlags & OPEN_WRITE) != 0, (myFlags & OPEN_STREAMING) != 0, millis);

	return true;
}

bool MF_File::Close()
{
	MF_FileMutexLock lock;

	DWORD i;
	//unsigned char* buf;

	myReadLineCutFlag = false;

	if(myFlags == MFILE_NOT_OPEN)
	{
		return false;
	}

	if((myFlags & OPEN_WRITE) && myFileHand != INVALID_HANDLE_VALUE)
	{
		SetFilePointer(myFileHand, 0, 0, FILE_BEGIN);

		if(myBuffer)
		{
			/*if(myFlags & OPEN_COMPRESSED)
			{
				unsigned int compSiz;
				buf = MP_Pack::CompressLZMA(myBuffer, myFileSize, compSiz);
				if(buf)
					WriteFile(myFileHand, buf, compSiz, &i, NULL);

				delete [] myBuffer;
				myBuffer = NULL;

				delete [] buf;
			}
			else
			{*/
				WriteFile(myFileHand, myBuffer, myFileSize, &i, NULL); // CHANGE LATER??? write in 1mb chunks at most?
			//}
		}
	}

	delete [] myBuffer;
	myBuffer = NULL;

	delete [] myStreamBuffer;
	myStreamBuffer = NULL;

	if(myFileHand != INVALID_HANDLE_VALUE)
	{
		if(myFlags & OPEN_WRITE)
			SetEndOfFile(myFileHand);

		CloseHandle(myFileHand);
		myFileHand = INVALID_HANDLE_VALUE;
	}

	myFlags = MFILE_NOT_OPEN;

	return true;
}


bool MF_File::SetFilePos(int aPosition, MF_Position aReference)
{
	int oldPos = (int) myFilePos;

	assert(myFlags != MFILE_NOT_OPEN);

	switch(aReference)
	{
	case POS_BEGINNING:
		myFilePos = aPosition;
		break;

	case POS_CURRENT:
		myFilePos += aPosition;
		break;

	case POS_END:
		myFilePos = myFileSize - aPosition;
		break;

	default:
		return false;
	}

	assert(myFilePos <= myFileSize);

	if(myFlags & OPEN_STREAMING)
	{
		SetFilePointer(myFileHand, myFilePos - oldPos - (myStreamBufferEnd - myStreamBufferStart), NULL, FILE_CURRENT); // move relative current position - this works even if the file's offset 0 is the actual start of the file (sdf's)
		myStreamBufferStart = 0;
		myStreamBufferEnd = 0;
	}

	return true;
}


unsigned int MF_File::GetPos() const
{
	assert(myFlags != MFILE_NOT_OPEN);

	return myFilePos;
}


bool MF_File::Read(void* someData, unsigned int* aLength)
{
	assert(myFlags & OPEN_READ);

	if(*aLength == 0)
		return false;

	if(myFilePos + *aLength >= myFileSize)
	{
		*aLength = myFileSize - myFilePos;
		if(*aLength <= 0)
			return false;
	}

	return Read(someData, *aLength);
}


bool MF_File::Read(void* someData, unsigned int aLength)
{
	DWORD dw;

	assert(myFlags & OPEN_READ);

	if(aLength == 0)
		return false;

	if(myFilePos + aLength >= myFileSize)
	{
		aLength = myFileSize - myFilePos;
		if(aLength <= 0)
			return false;
	}	

	bool retval = true;
	if(myFlags & OPEN_STREAMING)
	{
		assert(myFileHand != INVALID_HANDLE_VALUE);

		unsigned int buflen = myStreamBufferEnd - myStreamBufferStart;
		if(buflen >= aLength)
		{
			assert(myStreamBuffer);
			memcpy(someData, myStreamBuffer + myStreamBufferStart, aLength);
			myStreamBufferStart += aLength;
		}
		else
		{
			if(buflen)
			{
				assert(myStreamBuffer);
				memcpy(someData, myStreamBuffer + myStreamBufferStart, buflen);
				someData = (char*)someData + buflen;
				myFilePos += buflen;
				aLength -= buflen;
				myStreamBufferEnd = myStreamBufferStart;
			}

			if(aLength > STREAM_BUFFER_SIZE)
			{
				if(!ReadFile(myFileHand, someData, aLength, &dw, NULL) || dw != aLength)
				{
					retval = false;
				}
			}
			else
			{
				if(!myStreamBuffer)
					myStreamBuffer = new unsigned char[STREAM_BUFFER_SIZE];

				assert(myStreamBuffer);

				unsigned int bytesToRead = STREAM_BUFFER_SIZE;
				if(bytesToRead > myFileSize - myFilePos)
					bytesToRead = myFileSize - myFilePos;

				if(!ReadFile(myFileHand, myStreamBuffer, bytesToRead, &dw, NULL) || dw != bytesToRead)
				{
					retval = false;
				}
				else
				{
					memcpy(someData, myStreamBuffer, aLength);
					myStreamBufferStart = aLength;
					myStreamBufferEnd = bytesToRead;
				}
			}
		}
	}
	else
		memcpy(someData, myBuffer + myFilePos, aLength);

	myFilePos += aLength;
	return retval;
}


void MF_File::ReadWord(char* someData, unsigned int aMaxLength)
{
	unsigned int i = 0;
	unsigned char ch;

	assert(myFlags & OPEN_READ);

	while(i < aMaxLength - 1 && myFilePos < myFileSize)
	{
		Read(someData[i]);
		if(someData[i] == ' ' || someData[i] == '\t' || someData[i] == '\n' || someData[i] == '\r')
		{
			ch = someData[i];
			while(myFilePos < myFileSize && (ch == '\r' || ch == '\n' || ch == ' ' || ch == '\t'))
				Read(ch);

			if(ch != '\r' && ch != '\n' && ch != ' ' && ch != '\t')
				myFilePos--;
			break;
		}
		i++;
	}
	someData[i] = 0;
}


void MF_File::ReadASCIIZ(char* someData, unsigned int aMaxLength)
{
	unsigned int i = 0;

	assert(myFlags & OPEN_READ);
	assert(myFilePos < myFileSize);

	while(i < aMaxLength - 1 && myFilePos < myFileSize)
	{
		Read(someData[i]);
		if(!someData[i])
			break;
		i++;
	}
	someData[i] = 0;
}


void MF_File::ReadLine(char* someData, unsigned int aMaxLength)
{
	unsigned char* st = (unsigned char*) someData;
	unsigned int i = 0;
	unsigned char ch = 0;
	unsigned char och = 0;
	unsigned char voch = 0;
	bool startOfLine = true;
	bool endOfLine = false;
	bool comments = false;


	assert(myFlags & OPEN_READ);

	while(myFilePos < myFileSize && i < aMaxLength - 1)
	{
		voch = och;
		och = ch;
		Read(ch);
		if(ch == '\n' && och == '\r')
		{
			if(i > 1)
			{
				i--;
				while(st[i - 1] < 33 && i > 0)
					i--;
				st[i] = 0;
				myReadLineCutFlag = false;
				if (i == 1 && comments && st[0] == '/')
					st[0] = 0;
				return; // end of line
			}
			else if(i == 1 && comments) //only comments on the line
			{
				i--;
				startOfLine = true;
				endOfLine = false;
			} 
			else
			{
				startOfLine = true;
				endOfLine = false;
			}
		}

		if(ch >= 33 || myReadLineCutFlag)
			startOfLine = false;

		if(!startOfLine && !endOfLine)
		{
			if(och == '/' && ch == '/'
			&& (!ourURLCommentFix || voch != ':')) // special fix for url's
			{
				endOfLine = true;
				comments = true;
			}
			else
				st[i++] = ch;
		}
	}

	myReadLineCutFlag = true;

	st[i] = 0;

	if (i == 1 && comments && st[0] == '/')
		st[0] = 0;
}


void MF_File::ReadLineTrue(char* someData, unsigned int aMaxLength)
{
	unsigned char* st = (unsigned char*) someData;
	unsigned int i = 0;
	unsigned char ch = 0, och;

	assert(myFlags & OPEN_READ);

	while(myFilePos < myFileSize && i < aMaxLength - 1)
	{
		och = ch;
		Read(ch);
		if(ch == '\n' && och == '\r')
		{
			st[i - 1] = 0;
			return; // end of line
		}
		else
			st[i++] = ch;
	}

	st[i] = 0;
}


void MF_File::ReadLineLocalized(MC_LocChar* someData, unsigned int aMaxLength)
{
	MC_LocChar* st = someData;
	unsigned int i = 0;
	MC_LocChar ch = 0, och;
	bool startOfLine = true;
	bool endOfLine = false;

	assert(myFlags & OPEN_READ);

	while(myFilePos < myFileSize && i < aMaxLength - 1)
	{
		och = ch;
		Read(ch);
		if(ch == '\n' && och == '\r')
		{
			if(i > 1)
			{
				i--;
				while(st[i - 1] < 33 && i > 0)
					i--;
				st[i] = 0;
				myReadLineCutFlag = false;
				return; // end of line
			}
			else
			{
				startOfLine = true;
				endOfLine = false;
			}
		}

		if(ch >= 33 || myReadLineCutFlag)
			startOfLine = false;

		if(!startOfLine && !endOfLine)
		{
			if(och == '/' && ch == '/')
			{
				i--;
				while(i > 0 && st[i] < 33)
					i--;
				endOfLine = true;
			}
			else
				st[i++] = ch;
		}
	}

	myReadLineCutFlag = true;

	st[i] = 0;
}


void MF_File::ReadLineTrueLocalized(MC_LocChar* someData, unsigned int aMaxLength)
{
	MC_LocChar* st = someData;
	unsigned int i = 0;
	MC_LocChar ch = 0, och;

	assert(myFlags & OPEN_READ);

	while(myFilePos < myFileSize && i < aMaxLength - 1)
	{
		och = ch;
		Read(ch);
		if(ch == '\n' && och == '\r')
		{
			st[i - 1] = 0;
			return; // end of line
		}
		else
			st[i++] = ch;
	}

	st[i] = 0;
}

void MF_File::ReadLine(MC_String& aString)
{
	MC_StaticString<8192> temp;
	ReadLine(temp.GetBuffer(), 8192);
	aString = temp;
} // reads a line and removes beginning and trailing whitespaces

void MF_File::ReadASCIIZ(MC_String& aString)
{
	MC_StaticString<8192> temp;
	ReadASCIIZ(temp.GetBuffer(), 8192);
	aString = temp;
} // reads a NULL-terminated string



bool MF_File::Write(const void* someData, unsigned int aLength)
{
	assert(myFlags & OPEN_WRITE);

	if(aLength == 0)
		return true;

	assert(someData != 0);

	// check if buffer needs to increase size
	if(myFilePos + aLength > myBufferSize)
	{
		int oldBufSiz = myBufferSize;
		unsigned char* myNewBuf;
		while(myFilePos + aLength > myBufferSize)
			myBufferSize += 65536; // increase buffer by 64kb

		myNewBuf = new unsigned char[myBufferSize];
		if(!myNewBuf)
		{
			myBufferSize = oldBufSiz;
			return false;
		}

		if(myBuffer) // old buffer exists - copy contents to new buffer and delete old.
		{
			memcpy(myNewBuf, myBuffer, oldBufSiz);
			delete [] myBuffer;
		}
		myBuffer = myNewBuf;
	}

	memcpy(myBuffer + myFilePos, someData, aLength);

	myFilePos += aLength;
	if(myFilePos > myFileSize)
		myFileSize = myFilePos;

	return true;
}


bool MF_File::ExistDir(const char* aDirName)
{
	HANDLE handle;
	WIN32_FIND_DATA findData;

	handle = PLATFORM::FindFirstFile(aDirName, &findData);

	if(handle == INVALID_HANDLE_VALUE)
	{
		// CHANGE LATER??? check sdf's as well?
		return false;
	}


	FindClose(handle);


	return (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
};


bool MF_File::GetFileInfo(const char* aFileName, unsigned int& aReturnDateTime, unsigned int& aReturnSize, char* aSDFName)
{
	WIN32_FIND_DATA findData;
	HANDLE handle;
	
	handle = PLATFORM::FindFirstFile(aFileName, &findData);
	if(handle == INVALID_HANDLE_VALUE)
	{		
		return false;
	}

	FindClose(handle);

	if(aSDFName)
		aSDFName[0] = 0;

	aReturnDateTime = ReadFileDateTimeFromHandle(findData.ftLastWriteTime);
	if(findData.nFileSizeHigh != 0)
	{
		return false;
	}

	aReturnSize = findData.nFileSizeLow;

	return true;
}


bool MF_File::DelTree(const char* aDir)
{
	HANDLE handle;
	WIN32_FIND_DATA findData;
	BOOL err;
	char st[256];

	if (aDir[strlen(aDir)-1] == '/' || aDir[strlen(aDir)-1] == '\\')
		sprintf(st, "%s*.*", aDir);
	else
		sprintf(st, "%s/*.*", aDir);
	
	handle = PLATFORM::FindFirstFile(st, &findData);
	if(handle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	do
	{
		if(strcmp(findData.cFileName, ".") && strcmp(findData.cFileName, ".."))
		{
			sprintf(st, "%s/%s", aDir, findData.cFileName);
			if(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				DelTree(st);
			else
				DeleteFile(st);
		}
		err = FindNextFile(handle, &findData);
	} while(err != FALSE);
	FindClose(handle);

	RemoveDirectory(aDir);

	return true;
}


bool MF_File::DelFile(const char* aFile)
{
	return (DeleteFile(aFile) != 0);
}


bool MF_File::RenameFile(const char* anOldFileName, const char* aNewFileName)
{
	return (MoveFile(anOldFileName, aNewFileName) != 0);
}

bool MF_File::GetDir(const char* aDir, MF_GetDirData* someOutData)
{
	char st[256];
	WIN32_FIND_DATA findData;
	HANDLE ha;
	MF_GetDirData* getdat;

	getdat = someOutData;
	if(!getdat)
		return false;
	getdat->myFirstEntry = NULL;

	if(aDir && aDir[0])
	{
		strcpy(st, aDir);
		strcat(st, "/*");
	}
	else
		strcpy(st, "*");

	ha = PLATFORM::FindFirstFile(st, &findData);
	if(ha != INVALID_HANDLE_VALUE)
	{
		do {
			if(strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0)
			{
				getdat->AddFile(findData.cFileName, (findData.nFileSizeHigh != 0 ? 0xFFFFFFFF : findData.nFileSizeLow), ReadFileDateTimeFromHandle(findData.ftLastWriteTime), ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0));
			}
		} while(FindNextFile(ha, &findData) != FALSE);

		FindClose(ha);
	}

	return true;
}


bool MF_File::CreatePath(const char* aFileName)
{
	char st[256];
	unsigned int i;

	int ret = 0;

	i = 0;
	while( ret == 0 && aFileName[i])
	{
		if(aFileName[i] == '/' || aFileName[i] == '\\')
		{
			st[i] = 0;
			{
				ret = _mkdir(st);
				if( ret == -1 && errno == 17 /*EEXIST*/ )  // Directory already exist so we shouldn't abort creating the rest of the path.
					ret = 0;
				else if( ret == -1 && st[1] == ':' && st[2] == 0 )  // Ignore error if someone is trying to create c: as a dir
					ret = 0;
				//else if( ret == -1 )
				//{
				//	LPVOID lpMsgBuf;
				//	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL );
				//	printf( "\nCould not create directory! (errno:'%d':\n %s\n", GetLastError(), lpMsgBuf );
				//}
			}
		}

		st[i] = aFileName[i];

		i++;
	}
	return (ret==0);
}

bool MF_File::ConvertExtendedDateTimeToCompressedForm(const MF_FileDatTime& anExtendedDateTime, unsigned int& aReturnDateTime)
{
	if(anExtendedDateTime.mySecond > 59 || anExtendedDateTime.myMinute > 59 || anExtendedDateTime.myHour > 23 || anExtendedDateTime.myDay < 1 || anExtendedDateTime.myDay > 31 || anExtendedDateTime.myMonth < 1 || anExtendedDateTime.myMonth > 12 || anExtendedDateTime.myYear < 2000 || anExtendedDateTime.myYear > 2133)
		return false; // can't convert this date

	aReturnDateTime = anExtendedDateTime.myYear - 2000;
	aReturnDateTime *= 12;
	aReturnDateTime += anExtendedDateTime.myMonth - 1;
	aReturnDateTime *= 31;
	aReturnDateTime += anExtendedDateTime.myDay - 1;
	aReturnDateTime *= 24;
	aReturnDateTime += anExtendedDateTime.myHour;
	aReturnDateTime *= 60;
	aReturnDateTime += anExtendedDateTime.myMinute;
	aReturnDateTime *= 60;
	aReturnDateTime += anExtendedDateTime.mySecond;

	return true;
}


void MF_File::ConvertCompressedDateTimeToExtendedForm(unsigned int aDateTime, MF_FileDatTime& aReturnExtendedDateTime)
{
	aReturnExtendedDateTime.mySecond = aDateTime % 60;
	aDateTime /= 60;

	aReturnExtendedDateTime.myMinute = aDateTime % 60;
	aDateTime /= 60;

	aReturnExtendedDateTime.myHour = aDateTime % 24;
	aDateTime /= 24;

	aReturnExtendedDateTime.myDay = (aDateTime % 31) + 1;
	aDateTime /= 31;

	aReturnExtendedDateTime.myMonth = (aDateTime % 12) + 1;
	aDateTime /= 12;

	aReturnExtendedDateTime.myYear = aDateTime + 2000;
}


bool MF_File::CopyFile(const char* aSourceFileName, const char* aDestinationFileName)
{
	if (!CreatePathValidated(aDestinationFileName))
		return false;

	const BOOL result = ::CopyFile(aSourceFileName, aDestinationFileName, FALSE);
	return (result != FALSE);
}

bool MF_File::CreatePathValidated(const char* aPath)
{
	char dir[MAX_PATH];
	char temp[MAX_PATH];

	ExtractDirectory(dir, aPath);

	if (dir[0] == 0)
		return true;

	sprintf(temp, "%s/", dir);
	MF_File::CreatePath(temp);

	if (!MF_File::ExistDir(dir))
	{
		return false;
	}
	return true;
}

const char* MF_File::ExtractExtension(const char* aPath)
{
	const char* ptr;
	const char* endOfString;
	unsigned int len;

	if (aPath == NULL)
	{
		assert(false);
		return "";
	}

	len = strlen(aPath);
	endOfString = aPath + len;
	if (len == 0)
	{
		return endOfString;
	}

	ptr = endOfString - 1;
	while (*ptr != '\0')
	{
		if (*ptr == '/' || *ptr == '\\')
		{
			return endOfString;
		}
		else if (*ptr == '.')
		{
			return (ptr + 1);
		}
		ptr--;
	}

	return endOfString;
}

const char* MF_File::ExtractFileName(const char* aPath)
{
	const char* currPos;

	if (aPath == NULL)
	{
		assert(false);
		return "";
	}

	currPos = aPath;

	// if path contains extension, then it has a file at the end.
	// in that case look up slash
	if (ExtractExtension(aPath)[0] != '\0')
	{
		while (*aPath != '\0')
		{
			if (*aPath == '/' || *aPath == '\\')
			{
				currPos = aPath + 1;
			}
			aPath++;
		}
	}
	// else point to path's termination character
	else
	{
		currPos += strlen(aPath);
	}

	return currPos;
}

char* MF_File::RemoveExtensionFromPath(char* aDest, const char* aFullPath)
{
	const char* currPos;
	int len;

	if (aFullPath == NULL)
	{
		assert(false);
		aDest[0] = '\0';
		return aDest;
	}

	len = strlen(aFullPath);
	currPos = aFullPath + len;
	while (len > 0)
	{
		if (*currPos-- == '.')
		{
			memcpy(aDest, aFullPath, len);
			break;
		}
		len--;
	}
	if (len >= 0)
		aDest[len] = '\0';
	else
		aDest[0] = '\0';
	return aDest;
}

char*  MF_File::ReplaceExtension(char* aDest, const char* aFullPath, const char* aNewExtension)
{
	if (aDest == 0 || aFullPath == 0 || aNewExtension == 0)
	{
		if (aDest)
			aDest[0] = 0;
		return aDest;
	}

	RemoveExtensionFromPath(aDest, aFullPath);
	if (aDest[0] == 0)
		return aDest;

	if (*aNewExtension != '.')
		strcat(aDest, ".");
	strcat(aDest, aNewExtension);

	return aDest;
}

char* MF_File::ExtractDirectory(char* aDirectoryDest, const char* aFullPath)
{
	int len;
	unsigned int dirPathLen;
	int i;

	if (aFullPath == NULL)
	{
		assert(false);
		return "";
	}

	len = strlen(aFullPath);

	// if path contains extension, then it has a file at the end.
	// in that case look up slash
	if (ExtractExtension(aFullPath)[0] != '\0')
	{
		dirPathLen = 0;
		for (i = len - 1; i >= 0; i--)
		{
			if (aFullPath[i] == '\\' || aFullPath[i] == '/')
			{
				dirPathLen = i + 1;
				break;
			}
		}
	}
	// else use entire path as dir path
	else
	{
		dirPathLen = len;
	}

	// copy dir portion of path to dest
	for (i = 0; i < (signed)dirPathLen; i++)
	{
		aDirectoryDest[i] = aFullPath[i];
	}
	aDirectoryDest[i] = '\0';

	// remove last slash (if exists)
	if (dirPathLen > 0 && (aDirectoryDest[dirPathLen-1] == '/' || aDirectoryDest[dirPathLen-1] == '\\'))
		aDirectoryDest[dirPathLen - 1] = '\0';

	return aDirectoryDest;
}

char* MF_File::ExtractUpperDirectory(char* aDirectoryDest, const char* aFullPath)
{
	int len;
	int dirPathLen;
	char extractedDir[MAX_PATH];
	int i;

	ExtractDirectory(extractedDir, aFullPath);

	dirPathLen = -1;
	len = strlen(extractedDir);
	for (i = len - 1; i >= 0; i--)
	{
		if (extractedDir[i] == '\\' || extractedDir[i] == '/')
		{
			dirPathLen = i;
			break;
		}
	}
	if (dirPathLen == -1)
	{
		strcpy(aDirectoryDest, extractedDir);
		return aDirectoryDest;
	}

	// copy dir portion of path to dest
	for (i = 0; i < dirPathLen; i++)
	{
		aDirectoryDest[i] = extractedDir[i];
	}
	aDirectoryDest[i] = '\0';

	// remove last slash (if exists)
	if (dirPathLen > 0 && (aDirectoryDest[dirPathLen-1] == '/' || aDirectoryDest[dirPathLen-1] == '\\'))
		aDirectoryDest[dirPathLen - 1] = '\0';

	return aDirectoryDest;
}

bool MF_File::CompareStrings(const char* aString1, const char* aString2)
{
	unsigned int s1Length;
	unsigned int s2Length;

	if (aString1 == NULL || aString2 == NULL)
	{
		assert(false);
		return (aString1 == aString2);
	}

	s1Length = strlen(aString1);
	s2Length = strlen(aString2);
	if (s1Length != s2Length)
		return false;

	while (*aString1 != '\0')
	{
		if (tolower(*aString1) != tolower(*aString2))
		{
			return false;
		}
		aString1++;
		aString2++;
	}

	return true;
}

void MF_File::MakeTempFileName(char* aDestination, const char* anExtension, const char* aBaseDir)
{
	char path[MAX_PATH];
	char totalPath[MAX_PATH];
	char* p;

	// get path
	if (aBaseDir != NULL)
	{
		sprintf(path, "%s\\", aBaseDir);
	}
	else
	{
		PLATFORM::GetTempPath(sizeof(path), path);
	}
	// generate temp file name and append with path
	PLATFORM::GetTempFileName(path, "", 0, totalPath);
	DelFile(totalPath); // remove the temp file generated by GetTempFileName()

	// remove all dots in temp file name (would otherwise ruin possible extension)
	p = totalPath;
	while (*p)
	{
		if (*p == '.')
		{
			*p = 't'; //(char)(rand() % 256);
		}
		p++;
	}

	// append extension if requested
	if (anExtension != NULL)
		sprintf(aDestination, "%s.%s", totalPath, anExtension);
	else
		sprintf(aDestination, "%s", totalPath);

	// if the modified file name exists, call recursively until non-existing variant found
	if (MF_File::ExistFile(aDestination))
		MakeTempFileName(aDestination, anExtension, aBaseDir);
}

MC_GrowingArray<MC_String> MF_File::FindFiles(MC_String aSearchString, bool aRecursive)
{
	struct RecursiveSearch
	{
		MC_GrowingArray<MC_String> myFound;
		bool myRecurse;
		MC_String myExt;

		void RecursiveFind(MC_String aDir)
		{
			MF_GetDirData dirdata;
			MF_File::GetDir(aDir, &dirdata);

			MF_GetDirEntryData* curDir = dirdata.myFirstEntry;
			while(curDir)
			{
				MC_String temp;
				if(curDir->myIsDirectoryFlag)
				{
					if(myRecurse)
						RecursiveFind(aDir + "/" + curDir->myName);
				}
				else
				{
					if(myExt.IsEmpty() || myExt == ExtractExtension(curDir->myName))
						myFound.Add(aDir + "/" + curDir->myName);

				}
				curDir=curDir->myNextPtr;
			}
		}
	};

	aSearchString.Replace('\\','/');
	int lastSlash = aSearchString.ReverseFind('/');

	MC_String finishedDir;
	if(lastSlash>=0)
		finishedDir = aSearchString.Mid(0,lastSlash);

	MC_String extension;
	if(aSearchString.ReverseFind('.') >= 0)
		extension=aSearchString.Mid(aSearchString.ReverseFind('.')+1,10000);


	RecursiveSearch rs;
	rs.myExt=extension;
	rs.myRecurse=aRecursive;
	rs.myFound.Init(128,128);
	rs.RecursiveFind(finishedDir);

	return rs.myFound;
}

void MF_File::SetLogMissingFiles(bool anEnableFlag, const char* aLogFilename)
{
	ourMissingFileLog = "";
	if (anEnableFlag)
	{
		MF_File fi;
		if (!fi.Open(aLogFilename, OPEN_WRITE))
		{
			MC_DEBUG("Couldn't activate missing file logging.");
			return;
		}

		ourMissingFileLog = aLogFilename;
	}
}

void MF_File::LogMissingFile(const char* aFilename)
{
	if (ourMissingFileLog.IsEmpty())
		return;

	MC_String filename = aFilename;

	if (ourMissingFiles.Find(filename) != -1)
		return;

	ourMissingFiles.Add(filename);

	MF_File log;
	if (log.Open(ourMissingFileLog, OPEN_APPEND))
		log.WriteLine(aFilename);
}
