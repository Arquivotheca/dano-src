#include <Be.h>
#include <stdlib.h>
/***
Adapted from "A Cross Platform Binary Diff Algorithm" from Dr. Dobbs

Could be improved --

	use GDIFF output format
	use hashing instead of binary tree?
		-- use profiling to see how often the search is worst case
		-- check ram usage of tree

****/

#include "FilePatcher.h"

#include "Util.h"
/**
// not needed with precompiled headers
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
**/
#include "zlib.h"

/* ***********************************************************************
*
* Error message display + terminate program
*/

// fix this up
void FilePatcher::FatalError(char *errMsg)
{
  doError(errMsg);
  exit(-1);
}

/* ***********************************************************************
*
* Calculate size of an open file
*/

// 64 bit?
long FilePatcher::FileSize(FILE *pFil)
{
  long filSiz;

  long curPos;

  /*
  * Save file position
  */
  curPos = ftell(pFil);
  /*
  * Go to end of file and determine file size.
  */
  fseek(pFil,0L,SEEK_END);
  filSiz = ftell(pFil);
  /*
  * Restore previous position
  */
  fseek(pFil,curPos,SEEK_SET);

  return(filSiz);

}

/* ***********************************************************************
*
* Read n bytes and return in a long (n=1,2,3,4)
*/

long FilePatcher::ReadLongNBytes(FILE * pFil,int n)
{
	long x;
	int c;

	x = 0;
	while (n > 0)
	{
		c = getc(pFil);
		if (c == EOF)
		{
			n = 0;
		}
		else
		{
			x = (x << 8) | c;
		}
		n--;
	}
	return(x);
}


/* ***********************************************************************
*
* Write n lowest bytes of a long
*/

void FilePatcher::WriteLongNBytes(long x, FILE * pFil,int n)
{

	while (n > 4)
	{
		putc(0,pFil);
		n--;
	}

	if (n == 4)
	{
		putc(x >> 24,pFil);
		n--;
	}
	if (n == 3)
	{
		putc((x >> 16) & 0xFF,pFil);
		n--;
	}

	if (n == 2)
	{
		putc((x >> 8) & 0xFF,pFil);
		n--;
	}

	if (n == 1)
	{
		putc(x & 0xFF,pFil);
	}
}

/**
void FilePatcher::WriteLongLong(long long x, FILE *pFil)
{
	
}
**/

/* ***********************************************************************
*
* Scans file and for each byte 0-255, calculate mean distance and standard
* deviation of the distances between occurences of these bytes:
*
* E.g. look at byte b in the file:
*
* xxxxxxbxxxxxxxxxbxxxxxbxxxxxxxxxxxbxxxxxxbxxxxxbxxxx
* < d1  ><   d2   >< d3 ><     d4   >< d5  >< d6 >
* <  7  ><   10   ><  6 ><    12    ><  7  ><  6 >
* The mean and std.dev. are calculated on distances d1,d2,... for byte b.
*/

void FilePatcher::ScanFile(FILE * pFil,
							TByteAttribs *byteTable,
							off_t pSize,
							ulong *csum)
{
	int				byte;
	off_t			curPos;
	off_t			count;
	unsigned long	dist;
	TByteAttribs	*pByteAttribs;
	double			dDist;
	ulong			crc;
	const long COPY_BUF_SIZ = 8192;
	uchar			*copyBuf;
	/*
	* Initialize tables
	*/
	pByteAttribs = byteTable;
	for (byte = 0; byte < 256; byte++)
	{
		pByteAttribs->lastPos = -1L;
		pByteAttribs->sum = 0L;
		pByteAttribs->sumSquares = 0.0;
		pByteAttribs->mean = 0.0;
		pByteAttribs->stdDev = 0.0;
		pByteAttribs->cnt = 0L;
		pByteAttribs++;
	}

	/*
	* Scan through file
	*/
	/* read 8k buffers instead of getc! */
	copyBuf = new uchar[COPY_BUF_SIZ];
	
	fseek(pFil,0L,SEEK_SET);

	crc = crc32(0L, Z_NULL, 0);	
	curPos = 0;
	count = pSize;
	while (count) {
		long amount = min_c(count,COPY_BUF_SIZ);
		long res = fread(copyBuf,sizeof(uchar),amount,pFil);		
		if (res != amount) {
			// doError("premature eof or error");
			break;
		}
		// compute checksum for buffer
		crc = crc32(crc, copyBuf, amount);
		
		uchar *byte = copyBuf;
		uchar *max = copyBuf + amount;
		while (byte < max) {
			pByteAttribs = &byteTable[*byte];
			/* Calculate distance from last occurrence of this byte */
			dDist = dist = curPos - pByteAttribs->lastPos;
			/* Remember this byte's position */
			pByteAttribs->lastPos = curPos;
			pByteAttribs->sum += dist;
			pByteAttribs->sumSquares += dDist * dDist;
			
			/* cnt contains the number of occurrences */
			pByteAttribs->cnt++;
			
			curPos++;
			byte++;
		}

		count -= amount;
	}
	delete copyBuf;
	*csum = crc;	

	/*
	* Calculate mean and standard deviation for all bytes.
	*/
	pByteAttribs = byteTable;
	for (byte = 0; byte < 256; byte++)
	{
		/*
		* Make byte 'occur' just after EOF
		*/
		dDist = dist = curPos - pByteAttribs->lastPos;
		pByteAttribs->sum += dist;
		pByteAttribs->sumSquares += dDist*dDist;
		pByteAttribs->cnt++;

		/*
		* Calculate mean. Bytes that did not occur get mean equal to file size
		*/
		pByteAttribs->mean =
			(double) pByteAttribs->sum / (double) pByteAttribs->cnt;

		/*
		* Calculate standard deviation. We could also use the variance
		* but I like the std. dev. more.
		*/
		pByteAttribs->stdDev =
			sqrt(pByteAttribs->sumSquares / (double) pByteAttribs->cnt
				- pByteAttribs->mean*pByteAttribs->mean);

		pByteAttribs++;
	}
}

