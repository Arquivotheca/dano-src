//
//	WORDstyles.h
//
#ifndef __WORDSTYLES_H__
#define __WORDSTYLES_H__

#include "TranslatorLib.h"

#include "WORDconsts.h"
#include "WORDfont.h"

class TFontTblWORD;
class TStyleSheetWORD;

//-------------------------------------------------------------------
// TFilterStyleWORD
//-------------------------------------------------------------------

//class TFilterStyleWORD : public TFilterStyle
class TFilterStyleWORD : public TTranslatorStyle
{
public:	
					TFilterStyleWORD();
	virtual			~TFilterStyleWORD();

		bool		IsSpecialChar(void)					{return mSpecialChar;}
		void		SetSpecialChar(bool value)			{mSpecialChar = value;}

		bool		IsEmbossedImprint(void)					{return mEmbossedImprint;}
		void		SetEmbossedImprint(bool value)			{mEmbossedImprint = value;}

		long		GetPicFC(void)						{return mPicFC;}
		void		SetPicFC(long value)				{mPicFC = value;}

		bool		IsInTable(void)						{return mInTable;}
		void		SetInTable(bool value)				{mInTable = value;}

		bool		IsEndTableRow(void)					{return mEndTableRow;}
		void		SetEndTableRow(bool value)			{mEndTableRow = value;}

		//short		GetTableGapHalf(void)				{return mTableGapHalf;}
		//void		SetTableGapHalf(short value)		{mTableGapHalf = value;}

		short		GetRowHeight(void)					{return mRowHeight;}
		void		SetRowHeight(short value)			{mRowHeight = value;}
		
		short		GetCellsInRow(void)					{return mCellsInRow;}
		void		SetCellsInRow(short value)			{mCellsInRow = value;}
		
		BList*		GetTCList(void)						{return &mTCList;}
		BList*		GetCellWidths(void)					{return &mCellWidths;}
		
private:
		bool		mSpecialChar;
		bool		mEmbossedImprint;
		long		mPicFC;
		
		
		// table stuff
		bool		mInTable;
		bool		mEndTableRow;	
		short		mRowHeight;
		//short		mTableGapHalf;
		short		mCellsInRow;
		BList		mTCList;
		BList		mCellWidths;
};


//-------------------------------------------------------------------
// TStyleWORD
//-------------------------------------------------------------------

class TStyleWORD
{
public:	
					TStyleWORD(TOLEEntryReader *tableReader, long bytesSTD, ulong bytesSTDbase, short istd, int16);
					TStyleWORD(short istd);
	virtual			~TStyleWORD();

	void			BuildFilterStyle(BList *tableSTD, TFontTblWORD *fontTblWORD, TStyleSheetWORD* styleTblWORD, ushort defaultFTC);
	
	char *			StyleName(void)				{return mStyleName;}
	void			SetStyleName(const char*);
	TFilterStyleWORD *	GetFilterStyle(void)	{return mFilterStyle;}
	ushort			GetStyleID(void)			{return (ushort)(mSTDbase.bitField1 & sti);}
	void			SetStyleID(short newSTI);
	ushort			GetStyleTypeCode(void)		{return (ushort)(mSTDbase.bitField2 & sgc);}
	void			SetStyleTypeCode(short newSGC);
	ushort			GetBaseStyleIndex(void)		{return (ushort)((mSTDbase.bitField2 & istdBase) >> 4);}
	void			SetBaseStyleIndex(short newBase);
	ushort			GetCUPX(void)				{return (ushort)(mSTDbase.bitField3 & cupx);}
	void			SetCUPX(ushort count);
	ushort			GetStyleIndexNext(void)		{return (ushort)((mSTDbase.bitField3 & istdNext) >> 4);}
	void			SetStyleIndexNext(ushort index);
	ushort			GetHasUPE(void)				{return (ushort)((mSTDbase.bitField1 & fHasUpe) >> 14);}
	void			SetHasUPE(ushort);
	ushort			GetBchUPE(void)				{return mSTDbase.bchUpe;}
	void			SetBchUPE(ushort offset)	{mSTDbase.bchUpe = offset;}
	int32			GetBytes();
	int32			Write(TOLEEntryWriter *tableWriter);
	void			SetNull(bool isNull)		{mNullStyle = isNull;}
	bool			IsNull()					{return mNullStyle;}
	void			SetUPXPara(const char* sprms, short bytes);
	void			SetUPXChar(const char* sprms, short bytes);
	void			SetSent(bool isSent) 		{mSent = isSent;}
	bool			GetSent() 					{return mSent;}
	
private:
	void			initFields(short istd);
	
	bool			mNullStyle;
	bool			mSent;
	STD_BASE		mSTDbase;
	char *			mStyleName;		// utf8 C string
	short			mUPXparaISTD;
	short			mUPXparaBytes;
	char *			mUPXpara;
	short			mUPXcharBytes;
	char *			mUPXchar;
	TFilterStyleWORD *	mFilterStyle;
	short			mISTD;			// index that this STD occupies in the STD table
	int16			mNFib;
};


//-------------------------------------------------------------------
// TStyleSheetWORD
//-------------------------------------------------------------------

class TStyleSheetWORD
{
public:	
						TStyleSheetWORD(TOLEEntryReader *tableReader, long tableStreamSize, long offset, ulong offsetBytes, TFontTblWORD *fontTblWORD, int16);
						TStyleSheetWORD();
	virtual				~TStyleSheetWORD();

			void		SendStyles(TTranslatorDoc *outTransDoc);
			void		SendStyle(TStyleWORD* theStyle, TTranslatorDoc *outTransDoc);
	TFilterStyleWORD *	NewFilterStylePara(ushort istd);
	TStyleWORD *		StyleFromISTD(ushort istd);
	const char *		NameFromISTD(ushort istd);
			ushort		GetDefaultFTC(short i)	{return mHeader.rgftcStandardChpStsh[i];}
	
