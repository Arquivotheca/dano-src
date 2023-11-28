// ============================================================
//  TextViewSupport.h	by Hiroshi Lockheimer
// ============================================================

#ifndef _TEXTVIEWSUPPORT_H
#define _TEXTVIEWSUPPORT_H

#include <TextView.h>
#include <MessageRunner.h>

static const char *kPasswordGlyph = "\xe2\x80\xa2";
static const uint32 kPasswordGlyphUInt32 = 0xe280a200;
static const int kPasswordGlyphLen = sizeof(kPasswordGlyph) - 1;

// I hate macros, let's inline these instead...
inline int32
UTF8CharLen(uchar c)
	{ return (((0xE5000000 >> ((c >> 3) & 0x1E)) & 3) + 1); }

inline bool
IsInitialUTF8Byte(uchar b)	
	{ return ((b & 0xC0) != 0x80); }

inline uint32
UTF8CharToUint32(
	const uchar	*src,
	uint32		srcLen)
{	
	uint32 result = 0;

	for (uint32 i = 0; i < srcLen; i++)
		result |= src[i] << (24 - (i * 8));

	return (result);
}

inline int32 UTF8CountChars(const char *buf, int32 len)
{
	int32 count = 0;
	for(int32 i = 0; i < len; i += UTF8CharLen(buf[i]))
		count++;

	return count;
}


struct _width_entry_ {
	uint32			character;	// UTF-8 character, 0xFFFFFFFF if invalid
	float			escapement; // escapament of character
};

struct _width_table_ {
	BFont			font;		// corresponding font
	int32			hashCount;	// number of hashed items
	int32			tableCount;	// size of table
	_width_entry_*	widths;		// width table	
};

struct STELine {
	int32			offset;		// offset of first character of line
	float			origin;		// pixel position of top of line
	float			ascent;		// maximum ascent for line
	float			width;		// width of string in line
};

struct STEStyleRecord {
	int32			refs;		// reference count for this style
	float			ascent;		// ascent for this style
	float			descent;	// descent for this style
	BFont			font;		// font info
	rgb_color		color;		// color
};

struct STEStyleRunDesc {
	int32			offset;		// byte offset of first character of run
	int32			index;		// index of corresponding style record
};

class _BTextGapBuffer_ {
public:
							_BTextGapBuffer_();
							~_BTextGapBuffer_();
	
	void					InsertText(const char *inText, int32 inNumItems, 
									   int32 inAtIndex);
	void					InsertText(BFile *inFile, int32 inPosition, 
									   int32 inNumItems, int32 inAtIndex);
	void					RemoveRange(int32 start, int32 end);
	
	void					MoveGapTo(int32 toIndex);
	void					SizeGapTo(int32 inCount);

	const char*				GetString(int32 fromOffset, int32 *numChars);
	bool					FindChar(char inChar, int32 fromIndex, int32 *ioDelta);

	const char*				Text();
	const char*				RealText();
	int32					Length() const;
	int32					Chars(int32 to) const;
	char					operator[](int32 index) const;
	char					RealCharAt(int32 index) const;
	uint32					UTF8CharToUint32(int32 offset, int32 charLen) const;

	void					SetPasswordMode(bool password = true);
	bool					PasswordMode() const;

protected:
	int32					mExtraCount;	// when realloc()-ing
	int32					mItemCount;		// logical count
	char*					mBuffer;		// allocated memory
	int32					mBufferCount;	// physical count
	int32					mGapIndex;		// gap position
	int32					mGapCount;		// gap count
	char*					mScratchBuffer;	// for GetString
	int32					mScratchSize;	// scratch size
	bool					mPassword;		// is password mode
};

inline int32
_BTextGapBuffer_::Length() const
	{ return mItemCount; }

inline int32 
_BTextGapBuffer_::Chars(int32 to) const
{
	int32 chars = 0;
	if(to > mItemCount)
		to = mItemCount;
	for(int32 i = 0; i < to; i += UTF8CharLen(RealCharAt(i)))
		chars++;

	return chars;
}