/* ***********************************************************************
*
* Analyze open file and determine a suitable delimiter for chopping the file
* into chunks. This routine changes the current file position.
*/

int FilePatcher::FindDelimiter(FILE * pFil, ulong *crc,
					double minMeanChunkLen,
					double maxMeanChunkLen)
 {
  int			byte;
  int			bestByte;
  TByteAttribs	byteTable[256];
  off_t			filSiz;

  filSiz = FileSize(pFil);
  // InitProgressBar(0L,filSiz,"Pass 1 of 3:");

  ScanFile(pFil, byteTable, filSiz, crc);

  /*
  * Determine best byte
  */
  bestByte = -1;
  while (bestByte == -1
         && maxMeanChunkLen < cBigChunkLen
         && maxMeanChunkLen < filSiz)
   {
    TByteAttribs *pByteAttribs;
    TByteAttribs *pBestByteAttribs;

    pByteAttribs = byteTable;
    pBestByteAttribs = NULL;
    for (byte = 0; byte < 256; byte++)
     {
#if cDropEOL
      if (byte != '\015' && byte != '\012')
#endif
       {
        /*
        * Check if chunk length is between minMeanLen and maxMeanLen.
        */
        if (pByteAttribs->mean >= minMeanChunkLen &&
            pByteAttribs->mean <= maxMeanChunkLen)
         {
	  if (bestByte == -1)
	   {
	    bestByte = byte;
	    pBestByteAttribs = pByteAttribs;
	   }
	  else
	   {
	    /*
	    * Compare stddev: if it is lower, the byte is better
	    */
	    if (pBestByteAttribs->stdDev > pByteAttribs->stdDev)
	     {
	      bestByte = byte;
	      pBestByteAttribs = pByteAttribs;
	     }
	   }
         }
       }
      pByteAttribs++;
     }
    /*
    * Increase allowable chunk length for the case no acceptable delimiter was
    * found: we will loop then
    */
    maxMeanChunkLen += 50;
   }

  // CloseProgressBar();

  return(bestByte);
}

/* ***********************************************************************
*
* NewTreeNode(): create a new tree node or reuse one of the free list
*/

TTreeNode *FilePatcher::NewTreeNode()
{
	TTreeNode *pTreeNode;

	if (gLstFreeTreeNode == NULL)
	{
		pTreeNode = new TTreeNode();
		if (pTreeNode == NULL)
		{
			FatalError("Out of memory");
		}
	}
	else
	{
		pTreeNode = gLstFreeTreeNode;
		gLstFreeTreeNode = gLstFreeTreeNode->pGE;
	}
	pTreeNode->filPos = 0L;
	pTreeNode->pGE = NULL;
	pTreeNode->pLT = NULL;

	return(pTreeNode);
}

/* ***********************************************************************
*
* Release a tree node: put on the free list
*/

void FilePatcher::FreeTreeNode(TTreeNode *pTreeNode)
{
	pTreeNode->pGE = gLstFreeTreeNode;
	gLstFreeTreeNode = pTreeNode;
}

/* ***********************************************************************
*
* Compare two tree nodes; return -1, 0, 1 if <, = , >.
* Every tree node has cMinMatchLen characters of the file
* buffered within the node: first compare these. If all these characters
* are equal, read characters from the associated files and compare these.
* If the nodes are different, set equalLen to the number of equal characters
* encountered.
* delim is the chopping character.
*/

int FilePatcher::CmpNode(TTreeNode *pNode1,
            FILE *      pFil1,
            TTreeNode *pNode2,
            FILE *      pFil2,
            int        delim,
            long      *pEqualLen)
 {
  long   pos1;
  uchar  buf1[cBufSiz];
  uchar * p1;
  long   pos2;
  uchar  buf2[cBufSiz];
  uchar * p2;
  int    result;
  long   l;

  pos1 = pNode1->filPos;
  pos2 = pNode2->filPos;
  result = -2; /* -2 means: not yet defined */
  *pEqualLen = 0;

  /*
  * First compare the cMinMatchLen buffered bytes from both nodes
  */
  p1 = pNode1->bytes;
  p2 = pNode2->bytes;
  l = 0;
  while (l < cMinMatchLen && *p1 == *p2 && *p1 != delim /* && *p2 != delim */)
   {
    p1++;
    p2++;
    l++;
    (*pEqualLen)++;
   }

  if (l == cMinMatchLen)
   {
    /*
    * If no difference was found, we will have to compare both files from
    * cMinMatchLen bytes after pos1 and pos2.
    */
    pos1 += cMinMatchLen;
    pos2 += cMinMatchLen;
   }
  else if (*p1 == delim && *p2 != delim)
   {
    result = -1; /* node 1 < node 2 because node 1 is shorter */
   }
  else if (*p2 == delim && *p1 != delim)
   {
    result = 1; /* node 2 < node 1 because node 2 is shorter */
   }
  else if (*p1 == *p2 && *p1 == delim)
   {
    result = 0; /* node 1 == node 2: both end in a delimiter at the same
time */
   }
  else if (*p1 < *p2)
   {
    result = -1; /* node 1 < node 2 because a different character found */
   }
  else if (*p2 < *p1)
   {
    result = 1; /* node 2 < node 1 because a different character found */
   }

  /*
  * If result is -2, no difference was found in the buffered bytes. Start
  * reading bytes from the files in chuncks of cBufSiz bytes.
  */
  if (result == -2)
   {
    do
     {
      /*
      * Read a buffer from file 1. Pad file with delimiter after eof.
      */
      fseek(pFil1,pos1,SEEK_SET);
      l = fread(buf1,1L,cBufSiz,pFil1);
      if (l < cBufSiz) buf1[l] = delim;

      /*
      * Read a buffer from file 2. Pad file with delimiter after eof.
      */
      fseek(pFil2,pos2,SEEK_SET);
      l = fread(buf2,1L,cBufSiz,pFil2);
      if (l < cBufSiz) buf2[l] = delim;

      /*
      * Compare buffers
      */
      p1 = buf1;
      p2 = buf2;
      l = 0;
      while (l < cBufSiz && *p1 == *p2 && *p1 != delim /* && *p2 != delim */)
       {
	p1++;
        p2++;
	l++;
        (*pEqualLen)++;
       }

      /*
      * If no difference was found: set file positions to read new buffers
      */
      if (l == cBufSiz)
       {
	pos1 += cBufSiz;
        pos2 += cBufSiz;
       }
      else if (*p1 == delim && *p2 != delim)
       {
        result = -1; /* node 1 < node 2  */
       }
      else if (*p2 == delim && *p1 != delim)
       {
	result = 1; /* node 2 < node 1 */
       }
      else if (*p1 == *p2 && *p1 == delim)
       {
        result = 0; /* node 1 == node 2 */
       }
      else if (*p1 < *p2)
       {
        result = -1;
       }
      else if (*p2 < *p1)
       {
        result = 1;
       }
     }
    while (result == -2);

   }

  return(result);

 }

