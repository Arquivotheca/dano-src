// ============================================================
//  STESupport.cpp	1996 Hiroshi Lockheimer
// ============================================================
// 	STE Version 1.0a5

#include <math.h>
#include <string.h>
// use min/max from IDEConstants.h
// #include <algobase.h> 

#include "STESupport.h"
#include "MLocker.h"
#include "IDEConstants.h"

#include <Locker.h>


BLocker		STEWidthBuffer::fLocker("widthslock");

const rgb_color	steblack = { 0, 0, 0, 255 };

STEStyle&
STEStyle::operator=(const STEStyle &inStyle)
{
	BFont::operator=(inStyle);
	color = inStyle.color;
	extra = inStyle.extra;
	
	return *this;
}

STEStyle&
STEStyle::operator=(const BFont &inFont)
{
	BFont::operator=(inFont);
	color = steblack;
	extra = 0;
	
	return *this;
}

STEStyle::STEStyle()
{
	color = steblack;
	extra = 0;
}

bool
STEStyle::operator==(const STEStyle &inStyle) const
{
	return extra == inStyle.extra && color == inStyle.color && BFont::operator==(inStyle);
}

bool
STEStyle::operator!=(const STEStyle &inStyle) const
{
	return extra != inStyle.extra || color != inStyle.color || BFont::operator!=(inStyle);
}

void
STEStyle::PrintToStream() const
{
	printf("style: ");
	BFont::PrintToStream();
	int		red = color.red;
	int		green = color.green;
	int		blue = color.blue;
	int		alpha = color.alpha;

	printf("color: %ld, %ld, %ld, %ld; extra: %ld\n", red, green, blue, alpha, extra);

}

STEStyle::STEStyle(const BFont &inFont, rgb_color inColor, int32 inExtra)
: BFont(inFont)
{
	color = inColor;
	extra = inExtra;
}

STEStyle::STEStyle(const STEStyle &inStyle)
: BFont(inStyle)
{
	color = inStyle.color;
	extra = inStyle.extra;
}

STEStyleRun::STEStyleRun(const STEStyle &inStyle, int32 inOffset)
{
	style = inStyle;
	offset = inOffset;
}

STEStyleRun::STEStyleRun()
{
	offset = 0;
}


STEStyleRun::STEStyleRun(const STEStyleRun &inStyleRun)
{
	style = inStyleRun.style;
	offset = inStyleRun.offset;
}

STEStyleRun&
STEStyleRun::operator=(const STEStyleRun &inStyleRun)
{
	style = inStyleRun.style;
	offset = inStyleRun.offset;
	
	return *this;
}

STEStyleRecord::STEStyleRecord()
{
	refs = 0;
	ascent = descent = 0.0;
}

STEStyleRecord&
STEStyleRecord::operator=(const STEStyleRecord &inStyleRecord)
{
	style = inStyleRecord.style;
	refs = inStyleRecord.refs;
	ascent = inStyleRecord.ascent;
	descent = inStyleRecord.descent;
	
	return *this;
}

STETextBuffer::STETextBuffer()
{
	mExtraCount = 2048;
	mItemCount = 0;
	mBuffer = (char *)malloc(mExtraCount + mItemCount);
	mBufferCount = mExtraCount + mItemCount;
	mGapIndex = mItemCount;
	mGapCount = mBufferCount - mGapIndex;
	mScratchBuffer = (char *)malloc(0);
	mScratchSize = 0;
}

STETextBuffer::~STETextBuffer()
{
	free(mBuffer);
	free(mScratchBuffer);
}

