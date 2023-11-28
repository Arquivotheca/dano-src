// ArchivePatchItem.h

#include "FArchiveItem.h"

class ArchivePatchItem : public ArchiveFileItem
{
public:
			ArchivePatchItem(BEntry &oldF, BEntry &newF);
			ArchivePatchItem();
virtual 	~ArchivePatchItem();
	
	// compress the already created patch file
	virtual long	CompressItem(PackArc *arcFile);
	virtual long	ExtractItem(PackArc *arcFile, BDirectory *dest);
	virtual long	TestPatch(PackArc *arcFile, entry_ref &oldFileRef,
							entry_ref &dirRef, const char *newName);
	
	entry_ref		tempFileRef;
	
	off_t	oldFileSize;
	
	time_t	oldCreationTime;
	time_t	oldModTime;
	
	bool	canDelete;
private:
	long			ApplyPatch(char *orgFilNam,
								  char *derivedFilNam,
								  char *diffFilNam,
								  long remainBytes);
	
	void			AdjustProgressBar(long curVal);
	void			CopyFileChars(long count, FILE *inFil,
								FILE *outFil, ulong *pSum);
	long			ReadLongNBytes(FILE *pFil,int n);
	long			FileSize(FILE *pFil);

	float				progressAdjust;
	PackArc				*theArchiveFile;
	char				*copyBuf;
	long				gLastVal;
	
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
