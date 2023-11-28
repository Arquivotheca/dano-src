#include <Be.h>
#include "FilePatcher.h"
#include "GStatusWind.h"
#include "PackMessages.h"
#include <Path.h>

#include "Util.h"
#include "ExtractUtils.h"

#include "MyDebug.h"

/* ***********************************************************************
*
* Initialise progress bar
*/

void FilePatcher::InitProgressBar(long minVal, long maxVal, char * message)
{
	message;
  if (maxVal == minVal)
  {
  	maxVal = minVal + 1;
  }

  gMinVal = minVal;
  gMaxVal = maxVal;
  gCurPos = 0;
  gLastVal = gCurVal = 0;

	/**
  fprintf(stderr,"%s\n", message);
  fprintf(stderr,"____________________________________________________________\n");
	**/

	// GStatusWindow *statWind = new GStatusWindow(rect,"Patch Status");
	
	PatchStatusWindow *statWind = new PatchStatusWindow("Patch Status",&doCancel);
	
	statMess = statWind->StatusMessenger();
	BMessage setupMsg(M_SETUP_STATUS);
	setupMsg.AddString("message","Computing File Differences...");
	setupMsg.AddInt32("bytes",gMaxVal - gMinVal);
	setupMsg.AddBool("cancancel",TRUE);
	statMess.SendMessage(&setupMsg);
}

/* ***********************************************************************
*
* Change progress bar length according to curVal between gMinVal and gMaxVal
*/

void FilePatcher::AdjustProgressBar(long curVal)
{
  // int pos;
  
  if (curVal < gLastVal)
  	gLastVal = 0;
  long delta = curVal - gLastVal;
  
  // delta is fed to the be progress bar
  BMessage updt(M_UPDATE_PROGRESS);
  updt.AddInt32("bytes",delta);
  statMess.SendMessage(&updt);
  
  gCurVal += curVal - gLastVal;
  gLastVal = curVal;

	/***
  pos = 60*(gCurVal - gMinVal) / (gMaxVal - gMinVal);
  while (pos > gCurPos)
  {
    fprintf(stderr,"#");
    gCurPos++;
  }
  	***/
}

/* ***********************************************************************
*
* Close progress bar
*/

void FilePatcher::CloseProgressBar()
{	
  	AdjustProgressBar(gMaxVal);
	
	statMess.SendMessage(B_QUIT_REQUESTED);
  	// fprintf(stderr,"\n");
}

/* ***********************************************************************
*
* Calculate patch file.
*/

entry_ref FilePatcher::MakePatch()
{
  	TTreeNode	*pOrgTreeRoot;
  	int          delim;
  	TMatchBlock *pMatchLst;
  	
  	unsigned long		size;
  	unsigned long		orgSum;
  	unsigned long		orgSize;
  	unsigned long		derivedSum;
  	unsigned long		derivedSize;
  	
  	if (!pOrgFil || !pDerivedFil || !pDiffFil) {
  		doError("Sorry, the patch could not be created");
  		doCancel = TRUE;
  		return outputPatchRef;
  	}

	// write magic number
	WriteLongNBytes(PATCH_MAGIC_NUM,pDiffFil,4);

	// get file sizes
  	orgSize = FileSize(pOrgFil);
  	size = derivedSize = FileSize(pDerivedFil);

	InitProgressBar(0L,/* orgSize + */ orgSize + size,"Computing Patch:");
  	/*
  	* Write dummy check-data; gets rewritten at end of this procedure
	*/
	// bogus, do this with seek
  	WriteLongNBytes(0L,pDiffFil,4);
  	WriteLongNBytes(0L,pDiffFil,4);
  	WriteLongNBytes(0L,pDiffFil,4);
  	WriteLongNBytes(0L,pDiffFil,4);

	// checksums (improved to crc32)
  	orgSum = 0L;
  	derivedSum = 0L;

	{  /* Dummy block */

		// if new file has zero size
    	if (size == 0)
		{
	      	int byte;
	
	      	/*
	      	* EOF on patch file
	      	*/
	      	putc(cTagEOF,pDiffFil);
	      	/*
	      	* Adjust checksum
	      	*/
	      	// compute checksum of original, UPDATE this!!
	      	do
	       	{
	        	if ((byte = getc(pOrgFil)) != EOF)
	         	{
	          		orgSum += byte;
	         	}
	       	} while (byte != EOF);

     	}
    	else
     	{
      		/*
      		* Find suitable delimiter
      		*/
      		delim = FindDelimiter(pOrgFil,&orgSum,cMinMeanChunkLen,cMaxMeanChunkLen);
      		if (delim < 0) delim = 0;


			if (doCancel)
				return outputPatchRef;
								
      		/*
      		* Build indexed position tree
      		*/
     		pOrgTreeRoot = BuildTree(pOrgFil,delim);
			
			if (doCancel) {
				FreeAllTree(pOrgTreeRoot);
				return outputPatchRef;
			}
			
	      	/*
	      	* Match files
      		*/
      		pMatchLst =
				MatchFiles(pOrgTreeRoot,pOrgFil,pDerivedFil,delim,&derivedSum);

			FreeAllTree(pOrgTreeRoot);

			if (doCancel) {
				FreeMatchList(pMatchLst);
				return outputPatchRef;
			}

      		/*
      		* Write patch file
      		*/
			DumpDiff(pMatchLst,pDerivedFil,pDiffFil);
			
			FreeMatchList(pMatchLst);
		}
		// done in destructor
		//fclose(pOrgFil);
		//fclose(pDerivedFil);

	}

	/*
	* Encode extra EOF
	*/
	putc(cTagEOF,pDiffFil);

	/*
	* Adjust the check-data
	*/
	fseek(pDiffFil,sizeof(ulong),SEEK_SET);

	// write original file size
	WriteLongNBytes(orgSize,pDiffFil,4);
	// original checksum
	WriteLongNBytes(orgSum,pDiffFil,4);
	// new file size
	WriteLongNBytes(derivedSize,pDiffFil,4);
	// new file checksum
	WriteLongNBytes(derivedSum,pDiffFil,4);

	// done in destructor
	//fclose(pDiffFil);
	
	return outputPatchRef;
}

