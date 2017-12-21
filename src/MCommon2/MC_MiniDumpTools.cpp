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
#include "StdAfx.h"
#include <DbgHelp.h>
#include "MC_MiniDumpTools.h"
#include <time.h>

#define MC_MINIDUMPSERVER_NAMED_PIPE "\\\\.\\pipe\\bt_minidump"


bool 
MC_MiniDumpTools::AskDumpServerToDumpUs(const char* theRequestedDumpFilename, void* theExceptionPointers)
{
#if IS_PC_BUILD
	HANDLE hPipe = CreateFile(	MC_MINIDUMPSERVER_NAMED_PIPE,
								GENERIC_READ | GENERIC_WRITE,
								0,
								NULL,
								OPEN_EXISTING,
								0,
								NULL);
	if (hPipe == INVALID_HANDLE_VALUE)
		return false;
	else if (GetLastError() == ERROR_PIPE_BUSY)
	{
		if (!WaitNamedPipe(MC_MINIDUMPSERVER_NAMED_PIPE, 10000))
			return false;
	}
	DWORD mode = PIPE_READMODE_MESSAGE;
	if (!SetNamedPipeHandleState(hPipe, &mode, NULL, NULL))
		return false;

	CoreDumpPipeData_t cdp;
	cdp.processId = GetCurrentProcessId();
	if (theExceptionPointers)
	{
		cdp.crashingThread = GetCurrentThreadId();
		cdp.exceptionPointers = theExceptionPointers;
	}
	else
	{
		cdp.crashingThread = 0;
		cdp.exceptionPointers = NULL;
	}
	strcpy_s(cdp.requestedDumpFilename, theRequestedDumpFilename);

	DWORD numBytes = 0;
	if (!WriteFile(hPipe, &cdp, sizeof(cdp), &numBytes, NULL))
		return false;
	FlushFileBuffers(hPipe);

	// Wait for response and see if dump was written
	CoreDumpPipeData_t response;

	// IS YOUR CALLSTACK POINTING TO THIS?
	if (!ReadFile(hPipe, &response, sizeof(response), &numBytes, NULL))
	// WELL; IS IT? IT'S BECAUSE YOU CRASHED, AND THE DUMP WAS WRITTEN BY THE DUMPSERVER.
	// CHECK THE CALLSTACK FOR YOUR ERROR
		return false;

	if (numBytes != sizeof(response))
		return false;
	if (cdp.processId == 0)
		return false;

#endif
	return true;
}

void 
MC_MiniDumpTools::RunDumpServer(int theDumpType, const char* theDumpTypeStr)
{
#if IS_PC_BUILD
	const unsigned int BufferSize = 512;

	// Based on http://msdn.microsoft.com/en-us/library/aa365588(VS.85).aspx

	while(true)
	{
		HANDLE pipeHandle = CreateNamedPipe(MC_MINIDUMPSERVER_NAMED_PIPE,
			PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES,
			BufferSize,
			BufferSize,
			0,
			NULL);
		if (pipeHandle == INVALID_HANDLE_VALUE)
		{
			puts("Could not create pipe.");
			return;
		}

		// Wait for client to connect.
		bool connected = ConnectNamedPipe(pipeHandle, NULL) ? true : (GetLastError() == ERROR_PIPE_CONNECTED);
		char inputBuffer[BufferSize];
		if (!connected)
		{
			CloseHandle(pipeHandle);
			continue;
		}
		DWORD numBytes = 0;
		if (ReadFile(pipeHandle, inputBuffer, sizeof(inputBuffer), &numBytes, NULL))
		{
			if (numBytes && (numBytes == sizeof(CoreDumpPipeData_t)))
			{
				CoreDumpPipeData_t* pd = (CoreDumpPipeData_t*)inputBuffer;

				__time64_t ltime;
				_time64(&ltime);

				printf("[%.24s] Received request to create dumpfile %s...", _ctime64(&ltime),pd->requestedDumpFilename);

				MC_String filename = pd->requestedDumpFilename;
				filename = filename.Left(filename.GetLength()-3);
				filename += theDumpTypeStr+8;
				filename += ".dmp";

				MINIDUMP_EXCEPTION_INFORMATION mei;
				mei.ThreadId = pd->crashingThread;
				mei.ExceptionPointers = (PEXCEPTION_POINTERS)pd->exceptionPointers;
				mei.ClientPointers = TRUE; // EceptionPointers point to *clients* address space

				if (DumpProcess(pd->processId, 
					filename.GetBuffer(), theDumpType, &mei))
				{
					if (WriteFile(pipeHandle, pd, sizeof(*pd), &numBytes, NULL))
					{
						puts("done.");
					}
					else
					{
						puts("Wrote dump ok. Could not inform caller of completion.");
					}
				}
				else
				{
					printf("dump failed with code %u.\n", GetLastError());
					// Telling client that we failed, let it try by itself
					pd->processId = 0;
					WriteFile(pipeHandle, pd, sizeof(*pd), &numBytes, NULL);
				}
			}
			else
			{
				puts("Received invalid request from client. Ignoring.");
			}
		}
		else
		{
			puts("Could not read from client. Ignoring.");
		}

		FlushFileBuffers(pipeHandle);
		DisconnectNamedPipe(pipeHandle);
		CloseHandle(pipeHandle);
	}
#endif
}


bool
MC_MiniDumpTools::DumpProcess(uint32 theProcessId, 
										  const char* dumpFileName, int dumptype,
										  void* WinDbgExceptionInformation)
{
#if IS_PC_BUILD
	HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, theProcessId);
	if (!processHandle)
	{
		puts("Could not open requested process.");
		return false;
	}
	HANDLE hDumpFile = CreateFile(dumpFileName, GENERIC_READ|GENERIC_WRITE, 
		FILE_SHARE_WRITE|FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

	
	MINIDUMP_EXCEPTION_INFORMATION* mei = (MINIDUMP_EXCEPTION_INFORMATION*)WinDbgExceptionInformation;
	if (mei && mei->ThreadId == 0)
		mei = NULL; // invalid, called may have requested dump during runtime (i.e not excepted state)

	bool good = TRUE == MiniDumpWriteDump(processHandle, theProcessId, hDumpFile, (MINIDUMP_TYPE)dumptype, mei, NULL, NULL);
	CloseHandle(hDumpFile);
	CloseHandle(processHandle);
	return good;
#else
	return false;
#endif
}
