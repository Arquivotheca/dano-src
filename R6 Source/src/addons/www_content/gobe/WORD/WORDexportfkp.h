//
//	WORDexportfkp.h
//
#ifndef __WORD_EXPORT_FKP_H__
#define __WORD_EXPORT_FKP_H__

#include "WORDconsts.h"
#include "WORDfont.h"
#include "WORDstyles.h"

//-------------------------------------------------------------------
// TExportFKPTbl
//-------------------------------------------------------------------

class TExportFKPTbl
{
public:
						TExportFKPTbl(TOLEEntryWriter*, TOLEEntryWriter*);
	virtual				~TExportFKPTbl();
	
	void				setFirstFC(int32 firstFC) {mfirstFC = firstFC;}
	int32 				setEntryPosition(TOLEEntryWriter* entryWriter, int32 pos);
	int32				write(long*, unsigned long*);
	virtual int32		writeFKP(int32 firstIndex) = 0;

	int32				writeFKPs();
	int32				numFkps(){return mBinTbl.CountItems();}
	int32				firstPN();
	int32				extendLastRun(long);

protected:
	
	TOLEEntryWriter*	mDocWriter;
	TOLEEntryWriter*	mTableWriter;
	
	BList				mBinTbl; // stores the bin table that will be created right
									// before the FKPs are writen to disk
	
	BList				mEndFCTbl; // stores the EndFC of a run
	BList				mBXTbl; // stores the BX of a run
	BList				mSTSHITbl; // stores the STSHI of a run
	BList				mSPRMTbl; // stores the SPRMs of a run
	BList				mSPRMsizeTbl; // stores the SPRMs of a run
	BList				mStyleIndexTbl; // stores the SPRMs of a run

	int32				mfirstFC;
	int32				mLatestFC;
};


//-------------------------------------------------------------------
// TPAPXExportTbl
//-------------------------------------------------------------------

class TPAPXExportTbl : public TExportFKPTbl
{
public:
					TPAPXExportTbl(TOLEEntryWriter*, TOLEEntryWriter*);

	int32			addRun(long, const TTranslatorStyle *, TFontTblWORD*, TStyleSheetWORD*);
	//int32			writeFKPs();
	virtual int32	writeFKP(int32 firstIndex);
};

class TCHPXExportTbl : public TExportFKPTbl
{
public:
					TCHPXExportTbl(TOLEEntryWriter*, TOLEEntryWriter*);

	int32			addRun(long, const TTranslatorStyle *, TFontTblWORD*, TStyleSheetWORD*, bool special = false);
	//int32			writeFKPs();
	virtual int32	writeFKP(int32 firstIndex);
};



#endif
