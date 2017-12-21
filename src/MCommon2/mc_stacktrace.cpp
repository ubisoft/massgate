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
#include "mc_stacktrace.h"
#include "mf_file.h"
#include "mp_pack.h"
#include "MC_GlobalDefines.h"

void UnpackProgramDatabase()
{
	// Unpack the .pdb that's attached to the end of the .exe and write it to the same directory as the pdb.
	// Yes, we do this everytime we startup for now. Fix this if you feel like it.
	// However - NEVER wait with the unpack since it is not safe to do it from the exception handler (zip-lib may not
	// be able to allocate memory in case of crash).

#if IS_PC_BUILD

#ifdef MC_NO_PDB_APPEND

	// Dummy code, just to make sure we link with the "don't add" string
	char dummyPath[MAX_PATH+1];
	sprintf(dummyPath, "%s", MC_MEM_DONT_ADD_PDB_MARKER);
	GetModuleFileName(NULL, dummyPath, sizeof(dummyPath)-1);

	return;

#else // MC_NO_PDB_APPEND


#ifndef _RELEASE_
	MF_File exeFile;
	char programPath[MAX_PATH+1];
	memset(programPath, 0, sizeof(programPath));
	GetModuleFileName(NULL, programPath, sizeof(programPath)-1);
	if (exeFile.Open(programPath, MF_File::OPEN_READ | MF_File::OPEN_STREAMING))
	{
		unsigned int pdbSize, header, pdbUnpackedSize;
		exeFile.SetFilePos(exeFile.GetSize() - 3*sizeof(unsigned int));
		exeFile.Read(pdbSize);
		exeFile.Read(pdbUnpackedSize);
		exeFile.Read(header);
		if (header == 'pdbf')
		{
			exeFile.SetFilePos(exeFile.GetSize() - 3*sizeof(unsigned int) - pdbSize);
			char* compressedPdbBuffer = new char[pdbSize];
			char* uncompressedPdbBuffer = new char[pdbUnpackedSize];
			if(exeFile.Read(compressedPdbBuffer, pdbSize))
			{
				const unsigned int unpackLen = MP_Pack::UnpackZip(compressedPdbBuffer, pdbSize, uncompressedPdbBuffer, pdbUnpackedSize);
				if (unpackLen == pdbUnpackedSize)
				{
					MF_File outputFile;
					strcpy(programPath + strlen(programPath)-3, "pdb");
					if(outputFile.Open(programPath, MF_File::OPEN_WRITE))
					{
						outputFile.Write(uncompressedPdbBuffer, pdbUnpackedSize);
					}
				}
			}
			delete [] compressedPdbBuffer;
			delete [] uncompressedPdbBuffer;
		}
		else
		{
//			MessageBox(NULL, "No pdb file appended to exe in post-build step.", "The BOOOOM will be thin!", MB_OK);
		}
	}
	else
	{
//		MessageBox(NULL, "Could not access debug-info. BOOOM Messages will only contain stack addresses.", "Boombadabing!", MB_OK);
	}
#endif //_RELEASE_

#endif //MC_NO_PDB_APPEND

#endif // IS_PC_BUILD
}

