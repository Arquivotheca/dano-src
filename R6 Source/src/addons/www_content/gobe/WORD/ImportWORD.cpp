//
//	ImportWORD.cpp
//

#include <stdlib.h>
#include <ctype.h>
#include <algobase.h>		// for min and max
#include <stdio.h>

#include "ImportWORD.h"
#include "WORDLocalStrings.h"

#if 0
#define DEBUG 1
#endif

// Returns the ole entry with this entryName, or 0 if not found
const TOLEEntry *getOleEntry(TOLEReader *oleReader, const char *entryName, const TOLEEntry *currentEntry, int *depth) 
{ 
	if (!oleReader) 
		return 0; 
	
	bool firstPass = false; 
	int prevDepth; 
	int nextDepth; 
	int childDepth; 
	
	const TOLEEntry* prevEntry = 0; 
	const TOLEEntry* nextEntry = 0; 
	const TOLEEntry* childEntry = 0; 
	
	if (!currentEntry) 
	{ 
		firstPass = true; 
		currentEntry = oleReader->Entry(0); 
		depth = new int; 
		*depth = -1; 
	} 
	
	(*depth)++; 
	
	prevDepth = *depth; 
	nextDepth = *depth; 
	childDepth = *depth; 
	
	if (!strcmp(currentEntry->Name(), entryName)) 
		return currentEntry; 
	
	const TOLEEntry *returnEntry = 0; 
	
	if (currentEntry->PrevDirEntry() >= 0) 
		prevEntry = getOleEntry(oleReader, entryName, oleReader->Entry(currentEntry->PrevDirEntry()), &prevDepth); 
	
	if (currentEntry->NextDirEntry() >= 0) 
		nextEntry = getOleEntry(oleReader, entryName, oleReader->Entry(currentEntry->NextDirEntry()), &nextDepth); 
	
	if (currentEntry->ChildDirEntry() >= 0) 
		childEntry = getOleEntry(oleReader, entryName, oleReader->Entry(currentEntry->ChildDirEntry()), &childDepth); 
	
	
	int bestDepth = 0xFFFFFFF; 
	if (prevEntry && prevDepth < bestDepth) 
	{ 
		bestDepth = prevDepth; 
		returnEntry = prevEntry; 
	} 
	if (nextEntry && nextDepth < bestDepth) 
	{ 
		bestDepth = nextDepth; 
		returnEntry = nextEntry; 
	} 
	if (childEntry && childDepth < bestDepth) 
	{ 
		bestDepth = childDepth; 
		returnEntry = childEntry; 
	} 
  
	if (firstPass)
	{
		delete depth;
	}
       
	return returnEntry; 
	/*int32 count = oleReader->Entries();   
	if (count <= 0) 
		return 0; 
	
	// try to find this entry name 
	for (int32 i = 0; i < count; i++) 
	{ 
		const TOLEEntry* ptr = oleReader->Entry(i); 
		if (!strcmp(ptr->Name(), entryName)) 
			return ptr; 
	} 

	return 0;*/ 
}

//-------------------------------------------------------------------
// TImportWORD
//-------------------------------------------------------------------

TImportWORD::TImportWORD(BPositionIO *inStream, TBlockStreamWriter* writer, BMessage* ioExt)
	: mTransDoc(ioExt)
{
	mInStream = inStream;
	mWriter = writer;

	mMainPartPtr = mHeaderPartPtr = mFooterPartPtr = NULL;
	mPartPtr = NULL;

	mOleReader = 0;
	mDocReader = 0;
	mTableReader = 0;
	mStyleSheetWORD = 0;
	mFontTblWORD = 0;
	mFKP_PAPX = 0;
	mFKP_CHPX = 0;
	mLogCharPos = 0;
	mTPtrStore = 0;
	mTCharPosStore = 0;
	mFieldDataSize = 0;
	
	mBufferMaxSize = INIT_BUFFER_SIZE;
	mBuffer = new char[mBufferMaxSize];
	
	mBufferUTF8MaxSize = (3 * INIT_BUFFER_SIZE);
	mBufferUTF8 = new char[mBufferUTF8MaxSize];
	
	mAltBufferMaxSize = INIT_BUFFER_SIZE;
	mAltBuffer = new char[mAltBufferMaxSize];
	
	mAltBufferUTF8MaxSize = (3 * INIT_BUFFER_SIZE);
	mAltBufferUTF8 = new char[mAltBufferUTF8MaxSize];
	
	mPieceTbl = 0;
	mFspaTbl = 0;
	
	for (long i = 0; i < S_TYPE_COUNT; i++)
	{
		mCurrFilterStyle[i] = 0;		
		mNewFilterStyle[i] = 0;
	}
	
	for (long i = 0; i < N_TYPE_COUNT; i++)
	{
		mNoteRefCP[i] = 0;
		mNoteTxtCP[i] = 0;
	}
	
	mInTable = mInField = mProcessingField = false;		
	mCellDividers.AddItem(0);
}


TImportWORD::~TImportWORD()
{
	if (mFIBHeader.nFib >= 190)
		delete mTableReader;
	delete mDocReader;
	delete mOleReader;
	delete mStyleSheetWORD;
	delete mFontTblWORD;
	delete mFKP_PAPX;
	delete mFKP_CHPX;
	
	if (mBuffer)
		delete [] mBuffer;
	if (mBufferUTF8)
		delete [] mBufferUTF8;	
	if (mAltBuffer)
		delete [] mAltBuffer;
	if (mAltBufferUTF8)
		delete [] mAltBufferUTF8;
	
	if (mPieceTbl)
		delete [] mPieceTbl;
	if (mFspaTbl)
		delete [] mFspaTbl;
	
	for (long i = mCLXsprms.CountItems() - 1; i >= 0; i--)
		delete [] (char *)(mCLXsprms.ItemAt(i));
	
	for (long i = 0; i < N_TYPE_COUNT; i++)
	{
		delete [] mNoteRefCP[i];
		delete [] mNoteTxtCP[i];
	}
	
	for (long i = 0; i < S_TYPE_COUNT; i++)
	{
		delete mCurrFilterStyle[i];		
		delete mNewFilterStyle[i];
	}		
}


