
#include <textencoding/BTextEncoding.h>
#include <add-ons/textencoding/BTextCodec.h>
#include "BTextEncodingAddOn.h"

#include <stdlib.h>
#include <stdio.h>

using namespace B::TextEncoding;

BTextCodec::BTextCodec(	tcodec_array replacements, const uint16 &replaceCount,
						tcodec_array runs, const uint16 &runCount,
						tcodec_array blocks, tcodec_array blockIndex, const uint16 &blockCount,
						BTextEncodingAddOn *addOn, uint32 flags, int32 adjustValue) :
	fReplaces(replacements),
	fReplaceCount(replaceCount),
	fRuns(runs),
	fRunCount(runCount),
	fBlocks(blocks),
	fBlockIndex(blockIndex),
	fBlockCount(blockCount),
	fAddOn(addOn),
	fFlags(flags),
	fAdjustValue(adjustValue)
	
{
}


BTextCodec::~BTextCodec()
{
	if (fAddOn)
		fAddOn->Close();
}

status_t 
BTextCodec::DecodeString(const char *src, int32 *srcLen, char *dst, int32 *dstLen, conversion_info &info) const
{
	int32 srcEnd = *srcLen;
	int32 dstEnd = *dstLen;
	int32 srcPos = 0;
	int32 dstPos = 0;
	while ((srcPos < srcEnd) && (dstPos < dstEnd)) {
		bool multibyte = false;
		uchar c = (uchar)src[srcPos];
		uint16 charcode = c;
		if (fFlags & HAS_MULTIBYTE && c > 0x80) {
			if (src[srcPos + 1] >= srcEnd)
				break;

			charcode = (((uchar)src[srcPos]) << 8) | ((uchar)src[srcPos + 1]);
			multibyte = true;
		}

		uint16 unicode = 0xffff;
		unicode = Decode(charcode);

		if (unicode == 0xffff)
			unicode = info.substitute;
		
//		printf("0x%04x->0x%04x\n", charcode, unicode);		

		// get the unicode_to_utf8
		ssize_t add = unicode_to_utf8(unicode, &dst[dstPos], dstEnd - dstPos);
		if (add < 0)
			break;
		dstPos += add;
		// increment the src pointer
		srcPos += (multibyte) ? 2 : 1;
	}
	*srcLen = srcPos;
	*dstLen = dstPos;
	return (dstPos > 0) ? B_OK : B_ERROR;
}

status_t 
BTextCodec::EncodeString(const char *src, int32 *srcLen, char *dst, int32 *dstLen, conversion_info &info) const
{
	int32 srcEnd = *srcLen;
	int32 dstEnd = *dstLen;
	int32 srcPos = 0;
	int32 dstPos = 0;
	
	while ((srcPos < srcEnd) && (dstPos < dstEnd)) {
		// determine the unicode character to try and convert
		if (srcPos + utf8_char_len(src[srcPos]) > srcEnd)
			break;
			
		uint16 unicode = 0xffff;
		ssize_t addToSrc = 0;
		bool multibyte = false;
		// convert utf8 to unicode
		addToSrc = utf8_to_unicode(src + srcPos, srcEnd - srcPos, &unicode);
		if (addToSrc < 0)
			break;
		
		// get the charcode value for this unicode
		uint16 charcode = 0xffff;
		charcode = EncodeFor(unicode);
		
		if (charcode == 0xffff)
			charcode = info.substitute;
		
		if (charcode > 0xff) {
				dst[dstPos] = (charcode >> 8) & 0xff;
				dst[dstPos + 1] = charcode & 0xff;
				multibyte = true;
		}
		else
			dst[dstPos] = (uchar)charcode;

		srcPos += addToSrc;
		dstPos += (multibyte) ? 2 : 1;
	}
	*srcLen = srcPos;
	*dstLen = dstPos;
	return (srcPos > 0) ? B_OK : B_ERROR;
}

uint16 
BTextCodec::Decode(uint16 charcode) const
{
	uint16 unicode = 0xffff;

	// if it uses ascii and is an ascii char - return it
	if (fFlags & USES_ASCII && charcode < 0x80) {
		return charcode;
	}

	if (fFlags & ADJUST_VALUE)
		charcode += fAdjustValue;
	else if (fFlags & ADJUST_FUNCTION)
		charcode = AdjustCharcode(charcode, true);

	// check replacements
	if (fReplaces) {
		if (fReplaceCount > 5)
			unicode = BinarySearchFor(charcode, REPLACE);
		else
			unicode = LinearSearchFor(charcode, REPLACE, true);
	}	

	// if not there check the runs
	if (unicode == 0xffff && fRuns) {
		if (fRunCount > 5)
			unicode = BinarySearchFor(charcode, RUN);
		else
			unicode = LinearSearchFor(charcode, RUN, true);
	}
	
	// finally check the blocks
	if (unicode == 0xffff && fBlocks) {
		if (fBlockCount > 5)
			unicode = BinarySearchFor(charcode, BLOCK);
		else
			unicode = LinearSearchFor(charcode, BLOCK, true);
	}
	
	return unicode;
}

