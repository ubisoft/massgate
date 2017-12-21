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
#ifndef MMG_BITSTREAM___H__
#define MMG_BITSTREAM___H__

class MMG_BitStream /* abstract */
{
public:
	enum StreamStatus { NoChange, OperationOk, EndOfStream };
	enum StreamOffset { StartPosition, CurrentPosition, EndPosition };

	unsigned int Tell() { return myPosition; }
	inline void Seek(StreamOffset theStart, unsigned int numBits) 
	{ 	switch(theStart){	case StartPosition:	myPosition = 0; 
							case CurrentPosition: myPosition += numBits; break;	
							default: assert(false);	};	
		if (myPosition == myMaxLength) SetStatus(EndOfStream); 
	}
	void Rewind() { myPosition = 0; }
	StreamStatus GetStatus() { return myStatus; }

protected:
	void SetStatus(StreamStatus theStatus) { myStatus = theStatus; }
	MMG_BitStream(unsigned int theMaxLengthInBits) : myMaxLength(theMaxLengthInBits) { myStatus = NoChange; myPosition = 0; }
	virtual ~MMG_BitStream() = 0 { };
private:
	unsigned int myPosition;
	unsigned int myMaxLength;
	StreamStatus myStatus;
};

template<typename T> // T is UNSIGNED char, UNSIGNED short, UNSIGNED int, etc
class MMG_BitReader : public MMG_BitStream
{
public:
	inline T ReadBit();
	MMG_BitReader(const T* theRawBuffer, unsigned int theRawBufferLengthInBits);
	T ReadBits(T theNumBits);
	virtual ~MMG_BitReader();
private:
	MMG_BitReader() : MMG_BitStream(0) { }
	unsigned int myBufferLength;
	const T* myReadBuffer;
};

template<typename T>
class MMG_BitWriter : public MMG_BitStream
{
public:
	MMG_BitWriter(T* theDest, unsigned int theDestBufferLengthInBits);
	inline void WriteBit(T theBit);
	void WriteBits(T theValue, T theNumBits);
	virtual ~MMG_BitWriter();
private:
	MMG_BitWriter() : MMG_BitStream(0) { }
	unsigned int myBufferLength;
	T* myDestBuffer;
};



template<typename T>
MMG_BitReader<T>::MMG_BitReader(const T* theRawBuffer, unsigned int theRawBufferLengthInBits)
: MMG_BitStream(theRawBufferLengthInBits)
, myReadBuffer(theRawBuffer)
, myBufferLength(theRawBufferLengthInBits)
{
	// so we don't read outside by mistake (e.g. BitReader<int>(char* buffer, sizeof(char)))
	assert(theRawBufferLengthInBits/8 >= sizeof(T));
	// make sure that bufferlen is even multiple T (e.g. BitReader<int>(int*, 40) is no good)
	assert((theRawBufferLengthInBits & (~(sizeof(T)*8-1))) == theRawBufferLengthInBits);
}

template<typename T>
T
MMG_BitReader<T>::ReadBits(T theNumBits)
{
	if (theNumBits == 1)
		return ReadBit();
	assert(theNumBits <= sizeof(T)*8);

	const unsigned int where = Tell();
	const unsigned int shl = where & (sizeof(T)*8 - 1);
	const unsigned int dataPos = where / (sizeof(T)*8);

	if ((dataPos) == ((where - 1 + theNumBits) / (sizeof(T)*8)))
	{
		const T bitmask = ((1<<theNumBits)-1) << shl;
		Seek(CurrentPosition, theNumBits);
		return ((myReadBuffer[dataPos] & bitmask) >> shl);
	}
	else // split the request into two recursive calls
	{
		const unsigned int secondHalf = ((where + theNumBits) & (sizeof(T)*8 - 1));
		const unsigned int firstHalf = theNumBits - secondHalf;
		T retval = ReadBits(firstHalf);
		retval |= (ReadBits(secondHalf) << firstHalf);
		return retval;
	}
	return 0;
}

template<typename T>
T
MMG_BitReader<T>::ReadBit()
{
	const T data = myReadBuffer[Tell() / (sizeof(T)*8)];
	const T offset = 1 << (Tell() & (sizeof(T)*8 - 1));
	Seek(CurrentPosition, 1);
	return (data & offset) > 0;
}

template<typename T>
MMG_BitReader<T>::~MMG_BitReader()
{
}


template<typename T>
MMG_BitWriter<T>::MMG_BitWriter(T* theDest, unsigned int theDestBufferLengthInBits)
: MMG_BitStream(theDestBufferLengthInBits)
, myDestBuffer(theDest)
, myBufferLength(theDestBufferLengthInBits)
{
	// so we don't write outside by mistake (e.g. BitWriter<int>(char* buffer, sizeof(char)))
	assert(theDestBufferLengthInBits/8 >= sizeof(T));
	// make sure that bufferlen is even multiple T (e.g. BitWriter<int>(int*, 40) is no good)
	assert((theDestBufferLengthInBits & (~(sizeof(T)*8-1))) == theDestBufferLengthInBits);

}

template<typename T>
void 
MMG_BitWriter<T>::WriteBits(T theValue, T theNumBits)
{
	if (theNumBits == 1)
		return WriteBit(theValue&1);

	const unsigned int where = Tell();
	const unsigned int dataPos = where / (sizeof(T)*8);
	if ((dataPos) == ((where - 1 + theNumBits) / (sizeof(T)*8)))
	{
		const unsigned int shl = where & (sizeof(T)*8-1);
		const T bitmask = ((1<<theNumBits)-1) << shl;
		T& dest = myDestBuffer[dataPos];
		dest &= ~bitmask;
		dest |= (theValue << shl);
		Seek(CurrentPosition, theNumBits);
	}
	else
	{
		const unsigned int secondHalf = ((where + theNumBits) & (sizeof(T)*8-1));
		const unsigned int firstHalf = theNumBits - secondHalf;
		WriteBits(theValue, firstHalf);
		WriteBits(theValue >> firstHalf, secondHalf);
	}
}

template<typename T>
void
MMG_BitWriter<T>::WriteBit(T theBit)
{
	const unsigned int where = Tell();
	const T offset = 1 << (where & (sizeof(T)*8-1));
	const unsigned int dataPos = where / (sizeof(T)*8);
	if (theBit)	myDestBuffer[dataPos] |= offset;
	else myDestBuffer[dataPos] &= ~offset;
	Seek(CurrentPosition, 1);
}

template<typename T>
MMG_BitWriter<T>::~MMG_BitWriter()
{
}



#endif

