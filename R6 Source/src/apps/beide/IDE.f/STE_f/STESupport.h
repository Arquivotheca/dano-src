// ============================================================
//  STESupport.h	©1996 Hiroshi Lockheimer
// ============================================================
// 	STE Version 1.0a5

#ifndef STESUPPORT_H
#define STESUPPORT_H

#include "STE.h"
#include <Debug.h>


class BLocker;

class STETextBuffer {
public:
							STETextBuffer();
							~STETextBuffer();
	
	void					InsertText(const char *inText, int32 inNumItems, int32 inAtIndex);
	void					RemoveRange(int32 start, int32 end);
	
	void					MoveGapTo(int32 toIndex);
	void					SizeGapTo(int32 inCount);

	const char*				GetString(int32 fromOffset, int32 numChars);
	bool					FindChar(char inChar, int32 fromIndex, int32 *ioDelta);

	const char*				Text();
	int32					Length() const;
	uchar					operator[](int32 index) const;
	
protected:
	int32					mExtraCount;	// when realloc()-ing
	int32					mItemCount;		// logical count
	char*					mBuffer;		// allocated memory
	int32					mBufferCount;	// physical count
	int32					mGapIndex;		// gap position
	int32					mGapCount;		// gap count
	char*					mScratchBuffer;	// for GetString
	int32					mScratchSize;	// scratch size
};

inline int32
STETextBuffer::Length() const
	{ return (mItemCount); }

inline uchar
STETextBuffer::operator[](int32 index) const
{
//	ASSERT( index <= mItemCount );
	return ( (index < mGapIndex) ? mBuffer[index] : 
								   mBuffer[index + mGapCount] );
}

template<class T> class STEPODBuffer {
public:
							STEPODBuffer(int32 inExtraCount = 16, 
									  int32 inCount = 0);
							~STEPODBuffer();
					
	void					InsertItemsAt(int32 inNumItems, int32 inAtIndex, const T *inItem);
	void					RemoveItemsAt(int32 inNumItems, int32 inAtIndex);

	int32					ItemCount() const;
	void					ReallocBuffer(int32 inNewSize);

protected:
	int32					mExtraCount;	
	int32					mItemCount;	
	int32					mBufferCount;	
	T*						mBuffer;		
};	

template<class T>
inline int32
STEPODBuffer<T>::ItemCount() const
	{ return (mItemCount); }


template<class T> class STEBuffer {
public:
							STEBuffer(int32 inExtraCount = 16, 
									  int32 inCount = 0);
							~STEBuffer();
					
	void					InsertItemsAt(int32 inNumItems, int32 inAtIndex, const T *inItem);
	void					RemoveItemsAt(int32 inNumItems, int32 inAtIndex);

	int32					ItemCount() const;
	void					ReallocBuffer(int32 inNewSize);

protected:
	int32					mExtraCount;	
	int32					mItemCount;	
	int32					mBufferCount;	
	T*						mBuffer;		
};	

template<class T>
inline int32
STEBuffer<T>::ItemCount() const
	{ return (mItemCount); }


inline int32
UTF8CharLen(uchar c)
	{ return (((0xE5000000 >> ((c >> 3) & 0x1E)) & 3) + 1); }

inline bool
IsInitialUTF8Byte(uchar b)	
	{ return ((b & 0xC0) != 0x80); }

union UTF8_char {
	char			bytes[4];	// UTF-8 char as an array of bytes
	uint32			code;		// UTF-8 char as a uint32 (reverse order)
};

struct STEWidthEntry {
	UTF8_char		character;	// UTF-8 character, 0xFFFFFFFF if invalid
	float			escapement; // escapament of character
};

struct STEWidthTable {
	BFont			font;		// corresponding font
	int32			hashCount;	// number of hashed items
	int32			tableCount;	// size of table
	STEWidthEntry*	widths;		// width table	
};

class STEWidthBuffer : public STEPODBuffer<STEWidthTable> {
public:
							STEWidthBuffer();
							~STEWidthBuffer();
		
	void					CCheck();

	float					StringWidth(
											STETextBuffer		&inText, 
											int32				fromOffset, 
											int32				length,
											const BFont			*inStyle);
	float					StringWidth(
											const char			*inText,
											int32				fromOffset,
											int32				length,
											const BFont			*inStyle);

	uint32					Hash(UTF8_char inChar);

private:

	static BLocker			fLocker;

	int32					InsertTable(const BFont *inStyle);
	bool					FindTable(const BFont *inStyle, int32 *outIndex);
	float					GetEscapement(
											UTF8_char			inChar,
											int32				inLength, 
											int32				index, 
											const BFont	*		inStyle);
};


class STELineBuffer : public STEPODBuffer<STELine>{
public:
							STELineBuffer();
							~STELineBuffer();
	