			void		AddStyle(TStyleWORD* theStyle) {mTableSTD->AddItem(theStyle);}
			int32		GetStyleIndex(const char*);
			int32		GetSize() 				{return	mTableSTD->CountItems();}
			void		AddStyleTable(TTranslatorStylesTable* styleTable, TFontTblWORD* fontTbl);
			int32		Write(TOLEEntryWriter *tableWriter, long &offset, ulong &offsetBytes);
private:

	bool				readSTSHI(ushort bytesSTSHI);

	TOLEEntryReader *	mTableReader;
	long				mTableStreamSize;
	long				mOffsetSTSH;
	ulong				mBytesSTSH;
			
	STSHI				mHeader;
	
	BList *				mTableSTD;
	int16				mNFib;
};




//-------------------------------------------------------------------
// Utility Stuff
//-------------------------------------------------------------------


//
// Built-in Style Indexes (need to find doc for this)
//
enum
{
 	istdNormal = 0,
 	istdBlockQuote = 16
};


//
// Built-in Style Names
//
enum
{
	stiNormal = 0,
	stiLev1,
	stiLev2,
	stiLev3,
	stiLev4,
	stiLev5,
	stiLev6,
	stiLev7,
	stiLev8,
	stiLev9,
	stiIndex1,				// 10
	stiIndex2,
	stiIndex3,
	stiIndex4,
	stiIndex5,
	stiIndex6,
	stiIndex7,
	stiIndex8,
	stiIndex9,
	stiToc1,
	stiToc2,				// 20
	stiToc3,
	stiToc4,
	stiToc5,
	stiToc6,
	stiToc7,
	stiToc8,
	stiToc9,
	stiNormIndent,
	stiFtnText,
	stiAtnText,				// 30
	stiHeader,
	stiFooter,
	stiIndexHeading,
	stiCaption,
	stiToCaption,
	stiEnvAddr,
	stiEnvRet,
	stiFtnRef,
	stiAtnRef,
	stiLnn,					// 40
	stiPgn,
	stiEdnRef,
	stiEdnText,
	stiToa,
	stiMacro,
	stiToaHeading,
	stiList,
	stiListBullet,
	stiListNumber,
	stiList2,				// 50
	stiList3,
	stiList4,
	stiList5,
	stiListBullet2,
	stiListBullet3,
	stiListBullet4,
	stiListBullet5,
	stiListNumber2,
	stiListNumber3,
	stiListNumber4,			// 60
	stiListNumber5,
	stiTitle,
	stiClosing,
	stiSignature,
	stiNormalChar,
	stiBodyText,
	stiBodyTextInd,
	stiListCont,
	stiListCont2,
	stiListCont3,			// 70
	stiListCont4,
	stiListCont5,
	stiMsgHeader,
	stiSubtitle,
	stiSalutation,
	stiDate,
	stiBodyText1I,
	stiBodyText1I2,
	stiNoteHeading,
	stiBodyText2,			// 80
	stiBodyText3,
	stiBodyTextInd2,
	stiBodyTextInd3,
	stiBlockQuote,
	stiHyperlink,
	stiHyperlinkFollowed,
	stiStrong,
	stiEmphasis,
	stiNavPane,
	stiPlainText,			// 90
	stiMax,					// 91
	stiUser = 0x0FFE,
	stiNil = 0x0FFF	
};

#define MAX_STYLENAME_LENGTH		255
// Just to save some space, the largest name in the 91
// style names I know is only 24, so set max to just above that.
// If you add a name that's longer, increase this.
#define FAKE_MAX_STYLENAME_LENGTH	32

typedef struct builtinstylenames
{
	char	name[FAKE_MAX_STYLENAME_LENGTH];
} BuiltInStyleNames;

extern BuiltInStyleNames BuiltInStyleNamesTable[stiMax];

extern ushort rgsprmPrm[128];

//extern void 	WriteStyleSheet(TTranslatorStylesTable* styles);
extern void 	GetParaSPRMList(const TTranslatorStyle* style, char* SPRMList, TFontTblWORD* fontTbl, TStyleSheetWORD* styleTblWORD);
extern short 	GetParaSPRMListSize(const TTranslatorStyle* style);

extern void 	GetCharSPRMList(const TTranslatorStyle* style, char* SPRMList, TFontTblWORD* fontTbl, TStyleSheetWORD* styleTblWORD, bool special = false);
extern short 	GetCharSPRMListSize(const TTranslatorStyle* style, bool special = false);

extern void 	GetSPRMList(const TTranslatorStyle* style, char* SPRMList, TFontTblWORD* fontTbl, TStyleSheetWORD* styleTblWORD);
extern short 	GetSPRMListSize(const TTranslatorStyle* style);

extern uint16	SprmOpCode6to8(uint8 opcode6);
extern short	ReadSPRM(TOLEEntryReader *reader, char *buffer, ushort& theSprm, int16);
extern void		ApplyAllSprms(char *sprmBuffer, ushort sprmBytes, TFilterStyleWORD *filterStyle, long styleType, TFontTblWORD *fontTblWORD, TStyleSheetWORD* styleTblWORD, int16 nFib);
extern void		ApplySprm(char *sprmBuffer, ushort sprmBytes, ushort theSprm, TFilterStyleWORD *filterStyle, long styleType, TFontTblWORD *fontTblWORD, TStyleSheetWORD* styleTblWORD, int16 nFib);
void			PostProccessStyle(TFilterStyleWORD *filterStyle);
uint16			STIForName(const char* styleName);

#endif	// __WORDSTYLES_H__