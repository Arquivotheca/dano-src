// ============================================================
//  TextViewSupport.cpp	by Hiroshi Lockheimer
// ============================================================

#include <Clipboard.h>
#include <Debug.h>
#include <malloc.h>
#include <memory.h>
#include <File.h>
#include <View.h>
#include <Window.h>
#include <TextViewSupport.h>
#include <TypeConstants.h>

#include <new.h>

_BTextGapBuffer_::_BTextGapBuffer_()
{
	mExtraCount = 2048;		// must be greater than 0
	mItemCount = 0;
	mBuffer = (char *)malloc(mExtraCount + mItemCount);
	mBufferCount = mExtraCount + mItemCount;
	mGapIndex = mItemCount;
	mGapCount = mBufferCount - mGapIndex;
	mScratchBuffer = (char *)malloc(0);
	mScratchSize = 0;
	mPassword = false;
}

_BTextGapBuffer_::~_BTextGapBuffer_()
{
	free(mBuffer);
	free(mScratchBuffer);
}

void
_BTextGapBuffer_::InsertText(
	const char	*inText,
	int32 		inNumItems,
	int32 		inAtIndex)
{
	if (inNumItems < 1)
		return;

	inAtIndex = (inAtIndex > mItemCount) ? mItemCount : inAtIndex;
	inAtIndex = (inAtIndex < 0) ? 0 : inAtIndex;
		
	if (mGapIndex != inAtIndex) 
		MoveGapTo(inAtIndex);
	
	if (mGapCount < inNumItems)
		SizeGapTo(inNumItems + mExtraCount);
		
	memcpy(mBuffer + mGapIndex, inText, inNumItems);

	mGapCount -= inNumItems;
	mGapIndex += inNumItems;
	mItemCount += inNumItems;
}

void
_BTextGapBuffer_::InsertText(
	BFile	*inFile,
	int32	inPosition,
	int32	inNumItems, 
	int32	inAtIndex)
{
	if (inNumItems < 1)
		return;
	
	inAtIndex = (inAtIndex > mItemCount) ? mItemCount : inAtIndex;
	inAtIndex = (inAtIndex < 0) ? 0 : inAtIndex;
		
	if (mGapIndex != inAtIndex) 
		MoveGapTo(inAtIndex);
	
	if (mGapCount < inNumItems)
		SizeGapTo(inNumItems + mExtraCount);
		
	inFile->ReadAt(inPosition, mBuffer + mGapIndex, inNumItems);	

	mGapCount -= inNumItems;
	mGapIndex += inNumItems;
	mItemCount += inNumItems;
}

void
_BTextGapBuffer_::RemoveRange(
	int32	start,
	int32 	end)
{ 
	int32 atIndex = start;
	atIndex = (atIndex > mItemCount) ? mItemCount : atIndex;
	atIndex = (atIndex < 0) ? 0 : atIndex;
	
	int32 numItems = end - atIndex;
	numItems = (numItems > mItemCount - atIndex) ? (mItemCount - atIndex) : numItems;
	
	if (numItems < 1)
		return;
	
	if (mGapIndex != atIndex)
		MoveGapTo(atIndex);
	
	mGapCount += numItems;
	mItemCount -= numItems;
	
	if (mGapCount > mExtraCount)
		SizeGapTo(mExtraCount / 2);
}

void
_BTextGapBuffer_::MoveGapTo(
	int32	toIndex)
{
	if (mGapCount > 0) {
		int32 srcIndex = 0;
		int32 dstIndex = 0;
		int32 count = 0;
	
		if (toIndex > mGapIndex) {
			int32 gapEndIndex = mGapIndex + mGapCount;
			int32 trailGapCount = mBufferCount - gapEndIndex;
			srcIndex = gapEndIndex;
			dstIndex =  mGapIndex;
			count = toIndex - mGapIndex;
			count = (count > trailGapCount) ? trailGapCount : count;
		}
		else {
			srcIndex = toIndex;
			dstIndex = toIndex + mGapCount;
			count = mGapIndex - toIndex;
		}
	
		if (count > 0)
			memmove(mBuffer + dstIndex, mBuffer + srcIndex, count);	
	}

	mGapIndex = toIndex;
}

void
_BTextGapBuffer_::SizeGapTo(
	int32	inCount)
{
	bool smaller = inCount < mGapCount;
	
	if (smaller)
		memmove(mBuffer + mGapIndex + inCount,
				mBuffer + mGapIndex + mGapCount,
				mBufferCount - (mGapIndex + mGapCount));
			
	mBuffer = (char *)realloc(mBuffer, mItemCount + inCount);
	
	if (!smaller)
		memmove(mBuffer + mGapIndex + inCount, 
				mBuffer + mGapIndex + mGapCount, 
				mBufferCount - (mGapIndex + mGapCount));

	mGapCount = inCount;
	mBufferCount = mItemCount + mGapCount;
}

const char*
_BTextGapBuffer_::GetString(
	int32	fromOffset,
	int32 	*numChars)
{
	char *result = "";
	
	if (*numChars < 1)
		return (result);

	if(mPassword)
	{
		int32 chars = 0;
		for(int32 i = 0; i < *numChars; i += UTF8CharLen(RealCharAt(fromOffset + i)))
			chars++;

		if (mScratchSize < chars * kPasswordGlyphLen) {
			mScratchBuffer = (char *)realloc(mScratchBuffer, chars * kPasswordGlyphLen);
			mScratchSize = chars * kPasswordGlyphLen;
		}

		for(int32 i = 0; i < chars; i++)
			memcpy(mScratchBuffer + i * kPasswordGlyphLen, kPasswordGlyph, kPasswordGlyphLen);
		*numChars = chars * kPasswordGlyphLen;
		result = mScratchBuffer;
	}
	else
	{
		bool isStartBeforeGap = (fromOffset < mGapIndex);
		bool isEndBeforeGap = ((fromOffset + *numChars - 1) < mGapIndex);

		if (isStartBeforeGap == isEndBeforeGap) {
			result = mBuffer + fromOffset;
			if (!isStartBeforeGap)
				result += mGapCount;
		}
		else {
			if (mScratchSize < *numChars) {
				mScratchBuffer = (char *)realloc(mScratchBuffer, *numChars);
				mScratchSize = *numChars;
			}

			for (int32 i = 0; i < *numChars; i++)
				mScratchBuffer[i] = (*this)[fromOffset + i];

			result = mScratchBuffer;
		}
	}

	return (result);
}

bool
_BTextGapBuffer_::FindChar(
	char	inChar,
	int32	fromIndex,
	int32	*ioDelta)
{
	int32 numChars = *ioDelta;
	for (int32 i = 0; i < numChars; i++) {
		if ((*this)[fromIndex + i] == inChar) {
			*ioDelta = i;
			return (TRUE);
		}
	}
	
	return (FALSE);
}

const char*
_BTextGapBuffer_::RealText()
{
	if (mGapIndex != mItemCount)
		MoveGapTo(mItemCount);
	
	if (mGapCount < 1)
		SizeGapTo(mExtraCount);

	mBuffer[mItemCount] = '\0';
	
	return (mBuffer);
}

