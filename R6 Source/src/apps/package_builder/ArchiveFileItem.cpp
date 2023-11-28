#include <Be.h>
// ArchiveFileItem.cpp

#include "FArchiveItem.h"
#include "PackArc.h"

#include "GlobalIcons.h"
#include "PackMessages.h"
#include "DestinationList.h"

#include "Replace.h"
#include "ExtractUtils.h"
#include <Path.h>

#include "Util.h"
#define MODULE_DEBUG 0
#include "MyDebug.h"
#include "MyPEF.h"

ArchiveFileItem::ArchiveFileItem()
	: ArchiveItem()
{
	// info needs to be filled in from disk
	fDependList = NULL;
	fType = NULL;
	fSignature = NULL;
}

ArchiveFileItem::ArchiveFileItem(BEntry &theFile, struct stat *stBuf)
	: ArchiveItem(theFile)
{
	fDependList = NULL;
	fType = NULL;
	fSignature = NULL;
	//adler32 = 0;
	//seekOffset = -1;
	
	// get the info
	name = strdup(fRef.name);

	struct stat		statBuf;
	if (!stBuf) {
		stBuf = &statBuf;
		theFile.GetStat(stBuf);
	}	
	uncompressedSize = stBuf->st_size;
	mode = stBuf->st_mode;
	
	creationTime = stBuf->st_crtime;
	modTime = stBuf->st_mtime;
	
	// get mime type
	char mimestr[B_MIME_TYPE_LENGTH];
	if (!GetEntryType(&theFile,mimestr))
		fType = strdup(mimestr);
	else
		fType = strdup(B_EMPTY_STRING);
	
	// doError("Mime type is %s\n",mimestr);
	if (!strcmp(fType,B_APP_MIME_TYPE)) {
		GetEntrySignature(&theFile,mimestr);
		if (*mimestr) fSignature = strdup(mimestr);
	}
	// this might need freeing??
	void *iconData = NULL;
	smallIcon = gGenericFileIcon;
		
	// scan PEF/PE files
	CheckDepends();
}

ArchiveFileItem::~ArchiveFileItem()
{
	if (smallIcon != NULL &&
		smallIcon != gGenericFileIcon &&
		smallIcon != gPatchIcon) {
		// don't do this!!! free(smallIcon->Bits());
		delete smallIcon;
	}
	if (fDependList) {
		for (long i = fDependList->CountItems()-1; i >= 0; i--) {
			free(fDependList->ItemAt(i));
		}
		delete fDependList;
	}
	if (fType)
		free(fType);
	if (fSignature)
		free(fSignature);
}

