/* ++++++++++

   FILE:  TextBuffer.cpp
   REVS:  $Revision: 1.4 $
   NAME:  peter
   DATE:  Fri Aug 18 14:27:00 PDT 1995

   Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _DEBUG_H
#include <Debug.h>
#endif
#ifndef _TEXT_BUFFER_H
#include <TextBuffer.h>
#endif
#ifndef _VIEW_H
#include <View.h>
#endif
#ifndef _STRING
#include <string.h>
#endif
#ifndef _STDLIB
#include <stdlib.h>
#endif

/* ------------------------------------------------------------------- */
/* ------------------------------------------------------------------- */
/* ------------------------------------------------------------------- 


    01234567           abcdefgh
    ------------------------------------
            ^          ^      ^        ^
            |          |      |        |
        fGapStart   fGapEnd   |        |
            | <------> |      |        |
              11 bytes        |        |
                         logical end   |
                                  physical end

fPhysicalSize	: size of entire buffer (8+11+8+9 = 36 in example)
fLogicalSize	: size of real text (8+8 = 16 in example)
fGapStart		: INDEX of first byte that isn't a valid char.
				  (8 in example)
fGapEnd			: INDEX of first byte that contains a valid char.
				  (19 in example)

There's a method CollapseGap which collapse all the valid text to
the beginning of the buffer. After this call there is a GAP at the
end of the buffer. This GAP differs in that fGapEnd does not index
a valid character:

    01234567abcdefgh
    ------------------------------------
                    ^                   ^ 
                    |                   |
                fGapStart            fGapEnd
                    | <---------------> |
                          20 bytes


NOTE: because of the poor way in which BTextView was written the
TextBuffer needs to keep 1 extra byte at the end of the array. BTextView
uses a bad strategy in that if there's n chars of data (so valid
indices into array are 0, 1, 2, ..., n-1) it often uses n as an index.
Well, 'n' is an invalid index!

   ------------------------------------------------------------------- */
/* ------------------------------------------------------------------- */
/* ------------------------------------------------------------------- */
/* ------------------------------------------------------------------- */
#if 0
void dump_buffer(_BTextBuffer_ *buf)
{
	long len = buf->TextLength();
//	PRINT(("buffer:\n"));
	for (long i = 0; i < len; i++) {
//		PRINT(("%c", (*buf)[i]));
	}
//	PRINT(("<>..<>\n"));
}
#endif

/* ------------------------------------------------------------------- */

_BTextBuffer_::_BTextBuffer_(long allocSize, long initGapSize)
{
	fBuffer = NULL;
	fBlockSize = allocSize;
	fInitGapSize = initGapSize;
	if (fBlockSize <= fInitGapSize)
		debugger("allocSize must be greater than initGapSize");
	Reset();
	Set(NULL, 0);
}

/* ------------------------------------------------------------------- */

_BTextBuffer_::~_BTextBuffer_()
{
	Reset();
}

/* ------------------------------------------------------------------- */

void _BTextBuffer_::Reset()
{
	if (fBuffer) {
		free(fBuffer);
		fBuffer = NULL;
	}
	fLogicalSize = 0;
	fPhysicalSize = 0;
	fGapStart = fGapEnd = -1;
}

/* ------------------------------------------------------------------- */

void _BTextBuffer_::Set(const char *string)
{
	long length;
	length = string ? strlen(string) : 0;
	Set(string, length);
}

/* ------------------------------------------------------------------- */

void _BTextBuffer_::Set(const char *data, long length)
{
	Reset();
	fLogicalSize = length;

	// add 1 extra for NULL termination by Text() method
	fPhysicalSize = ((length + 1 + fBlockSize) / fBlockSize) * fBlockSize;
	fBuffer = (char *) malloc(fPhysicalSize);

	// non-overlapping blocks - use memcpy
	memcpy(fBuffer, data, length);

	fGapStart = length;
	fGapEnd = fPhysicalSize;
}

/* ------------------------------------------------------------------- */

void _BTextBuffer_::Insert(long index, const char *string)
{
	Insert(index, string, strlen(string));
}

/* ------------------------------------------------------------------- */