inline char
_BTextGapBuffer_::operator[](int32 index) const
{
	return mPassword ? kPasswordGlyph[index % kPasswordGlyphLen] : RealCharAt(index);
}

inline char
_BTextGapBuffer_::RealCharAt(int32 index) const
{
	return ( (index < mGapIndex) ? mBuffer[index] : 
								   mBuffer[index + mGapCount] );
}

inline uint32
_BTextGapBuffer_::UTF8CharToUint32(
	int32	offset,
	int32	charLen) const
{
	uint32 result = 0;

	if(mPassword)
		result = kPasswordGlyphUInt32;
	else
	{
		for (int32 i = 0; i < charLen; i++)
			result |= ((uchar)(*this)[offset + i]) << (24 - (i * 8));
	}

	return (result);
}
	
	
template<class T> class _BTextViewSupportBuffer_ {
public:
							_BTextViewSupportBuffer_(int32 inExtraCount = 0, 
									  				 int32 inCount = 0);
	virtual					~_BTextViewSupportBuffer_();
					
	void					InsertItemsAt(int32 inNumItems, int32 inAtIndex, const T *inItem);
	void					RemoveItemsAt(int32 inNumItems, int32 inAtIndex);

	int32					ItemCount() const;
	
protected:
	int32					mExtraCount;	
	int32					mItemCount;	
	int32					mBufferCount;	
	T*						mBuffer;		
};	

template<class T>
inline int32
_BTextViewSupportBuffer_<T>::ItemCount() const
	{ return (mItemCount); }
	

class _BWidthBuffer_ : public _BTextViewSupportBuffer_<_width_table_> {
public:
							_BWidthBuffer_();
	virtual					~_BWidthBuffer_();
		
	void					CCheck();

	float					StringWidth(_BTextGapBuffer_	&inText, 
										int32				fromOffset, 
										int32				length,
										const BFont			*inStyle);
	float					StringWidth(const char *inText,
										int32 fromOffset,
										int32 length,
										const BFont *inStyle);

	bool					GetEscapement(uint32	inChar,
										  int32		index, 
										  float		*outEscapement);
	float					HashEscapements(const char	*inChars,
											int32		inNumChars,
											int32		inNumBytes,
											int32		index,
											const BFont	*inFont);	 

	int32					InsertTable(const BFont *inStyle);
	bool					FindTable(const BFont *inStyle, int32 *outIndex);

	uint32					Hash(uint32 inChar);
};


class _BLineBuffer_ : public _BTextViewSupportBuffer_<STELine>{
public:
							_BLineBuffer_();
	
	void					InsertLine(STELine *inLine, int32 index);
	void					RemoveLines(int32 index, int32 count = 1);
	void					RemoveLineRange(int32 fromOffset, int32 toOffset);
	
	int32					OffsetToLine(int32 offset) const;
	int32					PixelToLine(float pixel) const; 
	
	void					BumpOrigin(float delta, int32 index);
	void					BumpOffset(int32 delta, int32 index);
		
	int32					NumLines() const;
	const STELine*			operator[](int32 index) const;
};

inline int32
_BLineBuffer_::NumLines() const
	{ return (mItemCount - 1); }

inline const STELine*
_BLineBuffer_::operator[](int32 index) const
	{ return (&mBuffer[index]); }
	

class _BStyleRunDescBuffer_ : public _BTextViewSupportBuffer_<STEStyleRunDesc> {
public:
							_BStyleRunDescBuffer_();
						
	void					InsertDesc(STEStyleRunDesc *inDesc, int32 index);
	void					RemoveDescs(int32 index, int32 count = 1);

	int32					OffsetToRun(int32 offset) const;
	void					BumpOffset(int32 delta, int32 index);
	
	const STEStyleRunDesc*	operator[](int32 index) const;
};

inline const STEStyleRunDesc*
_BStyleRunDescBuffer_::operator[](int32 index) const
	{ return (&mBuffer[index]); }
	

