#include <Be.h>

//////////////////////////
///  New file format stuff
//////////////////////////

#include "PackArc.h"
#include "Attributes.h"
#include "StLocker.h"
#include "AutoPtr.h"
#include "SPackData.h"
#include "DataID.h"
#include "MMallocIO.h"

#if (!__INTEL__)
#include "BitmapStream.h"
#include <DataFormats.h>
#else
#include <BitmapStream.h>
#endif

#include "SAttrIO.h"



#include <sys/socket.h>
#include <netinet/in.h>
#include "Util.h"

#include "MyDebug.h"

enum {
	PATH_ITEM = 0x00,
	FIND_ITEM = 0x01
};


inline void	strassign(char *&dst, char *src);

inline void	strassign(char *&dst, char *src)
{
	if (!src) return;
	if (dst) free(dst);
	dst = src;
}


status_t PackArc::WriteMasterGroupList(GroupList *groupList)
{
	status_t	err = B_NO_ERROR;
		
	StBLock locker(&(groupList->lock));
	PRINT(("got group list lock, writing master groups...\n"));

	fPackData->WriteRecordHeader(fArcFile,A_MASTER_GROUP_LIST,LIS_TYPE,0);

	long count = groupList->masterList->CountItems();	
	for(long i = 0; i < count; i++) {
		GroupItem *grp = groupList->masterList->ItemAt(i);
		
		err = fPackData->WriteRecordHeader(fArcFile,ID_GRP_ITEM,LIS_TYPE,0);
		
		err = fPackData->WriteString(fArcFile,grp->name,ID_GRP_NAME);
		char c = 0;
		if (grp->description) fPackData->WriteString(fArcFile,grp->description,ID_GRP_DESC);
		else fPackData->WriteString(fArcFile,&c,ID_GRP_DESC);
			
		if (!grp->helpText || !grp->doHelp) fPackData->WriteString(fArcFile,&c,ID_GRP_HTXT);
		else fPackData->WriteString(fArcFile,grp->helpText,ID_GRP_HTXT);
		
		fPackData->WriteEndHeader(fArcFile);
	}
	fPackData->WriteEndHeader(fArcFile);
	
	return err;
}

status_t PackArc::WriteViewGroupList(GroupList *groupList)
{
	status_t	err = B_NO_ERROR;
	
	StBLock locker(&(groupList->lock));
	PRINT(("writing view groups..."));
	
	fPackData->WriteRecordHeader(fArcFile,A_VIEW_GROUP_LIST,LIS_TYPE,0);
	long count = groupList->viewList->CountItems();
	for(long i = 0; i < count; i++) {
		IndexItem *grp = groupList->viewList->ItemAt(i);
		
		fPackData->WriteInt32(fArcFile,grp->index,ID_GRP_INDX);
	}
	fPackData->WriteEndHeader(fArcFile);
	return err;
}

status_t PackArc::WriteDefaultDestList(RList<DestItem *> *def)
{
	status_t	err = B_NO_ERROR;	
	
	fPackData->WriteRecordHeader(fArcFile,A_DEFAULT_DEST_LIST,LIS_TYPE,0);
	
	PRINT(("writing default destinations..."));
	long count = def->CountItems();
	for(long i = 0; i < count; i++) {
		DestItem *dst = def->ItemAt(i);
		
		fPackData->WriteRecordHeader(fArcFile,ID_DEST_PATH,LIS_TYPE,0);
		fPackData->WriteString(fArcFile, dst->path, ID_PATHNAME);
		if (dst->findCode != D_NO_DEST)
			fPackData->WriteInt32(fArcFile, dst->findCode, ID_FIND_DEST);
		fPackData->WriteEndHeader(fArcFile);
	}
	fPackData->WriteEndHeader(fArcFile);
	
	PRINT(("done...\n"));
	return err;
}