const char*
_BTextGapBuffer_::Text()
{
	if(mPassword)
	{
		if(mScratchSize < mItemCount * kPasswordGlyphLen + 1) {
			mScratchBuffer = (char *)realloc(mScratchBuffer, mItemCount * kPasswordGlyphLen + 1);
			mScratchSize = mItemCount * kPasswordGlyphLen + 1;
		}

		for(int32 i = 0; i < mItemCount; i++)
			memcpy(mScratchBuffer + i * kPasswordGlyphLen, kPasswordGlyph, kPasswordGlyphLen);
		mScratchBuffer[mItemCount * kPasswordGlyphLen] = '\0';
		return mScratchBuffer;
	}
	else
		return RealText();
}

void 
_BTextGapBuffer_::SetPasswordMode(bool password)
{
	mPassword = password;
}

bool 
_BTextGapBuffer_::PasswordMode() const
{
	return mPassword;
}

template<class T>
_BTextViewSupportBuffer_<T>::_BTextViewSupportBuffer_(
	int32	inExtraCount,
	int32	inCount)
{
	mExtraCount = inExtraCount;
	mItemCount = inCount;
	mBufferCount = mItemCount + mExtraCount;
	mBuffer = (T *)calloc(mBufferCount, sizeof(T));
	if (mBuffer) {
		for (int32 i=0; i<mBufferCount; i++)
			new(&mBuffer[i]) T();
	}
}

template<class T>
_BTextViewSupportBuffer_<T>::~_BTextViewSupportBuffer_()
{
	if (mBuffer) {
		for (int32 i=0; i<mBufferCount; i++)
			mBuffer[i].~T();
		free(mBuffer);
	}
}

template<class T>
void
_BTextViewSupportBuffer_<T>::InsertItemsAt(
	int32	inNumItems,
	int32 	inAtIndex,
	const T	*inItem)
{
	if (inNumItems < 1)
		return;
	
	inAtIndex = (inAtIndex > mItemCount) ? mItemCount : inAtIndex;
	inAtIndex = (inAtIndex < 0) ? 0 : inAtIndex;

	if ((inNumItems + mItemCount) >= mBufferCount) {
		mBufferCount = mItemCount + inNumItems + mExtraCount;
		mBuffer = (T *)realloc(mBuffer, mBufferCount * sizeof(T));
	}
	
	T *loc = mBuffer + inAtIndex;
	memmove(loc + inNumItems, loc, (mItemCount - inAtIndex) * sizeof(T));
	for (int32 i=0; i<inNumItems; i++)
		new(&loc[i]) T(inItem[i]);
	
	mItemCount += inNumItems;
}

template<class T>
void
_BTextViewSupportBuffer_<T>::RemoveItemsAt(
	int32	inNumItems,
	int32 	inAtIndex)
{
	if (inNumItems < 1)
		return;
	
	inAtIndex = (inAtIndex >= mItemCount) ? (mItemCount - 1) : inAtIndex;
	inAtIndex = (inAtIndex < 0) ? 0 : inAtIndex;
	
	T *loc = mBuffer + inAtIndex;
	
	for (int32 i=0; i<inNumItems; i++)
		loc[i].~T();
	memmove(loc, loc + inNumItems, 
			(mItemCount - (inNumItems + inAtIndex)) * sizeof(T));
	
	mItemCount -= inNumItems;
	
	if ((mBufferCount - mItemCount) > mExtraCount) {
		mBufferCount = mItemCount + mExtraCount;
		mBuffer = (T *)realloc(mBuffer, mBufferCount * sizeof(T));
	}
}

_BWidthBuffer_::_BWidthBuffer_()
	: _BTextViewSupportBuffer_<_width_table_>()
{
}

_BWidthBuffer_::~_BWidthBuffer_()
{
	for (int32 i = 0; i < mItemCount; i++)
		free(mBuffer[i].widths);
}

void
_BWidthBuffer_::CCheck()
{
	if (mItemCount > 0) {
		int32 cCount = 0;
		for (int32 i = 0; i < mBuffer[0].tableCount; i++) {
			if (mBuffer[0].widths[i].character != 0xFFFFFFFF) {
				if ((uint32)i != (Hash(mBuffer[0].widths[i].character) & 
						  (mBuffer[0].tableCount - 1))) {
					//printf("collision %c\n", 
					//	   mBuffer[0].widths[i].character.bytes[0]);
					cCount++;
				}
			}
		}
		printf("hashCount = %ld, tableCount = %ld, %ld collisions, %f\n", 
			   mBuffer[0].hashCount, mBuffer[0].tableCount, cCount, 
			   ((float)cCount) / ((float)mBuffer[0].hashCount) * 100.0);
	}
}

float
_BWidthBuffer_::StringWidth(
	_BTextGapBuffer_	&inText, 
	int32				fromOffset, 
	int32				length,
	const BFont			*inStyle)
{
	if (length < 1)
		return (0.0);
			
	int32 index = 0;
	if (!FindTable(inStyle, &index))
		index = InsertTable(inStyle);
	
	float	result = 0.0;
	float	conversion = inStyle->Size();
	char	*newChars = NULL;
	int32	newCharsLen = 0;
	int32	newCharsCount = 0;

	int32 i = 0;
	
	int32	localOffset = fromOffset;
	const bool passwordMode = inText.PasswordMode();
	if(passwordMode)
		// convert offset in real string to offset in password string
		localOffset = inText.Chars(fromOffset) * kPasswordGlyphLen;
		
	while (i < length) {
		int32	charLen = UTF8CharLen(inText.RealCharAt(fromOffset + i));
		uchar	firstByte = passwordMode
						  ? kPasswordGlyph[0]
						  : inText[localOffset + i];
		int32	localLen = UTF8CharLen(firstByte);
		if (i + charLen > length)
			break;

		// tab widths are defined by the user (not the font) and
		// newlines are special
		if ((firstByte != '\t') && (firstByte != '\n')) {
			uint32	theChar = passwordMode
							? kPasswordGlyphUInt32
							: (inText.UTF8CharToUint32(localOffset + i, localLen));
			float	escapement = 0.0;

			if (GetEscapement(theChar, index, &escapement))
				// character was cached
				result += escapement * conversion;
			else {
				// character wasn't cached, store it, and send it off  
				// to the app_server all at once at the end 
				newChars = (char *)realloc(newChars, newCharsLen + localLen);

				for (int32 j = 0; j < localLen; j++) {
					newChars[newCharsLen + j] = passwordMode
											  ? kPasswordGlyph[j]
											  : inText[localOffset + i + j];
				}

				newCharsLen += localLen;
				newCharsCount++;
			}
		}

		i += charLen;
	}

	if (newChars != NULL) {
		// get the escapements from the app_server
		result += HashEscapements(newChars, newCharsCount, newCharsLen, 
								  index, inStyle) * conversion;
		free(newChars);
	}

	return (result);
}