/* ***********************************************************************
*
* From a tree with root node pOrgTreeRoot, find the node pOrgTreeNode that best
* matches pDerivedTreeNode. Also find length of match.
*/

void FilePatcher::FindBestMatch(TTreeNode *pOrgTreeRoot,
		   			TTreeNode **ppOrgTreeNode,
		   			FILE *pOrgFil,
		   			TTreeNode *pDerivedTreeNode,
		   			FILE *pDerivedFil,
                   	int delim,
		   			long *pMatchLen)
{
  int        direction;
  TTreeNode *pNode;
  long       equalLen;

  /*
  * Find best location from tree
  */
  *ppOrgTreeNode = NULL;
  pNode = pOrgTreeRoot;
  *pMatchLen = 0L;

  /*
  * Descend tree and remember node with longest match
  */
  while (pNode != NULL)
   {

    direction =
CmpNode(pDerivedTreeNode,pDerivedFil,pNode,pOrgFil,delim,&equalLen);

    /*
    * Remember match if length is greater than previous best
    */
    if (equalLen > *pMatchLen)
     {
      *pMatchLen = equalLen;
      *ppOrgTreeNode = pNode;
     }

    if (direction == -1)
     {
      pNode = pNode->pLT;
     }
    else if (direction == 1)
     {
      pNode = pNode->pGE;
     }
    else /* Node is equal: stop search */
     {
      pNode = NULL;
     }
   }

 }

/* ***********************************************************************
*
* Add new node to index tree and linked list.
*
*/

void FilePatcher::AddTreeNode(TTreeNode **ppTreeRoot, FILE *pFil, int delim, TTreeNode *pNewNode)
{
  TTreeNode *pNode;
  TTreeNode *pPrvNode;
  int        cmp;
  long       equalLen;

  /*
  * Find location in tree
  */
	pNode = *ppTreeRoot;
	pPrvNode = NULL;
	while (pNode != NULL)
	{
		pPrvNode = pNode;
		cmp = CmpNode(pNewNode,pFil,pNode,pFil,delim,&equalLen);
		if (cmp == -1)
		{
			pNode = pNode->pLT;
		}
		else if (cmp == 1)
		{
			pNode = pNode->pGE;
		}
		else /* There is an equal node: stop looking */
		{
			pNode = NULL;
		}
	}

	if (pPrvNode == NULL) /* if the tree is empty */
	{
		*ppTreeRoot = pNewNode;
	}
	else
	{
		if (cmp == -1)
		{
			pPrvNode->pLT = pNewNode;
		}
		else if (cmp == 1)
		{
			pPrvNode->pGE = pNewNode;
		}
		else
		{
			FreeTreeNode(pNewNode);
		}
	}
}

/* ***********************************************************************
*
* Build index tree: divide file in chunks by using the delimiter. We also
* create chunks of strings that contain cMinMatchLen or more equal bytes.
*
*/

