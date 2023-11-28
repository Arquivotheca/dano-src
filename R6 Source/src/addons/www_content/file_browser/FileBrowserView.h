/*-----------------------------------------------------------------*/
//
//	File:		FileBrowserView.h
//
//	Written by:	Robert Polic
//
//	Copyright 2000, Be Incorporated
//
/*-----------------------------------------------------------------*/

#ifndef FILE_BROWSER_H
#define FILE_BROWSER_H

#include <Directory.h>
#include <FtpProtocol.h>
#include <Path.h>
#include <String.h>
#include <View.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "MediaFilesView.h"
#include "ImageTextFilesView.h"


//#define DEBUG				1
#define kEXTENSION_MAP_NAME	"beia_file_extension_map"

using namespace Wagner;


/*-----------------------------------------------------------------*/

struct extension_map
{
					extension_map(const char* ext, const char* type)
					{
						extension = (char*)malloc(strlen(ext) + 1);
						strcpy(extension, ext);
						mime_type = (char*)malloc(strlen(type) + 1);
						strcpy(mime_type, type);
						next = NULL;
					}

					~extension_map()
					{
						if (next)
							delete next;

						free(extension);
						free(mime_type);
					}

	void			PrintToStream()
					{
#ifdef DEBUG
						printf("%s\t%s\n", extension, mime_type);
						if (next)
							next->PrintToStream();
#endif /* DEBUG */
					}

	char*			extension;
	char*			mime_type;
	extension_map*	next;
};

/*-----------------------------------------------------------------*/

enum file_type
{
	eMediaFile = 0,
	eImageFile,
	eTextFile,
	eOtherFile
};


/*======================== FileBrowserView =======================*/

class FileBrowserView : public BView
{
	public:
							FileBrowserView		(BRect,
												 drawing_parameters*,
												 BString* default_volume,
												 BString* default_view);
							~FileBrowserView	();
		void				AllAttached			();
		void				MessageReceived		(BMessage*);
		sem_id				ProcessingSem		()
													{ return fProcessingSem; };
		extension_map*		ExtensionMap		()
													{ return fExtensionMap; };
		void				KillProcessingThread();
		bool				Quit				()
													{ return fQuit; };
		const char*			MountedAt			()
													{ return fMountedAt.String(); };
		const char*			Domain				()
													{ return fDomain.String(); };
		const char*			User				()
													{ return fUser.String(); };
		const char*			Password			()
													{ return fPassword.String(); };
		const char*			Path				()
													{ return fPath.String(); };
		FtpProtocol*		FTP					()
													{ return fFtpProtocol; };
		URL&				GetURL				()
													{ return fURL; };
		BView*				CurrentView			()
													{ return fCurrentView; };
		MediaFilesView*		MediaFiles			()
													{ return fMediaFiles; };
		ImageTextFilesView*	ImageFiles			()
													{ return fImageFiles; };
		ImageTextFilesView*	TextFiles			()
													{ return fTextFiles; };
		ImageTextFilesView*	OtherFiles			()
													{ return fOtherFiles; };
		drawing_parameters*	fParameters;

	private:
		void				LoadExtensionMap	();

		bool				fQuit;
		BString				fMountedAt;
		BString				fDomain;
		BString				fUser;
		BString				fPassword;
		BString				fPath;
		BString				fDefaultVolume;
		BString				fDefaultView;
		sem_id				fProcessingSem;
		extension_map*		fExtensionMap;
		FilesView*			fCurrentView;
		MediaFilesView*		fMediaFiles;
		ImageTextFilesView*	fImageFiles;
		ImageTextFilesView*	fTextFiles;
		ImageTextFilesView*	fOtherFiles;
		FtpProtocol*		fFtpProtocol;
		URL					fURL;
};
#endif