// This method uses an explicitly passed string, so it's not concerned with
// password mode
float
_BWidthBuffer_::StringWidth(
	const char	*inText, 
	int32		fromOffset, 
	int32		length,
	const BFont	*inStyle)
{
	if ((inText == NULL) || (length < 1))
		return (0.0);
			
	int32 index = 0;
	if (!FindTable(inStyle, &index))
		index = InsertTable(inStyle);
	
	float	result = 0.0;
	float	conversion = inStyle->Size();
	char	*newChars = NULL;
	int32	newCharsLen = 0;
	int32	newCharsCount = 0;
	int32	i = 0;
	while (i < length) {
		int32 charLen = UTF8CharLen(inText[fromOffset + i]);
		if (i + charLen > length)
			break;

		uint32	theChar = UTF8CharToUint32((uchar *)inText + fromOffset + i, charLen);

		float escapement = 0.0;
		if (GetEscapement(theChar, index, &escapement)) 
			// character was cached
			result += escapement * conversion;
		else {	
			// character wasn't cached, store it, and send it off  
			// to the app_server all at once at the end 
			newChars = (char *)realloc(newChars, newCharsLen + charLen);

			for (int32 j = 0; j < charLen; j++)
				newChars[newCharsLen + j] = inText[fromOffset + i + j];	

			newCharsLen += charLen;
			newCharsCount++;
		}

		i += charLen;
	}

	if (newChars != NULL) {
		// get the escapements from the app_server
		result += HashEscapements(newChars, newCharsCount, newCharsLen, 
								  index, inStyle) * conversion;
		free(newChars);
	}

	return (result);
}

bool
_BWidthBuffer_::GetEscapement(
	uint32	inChar,
	int32	index,
	float	*outEscapement)
{	
	_width_table_	*table = &mBuffer[index];
	_width_entry_	*widths = table->widths;	
	uint32			key = Hash(inChar) & (table->tableCount - 1);

	while ( (widths[key].character != inChar) &&
			(widths[key].character != 0xFFFFFFFF) ) {
		key++;
		if (key >= (uint32)table->tableCount)
			key = 0;
	} 

	if (widths[key].character == inChar) {
		// found the character, we're done
		*outEscapement = widths[key].escapement;
		return (TRUE);
	}

	return (FALSE);
}

float
_BWidthBuffer_::HashEscapements(
	const char	*inChars,
	int32		inNumChars,
	int32		inNumBytes,
	int32		index,
	const BFont	*inFont)
{
	float result = 0.0;
	float *escapements = (float *)malloc(sizeof(float) * inNumChars);

	inFont->GetEscapements(inChars, inNumChars, escapements);

	_width_table_	*table = &mBuffer[index];
	_width_entry_	*widths = table->widths;
	int32			curCharNum = 0;	
	int32			i = 0;
	while (i < inNumBytes) {
		int32 charLen = UTF8CharLen(inChars[i]);
		if (i + charLen > inNumBytes)
			break;

		uint32 theChar = UTF8CharToUint32((uchar *)inChars + i, charLen);
		uint32 key = Hash(theChar) & (table->tableCount - 1);

		while ( (widths[key].character != theChar) &&
				(widths[key].character != 0xFFFFFFFF) ) {
			key++;
			if (key >= (uint32)table->tableCount)
				key = 0;
		} 

		if (widths[key].character == theChar)
			// oops, this character is already hashed...  
			goto NextIteration;

		widths[key].character = theChar;
		widths[key].escapement = escapements[curCharNum];
		table->hashCount++;

		if ((((float)table->hashCount) / ((float)table->tableCount)) >= 0.70) {
			// we're >= 70% full, time to resize
			int32 saveTableCount = table->tableCount;
			table->hashCount = 0;
			table->tableCount = saveTableCount * 2;
			table->widths = (_width_entry_ *)malloc(sizeof(_width_entry_) *
													table->tableCount);

			// initialize the new table
			for (int32 j = 0; j < table->tableCount; j++) {
				table->widths[j].character = 0xFFFFFFFF;
				table->widths[j].escapement = 0.0;
			}

			// rehash the data in the old table
			uint32 hashMask = table->tableCount - 1;
			for (int32 j = 0; j < saveTableCount; j++) {
				if (widths[j].character != 0xFFFFFFFF) {
					uint32 key = Hash(widths[j].character) & hashMask;

					while (table->widths[key].character != 0xFFFFFFFF) {
						key++;
						if (key >= (uint32)table->tableCount)
							key = 0;
					}

					table->widths[key].character = widths[j].character;
					table->widths[key].escapement = widths[j].escapement;
					table->hashCount++;
				}
			}

			free(widths);
			widths = table->widths;
		}

NextIteration:
		result += escapements[curCharNum++];
		i += charLen; 
	}

	free(escapements);

	return (result);
}

int32
_BWidthBuffer_::InsertTable(
	const BFont	*inStyle)
{
	int32 index = mItemCount;

	_width_table_ table;
	table.font = *inStyle;
	table.hashCount = 0;
	table.tableCount = 128;		// must be power of two			
	table.widths = (_width_entry_ *)malloc(sizeof(_width_entry_) * 
										   table.tableCount);

	for (int i = 0; i < table.tableCount; i++) {
		table.widths[i].character = 0xFFFFFFFF;
		table.widths[i].escapement = 0.0;
	}

	InsertItemsAt(1, index, &table);

	return (index);
}

bool
_BWidthBuffer_::FindTable(
	const BFont	*inStyle, 
	int32		*outIndex)
{
	for (int32 i = 0; i < mItemCount; i++) {
		if ((mBuffer[i].font == *inStyle)) {
			*outIndex = i;
			return (TRUE);
		}
	}
	
	return (FALSE);
}

uint32
_BWidthBuffer_::Hash(
	uint32	inChar)
{
	return ( ((3 * (inChar >> 24)) + (inChar >> 15)) ^ 
			 ((inChar >> 6) - ((inChar >> 24) * 22)) ^ 
			 (inChar << 3) );
}

_BLineBuffer_::_BLineBuffer_()
	: _BTextViewSupportBuffer_<STELine>(20, 2)
{
}
	
void
_BLineBuffer_::InsertLine(
	STELine	*inLine,
	int32	index)
{
	InsertItemsAt(1, index, inLine);
}

void
_BLineBuffer_::RemoveLines(
	int32	index,
	int32	count)
{
	RemoveItemsAt(count, index);
}

void
_BLineBuffer_::RemoveLineRange(
	int32	fromOffset,
	int32 	toOffset)
{
	int32 fromLine = OffsetToLine(fromOffset);
	int32 toLine = OffsetToLine(toOffset);

	int32 count = toLine - fromLine;
	if (count > 0)
		RemoveLines(fromLine + 1, count);

	BumpOffset(fromOffset - toOffset, fromLine + 1);
}

int32
_BLineBuffer_::OffsetToLine(
	int32	offset) const
{
	int32 minIndex = 0;
	int32 maxIndex = mItemCount - 1;
	int32 index = 0;
	
	while (minIndex < maxIndex) {
		index = (minIndex + maxIndex) >> 1;
		if (offset >= mBuffer[index].offset) {
			if (offset < mBuffer[index + 1].offset)
				break;
			else
				minIndex = index + 1;
		}
		else
			maxIndex = index;
	}
	
	return (index);
}

int32
_BLineBuffer_::PixelToLine(
	float	pixel) const
{
	int32 minIndex = 0;
	int32 maxIndex = mItemCount - 1;
	int32 index = 0;
	
	while (minIndex < maxIndex) {
		index = (minIndex + maxIndex) >> 1;
		if (pixel >= mBuffer[index].origin) {
			if (pixel < mBuffer[index + 1].origin)
				break;
			else
				minIndex = index + 1;
		}
		else
			maxIndex = index;
	}
	
	return (index);
} 

void
_BLineBuffer_::BumpOrigin(
	float	delta,
	int32	index)
{	
	for (int32 i = index; i < mItemCount; i++)
		mBuffer[i].origin += delta;
}