status_t PackArc::WriteCustomDestList(DestList *customDestList)
{
	status_t	err = B_NO_ERROR;
	
	StBLock locker(&(customDestList->lock));
	PRINT(("custom dest list locked... writing destinations..."));
	
	fPackData->WriteRecordHeader(fArcFile,A_CUST_DEST_LIST,LIS_TYPE,0);
	
	long count = customDestList->CountItems();
	for(long i = 0; i < count; i++) {
		DestItem *dst = customDestList->ItemAt(i);
		bool	doFindData = FALSE;
		int32 	code;
		
		if (typeid(*dst) == typeid(DestItem))
			code = ID_DEST_PATH;
		else if (typeid(*dst) == typeid(FindItem)) {
			code = ID_DEST_QUERY;
		}
		fPackData->WriteRecordHeader(fArcFile,code,LIS_TYPE,0);
		
		if (code == ID_DEST_PATH) {
			fPackData->WriteString(fArcFile, dst->path, ID_PATHNAME);		
		}
		else if (code == ID_DEST_QUERY) {
			FindItem *fItem = (FindItem *)dst;

			const char	*signature = fItem->Signature();			
			off_t	fsize = fItem->size;
			
			// write 64-bit siz
			fPackData->WriteString(fArcFile,fItem->path,ID_QUERY_TITLE);
			fPackData->WriteInt64(fArcFile,fsize,ID_QUERY_SIZE);
			fPackData->WriteString(fArcFile,signature,ID_QUERY_MIME);
		}
		fPackData->WriteEndHeader(fArcFile);
	}
	fPackData->WriteEndHeader(fArcFile);
	PRINT(("done... \n"));
	return err;
}

status_t PackArc::WriteDoLicense(bool doLicense, BPath &lfile)
{
	// write a path to the license agreement
	status_t err = B_NO_ERROR;
	
	PRINT(("writing license..."));
	
	fPackData->WriteRecordHeader(fArcFile,A_LICENSE_FILE,LIS_TYPE,0);
	
	fPackData->WriteInt32(fArcFile,doLicense,ID_DO_LICENSE);
	
	const char *pth = lfile.Path();
	if (!pth) {
		char c = 0;
		fPackData->WriteString(fArcFile,&c,ID_LICENSE_PATH);
	}	
	else {
		fPackData->WriteString(fArcFile,pth,ID_LICENSE_PATH);
	}
	fPackData->WriteEndHeader(fArcFile);
	
	PRINT(("done...\n"));
	return err;
}

#if __INTEL__
class BitmapHeaderSwap : public BBitmapStream
{
public:
	BitmapHeaderSwap()
		: BBitmapStream(NULL)
	{
	};
	void SwapHeader(
				const TranslatorBitmap *	source,
				TranslatorBitmap *			destination)
	{
		BBitmapStream::SwapHeader(source,destination);
	};
};
#endif

status_t PackArc::WriteBitmap(BBitmap *bmap, int32 what)
{
	status_t err = B_NO_ERROR;
	off_t encsz, decsz;
	encsz = decsz = -1;
	int flags = 0;
	
	if (bmap) {
#if (__INTEL__)

#if 0
		// this gave me a white bitmap!
		BBitmapStream	bstream(bmap);
		fPackData->WriteBin(fArcFile,&bstream,&encsz,&decsz,&flags,NULL,what);
		bstream.DetachBitmap(&bmap);
		
#else		
		// make a new buffer
		MMallocIO	buf(NULL, sizeof(TranslatorBitmap) + bmap->BitsLength());
		TranslatorBitmap	src;
		TranslatorBitmap	*header = (TranslatorBitmap *)buf.Buffer();
			
		// write the DATABitmap header
		src.magic = B_TRANSLATOR_BITMAP;
		
		BRect r = bmap->Bounds();
		src.bounds = r;
		
		src.rowBytes = bmap->BytesPerRow();
		color_space cspace = bmap->ColorSpace();
		//swap_data(B_ANY_TYPE,&cspace,sizeof(cspace),B_SWAP_HOST_TO_BENDIAN);
		src.colors = cspace;
		src.dataSize = bmap->BitsLength();
		
		BitmapHeaderSwap swap;
		swap.SwapHeader(&src,header);
		
		// bad news!
		memcpy((char *)header + sizeof(TranslatorBitmap), bmap->Bits(), bmap->BitsLength());
		fPackData->WriteBin(fArcFile,&buf,&encsz,&decsz,&flags,NULL,what);
#endif
#else
		// make a new buffer
		MMallocIO	buf(NULL, sizeof(DATABitmap) + bmap->BitsLength());
		DATABitmap	*header = (DATABitmap *)buf.Buffer();
			
		// write the DATABitmap header
		header->magic = DATA_BITMAP;
		header->bounds = bmap->Bounds();
		header->rowBytes = bmap->BytesPerRow();
		header->colors = bmap->ColorSpace();
		header->dataSize = bmap->BitsLength();
		
		// bad news!
		memcpy((char *)header + sizeof(DATABitmap), bmap->Bits(), header->dataSize);
		fPackData->WriteBin(fArcFile,&buf,&encsz,&decsz,&flags,NULL,what);
#endif
	}
	return err;
}