class _BStyleRecordBuffer_ : public _BTextViewSupportBuffer_<STEStyleRecord> {
public:
							_BStyleRecordBuffer_();
						
	int32					InsertRecord(const BFont *inRecord, 
										 const rgb_color *inColor);
	void					CommitRecord(int32 index);
	void					RemoveRecord(int32 index); 
	
	bool					MatchRecord(const BFont *inRecord, 
										const rgb_color *inColor, int32 *outIndex);

	const STEStyleRecord*	operator[](int32 index) const;
};

inline const STEStyleRecord*
_BStyleRecordBuffer_::operator[](int32 index) const
	{ return (&mBuffer[index]); }
	

class _BStyleBuffer_ {
public:
							_BStyleBuffer_(const BFont *inStyle, 
										   const rgb_color *inColor);
	
	void					InvalidateNullStyle();
	bool					IsValidNullStyle() const;
	
	void					SyncNullStyle(int32 offset);
	void					SetNullStyle(uint32 inMode, const BFont *inStyle, 
										 const rgb_color *inColor, int32 offset = 0);
	void					GetNullStyle(const BFont **inFont, 
										 const rgb_color **inColor) const;	
	
	void					ContinuousGetStyle(BFont *outFont, uint32 *outMode,
											   rgb_color *outColor, 
											   bool *outEqColor,
											   int32 fromOffset, int32 toOffset) const;
	void					SetStyleRange(int32 fromOffset, int32 offset,
									 	  int32 textLen, uint32 inMode, 
										  const BFont *inStyle, 
										  const rgb_color *inColor);
	void					GetStyle(int32 inOffset, BFont *outStyle, 
									 rgb_color *outColor) const;
	text_run_array*			GetStyleRange(int32 startOffset, int32 endOffset) const;
	
	void					RemoveStyleRange(int32 fromOffset, int32 toOffset);
	void					RemoveStyles(int32 index, int32 count = 1);
	
	int32					Iterate(int32 fromOffset, int32 length,
									_BInlineInput_ *inlineState,
									const BFont **outStyle = NULL,
									const rgb_color **outColor = NULL,
									float *outAscent = NULL, float *outDescent = NULL,
									uint32 *inlineFlags = NULL) const;	
	
	int32					OffsetToRun(int32 offset) const;
	void					BumpOffset(int32 delta, int32 index);
	
	void					SetStyle(uint32 mode, const BFont *fromStyle,
									 BFont *toStyle, const rgb_color *fromColor,
									 rgb_color *toColor) const;
							
	text_run				operator[](int32 index) const;
								
	int32					NumRuns() const;

protected:
	_BStyleRunDescBuffer_	mStyleRunDesc;
	_BStyleRecordBuffer_	mStyleRecord;
	bool					mValidNullStyle;
	BFont					mNullFont;
	rgb_color				mNullColor;
};

inline int32
_BStyleBuffer_::NumRuns() const
	{ return (mStyleRunDesc.ItemCount()); }


class _BUndoBuffer_ {
public:
							_BUndoBuffer_(BTextView *inTextView, 
										  undo_state inState = B_UNDO_UNAVAILABLE);
	virtual					~_BUndoBuffer_();

	undo_state				State(bool *isRedo);

	virtual void			Undo(BClipboard *clipboard);
		
protected:
	virtual void			UndoSelf(BClipboard *clipboard);
	virtual void			RedoSelf(BClipboard *clipboard);

protected:
	BTextView*				mTextView;
	undo_state				mState;
	bool					mRedo;
	bool					mStyled;
	int32					mSelStart;
	int32					mSelEnd;
	char*					mDeletedText;
	int32					mDeletedTextLen;
	text_run_array*			mDeletedStyle;
	int32					mDeletedStyleLen;
};


class _BCutUndoBuffer_ : public _BUndoBuffer_ {
public:
							_BCutUndoBuffer_(BTextView *inTextView);

protected:
	virtual void			RedoSelf(BClipboard *clipboard);
};