int32 TImportWORD::DoTranslate(void)
{	
	int32		rtnCode = B_OK;

	// Translate to blockstream, write block stream, import are the 3 major
	// steps during import. We handle the first one here.
	mTransDoc.BeginTask(kReadStr, 0, .33);
	mMainPartPtr = new TTranslatorPart_WP(&mTransDoc);
	mPartPtr = mMainPartPtr;

	// main ole reader
	mOleReader = new TOLEReader(mInStream);
	if (!mOleReader)
	{
		IFDEBUG(fprintf(stderr, "WORD Go: bad OleReader\n"));
		return B_ERROR;
	}

	// document stream reader
	mDocEntry = getOleEntry(mOleReader, "WordDocument");
	if (mDocEntry)
		mDocReader = new TOLEEntryReader(mDocEntry, mOleReader);
	if (!mDocEntry || !mDocReader)
	{
		IFDEBUG(fprintf(stderr, "WORD Go: bad DocReader or DocEntry\n"));
		return B_ERROR;
	}

	
	// Start parsing
	mDocReader->SetStreamPos(0);
	mDocStreamSize = mDocReader->StreamDataSize();

	// load FIB header
	if (rtnCode = loadFIB(), rtnCode)
		return rtnCode;

	IFDEBUG(fprintf(stderr, "mFIBHeader.fcMin = %d (%#4.4x) mFIBHeader.fcMac = %d (%#4.4x)\n", mFIBHeader.fcMin, mFIBHeader.fcMin,
				mFIBHeader.fcMac, mFIBHeader.fcMac));
	
	// if this file is encripted, then we can't open it
	if (mFIBHeader.bitField1 & fEncrypted)
		return B_ERROR;
	
// table stream reader
	if (mFIBHeader.nFib > 190)
	{
		mTableEntry = getOleEntry(mOleReader, (mFIBHeader.bitField1 & fWhichTblStm) ? "1Table" : "0Table");
		if (mTableEntry)
			mTableReader = new TOLEEntryReader(mTableEntry, mOleReader);		
		if (!mTableEntry || !mTableReader)
		{
			IFDEBUG(fprintf(stderr, "WORD Go: ERROR bad TableReader\n"));
			return B_ERROR;
		}
	}
	else // assume this is Word 6
	{
		mTableEntry = mDocEntry;
		mTableReader = mDocReader;
	}

	mTableReader->SetStreamPos(0);
	mTableStreamSize = mTableReader->StreamDataSize();

	// load document properties
	if (rtnCode = loadDocProperties(), rtnCode)
	{
		IFDEBUG(fprintf(stderr, "WORD Go: ERROR bad loadDocProperties\n"));
		return rtnCode;
	}
	sendFormatDocMsg();
	
	//IFDEBUG(fprintf(stderr, "WORD Stsh: orig x%x, %d bytes, new x%x, %d bytes\n", mFIBPair[StshfOrig].theLong, mFIBPair[StshfOrig].theULong, mFIBPair[Stshf].theLong, mFIBPair[Stshf].theULong));
	//IFDEBUG(fprintf(stderr, "WORD FontNames: x%x, %d bytes\n", mFIBPair[Sttbfffn].theLong, mFIBPair[Sttbfffn].theULong));
	//IFDEBUG(fprintf(stderr, "WORD PieceTbl: x%x, %d bytes\n", mFIBPair[Clx].theLong, mFIBPair[Clx].theULong));

	// load font names
	mTransDoc.BeginTask(kFontInfoStr, 0, .05);
	mFontTblWORD = new TFontTblWORD(mTableReader, mTableStreamSize, mFIBPair[Sttbfffn].theLong, mFIBPair[Sttbfffn].theULong, mFIBHeader.nFib);
	if (mTableReader->Error() < 0)
		return mTableReader->Error();
	mTransDoc.EndTask();
	
	// load Style Sheet
	mTransDoc.BeginTask(kStylesheetInfoStr, .05, .15);
	mTransDoc.SendTaskProgress(.05);
	mStyleSheetWORD = new TStyleSheetWORD(mTableReader, mTableStreamSize, mFIBPair[Stshf].theLong, mFIBPair[Stshf].theULong, mFontTblWORD, mFIBHeader.nFib);
	if (mTableReader->Error() < 0)
		return mTableReader->Error();
	mTransDoc.EndTask();
	
	spew(mTableReader, mFIBPair[Stshf].theLong, mFIBPair[Stshf].theULong);
	
	// load piece table
	mTransDoc.BeginTask(kPieceTableStr, .15, .20);
	if (rtnCode = loadPieceTable(), rtnCode)
		return rtnCode;
	mTransDoc.EndTask();
	
	// load table of CPs of footnotes, endnotes, and annotations
	mTransDoc.BeginTask(kFootnotesStr, .20, .25);
	if (rtnCode = loadAllNoteCPs(), rtnCode)
		return rtnCode;
	mTransDoc.EndTask();
	
	// load file shapes
	if (mFIBHeader.nFib > 190)
		if (rtnCode = loadFileShapes(), rtnCode)
			return rtnCode;
	
	// load PAPX 
	mTransDoc.BeginTask(kParFormatStr, .25, .37);
	mFKP_PAPX = new TPAPXTbl(mDocReader, mTableReader, mFIBPair[PlcfbtePapx].theLong, mFIBPair[PlcfbtePapx].theULong, mFIBHeader.nFib);
	if (mDocReader->Error() < 0)
		return mDocReader->Error();
	if (mTableReader->Error() < 0)
		return mTableReader->Error();
	if (!mFKP_PAPX || !mFKP_PAPX->Load())
	{
		IFDEBUG(fprintf(stderr, "WORD Go: ERROR bad load of PAPX\n"));
		if (mDocReader->Error() < 0)
			return mDocReader->Error();
		if (mTableReader->Error() < 0)
			return mTableReader->Error();
		return B_ERROR;
	} 
	mTransDoc.EndTask();
	
	// load CHPX
	mTransDoc.BeginTask(kCharFormatStr, .37, .50);
	mFKP_CHPX = new TCHPXTbl(mDocReader, mTableReader, mFIBPair[PlcfbteChpx].theLong, mFIBPair[PlcfbteChpx].theULong, mFIBHeader.nFib);
	if (mDocReader->Error() < 0)
		return mDocReader->Error();
	if (mTableReader->Error() < 0)
		return mTableReader->Error();
	if (!mFKP_CHPX || !mFKP_CHPX->Load())
	{
		IFDEBUG(fprintf(stderr, "WORD Go: ERROR bad load of CHPX\n"));
		if (mDocReader->Error() < 0)
			return mDocReader->Error();
		if (mTableReader->Error() < 0)
			return mTableReader->Error();
		return B_ERROR;
	} 
	mTransDoc.EndTask();
	
	// send the styles to the app so we can refer to them by name
	mStyleSheetWORD->SendStyles(&mTransDoc);

	// Don't bother reading the SED and SEP section properties yet since Squirrel
	// doesn't support sections.  Is similar to loading PAPX and CHPX.
	
	// init character parsing fields	
	for (long i = 0; i < S_TYPE_COUNT; i++)
	{
		mStyleStartFC[i] = 0;
		mStyleEndFC[i] = 0;
		mStyleChanged[i] = false;
	}

	// Character counts of all of the components of the text stream.
	IFDEBUG(fprintf(stderr, "WORD (ccp): Text %d, Ftn %d, Hdd %d, Atn %d, Edn %d, Txbx %d, HdrTxbx %d\n", 
			mFIBLong[ccpText], mFIBLong[ccpFtn], mFIBLong[ccpHdr], mFIBLong[ccpAtn], mFIBLong[ccpEdn], mFIBLong[ccpTxbx], mFIBLong[ccpHdrTxbx]));
		
	// Start sending the actual text to Squirrel

	// main body text
	mTransDoc.BeginTask(kMainBodyStr, .50, .95);
	if (rtnCode = processText(0, mFIBLong[ccpText] - 1, mBuffer, mBufferMaxSize, mBufferUTF8, mBufferUTF8MaxSize), rtnCode)
		return rtnCode;
	mTransDoc.EndTask();

	// header text
	mTransDoc.BeginTask("", .95, 1);
	if (rtnCode = processHeaders(), rtnCode)
		return rtnCode;
	mTransDoc.EndTask();

	mTransDoc.EndTask();  // Translating Word
#ifdef DEBUG
	
// add done, send quit message
//	mOutMsgStream->AddMessage(new BMessage(B_QUIT_REQUESTED));
	
	//mFKP_PAPX->AddParagraphRuns(mMainPartPtr, mFontTblWORD);
	//mFKP_CHPX->AddCharacterRuns(mMainPartPtr);
		
	// Create a sheet with our wordprocessing part as the main part.
	// Then write the entire block stream document.

/*	printf("mFIBLong[pnLvcFirst] = %d, mFIBLong[cpnBteLvc] = %d\n", mFIBLong[pnLvcFirst], mFIBLong[cpnBteLvc]);
	printf("mFIBPair[PlcfbteLvc].theLong = %d, mFIBPair[PlcfbteLvc].theULong = %d\n",
							mFIBPair[PlcfbteLvc].theLong, mFIBPair[PlcfbteLvc].theULong);
	printf("mFIBPair[Plcflvc].theLong = %d, mFIBPair[Plcflvc].theULong = %d\n",
							mFIBPair[Plcflvc].theLong, mFIBPair[Plcflvc].theULong);
	printf("mFIBPair[Plcfsed].theLong = %d, mFIBPair[Plcfsed].theULong = %d\n",
							mFIBPair[Plcfsed].theLong, mFIBPair[Plcfsed].theULong);
	
	*/
	
/*	if (mNewFilterStyle[S_PARA] || mNewFilterStyle[S_CHAR])
		fprintf(stderr, "WORD checkStyles: ERROR - still have new filterStyles\n");	 
	fprintf(stderr, "the SED table starts at %d, is %d long and looks like\n", mFIBPair[Plcfsed].theLong, mFIBPair[Plcfsed].theULong);
	spew(mTableReader, mFIBPair[Plcfsed].theLong, mFIBPair[Plcfsed].theULong);

	fprintf(stderr, "the header info is at %d, is %d long and looks like\n", mFIBPair[Plcfhdd].theLong, mFIBPair[Plcfhdd].theULong);
	spew(mTableReader, mFIBPair[Plcfhdd].theLong, mFIBPair[Plcfhdd].theULong);
	
	fprintf(stderr, "the dop is at 0x%x, is 0x%x long and looks like\n", mFIBPair[Dop].theLong, mFIBPair[Dop].theULong);
	spew(mTableReader, mFIBPair[Dop].theLong, mFIBPair[Dop].theULong);
	
	printf("the font table starts at %d, is %d long and looks like\n", mFIBPair[Sttbfffn].theLong, mFIBPair[Sttbfffn].theULong);
	spew(mTableReader, mFIBPair[Sttbfffn].theLong, mFIBPair[Sttbfffn].theULong);

	printf("the styles table starts at %d, is %d long and looks like\n", mFIBPair[Stshf].theLong, mFIBPair[Stshf].theULong);
	spew(mTableReader, mFIBPair[Stshf].theLong, mFIBPair[Stshf].theULong);

	printf("dgg starts at %d, is %d long and looks like\n", mFIBPair[DggInfo].theLong, mFIBPair[DggInfo].theULong);
	spew(mTableReader, mFIBPair[DggInfo].theLong, mFIBPair[DggInfo].theULong);

	printf("cbMac is %x\n", mFIBLong[cbMac]);*/
	
	//spewFIB();
	
	/*printf("the LVC bin table starts at %d, is %d long and looks like\n", mFIBPair[PlcfbteLvc].theLong, mFIBPair[PlcfbteLvc].theULong);
	spew(mTableReader, mFIBPair[PlcfbteLvc].theLong, mFIBPair[PlcfbteLvc].theULong);

	printf("the PAPx bin table starts at %d, is %d long and looks like\n", mFIBPair[PlcfbtePapx].theLong, mFIBPair[PlcfbtePapx].theULong);
	spew(mTableReader, mFIBPair[PlcfbtePapx].theLong, mFIBPair[PlcfbtePapx].theULong);

	printf("the Chpx bin table starts at %d, is %d long and looks like\n", mFIBPair[PlcfbteChpx].theLong, mFIBPair[PlcfbteChpx].theULong);
	spew(mTableReader, mFIBPair[PlcfbteChpx].theLong, mFIBPair[PlcfbteChpx].theULong);

*/
#endif	// DEBUG


	TTranslatorSheet* sheetPtr = new TTranslatorSheet();
	sheetPtr->SetMainPart( mMainPartPtr );
	if (mHeaderPartPtr)
		sheetPtr->SetHeaderPart( mHeaderPartPtr );
	if (mFooterPartPtr)
		sheetPtr->SetFooterPart( mFooterPartPtr );
	mTransDoc.AddSheet( sheetPtr );
	rtnCode = mTransDoc.Write( mWriter );
	
	printf("Outta here\n");
	return rtnCode; 
}


// FIB - this is the File Information Block, and is the first thing in the file
int32 TImportWORD::loadFIB(void)
{
	if ((size_t) mDocStreamSize < sizeof(FIBHeader))
	{
		IFDEBUG(fprintf(stderr, "WORD loadFIB:, ERROR - stream too small\n"));
		return B_ERROR;
	} 
		
	mFIBHeader.wIdent = mDocReader->ReadShort(); // 0
	mFIBHeader.nFib = mDocReader->ReadShort(); // 2
	
	// If less than Word97, give up.
	// May be able to change this if I can get ahold of the old formats
	// our WinWord97 is 193, all WinWord 6.0 is >= 101, and Tom's MacWord 6.0 is 104, 
	//if (mFIBHeader.nFib < 190)	
	if (mFIBHeader.nFib < 101)	
	{	
		IFDEBUG(fprintf(stderr, "WORD loadFIB:, ERROR - version %d too low\n", mFIBHeader.nFib));
		(new BAlert(kBadVersionStr, kUnsupportedStr, kOKStr, (char*) NULL, (char*) NULL,
							B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_STOP_ALERT))->Go();
		//int32 button_index = alert->Go();
		//return B_ERROR;
		return B_CANCEL;
	} 

	mFIBHeader.nProduct = mDocReader->ReadShort(); // 4
	mFIBHeader.lid = mDocReader->ReadShort(); // 6
	mFIBHeader.pnNext = mDocReader->ReadShort(); // 8
	mFIBHeader.bitField1 = mDocReader->ReadShort(); // 10
	mFIBHeader.nFibBack = mDocReader->ReadShort(); // 12
	mFIBHeader.lKey = mDocReader->ReadLong(); // 14
	mFIBHeader.envr = mDocReader->ReadChar(); // 18
	mFIBHeader.bitField2 = mDocReader->ReadChar(); // 19
	mFIBHeader.chs = mDocReader->ReadShort(); // 20
	mFIBHeader.chsTables = mDocReader->ReadShort(); // 22
	mFIBHeader.fcMin = mDocReader->ReadLong(); // 24
	mFIBHeader.fcMac = mDocReader->ReadLong(); // 28
	
	IFDEBUG(fprintf(stderr, "WORD loadFIB: %sCOMPLEX, ver %d, table%d, fcMin %d, FIBbitField1 x%04x\n", (mFIBHeader.bitField1 & fComplex) ? "" : "NON-", mFIBHeader.nFib, (mFIBHeader.bitField1 & fWhichTblStm) ? 1 : 0, mFIBHeader.fcMin, mFIBHeader.bitField1));

	//mEncoding = (mFIBHeader.chs == 0) ? B_MS_WINDOWS_CONVERSION : B_MAC_ROMAN_CONVERSION;
	mEncoding = B_MS_WINDOWS_CONVERSION;
	
	// For Word 8.0, the FIBHeader is followed by an array of shorts, an array of longs,
	// and an array of long pairs.
	if (mFIBHeader.nFib > 190)
	{
		short	tempShort;
		long	tempLong, i;
		ulong	tempULong;
		ushort	theCount;
		
		// array of shorts
		theCount = mDocReader->ReadShort();
		for (i = 0; i < theCount; i++)
		{
			tempShort = mDocReader->ReadShort();
			if (i < NUM_FIB_SHORTS)
				mFIBShort[i] = tempShort;
		}
		
		// array of longs
		theCount = mDocReader->ReadShort();
		for (i = 0; i < theCount; i++)
		{
			tempLong = mDocReader->ReadLong();
			if (i < NUM_FIB_LONGS)
				mFIBLong[i] = tempLong;
		}
		
		// array of pairs
		theCount = mDocReader->ReadShort();
		for (i = 0; i < theCount; i++)
		{
			tempLong = mDocReader->ReadLong();
			tempULong = mDocReader->ReadLong();
			if (i < NUM_FIB_PAIRS)
			{
				mFIBPair[i].theLong = tempLong;
				mFIBPair[i].theULong = tempULong;
			}
		}
	}
	else // assume this is a WRD 6 doc
	{
		mFIBLong[cbMac] = mDocReader->ReadLong(); // 32

		mDocReader->ReadLong(); // fcSpare0 36
		mDocReader->ReadLong(); // fcSpare1
		mDocReader->ReadLong(); // fcSpare2
		mDocReader->ReadLong(); // fcSpare3
		
		mFIBLong[ccpText] = mDocReader->ReadLong();
		mFIBLong[ccpFtn] = mDocReader->ReadLong();
		mFIBLong[ccpHdr] = mDocReader->ReadLong();
		mFIBLong[ccpMcr] = mDocReader->ReadLong();
		mFIBLong[ccpAtn] = mDocReader->ReadLong();
		mFIBLong[ccpEdn] = mDocReader->ReadLong();
		mFIBLong[ccpTxbx] = mDocReader->ReadLong();
		mFIBLong[ccpHdrTxbx] = mDocReader->ReadLong();
		
		mDocReader->ReadLong(); // ccpSpare2
		
		for (int i = StshfOrig; i < PlcdoaMom; i++)
		{
			mFIBPair[i].theLong = mDocReader->ReadLong();
			mFIBPair[i].theULong = mDocReader->ReadLong();
		}
		
		mDocReader->ReadLong(); // wSpare4Fib
		mFIBLong[pnChpFirst] = mDocReader->ReadLong();
		mFIBLong[pnPapFirst] = mDocReader->ReadLong();
		mFIBLong[cpnBteChp] = mDocReader->ReadLong();
		mFIBLong[cpnBtePap] = mDocReader->ReadLong();
		
		for (int i = PlcdoaMom; i < PlcfLst; i++)
		{
			mFIBPair[i].theLong = mDocReader->ReadLong();
			mFIBPair[i].theULong = mDocReader->ReadLong();
		}
		
	}
	
	return mDocReader->Error();
}