////

/***
long PackArc::WriteAttribHeader(ushort code, off_t startOff)
{
	off_t			endOff;
	status_t		err;
	AttribHeader	head;
	
	endOff = fArcFile->Position();

	head.code = code;
	head.size = endOff-startOff;
	
	PRINT(("data size is %d bytes\n",head.size));
	
	fArcFile->Seek(startOff - sizeof(AttribHeader),SEEK_SET);
	
	// need to make portable!
	err = fArcFile->Write(&head,sizeof(AttribHeader));
	fArcFile->Seek(endOff,SEEK_SET);
	return err;
}
***/

//////// error checking!!!!!!!! /////////////
long PackArc::NewWriteAttributes(	AttribData *att ,long fileCount)
{
	PRINT(("PackArc::NewWriteAttributes\n"));

	// write out groups, destinations
	// file open
	// Seek to the proper location
	status_t	err = B_NO_ERROR;	

	PRINT(("		writing to attributes to %d\n",attribOffset));

	off_t		off = fArcFile->Position();
	int32		pos = off;

	PRINT(("		position is %d\n",pos));		
	GroupList *groupList = att->groupList;
	RList<DestItem *>	*defaultDestList = att->defaultDestList;
	DestList			*customDestList = att->customDestList;
	const char			*descriptionText = att->descriptionText;
	BPath				&licenseFile = att->licenseFile;
	bool				doLicense = att->showLicense;
	bool				doInstallFolder = att->doInstallFolder;
	//////////////////////////////////////////////
	
	fPackData->WriteRecordHeader(fArcFile,ID_PKGATTR,LIS_TYPE,0);
	
	WriteMasterGroupList(groupList);
	WriteViewGroupList(groupList);
	WriteDefaultDestList(defaultDestList);
	WriteCustomDestList(customDestList);
	WriteBitmap(att->splashBitmap,A_SPLASH_SCREEN);
	WriteDoLicense(doLicense, licenseFile);
	
	fPackData->WriteString(fArcFile, descriptionText, A_INST_DESCRIPTION);
	fPackData->WriteInt32(fArcFile, fileCount, A_FILECOUNT);
	fPackData->WriteInt32(fArcFile, doInstallFolder, A_INSTALLFOLDER);
	fPackData->WriteInt32(fArcFile,att->doFolderPopup,A_FOLDER_POPUP);

	off_t encsz, decsz;
	int flags;	
	{
		MMallocIO	buf(att->packageHelpText, strlen(att->packageHelpText)+1);
	
		encsz = decsz = -1;
		flags = 0;
		fPackData->WriteBin(fArcFile,&buf,&encsz,&decsz,&flags,NULL,A_PACKAGE_HELP);
	}
	if (doLicense) {
		// encode all
		encsz = decsz = -1;
		flags = 0;
		BFile	lFile(licenseFile.Path(),O_RDONLY);
		if (lFile.InitCheck() == B_NO_ERROR) {
			fPackData->WriteBin(fArcFile,&lFile,&encsz,&decsz,&flags,NULL,A_LICENSE);
			
			// get the styles
			// encode all
			encsz = decsz = -1;
			flags = 0;
	
			attr_info	ai;
			if (lFile.GetAttrInfo("styles",&ai) == B_NO_ERROR) {
				SAttrIO		aIO(&lFile,"styles",&ai);
				
				fPackData->WriteBin(fArcFile,&aIO,&encsz,&decsz,
									&flags,NULL,A_LICENSE_STYLE);
			}
		}
	}
	
	fPackData->WriteString(fArcFile,att->name,A_PACK_NAME);
	fPackData->WriteString(fArcFile,att->version,A_PACK_VERSION);
	fPackData->WriteString(fArcFile,att->developer,A_PACK_DEVELOPER);
	fPackData->WriteString(fArcFile,att->description,A_PACK_DESCRIPTION);
	fPackData->WriteInt32(fArcFile,att->releaseDate,A_PACK_REL_DATE);
	fPackData->WriteString(fArcFile,att->serialno,A_PACK_DEPOT_SERIAL);
	
	int32 pflags = 0;
	//if (att->isUpdate) pflags |= PKGF_ISUPDATE;
	if (att->doReg) pflags |= PKGF_DOREG;
	if (att->doUpdate) pflags |= PKGF_DOUPDATE;
	if (att->abortScript) pflags |= PKGF_ABORTSCRIPT;
	fPackData->WriteInt32(fArcFile,pflags,A_PACK_FLAGS);
	
	fPackData->WriteInt32(fArcFile,att->softType,A_SOFT_TYPE);
	fPackData->WriteString(fArcFile,att->prefixID,A_PREFIX_ID);
	fPackData->WriteString(fArcFile,att->versionID,A_VERSION_ID);
	/// last attribute marker ///
	fPackData->WriteEndHeader(fArcFile);
	
	return err;
}

