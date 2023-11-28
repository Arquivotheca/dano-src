#ifndef _FILEPATCHER_H
#define _FILEPATCHER_H

// FilePatcher.h

#define cBufSiz				512L

#define cMinMatchLen         6
#define cMinEqualRunLen      (cMinMatchLen+4) /* Must be >= cMinMatchLen */

/*
* Tree node: has 3 pointers: a pointer to lesser, and greater or equal nodes
* The third pointer is a linked list pointer: the root node of the tree is also
* the head of the linked list. The linked list is not built in a special order:
* nodes are added to it in order of occurence. The tree is a binary tree.
*/
// linked list pointer is unused!

typedef struct STreeNode
{
	long       filPos;
	uchar      bytes[cMinMatchLen];
	struct STreeNode *pGE;  /* Greater or equal */
	struct STreeNode *pLT;  /* Less than */
} TTreeNode;

/*
* Match block structure: for each match found between the two files, we
* encode the positions in the two files and the length of the match.
*/

typedef struct SMatchBlock
{
	long         len;
	long         orgFilPos;
	long         derivedFilPos;
	long         distance; /* = orgFilPos - derivedFilPos */
	struct SMatchBlock *pNxt;
} TMatchBlock;



typedef struct
{
	unsigned long   sum;
	double sumSquares;
	double mean;
	double stdDev;
	/*
	* Last file position where a byte was encountered, for all bytes. Initialized
	* to -1 ('before' the first file position).
	*/
	long lastPos;
	/*
	* Count of occurences
	*/
	long cnt;
} TByteAttribs;

#define cMinMeanChunkLen	20.0
#define cMaxMeanChunkLen	80.0
#define cBigChunkLen		500.0
///////////////////////////////////////////////////////////////


class FilePatcher
{
public:
	FilePatcher(entry_ref &oldFile, entry_ref &newFile);
	~FilePatcher();

	entry_ref	MakePatch();
	long		Error();
			
private:
	entry_ref	outputPatchRef;
	BMessenger 	statMess;
	bool		doCancel;

	long gMinVal;
	long gMaxVal;
	int  gCurPos;
	long gLastVal;
	long gCurVal;
	
	FILE *pOrgFil;
	FILE *pDerivedFil;
	FILE *pDiffFil;

	///////***** reusable list of alloced	
	TTreeNode *gLstFreeTreeNode;
	TMatchBlock *gLstFreeMatchBlock;
	
	void	FreeAllTree(TTreeNode *node);
	void	FreeTree(TTreeNode *node);
	void	FreeTreeList();
	void	FreeMatchList(TMatchBlock *item);
	
	void FatalError(char *errMsg);
	long FileSize(FILE *pFil);
	long ReadLongNBytes(FILE * pFil,int n);
	void InitProgressBar(long minVal, long maxVal, char * message);
	void AdjustProgressBar(long curVal);
	void CloseProgressBar();
	void WriteLongNBytes(long x, FILE * pFil,int n);
	void ScanFile(	FILE * pFil,
				  	TByteAttribs *byteTable,
				  	off_t pSize,
					ulong *csum);
	int  FindDelimiter(FILE * pFil, ulong *crc,
		  				double minMeanChunkLen,
		  				double maxMeanChunkLen);

	///////******** allocates memory
	// called in BuildTree
	TTreeNode *NewTreeNode();
	// called when a node is created that doesn't need to be added
	// since it is already in the tree
	void FreeTreeNode(TTreeNode *pTreeNode);

	int CmpNode(TTreeNode *pNode1,
		            FILE *pFil1,
		            TTreeNode *pNode2,
		            FILE *pFil2,
		            int   delim,
		            long *pEqualLen);
	void FindBestMatch(TTreeNode *pOrgTreeRoot,
		   			TTreeNode **ppOrgTreeNode,
		   			FILE *pOrgFil,
		   			TTreeNode *pDerivedTreeNode,
		   			FILE *pDerivedFil,
                   	int delim,
		   			long *pMatchLen);
	void AddTreeNode(TTreeNode **ppTreeRoot,
					FILE *pFil,
					int delim,
					TTreeNode *pNewNode);
	TTreeNode *BuildTree(FILE * pFil, int delim);
	void ExtendMatch(long *pOrgFilPos,
					FILE *pOrgFil,
		 			long *pDerivedFilPos, FILE * pDerivedFil,
					long *pMatchLen);
					
	//**** allocs mem
    TMatchBlock	*NewMatchBlock();
    void FreeMatchBlock(TMatchBlock *pMatchBlock);
    void AddMatch(TTreeNode *pOrgTreeNode, FILE *pOrgFil,
              TTreeNode *pDerivedTreeNode, FILE *pDerivedFil,
              long matchLen,
              TMatchBlock **ppMatchLst);
    void ShrinkMatchList(TMatchBlock **ppMatchLst);
    
    TMatchBlock *MatchFiles(TTreeNode *pOrgTreeRoot,
			FILE *pOrgFil,
			FILE *pDerivedFil,
			int		delim,
			ulong	*pDerivedSum);
	void DumpDiff(TMatchBlock *pMatchLst,
				FILE *pDerivedFil, FILE *pDiffFil);
	
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

};



// change this
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

#define cMaxStrLen       255

typedef char          	TStr[cMaxStrLen+1];


#include "GStatusWind.h"

class PatchStatusWindow : public GStatusWindow
{
public:
	PatchStatusWindow(const char *title,
					bool *_cancelBool);
					
	virtual void	DoCancel(bool stat = TRUE);
private:
	bool *cancelBool;
};

#endif