void TImportWORD::spewFIB(void) 
{
#ifdef DEBUG	
	fprintf(stderr, "mFIBPair[Clx].theLong = %d\n", mFIBPair[Clx].theLong); 
	fprintf(stderr, "mFIBPair[PlcfbteChpx].theLong = %d\n", mFIBPair[PlcfbteChpx].theLong); 
	fprintf(stderr, "mFIBPair[PlcfbtePapx].theLong = %d\n", mFIBPair[PlcfbtePapx].theLong); 
	
	fprintf(stderr, "mFIBHeader.wIdent = %#2hx;\n", mFIBHeader.wIdent);  // magic number 
	fprintf(stderr, "mFIBHeader.nFib = %#2hx;\n", mFIBHeader.nFib);      // FIB version - Word 6.0 and later will be >= 101 
	fprintf(stderr, "mFIBHeader.nProduct = %#2hx;\n", mFIBHeader.nProduct);      // product version written by 
	fprintf(stderr, "mFIBHeader.lid = %#2hx;\n", mFIBHeader.lid);                // language stamp - localized version 
	fprintf(stderr, "mFIBHeader.pnNext = %#2hx;\n", mFIBHeader.pnNext); 
	fprintf(stderr, "mFIBHeader.bitField1 = %#2hx;\n", mFIBHeader.bitField1); 
	fprintf(stderr, "mFIBHeader.nFibBack = %#2hx;\n", mFIBHeader.nFibBack);      // compatible with readers that understand nFib >= this value 
	fprintf(stderr, "mFIBHeader.lKey = %#2lx;\n", mFIBHeader.lKey);              // encryption key 
	fprintf(stderr, "mFIBHeader.envr = %#2x;\n", mFIBHeader.envr);               // 0 - Win Word, 1 - Mac Word 
	fprintf(stderr, "mFIBHeader.bitField2 = %#2x;\n", mFIBHeader.bitField2); 
	fprintf(stderr, "mFIBHeader.chs = %#2hx;\n", mFIBHeader.chs);                // default char set for text stream, 0 - Windows ANSI, 256 - Mac char set 
	fprintf(stderr, "mFIBHeader.chsTables = %#2hx;\n", mFIBHeader.chsTables);    // default char set for text in internal structures, 0 - Windows ANSI, 256 - Mac char set 
	fprintf(stderr, "mFIBHeader.fcMin = %#2lx;\n", mFIBHeader.fcMin);            // file offset of first char of text 
	fprintf(stderr, "mFIBHeader.fcMac = %#2lx;\n", mFIBHeader.fcMac);            // file offset of last character of text in doc text stream + 1 
	
	fprintf(stderr, "mFIBHeader.fcMin = %d\n", mFIBHeader.fcMin); 
	fprintf(stderr, "mFIBHeader.fcMac = %d\n", mFIBHeader.fcMac); 
	
	
	
	
	for (int i = 0; i < NUM_FIB_SHORTS; i++) 
	{ 
	        fprintf(stderr, "mFIBShort[%d] = %#2hx;\n", i, mFIBShort[i]); 
	} 
	for (int i = 0; i < NUM_FIB_LONGS; i++) 
	{ 
	        fprintf(stderr, "mFIBLong[%d] = %#2lx;\n", i, mFIBLong[i]); 
	} 
	for (int i = 0; i < NUM_FIB_PAIRS; i++) 
	{ 
	        fprintf(stderr, "mFIBPair[%d].theLong = %#2lx;\n", i, mFIBPair[i].theLong); 
	        fprintf(stderr, "mFIBPair[%d].theULong = %#2lx;\n", i, mFIBPair[i].theULong); 
	}
#endif // DEBUG
} 

void TImportWORD::spew(TOLEEntryReader* reader, int32 position, int32 length)
{
#ifdef DEBUG
	char *data = new char[length];
	reader->SetStreamPos(position);
	
	memset(data, 0, length);
	
	reader->ReadBytes(data, length);
	
	fprintf(stderr, "data = {");
	for (int i = 0; i < length; i++)
	{
		if (!(i%16))
			fprintf(stderr, "\n");
		fprintf(stderr, "%#2.2hx ", ((uchar *)data)[i]);
	}
	fprintf(stderr, "}\n");
	
	delete [] data;
#endif // DEBUG
}


// DOP - The document properties in the table stream
int32 TImportWORD::loadDocProperties(void)
{
	long	dop = mFIBPair[Dop].theLong;
	long	dopBytes = mFIBPair[Dop].theULong;
	
	if ((dop + dopBytes) > mTableStreamSize)
	{
		IFDEBUG(fprintf(stderr, "WORD loadDocProperties: ERROR: %d offset, %d offsetBytes, %d tableStreamSize\n", dop, dopBytes, mTableStreamSize));
		(new BAlert(kBadVersionStr, kBadDocPropsStr, kOKStr, (char*) NULL, (char*) NULL,
								B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_STOP_ALERT))->Go();
		return B_CANCEL;
//		return B_ERROR;
	}
	//IFDEBUG(fprintf(stderr, "readDocProperties  DOP bytes: %d\n", dopBytes));
	
	mTableReader->SetStreamPos(dop);
	
	// need to read in each field separately so that swapping will
	// happen, and we can ignore structure byte padding
	memset(&mDOP.bitField1, 0, sizeof(DOP));
	
	mDOP.bitField1 			= mTableReader->ReadShort();
	mDOP.bitField2 			= mTableReader->ReadShort();
	mDOP.bitField3 			= mTableReader->ReadShort();
	mDOP.bitField4 			= mTableReader->ReadShort();
	mDOP.bitField5 			= mTableReader->ReadShort();
	mDOP.dxaTab 			= mTableReader->ReadShort();
	mDOP.wSpare 			= mTableReader->ReadShort();
	mDOP.dxaHotZ 			= mTableReader->ReadShort();
	mDOP.cConsecHypLim 		= mTableReader->ReadShort();
	mDOP.wSpare2 			= mTableReader->ReadShort();
	mDOP.dttmCreated 		= mTableReader->ReadLong();
	mDOP.dttmRevised 		= mTableReader->ReadLong();
	mDOP.dttmLastPrint 		= mTableReader->ReadLong();
	mDOP.nRevision 			= mTableReader->ReadShort();
	mDOP.tmEdited 			= mTableReader->ReadLong();
	mDOP.cWords 			= mTableReader->ReadLong();
	mDOP.cCh 				= mTableReader->ReadLong();
	mDOP.cPg 				= mTableReader->ReadShort();
	mDOP.cParas 			= mTableReader->ReadLong();
	mDOP.bitField6 			= mTableReader->ReadShort();
	mDOP.bitField7 			= mTableReader->ReadShort();
	mDOP.cLines 			= mTableReader->ReadLong();
	mDOP.cWordsFtnEdn 		= mTableReader->ReadLong();
	mDOP.cChFtnEdn 			= mTableReader->ReadLong();
	mDOP.cPgFtnEdn 			= mTableReader->ReadShort();
	mDOP.cParasFtnEdn 		= mTableReader->ReadLong();
	mDOP.cLinesFtnEdn 		= mTableReader->ReadLong();
	mDOP.lKeyProtDoc 		= mTableReader->ReadLong();	
	mDOP.bitField8 			= mTableReader->ReadShort();
	
	if (mFIBHeader.nFib >= 103)
	{
		mDOP.bitField9 		= mTableReader->ReadLong();
	}
	
	if (mFIBHeader.nFib > 105)
	{
		mDOP.adt 				= mTableReader->ReadShort();
		mTableReader->ReadBytes(&mDOP.doptypography[0], 310);	// not swapped
		mTableReader->ReadBytes(&mDOP.dogrid[0], 10);			// not swapped
		mDOP.bitField10 			= mTableReader->ReadShort();
		mDOP.bitField11 			= mTableReader->ReadShort();
		mTableReader->ReadBytes(&mDOP.asumyi[0], 12);			// not swapped
		mDOP.cChWS 				= mTableReader->ReadLong();
		mDOP.cChWSFtnEdn 		= mTableReader->ReadLong();
		mDOP.grfDocEvents 		= mTableReader->ReadLong();
		mDOP.bitField12 			= mTableReader->ReadLong();
		mTableReader->ReadBytes(&mDOP.spare[0], 30);			// not swapped, but not needed
		mDOP.reserved1 			= mTableReader->ReadLong();
		mDOP.reserved2 			= mTableReader->ReadLong();
		mDOP.cDBC 				= mTableReader->ReadLong();
		mDOP.cDBCFtnEdn 		= mTableReader->ReadLong();
		mDOP.reserved3 			= mTableReader->ReadLong();
		mDOP.nfcFtnRef 			= mTableReader->ReadShort();
		mDOP.nfcEdnRef 			= mTableReader->ReadShort();
		mDOP.hpsZoonFontPag 	= mTableReader->ReadShort();
		mDOP.dywDispPag 		= mTableReader->ReadShort();
	}
	
	return B_OK;
}

