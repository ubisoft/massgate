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

#include <sys/stat.h>
#include <time.h>

#include "mc_md5.h"
#include "mf_file.h"
#include "mn_netrequester.h"

#include "MMG_ServerBannerDownloader.h"

MMG_ServerBannerDownloader*		MMG_ServerBannerDownloader::ourInstance = new MMG_ServerBannerDownloader();

MMG_ServerBannerDownloader::MMG_ServerBannerDownloader()
	: myRequestPending(false)
{

}

void
MMG_ServerBannerDownloader::SetTargetDirectory(
	const char*					aDirectory)
{
	myTargetDirectory = aDirectory;
}

void
MMG_ServerBannerDownloader::BeginDownload(
	const char*					anUrl,
	Callback*					aCallback)
{
	if(myRequestPending)
	{
		MN_NetRequester::GetInstance()->CancelRequest(this);
		MF_File::DelFile(myTargetFile.GetBuffer());
	}

	const char*		lastdot = strrchr(anUrl, '.');

	// Basic sanity checking
	if(lastdot == NULL )
		return;
	
	MC_MD5			md5;
	md5.MD5Start();
	md5.MD5Continue((unsigned char*) anUrl, strlen(anUrl));

	unsigned char	hash[33];
	md5.MD5Complete(hash);

	myTargetFile = myTargetDirectory;
	myTargetFile.Append((char*) hash, 32);
	myTargetFile.Append(lastdot, strlen(lastdot));

	myCurrentCallback = aCallback;
	bool		download = false;

	// File exists and is newer than one week => use local cache
	if(MF_File::ExistFile(myTargetFile.GetBuffer()))
	{
		struct stat		info;
		::stat(myTargetFile.GetBuffer(), &info);

		if(info.st_ctime + 3600 * 24 * 7 < time(NULL))
			download = true;
	}else
		download = true;

	if(download)
	{
		myCurrentFile.Open(myTargetFile.GetBuffer(), MF_File::OPEN_WRITE);
		MN_NetRequester::GetInstance()->Get(this, anUrl, 0);
	}
	else
	{
		myRequestPending = false;
		myCurrentCallback->DownloadReady(Callback::BANNER_READY, myTargetFile);
	}
}

// Callbacks from MN_NetRequester
bool
MMG_ServerBannerDownloader::ReceiveData(
	const void*					someData, 
	unsigned int				someDataLength, 
	int							theExpectedTotalDataLength)
{
	myCurrentFile.Write(someData, someDataLength);

	return true;
}

bool
MMG_ServerBannerDownloader::RequestFailed(
	int							anError)
{
	if(MF_File::ExistFile(myTargetFile.GetBuffer()))
		MF_File::DelFile(myTargetFile.GetBuffer());
	
	myRequestPending = false;
	MC_StaticString<512>		dummy;
	myCurrentCallback->DownloadReady(Callback::BANNER_ERROR, dummy);
	return true;
}

bool
MMG_ServerBannerDownloader::RequestComplete()
{
	myCurrentFile.Close();
	myRequestPending = false;
	myCurrentCallback->DownloadReady(Callback::BANNER_READY, myTargetFile);
	return true;
}