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
#ifndef MMG_PLAINTEXT____H_
#define MMG_PLAINTEXT____H_


class MMG_PlainText
{
public:
	static bool ToText(char* theDestBuffer, unsigned int theDestBufferLength, const void* theRawBuffer, unsigned int theRawBufferLengthInBytes);
	static bool ToRaw(void* theDest, unsigned int theDestBufferLengthInBytes, const char* thePlaintext);
private:
	static const char myCharset[64+1];
	static const char myCharLUT[128];
};

#endif

