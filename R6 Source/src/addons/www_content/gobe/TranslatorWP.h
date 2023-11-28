//*****************************************************************************************************
// TranslatorWP.h
//
//	This document contains the definition of classes for use when reading or writing wordprocessing
// parts from a translator document.
//*****************************************************************************************************

#ifndef __TRANSLATORWP_H__
#define __TRANSLATORWP_H__

#include "TranslatorDoc.h"

class TSpecialCharacter;
class TTranslatorStyle;

//*****************************************************************************************************
// TTranslatorPart_WP
//*****************************************************************************************************
class TTranslatorPart_WP : public TTranslatorPart
{
	public:
		TTranslatorPart_WP(TTranslatorDoc* docPtr);
		virtual ~TTranslatorPart_WP();
		
		virtual status_t			WriteData(TBlockStreamWriter* writer) const;
		virtual bool				ReadBlock(TBlockStreamReader* reader, int32 id);
		virtual int32				PartKind(void) const;

		int32						TextBlocks(void) const;
		const char*					TextBlock(int32 index, ssize_t* textLength) const;
		void						AddText(const char* textPtr, ssize_t textLength);
		ssize_t						TotalLength(void) const;
		
		int32						ParagraphStyleRuns(void) const;
		int32						CharacterStyleRuns(void) const;
		const TTranslatorStyle*		ParagraphStylePtr(int32 index, int32* start, int32* end) const;
		const TTranslatorStyle*		CharacterStylePtr(int32 index, int32* start, int32* end) const;		

		// Takes over ownership of the stylePtr. Assume stylePtr can change, use accessor for further access.
		void						AddParagraphStyleRun(int32 start, int32 end, TTranslatorStyle* stylePtr);
		void						AddCharacterStyleRun(int32 start, int32 end, TTranslatorStyle* stylePtr);
	
		int32						SpecialCharacters(void) const;
		const TSpecialCharacter*	SpecialCharacter(int32 index) const;
		void						AddSpecialCharacter(TSpecialCharacter* scPtr);
		TSpecialCharacter*			CreateSpecialCharacter(int32 kind) const;
		
		// These will probably become depracted when we introduce sections to the api.
		int32						NumberOfColumns(void) const;
		void						SetNumberOfColumns(int32 numCols);
				
	protected:
		status_t 					ReadStyleRun(int32* start, int32* end, TTranslatorStyle** stylePtr);
		status_t					ReadSpecialCharacter(void);
		
	private:
		TBlockStreamReader*			mReader;

		int32						mTotalLength;
		int32						mNumColumns;	
		BList						mTextBlocks;
		BList						mTextBlockSizes;		
		BList						mParagraphStyles;
		BList						mParagraphStarts;
		BList						mParagraphEnds;
		BList						mCharacterStyles;
		BList						mCharacterStarts;
		BList						mCharacterEnds;
		BList						mSpecialChars;
};

//*****************************************************************************************************
// TSpecialCharacter
//*****************************************************************************************************
class TSpecialCharacter
{
	public:
		TSpecialCharacter();
		virtual ~TSpecialCharacter();
		
		status_t				Write(TBlockStreamWriter* writer, TTranslatorDoc* docPtr) const;
		
		int32					Offset(void) const				{ return mOffset; }
		void					SetOffset(int32 offset) 		{ mOffset = offset; }
		
		virtual status_t		WriteData(TBlockStreamWriter* writer, TTranslatorDoc* docPtr) const;
		virtual status_t		ReadData(TBlockStreamReader* reader, TTranslatorDoc* docPtr);
		virtual bool			ReadBlock(TBlockStreamReader* reader, int32 id, TTranslatorDoc* docPtr);
		
		virtual int32			Kind(void) const				{ return kSCUnknown; }
		
		const char*				Text(void) const;
		void					SetText(const char* text);
		
	private:
		int32					mOffset;
		char*					mText;
};

//*****************************************************************************************************
// TSpecialCharacter_Frame
//*****************************************************************************************************
class TSpecialCharacter_Frame : public TSpecialCharacter
{
	public:
		TSpecialCharacter_Frame();
		virtual ~TSpecialCharacter_Frame();
		
		virtual status_t		WriteData(TBlockStreamWriter* writer, TTranslatorDoc* docPtr) const;
		virtual bool			ReadBlock(TBlockStreamReader* reader, int32 id, TTranslatorDoc* docPtr);
		virtual int32			Kind(void) const				{ return kSCFrame; }
		
		const TTranslatorPart*	FramePart(void) const			{ return mPartPtr; }
		const TTranslatorStyle*	FrameStyle(void) const			{ return mStylePtr; }
		const BBitmap*			FrameImage(void) const			{ return mBitmapPtr; }

		void					SetFramePart(TTranslatorPart* partPtr);
		void					SetFrameStyle(TTranslatorStyle* stylePtr);
		void					SetFrameImage(BBitmap* bitmapPtr);
		
	private:
		status_t				ReadFrame(TBlockStreamReader* reader, TTranslatorDoc* docPtr);
	
		TTranslatorPart*		mPartPtr;
		TTranslatorStyle*		mStylePtr;
		BBitmap*				mBitmapPtr;
};