TTreeNode *FilePatcher::BuildTree(FILE * pFil, int delim)
 {
  TTreeNode *pTreeRoot;
  TTreeNode *pNewNode;
  TTreeNode *pEqualRunNode;
  int        byte;
  int        prevByte;
  long       equalByteCnt;
  long       curPos;
  long       prevPos;
  long       len;
  uchar *     pByte;

  // InitProgressBar(0L,FileSize(pFil),"Pass 2 of 3:");

  pTreeRoot = NULL;

  curPos = 0;
  prevPos = 0;
  do
   {
    if (curPos - prevPos > 0x3FF)
     {
      AdjustProgressBar(curPos);
      prevPos = curPos;
      if (doCancel)
      	return pTreeRoot;
     }

    /*
    * Restore file position (because it is destroyed by CmpNode)
    */
    fseek(pFil,curPos,SEEK_SET);

    /*
    * Prepare new node
    */
    pNewNode = NewTreeNode();

    len = 0;
    pNewNode->filPos = curPos;
    pByte = pNewNode->bytes;
    byte = -1;
    equalByteCnt = 0;
    do
     {
      prevByte = byte;
      if ((byte = getc(pFil)) != EOF)
       {
        if (byte == prevByte)
         {
          equalByteCnt++;
         }
        else
         {
          /*
          * No need to check for cMinEqualRunLen or more equal bytes here;
          * if they were here, they will be added to the tree anyhow. This
          * loop only executes at most cMinMatchLen times.
          */
          equalByteCnt = 1;
         }
        //(*pOrgSum) += byte;
        *(pByte++) = byte;
       }
      else
       {
        *(pByte++) = delim;
       }

      len++;
     }
    while (len < cMinMatchLen && byte != delim && byte != EOF);

    while (byte != delim && byte != EOF)
     {
      prevByte = byte;
      if ((byte = getc(pFil)) != EOF)
       {
        if (byte == prevByte)
         {
          equalByteCnt++;
         }
        else
         {
          /*
          * Check for long runs of equal bytes, and add these to
          * the tree also.
          */
          if (equalByteCnt >= cMinEqualRunLen)
           {
            pEqualRunNode = NewTreeNode();
            /* Buffered characters consists of cMinMatchLen equal bytes */
            memset(pEqualRunNode->bytes,prevByte,cMinMatchLen);
            /* Calculate position of start of run */
            pEqualRunNode->filPos = curPos + len - equalByteCnt;
            AddTreeNode(&pTreeRoot,pFil,delim,pEqualRunNode);
            fseek(pFil,curPos+len+1,SEEK_SET); /* Restore file position */
           }
          equalByteCnt = 1;
         }
        //(*pOrgSum) += byte;
       }
      len++;
     }

    AddTreeNode(&pTreeRoot,pFil,delim,pNewNode);
    curPos += len;

   }
  while (byte != EOF);

  // CloseProgressBar();

  return(pTreeRoot);
 }

/* ***********************************************************************
*
* Extend match in two directions, ignoring delimiters: if there is a match
* of length n at position p and p', it can be changed into a match of
* length n+m+q at position p - m and p' - m
*        <  m   ><   n     ><  q  >
*                p
*  ...aaazzzzzzzzxxxxxxxxxxxyyyyyyyccccccccccccccccc    (pOrgFil)
*  ...bbbzzzzzzzzxxxxxxxxxxxyyyyyyyddddddddddddddddd    (pDerivedFil)
*                p'
*/

void FilePatcher::ExtendMatch(long *pOrgFilPos, FILE * pOrgFil,
		 long *pDerivedFilPos, FILE * pDerivedFil,
                 long *pMatchLen)
 {
  uchar buf1[cBufSiz];
  uchar buf2[cBufSiz];
  int l;
  long step;
  bool isDone;
  uchar *p1, *p2;
  long endPos1, endPos2;

  /*
  * First, try to read forward and extend match toward the end
  */
  isDone = FALSE;
  endPos1 = *pOrgFilPos + *pMatchLen;
  endPos2 = *pDerivedFilPos + *pMatchLen;
  while (! isDone)
   {
    step = cBufSiz;
    fseek(pOrgFil,endPos1,SEEK_SET);
    /* Eventually shrink step if pOrgFil is too short */
    step = fread(buf1,1L,step,pOrgFil);

    fseek(pDerivedFil,endPos2,SEEK_SET);
    /* Eventually shrink step if derivedFil is too short */
    step = fread(buf2,1L,step,pDerivedFil);

    /* step < cBufSiz if one of the files at eof: stop comparing */
    isDone = (step < cBufSiz);

    /* Prepare endPos1 and endPos2 for reading next buffer */
    endPos1 += step;
    endPos2 += step;

    p1 = buf1;
    p2 = buf2;
    while (*p1 == *p2 && step > 0)
     {
      p1++;
      p2++;
      step--;
      (*pMatchLen)++;
     }

    /* step > 0 if *p1 != *p2 found: stop comparing */
    isDone = isDone || step > 0;

   }

  /*
  * Second, try to read backwards and extend the match towards the file start.
  * Stop extending if orgFilPos or derivedFilPos equal 0.
  */
  isDone = FALSE;
  while (*pOrgFilPos > 0 && *pDerivedFilPos > 0 && ! isDone)
   {
    /*
    * Try to step cBufSiz bytes back. If orgFilPos or derivedFilPos is
    * less than that, reduce the step size accordingly.
    */
    step = cBufSiz;
    if (*pOrgFilPos < step)
      step = *pOrgFilPos;
    if (*pDerivedFilPos < step)
      step = *pDerivedFilPos;

    /* Jump back 'step' bytes and read two buffers of 'step' bytes. */
    (*pOrgFilPos) -= step;
    fseek(pOrgFil,*pOrgFilPos,SEEK_SET);
    fread(buf1,1L,step,pOrgFil);

    (*pDerivedFilPos) -= step;
    fseek(pDerivedFil,*pDerivedFilPos,SEEK_SET);
    fread(buf2,1L,step,pDerivedFil);

    /* Put pointers at the end of the buffers */
    p1 = buf1 + step - 1;
    p2 = buf2 + step - 1;

    /* Run backwards until a difference found or at start of buffer */
    l = step;
    while (*p1 == *p2 && l > 0)
     {
      p1--;
      p2--;
      l--;
      /*
      * Adjust matchLen for each matched byte.
      */
      (*pMatchLen)++;
     }

    isDone = (l > 0);
    /*
    * Adjust orgFilPos and derivedFilPos: add length of unequal part of buffer
    */
    (*pOrgFilPos) += l;
    (*pDerivedFilPos) += l;
   }

 }

/* ***********************************************************************
*
* NewMatchBlock(): create a new match block or reuse one of the free list
*/

