//
//	WORDfont.h
//
#ifndef __WORDFONT_H__
#define __WORDFONT_H__

#include "TranslatorLib.h"

#include "WORDconsts.h"

class TFontWORD;


//-------------------------------------------------------------------
// TFontTblWORD
//-------------------------------------------------------------------

class TFontTblWORD : public TFontTable
{
public:	
					TFontTblWORD();
					TFontTblWORD(TOLEEntryReader *tableReader, long tableStreamSize, long offset, ulong offsetBytes, int16 nFib);
	virtual			~TFontTblWORD();
	
		int32		AddFont(TFontWORD*);
		int32		AddFont(char* fontName);
		int32		Write(TOLEEntryWriter *tableWriter, long &offset, ulong &offsetBytes);
		int32		GetFontIndex(char* name);

		const char*	ConvertFTCtoName(ushort ftc);
		uint32		ConvertFTCtoBeFontID(ushort ftc);
		
private:
	BList *			mFontTableWORD;
	int16			mNFib;
};


//-------------------------------------------------------------------
// TFontWORD
//-------------------------------------------------------------------

class TFontWORD
{
public:	
					TFontWORD() {memset(&mFFN, 0, sizeof(FFN));}
					TFontWORD(TFontTblWORD *fontTbl, TOLEEntryReader *tableReader, uchar bytesFFN, int16 nFib);
	virtual 		~TFontWORD();
	
	int32			Write(TOLEEntryWriter *tableWriter);
	int32			GetBytesFFN();
	
	char *			GetFontName(void)			{return &mFontName[0];}
	uchar			GetFontFamilyCode(void)		{return ((mFFN.bitField & ff) >> 4);}
	uchar			GetFontCharSet(void)		{return mFFN.chs;}
	uint32			GetBeFontID(void)			{return mBeFontID;}
			
	int32			SetFontName(char * name)		{strcpy(mFontName, name); return B_NO_ERROR;}
	int32			SetFontFamilyCode(uchar code)	{mFFN.bitField |= (code << 4); return B_NO_ERROR;}
	int32			SetFontCharSet(uchar code)		{mFFN.chs = code;  return B_NO_ERROR;}
			
private:
	// these are the FFN fields
	FFN				mFFN;	
	char			mFontName[256];	// utf8
	uint32			mBeFontID;		// this is the Be internal family and style id
	int16			mNFib;
};

#endif	// __WORDFONT_H__