void
_BLineBuffer_::BumpOffset(
	int32	delta,
	int32 	index)
{
	for (int32 i = index; i < mItemCount; i++)
		mBuffer[i].offset += delta;
}

_BStyleRunDescBuffer_::_BStyleRunDescBuffer_()
	: _BTextViewSupportBuffer_<STEStyleRunDesc>(20)
{
}

void
_BStyleRunDescBuffer_::InsertDesc(
	STEStyleRunDesc	*inDesc,
	int32			index)
{
	InsertItemsAt(1, index, inDesc);
}

void
_BStyleRunDescBuffer_::RemoveDescs(
	int32	index,
	int32 	count)
{
	RemoveItemsAt(count, index);
}

int32
_BStyleRunDescBuffer_::OffsetToRun(
	int32	offset) const
{
	if (mItemCount <= 1)
		return (0);
		
	int32 minIndex = 0;
	int32 maxIndex = mItemCount;
	int32 index = 0;
	
	while (minIndex < maxIndex) {
		index = (minIndex + maxIndex) >> 1;
		if (offset >= mBuffer[index].offset) {
			if (index >= (mItemCount - 1)) {
				break;
			}
			else {
				if (offset < mBuffer[index + 1].offset)
					break;
				else
					minIndex = index + 1;
			}
		}
		else
			maxIndex = index;
	}
	
	return (index);
}

void
_BStyleRunDescBuffer_::BumpOffset(
	int32	delta,
	int32 	index)
{
	for (int32 i = index; i < mItemCount; i++)
		mBuffer[i].offset += delta;
}

_BStyleRecordBuffer_::_BStyleRecordBuffer_()
	: _BTextViewSupportBuffer_<STEStyleRecord>()
{
}

int32
_BStyleRecordBuffer_::InsertRecord(
	const BFont		*inStyle,
	const rgb_color	*inColor)
{
	int32 index = 0;

	// look for style in buffer
	if (MatchRecord(inStyle, inColor, &index))
		return (index);

	// style not found, add it
	font_height fontHeight;
	inStyle->GetHeight(&fontHeight);
	fontHeight.ascent = ceil(fontHeight.ascent);
	fontHeight.descent = ceil(fontHeight.descent);
	fontHeight.leading = ceil(fontHeight.leading);

	// check if there's any unused space
	for (index = 0; index < mItemCount; index++) {
		if (mBuffer[index].refs < 1) {
			mBuffer[index].refs = 0;
			mBuffer[index].ascent = fontHeight.ascent;
			mBuffer[index].descent = fontHeight.descent + fontHeight.leading;
			mBuffer[index].font = *inStyle;
			mBuffer[index].color = *inColor;
			return (index);
		}	
	}
	
	// no unused space, expand the buffer
	index = mItemCount;
	STEStyleRecord newRecord;
	newRecord.refs = 0;
	newRecord.ascent = fontHeight.ascent;
	newRecord.descent = fontHeight.descent + fontHeight.leading;
	newRecord.font = *inStyle;
	newRecord.color = *inColor;
	InsertItemsAt(1, index, &newRecord);

	return (index);
}

void
_BStyleRecordBuffer_::CommitRecord(
	int32	index)
{
	mBuffer[index].refs++;
}

void
_BStyleRecordBuffer_::RemoveRecord(
	int32	index)
{
	mBuffer[index].refs--;
}

bool
_BStyleRecordBuffer_::MatchRecord(
	const BFont		*inRecord,
	const rgb_color	*inColor,
	int32			*outIndex)
{
	for (int32 i = 0; i < mItemCount; i++) {
		if ( (mBuffer[i].font == *inRecord) &&
			 (mBuffer[i].color.red == inColor->red) &&
			 (mBuffer[i].color.green == inColor->green) &&
			 (mBuffer[i].color.blue == inColor->blue) &&
			 (mBuffer[i].color.alpha == inColor->alpha) ) {
			*outIndex = i;
			return (TRUE);
		}
	}

	return (FALSE);
}


_BStyleBuffer_::_BStyleBuffer_(
	const BFont		*inStyle,
	const rgb_color	*inColor)
{
	mValidNullStyle = TRUE;
	mNullFont = *inStyle;
	mNullColor = *inColor;
}

void
_BStyleBuffer_::InvalidateNullStyle()
{
	if (mStyleRunDesc.ItemCount() < 1)
		return;
		
	mValidNullStyle = FALSE;
}

bool
_BStyleBuffer_::IsValidNullStyle() const
{
	return (mValidNullStyle);
}

void
_BStyleBuffer_::SyncNullStyle(
	int32	offset)
{
	if ((mValidNullStyle) || (mStyleRunDesc.ItemCount() < 1))
		return;
	
	int32 index = OffsetToRun(offset);
	mNullFont = mStyleRecord[mStyleRunDesc[index]->index]->font;
	mNullColor = mStyleRecord[mStyleRunDesc[index]->index]->color;

	mValidNullStyle = TRUE;
}	

void
_BStyleBuffer_::SetNullStyle(
	uint32			inMode,
	const BFont		*inStyle,
	const rgb_color	*inColor,
	int32			offset)
{
	if ((mValidNullStyle) || (mStyleRunDesc.ItemCount() < 1))
		SetStyle(inMode, inStyle, &mNullFont, inColor, &mNullColor);
	else {
		int32 index = OffsetToRun(offset - 1);
		mNullFont = mStyleRecord[mStyleRunDesc[index]->index]->font;
		mNullColor = mStyleRecord[mStyleRunDesc[index]->index]->color;
		SetStyle(inMode, inStyle, &mNullFont, inColor, &mNullColor);	
	}
	
	mValidNullStyle = TRUE;
}

void
_BStyleBuffer_::GetNullStyle(
	const BFont		**outFont,
	const rgb_color	**outColor) const
{
	*outFont = &mNullFont;
	*outColor = &mNullColor;
}	

void
_BStyleBuffer_::ContinuousGetStyle(
	BFont		*outFont,
	uint32		*outMode,
	rgb_color	*outColor,
	bool		*outEqColor,
	int32		fromOffset,
	int32		toOffset) const
{	
	BFont		oFont;
	uint32		oMode = B_FONT_ALL;
	rgb_color	oColor;
	bool		oEqColor = TRUE;

	if (mStyleRunDesc.ItemCount() < 1) {
		SetStyle(B_FONT_ALL, &mNullFont, &oFont, &mNullColor, &oColor);
		oMode = B_FONT_ALL;
		oEqColor = TRUE;
	}
	else {
		int32 fromIndex = OffsetToRun(fromOffset);
		int32 toIndex = OffsetToRun(toOffset - 1);
	
		if (fromIndex == toIndex) {		
			int32			styleIndex = mStyleRunDesc[fromIndex]->index;
			const BFont		*style = &mStyleRecord[styleIndex]->font;
			const rgb_color	*color = &mStyleRecord[styleIndex]->color;
		
			SetStyle(B_FONT_ALL, style, &oFont, color, &oColor);
			oMode = B_FONT_ALL;
			oEqColor = TRUE;
		}
		else {
			int32			styleIndex = mStyleRunDesc[toIndex]->index;
			BFont			theStyle = mStyleRecord[styleIndex]->font;
			rgb_color		theColor = mStyleRecord[styleIndex]->color;
			const BFont		*style = NULL;
			const rgb_color	*color = NULL;
		
			for (int32 i = fromIndex; i < toIndex; i++) {
				styleIndex = mStyleRunDesc[i]->index;
				style = &mStyleRecord[styleIndex]->font;
				color = &mStyleRecord[styleIndex]->color;

				if (theStyle.Compare(*style, B_FONT_FAMILY_AND_STYLE) == 0)
					oMode &= ~B_FONT_FAMILY_AND_STYLE;	
				
				if (theStyle.Size() != style->Size())
					oMode &= ~B_FONT_SIZE;
				
				if (theStyle.Shear() != style->Shear())
					oMode &= ~B_FONT_SHEAR;
		
				if ( (theColor.red != color->red) ||
					 (theColor.green != color->green) ||
					 (theColor.blue != color->blue) ||
					 (theColor.alpha != color->alpha) )
					oEqColor = FALSE;
			}
		
			SetStyle(oMode, &theStyle, &oFont, &theColor, &oColor);
		}
	}

	if (outFont != NULL)
		*outFont = oFont;
	if (outMode != NULL)
		*outMode = oMode;
	if (outColor != NULL)
		*outColor = oColor;
	if (outEqColor != NULL)
		*outEqColor = oEqColor;
}