TMatchBlock *FilePatcher::NewMatchBlock()
{
	TMatchBlock *pMatchBlock;

	if (gLstFreeMatchBlock == NULL)
	{
		pMatchBlock = new TMatchBlock();
		if (pMatchBlock == NULL)
		{
			FatalError("Out of memory");
		}
	}
	else
	{
		pMatchBlock = gLstFreeMatchBlock;
		gLstFreeMatchBlock = gLstFreeMatchBlock->pNxt;
	}

	pMatchBlock->orgFilPos = 0L;
	pMatchBlock->len = 0L;
	pMatchBlock->derivedFilPos = 0L;
	pMatchBlock->pNxt = NULL;

	return(pMatchBlock);
}

/* ***********************************************************************
*
* Release a match block: put on the free list
*/

void FilePatcher::FreeMatchBlock(TMatchBlock *pMatchBlock)
{
	pMatchBlock->pNxt = gLstFreeMatchBlock;
	gLstFreeMatchBlock = pMatchBlock;
}

/* ***********************************************************************
*
* Add new match between pOrgTreeNode and pDerivedTreeNode, with length
* matchlen. If the match is long enough (after extending): add it to
* the list of matches. The list of matches is kept in order of increasing
* starting position in derivedFil.
* If a new match is added that is completely part of an existing match, it
* is dropped.
* If a new match is added that encloses one or more existing matches, the
* enclosed matches are dropped, and the new one is added.
* The resulting list will only contain matches with different derivedFilPos
* values: if there would be two equal derivedFilPos values, one of the two
* blocks would enclose the other and would have been dropped.
*/

void FilePatcher::AddMatch(TTreeNode *pOrgTreeNode, FILE * pOrgFil,
              TTreeNode *pDerivedTreeNode, FILE * pDerivedFil,
              long matchLen,
              TMatchBlock **ppMatchLst)
 {
  long orgFilPos, derivedFilPos;
  long distance;
  TMatchBlock *pMatchBlock, *pPrvMatchBlock, *pNxtMatchBlock;
  bool dropNewMatch;

  orgFilPos = pOrgTreeNode->filPos;
  derivedFilPos = pDerivedTreeNode->filPos;
  /*
  * Pass 1: check if there are matchblocks that enclose the new match
  * (relative to derivedFil), in a way that they are an expansion of this
  * new match block. This saves us expanding this block, because after
  * expansion, it would be the same as the overlapping block.
  * This is done by checking the distance between the positions for
  * orgFil and derivedFil: if the larger block and the smaller block have
  * the same distance, and the smaller block is part of the larger
  * one, the smaller will expand to be equal to the larger one.
  */

  distance = orgFilPos - derivedFilPos;

  pMatchBlock = *ppMatchLst;
  dropNewMatch = FALSE;
  while (pMatchBlock != NULL
         && pMatchBlock->derivedFilPos <= derivedFilPos
	 && ! dropNewMatch)
   {
    dropNewMatch =
    (
   /* pMatchBlock->derivedFilPos <= derivedFilPos
     &&                  Already tested in while condition */
      derivedFilPos <= pMatchBlock->derivedFilPos + pMatchBlock->len
     &&
      pMatchBlock->distance == distance
    );
    pMatchBlock = pMatchBlock->pNxt;
   }

  if (! dropNewMatch)
   {
    ExtendMatch(&orgFilPos,pOrgFil,&derivedFilPos,pDerivedFil,&matchLen);

    if (matchLen >= cMinMatchLen)
     {
      /*
      * Pass 2: check if there are matchblocks that enclose the new match
      * (relative to derivedFil).
      */
      pMatchBlock = *ppMatchLst;
      while (pMatchBlock != NULL
             && pMatchBlock->derivedFilPos <= derivedFilPos
	     && ! dropNewMatch)
       {
	dropNewMatch =
	   (/*
             pMatchBlock->derivedFilPos <= derivedFilPos
	    &&      Already tested in while condition */
             derivedFilPos+matchLen
               <=
             pMatchBlock->derivedFilPos + pMatchBlock->len
           );
        pMatchBlock = pMatchBlock->pNxt;
       }

      if (! dropNewMatch)
       {
        /*
	* Pass 3: drop all matchblocks from list that are enclosed by the
new one
        */
        pMatchBlock = *ppMatchLst;
	pPrvMatchBlock = NULL;
        while (pMatchBlock != NULL)
         {
	  pNxtMatchBlock = pMatchBlock->pNxt;
	  /* Check if pMatchBlock completely enclosed by new match. */
	  if
	   (
             derivedFilPos <= pMatchBlock->derivedFilPos
	    &&
             pMatchBlock->derivedFilPos + pMatchBlock->len
              <=
             derivedFilPos+matchLen
           )
           {
	    /* If completely enclosed: remove from list */
            if (pPrvMatchBlock == NULL)
             {
              *ppMatchLst = pNxtMatchBlock;
             }
            else
             {
              pPrvMatchBlock->pNxt = pNxtMatchBlock;
	     }
            FreeMatchBlock(pMatchBlock);
           }
	  else
           {
	    pPrvMatchBlock = pMatchBlock;
	   }
          pMatchBlock = pNxtMatchBlock;
	 }

        /*
	* Pass 4: Find location to add match to list; keep list sorted on
	* derivedFilPos.
        */
        pNxtMatchBlock = *ppMatchLst;
        pPrvMatchBlock = NULL;
        while (pNxtMatchBlock != NULL
               && pNxtMatchBlock->derivedFilPos < derivedFilPos)
         {
          pPrvMatchBlock = pNxtMatchBlock;
	  pNxtMatchBlock = pNxtMatchBlock->pNxt;
         }

	/*
	* Add new match
	*/
	pMatchBlock = NewMatchBlock();
        pMatchBlock->orgFilPos = orgFilPos;
	pMatchBlock->derivedFilPos = derivedFilPos;
        pMatchBlock->distance = distance;
	pMatchBlock->len = matchLen;
        pMatchBlock->pNxt = pNxtMatchBlock;
        if (pPrvMatchBlock == NULL)
	 {
          *ppMatchLst = pMatchBlock;
         }
        else
         {
          pPrvMatchBlock->pNxt = pMatchBlock;
         }
       }
     }
   }
 }