uint16 
BTextCodec::Encode(uint16 unicode) const
{
	uint16 charcode = 0xffff;
	if (fFlags & USES_ASCII && unicode < 0x80) {
		return unicode;
	}
	
	if (charcode == 0xffff && fReplaces) {
		charcode = LinearSearchFor(unicode, REPLACE, faslse);
	}
	if (charcode == 0xffff && fRuns) {
		charcode = LinearSearchFor(unicode, RUN, false);
	}
	if (charcode == 0xffff && fBlocks) {
		charcode = LinearSearchFor(unicode, BLOCK, false);
	}
	
	if (charcode != 0xffff) {
		if (fFlags & ADJUST_VALUE)
			charcode -= fAdjustValue;
		else if (fFlags & ADJUST_FUNCTION)
			charcode = AdjustCharcode(charcode, false);
	}
	
	return charcode;
}

uint16 
BTextCodec::AdjustCode(uint16 code, bool decode) const
{
	return code;
}

#define replaceCharStart(x) fReplaces[x * 2]
#define replaceCharEnd(x) replaceCharStart(x)
#define replaceUnicode(x) fReplaces[(x * 2) + 1]

#define runCharStart(x) fRuns[x * 3]
#define runCharEnd(x) fRuns[(x * 3) + 1];
#define runUnicode(x, offset) fRuns[(x * 3) + 2] + offset

#define blockCharStart(x) fBlocks[fBlockIndex[x]]
#define blockCharend(x) fBlocks[fBlockIndex[x] + 1]
#define blockUnicode(x, offset) fBlocks[fBlockIndex[x] + 2 + offset]


uint16 
BTextCodec::BinarySearchFor(uint16 charcode, uint8 which)
{
	uint16 unicode = 0xffff;
	
	uint16 lowerBound = 0;
	uint16 upperBound = 0;
	tcodec_array array = NULL;

	if (which == REPLACEMENT) {
		upperBound = fReplaceCount;
		array = fReplaces;
	}
	else if (which == RUN) {
		upperBound = fRunCount;
		array = fRuns;
	}
	else if (which == BLOCK) {
		upperBound = fBlockCount;
		array = fBlocks;
	}
	
	while (unicode == 0xffff) {
		bool end = (lowerBound == upperBound);
		uint16 middle = (uint16) (upperBound - lowerBound)/2;
		uint16 checkIndex = lowerBound + middle;
		uint16 start = 0, end = 0;
		
		if (which == REPLACEMENT) {
			uint16 repIndex = checkIndex * 2;
			start = end = fReplaces[repIndex];
			if (charcode < start)
				upperBound = max_c(checkIndex - 1, lowerBound);
			else if (charcode > start)	
				lowerBound = min_c(checkIndex + 1, upperBound);
			else
				unicode = fReplaces[repIndex + 1];
		
		}
		else if (which == RUN) {
			uint16 runIndex = checkIndex * 3;
			start = fRuns[runIndex];
			end = fRuns[runIndex + 1];
			if (charcode < start)
				upperBound = max_c(checkIndex - 1, lowerBound);
			else if (charcode > end)
				lowerBound = min_c(checkIndex + 1, upperBound);
			else {
				// we found it!
				unicode = fRuns[runIdex + 2] + (charcode - start);
			}
		}
		else if (which == BLOCK) {
			uint16 blockIndex = fBlocks[fBlockIndex[checkIndex]];
			start = fBlocks[blockIndex];
			end = fBlocks[blockIndex + 1];
			if (charcode < start)
				upperBound = max_c(checkIndex - 1, lowerBound);
			else if (charcode > end)
				lowerBound = max_c(checkIndex + 1, upperBound);
			else {
				// found it!
				unicode = fBlocks[blockIndex + 2 + (charcode - start)];
			}
		}
		if (end)
			break;
	}
	return unicode;
}

uint16 
BTextCodec::LinearSearchFor(uint16 code, uint8 array, bool decode)
{
	uint16 outCode = 0xffff;
	
	if (array == REPLACE) {
		uint16 toMatch = 0xffff;
		for (uint16 ix = 0; ix < fReplaceCount; ix++) {
			toMatch = (decode) ? replaceCharStart(ix) : replaceUnicode(ix) ;		
			if (toMatch == code) {
				outCode = (decode) ? replaceUnicode(ix) : replaceCharStart(ix);
				break;
			}
		}
	}
	else if (array == RUN) {
		for (uint16 ix = 0; ix < fRunCount; ix++) {
			uint16 charStart =  runCharStart(ix);
			uint16 charEnd = runCharEnd(ix);
			uint16 uniStart = runUnicode(ix, 0);
			uint16 uniEnd = uniStart + charEnd - charStart;
			if (decode && code >= charStart && code <= charEnd) {
				outCode = uniStart + code - charStart;
				break;
			}
			else if (!decode && code >= uniStart && code <= uniEnd) {
				outCode = charStart + code - uniStart;
				break;
			}
		}
	}
	else if (array == BLOCK) {
		for (uint16 ix = 0; ix < fBlockCount; ix++) {
			uint16 charStart = blockCharStart(ix);
			uint16 charEnd = blockCharEnd(ix);
			if (!decode) {
				uint16 count = charEnd - charStart + 1;
				for (int jx = 0; jx < count; jx++) {
					if (code == blockUnicode(ix, jx)) {
						outCode = charStart + jx;
						break;
					}
				}
				if (outCode != 0xffff)
					break;
			}
			else {
				// we can see if the value
				if (code >= charStart && code <= charEnd) {
					outCode = blockUnicode(ix, code - charStart);
					break;
				}
			}
		}
	}
	return outCode;

}

