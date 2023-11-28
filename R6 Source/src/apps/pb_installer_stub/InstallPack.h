// #include "CStream.h"

#ifndef _INSTALLPACK_H_
#define _INSTALLPACK_H_

#include <Messenger.h>

#include "SimpleListView.h"
#include "RList.h"

#include "DestinationList.h"
#include "FileEntry.h"

#include "InstallerType.h"

#define INBUFSIZ 4096
#define OUTBUFSIZ 4096

typedef struct {
	entry_ref 	appRef;
	ulong		signature;
} AppSet;

class ArchiveItem;
class ArchiveFileItem;
class ArchiveFolderItem;
class ArchivePatchItem;
class ArchiveScriptItem;

class DestManager;
class GroupList;
class PackAttrib;
struct arcHeader;
class SPackData;
class ArcCatalog;

class InstallPack
{

public :	
	InstallPack();
	InstallPack(entry_ref &ref, bool expectRes = TRUE);

	~InstallPack();
	
	// long 	ResRead(long id,void *buf,long bytes, long &curOffset);
	long		BeginDecompression();
	long		EndDecompression();
	long		ExtractFile(BNode *dest, off_t offset);
	
	long		CheckZError(long err);
	void		ResetDecompress();

	/////////////
	status_t 	ReadArcHeader(BFile *f, arcHeader &head);
	status_t	VerifyMagic(BFile *f);
	
	/////////////
	long		NewReadAttributes(PackAttrib *attr);
	long 		ReadAttribHeader(ushort &code, long &dataSize);


	long		ReadMasterGroupList(GroupList *groupList);
	long		ReadViewGroupList(GroupList *groupList);
	long		ReadDefaultDestList(RList<DestItem *> *defaultDestList);
	long		ReadCustomDestList(RList<DestItem *> *customDest);
	
	long		ReadCompressedBitmap(BBitmap **bmap);

	/////////////	
	void 		ReadCatalog(ArchiveFolderItem *parent, long &offset);
	
	long		BuildGroupList(FileEntry **entryList,
					FileEntry *entryMax,
					bool topFolder,
					uint32 &instFoldGroups);
					
	/////////////
	bool			SetFile();
	void			ClearFile();

	uint32			arcFlags;
	
	// use when seeking to locations specified by the catalog 
	off_t			entriesOffsetAdjust;
	off_t			attribOffset;
	off_t			catalogOffset;
	off_t			instDataSize;
	
	// number of bytes in
	off_t			grandTotal;
	
	long 			byteUpdateSize;
	long 			bucket;
	// buffers for each file
	char 			inBuf[INBUFSIZ];
	char 			outBuf[OUTBUFSIZ];
	bool			doCancel;
	BMessenger		statusMessenger;
	
	BEntry				*pEntry;
	
private:	
friend class InstallLooper;	
friend class ArcCatalog;
	BFile				*fArcFile;
	ArcCatalog			*fCatalog;
	
	ArchiveFileItem		*curFile;
	ArchiveFolderItem	*curFolder;
	
	SPackData			*fPackData;
};

//#define HEADER_ATTRIB_SIZE (2*sizeof(AttribHeader) + sizeof(long))
//#define HEADER_SIZE (sizeof(fileHeader) + HEADER_ATTRIB_SIZE)


enum {
	E_Z_ERRNO,			// zlib returned errno (meaning file system error)
	E_Z_STREAM_ERROR,	// zlib stream error
	E_Z_DATA_ERROR,		// zlib data error
	E_Z_MEM_ERROR,		// zlib memory error
	E_Z_BUF_ERROR,		// zlib buffer error
	E_Z_VERSION_ERROR	// zlib version error
};


//#define MAGIC_NO		'alb_'

enum {
	CD_INSTALL =		0x0001,
	AUTO_COMPRESS =		0x0002,
	HAS_RECORD_REFS =   0x0004
};

typedef struct arcHeader {
	uint32	vers;
	int64	fileSize;
} arcHeader;

typedef struct {
	ulong 	magic;
	ulong	vers;
	ushort 	flags;
	long	entriesOffset;
	long	catalogOffset;
	long	attribOffset;
} fileHeader;

typedef struct {
	long	numberOfEntries;
	// gnerally we want to calculate groups from the contained items, but the folder
	// also has its own specific group (important if the folder is empty)
	ulong	groups;
	long	destination;
	short	replacement;
	char	customDest;
	long	version;
	long	creationTime;
	long	modTime;
	long	xLoc, yLoc;
	short	folderNameLength;
} folderEntry;
// folderNameString

typedef struct {
	ulong	mode;
	long	creationTime;
	long	modTime;
	long	compressedSize;
	long	uncompressedSize;
	long	seekOffset;
	long	adler32;
	ulong	groups;
	long	destination;
	short	replacement;
	char	customDest;
	long	version;
	
	short	typeNameLength;
	short	fileNameLength;	
	// fileNameString
} fileEntry;

typedef struct {
	// size of existing file
	long	oldSize;
	ulong	oldType;
	ulong	oldCreator;

	ulong	mode;
	long	creationTime;
	long	modTime;
	// size of compressed patch data
	long	compressedSize;
	// size of the applied patch
	long	uncompressedSize;
	long	seekOffset;
	long	adler32;
	ulong	groups;

	// destination -- found via search criteria ??
	long destination;
	char customDest;
	// replacement -- does not apply	
	
	long	version;
	// name for new file
	short	typeNameLength;
	short	fileNameLength;
} patchEntry;

typedef struct {
	// size of compressed patch data
	long	compressedSize;
	// size of the applied patch
	long	uncompressedSize;
	long	seekOffset;
	long	adler32;
	ulong	groups;

	// destination -- found via search criteria ??
	long destination;
	char customDest;
	short	fileNameLength;
} scriptEntry;

					/***********************/
					/*	  Error messages   */
					/***********************/
					

/// PackResFile--Attrib.cpp
// this file needs way more error checking
// version checking, bounds checking
#define errCMPSTRLEN	"Error reading compressed string. The data should be less than 32K."
#define errCMPSTRSIZE	"Error reading compressed text. The data size is less than expected."


// PackResFile.cpp
#define errREADPKHEAD	"File system error reading the package header."
//
#define errSZPKHEAD		"Error, could not read the entire package header."
#define errPKMAGIC		"Warning this is probably not a valid package file, magic number not found."
#define errPKTOOOLD		"Sorry this package file format is too old for this version of the software."
#define errPKTONEW		"Sorry this package file format is too new for this version of the software. Please update to use this package file."
#define errOPENARC		"File system error opening archive for decompression."
//
#define errINITINFLATE	"Error initializing decompression engine."
#define errNUMENTRIES	"Error, the package contains more entries than expected."
#define errCATFLAG		"Error, read unexpected catalog flag %d."
#define errPKTOOSMALL	"The package file size is smaller than expected."
#define errPKLARGER		"The package file size was larger than expected.\n\nContinue at your own risk!"


#endif