/* ***********************************************************************
*
* Clean up the list of matches: shrink overlapping matches
* and drop those that become too short
*/

void FilePatcher::ShrinkMatchList(TMatchBlock **ppMatchLst)
{
	TMatchBlock *pMatchBlock;
	TMatchBlock *pPrvMatchBlock;
	TMatchBlock *pNxtMatchBlock;
	long distance;

	pPrvMatchBlock = NULL;
	pMatchBlock = *ppMatchLst;
	while (pMatchBlock != NULL)
 	{
		pNxtMatchBlock = pMatchBlock->pNxt;
		if (pNxtMatchBlock != NULL)
		{
			/* distance is maximal length of pMatchBlock without overlap */
      		distance = pNxtMatchBlock->derivedFilPos - pMatchBlock->derivedFilPos;
      		/* Shrink block if too long */
			if (distance < pMatchBlock->len)
			{
				pMatchBlock->len = distance;
			}
		}
		/*
		* Drop blocks that become too short.
		*/
		if (pMatchBlock->len < cMinMatchLen)
		{	
			if (pPrvMatchBlock == NULL)
			{
				*ppMatchLst = pNxtMatchBlock;
			}
			else
			{
				pPrvMatchBlock->pNxt = pNxtMatchBlock;
			}
			FreeMatchBlock(pMatchBlock);
		}
		else
		{
			pPrvMatchBlock = pMatchBlock;
		}
		pMatchBlock = pNxtMatchBlock;
	}
}

#define CRC_COMP(crc) crc = crc ^ 0xffffffffL;
#define CRC_UPDT(crc,byte,table) crc = table[((int)crc ^ (byte)) & 0xff] ^ (crc >> 8);

/* ***********************************************************************
*
* Compare chunks from file 2 with tree from file 1
*/

TMatchBlock *FilePatcher::MatchFiles(TTreeNode *pOrgTreeRoot,
			FILE *      pOrgFil,
			FILE *      pDerivedFil,
			int        delim,
			ulong      *pDerivedSum)
{
	TTreeNode *pOrgTreeNode;
	long         matchLen;
	TMatchBlock *pMatchLst;
	int          byte;
	int          prevByte;
	long         curPos;
	long         prevPos;
	TTreeNode    treeNode;
	TTreeNode    equalRunNode;
	long         len;
	long         equalByteCnt;
	uchar *       pByte;
	const ulong	*crc_table;
	ulong		crc;


	// InitProgressBar(0L,FileSize(pDerivedFil),"Pass 3 of 3:");

	pMatchLst = NULL;

	crc_table = get_crc_table();	
	crc = 0L;
	CRC_COMP(crc);
	
	curPos = 0;
	prevPos = 0;
	do
 	{
 		if (curPos - prevPos > 0x3FF)
 		{
 			AdjustProgressBar(curPos);
 			prevPos = curPos;
 			if (doCancel)
 				return pMatchLst;
 		}

		/*
		* Restore file position (destroyed by FindBestMatch below)
		*/
		fseek(pDerivedFil,curPos,SEEK_SET);

		/*
		* Buffer some characters from the file.
		*/
	    len = 0;
	    treeNode.filPos = curPos;
	    pByte = treeNode.bytes;
	    byte = -1;
	    equalByteCnt = 0;
	    do
	    {
	    	prevByte = byte;
	    	if ((byte = getc(pDerivedFil)) != EOF)
	    	{
	    		if (byte == prevByte)
	    		{	
	    			equalByteCnt++;
	    		}
	    		else
	    		{
		          /*
		          * No need to check for cMinMatchLen or more equal bytes here;
		          * if they were here, they will be checked against the tree
		          * anyhow. This loop only executes at most cMinMatchLen times.
		          */
					equalByteCnt = 1;
				}
				CRC_UPDT(crc,byte,crc_table);
				// (*pDerivedSum) += byte;
				*(pByte++) = byte;
			}
			else
			{
				*(pByte++) = delim;
			}
			len++;
		}
		while (len < cMinMatchLen && byte != delim && byte != EOF);

		// update crc, we read in len bytes
		// crc = crc32(crc, treeNode.bytes, len);

		while (byte != delim && byte != EOF)
		{
			prevByte = byte;
			
			/**** handle runs of equal bytes *****/
			if ((byte = getc(pDerivedFil)) != EOF)
			{
				if (byte == prevByte)
				{
					equalByteCnt++;
				}
				else
				{
					/*
					* Check for long runs of equal bytes, and check these against
					* the tree also.
					*/
					if (equalByteCnt >= cMinEqualRunLen)
					{
						/* Buffered characters consists of cMinMatchLen equal bytes */
						memset(equalRunNode.bytes,prevByte,cMinMatchLen);
						/* Calculate position of start of run */
						equalRunNode.filPos = curPos + len - equalByteCnt;
						/*
						* Search best match in original tree
						*/
						FindBestMatch(pOrgTreeRoot,&pOrgTreeNode,pOrgFil,
                          			&equalRunNode,pDerivedFil,
                          			delim,&matchLen);

						if (pOrgTreeNode != NULL)
						{
							/*
							* Add match to list of matches
							*/
							AddMatch(pOrgTreeNode,pOrgFil,
									&equalRunNode,pDerivedFil,
									matchLen,&pMatchLst);
						}
						fseek(pDerivedFil,curPos+len+1,SEEK_SET); /* Restore file
																		position */
					}
					/*
					* Reset equal byte count to 1.
					*/
					equalByteCnt = 1;
				}
				CRC_UPDT(crc,byte,crc_table);
				//(*pDerivedSum) += byte;
			}
			len++;
		}
		curPos += len;

		// update crc, we read len count bytes

		/*
		* Search best match in original tree
		*/
		FindBestMatch(pOrgTreeRoot,&pOrgTreeNode,pOrgFil,
        	          &treeNode,pDerivedFil,
            	      delim,&matchLen);

		if (pOrgTreeNode != NULL)
		{
			/*
			* Add match to list of matches
			*/
			AddMatch(pOrgTreeNode,pOrgFil,
               	&treeNode,pDerivedFil,
               	matchLen,&pMatchLst);
		}
	}
	while (byte != EOF);

	// CloseProgressBar();

	/*
	* Remove overlapping matches
	*/
	ShrinkMatchList(&pMatchLst);
	
	
	CRC_COMP(crc);
	*pDerivedSum = crc;

	return(pMatchLst);
}