int32 TImportWORD::loadPieceTable(void)
{
	long	clx = mFIBPair[Clx].theLong;
	long	clxBytes = mFIBPair[Clx].theULong;
	IFDEBUG(fprintf(stderr, "WORD loadPieceTable: %d offset, %d offsetBytes\n", clx, clxBytes));
	
	if ((clx + clxBytes) > mTableStreamSize)
	{
		IFDEBUG(fprintf(stderr, "WORD loadPieceTable: ERROR: %d offset, %d offsetBytes, %d tableStreamSize\n", clx, clxBytes, mTableStreamSize));
		return B_ERROR;
	}
	
	if (clxBytes == 0)
	{
		mPieceTblEntries = 1;
		
		mPieceTbl = new PieceTbl[mPieceTblEntries];
		
		mPieceTbl[0].cpMax =  mFIBLong[ccpText] + mFIBLong[ccpFtn] + mFIBLong[ccpHdr] + mFIBLong[ccpMcr] +
								mFIBLong[ccpAtn] + mFIBLong[ccpEdn] + mFIBLong[ccpTxbx] + mFIBLong[ccpHdrTxbx];
		
		mPieceTbl[0].numChars = mPieceTbl[0].cpMax;
	
		mPieceTbl[0].fcStart = mFIBHeader.fcMin;
		mPieceTbl[0].prm = 0x30;
		mPieceTbl[0].isUNICODE = false;
		
		return B_OK;

	}
	
	
	mTableReader->SetStreamPos(clx);
	
	
	short	byteCount;
	ushort	theSprm;
	long	streamPos;	
	char *	grpprl;
	char 	clxType = mTableReader->ReadChar();

	// These are grpprl sprms for the PRM below, so save them in a table
	while (clxType == 0x01 && mTableReader->Error() == B_OK)
	{
		byteCount = mTableReader->ReadShort();
		streamPos = mTableReader->GetStreamPos();

		grpprl = new char[byteCount];
		// use ReadSPRM so we'll get some swapping
		ReadSPRM(mTableReader, grpprl, theSprm, mFIBHeader.nFib); 
		mCLXsprms.AddItem(grpprl);
		mCLXsprmBytes.AddItem((char *)byteCount);
		
		// just in case the sprm reader screws up, set the position
		mTableReader->SetStreamPos(streamPos + byteCount);
		
		// read the next clx type
		clxType = mTableReader->ReadChar();
	}
	
	if (clxType != 0x02)
	{
		IFDEBUG(fprintf(stderr, "WORD loadPieceTable: ERROR unknown clxType: %2x\n", clxType));
		return B_ERROR; 
	}
		
	long	pcdByteCount = mTableReader->ReadLong();
	
	if (mTableReader->Error() < 0)
		return mTableReader->Error();
	
	mPieceTblEntries = (pcdByteCount - 4) / (4 + SIZEOF_PCD);
	//IFDEBUG(fprintf(stderr, "WORD loadPieceTable: %d entries\n", mPieceTblEntries));
	
	// We are going to save the piece table in our own memory structure,
	// which will be the PCD plus the CP for that PCD
	
	mPieceTbl = new PieceTbl[mPieceTblEntries];
	if (!mPieceTbl)
	{
		IFDEBUG(fprintf(stderr, "WORD loadPieceTable: ERROR no memory for piecetbl\n"));
		return B_NO_MEMORY;
	}
	
	// Read in the array of CPs.  In the pclfpcd, number of CPs is mPieceTblEntries + 1.
	// The first CP will be 0, so we can skip it.
	mTableReader->ReadLong();
	long	tempLong;
	int16	tempShort;
	for (long i = 0; i < mPieceTblEntries; i++)
	{
		mPieceTbl[i].cpMax = tempLong = mTableReader->ReadLong();		
		mPieceTbl[i].numChars = mPieceTbl[i].cpMax - ((i == 0) ? 0 : mPieceTbl[i - 1].cpMax);
	}

	// Now read the array of PCDs into our piece tbl
	for (long i = 0; i < mPieceTblEntries; i++)
	{	
		tempShort = mTableReader->ReadShort();	// skip pcd.bitField1
		mPieceTbl[i].fcStart = mTableReader->ReadLong();
		mPieceTbl[i].prm = mTableReader->ReadShort();
		
		if (mFIBHeader.nFib > 190)
		{
			if (mPieceTbl[i].fcStart & 0x40000000)
			{
				// it's single-byte CP1254, so mask out 2nd MSB and divide by two to get FC
				mPieceTbl[i].isUNICODE = false;
				mPieceTbl[i].fcStart &= 0xBFFFFFFF;
				mPieceTbl[i].fcStart /= 2;
			}
			else
				mPieceTbl[i].isUNICODE = true;
		}
		else // word 6
			mPieceTbl[i].isUNICODE = false;
			
			
		//IFDEBUG(fprintf(stderr, "WORD loadPieceTbl[%d].fcStart == %d, cpMax %d, numChars %d, unicode: %d\n", i, mPieceTbl[i].fcStart, mPieceTbl[i].cpMax, mPieceTbl[i].numChars, mPieceTbl[i].isUNICODE));
	}
	
	return mTableReader->Error();
}


int32 TImportWORD::loadAllNoteCPs()
{
	mInNote = false;
	
	long 	txtPos, txtBytes, refPos, refBytes;
	int32	ccpNote, plcfNoteTxt, plcfNoteRef;
	
	for (long i = 0; i < N_TYPE_COUNT; i++)
	{
		mNoteRefEntries[i] = 0;
		mNoteTxtEntries[i] = 0;
		
		switch (i)
		{
			case N_FOOTNOTE:	plcfNoteRef = PlcffndRef;	plcfNoteTxt = PlcffndTxt;	ccpNote = ccpFtn;	break;
			case N_ANNOTATION:	plcfNoteRef = PlcfandRef;	plcfNoteTxt = PlcfandTxt;	ccpNote = ccpAtn;	break;
			case N_ENDNOTE:		plcfNoteRef = PlcfednRef;	plcfNoteTxt = PlcfednTxt;	ccpNote = ccpEdn;	break;
			default:
				IFDEBUG(fprintf(stderr, "WORD loadAllNoteCPs[%d]: ERROR unknown note type\n", i));
				return B_ERROR;
		}
		
		txtPos = mFIBPair[plcfNoteTxt].theLong;
		txtBytes = mFIBPair[plcfNoteTxt].theULong;
		
		refPos = mFIBPair[plcfNoteRef].theLong;
		refBytes = mFIBPair[plcfNoteRef].theULong;
	
		if (!mFIBLong[ccpNote] || !txtBytes || !refBytes)
			continue;		// skip, no text of this note type
	
		if ((txtPos + txtBytes) > mTableStreamSize || (refPos + refBytes) > mTableStreamSize)
		{
			IFDEBUG(fprintf(stderr, "WORD loadAllNoteCPs[%d]: ERROR: %d-%d offset, %d-%d offsetBytes, %d tableStreamSize\n", i, txtPos, refPos, txtBytes, refBytes, mTableStreamSize));
			return B_ERROR;
		}
		
#ifdef DEBUG
		printf("txtPos = %d\n", txtPos);
		printf("txtBytes = %d\n", txtBytes);
		printf("refPos = %d\n", refPos);
		printf("refBytes = %d\n", refBytes);

		long tempStreamPos = mTableReader->GetStreamPos();

// 	txtPos	
		mTableReader->SetStreamPos(txtPos);
		
		char *byteArr = new char[txtBytes];
		mTableReader->ReadBytes(byteArr, txtBytes);
		
		fprintf(stderr, "bytes in txtPos");
		for (int i = 0; i < txtBytes; i++)
		{
			if (!(i%16))
				fprintf(stderr, "\n");
			fprintf(stderr, "%#2.2hx ", (uchar) byteArr[i]);
		}
		fprintf(stderr, "}\n");
// refPos
		delete [] byteArr;
		mTableReader->SetStreamPos(refPos);
		
		byteArr = new char[refBytes];
		mTableReader->ReadBytes(byteArr, refBytes);
		
		fprintf(stderr, "bytes in refPos");
		for (int i = 0; i < refBytes; i++)
		{
			if (!(i%16))
				fprintf(stderr, "\n");
			fprintf(stderr, "%#2.2hx ", (uchar) byteArr[i]);
		}
		fprintf(stderr, "}\n");
		delete [] byteArr;
		
		mTableReader->SetStreamPos(tempStreamPos);
	#endif // DEBUG
		mNoteTxtEntries[i] = txtBytes / 4;
		mNoteRefEntries[i] = mNoteTxtEntries[i] - 1;
		//IFDEBUG(fprintf(stderr, "WORD loadAllNoteCPs[%d]: %d txt entries, %d offset, %d offsetBytes\n", i, mNoteTxtEntries[i], txtPos, txtBytes));
	
		// Read in the CPs of plcfNoteRef, which are actual CPs in the text stream
		mTableReader->SetStreamPos(refPos);
		mNoteRefCP[i] = new long[mNoteRefEntries[i]];
		for (long j = 0; j < mNoteRefEntries[i]; j++)
		{
			mNoteRefCP[i][j] = mTableReader->ReadLong();
			//IFDEBUG(fprintf(stderr, "\mNoteRefCP[%d][%d]: %d\n", i, j, mNoteRefCP[i][j]));
		}
	
		// Read in the CPs of plcfNoteTxt, which are relative to the type stream
		mTableReader->SetStreamPos(txtPos);
		mNoteTxtCP[i] = new long[mNoteTxtEntries[i]];
		for (long j = 0; j < mNoteTxtEntries[i]; j++)
		{
			mNoteTxtCP[i][j] = mTableReader->ReadLong();
			//IFDEBUG(fprintf(stderr, "\mNoteTxtCP[%d][%d]: %d\n", i, j, mNoteTxtCP[i][j]));
		}
	}
	
	return mTableReader->Error();
}


