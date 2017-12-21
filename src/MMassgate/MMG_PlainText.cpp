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
#include "MMG_PlainText.h"
#include "MMG_BitStream.h"

#include "MC_Debug.h"

const char MMG_PlainText::myCharset[64+1] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-$";
const char MMG_PlainText::myCharLUT[128] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 63, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1, -1, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1 };

bool 
MMG_PlainText::ToText(char* theDestBuffer, unsigned int theDestBufferLength, const void* theRawBuffer, unsigned int theRawBufferLengthInBytes)
{
	unsigned int iter = 0;
	MMG_BitReader<unsigned int> br((const unsigned int*)theRawBuffer, theRawBufferLengthInBytes*8);

	while((br.GetStatus() != MMG_BitStream::EndOfStream) && (iter != theDestBufferLength-1))
		theDestBuffer[iter++] = myCharset[br.ReadBits(6)];

	theDestBuffer[iter] = 0;
	return br.GetStatus() == MMG_BitStream::EndOfStream;
}

bool 
MMG_PlainText::ToRaw(void* theDest, unsigned int theDestBufferLengthInBytes, const char* thePlaintext)
{
	MMG_BitWriter<unsigned int> bw((unsigned int*)theDest, theDestBufferLengthInBytes*8);
	unsigned int iter = 0;
	while ((thePlaintext[iter]) && (bw.GetStatus() != MMG_BitStream::EndOfStream))
	{
		char c = myCharLUT[thePlaintext[iter++]];
		if (c == -1) return false;
		bw.WriteBits(c, 6);
	}
	if (thePlaintext[iter]) return false;
	return true;
}

