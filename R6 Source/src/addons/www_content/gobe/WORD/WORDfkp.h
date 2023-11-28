//
//	WORDfkp.h
//
#ifndef __WORDFKP_H__
#define __WORDFKP_H__

#include "WORDconsts.h"
#include "WORDfont.h"

//void TranApplyAllSprms(char *sprmBuffer, ushort sprmRunBytes, TTranslatorStyle *filterStyle, TFontTblWORD *fontTblWORD);
//void TranApplySprm(char *sprmBuffer, ushort theSprm, TTranslatorStyle *filterStyle, TFontTblWORD *fontTblWORD);



//-------------------------------------------------------------------
// TFKPTbl
//-------------------------------------------------------------------

class TFKPTbl
{
public:	
					TFKPTbl(TOLEEntryReader *docReader, TOLEEntryReader *tableReader, long binPos, long binBytes, int16 nFib);
	virtual			~TFKPTbl();
	
	// This is called to load the FKPs from the stream
	bool			Load(void);

protected:
	// Implemented by CHPX or PAPX to handle it's own unique data		
	virtual bool		LoadBytes(long pn, long fkpIndex, short crun) = 0;	
	int32				loadBinTable(long binPos, long binBytes);
	char *				getFKP(long fc, long& startFC, long& endFC, char& index);

	TOLEEntryReader *	mDocReader;
	TOLEEntryReader *	mTableReader;
	long				mBinTblEntries;
	BinTbl *			mBinTbl;
	
	BList				mStartFCList;
	BList				mEndFCList;
	
	long				mTotalPages;
	long *				mMapPN;
	
	long				mFKPCount;
	FKP *				mFKPTbl;
	int16				mNFib;
};


//-------------------------------------------------------------------
// TCHPXTbl
//-------------------------------------------------------------------

class TCHPXTbl : public TFKPTbl
{
public:	
					TCHPXTbl(TOLEEntryReader *docReader, TOLEEntryReader *tableReader, long binPos, long binBytes, int16 nFib);
	virtual			~TCHPXTbl()		{}
	
	bool			GetSprms(long fc, long& startFC, long& endFC, char *& sprms, ushort& sprmBytes);

protected:		
	virtual bool	LoadBytes(long pn, long fkpIndex, short crun);

private:	
};


//-------------------------------------------------------------------
// TPAPXTbl
//-------------------------------------------------------------------

class TPAPXTbl : public TFKPTbl
{
public:	
					TPAPXTbl(TOLEEntryReader *docReader, TOLEEntryReader *tableReader, long binPos, long binBytes, int16 nFib);
	virtual			~TPAPXTbl()		{}
	
	bool			GetSprms(long fc, long& startFC, long& endFC, ushort& istd, char *& sprms, ushort& sprmBytes);
	//void			AddParagraphRuns(TTranslatorPart_WP*, TFontTblWORD*);
protected:		
	virtual bool	LoadBytes(long pn, long fkpIndex, short crun);
};

#endif	// __WORDFKP_H__