int32 TImportWORD::loadFileShapes()
{
	long	fspaPos = mFIBPair[PlcspaMom].theLong;
	long	fspaBytes = mFIBPair[PlcspaMom].theULong;
	
	if ((fspaPos + fspaBytes) > mTableStreamSize)
	{
		IFDEBUG(fprintf(stderr, "WORD loadFileShapes: ERROR: %d offset, %d offsetBytes, %d tableStreamSize\n", fspaPos, fspaBytes, mTableStreamSize));
		return B_ERROR;
	}
	
	mFspaTblEntries = 0;
	
	if (fspaPos == 0 || fspaBytes == 0)
		return B_OK;	// no file shapes
	
	mFspaTblEntries = (fspaBytes - 4) / (SIZEOF_FSPA + 4);
	
	IFDEBUG(fprintf(stderr, "WORD loadFileShapes: plcspaMom(bytes-offset) %d-%d, fcDggInfo %d-%d\n", mFIBPair[PlcspaMom].theLong, mFIBPair[PlcspaMom].theULong, mFIBPair[DggInfo].theLong, mFIBPair[DggInfo].theULong));
	
	mTableReader->SetStreamPos(fspaPos);

	// We are going to save the FSPA table in our own memory structure,
	// which will be the FSPA plus the CP
	
	mFspaTbl = new FspaTbl[mFspaTblEntries];
	if (!mFspaTbl)
	{
		IFDEBUG(fprintf(stderr, "WORD loadFileShapes: ERROR no memory for FspaTbl\n"));
		return B_NO_MEMORY; 
	}
	
	// Read in the array of CPs.  We want the first one, and can skip the last one.
	for (long i = 0; i < mFspaTblEntries; i++)
		mFspaTbl[i].cp = mTableReader->ReadLong();
				
	mTableReader->ReadLong();	// skip the last CP		

	// Now read the array of FSPAs into our fspa tbl
	for (long i = 0; i < mFspaTblEntries; i++)
	{
		// FSPA structure	
		mFspaTbl[i].fspa.spid = mTableReader->ReadLong();
		mFspaTbl[i].fspa.xaLeft = mTableReader->ReadLong();
		mFspaTbl[i].fspa.yaTop = mTableReader->ReadLong();
		mFspaTbl[i].fspa.xaRight = mTableReader->ReadLong();
		mFspaTbl[i].fspa.yaBottom = mTableReader->ReadLong();
		mFspaTbl[i].fspa.bitField1 = mTableReader->ReadShort();
		mFspaTbl[i].fspa.cTxbx = mTableReader->ReadLong();
			
		IFDEBUG(fprintf(stderr, "WORD loadFspaTbl[%d].cp == %d, spid %d\n", i, mFspaTbl[i].cp, mFspaTbl[i].fspa.spid));
	}

	// BOGUS - I can't do the picture until I get a spec on the office art object table,
	// which is in DggInfo and must contain the offset to the image data.	
#if 0
	// Debug code to dump out the DGG to a file
	long	dggPos = mFIBPair[DggInfo].theLong;
	long	dggBytes = mFIBPair[DggInfo].theULong;
	mTableReader->SetStreamPos(dggPos);
	char *theData = new char[dggBytes];
	mTableReader->ReadBytes(theData, dggBytes);
	
	BDirectory	theDir("/boot/home");
	BFile		theFile;
	theDir.CreateFile("w97_dgg", &theFile);
	theFile.Write(theData, dggBytes);

	delete [] theData;
#endif	// 0	
	
	return mTableReader->Error();
}


// Process the text in the range cpStart-cpEnd by getting it from the piece table
// and building B_MIME_DATA messages of it.  The buffers passed in are used to hold
// the text as it's being read in.
int32 TImportWORD::processText(long cpStart, long cpEnd, char *& buffer, int32& bufferMaxSize, char *& bufferUTF8, int32& bufferUTF8MaxSize)
{
	int32 rtnCode = B_OK;

	if (cpStart >= cpEnd)
		return B_OK;		// no text for this type
			
	mTransDoc.SendTaskProgress(0);

	// Start parsing the text, one piece at a time
	long	pieceBytes, indexStart;
	int32	bufferUTF8Size;
	
	// Find the piece that this text begins in
	int prevEntryStart = 0;
	for (indexStart = 0; indexStart < mPieceTblEntries; indexStart++) // -BOGUS is this right? -joel
	{
		/*if (cpStart < mPieceTbl[indexStart].cpMax)
			break;*/
		
		if (cpStart >= prevEntryStart && cpStart < mPieceTbl[indexStart].cpMax)
			break;
		
		prevEntryStart =  mPieceTbl[indexStart].cpMax;
	}
	if (indexStart >= mPieceTblEntries)
	{
		IFDEBUG(fprintf(stderr, "WORD processText: ERROR - piece table entry not found, cpStart %d\n", cpStart));
		return B_ERROR;
	}
	
	long cpCurr = mPieceTbl[indexStart].cpMax - mPieceTbl[indexStart].numChars;
	long fcCurr = mPieceTbl[indexStart].fcStart;
	
	// Loop thru the piece table and read each char in each piece.
	for (long i = indexStart; i < mPieceTblEntries && cpCurr < cpEnd; i++)
	{
		cpCurr = mPieceTbl[i].cpMax - mPieceTbl[i].numChars;
		fcCurr = mPieceTbl[i].fcStart;		

		// read in the entire piece
		mDocReader->SetStreamPos(mPieceTbl[i].fcStart);
		
		// make sure buffer has enough room for this piece
		pieceBytes = (mPieceTbl[i].isUNICODE) ? (2 * mPieceTbl[i].numChars) : mPieceTbl[i].numChars; 
		if (pieceBytes >= bufferMaxSize)
		{
			delete [] buffer;
			bufferMaxSize = pieceBytes + 1;
			buffer = new char[bufferMaxSize];
			if (!buffer)
			{
				IFDEBUG(fprintf(stderr, "WORD processText: ERROR - out of memory parsing text\n"));
				return B_NO_MEMORY;
			}
		}
		
		// read in this piece
		if (mPieceTbl[i].isUNICODE)
		{
			// need to swap in unicode chars
			long	numShorts = pieceBytes / 2;
			short	tempShort;
			for (long i = 0; i < numShorts; i++)
			{
				mDocReader->ReadBytes((char*)&tempShort, sizeof(short));
				tempShort = B_SWAP_INT16(tempShort);
				memmove(&buffer[i * 2], &tempShort, 2);
			}
		}
		else
			mDocReader->ReadBytes(buffer, pieceBytes);
		
		if (mDocReader->Error() < 0)
			return mDocReader->Error();

		// convert piece to utf8
		if (mPieceTbl[i].isUNICODE)
			convertBufferToUTF8(buffer, pieceBytes, bufferUTF8, bufferUTF8Size, bufferUTF8MaxSize, B_UNICODE_CONVERSION);
		else
			convertBufferToUTF8(buffer, pieceBytes, bufferUTF8, bufferUTF8Size, bufferUTF8MaxSize, mEncoding);

		char *ptr = bufferUTF8;	
		
		// make sure we're starting at the right spot in the piece
		while (cpStart > cpCurr)
		{
			ptr += CHAR_BYTES_UTF8(*ptr);			
			cpCurr++;
			fcCurr += ((mPieceTbl[i].isUNICODE) ? 2 : 1);
		}

		char *ptrStart = ptr;
		char cancelChecker = 0;
		int32 sendTaskProgressModValue = (cpEnd - cpStart) / 40;
		
		if(sendTaskProgressModValue == 0)
			sendTaskProgressModValue = 1;
		
		// look at each character, and check the styles for each
		while (cpCurr < cpEnd && cpCurr < mPieceTbl[i].cpMax)
		{				
			cancelChecker++;
			cancelChecker %= 1000;
			if (!cancelChecker)
			{
				mDocReader->SetStreamPos(0);
				mDocReader->ReadChar();
				if (mDocReader->Error() < 0) // see if the import has been canceled while we are proccessing
					return mDocReader->Error();
			}
			
			// send status
			if (!cpCurr % sendTaskProgressModValue)
				mTransDoc.SendTaskProgress((float)(cpCurr - cpStart) / (float)(cpEnd - cpStart));
			// if para or char styles have changed for this char,
			// send off anything currently in buffer
			
			checkStyles(fcCurr, mPieceTbl[i].prm);
			if (mStyleChanged[S_CHAR] || mStyleChanged[S_PARA])
			{
				if (ptr > ptrStart && !mInField)
				{
					sendMimeDataMsg(ptrStart, ptr - ptrStart);
					ptrStart = ptr;

				}
				makeNewStylesCurr();
			}
			
			if (mCurrFilterStyle[S_PARA]->IsInTable() && !mInTable)
			{
				mTPtrStore = mPartPtr;
				mTCharPosStore = mLogCharPos;
				mLogCharPos = 0;
				mPartPtr = new TTranslatorPart_WP(&mTransDoc);
				TTableCell* newCell = new TTableCell();
				newCell->SetText(mPartPtr);
				mTCellList.AddItem(newCell);

				mInTable = true;
			}
			else
			{			
				if (mInTable && !(mCurrFilterStyle[S_PARA]->IsInTable()))
				{
					sendTable();
				}
			}
			
			if (mCurrFilterStyle[S_PARA]->GetCellsInRow())
			{
				processRow();
			}
			
			if (mCurrFilterStyle[S_CHAR]->IsSpecialChar())
			{
				// It's a WORD special char, so convert to Squirrel format
				// send off anything else in buffer
				if (ptr > ptrStart && !mInField)
				{
					sendMimeDataMsg(ptrStart, ptr - ptrStart);
					ptrStart = ptr;
				}
				
				switch (*ptr)
				{
					case 0x00:	// current page number
//						sendMsgID(msg_WP_INSERT_PAGENO);
						break;
												
					case 0x02:	// autonumbered footnote and endnote references
						mTransDoc.BeginTask(kInsertFootnoteStr, (float)(cpCurr - cpStart) / (float)(cpEnd - cpStart),
										/* stop the status bar	*/	  (float)(cpCurr - cpStart) / (float)(cpEnd - cpStart)); 
						if ((rtnCode = insertNote(cpCurr)) < 0)
							return rtnCode;
						mTransDoc.EndTask();
						break;
						
					case 0x05:	// annotation reference
						mTransDoc.BeginTask(kInsertAnnotationStr, (float)(cpCurr - cpStart) / (float)(cpEnd - cpStart),
										/* stop the status bar	*/	  (float)(cpCurr - cpStart) / (float)(cpEnd - cpStart)); 
							if ((rtnCode = insertNote(cpCurr)) < 0)
							return rtnCode;
						mTransDoc.EndTask();
						break;
						
					case 0x08:	// drawn object
						printf("there is a picture at %x", mCurrFilterStyle[S_CHAR]->GetPicFC());
						IFDEBUG(fprintf(stderr, "WORD processText: drawn object, cpCurr %d\n", cpCurr));
						break;
						
					case 0x0a:	// abbreviated date (Wed, Dec 1, 1993)
					case 0x0e:	// abbreviated day of week (Thu for Thursday)	
					case 0x0f:	// day of week (Thursday)
					case 0x10:	// day short (9 for May 9)
					case 0x1d:	// Date M (December 2, 1993)
					case 0x1e:	// Short Date (12/2/93)
					case 0x21:	// short month (12 for December)
					case 0x22:	// long year (1993)
					case 0x23:	// short year (93)
					case 0x24:	// abbreviated month (Dec for December)
					case 0x25:	// long month (December)
					case 0x27:	// long date (Thursday, December 2, 1993)
						IFDEBUG(fprintf(stderr, "WORD processText: FIX FORMAT, date special character x%x\n", *ptr));
//						sendMsgID(msg_WP_INSERT_DATE, "bogusDateFormat");
						break;
						
					case 0x0b:	// time in hours:minutes:seconds
					case 0x16:	// hour of current time (no leading zero)
					case 0x17:	// hour of current time (always two digits)
					case 0x18:	// minute of current time (no leading zero)
					case 0x19:	// minute of current time (always two digits)
					case 0x1a:	// seconds of current time
					case 0x1b:	// AM/PM for current time
					case 0x1c:	// current time in hours:minutes:seconds in old format
					case 0x26:	// current time in hours:minutes (2:01)
						IFDEBUG(fprintf(stderr, "WORD processText: FIX FORMAT, time special character x%x\n", *ptr));
//						sendMsgID(msg_WP_INSERT_TIME, "bogusTimeFormat");
						break;

						
					case 0x13:		// field start				
						mInField = mProcessingField = true;
						mFieldDataSize = 0;
						break;						
					case 0x14:		// field separator
						if(processField())
							mProcessingField = false; //  we need to make sure we are no longer in the field -Joel
						else
							mInField = mProcessingField = false;
						break;

					case 0x15:		// field end (no need to do anything). wrong! what if ther is no field seperator?
						if (mProcessingField)
							processField();
						mInField = mProcessingField = false; //  we need to make sure we are no longer in the field -Joel
						break;

					case 0x01:	// picture
					case 0x03:	// footnote separator character
					case 0x04:	// footnote continuation character
					case 0x06:	// line number
					case 0x07:	// hand annotation picture (generated in pen windows)
					case 0x0c:	// current section number
					case 0x29:	// print merge helper field
						IFDEBUG(fprintf(stderr, "WORD processText: unhandled special character x%x\n", *ptr));
						break;
						
					default:
						IFDEBUG(fprintf(stderr, "WORD Go: unknown special character x%x\n", *ptr));
						break;
				}
				
				//if (!mProcessingField)
				ptrStart = ptr + CHAR_BYTES_UTF8(*ptr);	// skip past this special char
			}
			else
			{
				if (*ptr >= 0x00 && *ptr < 0x20)
				{
					// More WORD special chars (although not SPECIAL special like above)
					switch (*ptr)
					{
						case 0x07:		// cell or row mark
						{
							if (mCurrFilterStyle[S_PARA]->IsInTable() && mInTable)
							{
								// send off anything that is still in the buffer
								if (ptr > ptrStart && !mInField)
								{
									sendMimeDataMsg(ptrStart, ptr - ptrStart);
									ptrStart = ptr;
								}
								
								mPartPtr = new TTranslatorPart_WP(&mTransDoc);
								mLogCharPos = 0;
								TTableCell* newCell = new TTableCell();
								newCell->SetText(mPartPtr);
								mTCellList.AddItem(newCell);
							}
							
							ptrStart = ptr + 1; // skip past this char
							/*if (mCurrFilterStyle[S_PARA]->IsInTable() && mCurrFilterStyle[S_PARA]->IsEndTableRow())
								*ptr = X_PARBREAK;	// row mark
							else
								*ptr = X_TAB;		// cell
							break;*/
							
							/*if (mCurrFilterStyle[S_PARA]->IsInTable())
							{
								if (mCurrFilterStyle[S_PARA]->IsEndTableRow())
									*ptr = X_PARBREAK;	// row mark
								else
									*ptr = X_TAB;		// cell
							}*/
							break;
						}	
						case 0x0d:		// CR (normal paragraph mark)
							*ptr = X_PARBREAK;	// convert to LF
							break;
						case 0x0e:		// Column Break
							//*ptr = X_FRAMEBREAK;
							*ptr = X_PAGEBREAK; // -BOGUS how do I pass a FRAMEBREAK through nowadays?
							break;
						
						case 0x09:		// tab
							*ptr = X_TAB;
							break;
						
						case 0x0b:		// hard line break
							*ptr = X_LINEBREAK;
							break;
								
						case 0x0c:		// Page or Section Break
							*ptr = X_PAGEBREAK;
							break;
//						{
//							// was doing this, but scott says i can just change the char
//							// send off anything up to this char
//							if (ptr > ptrStart)
//								sendMimeDataMsg(ptrStart, ptr - ptrStart);
//								
//							ptrStart = ptr + 1;	// skip page break char
//							
//							BMessage* msg = new BMessage(msg_WP_INSERT_PAGEBREAK);
//							msg->AddBool("NoUndo", true);
//							mOutMsgStream->AddMessage(msg);	
//							break;
//						}
						
						case 0x1e:		// non-breaking hyphen
						case 0x1f:		// non-required hyphen
							*ptr = 0x2d;		// regular hyphen
							break;
									
						default:
							IFDEBUG(fprintf(stderr, "WORD processText: convert low-ascii x%x to space\n", *ptr));
							*ptr = X_SPACE;
					}
				}
			}
			
			if(mProcessingField)
			{
				if (mFieldDataSize + CHAR_BYTES_UTF8(*ptr) < MAX_FIELD_DATA_SIZE)
				{
					memcpy(&mFieldData[mFieldDataSize], ptr, CHAR_BYTES_UTF8(*ptr));
					mFieldDataSize += CHAR_BYTES_UTF8(*ptr);
				}
			}
			ptr += CHAR_BYTES_UTF8(*ptr);
			
			cpCurr++;
			fcCurr += ((mPieceTbl[i].isUNICODE) ? 2 : 1);
		}
		
		// End of Piece:  send off anything left in the buffer
		if (ptr > ptrStart && !mInField)
			sendMimeDataMsg(ptrStart, ptr - ptrStart);
		
	}
	if (mInTable)
		sendTable();
	
	return B_OK;
}


