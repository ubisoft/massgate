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
#ifndef __MC_DEBUGLISTENERIMPLEMENTATIONS_H__
#define __MC_DEBUGLISTENERIMPLEMENTATIONS_H__

#include "MC_Debug.h"

// Debuglistener that outputs to the MsDev output pane. Like the TRACE() macros, but not using MFC.
// This class only outputs things in _DEBUG so we #ifdef away in release
class MC_MsDevOutputDebugListener : public MC_DebugListener
{
public:
#ifdef _RELEASE_
	MC_MsDevOutputDebugListener() {}
	virtual void DebugMessage( const char* aMessage ) {}
#else
	MC_MsDevOutputDebugListener();
	virtual void DebugMessage( const char* aMessage );
#endif

	virtual void Destroy() {}

	virtual void Commit();

private:
};

// Debuglistener that outputs to a file.
class MC_FileDebugListener : public MC_DebugListener
{

public:
	MC_FileDebugListener( const char* aFilename, bool anAppendFileFlag = false, bool aFlushFlag = true);
//#ifndef	_RELEASE_	
	virtual void DebugMessage( const char* aMessage );
//#else
//	virtual void DebugMessage( const char* aMessage ) {}
//#endif

	virtual void Destroy();

	virtual void Commit();

private:
	MC_FileDebugListener( void ) {}
//	char* myDebugFilename;
	FILE* myFile;
	bool myFlushFlag;
};

// Debuglistener that outputs to STDERR
class MC_StderrDebugListener : public MC_DebugListener
{
public:
	MC_StderrDebugListener( void ) {}
	~MC_StderrDebugListener( void ) {}
//#ifndef	_RELEASE_	
	virtual void DebugMessage( const char* aMessage );
//#else
//	virtual void DebugMessage( const char* aMessage ) {}
//#endif
	void Destroy() {}
	virtual void Commit();
};

// Error listener that outputs to an XML file.
class MC_XMLFileListener
{
public:
	MC_XMLFileListener( const char* aFilename, bool anAppendFileFlag = false);
	~MC_XMLFileListener( void );

	void StartError( const char* aDescription );
	void CloseError( void );

	void StartTag( const char* aTag, const char* aDesc );
	void WriteData( const char* data );
	void EndTag( const char* aTag );

	void Commit();

private:
	MC_XMLFileListener( void ) {}
	MC_StaticString<256> myFilename;
	FILE* myFile;
	bool myAppendFlag;
};

#endif  //__MC_DEBUGLISTENERIMPLEMENTATIONS_H__
