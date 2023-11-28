#ifndef _IARCHIVEPATCHITEM_H_
#define _IARCHIVEPATCHITEM_H_

#include <stdio.h>
class BDirectory;

// ArchivePatchItem.h

#include "IArchiveItem.h"

class ArchivePatchItem : public ArchiveFileItem
{
public:
	ArchivePatchItem();
	ArchivePatchItem(const ArchivePatchItem &item);
	
	// compress the already created patch file
	virtual long	ExtractItem(InstallPack *arcFile,
									BDirectory *parDir,
									DestManager *dests,
									ulong inGrps,
									bool skip);
	
	off_t			oldFileSize;
	
	long			oldCreationTime;
	long			oldModTime;
	
	entry_ref		fileRef;
	
/**
	static void			PathForFile(BFile *bfil,
								char *buf, long bufLen);
	static char			*PathForDir(BDirectory *dir,
								char *buf, char *bufMax);
**/
private:
	long			ApplyPatch(const char *orgFilNam,
								  const char *derivedFilNam,
								  const char *diffFilNam,
								  long remainBytes);
	
	void			AdjustProgressBar(long curVal);
	void			CopyFileChars(long count, FILE *inFil,
								FILE *outFil, ulong *pSum);
	long			ReadLongNBytes(FILE *pFil,int n);
	long			FileSize(FILE *pFil);

	float			progressAdjust;
	InstallPack		*theArchiveFile;
	char			*copyBuf;
	long			gLastVal;
	
	/*
	* Tag values. A tag value is encoded in the 4 lowest bits of a tag byte.
	* The 4 high bits of the tag byte are used for encoding a value 0-15.
	*/
	enum {	cTagSmallDiff =          0,
			cTagMediumDiff =         1,
			cTagLargeDiff =          2,
			cTagSmallNearCopy =      3,
			cTagMediumNearCopy =     4,
			cTagLargeNearCopy =      5,
			cTagSmallDistantCopy =   6,
			cTagMediumDistantCopy =  7,
			cTagLargeDistantCopy =   8,
			cTagSmallFarCopy =       9,
			cTagMediumFarCopy =   0x0A,
			cTagLargeFarCopy =    0x0B,
			/* Tags 0x0C,0x0D,0x0E unused. */
			cTagEOF =             0x0F };
	enum {
			COPY_BUF_SIZ =			4096};
};


#define PATCH_MAGIC_NUM		'SCpt'

/*
* if cDropEOL is TRUE, cr and lf are not allowed to function as delimiters
*/
#define cDropEOL           FALSE

/*
* Maximum values encodable by different tags.
* 4-bit value (0-15)       is used to encode a value             1 - cSmallSize
* 12-bit value (0-4095)    is used to encode a value  cSmallSize+1 -
cMediumSize
* 20-bit value (0-1048575) is used to encode a value cMediumSize+1 - cLargeSize
*/

#define cSmallSize            16L
#define cMediumSize           (4096L+cSmallSize)
#define cLargeSize            (1048576L+cMediumSize)

/*
* Maximum file positions encodable in 2 or 3 bytes
*/

#define cNearDistance         0xFFFFL
#define cDistantDistance      0xFFFFFFL

// #define cMaxStrLen       255

// typedef char          	TStr[cMaxStrLen+1];


/***************************
	Error Messages
***************************/


#define errPATCHTMP		"Could not create temporary patch file."
#define errPATCHDCMP	"Encountered error when decompressing patch for \"%s\"."
#define errPATCHCKSUM	"Warning, checksums do not not match. The patch for \"%s\" may be corrupt."
#define errPATCHBADSIZE	"Warning, incorrect size for patch \"%s\"."
#define errPATCHRENAME	"Error renaming newly patched file."
//
#define errNOCOPYBUF	"Could not allocate copy buffer."
#define errPATCHOPEN	"Error opening patch data."
#define errPATCHHEAD	"Patch header is invalid. The patch cannot be applied."
//
#define errPATCHOPENEXISTING	"Cannot open existing file. The patch cannot be applied."
#define errPATCHOPENNEW	"Cannot open new file. The patch cannot be applied."
#define errPATCHTAG		"Error, unknown tag in patch data."
#define errPATCHOSZ		"Warning, original file size does not match. Original file is not the file from which the patch was created. Use patched file with caution."
#define errPATCHNSZ		"Patched file size does not match the original. Patch could not be completed."
//
#define errPATCHOCHK		"Orginal file checksum does not match. Use patched file with caution."
#define errPATCHNCHK		"Patched file checksum does not match. Patch could not be completed."


#endif