int32 TImportWORD::processHeaders()
{
	long	hddPos = mFIBPair[Plcfhdd].theLong;
	long	hddBytes = mFIBPair[Plcfhdd].theULong;
	
	if (!mFIBLong[ccpHdr] || !hddBytes)
		return B_OK;	// no header text
	
	if ((hddPos + hddBytes) > mTableStreamSize)
	{
		IFDEBUG(fprintf(stderr, "WORD processHeaders: ERROR: %d offset, %d offsetBytes, %d tableStreamSize\n", hddPos, hddBytes, mTableStreamSize));
		return B_ERROR;
	}
	
	long hddEntries = hddBytes / 4;
	//IFDEBUG(fprintf(stderr, "WORD processHeaders: %d entries, %d offset, %d offsetBytes\n", hddEntries, hddPos, hddBytes));
	
	// Read in the plcfHdd, which is a CP table of where headers
	// are located in the text stream.
	mTableReader->SetStreamPos(hddPos);
	long *	cpHdd = new long[hddEntries];
	for (long i = 0; i < hddEntries; i++)
	{
		cpHdd[i] = mTableReader->ReadLong();
		//IFDEBUG(fprintf(stderr, "\tcpHdd[%i]: %d\n", i, cpHdd[i]));
	}
	
	if (mTableReader->Error() < 0)
		return mTableReader->Error();
		
#ifdef DEBUG
	fprintf(stderr, "cpHdd = {");
	for (int i = 0; i < hddEntries; i++)
	{
		fprintf(stderr, "%#4.4hx ", cpHdd[i]);
	}
	fprintf(stderr, "}\n");
#endif // DEBUG

	// Figure out the cp where the first header starts
	long 	cpStart = mFIBLong[ccpText] + mFIBLong[ccpFtn];
	int32	rtnCode;
	
	// Check for a regular (odd page) header
	if ((hddEntries > (hddHeaderOdd + 1)) && (cpHdd[hddHeaderOdd + 1] > cpHdd[hddHeaderOdd]))
	{
		//IFDEBUG(fprintf(stderr, "\todd header[%d]: %d %d\n", hddHeaderOdd, cpHdd[hddHeaderOdd], cpHdd[hddHeaderOdd + 1]));
		mTransDoc.BeginTask(kHeaderStr, 0, .5);
		mHeaderPartPtr = new TTranslatorPart_WP(&mTransDoc);
		mPartPtr = mHeaderPartPtr;
		long old_mLogCharPos = mLogCharPos;
		mLogCharPos = 0;

		// Use the main buffers here since text is done with them at this point
			//subtract 1 to CPend because cpHdd[hddFooterOdd + 1] is the start of the next header, not the end of the main text
			//subtract another because we don't want to import the last CR word saves out
		if (rtnCode = processText(cpStart + cpHdd[hddHeaderOdd], cpStart + cpHdd[hddHeaderOdd + 1] - 2, mBuffer, mBufferMaxSize, mBufferUTF8, mBufferUTF8MaxSize), rtnCode)
		{
			delete [] cpHdd;
			return rtnCode;
		}
		mLogCharPos = old_mLogCharPos;
		mPartPtr = mMainPartPtr;
		mTransDoc.EndTask();
	} 
	
	// Check for a regular (odd page) footer
	if ((hddEntries > (hddFooterOdd + 1)) && (cpHdd[hddFooterOdd + 1] > cpHdd[hddFooterOdd]))
	{
		//IFDEBUG(fprintf(stderr, "\todd footer[%d]: %d %d\n", hddFooterOdd, cpHdd[hddFooterOdd], cpHdd[hddFooterOdd + 1]));
		mTransDoc.BeginTask(kFooterStr, .5, 1);
		mFooterPartPtr = new TTranslatorPart_WP(&mTransDoc);
		mPartPtr = mFooterPartPtr;
		long old_mLogCharPos = mLogCharPos;
		mLogCharPos = 0;
			//subtract 1 to CPend because cpHdd[hddFooterOdd + 1] is the start of the next header, not the end of the main text
			//subtract another because we don't want to import the last CR word saves out
		if (rtnCode = processText(cpStart + cpHdd[hddFooterOdd], cpStart + cpHdd[hddFooterOdd + 1] - 2, mBuffer, mBufferMaxSize, mBufferUTF8, mBufferUTF8MaxSize), rtnCode)
		{
			delete [] cpHdd;
			return rtnCode;
		}
		mLogCharPos = old_mLogCharPos;
		mPartPtr = mMainPartPtr;
		mTransDoc.EndTask();
	} 

	delete [] cpHdd;	
	return B_OK;
}

bool TImportWORD::processField(void)
{
	bool formFound = false;
	
	mFieldData[MAX_FIELD_DATA_SIZE - 1] = 0;
	if (strstr(mFieldData, "PAGE"))
 	{
		TSpecialCharacter_PageNumber* pageNumber = new TSpecialCharacter_PageNumber();
		mPartPtr->AddSpecialCharacter(pageNumber);
		mLogCharPos++;
		formFound = true;
	}
	
	return formFound;
}