//*****************************************************************************************************
// TSpecialCharacter_Footnote
//*****************************************************************************************************
class TSpecialCharacter_Footnote : public TSpecialCharacter
{
	public:
		TSpecialCharacter_Footnote();
		virtual ~TSpecialCharacter_Footnote();
		
		virtual status_t		WriteData(TBlockStreamWriter* writer, TTranslatorDoc* docPtr) const;
		virtual bool			ReadBlock(TBlockStreamReader* reader, int32 id, TTranslatorDoc* docPtr);
		virtual int32			Kind(void) const							{ return kSCFootnote; }
		
		bool					AutoNumber(void) const						{ return mAutoNumber; }
		bool					IsEndnote(void) const						{ return mIsEndnote; }
		const char* 			CustomMark(void) const						{ return mCustomMark; }
		const TTranslatorPart*	ContentPart(void) const						{ return mPartPtr; }

		void					SetAutoNumber(bool autoNumber)				{ mAutoNumber = autoNumber; }
		void					SetIsEndnote(bool isEndNote)				{ mIsEndnote = isEndNote; }
		void					SetCustomMark(const char* customMarkStr);
		void					SetContentPart(TTranslatorPart* partPtr);

	private:	
		TTranslatorPart*		mPartPtr;
		bool					mAutoNumber;
		bool					mIsEndnote;
		char*					mCustomMark;
};

//*****************************************************************************************************
// TSpecialCharacter_Formula
//*****************************************************************************************************
class TSpecialCharacter_Formula : public TSpecialCharacter
{
	public:
		TSpecialCharacter_Formula();
		virtual ~TSpecialCharacter_Formula();
		
		virtual status_t		WriteData(TBlockStreamWriter* writer, TTranslatorDoc* docPtr) const;
		virtual bool			ReadBlock(TBlockStreamReader* reader, int32 id, TTranslatorDoc* docPtr);
		virtual int32			Kind(void) const	{ return kSCFormula; }
		
		void					SetFormula(const char* formulaStr);
		const char*				Formula(void) const;
		void					SetFormulaResult(const char* resultStr);
		const char*				FormulaResult(void) const;
		void					SetFormatString(const char* formatStr);
		const char*				FormatString(void) const;
		
	private:
		char*					mFormula;
		char*					mFormulaResult;
		char*					mFormatString;
};

//*****************************************************************************************************
// TSpecialCharacter_Break
//*****************************************************************************************************
class TSpecialCharacter_Break : public TSpecialCharacter
{
	public:
		enum { kPageBreak, kFrameBreak, kLineBreak, kSectionBreak };
		
	public:
		TSpecialCharacter_Break();
		virtual ~TSpecialCharacter_Break();
		
		virtual status_t		WriteData(TBlockStreamWriter* writer, TTranslatorDoc* docPtr) const;
		virtual bool			ReadBlock(TBlockStreamReader* reader, int32 id, TTranslatorDoc* docPtr);
		virtual int32			Kind(void) const				{ return kSCBreak; }
		
		void					SetBreakKind(int32 breakKind);
		int32					BreakKind(void) const			{ return mBreakKind; }
		
	private:
		int32					mBreakKind;
};

//*****************************************************************************************************
// TSpecialCharacter_PageNumber
//*****************************************************************************************************
class TSpecialCharacter_PageNumber : public TSpecialCharacter
{
	public:
		TSpecialCharacter_PageNumber();
		virtual ~TSpecialCharacter_PageNumber();
		
		virtual status_t		WriteData(TBlockStreamWriter* writer, TTranslatorDoc* docPtr) const;
		virtual int32			Kind(void) const				{ return kSCPageNumber; }
};

//*****************************************************************************************************
// TSpecialCharacter_Rule
//*****************************************************************************************************
class TSpecialCharacter_Rule : public TSpecialCharacter
{
	public:
		TSpecialCharacter_Rule();
		virtual ~TSpecialCharacter_Rule();
		
		virtual status_t		WriteData(TBlockStreamWriter* writer, TTranslatorDoc* docPtr) const;
		virtual int32			Kind(void) const	{ return kSCHorizontalRule; }
};

//*****************************************************************************************************
// TTextIterator_WP
//*****************************************************************************************************
class TTextIterator_WP
{
	public:
		TTextIterator_WP(const TTranslatorPart_WP* target);
		~TTextIterator_WP();
		
		int32						NextTextRun(uchar* buffer, int32 maxBufferSize);
		const TTranslatorStyle*		ParagraphStylePtr(void) const;
		const TTranslatorStyle*		CharacterStylePtr(void) const;		
		const TSpecialCharacter*	SpecialCharacter(void) const;
		int32						RunStart(void) const;
		bool						IsParStart(void) const;
		bool						IsParEnd(void) const;
		bool						IsSpecialCharacter(void) const;
		
	private:
		const TTranslatorPart_WP*	mPartPtr;
		int32						mTextBlockIndex;
		int32						mTextBlockOffset;
		int32						mTextOffset;
		int32						mParStyleIndex;
		int32						mCharStyleIndex;
		int32						mSpecialCharIndex;
		int32						mLastTextOffset;
		bool						mIsParStart;
		bool						mIsParEnd;
		bool						mIsSpecialCharacter;
};


#endif // __TRANSLATORWP_H__

// $Header: /usr/local/cvsroot/8ball/Datatypes/TranslatorWP.h,v 1.1 1999/12/20 19:24:20 tom Exp $