	void					InsertLine(STELinePtr inLine, int32 index);
	void					RemoveLines(int32 index, int32 count = 1);
	void					RemoveLineRange(int32 fromOffset, int32 toOffset);
	
	int32					OffsetToLine(int32 offset) const;
	int32					PixelToLine(float pixel) const; 
	
	void					BumpOrigin(float delta, int32 index);
	void					BumpOffset(int32 delta, int32 index);
		
	int32					NumLines() const;
	ConstSTELinePtr			operator[](int32 index) const;
};

inline int32
STELineBuffer::NumLines() const
	{ return (mItemCount - 1); }

inline ConstSTELinePtr
STELineBuffer::operator[](int32 index) const
{ 
	ASSERT( index < mItemCount );
	return (&mBuffer[index]); 
}
	

class STEStyleRunDescBuffer : public STEPODBuffer<STEStyleRunDesc> {
public:
							STEStyleRunDescBuffer();
						
	void					InsertDesc(STEStyleRunDescPtr inDesc, int32 index);
	void					RemoveDescs(int32 index, int32 count = 1);

	int32					OffsetToRun(int32 offset) const;
	void					BumpOffset(int32 delta, int32 index);
	
	ConstSTEStyleRunDescPtr	operator[](int32 index) const;
};

inline ConstSTEStyleRunDescPtr
STEStyleRunDescBuffer::operator[](int32 index) const
{ 
	ASSERT( index < mItemCount );

	return (&mBuffer[index]); 
}
	

class STEStyleRecordBuffer : public STEBuffer<STEStyleRecord> {
public:
							STEStyleRecordBuffer();
						
	int32					InsertRecord(ConstSTEStylePtr inRecord);
	void					CommitRecord(int32 index);
	void					RemoveRecord(int32 index); 
	
	bool					MatchRecord(ConstSTEStylePtr inRecord, int32 *outIndex);
	
	ConstSTEStyleRecordPtr	operator[](int32 index) const;
};

inline ConstSTEStyleRecordPtr
STEStyleRecordBuffer::operator[](int32 index) const
{ 
	ASSERT( index < mItemCount );

	return (&mBuffer[index]); 
}
	

class STEStyleBuffer {
public:
							STEStyleBuffer(ConstSTEStylePtr inStyle);
	
	void					InvalidateNullStyle();
	bool					IsValidNullStyle() const;
	
	void					SyncNullStyle(int32 offset);
	void					SetNullStyle(uint32 inMode, ConstSTEStylePtr inStyle, 
										 int32 offset = 0);
	STEStyle				GetNullStyle() const;	
	
	bool					IsContinuousStyle(uint32 *ioMode, STEStylePtr outStyle,
											  int32 fromOffset, int32 toOffset) const;
	void					SetStyleRange(int32 fromOffset, int32 toOffset,
									 	  int32 textLen, uint32 inMode, 
										  ConstSTEStylePtr inStyle);
	void					GetStyle(int32 inOffset, STEStylePtr outStyle) const;
	STEStyleRangePtr		GetStyleRange(int32 startOffset, int32 endOffset) const;
	
	void					RemoveStyleRange(int32 fromOffset, int32 toOffset);
	void					RemoveStyles(int32 index, int32 count = 1);
	
	int32					Iterate(int32 fromOffset, int32 length,
									ConstSTEStylePtr *outStyle = NULL,
									float *outAscent = NULL, float *outDescent = NULL) const;	
	
	int32					OffsetToRun(int32 offset) const;
	void					BumpOffset(int32 delta, int32 index);
	
	void					SetStyle(uint32 mode, ConstSTEStylePtr fromStyle,
									 STEStylePtr toStyle) const;
							
	STEStyleRun				operator[](int32 index) const;
								
	int32					NumRuns() const;
	
	// use these with caution!
	const STEStyleRunDescBuffer&	RunBuffer() const;
	const STEStyleRecordBuffer&		RecordBuffer() const;

protected:
	void					InvalidateCache();


	STEStyleRunDescBuffer	mStyleRunDesc;
	STEStyleRecordBuffer	mStyleRecord;
	bool					mValidNullStyle;
	STEStyle				mNullStyle;
	
	mutable int32					mFromOffset;
	mutable int32					mNextOffset;
	mutable int32					mRunIndex;
};

inline int32
STEStyleBuffer::NumRuns() const
	{ return (mStyleRunDesc.ItemCount()); }

inline const STEStyleRunDescBuffer&
STEStyleBuffer::RunBuffer() const
	{ return (mStyleRunDesc); }
	
inline const STEStyleRecordBuffer&
STEStyleBuffer::RecordBuffer() const
	{ return (mStyleRecord); }

#endif
