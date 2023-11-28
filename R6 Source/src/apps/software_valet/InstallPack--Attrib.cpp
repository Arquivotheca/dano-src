#include <File.h>
#include <Path.h>
#include "InstallPack.h"
#include "StLocker.h"
#include "ErrHandler.h"
#include "AutoPtr.h"

#ifndef _BITMAP_STREAM_H
	//#include "DataFormats.h"
	#include "BitmapStream.h"
#endif

#include "PackAttrib.h"
#include "SPackData.h"
#include "DataID.h"

#include "MMallocIO.h"

#include "IGroupList.h"

#include "DestinationList.h"
#include "MyDebug.h"

#include <sys/socket.h>		// for ntohX
#include <errno.h>
#include <ctype.h>

#include "Util.h"

enum {
	PATH_ITEM = 0x00,
	FIND_ITEM = 0x01
};

// use exceptions
long InstallPack::NewReadAttributes(	PackAttrib *att )
{
	PRINT(("entering new read attributes\n"));
	ErrHandler		err;
	status_t		serr = B_NO_ERROR;
#if (!SEA_INSTALLER)
	char			sbuf[256];
#endif
	
	SetFile();
	
	record_header	header;		
	PRINT(("		seeking to attribOffset %d\n",attribOffset));
	fArcFile->Seek(attribOffset,SEEK_SET);
	try {
		err = fPackData->ReadRecordHeader(fArcFile,&header);
		if (header.what != ID_PKGATTR || header.type != LIS_TYPE) {
			err = B_ERROR;
		}
		int32 b;
		bool readingEntries = true;
		while(readingEntries) {
			err = fPackData->ReadRecordHeader(fArcFile,&header);
			switch (header.what) {
				case A_MASTER_GROUP_LIST:
					PRINT(("got master group list\n"));
					ReadMasterGroupList(att->groupList);
					// check errors and size
					break;
				case A_VIEW_GROUP_LIST:
					ReadViewGroupList(att->groupList);
					break;
				case A_DEFAULT_DEST_LIST:
					ReadDefaultDestList(att->defaultDest);
					break;
				case A_CUST_DEST_LIST:
					ReadCustomDestList(att->customDest);				
					break;
				case A_INST_DESCRIPTION:
					err = fPackData->ReadString(fArcFile,&att->descriptionText);
					// ReadString(&att->descriptionText);
					break;
				case A_INSTALLFOLDER:
					err = fPackData->ReadInt32(fArcFile,&b);
					att->doInstallFolder = b;
					break;
				case A_FOLDER_POPUP:
					err = fPackData->ReadInt32(fArcFile,&b);
					att->doFolderPopup = b;
					break;
				case A_LICENSE: {
					MMallocIO	buf;
					att->licenseText = NULL;
					// eventually do styled text!
					// read compressed data
					err = fPackData->ReadBin(&buf,fArcFile,0,NULL);
					
					buf.SetDispose(false);
					att->licenseSize = buf.Size();	
					att->licenseText = (char *)buf.Buffer();
						
					// do help only if non zero length string
					if (!*att->licenseText)
						att->licenseText = NULL;
					break;
				}
				case A_LICENSE_STYLE: {
					MMallocIO	buf;

					att->licenseStyle = NULL;
					// eventually do styled text!
					// read compressed data
					err = fPackData->ReadBin(&buf,fArcFile,0,NULL);
					buf.SetDispose(false);
					att->licenseStyle = (char *)buf.Buffer();
					break;
				}
#if (USING_HELP)
				case A_PACKAGE_HELP: {
					MMallocIO	buf;
					err = fPackData->ReadBin(&buf,fArcFile,0,NULL);
					
					buf.SetDispose(false);
					att->packageHelpText = (char *)buf.Buffer();
						
					// do help only if non zero length string
					if (!*att->packageHelpText)
						att->packageHelpText = NULL;
					break;
				}
#else
				case A_PACKAGE_HELP: {
					err = fPackData->SkipData(fArcFile,&header);
					att->packageHelpText = NULL;
					break;
				}
#endif

#if (!SEA_INSTALLER)
				case A_SPLASH_SCREEN: {
					// fixed to work with the new TranslationKit (if available)
				#ifdef _BITMAP_STREAM_H
					BBitmapStream	mapStream;
				#else
					BitmapStream	mapStream;
				#endif
					err = fPackData->ReadBin(&mapStream,fArcFile,0,NULL);
					
					//bitHeader = (DATABitmap *)buf.Buffer();
					
					BBitmap	*newMap;
				#ifdef _BITMAP_STREAM_H
					if (mapStream.DetachBitmap(&newMap) >= B_OK) {
				#else
					if (mapStream.DetachBitmap(newMap) >= B_OK) {
				#endif
						delete att->splashBitmap;
						att->splashBitmap = newMap;
					}
					break;
				}
#endif
				case A_PACK_NAME:
					err = fPackData->ReadString(fArcFile, &att->name);
					break;
				case A_PACK_VERSION:
					err = fPackData->ReadString(fArcFile, &att->version);
					break;
				case A_PACK_DEVELOPER:
					err = fPackData->ReadString(fArcFile, &att->developer);
					break;
				case A_PACK_REL_DATE:
					err = fPackData->ReadInt32(fArcFile, &att->releaseDate);
					break;
				case A_PACK_DESCRIPTION:
					err = fPackData->ReadString(fArcFile, &att->description);
					break;
#if (DOES_REGISTRY)
				case A_PACK_DEPOT_SERIAL:
					err = fPackData->ReadString(fArcFile, &att->devSerial);
					break;
				case A_SOFT_TYPE:
					int32	data;
					err = fPackData->ReadInt32(fArcFile,&data);
					att->downloadInfo.AddInt32("softtype",data);
					break;
				case A_PREFIX_ID:
					err = fPackData->ReadString(fArcFile,sbuf,256);
					att->downloadInfo.AddString("pid",sbuf);
					break;
				case A_VERSION_ID:
					err = fPackData->ReadString(fArcFile,sbuf,256);
					att->downloadInfo.AddString("vid",sbuf);
					break;
#endif
				case A_PACK_FLAGS:
					int32 flags;
					err = fPackData->ReadInt32(fArcFile, &flags);
					att->doReg = flags & PKGF_DOREG;
					att->doUpdate = flags & PKGF_DOUPDATE;
					att->abortScripts = flags & PKGF_ABORTSCRIPT;
					break;
				case END_TYPE:
					readingEntries = false;
					break;
				default:
					PRINT(("unknown code, skipping\n"));
					err = fPackData->SkipData(fArcFile,&header);
					break;
			}
		}
#if (!SEA_INSTALLER)		
		// now read in the download-meta info (if any)
		err = fArcFile->Seek(-4,SEEK_END);
		
		int32 bytes = 0;
		err = fArcFile->Read(&bytes,sizeof(bytes));
		bytes = B_BENDIAN_TO_HOST_INT32(bytes);
	
		BMessage	&metaData = att->downloadInfo;
	
		if (bytes == 'DIn_') {
			BPath	p;
			pEntry->GetPath(&p);
			FILE	*f;
			f = fopen(p.Path(), "r+");
			if (f == NULL) {
				err = errno;
			}
			err = fseek(f,-8,SEEK_END);
			err = fread(&bytes,1,sizeof(bytes),f);
			bytes = B_BENDIAN_TO_HOST_INT32(bytes);
			
			err = fseek(f,-bytes,SEEK_END);
			
			char linebuf[128];
			char *max = linebuf + 128;
			
			// now read in the meta list
			while (fgets(linebuf,128,f) != NULL) {
				if (*linebuf == 0 ||
					*linebuf == '\n' ||
					*linebuf == '\r')
					break;
					
				//
				char *hname = linebuf;
				char *c = hname;
				
				while(*c && *c != ':' && c < max) {
		        	*c = tolower(*c);
		        	c++;
		        }
		        *c = 0;
		        c++;
		        while (*c == ' ' && *c)
		        	c++;
		        
		        char *hval = c;
		        
		        // chop off extra newline or carriage-return
		        while (*c)
		        	c++;
		        if (c > hval && (*(c-1) == '\n' || *(c-1) == '\r'))
		        	*(c-1) = 0;
		        ReplaceString(&metaData,hname,hval);
			}
			fclose(f);
		}
#endif
	}
	catch (ErrHandler::OSErr e) {
		serr = e.err;	
	}
	
	
	return serr;	
}

#if 0
long InstallPack::ReadAttribHeader(ushort &ode, long &dataSize)
{
	lng err;
	AttribHeader head;
	
	err = pFile->Read(&head,sizeof(head));
	code = head.code;
	dataSize = head.size;
	
	return err;
}
#endif

///////////////////////////////////////////////////////////////////////////
long InstallPack::ReadMasterGroupList(GroupList *groupList)
{
	status_t err = B_NO_ERROR;
		
	record_header	header;
	
	for (;;) {
		err = fPackData->ReadRecordHeader(fArcFile,&header);
		if (header.what == ID_GRP_ITEM && header.type == LIS_TYPE) {
			bool reading = true;
			GroupItem *grp = new GroupItem;
			
			while (reading) {
				fPackData->ReadRecordHeader(fArcFile,&header);
				switch(header.what) {
					case ID_GRP_NAME:
						fPackData->ReadString(fArcFile,&grp->name);
						break;
					case ID_GRP_DESC:
						fPackData->ReadString(fArcFile,&grp->description);
						break;
					case ID_GRP_HTXT:
						fPackData->ReadString(fArcFile,&grp->helpText);
						// grp->doHelp = (*grp->helpText);
						if (!*grp->helpText)
							grp->helpText = NULL;
						break;
					case END_TYPE:
						reading = false;
						break;
					default:
						fPackData->SkipData(fArcFile,&header);
				}
			}
			groupList->masterList->AddItem(grp);
			// ???
			groupList->reverseList->AddItem(0);
		}
		else if (header.type == END_TYPE) {
			break;
		}
		else {
			fPackData->SkipData(fArcFile,&header);
		}
	}
	return err;
}

long InstallPack::ReadViewGroupList(GroupList *groupList)
{
	status_t err = B_NO_ERROR;

	record_header	header;
	int i = 0;
	for (;;) {
		err = fPackData->ReadRecordHeader(fArcFile,&header);
		if (header.what == ID_GRP_INDX && header.type == INT_TYPE) {
			IndexItem *grp = new IndexItem();
			fPackData->ReadInt32(fArcFile,&grp->index);
			groupList->viewList->AddItem(grp);	
			
			if (grp->index != -1)
				groupList->reverseList->Items()[grp->index] = i++;
		}
		else if (header.type == END_TYPE)
			break;
		else {
			fPackData->SkipData(fArcFile,&header);
		}
	}
	return err;
}

long InstallPack::ReadDefaultDestList(RList<DestItem *> *def)
{
	status_t err = B_NO_ERROR;

	record_header	header;
	for (;;) {
		err = fPackData->ReadRecordHeader(fArcFile,&header);
		if (header.what == ID_DEST_PATH && header.type == LIS_TYPE) {
			DestItem *dst = 0;
			for (;;) {
				err = fPackData->ReadRecordHeader(fArcFile,&header);
				if (header.what == ID_PATHNAME)
				{
					char *path;
					fPackData->ReadString(fArcFile,&path);
					dst = new DestItem(path);
					free(path);
				}
				else if (header.what == ID_FIND_DEST)
				{
					int32 code;
					fPackData->ReadInt32(fArcFile,&code);
					if (dst) {
						dst->findCode = code;
					}
				}
				else if (header.type == END_TYPE)
				{
					break;
				}
				else
				{
					fPackData->SkipData(fArcFile,&header);
				}
			}
			if (dst) def->AddItem(dst);
		}
		else if (header.type == END_TYPE)
			break;
		else {
			fPackData->SkipData(fArcFile,&header);
		}
	}	
	
	return err;
}


/// custom destinations, consist solely of pathnames
long InstallPack::ReadCustomDestList(RList<DestItem *> *customDest)
{
	status_t err = B_NO_ERROR;
	
	record_header	header;
	for (;;) {
		err = fPackData->ReadRecordHeader(fArcFile,&header);
		if (header.what == ID_DEST_PATH && header.type == LIS_TYPE) {
			DestItem *dst = 0;
			for (;;) {
				err = fPackData->ReadRecordHeader(fArcFile,&header);
				if (header.what == ID_PATHNAME)
				{
					char *path;
					fPackData->ReadString(fArcFile,&path);
					dst = new DestItem(path);
					free(path);
				}
				else if (header.type == END_TYPE)
				{
					break;
				}
				else {
					fPackData->SkipData(fArcFile,&header);
				}
			}
			if (dst) customDest->AddItem(dst);
		}
		else if (header.what == ID_DEST_QUERY && header.type == LIS_TYPE) {
			int64 size;
			char *name = 0;
			char *mime = 0;
			for (;;) {
				err = fPackData->ReadRecordHeader(fArcFile,&header);
				if (header.what == ID_QUERY_SIZE) {
					fPackData->ReadInt64(fArcFile,&size);
				}
				else if (header.what == ID_QUERY_TITLE) {
					fPackData->ReadString(fArcFile,&name);
				}
				else if (header.what == ID_QUERY_MIME) {
					fPackData->ReadString(fArcFile,&mime);
				}
				else if (header.type == END_TYPE) {
					break;
				}
				else {
					fPackData->SkipData(fArcFile,&header);
				}
			}
			if (name && mime)
				customDest->AddItem(new FindItem(name,size,mime));
		}
		else if (header.type == END_TYPE)
			break;
		else {
			fPackData->SkipData(fArcFile,&header);
		}
	}	
	
	return err;
}

#if 0
long InstallPack::ReadCompressedBitmap(BBitmap **bmap)
{
	long err;
	
	if (*bmap) {
		delete *bmap;
		*bmap = NULL;
	}
	CStream	stream;	
	stream.SetFile(pFile);
	err = stream.COpen(O_RDONLY);
	if (err < B_NO_ERROR) {
		PRINT(("error opening cstream\n"));
		return err;
	}
	// read the DATABitmap header
	DATABitmap	header;
		
	err = stream.CRead((const char *)&header,sizeof(header));
	
	if (err != sizeof(header)) {
		PRINT(("error reading header\n"));
		return err;
	}
	if (header.dataSize) {
		*bmap = new BBitmap(header.bounds,header.colors);
#if DEBUG
		header.bounds.PrintToStream();
#endif
		PRINT(("bitmap data size is %d bytes\n",(*bmap)->BitsLength()));
		err = stream.CRead((char *)(*bmap)->Bits(),(*bmap)->BitsLength());
		
		PRINT(("cread returned %d\n",err));
		if (err < B_NO_ERROR) {
			delete *bmap;
			*bmap = NULL;
		}
	}			
	return err;
}
#endif