/* ***********************************************************************
*
* Write patch file
*/

void FilePatcher::DumpDiff(TMatchBlock *pMatchLst, FILE * pDerivedFil, FILE * pDiffFil)
{
  	TMatchBlock *pMatchBlock;
  	long len, pos;
  	long blockPos;
  	long writeLen;
  	long blockLen;
  	long codeLen;
  	long filSiz;
  	long nextPos;

  	filSiz = FileSize(pDerivedFil);

  	// fprintf(stderr, "\nWriting patch file...\n");
  
  
  	/********  MK*/
#if DEBUG  
  	long matchedChunks = 0;
  	long unmatchedChunks = 0;
  	long maxMatch = 0;
  	long maxNoMatch = 0;
  	long matchSum = 0;
  	long noMatchSum = 0;
#endif  
  	/*********/
  
  

	/*
	* Descend match block list. Resulting file is a series of matches and
	* non-matches between original file and derived file.
	*/
  	pMatchBlock = pMatchLst;
  	len = filSiz;
  	pos = 0;
  	/*
  	* Repeat until all bytes of derivedFil have been checked
  	*/
  	while (len > 0)
   	{
    	/*
    	* Get next matching position from match block. If there is none,
    	* set nextPos beyond eof on derived file.
    	* If there are unmatched bytes between the last position written
    	* and the next position, write them to the patch file.
    	* Possibly there are no unmatched bytes (nextPos == pos), in case
    	* of consecutive match blocks.
    	*/
    	nextPos = pMatchBlock != NULL ? pMatchBlock->derivedFilPos : filSiz;
    	if (nextPos > pos)
     	{
      		/*
      		* There are unmatched bytes: write one or more block of unmatched bytes
      		* from derivedFil
      		*/
      		fseek(pDerivedFil,pos,SEEK_SET);
      		writeLen = nextPos - pos;
      
      		/****** MK */
      	#if DEBUG
      		unmatchedChunks++;
      		noMatchSum += writeLen;
      		if (writeLen > maxNoMatch)
      			maxNoMatch = writeLen;
      	#endif
      		/*********/
      
      		while (writeLen > 0)
       		{
				/*
        		* If remaining block is small, use tag for small
        		* blocks (1 tag/length byte)
				*/
        		if (writeLen <= cSmallSize)
         		{
          			blockLen = writeLen;
          			codeLen = blockLen - 1; /* Encode length 1-cSmallSize by subtr. 1 */
          			/* codeLen: 4 bit value */
          			/* bit 3-0 | Tag */
          			putc((codeLen <<  4)          | cTagSmallDiff       ,pDiffFil);
         		}
				/*
        		* If remaining block is medium size use tag for
				* medium blocks (2 tag/length bytes)
        		*/
        		else if (writeLen <= cMediumSize)
         		{
          			blockLen = writeLen;
          			/* Encode length cSmallSize+1 - cMediumSize by subtr. cSmallSize+1 */
	  				codeLen = blockLen - cSmallSize - 1;
          			/* codeLen: 12 bit value */
          			/* bit 11-8 | Tag */
	  				putc(((codeLen >>  4) & 0xF0) | cTagMediumDiff      ,pDiffFil);
          			/* bit 7-0 */
          			putc( (codeLen      ) & 0xFF                        ,pDiffFil);
         		}
        		else
         		{
          			/*
          			* If remaining block is large: write a large block,
          			* and then re-check the remaining length
	  				*/
          			if (writeLen > cLargeSize)
	   				{
            			blockLen = cLargeSize;
           			}
          			else
           			{
            			blockLen = writeLen;
	   				}
          			/* Encode length cMediumSize+1 - cLargeSize by subtracting
						cMediumSize+1 */
          			codeLen = blockLen - cMediumSize - 1;
	  				/* codeLen: 20 bit value */
          			/* bit 19-16 | Tag */
          			putc(((codeLen >> 12) & 0xF0) | cTagLargeDiff       ,pDiffFil);
          			/* bit 15-8 */
          			putc( (codeLen >>  8) & 0xFF                        ,pDiffFil);
          			/* bit 7-0 */
          			putc( (codeLen      ) & 0xFF                        ,pDiffFil);
         		}

				writeLen -= blockLen;
        		len -= blockLen;

				// copy bytes
        		while (blockLen > 0)
         		{
          			putc(getc(pDerivedFil),pDiffFil);
          			blockLen--;
         		}

       		}
     		pos = nextPos;
     	}

    	/*
    	* Write block of matching bytes: encode them as a count and a file position
    	* in the original file.
    	*/
    	if (pMatchBlock != NULL)
	     	{
	      		blockPos = pMatchBlock->orgFilPos;
	      		writeLen = pMatchBlock->len;
	      
	      		/******** MK */
	  		#if DEBUG
		      	matchedChunks++;
		      	matchSum += writeLen;
		      	if (writeLen > maxMatch)
		      		maxMatch = writeLen;
		   	#endif
		      	/************/
	      
		    while (writeLen > 0)
	       	{
	        	if (writeLen <= cSmallSize)
	         	{
		          	blockLen = writeLen;
		          	codeLen = blockLen - 1;
		          	if (blockPos <= cNearDistance)
			   		{
	            		/* codeLen: 4 bit value */
	            		/* bit 3-0 | Tag */
				    	putc((codeLen <<  4)          | cTagSmallNearCopy   ,pDiffFil);
	
	            		/* blockPos: 16 bit value */
		    			WriteLongNBytes(blockPos,pDiffFil,2);
	           		}
		          	else if (blockPos <= cDistantDistance)
		           	{
		            	/* codeLen: 4 bit value */
		            	/* bit 3-0 | Tag */
			    		putc((codeLen <<  4)          | cTagSmallDistantCopy,pDiffFil);
		
			    		/* blockPos: 24 bit value */
			    		WriteLongNBytes(blockPos,pDiffFil,3);
		           	}
		          	else
		           	{
		            	/* codeLen: 4 bit value */
			    		/* bit 3-0 | Tag */
		            	putc((codeLen <<  4)          | cTagSmallFarCopy    ,pDiffFil);
		
			    		/* blockPos: 32 bit value */
			    		WriteLongNBytes(blockPos,pDiffFil,4);
		           	}
	         	}
	        	else if (writeLen <= cMediumSize)
	         	{
	          		blockLen = writeLen;
	          		codeLen = blockLen - cSmallSize - 1;
	          		if (blockPos <= cNearDistance)
		   			{
		            	/* codeLen: 12 bit value */
			    		/* bit 11-8 | Tag */
		            	putc(((codeLen >>  4) & 0xF0) | cTagMediumNearCopy  ,pDiffFil);
		            	/* bit 7-0 */
		            	putc( (codeLen      ) & 0xFF                        ,pDiffFil);
		
		            	/* blockPos: 16 bit value */
			    		WriteLongNBytes(blockPos,pDiffFil,2);
	           		}
	          		else if (blockPos <= cDistantDistance)
		   			{
		            	/* codeLen: 12 bit value */
		            	/* bit 11-8 | Tag */
		            	putc(((codeLen >>  4) & 0xF0) |cTagMediumDistantCopy,pDiffFil);
		            	/* bit 7-0 */
		            	putc( (codeLen      ) & 0xFF                        ,pDiffFil);
		
		            	/* blockPos: 24 bit value */
			    		WriteLongNBytes(blockPos,pDiffFil,3);
		   			}
	          		else
	           		{
			            /* codeLen: 12 bit value */
			            /* bit 11-8 | Tag */
			            putc(((codeLen >>  4) & 0xF0) | cTagMediumFarCopy   ,pDiffFil);
			            /* bit 7-0 */
			            putc( (codeLen      ) & 0xFF                        ,pDiffFil);
			
			            /* blockPos: 32 bit value */
		    			WriteLongNBytes(blockPos,pDiffFil,4);
		   			}
	         	}
	        	else
	         	{
	          		if (writeLen > cLargeSize)
	           		{
	            		blockLen = cLargeSize;
	           		}
	          		else
		   			{
	            		blockLen = writeLen;
	           		}
	          		codeLen = blockLen - cMediumSize - 1;
	          		if (blockPos <= cNearDistance)
	           		{
			            /* codeLen: 20 bit value */
			            /* bit 19-16 | Tag */
					    putc(((codeLen >> 12) & 0xF0) | cTagLargeNearCopy   ,pDiffFil);
			            /* bit 15-8 */
			            putc( (codeLen >>  8) & 0xFF                        ,pDiffFil);
					    /* bit 7-0 */
			            putc( (codeLen      ) & 0xFF                        ,pDiffFil);
	
			            /* blockPos: 16 bit value */
					    WriteLongNBytes(blockPos,pDiffFil,2);
	 	            }
	          		else if (blockPos <= cDistantDistance)
	           		{
			            /* codeLen: 20 bit value */
					    /* bit 19-16 | Tag */
			            putc(((codeLen >> 12) & 0xF0) | cTagLargeDistantCopy,pDiffFil);
			            /* bit 15-8 */
			            putc( (codeLen >>  8) & 0xFF                        ,pDiffFil);
			            /* bit 7-0 */
			            putc( (codeLen      ) & 0xFF                        ,pDiffFil);
	
	            		/* blockPos: 24 bit value */
		    			WriteLongNBytes(blockPos,pDiffFil,3);
	           		}
	          		else
		  		 	{
	            		/* codeLen: 20 bit value */
			            /* bit 19-16 | Tag */
			            putc(((codeLen >> 12) & 0xF0) | cTagLargeFarCopy    ,pDiffFil);
			            /* bit 15-8 */
			            putc( (codeLen >>  8) & 0xFF                        ,pDiffFil);
			            /* bit 7-0  */
			            putc( (codeLen      ) & 0xFF                        ,pDiffFil);
	
				    	/* blockPos: 32 bit value */
		    			WriteLongNBytes(blockPos,pDiffFil,4);
	           		}
	         	}
	        	writeLen -= blockLen;
	        	len -= blockLen;
	        	pos += blockLen;
	       	}
			pMatchBlock = pMatchBlock->pNxt;
		}
	}

  	putc(cTagEOF,pDiffFil);
  

/**  
  	fprintf(stderr,"%d matched chunks, max size %d bytes, average size %d bytes\n",
  					matchedChunks,maxMatch,matchSum / matchedChunks);
  	fprintf(stderr,"%d unmatched chunks, max size %d bytes, average size %d bytes\n",
  					unmatchedChunks,maxNoMatch,noMatchSum / unmatchedChunks);
**/
}

