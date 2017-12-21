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
#ifndef MC_MINIDUMPTOOOLS_H_
#define MC_MINIDUMPTOOOLS_H_


class MC_MiniDumpTools
{
public:
	// Connect to a dumpserver and have it dump our process, return true if dumpserver dumped us
	static bool AskDumpServerToDumpUs(const char* theRequestedDumpFilename, void* theExceptionPointers=NULL);

	// Run a dumpserver listening to dumprequests
	static void RunDumpServer(int theDumpType, const char* theDumpTypeStr);

	// Dump any running process without interrupting it
	static bool DumpProcess(uint32 processId, 
							const char* dumpFileName, 
							int dumptype,
							void* WinDbgExceptionInformation);

	// NOTE! The dumptype reference above is a MINIDUMP_TYPE from <dbghelp.h>

private:
	struct CoreDumpPipeData_t
	{
		unsigned long processId;
		unsigned long crashingThread;
		void* exceptionPointers;
		char requestedDumpFilename[256];
	};
};


#endif