void
STETextBuffer::InsertText(
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
STETextBuffer::RemoveRange(
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
STETextBuffer::MoveGapTo(
	int32	toIndex)
{	
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

	mGapIndex = toIndex;
}


void
STETextBuffer::SizeGapTo(
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
STETextBuffer::GetString(
	int32	fromOffset,
	int32 	numChars)
{
	char *result = "";
	
	if (numChars < 1)
		return (result);
	
	bool isStartBeforeGap = (fromOffset < mGapIndex);
	bool isEndBeforeGap = ((fromOffset + numChars - 1) < mGapIndex);

	if (isStartBeforeGap == isEndBeforeGap) {
		result = mBuffer + fromOffset;
		if (!isStartBeforeGap)
			result += mGapCount;
	}
	else {
		if (mScratchSize < numChars) {
			mScratchBuffer = (char *)realloc(mScratchBuffer, numChars);
			mScratchSize = numChars;
		}
		
		for (int32 i = 0; i < numChars; i++)
			mScratchBuffer[i] = (*this)[fromOffset + i];

		result = mScratchBuffer;
	}
	
	return (result);
}

bool
STETextBuffer::FindChar(
	char	inChar,
	int32	fromIndex,
	int32	*ioDelta)
{
	int32 numChars = *ioDelta;
	for (int32 i = 0; i < numChars; i++) {
		if ((*this)[fromIndex + i] == inChar) {
			*ioDelta = i;
			return (true);
		}
	}
	
	return (false);
}

const char*
STETextBuffer::Text()
{
	if (mGapIndex != mItemCount)
		MoveGapTo(mItemCount);
	
	mBuffer[mItemCount] = '\0';
	
	return (mBuffer);
}


template<class T>
STEBuffer<T>::STEBuffer(
	int32	inExtraCount,
	int32	inCount)
{
	mExtraCount = inExtraCount;
	mItemCount = inCount;
	mBufferCount = inCount + inExtraCount;
	mBuffer = new T[mBufferCount];
}

template<class T>
STEBuffer<T>::~STEBuffer()
{
	delete [] mBuffer;
}

template<class T>
void
STEBuffer<T>::ReallocBuffer(
	int32	inNewSize)
{
	T*	temp = new T[inNewSize];
	int	numToCopy = min(inNewSize, mItemCount);

	const T* 	from = mBuffer;
	T* 			to = temp;
	for (int i = 0; i < numToCopy; ++i)
	{
		*to++ = *from++;
	}

	delete [] mBuffer;
	mBuffer = temp;
	mBufferCount = inNewSize;

	return;
}


template<class T>
void
STEBuffer<T>::InsertItemsAt(
	int32	inNumItems,
	int32 	inAtIndex,
	const T	*inItem)
{
	if (inNumItems < 1)
		return;
	
	inAtIndex = (inAtIndex > mItemCount) ? mItemCount : inAtIndex;
	inAtIndex = (inAtIndex < 0) ? 0 : inAtIndex;

	if ((inNumItems + mItemCount) >= mBufferCount) {
		ReallocBuffer(mBufferCount * 2);
	}
	
	T* 			to = mBuffer + mItemCount;
	const T* 	from = to - 1;
	for (int i = mItemCount - inAtIndex; i > 0 ; --i)
	{
		*to-- = *from--;
	}
	from = inItem;
	to = mBuffer + inAtIndex;
	for (int i = 0; i < inNumItems; ++i)
	{
		*to++ = *from++;
	}

	mItemCount += inNumItems;
}

template<class T>
void
STEBuffer<T>::RemoveItemsAt(
	int32	inNumItems,
	int32 	inAtIndex)
{
	if (inNumItems < 1)
		return;
	
	inAtIndex = (inAtIndex >= mItemCount) ? (mItemCount - 1) : inAtIndex;
	inAtIndex = (inAtIndex < 0) ? 0 : inAtIndex;
	
	T* 			to = mBuffer + inAtIndex;
	const T* 	from = to + inNumItems;
	const int32	term = mItemCount - inNumItems;
	for (int32 i = 0; i < term; ++i)
	{
		*to++ = *from++;
	}

	mItemCount -= inNumItems;
	
	if ((mBufferCount / 2) > mItemCount) {
		ReallocBuffer(mBufferCount / 2);
	}
}

template<class T>
STEPODBuffer<T>::STEPODBuffer(
	int32	inExtraCount,
	int32	inCount)
{
	mExtraCount = inExtraCount;
	mItemCount = inCount;
	mBufferCount = inCount + inExtraCount;
	mBuffer = (T *)calloc(mBufferCount, sizeof(T));
}

template<class T>
STEPODBuffer<T>::~STEPODBuffer()
{
	free(mBuffer);
}


template<class T>
void
STEPODBuffer<T>::InsertItemsAt(
	int32	inNumItems,
	int32 	inAtIndex,
	const T	*inItem)
{
	if (inNumItems < 1)
		return;
	
	inAtIndex = (inAtIndex > mItemCount) ? mItemCount : inAtIndex;
	inAtIndex = (inAtIndex < 0) ? 0 : inAtIndex;

	if ((inNumItems + mItemCount) >= mBufferCount) {
		mBufferCount *= 2;
		mBuffer = (T *)realloc(mBuffer, mBufferCount * sizeof(T));
	}
	
	T *loc = mBuffer + inAtIndex;
	memmove(loc + inNumItems, loc, (mItemCount - inAtIndex) * sizeof(T));
	memcpy(loc, inItem, inNumItems * sizeof(T));

	mItemCount += inNumItems;
}

template<class T>
void
STEPODBuffer<T>::RemoveItemsAt(
	int32	inNumItems,
	int32 	inAtIndex)
{
	if (inNumItems < 1)
		return;
	
	inAtIndex = (inAtIndex >= mItemCount) ? (mItemCount - 1) : inAtIndex;
	inAtIndex = (inAtIndex < 0) ? 0 : inAtIndex;
	
	T *loc = mBuffer + inAtIndex;
	memmove(loc, loc + inNumItems, 
			(mItemCount - (inNumItems + inAtIndex)) * sizeof(T));

	mItemCount -= inNumItems;

	if ((mBufferCount / 2) > mItemCount) {
		mBufferCount /= 2;
		mBuffer = (T *)realloc(mBuffer, mBufferCount * sizeof(T));
	}
}


STEWidthBuffer::STEWidthBuffer()
	: STEPODBuffer<STEWidthTable>()
{
}

STEWidthBuffer::~STEWidthBuffer()
{
	int32		term = mItemCount;
	for (int32 i = 0; i < term; i++)
		free(mBuffer[i].widths);
}

inline uint32
STEWidthBuffer::Hash(
	UTF8_char	inChar)
{
	uint32 code = inChar.code;
	return ( ((3 * (code >> 24)) + (code >> 15)) ^ 
			 ((code >> 6) - ((code >> 24) * 22)) ^ 
			 (code << 3) );
}

void
STEWidthBuffer::CCheck()
{
	if (mItemCount > 0) {
		int32 cCount = 0;
		for (int32 i = 0; i < mBuffer[0].tableCount; i++) {
			if (mBuffer[0].widths[i].character.code != 0xFFFFFFFF) {
				if (i != (Hash(mBuffer[0].widths[i].character) & 
						  (mBuffer[0].tableCount - 1))) {
					printf("collision %c\n", 
						   mBuffer[0].widths[i].character.bytes[0]);
					cCount++;
				}
			}
		}
		printf("hashCount = %ld, tableCount = %ld, %ld collisions, %f%\n", 
			   mBuffer[0].hashCount, mBuffer[0].tableCount, cCount, 
			   ((float)cCount) / ((float)mBuffer[0].hashCount) * 100.0);
	}
}

float
STEWidthBuffer::StringWidth(
	STETextBuffer		&inText, 
	int32				fromOffset, 
	int32				length,
	const BFont			*inStyle)
{
	if (length < 1)
		return (0.0);
	
	MLocker<BLocker>	lock(fLocker);
	int32 				index = 0;

	if (!FindTable(inStyle, &index))
		index = InsertTable(inStyle);
	
	float 		result = 0.0;
	const float conversion = inStyle->Size();

	int32 curroffset = fromOffset;
	int32 lastoffset = fromOffset + length;
	while (curroffset < lastoffset) {
		int32		charLen = UTF8CharLen(inText[curroffset]);
		UTF8_char	theChar;
		theChar.code = 0;

		for (int32 j = 0; j < charLen; j++, curroffset++)
			theChar.bytes[j] = inText[curroffset];

		// tab widths are defined by the user, not the font
		if (theChar.bytes[0] != '\t')
			result += GetEscapement(theChar, charLen, index, inStyle) * conversion;
	}

	return (result);
}

float
STEWidthBuffer::StringWidth(
	const char			*inText, 
	int32				fromOffset, 
	int32				length,
	const BFont			*inStyle)
{
	if (length < 1)
		return (0.0);
			
	MLocker<BLocker>	lock(fLocker);
	int32 				index = 0;
	if (!FindTable(inStyle, &index))
		index = InsertTable(inStyle);
	
	float result = 0.0;
	float conversion = inStyle->Size();

	int32 i = 0;
	while (i < length) {
		int32		charLen = UTF8CharLen(inText[fromOffset + i]);
		UTF8_char	theChar;
		theChar.code = 0;

		for (int32 j = 0, offset = fromOffset + i; j < charLen; j++, offset++)
			theChar.bytes[j] = inText[offset];

		result += GetEscapement(theChar, charLen, index, inStyle) * conversion;
		i += charLen;
	}

	return (result);
}

//	If made public then needs to have locking calls added.

float
STEWidthBuffer::GetEscapement(
	UTF8_char	inChar,
	int32		inLength,
	int32		index,
	const BFont	*inStyle)
{
	STEWidthTable	*table = &mBuffer[index];
	STEWidthEntry	*widths = table->widths;	
	uint32			key = Hash(inChar) & (table->tableCount - 1);

	if (widths[key].character.code == inChar.code) 
		// we hashed to the correct position
		return (widths[key].escapement);

	// did we collide with another character?
	if (widths[key].character.code != 0xFFFFFFFF) {
		do {
			key++;
			if (key >= table->tableCount)
				key = 0;
		} while ( (widths[key].character.code != inChar.code) &&
				  (widths[key].character.code != 0xFFFFFFFF) );

		if (widths[key].character.code == inChar.code) 
			// found it, we're done
			return (widths[key].escapement);
	}

	// the escapement of inChar was no where to be found
	// note: we only use result[0] even in cases of multi-byte characters
	// waiting for Hiroshi to make final change [John Dance]
	float result[4] = { 0, 0, 0, 0};
	inStyle->GetEscapements(inChar.bytes, inLength, result);
	widths[key].character = inChar;
	widths[key].escapement = result[0];
	table->hashCount++;

	if ((((float)table->hashCount) / ((float)table->tableCount)) >= 0.70) {
		// we're >= 70% full, time to resize
		int32 saveTableCount = table->tableCount;
		table->hashCount = 0;
		table->tableCount = saveTableCount * 2;
		table->widths = (STEWidthEntry *)malloc(sizeof(STEWidthEntry) *
												table->tableCount);

		// initialize the new table
		const int32 		term = table->tableCount;
		for (int32 i = 0; i < term; i++) {
			table->widths[i].character.code = 0xFFFFFFFF;
			table->widths[i].escapement = 0.0;
		}

		// rehash the data in the old table
		uint32 hashMask = table->tableCount - 1;
		for (int i = 0; i < saveTableCount; i++) {
			if (widths[i].character.code != 0xFFFFFFFF) {
				uint32 key = Hash(widths[i].character) & hashMask;

				while (table->widths[key].character.code != 0xFFFFFFFF) {
					key++;
					if (key >= table->tableCount)
						key = 0;
				}

				table->widths[key].character = widths[i].character;
				table->widths[key].escapement = widths[i].escapement;
				table->hashCount++;
			}
		}

		free(widths);
		widths = table->widths;
	} 

	return (result[0]);
}

int32
STEWidthBuffer::InsertTable(
	const BFont	*inStyle)
{
	int32 index = mItemCount;

	STEWidthTable table;
	table.font = *inStyle;
	table.hashCount = 0;
	table.tableCount = 128;		// must be power of two			
	table.widths = (STEWidthEntry *)malloc(sizeof(STEWidthEntry) * 
										   table.tableCount);
	const int32			term = table.tableCount;

	for (int i = 0; i < term; i++) {
		table.widths[i].character.code = 0xFFFFFFFF;
		table.widths[i].escapement = 0.0;
	}

	InsertItemsAt(1, index, &table);

	return (index);
}

bool
STEWidthBuffer::FindTable(
	const BFont	*inStyle, 
	int32		*outIndex)
{
	const int32			term = mItemCount;


	for (int32 i = 0; i < term; i++) {
		if (mBuffer[i].font == *inStyle) {
			*outIndex = i;
			return (true);
		}
	}

	return (false);
}

STELineBuffer::STELineBuffer()
	: STEPODBuffer<STELine>(20, 2)
{
}

STELineBuffer::~STELineBuffer()
{
	// gcc has problems generating the base class destructor
	// (STEPODBuffer<STELine>::~STEPODBuffer<STELine>) if it
	// doesn't see a direct usage.  So we create a dummy destructor.
}

void
STELineBuffer::InsertLine(
	STELinePtr	inLine,
	int32		index)
{
	InsertItemsAt(1, index, inLine);
}

void
STELineBuffer::RemoveLines(
	int32	index,
	int32	count)
{
	RemoveItemsAt(count, index);
}

void
STELineBuffer::RemoveLineRange(
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
STELineBuffer::OffsetToLine(
	int32	offset) const
{
	int32 minIndex = 0;
	int32 maxIndex = mItemCount - 1;
	int32 index = 0;
	STELine const *	buffer = mBuffer;

	while (minIndex < maxIndex) {
		index = (minIndex + maxIndex) >> 1;
		if (offset >= buffer[index].offset) {
			if (offset < buffer[index + 1].offset)
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
STELineBuffer::PixelToLine(
	float	pixel) const
{
	int32 minIndex = 0;
	int32 maxIndex = mItemCount - 1;
	int32 index = 0;
	STELine const *	buffer = mBuffer;
	
	while (minIndex < maxIndex) {
		index = (minIndex + maxIndex) >> 1;
		if (pixel >= buffer[index].origin) {
			if (pixel < buffer[index + 1].origin)
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
STELineBuffer::BumpOrigin(
	float	delta,
	int32	index)
{
	const int32		term = mItemCount;
	for (int32 i = index; i < term; i++)
		mBuffer[i].origin += delta;
}

void
STELineBuffer::BumpOffset(
	int32	delta,
	int32 	index)
{
	const int32		term = mItemCount;
	for (int32 i = index; i < term; i++)
		mBuffer[i].offset += delta;
}

STEStyleRunDescBuffer::STEStyleRunDescBuffer()
	: STEPODBuffer<STEStyleRunDesc>(20)
{
}

void
STEStyleRunDescBuffer::InsertDesc(
	STEStyleRunDescPtr	inDesc,
	int32				index)
{
	InsertItemsAt(1, index, inDesc);
}

void
STEStyleRunDescBuffer::RemoveDescs(
	int32	index,
	int32 	count)
{
	RemoveItemsAt(count, index);
}

int32
STEStyleRunDescBuffer::OffsetToRun(
	int32	offset) const
{
	if (mItemCount <= 1)
		return (0);
		
	int32 minIndex = 0;
	int32 maxIndex = mItemCount;
	int32 index = 0;
	int32 lastIndex = mItemCount - 1;
	STEStyleRunDesc const *		buffer = mBuffer;

	while (minIndex < maxIndex) {
		index = (minIndex + maxIndex) >> 1;
		if (offset >= buffer[index].offset) {
			if (index >= lastIndex) {
				break;
			}
			else {
				if (offset < buffer[index + 1].offset)
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
STEStyleRunDescBuffer::BumpOffset(
	int32	delta,
	int32 	index)
{
	int32		term = mItemCount;
	for (int32 i = index; i < term; i++)
		mBuffer[i].offset += delta;
}


STEStyleRecordBuffer::STEStyleRecordBuffer()
	: STEBuffer<STEStyleRecord>()
{
}

int32
STEStyleRecordBuffer::InsertRecord(
	ConstSTEStylePtr	inStyle)
{
	int32 index = 0;

	// look for style in buffer
	if (MatchRecord(inStyle, &index))
		return (index);

	// style not found, add it
	font_height info;
	inStyle->GetHeight(&info);
	
	info.ascent = ceil(info.ascent);
	info.descent = ceil(info.descent);
	info.leading = ceil(info.leading);

	// check if there's any unused space
	int32				term = mItemCount;
	STEStyleRecord*		buffer = mBuffer;
	for (index = 0; index < term; index++) {
		STEStyleRecord*		rec = &buffer[index];
		if (rec->refs < 1) {
			rec->refs = 0;
			rec->ascent = info.ascent;
			rec->descent = info.descent + info.leading;
			rec->style = *inStyle;
			return (index);
		}	
	}

	// no unused space, expand the buffer
	index = mItemCount;
	STEStyleRecord 	newRecord;
	newRecord.refs = 0;
	newRecord.ascent = info.ascent;
	newRecord.descent = info.descent + info.leading;
	newRecord.style = *inStyle;
	InsertItemsAt(1, index, &newRecord);

	return (index);
}

void
STEStyleRecordBuffer::CommitRecord(
	int32	index)
{
	mBuffer[index].refs++;
}

void
STEStyleRecordBuffer::RemoveRecord(
	int32	index)
{
	mBuffer[index].refs--;
}

bool
STEStyleRecordBuffer::MatchRecord(
	ConstSTEStylePtr	inRecord,
	int32				*outIndex)
{
	const int32				term = mItemCount;
	ConstSTEStyleRecordPtr	buffer = mBuffer;
	for (int32 i = 0; i < term; i++) 
	{
		if (*inRecord == buffer[i].style)
		{
			*outIndex = i;
			return (true);		
		}
	}

	return (false);
}

STEStyleBuffer::STEStyleBuffer(
	ConstSTEStylePtr	inStyle)
{
	mValidNullStyle = true;
	mNullStyle = *inStyle;
	InvalidateCache();
}

void
STEStyleBuffer::InvalidateCache()
{
	mFromOffset = mNextOffset = -256;
}

void
STEStyleBuffer::InvalidateNullStyle()
{
	if (mStyleRunDesc.ItemCount() < 1)
		return;
		
	mValidNullStyle = false;
}

bool
STEStyleBuffer::IsValidNullStyle() const
{
	return (mValidNullStyle);
}

void
STEStyleBuffer::SyncNullStyle(
	int32	offset)
{
	if ((mValidNullStyle) || (mStyleRunDesc.ItemCount() < 1))
		return;
	
	int32 index = OffsetToRun(offset);
	mNullStyle = mStyleRecord[mStyleRunDesc[index]->index]->style;

	mValidNullStyle = true;
}	

void
STEStyleBuffer::SetNullStyle(
	uint32				inMode,
	ConstSTEStylePtr	inStyle,
	int32				offset)
{
	if ((mValidNullStyle) || (mStyleRunDesc.ItemCount() < 1))
		SetStyle(inMode, inStyle, &mNullStyle);
	else {
		int32 index = OffsetToRun(offset - 1);
		mNullStyle = mStyleRecord[mStyleRunDesc[index]->index]->style;
		SetStyle(inMode, inStyle, &mNullStyle);	
	}

	mValidNullStyle = true;
}

STEStyle
STEStyleBuffer::GetNullStyle() const
{
	return (mNullStyle);
}	

bool
STEStyleBuffer::IsContinuousStyle(
	uint32		*ioMode,
	STEStylePtr	outStyle,
	int32		fromOffset,
	int32 		toOffset) const
{	
	if (mStyleRunDesc.ItemCount() < 1) {
		SetStyle(*ioMode, &mNullStyle, outStyle);
		return (true);
	}
		
	bool result = true;
	int32 fromIndex = OffsetToRun(fromOffset);
	int32 toIndex = OffsetToRun(toOffset - 1);
	
	if (fromIndex == toIndex) {		
		int32				styleIndex = mStyleRunDesc[fromIndex]->index;
		ConstSTEStylePtr	style = &mStyleRecord[styleIndex]->style;
		
		SetStyle(*ioMode, style, outStyle);
		result = true;
	}
	else {
		int32				styleIndex = mStyleRunDesc[toIndex]->index;
		STEStyle			theStyle = mStyleRecord[styleIndex]->style;
		ConstSTEStylePtr	style = NULL;
		int32				mode = *ioMode;
		
		for (int32 i = fromIndex; i < toIndex; i++) {
			styleIndex = mStyleRunDesc[i]->index;
			style = &mStyleRecord[styleIndex]->style;

			if (mode & doFont) {
				if (theStyle.Compare(*style, B_FONT_FAMILY_AND_STYLE) == 0) {
					mode &= ~doFont;
					result = false;
				}	
			}

			if (mode & doSize) {
				if (theStyle.Size() != style->Size()) {
					mode &= ~doSize;
					result = false;
				}
			}
				
			if (mode & doShear) {
				if (theStyle.Shear() != style->Shear()) {
					mode &= ~doShear;
					result = false;
				}
			}
#if 0
			if (mode & doUnderline) {
				if (theStyle.underline != style->underline) {
					mode &= ~doUnderline;
					result = false;
				}
			}
#endif			
			if (mode & doColor) {
				if ( (theStyle.color.red != style->color.red) ||
					 (theStyle.color.green != style->color.green) ||
					 (theStyle.color.blue != style->color.blue) ||
					 (theStyle.color.alpha != style->color.alpha) ) {
					mode &= ~doColor;
					result = false;
				}
			}
			
			if (mode & doExtra) {
				if (theStyle.extra != style->extra) {
					mode &= ~doExtra;
					result = false;
				}
			}
		}
		
		SetStyle(mode, &theStyle, outStyle);
		*ioMode = mode;
	}
	
	return (result);
}

void
STEStyleBuffer::SetStyleRange(
	int32				fromOffset,
	int32				toOffset,
	int32				textLen,
	uint32				inMode,
	ConstSTEStylePtr	inStyle)
{
	if (inStyle == NULL)
		inStyle = &mNullStyle;

	if (fromOffset == toOffset) {
		SetNullStyle(inMode, inStyle, fromOffset);
		return;
	}

	InvalidateCache();
	
	if (mStyleRunDesc.ItemCount() < 1) {
		STEStyleRunDesc newDesc;
		newDesc.offset = fromOffset;
		newDesc.index = mStyleRecord.InsertRecord(inStyle);
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
		
		STEStyle style = mStyleRecord[runDesc.index]->style;
		SetStyle(inMode, inStyle, &style);

		styleIndex = mStyleRecord.InsertRecord(&style);

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
				((STEStyleRunDescPtr)mStyleRunDesc[runIndex])->index = styleIndex;
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
STEStyleBuffer::GetStyle(
	int32		inOffset,
	STEStylePtr	outStyle) const
{
	if (mStyleRunDesc.ItemCount() < 1) {
		*outStyle = mNullStyle;
		return;
	}
	
	int32 runIndex = OffsetToRun(inOffset);
	int32 styleIndex = mStyleRunDesc[runIndex]->index;
	*outStyle = mStyleRecord[styleIndex]->style;
}

STEStyleRangePtr
STEStyleBuffer::GetStyleRange(
	int32	startOffset,
	int32	endOffset) const
{
	STEStyleRangePtr result = NULL;
	
	int32 startIndex = OffsetToRun(startOffset);
	int32 endIndex = OffsetToRun(endOffset);
	
	int32 numStyles = endIndex - startIndex + 1;
	numStyles = (numStyles < 1) ? 1 : numStyles;
	result = (STEStyleRangePtr)malloc(sizeof(int32) +
									  (sizeof(STEStyleRun) * numStyles));

	result->count = numStyles;
	STEStyleRunPtr run = &result->runs[0];
	for (int32 index = 0; index < numStyles; index++) {
		*run = (*this)[startIndex + index];
		run->offset -= startOffset;
		run->offset = (run->offset < 0) ? 0 : run->offset;
		run++;
	}
	
	return (result);
}

void
STEStyleBuffer::RemoveStyleRange(
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
		((STEStyleRunDescPtr)mStyleRunDesc[toIndex + 1])->offset = fromOffset;
	
	if (fromIndex < (mStyleRunDesc.ItemCount() - 1)) {
		ConstSTEStyleRunDescPtr runDesc = mStyleRunDesc[fromIndex];
		if (runDesc->offset == (runDesc + 1)->offset) {
			RemoveStyles(fromIndex);
			fromIndex--;
		}
	}
	
	if ((fromIndex >= 0) && (fromIndex < (mStyleRunDesc.ItemCount() - 1))) {
		ConstSTEStyleRunDescPtr runDesc = mStyleRunDesc[fromIndex];
		if (runDesc->index == (runDesc + 1)->index)
			RemoveStyles(fromIndex + 1);
	}

	InvalidateCache();
}

void
STEStyleBuffer::RemoveStyles(
	int32	index,
	int32 	count)
{
	for (int32 i = index; i < (index + count); i++)
		mStyleRecord.RemoveRecord(mStyleRunDesc[i]->index);
		
	mStyleRunDesc.RemoveDescs(index, count);
	InvalidateCache();
}

int32
STEStyleBuffer::Iterate(
	int32				fromOffset,
	int32				length,
	ConstSTEStylePtr	*outStyle,
	float				*outAscent,
	float				*outDescent) const
{
	int32 numRuns = mStyleRunDesc.ItemCount();
	if ((length < 1) || (numRuns < 1))
		return (0);

	int32 						result = length;
	int32 						runIndex;

	if (fromOffset < mNextOffset && fromOffset >= mFromOffset)
		runIndex = mRunIndex;
	else
		runIndex = mStyleRunDesc.OffsetToRun(fromOffset);

	ConstSTEStyleRunDescPtr		run = mStyleRunDesc[runIndex];

	ConstSTEStyleRecordPtr		stylerec = mStyleRecord[run->index];

	if (outStyle != NULL)
		*outStyle = &stylerec->style;
	if (outAscent != NULL)
		*outAscent = stylerec->ascent;
	if (outDescent != NULL)
		*outDescent = stylerec->descent;

	if (runIndex < (numRuns - 1)) {
		int32 nextOffset = (run + 1)->offset;
		int32 len = nextOffset - fromOffset;
		result = (result > len) ? len : result;

		// iterating by bytes
		mFromOffset = run->offset;
		mNextOffset = nextOffset;
		mRunIndex = runIndex;
	}
	else
	{
		mFromOffset = mNextOffset = -256;
	}

	return (result);
}

int32
STEStyleBuffer::OffsetToRun(
	int32	offset) const
{
	return (mStyleRunDesc.OffsetToRun(offset));
}

void
STEStyleBuffer::BumpOffset(
	int32	delta,
	int32	index)
{
	mStyleRunDesc.BumpOffset(delta, index);
}

void
STEStyleBuffer::SetStyle(
	uint32				mode,
	ConstSTEStylePtr	fromStyle,
	STEStylePtr			toStyle) const
{
	if ((mode & (doSize|addSize)) == (doSize|addSize)) {
		toStyle->SetSize(toStyle->Size() + fromStyle->Size());
		mode &= ~doSize;
	}
	
	if (mode & B_FONT_ALL)
		toStyle->SetTo(*fromStyle, mode&B_FONT_ALL);
	
#if 0
	if (mode & doUnderline)
		toStyle->underline = fromStyle->underline;
#endif	
	if (mode & doColor)
		toStyle->color = fromStyle->color;
		
	if (mode & doExtra)
		toStyle->extra = fromStyle->extra;
}

STEStyleRun
STEStyleBuffer::operator[](
	int32	index) const
{
	int32				offset;
	ConstSTEStylePtr	style;

	if (mStyleRunDesc.ItemCount() < 1) {
		offset = 0;
		style = &mNullStyle;
	} else {
		ConstSTEStyleRunDescPtr	runDesc = mStyleRunDesc[index];
		ConstSTEStyleRecordPtr	record = mStyleRecord[runDesc->index];
		offset = runDesc->offset;
		style = &record->style;
	}
	
	return STEStyleRun(*style, offset);
}