void
_BStyleBuffer_::SetStyleRange(
	int32			fromOffset,
	int32			toOffset,
	int32			textLen,
	uint32			inMode,
	const BFont		*inStyle,
	const rgb_color	*inColor)
{
	if (fromOffset == toOffset) {
		SetNullStyle(inMode, inStyle, inColor, fromOffset);
		return;
	}
	
	if (mStyleRunDesc.ItemCount() < 1) {
		if (inStyle == NULL)
			inStyle = &mNullFont;
		if (inColor == NULL)
			inColor = &mNullColor;

		STEStyleRunDesc newDesc;
		newDesc.offset = fromOffset;
		newDesc.index = mStyleRecord.InsertRecord(inStyle, inColor);
		mStyleRunDesc.InsertDesc(&newDesc, 0);
		mStyleRecord.CommitRecord(newDesc.index);
		return;
	}

	int32 styleIndex = 0;
	int32 offset = fromOffset;
	int32 runIndex = OffsetToRun(offset);
	do {
		STEStyleRunDesc	runDesc = *mStyleRunDesc[runIndex];
		int32			runEnd = textLen;
		if (runIndex < (mStyleRunDesc.ItemCount() - 1))
			runEnd = mStyleRunDesc[runIndex + 1]->offset;
		
		BFont		style = mStyleRecord[runDesc.index]->font;
		rgb_color	color = mStyleRecord[runDesc.index]->color;
		SetStyle(inMode, inStyle, &style, inColor, &color);

		styleIndex = mStyleRecord.InsertRecord(&style, &color);
		
		if ( (runDesc.offset == offset) && (runIndex > 0) && 
			 (mStyleRunDesc[runIndex - 1]->index == styleIndex) ) {
			RemoveStyles(runIndex);
			runIndex--;	
		}

		if (styleIndex != runDesc.index) {
			if (offset > runDesc.offset) {
				STEStyleRunDesc newDesc;
				newDesc.offset = offset;
				newDesc.index = styleIndex;
				mStyleRunDesc.InsertDesc(&newDesc, runIndex + 1);
				mStyleRecord.CommitRecord(newDesc.index);
				runIndex++;
			}
			else {
				((STEStyleRunDesc *)mStyleRunDesc[runIndex])->index = styleIndex;
				mStyleRecord.CommitRecord(styleIndex);
			}
				
			if (toOffset < runEnd) {
				STEStyleRunDesc newDesc;
				newDesc.offset = toOffset;
				newDesc.index = runDesc.index;
				mStyleRunDesc.InsertDesc(&newDesc, runIndex + 1);
				mStyleRecord.CommitRecord(newDesc.index);
			}
		}
		
		runIndex++;
		offset = runEnd;	
	} while (offset < toOffset);
	
	if ( (offset == toOffset) && (runIndex < mStyleRunDesc.ItemCount()) &&
		 (mStyleRunDesc[runIndex]->index == styleIndex) )
		RemoveStyles(runIndex);
}		 

void
_BStyleBuffer_::GetStyle(
	int32		inOffset,
	BFont		*outStyle,
	rgb_color	*outColor) const
{
	if (mStyleRunDesc.ItemCount() < 1) {
		if (outStyle != NULL)
			*outStyle = mNullFont;
		if (outColor != NULL)
			*outColor = mNullColor;
		return;
	}
	
	int32 runIndex = OffsetToRun(inOffset);
	int32 styleIndex = mStyleRunDesc[runIndex]->index;
	if (outStyle != NULL)
		*outStyle = mStyleRecord[styleIndex]->font;
	if (outColor != NULL)
		*outColor = mStyleRecord[styleIndex]->color;
}

text_run_array*
_BStyleBuffer_::GetStyleRange(
	int32	startOffset,
	int32	endOffset) const
{
	text_run_array *result = NULL;
	
	int32 startIndex = OffsetToRun(startOffset);
	int32 endIndex = OffsetToRun(endOffset);
	
	int32 numStyles = endIndex - startIndex + 1;
	numStyles = (numStyles < 1) ? 1 : numStyles;
	result = BTextView::AllocRunArray(numStyles);

	text_run *run = &result->runs[0];
	for (int32 index = 0; index < numStyles; index++) {
		*run = (*this)[startIndex + index];
		run->offset -= startOffset;
		run->offset = (run->offset < 0) ? 0 : run->offset;
		run++;
	}
	
	return (result);
}

void
_BStyleBuffer_::RemoveStyleRange(
	int32	fromOffset,
	int32	toOffset)
{
	int32 fromIndex = mStyleRunDesc.OffsetToRun(fromOffset);
	int32 toIndex = mStyleRunDesc.OffsetToRun(toOffset) - 1;

	int32 count = toIndex - fromIndex;
	if (count > 0) {
		RemoveStyles(fromIndex + 1, count);
		toIndex = fromIndex;
	}
	
	mStyleRunDesc.BumpOffset(fromOffset - toOffset, fromIndex + 1);

	if ((toIndex == fromIndex) && (toIndex < (mStyleRunDesc.ItemCount() - 1)))
		((STEStyleRunDesc *)mStyleRunDesc[toIndex + 1])->offset = fromOffset;
	
	if (fromIndex < (mStyleRunDesc.ItemCount() - 1)) {
		const STEStyleRunDesc *runDesc = mStyleRunDesc[fromIndex];
		if (runDesc->offset == (runDesc + 1)->offset) {
			RemoveStyles(fromIndex);
			fromIndex--;
		}
	}
	
	if ((fromIndex >= 0) && (fromIndex < (mStyleRunDesc.ItemCount() - 1))) {
		const STEStyleRunDesc *runDesc = mStyleRunDesc[fromIndex];
		if (runDesc->index == (runDesc + 1)->index)
			RemoveStyles(fromIndex + 1);
	}
}

void
_BStyleBuffer_::RemoveStyles(
	int32	index,
	int32 	count)
{
	for (int32 i = index; i < (index + count); i++)
		mStyleRecord.RemoveRecord(mStyleRunDesc[i]->index);
		
	mStyleRunDesc.RemoveDescs(index, count);
}