// Insert a footnote for this particular index
int32 TImportWORD::insertNote(long cpCurr)
{	
	if (mInNote)
	{
		IFDEBUG(fprintf(stderr, "WORD insertNote: ERROR: note contains a note at CP %d\n", cpCurr));
		return B_ERROR;
	}

	// figure out what type of note this is by looking at the CPs for all the note starts
	// and finding the one that matches
	long	noteType, noteEntry;
	bool	foundIt = false;
	for (noteType = 0; noteType < N_TYPE_COUNT; noteType++)
	{
		for (noteEntry = 0; noteEntry < mNoteRefEntries[noteType]; noteEntry++)
		{
			if (mNoteRefCP[noteType][noteEntry] == cpCurr)
			{
				foundIt = true;
				break;
			}
		} 
		
		if (foundIt)
			break;
	}
	
	if (!foundIt)
	{
		IFDEBUG(fprintf(stderr, "WORD insertNote: ERROR: note not found at CP %d\n", cpCurr));
		return B_ERROR;
	}
		
	mInNote = true;
		
	// Figure out the cp where this particular note starts
	// It looks like footnotes start with the special footnote char and then a space, so skip first two
	// They also always end with a /r, so subtract one
	long noteStart = mFIBLong[ccpText];
	if (noteType != N_FOOTNOTE)
	{
		noteStart += (mFIBLong[ccpFtn] + mFIBLong[ccpHdr]);
		if (noteType == N_ENDNOTE)
			noteStart += mFIBLong[ccpAtn];
	}
		
	long cpStart = noteStart + mNoteTxtCP[noteType][noteEntry] + 2;
	long cpEnd = noteStart + mNoteTxtCP[noteType][noteEntry + 1] - 1;
	
	//IFDEBUG(fprintf(stderr, "WORD insertNote[%d]: index: %d, %d - %d\n", noteType, noteEntry, cpStart, cpEnd));
	
	TTranslatorPart_WP*	orig_part = mPartPtr;
	long orig_pos = mLogCharPos;
	
	mPartPtr = new TTranslatorPart_WP(&mTransDoc);
	mLogCharPos = 0;
	
	int32 rtnCode = processText(cpStart, cpEnd, mAltBuffer, mAltBufferMaxSize, mAltBufferUTF8, mAltBufferUTF8MaxSize);
	if (rtnCode < 0)
		return rtnCode;

	TSpecialCharacter_Footnote* footnote = new TSpecialCharacter_Footnote();
	footnote->SetContentPart(mPartPtr);
	if (noteType == N_ENDNOTE)
		footnote->SetIsEndnote(true);
	
	mPartPtr = orig_part;
	mLogCharPos = orig_pos;
	
	mPartPtr->AddSpecialCharacter(footnote);
	mLogCharPos++;
	
	mInNote = false;
	
	return B_OK;
}


void TImportWORD::sendFormatDocMsg(void)
{
	// Margins come from the section properties (SEP), but since
	// Squirrel doesn't support sections, just use the default SEP margin	
	BRect rect(1800 / TWIPS_PER_POINT, 1440 / TWIPS_PER_POINT, 1800 / TWIPS_PER_POINT, 1440 / TWIPS_PER_POINT);
	
//	BMessage* msg = new BMessage(msg_FORMAT_DOC);
//	msg->AddRect("Margins", rect);
//	msg->AddBool("MirrorFacingPages", mDOP.bitField1 & fFacingPages);
//	msg->AddBool("ShowMargins", true);
//	msg->AddBool("ShowPageGuides", true);
//	
//	msg->AddBool("NoUndo", true);
	
//	mOutMsgStream->AddMessage(msg);	
}


// This always sends text using the current styles, and then when it's
// done, it sets any new styles to be current
void TImportWORD::sendMimeDataMsg(char *ptr, long ptrBytes)
{
	if (ptrBytes <= 0 || mInField)
		return;
	
	//BMessage* msg = new BMessage(B_MIME_DATA);
	//msg->AddData("text/plain", B_MIME_DATA, ptr, ptrBytes);
 
 	//addStylesAndExtrasToMsg(msg);


/*	if (mCurrFilterStyle[S_CHAR])
		mCurrFilterStyle[S_CHAR]->AddStyleToMessage(msg, "CharStyle");
	if (mCurrFilterStyle[S_PARA])
		mCurrFilterStyle[S_PARA]->AddStyleToMessage(msg, "ParaStyle");*/
	
	
	if (mCurrFilterStyle[S_CHAR])
	{
		TTranslatorStyle* newStyle = new TTranslatorStyle( *((TTranslatorStyle*) (mCurrFilterStyle[S_CHAR])) );
		mPartPtr->AddCharacterStyleRun(mLogCharPos, mLogCharPos + ptrBytes, newStyle);
	}	
	if (mCurrFilterStyle[S_PARA])
	{
		TTranslatorStyle* newStyle = new TTranslatorStyle( *((TTranslatorStyle*) (mCurrFilterStyle[S_PARA])) );
		mPartPtr->AddParagraphStyleRun(mLogCharPos, mLogCharPos + ptrBytes, newStyle);
	}



	mLogCharPos += ptrBytes;
	
	mPartPtr->AddText(ptr, ptrBytes);
//	mOutMsgStream->AddMessage(msg);	
}


// Make any new styles current before sending message.
void TImportWORD::sendMsgID(int32 msgID, char *format)
{
//	BMessage* msg = new BMessage(msgID);
//	msg->AddBool("AtEnd", true);
//	msg->AddBool("NoUndo", true);
//	
//	if (format)
//		msg->AddString("format", format);
//	 
// 	addStylesAndExtrasToMsg(msg);

//	mOutMsgStream->AddMessage(msg);	
}

void  TImportWORD::processRow(void)
{
	BList* tcList = mCurrFilterStyle[S_PARA]->GetTCList();
	BList* widthList = mCurrFilterStyle[S_PARA]->GetCellWidths();
	
	TTableRow* newRow = new TTableRow();
	
	//ASSERT(tcList->CountItems() <= mTCellList.CountItems(), "cell count not valid");
	
	while (tcList->CountItems() < mTCellList.CountItems())
		delete mTCellList.RemoveItem(mTCellList.CountItems() - 1);
	
	if (mTCellList.CountItems() == 0)
	{
		delete newRow;
		return;
	}
	
	((TTableCell*) mTCellList.FirstItem())->SetCellPosition(BPoint(0, -1));

	for (int i = 1; i < mTCellList.CountItems(); i++)
	{
		int32 xpos = (long) widthList->ItemAt(i);
		((TTableCell*) mTCellList.ItemAt(i))->SetCellPosition(BPoint(xpos, -1));
	}
	
	int previousDxa = 0; // 0 instead of '(long) widthList->ItemAt(0)' because we always want the first cell to start at 0
	for (int i = 0; i < mTCellList.CountItems(); i++)
	{
		((TTableCell*) mTCellList.ItemAt(i))->SetWidth(((long) widthList->ItemAt(i + 1)) - previousDxa);
		((TTableCell*) mTCellList.ItemAt(i))->SetHeight(mCurrFilterStyle[S_PARA]->GetRowHeight());
		
		previousDxa = (long) widthList->ItemAt(i + 1);
	}
	
	for (int i = 1; i < widthList->CountItems(); i++)
		insertCellDivider((long) widthList->ItemAt(i));
	
	
//	int currentColSpan = 1;
	int currentWidth = 0;
	for (int i = mTCellList.CountItems() - 1; i >= 0; i--)
	{
		TC* currentTC = (TC*) tcList->ItemAt(i);
		TTableCell* currentCell = ((TTableCell*) mTCellList.ItemAt(i));
		
		if (currentTC->bitField1 & tc_fMerged && !(currentTC->bitField1 & tc_fFirstMerged))
		{
			bool the_bool;
			currentWidth += currentCell->Width(the_bool);
			delete currentCell;
			mTCellList.RemoveItem(i);
		}
		else if (currentTC->bitField1 & tc_fFirstMerged)
		{
			bool the_bool;
			int32 currentCellWidth = currentCell->Width(the_bool);
			currentCell->SetWidth(currentWidth + currentCellWidth);
			currentWidth = 0;
		}
		
		if (currentTC->bitField1 & tc_fVertMerged && !(currentTC->bitField1 & tc_fVertRestart))
		{
			currentCell->SetRowSpan(-1);
		}
	}
	
	
	for (int i = 0; i < mTCellList.CountItems(); i++)
		newRow->AddCell((TTableCell*) mTCellList.ItemAt(i));
	
	while(mTCellList.CountItems())
		mTCellList.RemoveItem((long) 0);
	mTRowList.AddItem(newRow);	
}

void TImportWORD::sendTable(void)
{
	TTranslatorPart_Table* newTable = new TTranslatorPart_Table(&mTransDoc);
	
//	transformTable();
	setColSpans();
	setRowSpans();
	
	mLogCharPos = mTCharPosStore;
	mPartPtr = mTPtrStore;
	mInTable = false;

	for(int i = 0; i < mTRowList.CountItems(); i++)
		newTable->AddRow(static_cast<TTableRow *>(mTRowList.ItemAt(i)));
	
	newTable->SetBorder(1);
	newTable->SetTableKind(kTableKind_WORD);
	TSpecialCharacter_Frame* newFrame = new TSpecialCharacter_Frame();
	newFrame->SetFramePart(newTable);
	
	mPartPtr->AddSpecialCharacter(newFrame);
	mLogCharPos++;
	sendMimeDataMsg("\n", 1);
	
	while (mTRowList.CountItems())
		mTRowList.RemoveItem(0L);
	
	while (mCellDividers.CountItems())
		mCellDividers.RemoveItem(0L);
	mCellDividers.AddItem(0);

	while(mTCellList.CountItems()) // just in case there were some cells that were added
		delete mTCellList.RemoveItem(0L);

}

int32 TImportWORD::calcCellColSpan(TTableCell* theCell)
{
	BPoint cellPos;
	bool theBool;
	theCell->GetCellPosition(&cellPos);
	int32 cellStart = (int32) cellPos.x;
	int32 cellEnd = cellStart + theCell->Width(theBool);
	int32 firstDivision = -1;
	int32 secondDivision = -1;
	
	for(int i = 0; i < mCellDividers.CountItems(); i++)
	{
		if (cellStart == (int32) mCellDividers.ItemAt(i))
			firstDivision = i;
		
		else if (cellEnd == (int32) mCellDividers.ItemAt(i))
		{
			secondDivision = i;
			break;
		}
	} 
	ASSERT(firstDivision != -1 && secondDivision != -1, "cell start or end division not found");
	
	return secondDivision - firstDivision;
}

void TImportWORD::setColSpans(void)
{
	for (int i = 0; i < mTRowList.CountItems(); i++)
	{

		TTableRow* currentRow = (TTableRow*) mTRowList.ItemAt(i);

		for (int j = 0; j < currentRow->CountItems(); j++)
		{
			TTableCell* currentCell = currentRow->CellPtr(j);
			
			int32 cellColSpan = calcCellColSpan(currentCell);
			currentCell->SetColumnSpan(cellColSpan);;
		}
	}
}


void TImportWORD::setRowSpans(void)
{
	for (int i = mTRowList.CountItems() - 1; i > 0; i--)
	{
		TTableRow* currentRow = (TTableRow*) mTRowList.ItemAt(i);
		
		for (int j = currentRow->CountItems() - 1; j >= 0; j--)
		{
			TTableCell* currentCell = currentRow->CellPtr(j);
			
			if (currentCell->RowSpan() < 0)
			{
				int32 currentCellIndex = getCellColIndex(i, j);
				TTableCell* aboveCell = getCell(i-1, currentCellIndex);
				
				if (aboveCell->RowSpan() < 0)
					aboveCell->SetRowSpan(currentCell->RowSpan() - 1);
				else
					aboveCell->SetRowSpan((currentCell->RowSpan() - 1) * -1);
				
				delete currentRow->RemoveItem(j);
			}
		}
	}	
}