/***
long PackArc::ReadAttribHeader(ushort &code, long &dataSize)
{
	PRINT(("PackArc::ReadAttribHeader\n"));
	
	long err;
	AttribHeader head;

	err = fArcFile->Read(&head,sizeof(head));
	
	code = head.code;
	dataSize = head.size;
	
	return err;
}
***/

///////////////////////////////////////////////////////////////////////////
status_t PackArc::ReadMasterGroupList(GroupList *groupList)
{
	status_t err = B_NO_ERROR;
	StBLock	locker(&(groupList->lock));
		
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
						grp->doHelp = (*grp->helpText);
						break;
					case END_TYPE:
						reading = false;
						break;
					default:
						fPackData->SkipData(fArcFile,&header);
				}
			}
			groupList->masterList->AddItem(grp);
			// the reverse list points to the view list
			// fill in dummy entries for now
			groupList->reverseList->AddItem(0);
		}
		else if (header.type == END_TYPE) {
			break;
		}
		else {
			fPackData->SkipData(fArcFile,&header);
		}
	}

/****	
	for(long i = 0; i < count; i++) {
		GroupItem *grp = new GroupItem;

		// group name
		err = fArcFile->Read(&lcount,sizeof(short));
		lcount = ntohs(lcount);
		char *gname = new char[lcount+1];
		err = fArcFile->Read(gname,(long)lcount);
		gname[lcount] = '\0';
		grp->name = gname;
		
		// description
		err = fArcFile->Read(&lcount,sizeof(short));
		lcount = ntohs(lcount);
		char *gdesc = new char[lcount+1];
		err = fArcFile->Read(gdesc,(long)lcount);
		gdesc[lcount] = '\0';
		grp->dLength = err;
		grp->description = gdesc;
			
		// group help text (eventually make compressed)
		err = fArcFile->Read(&lcount,sizeof(short));
		lcount = ntohs(lcount);
		gdesc = new char[lcount+1];
		err = fArcFile->Read(gdesc,(long)lcount);
		gdesc[lcount] = '\0';
		grp->hLength = err;
		grp->helpText = gdesc;
		grp->doHelp = (grp->hLength > 0);

		groupList->masterList->AddItem(grp);
		groupList->reverseList->AddItem(0);
	}
****/
	return err;
}

status_t PackArc::ReadViewGroupList(GroupList *groupList)
{
	status_t err = B_NO_ERROR;
	StBLock locker(&(groupList->lock));

	record_header	header;
	int i = 0;
	for (;;) {
		err = fPackData->ReadRecordHeader(fArcFile,&header);
		if (header.what == ID_GRP_INDX && header.type == INT_TYPE) {
			IndexItem *grp = new IndexItem();
			fPackData->ReadInt32(fArcFile,&grp->index);
			groupList->viewList->AddItem(grp);	
			
			
			// indices to master list
			if (grp->index != -1) {
				PRINT(("mgroup %s viewindex %d\n",groupList->masterList->ItemAt(grp->index)->name,
													i));
				groupList->reverseList->Items()[grp->index] = i;				
			}
			i++;
		}
		else if (header.type == END_TYPE)
			break;
		else {
			err = fPackData->SkipData(fArcFile,&header);
			if (err) break;
		}
	}
	return err;
}