int32
_BStyleBuffer_::Iterate(
	int32			fromOffset,
	int32			length,
	_BInlineInput_	*inlineState,
	const BFont		**outStyle,
	const rgb_color	**outColor,
	float			*outAscent,
	float			*outDescent,
	uint32			*inlineFlags) const
{
	int32 numRuns = mStyleRunDesc.ItemCount();
	if ((length < 1) || (numRuns < 1))
		return (0);

	int32 					result = length;
	int32 					runIndex = mStyleRunDesc.OffsetToRun(fromOffset);
	const STEStyleRunDesc	*run = mStyleRunDesc[runIndex];

	if (outStyle != NULL)
		*outStyle = &mStyleRecord[run->index]->font;
	if (outColor != NULL)
		*outColor = &mStyleRecord[run->index]->color;
	if (outAscent != NULL)
		*outAscent = mStyleRecord[run->index]->ascent;
	if (outDescent != NULL)
		*outDescent = mStyleRecord[run->index]->descent;
	
	if (runIndex < (numRuns - 1)) {
		int32 nextOffset = (run + 1)->offset - fromOffset;
		result = (result > nextOffset) ? nextOffset : result;
	}

	if ((inlineState != NULL) && (inlineFlags != NULL)) {
		*inlineFlags = kInlineNoClause;

		int32 numClauses = inlineState->NumClauses();
		if (numClauses > 0) {
			int32 inlineOffset = inlineState->Offset();
			int32 inlineLength = inlineState->Length();
			int32 resultOffset = result + fromOffset;
	
			if ((fromOffset < (inlineOffset + inlineLength)) && (resultOffset > inlineOffset)) {
				int32	clauseStart = 0;
				int32	clauseEnd = 0;
				uint32	flags = kInlineNoClause;
				inlineState->GetClause(0, &clauseStart, &clauseEnd, &flags);

				if ((fromOffset < clauseStart) && (resultOffset > clauseStart))
					result = clauseStart - fromOffset;
				else {
					for (int32 i = numClauses - 1; i >= 0; i--) {
						inlineState->GetClause(i, &clauseStart, &clauseEnd, &flags);
		
						if (fromOffset >= clauseStart) {
							result = (resultOffset > clauseEnd) ? clauseEnd - fromOffset : result;
							*inlineFlags = flags;
							break;
						}
					}
				}
			}
		}
	}
	
	return (result);
}

int32
_BStyleBuffer_::OffsetToRun(
	int32	offset) const
{
	return (mStyleRunDesc.OffsetToRun(offset));
}

void
_BStyleBuffer_::BumpOffset(
	int32	delta,
	int32	index)
{
	mStyleRunDesc.BumpOffset(delta, index);
}

void
_BStyleBuffer_::SetStyle(
	uint32			mode,
	const BFont		*fromStyle,
	BFont			*toStyle,
	const rgb_color	*fromColor,
	rgb_color		*toColor) const
{
	if ((fromStyle != NULL) && (toStyle != NULL)) {
		toStyle->SetTo(*fromStyle, mode);
	}
	
	if ((fromColor != NULL) && (toColor != NULL))
		*toColor = *fromColor;
}

text_run
_BStyleBuffer_::operator[](
	int32	index) const
{
	text_run run;
	
	if (mStyleRunDesc.ItemCount() < 1) {
		run.offset = 0;
		run.font = mNullFont;
		run.color = mNullColor;
	} else {
		const STEStyleRunDesc	*runDesc = mStyleRunDesc[index];
		const STEStyleRecord	*record = mStyleRecord[runDesc->index];
		run.offset = runDesc->offset;
		run.font = record->font;
		run.color = record->color;
	}
	
	return (run);
}


_BUndoBuffer_::_BUndoBuffer_(
	BTextView	*inTextView,
	undo_state	inState)
{
	mTextView = inTextView;
	mState = inState;
	mRedo = false;
	mStyled = inTextView->IsStylable();
	mTextView->GetSelection(&mSelStart, &mSelEnd);
	mDeletedTextLen = mSelEnd - mSelStart;
	mDeletedText = (char *)malloc(mDeletedTextLen);
	memcpy(mDeletedText, mTextView->Text() + mSelStart, mDeletedTextLen);
	mDeletedStyleLen = 0;
	if( mStyled ) {
		mDeletedStyle = mTextView->RunArray(mSelStart, mSelEnd,
											&mDeletedStyleLen);
	} else {
		mDeletedStyle = NULL;
	}
}

_BUndoBuffer_::~_BUndoBuffer_()
{
	free(mDeletedStyle);
	free(mDeletedText);
}

void
_BUndoBuffer_::Undo(
	BClipboard	*clipboard)
{
	if (mRedo)
		RedoSelf(clipboard);
	else
		UndoSelf(clipboard);

	mRedo = !mRedo;
}

undo_state
_BUndoBuffer_::State(
	bool	*isRedo)
{
	*isRedo = mRedo;
	return (mState);
}
		
void
_BUndoBuffer_::UndoSelf(
	BClipboard	*)
{
	// Restore deleted text
	mTextView->Select(mSelStart, mSelStart);
	mTextView->Insert(mDeletedText, mDeletedTextLen, mDeletedStyle);

	// Restore original selection
	mTextView->Select(mSelStart, mSelEnd);
}

void
_BUndoBuffer_::RedoSelf(
	BClipboard	*)
{
}


_BCutUndoBuffer_::_BCutUndoBuffer_(
	BTextView	*inTextView)
		: _BUndoBuffer_(inTextView, B_UNDO_CUT)
{
}

void
_BCutUndoBuffer_::RedoSelf(
	BClipboard	*clipboard)
{
	// Delete selected text
	mTextView->Select(mSelStart, mSelStart);
	mTextView->Delete(mSelStart, mSelEnd);

	// Put deleted text on clipboard
	if (clipboard->Lock()) {
		clipboard->Clear();
		clipboard->Data()->AddData("text/plain", B_MIME_TYPE, mDeletedText, mDeletedTextLen);
		if (mDeletedStyle) {
			clipboard->Data()->AddData("application/x-vnd.Be-text_run_array", 
							  B_MIME_TYPE, mDeletedStyle, mDeletedStyleLen);
		}
		clipboard->Commit();
		clipboard->Unlock();
	}
}


_BPasteUndoBuffer_::_BPasteUndoBuffer_(
	BTextView	*inTextView,
	const char	*inPastedText,
	int32		inPastedTextLen, 
	text_run_array *inPastedStyle,
	int32		inPastedStyleLen)
		: _BUndoBuffer_(inTextView, B_UNDO_PASTE)
{
	mPastedText = NULL;
	mPastedTextLen = inPastedTextLen;
	mPastedStyle = NULL;
	mPastedStyleLen = inPastedStyleLen;

	mPastedText = (char *)malloc(mPastedTextLen);
	memcpy(mPastedText, inPastedText, mPastedTextLen);
	if( inPastedStyle ) {
		mPastedStyle = BTextView::CopyRunArray(inPastedStyle);
	}
}

_BPasteUndoBuffer_::~_BPasteUndoBuffer_()
{
	BTextView::FreeRunArray(mPastedStyle);
	free(mPastedText);
}

void
_BPasteUndoBuffer_::UndoSelf(
	BClipboard	*)
{
	// Delete text that was pasted
	mTextView->Select(mSelStart, mSelStart);
	mTextView->Delete(mSelStart, mSelStart + mPastedTextLen);
	
	// Restore text deleted by the paste
	mTextView->Insert(mDeletedText, mDeletedTextLen, mDeletedStyle);

	// Restore selection
	mTextView->Select(mSelStart, mSelEnd);
}