void _BTextBuffer_::Insert(long index, const char *data, long length)
{
	char	*dest;
	char	*src;
	long	count;
	long	length2 = length + 1;
	long	gapSize;
	bool	enoughRoom;

	ASSERT(fBuffer);

//	PRINT(("Insert(index=%d, length=%d)\n", index, length));

	gapSize = fGapEnd - fGapStart;
	enoughRoom = (fLogicalSize + length2 + gapSize) <= fPhysicalSize;

	if ((index == fGapStart) && (length <= gapSize) && enoughRoom) {
		/*
		 Cool. We're inserting text right where the GAP is and there
		 is enough room. This will be fast.
		*/
		memcpy(fBuffer + fGapStart, data, length);
		fGapStart += length;
		ASSERT(fGapStart <= fGapEnd);
		fLogicalSize += length;
	} else {
		/*
		 We're inserting text someplace else (or the GAP is too small)
		 so get rid of current gap, insert the new text and make a new GAP.
		*/
//		PRINT_OBJ((*this));
		CollapseGap();

		/*
		 If there isn't enough room we need to allocate a new buffer.
		*/
		if ((fLogicalSize + length2 + fInitGapSize) > fPhysicalSize) {

			ASSERT(fBlockSize > fInitGapSize);

//			PRINT(("befor: ")); PRINT_OBJ((*this));

			/*
			 The new size needs to at least accomodate the new logical
			 size plus a GAP. Also, add 1 extra for NULL termination by
			 Text() method. Then round this number up to next fBlockSize.
			*/
			long newLogical = fLogicalSize + length;
			fPhysicalSize = ((newLogical + 1 + fInitGapSize + fBlockSize)
				/ fBlockSize) * fBlockSize;

//			PRINT(("newLog=%d, newPhys=%d\n", newLogical, fPhysicalSize));

			char *buf = (char *) malloc(fPhysicalSize);
			char *dest = buf;

			// copy text in 3 parts: (memcpy is OK since blocks don't overlap)

			// part 1: Old text 'before' insertion point
//			PRINT(("cpy 1: dest=%d, src=%d, length=%d\n", dest-buf, 0, index));
			memcpy(dest, fBuffer, index);

			// part 2: Copy new text
			dest += index;
//			PRINT(("cpy 2: dest=%d, src=<new>, length=%d\n", dest-buf, length));
			memcpy(dest, data, length);

			// part 3: Old text 'after' insertion point (leave room for GAP)
			dest += length + fInitGapSize;
//			PRINT(("cpy 3: dest=%d, src=%d, length=%d\n",
//				dest-buf, index, fLogicalSize - index));
			memcpy(dest, fBuffer + index, fLogicalSize - index);
			
			free(fBuffer);
			fBuffer = buf;
			fLogicalSize += length;
			fGapStart = index + length;
			fGapEnd = fGapStart + fInitGapSize;
//			PRINT(("after: ")); PRINT_OBJ((*this));
//			dump_buffer(this);
		} else {
			/*
			 There was enough room so move text after 'index' (making
			 room for the GAP) and then insert the new text. Must use
			 memmove() because the blocks might overlap.
			*/

			dest = fBuffer + index + length + fInitGapSize;
			src = fBuffer + index;
			count = fLogicalSize - index;

			ASSERT((index + length + fInitGapSize) < fPhysicalSize);
			memmove(dest, src, count);		// overlapping blks

			memcpy(fBuffer + index, data, length);	// non-overlapping blks
			fGapStart = index + length;
			fGapEnd = fGapStart + fInitGapSize;
			fLogicalSize += length;
		}
	}
}

/* ------------------------------------------------------------------- */

void _BTextBuffer_::CollapseGap()
{
	if (fGapStart == fGapEnd)
		return;
	
	ASSERT(fGapStart != -1);
	ASSERT(fGapEnd != -1);

	// use memove because blocks might overlap
	memmove(fBuffer + fGapStart, fBuffer + fGapEnd, fPhysicalSize - fGapEnd);
	
	/*
	 Setup the GAP pointers so that there's a GAP at the end of buffer.
	 This GAP is a little different in that there are no valid characters
	 at index of fGapEnd.
	*/
	fGapStart = fLogicalSize;
	fGapEnd = fPhysicalSize;
}

/* ------------------------------------------------------------------- */