status_t PackArc::ReadDefaultDestList(RList<DestItem *> *def)
{
	status_t err = B_NO_ERROR;

	record_header	header;
	for (;;) {
		err = fPackData->ReadRecordHeader(fArcFile,&header);
		if (header.what == ID_DEST_PATH && header.type == LIS_TYPE) {
			DestItem *dst = 0;
			for (;;) {
				err = fPackData->ReadRecordHeader(fArcFile,&header);
				if (header.what == ID_PATHNAME) {
					char *path;
					fPackData->ReadString(fArcFile,&path);
					dst = new DestItem(path);
					free(path);
				}
				else if (header.what == ID_FIND_DEST) {
					int32 code;
					fPackData->ReadInt32(fArcFile,&code);
					if (dst) {
						dst->findCode = code;
						free(dst->findName);
						dst->findName = NULL;
						for (int i = 0; i < kPathsCount; i++) {
							if (kPaths[i].code == code) {
								dst->findName = strdup(kPaths[i].name);
								break;
							}
						}
						if (!dst->findName)
							dst->findName = strdup("UNKNOWN_DESTINATION");
					}
				}
				else if (header.type == END_TYPE) {
					break;
				}
				else {
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

long PackArc::ReadCustomDestList(DestList *customDest)
{
	status_t err = B_NO_ERROR;
	
	StBLock locker(&(customDest->lock));

	record_header	header;
	for (;;) {
		err = fPackData->ReadRecordHeader(fArcFile,&header);
		if (header.what == ID_DEST_PATH && header.type == LIS_TYPE) {
			DestItem *dst = 0;
			for (;;) {
				err = fPackData->ReadRecordHeader(fArcFile,&header);
				if (header.what == ID_PATHNAME) {
					char *path;
					fPackData->ReadString(fArcFile,&path);
					dst = new DestItem(path);
					free(path);
				}
				else if (header.type == END_TYPE) {
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
			if (name && mime) {
				customDest->AddItem(new FindItem(name,size,mime));
				free(name);
				free(mime);	
			}
		}
		else if (header.type == END_TYPE)
			break;
		else {
			fPackData->SkipData(fArcFile,&header);
		}
	}	
	
	return err;
}

long PackArc::ReadDoLicense(bool *doLicense, BPath *lfile)
{
	long err;
	
	record_header	header;
	for (;;) {
		err = fPackData->ReadRecordHeader(fArcFile,&header);
		if (header.what == ID_DO_LICENSE) {
			int32 v;
			fPackData->ReadInt32(fArcFile,&v);
			*doLicense = v;
		}
		else if (header.what == ID_LICENSE_PATH) {
			char *buf;
			fPackData->ReadString(fArcFile,&buf);
			lfile->SetTo((char *)buf);
			free(buf);	
		}
		else if (header.type == END_TYPE) {
			break;
		}
		else
			fPackData->SkipData(fArcFile,&header);
	}
	return err;
}

// use exceptions
long PackArc::NewReadAttributes(	AttribData *att )
{
	PRINT(("entering new read attributes\n"));
	status_t	err;
	bool		c;
	
	GroupList *groupList = att->groupList;
	RList<DestItem *> *defaultDestList = att->defaultDestList;
	DestList *customDestList = att->customDestList;

	SetFile(O_RDONLY);
	fPackData = new SPackData(statusMessenger,&c);
	
	record_header	header;		
	PRINT(("		seeking to attribOffset %d\n",attribOffset));
	err = fArcFile->Seek(attribOffset,SEEK_SET);

	fPackData->ReadRecordHeader(fArcFile,&header);
	if (header.what != ID_PKGATTR || header.type != LIS_TYPE) {

		return B_ERROR;
	}
	int32 b;
	bool readingEntries = true;
	while(readingEntries) {
		err = fPackData->ReadRecordHeader(fArcFile,&header);
		PRINT(("header code %d\n",header.what));
		// off_t startOff = fArcFile->Position();
		char *buf;
		switch (header.what) {
			case A_MASTER_GROUP_LIST:
				PRINT(("got master group list\n"));
				ReadMasterGroupList(groupList);
				// check errors and size
				break;
			case A_VIEW_GROUP_LIST:
				ReadViewGroupList(groupList);
				break;
			case A_DEFAULT_DEST_LIST:
				ReadDefaultDestList(defaultDestList);
				break;
			case A_CUST_DEST_LIST:
				ReadCustomDestList(customDestList);				
				break;
			case A_INST_DESCRIPTION:
				fPackData->ReadString(fArcFile,&buf);
				strassign(att->descriptionText,buf);
				break;
			case A_INSTALLFOLDER:
				fPackData->ReadInt32(fArcFile,&b);
				att->doInstallFolder = b;
				break;
			case A_FOLDER_POPUP:
				fPackData->ReadInt32(fArcFile,&b);
				att->doFolderPopup = b;
				break;
			case A_LICENSE_FILE:
				ReadDoLicense(&att->showLicense, &att->licenseFile);
				break;
			case A_PACKAGE_HELP: {
				MMallocIO	buf;
				buf.SetDispose(false);
				
				// eventually do styled text!
				// read compressed data
				fPackData->ReadBin(&buf,fArcFile,0,NULL);
					
				att->packageHelpText = (char *)buf.Buffer();
				
				// do help only if non zero length string
				att->doPackageHelp = *att->packageHelpText;
				break;
			}
			case A_SPLASH_SCREEN: {
#if __INTEL__
				BBitmapStream	buf;
#else
				BitmapStream	buf;
#endif
				fPackData->ReadBin(&buf,fArcFile,0,NULL);
				delete att->splashBitmap;
#if __INTEL__
				buf.DetachBitmap(&att->splashBitmap);
#else
				buf.DetachBitmap(att->splashBitmap);
#endif			
				break;
			}
			case A_PACK_NAME:
				PRINT(("A_PACK_NAME\n"));
				fPackData->ReadString(fArcFile,&buf);
				PRINT(("package name %s\n",buf));
				strassign(att->name,buf);
				break;
			case A_PACK_VERSION:
				PRINT(("A_PACK_VERSION\n"));
				fPackData->ReadString(fArcFile, &buf);
				strassign(att->version,buf);
				break;
			case A_PACK_DEVELOPER:
				PRINT(("A_PACK_DEVELOPER\n"));
				fPackData->ReadString(fArcFile, &buf);
				strassign(att->developer,buf);
				break;
			case A_PACK_REL_DATE:
				fPackData->ReadInt32(fArcFile, (int32 *)&att->releaseDate);
				break;
			case A_PACK_DESCRIPTION:
				fPackData->ReadString(fArcFile, &buf);
				strassign(att->description,buf);
				break;
			case A_PACK_DEPOT_SERIAL:
				fPackData->ReadString(fArcFile, &buf);
				strassign(att->serialno,buf);
				break;
			case A_PACK_FLAGS:
				int32 flags;
				fPackData->ReadInt32(fArcFile, &flags);
				
				att->doReg = flags & PKGF_DOREG;
				att->doUpdate = flags & PKGF_DOUPDATE;
				att->abortScript = flags & PKGF_ABORTSCRIPT;
				break;
			case A_SOFT_TYPE:
				fPackData->ReadInt32(fArcFile, &att->softType);
				break;
			case A_PREFIX_ID:
				fPackData->ReadString(fArcFile, &buf);
				strassign(att->prefixID,buf);
				break;
			case A_VERSION_ID:
				fPackData->ReadString(fArcFile, &buf);
				strassign(att->versionID,buf);
				break;
			case END_TYPE:
				readingEntries = false;
				break;
			default:
				PRINT(("unknown code %d, skipping\n",header.what));
				fPackData->SkipData(fArcFile,&header);
				break;
		}
	}
	
	ClearFile();
	delete fPackData;
	
	return err;	
}
