//
//	ExportWORD.h
//
#ifndef __EXPORTWORD_H__
#define __EXPORTWORD_H__

#include "TranslatorLib.h"

#include "WORDconsts.h"
#include "WORDexportfkp.h"
#include "WORDfont.h"
#include "WORDstyles.h"

class TFootnoteTblWord;


//-------------------------------------------------------------------
// TExportWORD
//-------------------------------------------------------------------

class TExportWORD
{
public:	
					TExportWORD(TBlockStreamReader* reader, BPositionIO *outStream);
	virtual			~TExportWORD();
	
			int32	DoTranslate(void);

			void	createAllEntries();
			int32	setEntryPosition(TOLEEntryWriter*, int32);
			int32	writeStaticStreams();			
			void	WriteKnownStreams();
			int32	initFIB();
			int32	writeFIB();
			int32	initDOP();
			int32	writeDOP();
			int32	processText(const char* text, int32 text_length, long& char_count);
			int32	processPart(const TTranslatorPart_WP* part, int32 &char_count);
			int32	writePieceTable();
			int32	writeSED();
			int32	writeLVC();
			int32	writeNotes();
			int32	writeHeaders();
			int32	countChars() { return mFIBLong[ccpText] + mFIBLong[ccpFtn] + mFIBLong[ccpHdr] + mFIBLong[ccpMcr] +
							mFIBLong[ccpAtn] + mFIBLong[ccpEdn] + mFIBLong[ccpTxbx] + mFIBLong[ccpHdrTxbx];}
			
			int32	finishWordDocument();			
			int32	convertToUnicodeBuffer(const char *dataPtr, int32 dataSize);
			int32	convertToMSWindowsBuffer(const char *dataPtr, int32 dataSize);
private:
				
	TBlockStreamReader*	mReader;
	BPositionIO *	mOutStream;
	
	BFile *			mOutFile;		
	
	char *			mBuffer;
	int32			mBufferMaxSize;
	int32			mBufferSize;
	
	char *			mZeroBlock;
	
	TOLEWriter *	mOleWriter;
	TOLEEntryWriter *mWriterMain;
	TOLEEntryWriter *mWriterTable;
	
	int32			mIndexRoot;
	int32			mIndexSumInfo;
	int32			mIndexDocSumInfo;
	int32			mIndexMain;
	int32			mIndexTable;
	int32			mIndexCompObj;
	
	DOP				mDOP;
	
	TPAPXExportTbl*	mFKP_PAPX;
	TCHPXExportTbl*	mFKP_CHPX;
	
	TFontTblWORD*		mFontTbl;
	TStyleSheetWORD*	mStyleTbl;
	
	TFootnoteTblWord*	mFootnoteTbl;
	TFootnoteTblWord*	mEndnoteTbl;
	
	FIBHeader		mFIBHeader;
	short			mFIBShort[NUM_FIB_SHORTS];
	long			mFIBLong[NUM_FIB_LONGS];
	FIB_Pair		mFIBPair[NUM_FIB_PAIRS];
	

	int32			mBytesTextWritten;
	
	TTranslatorDoc				mTransDoc;
	const TTranslatorPart_WP*	mPartPtr;
	const TTranslatorPart_WP*	mMainPartPtr;
	const TTranslatorPart_WP*	mHeaderPartPtr;
	const TTranslatorPart_WP*	mFooterPartPtr;
	
	const TTranslatorStyle*	mBlankStyle;

	const TTranslatorStyle*	mLastParaStyle;
	const TTranslatorStyle*	mLastCharStyle;
};

class TFootnoteTblWord
{
public:
	TFootnoteTblWord() {}
	
	void	AddFootnote(int32 position, const TTranslatorPart* part, int type);

	void	AddNoteEnd(int32 end)		{mNoteEndList.AddItem((void *) end);}
	
	int32	GetPosition(int index)		{return (int32) mPositionList.ItemAt(index);}
	const	TTranslatorPart* GetPart(int index)	{return (TTranslatorPart*) mPartList.ItemAt(index);}
	int		GetType(int index)			{return (int) mTypeList.ItemAt(index);}
	int32	GetEnd(int index)			{return (int32) mNoteEndList.ItemAt(index);}
	
	int32	Count() 					{return mPositionList.CountItems();}
	
private:
	BList mPositionList;
	BList mPartList;
	BList mTypeList;
	
	BList mNoteEndList;
};


// hard-coded streams (at bottom of cpp file)
// CompObj
#define BYTES_COMPOBJ			106
extern char CompObjStreamData[];

// SummaryInformation - SUMINFO_2 is all zeros
#define BYTES_SUMINFO_TOTAL		(8 * BLOCKSIZE)
#define BYTES_SUMINFO_1			400
extern char SumInfoStreamData[];

// DocumentSummaryInformation - DOCSUMINFO_2 is all zeros 
#define BYTES_DOCSUMINFO_TOTAL		(8 * BLOCKSIZE)
#define BYTES_DOCSUMINFO_1			464
extern char DocSumInfoStreamData[];

// FibData 
#define BYTES_FIB				1024
extern char FibData[];

// 1table data
#define BYTES_TABLE_STSH		((9 * 16) + 2)
extern char TableStshData[];
#define BYTES_TABLE_1			((10 * 16) + 14)
extern char Table1Data[];
// piece table here
#define BYTES_TABLE_2			((16 *16) + 11)
extern char Table2Data[];
#define BYTES_TABLE_ZERO_PAD2	(16 * 19) 
#define BYTES_TABLE_3			176
extern char Table3Data[];
#define BYTES_TABLE_ZERO_PAD3	(16 * 28) 

#endif	// __EXPORTWORD_H__

// $Header: /usr/local/cvsroot/8ball/Datatypes/WORD/ExportWORD.h,v 1.22 2000/02/17 03:29:47 joel Exp $