FilePatcher::FilePatcher(entry_ref &oldFile, entry_ref &newFile)
		:	gLstFreeTreeNode(NULL),
			gLstFreeMatchBlock(NULL),
			pOrgFil(NULL),
			pDerivedFil(NULL),
			pDiffFil(NULL),
			doCancel(FALSE)
{
	//statMess = NULL;
	// outputPatchRef.database = outputPatchRef.record = -1;

	BPath	fPath;
	BEntry	fEnt;

	///////////
	fEnt.SetTo(&oldFile);	
	if (fEnt.InitCheck() < B_NO_ERROR) {
		doError("Old file not found");
		goto baderr;
	}	
	if (fEnt.GetPath(&fPath) < B_NO_ERROR) {
		doError("Error gettting pathname of old file.");
		goto baderr;
	}	
	
	PRINT((fPath.Path()));
	PRINT(("\n"));
	
  	pOrgFil = fopen(fPath.Path(),"rb");
  	if (pOrgFil == NULL) {
   		doError("Error opening old version of file");
   		goto baderr;
   	}

	/////////////////
	
	fEnt.SetTo(&newFile);	
	if (fEnt.InitCheck() < B_NO_ERROR) {
		doError("New file version not found");
		goto baderr;
	}	
	if (fEnt.GetPath(&fPath) < B_NO_ERROR) {
		doError("Error gettting pathname of new file.");
		goto baderr;
	}
	PRINT((fPath.Path()));
	PRINT(("\n"));

  	pDerivedFil = fopen(fPath.Path(),"rb");
  	if (pDerivedFil == NULL)
   	{
   	   	doError("Error opening new version of file");
   		goto baderr;
	}
		
	char tempPath[PATH_MAX];
	
	tmpnam(tempPath);
	PRINT((tempPath));
	PRINT(("\n"));
	
	pDiffFil = fopen(tempPath,"wb");
  	if (pDiffFil == NULL)
   	{
   		doError("Error opening temporary patch file");
   		goto baderr;
   	}
   	
   	get_ref_for_path(tempPath,&outputPatchRef);

baderr:
	return;
}

FilePatcher::~FilePatcher()
{
	if (pOrgFil) fclose(pOrgFil);
	if (pDerivedFil) fclose(pDerivedFil);
	if (pDiffFil) fclose(pDiffFil);
	statMess.SendMessage(B_QUIT_REQUESTED);
}

void FilePatcher::FreeAllTree(TTreeNode *node)
{
	FreeTree(node);
	FreeTreeList();
}

void FilePatcher::FreeTree(TTreeNode *node)
{
	if (!node)
		return;
		
	FreeTree(node->pGE);	
	FreeTree(node->pLT);

	delete node;
}

void FilePatcher::FreeTreeList()
{
	TTreeNode *node = gLstFreeTreeNode;
	while (node) {
		TTreeNode *next = node->pGE;
		delete node;
		node = next;
	}
}

void FilePatcher::FreeMatchList(TMatchBlock *item)
{
	TMatchBlock *cur = item;
	while (cur) {
		TMatchBlock *next = cur->pNxt;
		delete cur;
		cur = next;
	}
	cur = gLstFreeMatchBlock;
	while (cur) {
		TMatchBlock *next = cur->pNxt;
		delete cur;
		cur = next;
	}
}

long FilePatcher::Error()
{
	if (doCancel)
		return B_ERROR;
	
	return B_NO_ERROR;
}

PatchStatusWindow::PatchStatusWindow(const char *til,
									bool *_cancelBool)
	: GStatusWindow(BRect(0,0,250,90),til),
	  cancelBool(_cancelBool)
{
	PositionWindow(this,0.6,0.6);
}

void PatchStatusWindow::DoCancel(bool stat)
{
	PRINT(("setting cancel var to %d\n",stat));
	*cancelBool = stat;
}




/* ***********************************************************************
*
* Copy characters between files
*/

/********
void CopyFileChars(long count, FILE * inFil, FILE * outFil, long *pSum)
 {
  int c;

  c = 0;
  while (count > 0 && c != EOF)
   {
    c = getc(inFil);
    if (c != EOF)
     {
      (*pSum) += c;
      putc(c,outFil);
      count--;
     }
   }
 }
*/