class _BPasteUndoBuffer_ : public _BUndoBuffer_ {
public:
							_BPasteUndoBuffer_(BTextView *inTextView,
											  const char* inInsertedText,
											  int32 inInsertedTextLen,
											  text_run_array* styles, int32 styleLen);
	virtual					~_BPasteUndoBuffer_();

protected:
	virtual void			UndoSelf(BClipboard *clipboard);
	virtual void			RedoSelf(BClipboard *clipboard);

protected:
	char*					mPastedText;
	int32					mPastedTextLen;
	text_run_array*			mPastedStyle;
	int32					mPastedStyleLen;
};


class _BDropUndoBuffer_ : public _BUndoBuffer_ {
public:
							_BDropUndoBuffer_(BTextView *inTextView, const char* inInsertedText,
											  int32 inInsertedTextLen,
											  text_run_array* styles, int32 styleLen,
											  int32 inDropOffset, bool inSameWindow);
	virtual					~_BDropUndoBuffer_();

protected:
	virtual void			UndoSelf(BClipboard *clipboard);
	virtual void			RedoSelf(BClipboard *clipboard);

protected:
	char*					mInsertedText;
	int32					mInsertedTextLen;
	text_run_array*			mInsertedStyle;
	int32					mInsertedStyleLen;
	int32					mDropOffset;
	bool					mSameWindow;
};


class _BClearUndoBuffer_ : public _BUndoBuffer_ {
public:
							_BClearUndoBuffer_(BTextView *inTextView);

protected:
	virtual void			RedoSelf(BClipboard *clipboard);
};


class _BTypingUndoBuffer_ : public _BUndoBuffer_ {
public:
							_BTypingUndoBuffer_(BTextView *inTextView);
	virtual					~_BTypingUndoBuffer_();
	
	void					InputACharacter(int32 inGlyphWidth = 1);
	void					InputCharacters(int32 inHowMany);
	void					BackwardErase();
	void					ForwardErase();
	void					Reset();

protected:
	virtual void			UndoSelf(BClipboard *clipboard);
	virtual void			RedoSelf(BClipboard *clipboard);

protected:
	char*					mTypedText;
	int32					mTypingStart;
	int32					mTypingEnd;
	int32					mUndoCount;
};



enum { 
	kInlineNoClause			= 0x00000000,
	kInlineInClause			= 0x00000001,
	kInlineClauseEnd		= 0x00000002,
	kInlineSelectedClause	= 0x00000004
};


class _BInlineInput_ {
public:
							_BInlineInput_(BMessenger method);
	virtual					~_BInlineInput_();

	void					SetOffset(int32 offset);
	int32					Offset() const;

	void					SetLength(int32 length);
	int32					Length() const;

	void					ResetClauses();
	void					AddClause(int32 clauseStart, int32 clauseEnd);
	void					CommitClauses(int32 selStart, int32 selEnd);

	void					GetClause(int32 index, int32 *clauseStart, int32 *clauseEnd, uint32 *flags) const;
	int32					NumClauses() const;

	void					SetActive(bool active);
	bool					IsActive() const;

	const BMessenger*		Method() const;

private:
	BMessenger				fMethod;
	int32					fOffset;
	int32					fLength;
	int32					fSelStart;
	int32					fSelEnd;
	int32					fNumClauses;
	int32*					fClauses;
	bool					fClausesCommited;
	bool					fActive;
};

class _BTextTrackState_
{
public:
	_BTextTrackState_(BMessenger nonAsyncTarget);
	~_BTextTrackState_();

	void		SimulateMouseEvent(BTextView* in);
	
	int32		fMouseOffset;
	bool		fShiftDown;
	
	BRect		fDragRect;
	BPoint		fLastPoint;
	
	int32		fStart;
	int32		fEnd;
	int32		fAnchor;
	int32		fTextLen;
	
private:
	BMessage fPulseMsg;
	BMessageRunner fAsyncPulse;
};

#endif /* _TEXTVIEWSUPPORT_H */