void TImportWORD::insertCellDivider(int32 divider)
{
	ASSERT(divider >= 0, "negative cell divider");
	
	if (divider < 0)
		return;
	
	int32 previousDivider = (int32) mCellDividers.FirstItem();
	for (int32 i = 0; i < mCellDividers.CountItems(); i++)
	{
		if ((int32) mCellDividers.ItemAt(i) > divider)
		{
			mCellDividers.AddItem((void*) divider, i);
			break;
		}
		else if ((int32) mCellDividers.ItemAt(i) == divider)
			break;
	}
	if ((int32) mCellDividers.LastItem() < divider)
		mCellDividers.AddItem((void*) divider);
}

TTableCell* TImportWORD::getCell(int32 row, int32 col)
{
	ASSERT(mTRowList.CountItems() > row && row >= 0, "Row not valid");

	TTableRow* theRow = (TTableRow*) mTRowList.ItemAt(row);
	
	int pastCells = 0;
	for (int i = 0; i < theRow->Cells(); i++)
	{
		TTableCell* currentCell = theRow->CellPtr(i);
		int32 currentColSpan = 0;
		
		
		if (col >= pastCells && col < pastCells + currentCell->ColumnSpan())
			return currentCell;
		pastCells += currentCell->ColumnSpan();
	}
	
	//ASSERT(false, "if the cell exists, then we should have returned it by now");
	
	return 0;
}

int32 TImportWORD::getCellColIndex(int32 rowIndex, int32 cellIndex)
{
	ASSERT(mTRowList.CountItems() > rowIndex && rowIndex >= 0, "Row not valid");

	TTableRow* theRow = (TTableRow*) mTRowList.ItemAt(rowIndex);
	
	int32 index = 0;
	
	for (int i = 0; i < cellIndex; i++)
	{
		TTableCell* currentCell = theRow->CellPtr(i);
		
		index += currentCell->ColumnSpan();
	}
	return index;
}



void TImportWORD::makeNewStylesCurr(void)
{
 	for (long i = S_CHAR; i >= S_PARA; i--)
 	{
		if (mNewFilterStyle[i])
		{
			delete mCurrFilterStyle[i];
			mCurrFilterStyle[i] = mNewFilterStyle[i];
			mNewFilterStyle[i] = 0;
		}
	}
}


int32 TImportWORD::convertBufferToUTF8(char *& buffer, int32& bufferSize, char *& bufferUTF8, int32& bufferUTF8Size, int32& bufferUTF8MaxSize, int32 encoding)
{
	// make sure the utf8 buffer is big enough for the text
	if (bufferUTF8MaxSize < (3 * bufferSize))
	{
		delete [] bufferUTF8;
		bufferUTF8MaxSize = (3 * bufferSize);		
		bufferUTF8 = new char[bufferUTF8MaxSize];
		if (!bufferUTF8)
		{
			bufferUTF8MaxSize = 0;
			return B_NO_MEMORY;
		}
	}
	
	// now convert to utf8 from whatever char set this was
	int32 srcBytes = bufferSize;
	bufferUTF8Size = bufferUTF8MaxSize;
	int32 state = 0;
	
	if (encoding == B_UNICODE_CONVERSION) // -BOGUS SKANK take out M$'s private use chars
		for(int i = 0; i < bufferSize / 2; i++) 
			if ((uchar)(buffer[i * 2]) == 0xf0)
				buffer[i * 2] = 0;
	
	/*for (int i = 0; i < bufferSize; i++)
	{
		if ((!(i % 2)) && encoding == B_UNICODE_CONVERSION)
		{
			if ((uchar)(buffer[i]) == 0xf0)
				buffer[i] = 0;
		}
		else
		{
			if (buffer[i] == 12)
				buffer[i] = 10;
		}
	} */
	
	convert_to_utf8(encoding, buffer, &srcBytes, &bufferUTF8[0], &bufferUTF8Size, &state);
		
	return B_OK;
}


// Returns true if the style for the fcCurr character has changed from the previous style.
// Checks both para and char styles.
// 	
// TO GET PARAGRAPH PROPERTIES:
// 1- PAPX bin table will give you the page number of a FKP that contains this fcCurr 
// 2- find PAPX in that FKP, again using fcCurr
// 3- PAPX.istd gives the index of a STD in the style sheet
// 4- apply PAPX sprms to the STD
// END OF FULL SAVE
// 5- for complex, piece table may point to piece table sprms, which should be applied 
//
// TO GET CHARACTER PROPERTIES:
//
// 1- Get paragrph properties (which contain char styles) using above steps 
// 2- CHPX bin table will give you the page number of a FKP that contains this fcCurr 
// 3- find CHPX in that FKP, again using fcCurr
// 4- apply CHPX sprms to the STD
// END OF FULL SAVE
// 5- for complex, piece table may point to piece table sprms, which should be applied 
void TImportWORD::checkStyles(long fcCurr, ushort prm)
{	
	ushort	istd = 0, sprmBytes = 0;
	char *	sprms = NULL;
#ifdef DEBUG
	if (mNewFilterStyle[S_PARA] || mNewFilterStyle[S_CHAR])
		fprintf(stderr, "WORD checkStyles: ERROR - still have new filterStyles\n");	 
#endif	// DEBUG

	mStyleChanged[S_PARA] = mStyleChanged[S_CHAR] = false;
	
 	for (long i = S_PARA; i <= S_CHAR; i++) // first sheck papx
 	{
		if (!mCurrFilterStyle[i] || fcCurr < mStyleStartFC[i] || fcCurr >= mStyleEndFC[i])
		{
			bool gotOK = false;
			
			// get the FKP sprms
			if (i == S_PARA)
				gotOK = mFKP_PAPX->GetSprms(fcCurr, mStyleStartFC[i], mStyleEndFC[i], istd, sprms, sprmBytes);
			else
				gotOK = mFKP_CHPX->GetSprms(fcCurr, mStyleStartFC[i], mStyleEndFC[i], sprms, sprmBytes);
				
			if (gotOK)
			{
				mNewFilterStyle[i] = newFilterStyle(i, istd, sprmBytes);
				if (mNewFilterStyle[i])
				{
					//IFDEBUG(fprintf(stderr, "WORD checkStyles: fc: %d (%d-%d), istd: %3d, parent: %s\n", fcCurr, mStyleStartFC[i], mStyleEndFC[i], (i == S_PARA) ? istd : -1, mNewFilterStyle[i]->GetParentStyleName()));
					mStyleChanged[i] = true;
					
					// apply the FKP sprms
					if (sprms && sprmBytes)
						ApplyAllSprms(sprms, sprmBytes, mNewFilterStyle[i], i, mFontTblWORD, mStyleSheetWORD, mFIBHeader.nFib);
					
					// now apply any CLX sprms (step 5 above)
					if (prm)
						applyCLXsprm(prm, i);
					
					// if there is no current style, then make this new one current						
					if (!mCurrFilterStyle[i])
					{
						mCurrFilterStyle[i] = mNewFilterStyle[i];
						mNewFilterStyle[i] = 0;
					}
					
					// if para style changes, then force char style to change
					// -BOGUS the char style can change when it pleases -joel
					/*if (i == S_PARA) 
					{
						delete mCurrFilterStyle[S_CHAR];
						mCurrFilterStyle[S_CHAR] = 0;
					}*/
				}
			}
		}	
 	}
}


void TImportWORD::applyCLXsprm(ushort prm, long styleType)
{
	ushort	index, sprmBytes;
	char	sprmData[3];
	char *	sprm;
	
	if (prm & fComplexPRM)
	{
		// this is an index into the CLX grpprl
		index = (prm & igrpprl) >> 1;
		IFDEBUG(fprintf(stderr, "WORD applyCLXsprm (complex): prm: x%x, index: %d\n", prm, index));
		if (index >= mCLXsprms.CountItems())
		{
			IFDEBUG(fprintf(stderr, "WORD applyCLXsprm: ERROR bad index: %d\n", index));
			return;
		}
		sprm = (char *)(mCLXsprms.ItemAt(index));
		sprmBytes = (ushort)(mCLXsprmBytes.ItemAt(index));	
	}
	else
	{
		// this is an index into the rgsprmPrm and a 1 byte operand (3 bytes total sprm)
		if (mFIBHeader.nFib > 190)
		{
			index = (prm & isprm) >> 1;
			memcpy(&sprmData[0], &rgsprmPrm[index], 2);
			sprmData[2] = (prm & val) >> 8;
			IFDEBUG(fprintf(stderr, "WORD applyCLXsprm (non-complex): prm: x%x, index: %d, sprmData: x%0x%0x%0x\n", prm, index, sprmData[0], sprmData[1], sprmData[2]));
			sprm = &sprmData[0];
			sprmBytes = 3;
		}
		else
		{
			index = (prm & isprm) >> 1;
			sprmData[0] = index;
			sprmData[1] = (prm & val) >> 8;
			IFDEBUG(fprintf(stderr, "WORD applyCLXsprm (non-complex): prm: x%x, index: %d, sprmData: x%0x%0x%0x\n", prm, index, sprmData[0], sprmData[1], sprmData[2]));
			sprm = &sprmData[0];
			sprmBytes = 2;
		}
	}
	
	ApplyAllSprms(sprm, sprmBytes, mNewFilterStyle[styleType], styleType, mFontTblWORD, mStyleSheetWORD, mFIBHeader.nFib);
}
	

// create a para or char filter style, depending on i
TFilterStyleWORD * TImportWORD::newFilterStyle(long i, ushort istd, ushort sprmBytes)
{
	if (i == S_PARA)
		return mStyleSheetWORD->NewFilterStylePara(istd);
		
	if (i != S_CHAR)
	{
		IFDEBUG(fprintf(stderr, "WORD newFilterStyle: ERROR bad i: %d\n", i));
		return NULL;
	}
	
	TFilterStyleWORD *	filterStyle = new TFilterStyleWORD();
	if (filterStyle)
	{			
		filterStyle->SetStyleType(TTranslatorStyle::kBasicStyle);
		
		// it's char, so base it off of the para style, but only if we actually have sprms
		// to apply to it.
/*		if (sprmBytes)
		{	
			if (mNewFilterStyle[S_PARA])
			{
				const char * tempStr;
				mNewFilterStyle[S_PARA]->GetApplyStyleName(&tempStr);
				filterStyle->SetApplyStyleName(tempStr);
			}
			else
			{
				if (mCurrFilterStyle[S_PARA])
				{
					const char* tempStr;
					mCurrFilterStyle[S_PARA]->GetApplyStyleName(&tempStr);
					filterStyle->SetApplyStyleName(tempStr);
				}
				else
				{
					IFDEBUG(fprintf(stderr, "WORD newFilterStyle: ERROR no para style to be parent\n"));
					delete filterStyle;
					filterStyle = NULL;
				}
			}
		}*/
	}
	else
		IFDEBUG(fprintf(stderr, "WORD newFilterStyle: error creating TFilterStyleWORD\n"));
	
	return filterStyle;
}