void
_BPasteUndoBuffer_::RedoSelf(
	BClipboard	*)
{
	// Delete selected text
	mTextView->Select(mSelStart, mSelStart);
	mTextView->Delete(mSelStart, mSelEnd);
	
	// Restore the text that was on the clipboard
	mTextView->Insert(mPastedText, mPastedTextLen, mPastedStyle);
	mTextView->Select(mSelStart, mSelStart + mPastedTextLen);
}


_BDropUndoBuffer_::_BDropUndoBuffer_(
	BTextView	*inTextView, 
	const char	*inInsertedText,
	int32		inInsertedTextLen,
	text_run_array	*inInsertedStyle,
	int32		inInsertedStyleLen,
	int32		inDropOffset,
	bool		inSameWindow)
		: _BUndoBuffer_(inTextView, B_UNDO_DROP)
{
	//ASSERT(inInsertedTextLen > 0);
	//ASSERT(! inTextView.Window()->IsActive() || ! (inDropOffset > fSelStart && inDropOffset < fSelEnd));

	// Save the text from the drag message
	mInsertedText = NULL;
	mInsertedTextLen = inInsertedTextLen;
	mInsertedStyle = NULL;
	mInsertedStyleLen = inInsertedStyleLen;
	mDropOffset = inDropOffset;
	mSameWindow = inSameWindow;

	mInsertedText = (char *)malloc(mInsertedTextLen);
	memcpy(mInsertedText, inInsertedText, mInsertedTextLen);
	if( inInsertedStyle ) {
		mInsertedStyle = BTextView::CopyRunArray(inInsertedStyle);
	}
}

_BDropUndoBuffer_::~_BDropUndoBuffer_()
{
	BTextView::FreeRunArray(mInsertedStyle);
	free(mInsertedText);
}

void
_BDropUndoBuffer_::UndoSelf(
	BClipboard	*)
{
	int32 dropOffset = mDropOffset;

	// Delete text that was dropped
	mTextView->Select(dropOffset, dropOffset);
	mTextView->Delete(dropOffset, dropOffset + mInsertedTextLen);
	
	// Restore the text that was moved
	if (mSameWindow) {
		mTextView->Select(mSelStart, mSelStart);
		mTextView->Insert(mDeletedText, mDeletedTextLen, mDeletedStyle);
	}

	// Restore selection
	mTextView->Select(mSelStart, mSelEnd);
}

void
_BDropUndoBuffer_::RedoSelf(
	BClipboard	*)
{
	int32 dropOffset = mDropOffset;

	// Delete the text that was moved
	if (mSameWindow) {
		mTextView->Select(mSelStart, mSelStart);
		mTextView->Delete(mSelStart, mSelEnd);
	}

	// Restore the text that was dropped
	mTextView->Select(dropOffset, dropOffset);
	mTextView->Insert(mInsertedText, mInsertedTextLen, mInsertedStyle);
	mTextView->Select(dropOffset, dropOffset + mInsertedTextLen);
}


_BClearUndoBuffer_::_BClearUndoBuffer_(
	BTextView	*inTextView)
		: _BUndoBuffer_(inTextView, B_UNDO_CLEAR)
{
}

void
_BClearUndoBuffer_::RedoSelf(
	BClipboard	*)
{
	// Delete text that was uncleared
	mTextView->Select(mSelStart, mSelStart);
	mTextView->Delete(mSelStart, mSelEnd);
}


_BTypingUndoBuffer_::_BTypingUndoBuffer_(
	BTextView	*inTextView)
		: _BUndoBuffer_(inTextView, B_UNDO_TYPING)
{
	mTypedText = NULL;
	mTypingStart = mTypingEnd = mSelStart;
	mUndoCount = 0;
}

_BTypingUndoBuffer_::~_BTypingUndoBuffer_()
{
	free(mTypedText);
}

void
_BTypingUndoBuffer_::InputACharacter(
	int32	inGlyphWidth)
{
	int32 selStart = 0;
	int32 selEnd = 0;

	mTextView->GetSelection(&selStart, &selEnd);

	if ((mTypingEnd != selStart) || (mTypingEnd != selEnd)) 								
		// Selection has changed. Start a fresh typing sequence
		Reset(); 
	
	mTypingEnd += inGlyphWidth;
}

void
_BTypingUndoBuffer_::InputCharacters(
	int32	inHowMany)
{
	int32 selStart = 0;
	int32 selEnd = 0;
	int32 typingEnd = mTypingEnd + inHowMany;

	mTextView->GetSelection(&selStart, &selEnd);

	if ((typingEnd != selStart) || (typingEnd != selEnd))
		// Selection has changed. Start a fresh typing sequence
		Reset(); 

	mTypingEnd += inHowMany;
}

void
_BTypingUndoBuffer_::BackwardErase()
{
	int32 selStart = 0;
	int32 selEnd = 0;

	mTextView->GetSelection(&selStart, &selEnd);

	int32 glyphWidth = UTF8CharLen(mTextView->ByteAt(selStart - 1));

	if ((mTypingEnd != selStart) || (mTypingEnd != selEnd)) {
		// Selection has changed. Start a fresh typing sequence
		Reset(); 

		if (selStart != selEnd) 
			mTypingEnd += glyphWidth;
		else {
			// Deleting before beginning of typing
			int32	newDeletedTextLen = mDeletedTextLen + glyphWidth;
			char	*deletedText = (char *)malloc(newDeletedTextLen);
			memcpy(deletedText + glyphWidth, mDeletedText, mDeletedTextLen);
	
			mDeletedTextLen = newDeletedTextLen;
			mTypingStart = selStart - glyphWidth;
			selStart = mTypingStart;
	
			for (int32 i = 0; i < glyphWidth; i++)
				deletedText[i] = mTextView->ByteAt(selStart++);

			free(mDeletedText);
			mDeletedText = deletedText;			
		}
	}
	else {
		if (mTypingStart >= selStart) {
			// Deleting before beginning of typing
			int32	newDeletedTextLen = mDeletedTextLen + glyphWidth;
			char	*deletedText = (char *)malloc(newDeletedTextLen);
			memcpy(deletedText + glyphWidth, mDeletedText, mDeletedTextLen);
	
			mDeletedTextLen = newDeletedTextLen;
			mTypingStart = selStart - glyphWidth;
			selStart = mTypingStart;
	
			for (int32 i = 0; i < glyphWidth; i++)
				deletedText[i] = mTextView->ByteAt(selStart++);
	
			free(mDeletedText);
			mDeletedText = deletedText;
		}
	}	

	mTypingEnd -= glyphWidth;
}

