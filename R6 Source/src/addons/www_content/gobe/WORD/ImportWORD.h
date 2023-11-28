//
//	ImportWORD.h
//
#ifndef __IMPORTWORD_H__
#define __IMPORTWORD_H__

#include "TranslatorLib.h"
#include "BlockStream.h"

#include "WORDconsts.h"
#include "WORDstyles.h"
#include "WORDfkp.h"
#include "WORDfont.h"
#include <stdio.h> // -BOGUS because we have a FILE* in TImportWORD

const TOLEEntry *getOleEntry(TOLEReader *oleReader, const char *entryName, const TOLEEntry *currentEntry = NULL, int *depth = 0);


//-------------------------------------------------------------------
// TImportWORD
//-------------------------------------------------------------------

class TImportWORD
{
public:	
					TImportWORD(BPositionIO *inStream, TBlockStreamWriter* writer, BMessage* ioExt);
	virtual			~TImportWORD();
	
			int32	DoTranslate(void);

private:

	int32			loadFIB(void);	
	void			spew(TOLEEntryReader* reader, int32 position, int32 length);
	void			spewFIB(void);
	int32			loadDocProperties(void);
	int32			loadPieceTable(void);
	int32			loadCHPX(void);
	int32			loadPAPX(void);
	int32			loadAllNoteCPs(void);
	int32			loadFileShapes(void);
	
	void			checkStyles(long fcCurr, ushort prm);
	void			applyCLXsprm(ushort prm, long styleType);
	
	int32			processText(long cpStart, long cpEnd, char *&buffer, int32& bufferMaxSize, char *&utf8Buffer, int32& utf8BufferMaxSize);
	int32			processHeaders();
	bool			processField(void);
	int32			insertNote(int32 type);
	
	void			sendFormatDocMsg(void);
	void			sendMimeDataMsg(char *ptr, long ptrBytes);
	void			sendMsgID(int32 msgID, char *format = NULL);
	
	void			processRow(void);
	void			sendTable(void);
	void			setColSpans(void);
	int32			calcCellColSpan(TTableCell* theCell);
	void			setRowSpans(void);
	void			transformTable(void);
	void			insertCellDivider(int32 divider);
	TTableCell*		getCell(int32 row, int32 col);
	int32			getCellColIndex(int32 rowIndex, int32 cellIndex);
	
	void			makeNewStylesCurr(void);
	
	void			swapStyleSheetHeader(void);
	
	int32			convertBufferToUTF8(char *& buffer, int32& bufferSize, char *& bufferUTF8, int32& bufferUTF8Size, int32& bufferUTF8MaxSize, int32 encoding);
	
	TFilterStyleWORD *	newFilterStyle(long i, ushort istd, ushort sprmBytes);
	
	BPositionIO *		mInStream;
	TBlockStreamWriter*	mWriter;

	long				mDocStreamSize;
	long				mTableStreamSize;
	
	TOLEReader *		mOleReader;
	
	const TOLEEntry *	mDocEntry;
	TOLEEntryReader *	mDocReader;
	
	const TOLEEntry *	mTableEntry;
	TOLEEntryReader *	mTableReader;
		
	char *				mBuffer;
	int32				mBufferMaxSize;	
	char *				mBufferUTF8;
	int32				mBufferUTF8MaxSize;

	char *				mAltBuffer;
	int32				mAltBufferMaxSize;	
	char *				mAltBufferUTF8;
	int32				mAltBufferUTF8MaxSize;
	
	FIBHeader			mFIBHeader;
	short				mFIBShort[NUM_FIB_SHORTS];
	long				mFIBLong[NUM_FIB_LONGS];
	FIB_Pair			mFIBPair[NUM_FIB_PAIRS];
	
	DOP					mDOP;
	
	TStyleSheetWORD *	mStyleSheetWORD;
	TFontTblWORD *		mFontTblWORD;
	
	int32				mEncoding;
	
	long				mPieceTblEntries;
	PieceTbl *			mPieceTbl;
	BList				mCLXsprms;
	BList				mCLXsprmBytes;
	
	long				mFspaTblEntries;
	FspaTbl *			mFspaTbl;
	
	TCHPXTbl *			mFKP_CHPX;
	TPAPXTbl *			mFKP_PAPX;
	
	bool				mInNote;		
	long				mNoteRefEntries[N_TYPE_COUNT];
	long *				mNoteRefCP[N_TYPE_COUNT];
	long				mNoteTxtEntries[N_TYPE_COUNT];
	long *				mNoteTxtCP[N_TYPE_COUNT];
	
	long				mStyleStartFC[S_TYPE_COUNT];	
	long				mStyleEndFC[S_TYPE_COUNT];	
	bool				mStyleChanged[S_TYPE_COUNT];
	
	TFilterStyleWORD *	mCurrFilterStyle[S_TYPE_COUNT];
	TFilterStyleWORD *	mNewFilterStyle[S_TYPE_COUNT];
	
	bool				mInTable;

	bool				mInField;
	bool				mProcessingField;
	char				mFieldData[MAX_FIELD_DATA_SIZE];
	short				mFieldDataSize;
	
	BList				mCellDividers;
	BList				mTCellList;
	BList				mTRowList;
	TTranslatorPart_WP*	mTPtrStore;
	long				mTCharPosStore;
	
	
	TTranslatorDoc		mTransDoc;
	TTranslatorPart_WP*	mPartPtr;
	
	TTranslatorPart_WP*	mMainPartPtr;
	TTranslatorPart_WP*	mHeaderPartPtr;
	TTranslatorPart_WP*	mFooterPartPtr;
	
	long				mLogCharPos;
};

#endif	// __IMPORTWORD_H__