void ArchiveFileItem::CheckDepends()
{
	PRINT(("checking dependencies\n"));
	BFile	fil(&fRef,B_READ_ONLY);
	if (fil.InitCheck() < B_NO_ERROR)
		return;

	// get version info here
	// won't work on the opposite platform	
	BAppFileInfo	ainf(&fil);
	version_info	vinfo;
	if (ainf.GetVersionInfo(&vinfo, B_APP_VERSION_KIND) >= B_OK) {
		versionInfo[0] = vinfo.major;
		versionInfo[1] = vinfo.middle;
		versionInfo[2] = vinfo.minor;
		versionInfo[3] = vinfo.variety;
		versionInfo[4] = vinfo.internal;
	}
	else {
		for (int i = 0; i < sizeof(versionInfo)/sizeof(*versionInfo); i++)
			versionInfo[i] = 0;
	}
	
#if 0
	char			pe[2];
	
	///// 
	
	long			res;
	
	// check if Intel PE file
	res = fil.Read(pe,sizeof(pe));
	if (res < sizeof(pe)) {
		goto bailout;
	}
	if (pe[0] == 'M' && pe[1] == 'Z') {
		platform = (1 << 1);
		goto bailout;
	}
#endif

	char			x86header[4];
	
	///// 
	
	long			res;
	
	// check if Intel PE file
	res = fil.Read(x86header,sizeof(x86header));
	if (res < sizeof(x86header)) {
		goto bailout;
	}
	if ((x86header[0] == '\x7f' && x86header[1] == 'E' && x86header[2] == 'L' && x86header[3] == 'F')
		|| (x86header[0] == 'M' && x86header[1] == 'Z')) {
		platform = (1 << 1);
		goto bailout;
	}
	
	FileHeader		pefHeader;
	
	fil.Seek(0,SEEK_SET);
	res = fil.Read(&pefHeader,sizeof(pefHeader));
	if (res < sizeof(pefHeader)) {
		PRINT(("couldn't read pef header\n"));
		goto bailout;
	}
	PRINT(("header %12s\n",&pefHeader));
	pefHeader.magicl = B_BENDIAN_TO_HOST_INT16(pefHeader.magicl);
	pefHeader.magic2 = B_BENDIAN_TO_HOST_INT16(pefHeader.magic2);
	pefHeader.fileTypeID = B_BENDIAN_TO_HOST_INT32(pefHeader.fileTypeID);
	pefHeader.architectureID = B_BENDIAN_TO_HOST_INT32(pefHeader.architectureID);
	
	if (pefHeader.magicl != peffMagicl ||
		pefHeader.magic2 != peffMagic2 ||
		pefHeader.fileTypeID != peffTypeID ||
		pefHeader.architectureID != powerPCID)
	{
		PRINT(("invalid pef file\n"));
		goto bailout;
	}
	
	// valid PowerPC PEF file
	platform = (1 << 0);

#if 0	
	SectionHeader	secHeader;
	LoaderHeader	loadHeader;
	
	if (!fDependList) {
		fDependList = new RList<char *>;
	}
	
	PRINT(("currentVersion %d\n",pefHeader.currentVersion));
	PRINT(("oldDefVersion %d\n",pefHeader.oldDefVersion));
	PRINT(("oldImpVersion %d\n",pefHeader.oldImpVersion));
	
	short	loaderSection = 0;
	loaderSection = pefHeader.numberSections;
	long	loaderOffset = 0;
	
	// read in sections
	/***
	for (long i = 0; i < loaderSection; i++) {
		fil.Read(&secHeader,sizeof(secHeader));
		
		switch(secHeader.regionKind) {
			case kCodeSection:
				PRINT(("code section at offset %d\n",secHeader.containerOffset));
				break;	
			case kDataSection:
				PRINT(("data section at offset %d\n",secHeader.containerOffset));
				break;	
			case kLoaderSection:
				PRINT(("loader section at offset %d\n",secHeader.containerOffset));
				loaderOffset = secHeader.containerOffset;
				break;	
			default:
				PRINT(("other section at offset %d\n",secHeader.containerOffset));
				break;
		}		
	}***/
	
#if 1
	fil.Seek((loaderSection-1)*sizeof(secHeader),SEEK_CUR);
#else
	fil.Seek((loaderSection-1)*sizeof(secHeader),B_SEEK_MIDDLE);
#endif

	fil.Read(&secHeader,sizeof(secHeader));
	loaderOffset = secHeader.containerOffset;
	
	if (!loaderOffset)
		goto bailout;			// couldn't find the loader section

#if 1		
	fil.Seek(loaderOffset,SEEK_SET);
#else
	fil.Seek(loaderOffset,B_SEEK_TOP);
#endif
	fil.Read(&loadHeader,sizeof(loadHeader));
	
	long numImports = loadHeader.numImportFiles;
	
	PRINT(("Import count is %d\n",numImports));
	LoaderImportFileID	loadImport;
	
	long loadStringTable = loaderOffset + loadHeader.stringsOffset;
	
	for (long i = 0; i < numImports; i++) {
		fil.Read(&loadImport,sizeof(loadImport));
		
		long saveSeek;
#if 1
		saveSeek = fil.Position();
#else
		saveSeek = fil.Seek(0,B_SEEK_MIDDLE);
#endif
		fil.Seek(loadStringTable + loadImport.fileNameOffset,SEEK_SET);
		
		char buf[B_FILE_NAME_LENGTH];
		fil.Read(buf,B_FILE_NAME_LENGTH);
		
		fDependList->AddItem(strdup(buf));
		
#if 1
		fil.Seek(saveSeek,SEEK_SET);
#else
		fil.Seek(saveSeek,B_SEEK_TOP);
#endif
	}
#endif

bailout:
	//fil.Close();
	return;
}

RList<char *> *ArchiveFileItem::DependList()
{
	return fDependList;
}