void
_BTypingUndoBuffer_::ForwardErase()
{
	int32 selStart = 0;
	int32 selEnd = 0;

	mTextView->GetSelection(&selStart, &selEnd);

	int32 glyphWidth = UTF8CharLen(mTextView->ByteAt(selStart));

	// the undo count lets us know if the user has done an undo since
	// starting to hit the forward delete key.  Otherwise there
	// is no way to know if the user has hit the forward delete several
	// times and then undo.  If they do this the typing sequence needs
	// to be reset.
	if ((mTypingEnd != selStart) || (mTypingEnd != selEnd) || (mUndoCount > 0))	{
		// Selection has changed. Start a fresh typing sequence
		Reset();

		if (mSelStart == mSelEnd) {
			// Selection is a single insertion point, select next character
			free(mDeletedText);
			mDeletedTextLen = glyphWidth;
			mDeletedText = (char *)malloc(glyphWidth);
			
			for (int32 i = 0; i < glyphWidth; i++)
				mDeletedText[i] = mTextView->ByteAt(selStart++);
		}
	}
	else {
		// Selection hasn't changed, select next character
		int32	newDeletedTextLen = mDeletedTextLen + glyphWidth;
		char	*deletedText = (char *)malloc(newDeletedTextLen);
		memcpy(deletedText, mDeletedText, mDeletedTextLen);

		for (int32 i = mDeletedTextLen; i < newDeletedTextLen; i++)
			deletedText[i] = mTextView->ByteAt(selStart++);

		free(mDeletedText);
		mDeletedText = deletedText;
		mDeletedTextLen = newDeletedTextLen;
	}
}

void
_BTypingUndoBuffer_::Reset()
{
	free(mDeletedText);
	mDeletedText = NULL;

	mTextView->GetSelection(&mSelStart, &mSelEnd);
	mDeletedTextLen = mSelEnd - mSelStart;
	mDeletedText = (char *)malloc(mDeletedTextLen);
	memcpy(mDeletedText, mSelStart + mTextView->Text(), mDeletedTextLen);
	mRedo = false;

	mTypingStart = mTypingEnd = mSelStart;
	
	free(mTypedText);
	mTypedText = NULL;
	mUndoCount = 0;
}

void
_BTypingUndoBuffer_::UndoSelf(
	BClipboard	*)
{
	int32 textLen = mTypingEnd - mTypingStart;

	// Save current typing run
	free(mTypedText);
	mTypedText = NULL;

	mTypedText = (char *)malloc(textLen);
	memcpy(mTypedText, mTypingStart + mTextView->Text(), textLen);
	
	// Delete current typing run
	mTextView->Select(mTypingStart, mTypingStart);
	mTextView->Delete(mTypingStart, mTypingEnd);

	// Restore original text
	mTextView->Insert(mDeletedText, mDeletedTextLen);	

	// Restore original selection
	mTextView->Select(mSelStart, mSelEnd);
	
	mUndoCount++;
}

void
_BTypingUndoBuffer_::RedoSelf(
	BClipboard	*)
{
	// Delete original text
	mTextView->Select(mTypingStart, mTypingStart);
	mTextView->Delete(mTypingStart, mTypingStart + mDeletedTextLen);
	
	// Insert typing run
	mTextView->Insert(mTypedText, mTypingEnd - mTypingStart);	

	mUndoCount--;
}


_BInlineInput_::_BInlineInput_(
	BMessenger	method)
{
	fMethod = method;
	fOffset = 0;
	fLength = 0;
	fSelStart = 0;
	fSelEnd = 0;
	fNumClauses = 0;
	fClauses = NULL;
	fClausesCommited = false;
	fActive = false;
}


_BInlineInput_::~_BInlineInput_()
{
	free(fClauses);
}


void
_BInlineInput_::SetOffset(
	int32	offset)
{
	fOffset = offset;
}


int32
_BInlineInput_::Offset() const
{
	return (fOffset);
}


void
_BInlineInput_::SetLength(
	int32	length)
{
	fLength = length;
}


int32
_BInlineInput_::Length() const
{
	return (fLength);
}


void
_BInlineInput_::ResetClauses()
{
	fSelStart = 0;
	fSelEnd = 0;
	fNumClauses = 0;
	free(fClauses);
	fClauses = NULL;
	fClausesCommited = false;
}


void
_BInlineInput_::AddClause(
	int32	clauseStart,
	int32	clauseEnd)
{
	if (fClausesCommited)
		return;

	fClauses = (int32 *)realloc(fClauses, sizeof(int32) * ((fNumClauses + 1) * 3));	

	int32 index = fNumClauses * 3;
	fClauses[index] = clauseStart;
	fClauses[index + 1] = clauseEnd;
	fClauses[index + 2] = kInlineInClause;
	fNumClauses++;
}


void
_BInlineInput_::CommitClauses(
	int32	selStart,
	int32	selEnd)
{
	if (fClausesCommited)
		return;

	fSelStart = selStart;
	fSelEnd = selEnd;

	for (int32 i = fNumClauses - 1; i >= 0; i--) {
		int32 index = i * 3;
		int32 clauseStart = fClauses[index];
		int32 clauseEnd = fClauses[index + 1];

		if ((fSelStart == clauseStart) && (fSelEnd == clauseEnd)) {
			fClauses[index + 2] |= kInlineSelectedClause | kInlineClauseEnd;
			break;
		}

//		if (fSelStart >= clauseStart) {
//			if (clauseEnd >= fSelStart)
//				
//		}
	}	

	fClausesCommited = true;
}


void
_BInlineInput_::GetClause(
	int32	index,
	int32	*clauseStart,
	int32	*clauseEnd,
	uint32	*flags) const
{
	if ((!fClausesCommited) || (index < 0) || (index >= fNumClauses))
		return;

	index *= 3;
	*clauseStart = fClauses[index];
	*clauseEnd = fClauses[index + 1];
	*flags = fClauses[index + 2];
}


int32
_BInlineInput_::NumClauses() const
{
	return (fNumClauses);
}


void
_BInlineInput_::SetActive(
	bool	active)
{
	fActive = active;

	if (!fActive) {
		fOffset = 0;
		fLength = 0;
		ResetClauses();	
	}
}


bool
_BInlineInput_::IsActive() const
{
	return (fActive);
}


const BMessenger*
_BInlineInput_::Method() const
{
	return (&fMethod);
}

_BTextTrackState_::_BTextTrackState_(BMessenger nonAsyncTarget)
	: fMouseOffset(0), fShiftDown(false),
	  fStart(0), fEnd(0), fAnchor(0), fTextLen(0),
	  fPulseMsg(_PING_),
	  fAsyncPulse(nonAsyncTarget, &fPulseMsg,
				  nonAsyncTarget.IsValid() ? 30000 : 10000000LL)
{
	PRINT(("Starting BTextTrackState: Target IsValid() = %d\n",
			nonAsyncTarget.IsValid()));
}

_BTextTrackState_::~_BTextTrackState_()
{
	PRINT(("Stopping BTextTrackState\n"));
}

void _BTextTrackState_::SimulateMouseEvent(BTextView* in)
{
	BWindow* win = in ? in->Window() : 0;
	BMessage* msg = win ? win->CurrentMessage() : 0;
	if( !msg ) {
		TRESPASS();
		return;
	}
	
	if( (win->Flags()&B_ASYNCHRONOUS_CONTROLS) == 0 ) {
		// No asynchronous controls, so simulate mouse events.
		uint32	buttons = 0;
		BPoint	position;
		in->GetMouse(&position, &buttons);
		if( !buttons || position != fLastPoint ) {
			PRINT(("Ping!\n"));
			// eww, yuck -- add buttons to current message so that it
			// kind-of looks like a mouse event.
			msg->AddInt32("buttons", buttons);
			if( !buttons ) {
				in->PerformMouseUp(position);
			} else {
				in->PerformMouseMoved(position, B_INSIDE_VIEW);
			}
			fLastPoint = position;
		}
	}
}