const uchar &_BTextBuffer_::operator[](long index)
{
	// BTextView sucks. It tries to index past the end of real data.
	if (index == fLogicalSize)
		index--;

	ASSERT(index < fLogicalSize);

	if (index >= fGapStart)
		index += (fGapEnd - fGapStart);

	return fBuffer[index];
}

/* ------------------------------------------------------------------- */

void _BTextBuffer_::Delete(long index, long length)
{
	char	*src;
	char	*dest;
	long	count;

	ASSERT((index + length) <= fLogicalSize);

	/*
	 Like Insert() we want to make use of the GAP if possible. If we're
	 Deleting char's just before the GAP or just after the GAP we
	 don't have to move anything. Just adjust the size of the GAP.
	*/

	if ((index + length) == fGapStart) {
		/*
		 We're deleting text just before the GAP.
		*/
		fLogicalSize -= length;
		fGapStart -= length;
	} else if (index == fGapStart) {
		/*
		 We're deleting text just after the GAP. This is a little subtle.
		 Look at the diagram at the top of file. If fGapStart is 8. The
		 8th valid character is at index fGapEnd. So if we're deleting
		 from an index that equals fGapStart we're really deleting
		 text just pass the GAP.
		*/
		fLogicalSize -= length;
		fGapEnd += length;
	} else {
		/*
		 The GAP doesn't help. Collapse the GAP and then remove the
		 appropriate text. An optimization here assumes that if you
		 you delete from are particular spot you are likely to 
		 delete/insert from  that same spot. Example, changing
		 insertion point and then back spacng several times. For
		 this reason we'll effectively move the GAP to the deletion
		 point by simply making the GAP be the text being deleted.
		 So we don't have to 'move' any bytes.
		*/
		CollapseGap();

		fLogicalSize -= length;

		// make the GAP be the deleted text!
		fGapStart = index;
		fGapEnd = index + length;
	}
}

/* ------------------------------------------------------------------- */

void _BTextBuffer_::DrawString(long index, long length, BView *view) const
{
	// ??? no sanity checking on the arguments
	
//	PRINT(("DrawString(%d, %d), fLog=%d\n", index, length, fLogicalSize));

	// 3 cases:

	// case 1: string is completely before GAP
	if ((index + length) <= fGapStart)
		view->DrawString(fBuffer + index, length);
	
	// case 2: string is completely after GAP
	else if (index >= fGapStart)
		view->DrawString(fBuffer + index + (fGapEnd - fGapStart), length);
	
	// case 3: string spans the GAP
	else {
		// in this case draw the string in 2 pieces
		view->DrawString(fBuffer + index, fGapStart - index);
		view->DrawString(fBuffer + fGapEnd, length - (fGapStart - index));
	}
}

/* ------------------------------------------------------------------- */

const char *_BTextBuffer_::Text()
{
	ASSERT(fBuffer);
	ASSERT(fLogicalSize < fPhysicalSize);

//	PRINT(("BTextBuffer::Text()\n"));
	CollapseGap();
	fBuffer[fLogicalSize] = '\0';
	return fBuffer;
}

/* ------------------------------------------------------------------- */

char *_BTextBuffer_::GetText(char *copy, long index, long length) const
{
	ASSERT((index + length) <= fLogicalSize);

	// 3 cases:

	// case 1: string is completely before GAP
	if ((index + length) <= fGapStart)
		memcpy(copy, fBuffer + index, length);
	
	// case 2: string is completely after GAP
	else if (index >= fGapStart)
		memcpy(copy, fBuffer + index + (fGapEnd - fGapStart), length);
	
	// case 3: string spans the GAP
	else {
		// in this case draw the string in 2 pieces
		memcpy(copy, fBuffer + index, fGapStart - index);
		memcpy(copy + (fGapStart - index), fBuffer + fGapEnd,
			length - (fGapStart - index));
	}
	
	copy[length] = '\0';		// NULL terminate
	
	return copy;
}

/* ------------------------------------------------------------------- */

void _BTextBuffer_::PrintToStream() const
{
	_BTextBuffer_ *buf = (_BTextBuffer_ *) this;
	printf("log=%d, phys=%d, gapS=%d, gapE=%d\n",
		fLogicalSize, fPhysicalSize, fGapStart, fGapEnd);
}