long ArchiveFileItem::CompressItem(PackArc *archiveFile)
{
	if (!dirty || deleteThis)
		return B_NO_ERROR;
		
	status_t	err;
	long		res;
	BMessage upMsg(M_CURRENT_FILENAME);
	
	upMsg.AddString("filename",name);
	archiveFile->statusMessenger.SendMessage(&upMsg);
	
	off_t	totalEntrySize;
	
	PRINT(("calling add file for %s\n",name));
	BFile	theFile(&fRef,O_RDONLY);
	err = theFile.InitCheck();
	if (err < B_OK) {
		res = doError("There was an error opening the file \"%s\" (%s).",name,
					"Continue","Cancel",NULL,err);
		if (res == 0) err = B_OK;
		else err = B_CANCELED;
		return err;
	}
	err = archiveFile->AddNode(&theFile, &totalEntrySize, B_FILE_NODE);
	PRINT(("add file done\n"));
	if (err != B_NO_ERROR && err != B_CANCELED) {
		// option to continue or cancel
		res = doError("There was an error compressing the item \"%s\"",name,
					"Continue","Cancel");
		if (res == 0) err = B_OK;
		else err = B_CANCELED;
			
		// this might allow us to continue
		//archiveFile->ResetStream();
		return err;
	}
	if (err == B_CANCELED) {
		// compression was canceled
		PRINT(("*************Dirty is true for file %s\n",name));
		dirty = true;
		return err;
	}
	compressedSize = totalEntrySize;
	seekOffset = archiveFile->catalogOffset;
	PRINT(("compressed size is %d, by position %d\n",compressedSize));
	
	archiveFile->catalogOffset += compressedSize;
	dirty = false;
		
	ArchiveItem *cur = GetParent();
	while( cur != NULL) {
		cur->compressedSize += compressedSize;
		cur = cur->GetParent();
	}	
	return err;
}

long ArchiveFileItem::ExtractItem(PackArc *archiveFile,BDirectory *destDir)
{
	long err = B_NO_ERROR;
	BMessage upMsg(M_CURRENT_FILENAME);
	
	upMsg.AddString("filename",name);
	archiveFile->statusMessenger.SendMessage(&upMsg);

	if (dirty == TRUE)
		return B_ERROR;

	if (destDir->Contains(name) == TRUE) {
		/// existing item may be a file OR folder, find out which
		BEntry	existingItem;
		destDir->FindEntry(name,&existingItem);
		
		char fname[B_FILE_NAME_LENGTH];
		BEntry destDirEnt;
		destDir->GetEntry(&destDirEnt);
		destDirEnt.GetName(fname);
		
		char mbuf[B_FILE_NAME_LENGTH*2 + 45];
		sprintf(mbuf,"An item with the name \"%s\" already exists in the folder \"%s\".",
						name,fname);
		BAlert *dupAlert = new BAlert(B_EMPTY_STRING,mbuf,"Skip File",
			"Rename Existing","Replace Existing",B_WIDTH_FROM_WIDEST);
		long result = dupAlert->Go();
		switch (result) {
			case 0:
				return err;
				break;
			case 1:
				RenameExisting(destDir,&existingItem," copy");
				break;
			case 2:
				// if this is a folder blow it away!!
				// if the is a file, it will be blown away as well
				err = RecursiveDelete(&existingItem);
				if (err != B_NO_ERROR) {
					doError("Could not remove existing item. Skipping.");
					return err;
				}
				break;
		}
	}
	BFile	outFile;
	BEntry	outEntry;
	PRINT(("creating new file\n"));
	err = destDir->CreateFile(name, &outFile);
	if (err != B_NO_ERROR) {
		doError("Error creating new file");
		return err;	
	}
	destDir->FindEntry(name,&outEntry);
	

	err = archiveFile->ExtractFile(&outFile,compressedSize,seekOffset);
	if (err == B_CANCELED) {
		outEntry.Remove();
		return err;
	}
	if (err != B_NO_ERROR) {
		doError("Encountered an error when decompressing file");
		return err;
	}
	
	// checksumming!
	// archiveFile->ResetStream();
	// off_t fsize;
	// outFile.GetSize(&fsize);
	//if (uncompressedSize != fsize) {
	//	doError("Warning, original file size does not match.");
	// }
	// not sure how this works if the type isn't already there
	// may have to do the setfile charade
	
	// shouldn't need to do this now that we have types!!!
	// SetType(&outEntry,fType);

	// important!!!
	err = outEntry.SetPermissions(mode);
	if (mode & (S_IXOTH | S_IXGRP | S_IXUSR)) {	
		BPath	fpath(destDir,name);
		update_mime_info(fpath.Path(),false,true,false);
	}

	err = outEntry.SetModificationTime(modTime);
	err = outEntry.SetCreationTime(creationTime);

	return err;
}
