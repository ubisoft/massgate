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
#ifndef MMG_SERVERBANNERDOWNLOADER_H
#define MMG_SERVERBANNERDOWNLOADER_H

#include "mn_inethandler.h"

class MMG_ServerBannerDownloader : private MN_INetHandler {
public:
	class Callback {
	public:
		typedef enum {
			BANNER_READY,
			BANNER_ERROR
		}Status;

		virtual void			DownloadReady(
									Status						aStatus,
									const MC_StaticString<512>&	aPath) = 0;
	};

	// Don't forget to end path with '/'
	void						SetTargetDirectory(
									const char*					aDirectory);
	void						BeginDownload(
									const char*					anUrl,
									Callback*					aCallback);


	static MMG_ServerBannerDownloader*	GetInstance() { return ourInstance; }

private:
	static MMG_ServerBannerDownloader*	ourInstance;
	const char*					myTargetDirectory;
	MC_StaticString<512>		myTargetFile;
	bool						myRequestPending;
	Callback*					myCurrentCallback;
	MF_File						myCurrentFile;

								MMG_ServerBannerDownloader();

	// Callbacks from MN_NetRequester
	bool						ReceiveData(
									const void*					someData, 
									unsigned int				someDataLength, 
									int							theExpectedTotalDataLength);
	bool						RequestFailed(
									int							anError);
	bool						RequestComplete();

};

#endif /* MMG_SERVERBANNERDOWNLOADER_H */