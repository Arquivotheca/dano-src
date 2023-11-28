
#define BE_OS // for zlib
#include "zlib.h"
#include "RList.h"

#ifndef _PACKARC_H
#define _PACKARC_H

/* */
/***
typedef struct {
	int result;
	int zerr;
} CStatus;
***/

class DestItem;
class DestList;
class FileLooper;
class ArchiveItem;
class ArchiveFileItem;
class ArchiveFolderItem;
class ArchivePatchItem;
class ArchiveScriptItem;
class ArchiveLinkItem;

class SPackData;
class GroupList;
class AttribData;
class PackWindow;
struct arcHeader;

class PackArc
{
public :	
					PackArc();
					PackArc(entry_ref *ref, uint16 version);
	
	virtual 		~PackArc();
	
	status_t		InitCheck();
	
	status_t		ReadArcHeader(BFile *f, arcHeader &head);
	status_t		WriteArcHeader(BFile *f);
	
	status_t		VerifyMagic(BFile *f);
	status_t		WriteMagic(BFile *f);
	
		void		SetArcRef(entry_ref *ref) {
						fArcRef = *ref;
					};
	inline			const entry_ref *ArcRef() const;
	
	void			SetupLooper(PackWindow *doc);
	status_t		SetupNew(uint32 fileFlags);

	status_t		EndCompression();
		
		void 		ReadCatalog(ArchiveFolderItem *parent);
		void 		ReadCatalogFolder(ArchiveFolderItem *parent,
									ArchiveFolderItem *top = NULL);
		void		ReadCatalogFile(ArchiveFileItem *item);
		void		ReadCatalogPatch(ArchivePatchItem *item);
		void		ReadCatalogScript(ArchiveScriptItem *item);
		void		ReadCatalogLink(ArchiveLinkItem *item);
		
	status_t		AddNode(BNode *newItem, off_t	*outputSize, node_flavor flavor);
	status_t		ResetStream();
	
	status_t		BeginCompression();		//???
	status_t		BeginDecompression();	//???
	status_t		EndDecompression();		//???
	
	status_t 		RemoveItems(RList<ArchiveItem *> *deleteList,
							RList<ArchiveItem *> *cleanList);
	status_t		SetFile(uint32 mode);
	void			ClearFile();
	

	status_t		WriteFinalData(ArchiveFolderItem *fold,
									bool isCD,
									AttribData *att);

	status_t 		WriteCatalogFolder(ArchiveFolderItem *fold,long &bytes, bool isCD = FALSE);
	status_t		WriteArcItemCommon(ArchiveItem *arcItem);
	status_t		BuildInstaller(entry_ref &dirRef, const char *name, bool sea);
	status_t		ExtractFile(BNode *dest, long bytes, off_t atOffset);	
	
/*** old */

	// this will need to be redone
	
	////////////// NEW STUFF ///////////
	long		CopyInstallerAttributes(BFile &instFil);
	long		WriteLastAttrib();
	
	long		WriteMasterGroupList(GroupList *groupList);
	long		WriteViewGroupList(GroupList *groupList);
	long		WriteDefaultDestList(RList<DestItem *> *def);
	long		WriteCustomDestList(DestList *customDestList);
	
	long		WriteBitmap(BBitmap *bmap, int32 id);
	
	long		WriteDoLicense(bool doLicense, BPath &lfile);
	long		WriteLicenseText(bool doLicense, BPath &lfile);
	
	long		NewWriteAttributes(AttribData *att,long);
	long		WriteAttribHeader(ushort code, off_t offset);
	
	long		ReadMasterGroupList(GroupList *groupList);
	long		ReadViewGroupList(GroupList *groupList);
	long		ReadDefaultDestList(RList<DestItem *> *defaultDestList);
	long		ReadCustomDestList(DestList *customDest);
	
	long		ReadCompressedBitmap(ushort attrib,BBitmap **bmap);
	long		ReadString(const char **str);
	
	long		ReadDoLicense(bool *doLicense, BPath *lfile);
	
	long		ReadAttribHeader(ushort &code, long &dataSize);
	long		NewReadAttributes(	AttribData *att );
	////////////////////////////////////

	long			WriteInstallerData(long bytes,BFile *file);

	void		SetVersion(uint16 version);
	uint16		Version();
			
	off_t		catalogOffset;
	off_t		attribOffset;
	off_t		entriesOffset;
	off_t		instDataSize;

	uint32		arcFlags;
	
	// this looper receives messages to compress/remove files
	FileLooper *fileLooper;
	
	double		startTime;
	double		readTime;
	double		writeTime;
	// number of bytes in
	long		grandTotal;

	long		byteUpdateSize;
	long		bucket;
	long		backBytes;
	
	BMessenger	updateMessenger;
	BMessenger	statusMessenger;
	
private:
	friend class FileLooper;
	// buffers for each file
	enum {
		INBUFSIZ = 8192
	};
	char inBuf[INBUFSIZ];
	// char outBuf[OUTBUFSIZ];
	
	entry_ref		fArcRef;
	BFile			*fArcFile;
	status_t		fErr;
	SPackData		*fPackData;
	uint16			fVersion;
};

const entry_ref *PackArc::ArcRef() const {
	return &fArcRef;		
};

inline void PackArc::SetVersion(uint16 version)
{
	fVersion = version;
}

inline uint16 PackArc::Version()
{
	return fVersion;
}

extern const uint16 CURRENT_VERSION;
extern const uint16 COMPAT_VERSION;


// for version 0.9d1
enum {
	MASTER_GROUP_TYPE =		'Mgrp',
	VIEW_GROUP_TYPE =		'Vgrp',
	DEFAULT_DEST_TYPE =		'Ddst',
	CUST_DEST_TYPE =		'Cdst',
	DESCRIPTION_TYPE	=	'Desc',
	INSTALLFOLDER_TYPE =	'IFol',
	LICENSE_TYPE =			'Lice'
};

// attributes found after the header
enum {
	A_OLDEST_APP_VERSION =	0x0001,
	A_PACKAGE_DATASIZE =	0x0002
};

// #define HEADER_ATTRIB_SIZE (3*sizeof(AttribHeader) + 2*sizeof(long))
// #define HEADER_SIZE (sizeof(fileHeader) + HEADER_ATTRIB_SIZE)

// header structure for each attribute
// size indicates size of actual data
typedef struct
{
	ushort 	code;
	long	size;
} AttribHeader;


enum {
	FILE_FLAG = 0x0001,
	FOLDER_FLAG = 0x0002,
	PATCH_FLAG = 0x0003
};
	

// #define MAGIC_NO		'alb_'

enum {
	CD_INSTALL =		0x00000001,
	AUTO_COMPRESS =		0x00000002,
	HAS_RECORD_REFS =   0x00000004
};

typedef struct arcHeader {
	uint16	vers;
	uint64  fileSize;
} arcHeader;

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
	// icon
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
#endif
