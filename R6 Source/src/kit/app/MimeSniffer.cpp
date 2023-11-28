#if _SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING

#include "MimeSniffer.h"

#include <ByteOrder.h>
#include <Debug.h>
#include <Mime.h>
#include <Path.h>

#include <algorithm>
#include <ctype.h>
#include <elf.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <new>

using namespace BPrivate;
// ToDo:
//
// Add support for a sniff pattern with a mask:
// ("AB\000\000CD" & 0xFFFF0000FFFF)
//
// Add support for algoritmic sniffer plugins

template <class InitCheckable>
void
InitCheck(InitCheckable *item) throw (status_t)
{
	if (!item)
		throw B_ERROR;
	status_t error = item->InitCheck();
	if (error != B_OK)
		throw error;
}


Pattern::Pattern(const BString &string)
{
	int32 length = string.Length();
	// no self-asignment check here for simplicity
	if (length) {
		memcpy(LockBuffer(length), string.String(), length);
		UnlockBuffer(length);
	}
}

Pattern::Pattern(const Pattern &string)
	:	BString()
{
	int32 length = string.Length();
	// no self-asignment check here for simplicity
	if (length) {
		memcpy(LockBuffer(length), string.String(), length);
		UnlockBuffer(length);
	}
}

Pattern::Pattern(const char *buffer, int32 length)
{
	if (length) {
		memcpy(LockBuffer(length), buffer, length);
		UnlockBuffer(length);
	}
}

void 
Pattern::SetTo(const char *buffer, int32 length)
{
	if (length) {
		memcpy(LockBuffer(length), buffer, length);
		UnlockBuffer(length);
	} else
		BString::operator=("");
}

// #pragma mark -

SniffBuffer::SniffBuffer(BPositionIO *target)
	:	buffer(NULL),
		bufferSize(0),
		target(target),
		externalBuffer(NULL)
{
	targetSize = target->Seek(0, SEEK_END);
}


SniffBuffer::SniffBuffer(const void *externalBuffer, int32 externalBufferSize)
	:	buffer(NULL),
		bufferSize(externalBufferSize),
		target(NULL),
		targetSize(externalBufferSize),
		externalBuffer(externalBuffer)
{
	buffer = (char *)externalBuffer;
}


SniffBuffer::~SniffBuffer()
{
	if (!externalBuffer)
		free(buffer);
}

bool 
SniffBuffer::CaseSensitiveMatch(const char *pattern, off_t offset) const
{
	int32 patternLength = strlen(pattern);
	FillBufferIfNeeded(offset + patternLength);
	if (bufferSize < offset + patternLength)
		return false;
	
	ASSERT(buffer);
	return memcmp(buffer + offset, pattern, patternLength) == 0;
}

bool 
SniffBuffer::CaseSensitiveMatch(const BString &pattern, off_t offset) const
{
	FillBufferIfNeeded(offset + pattern.Length());
	if (bufferSize < offset + pattern.Length())
		return false;
	
	ASSERT(buffer);
	return memcmp(buffer + offset, pattern.String(), pattern.Length()) == 0;
}

bool 
SniffBuffer::CaseSensitiveMatchWithMask(const BString &pattern,
	const BString &mask, off_t offset) const
{
	FillBufferIfNeeded(offset + pattern.Length());
	if (bufferSize < offset + pattern.Length())
		return false;
	
	ASSERT(buffer);

	char *matchedBuffer = buffer + offset;
	int32 length = pattern.Length();
	for (int32 index = 0; index < length; index++) 
		if ((matchedBuffer[index] & mask[index]) != pattern[index])
			return false;

	return true;
}

bool 
SniffBuffer::CaseInsensitiveMatch(const BString &pattern, off_t offset) const
{
	FillBufferIfNeeded(offset + pattern.Length());
	if (bufferSize < offset + pattern.Length())
		return false;
	
	ASSERT(buffer);
	return strncasecmp(buffer + offset, pattern.String(), pattern.Length()) == 0;
}

void 
SniffBuffer::FillBuffer(off_t size)
{
	if (externalBuffer || size <= bufferSize || size > targetSize
		|| size > kBufferReadMax)
		// we are either fine, have the whole file or reached the
		// limit ammout of file we are ready to read
		return;
	
	// read some more into the buffer, pin to some reasonable values
	
	// don't bother reading too litle
	if (size < kBufferReadMin)
		size = kBufferReadMin;
	else
		size = bufferSize + kBufferReadSlop;
	
	// don't read too much either
	size = std::min(size, std::min(kBufferReadMax, targetSize));

	if (!size) {
		ASSERT(bufferSize == 0);
		return;
	}
	
	// make room for more data
	buffer = (char *)realloc(buffer, size);
	if (!buffer)
		throw std::bad_alloc();
	
	// read it
	size_t readSize = (size_t)(size - bufferSize);
	ssize_t readResult = target->ReadAt(bufferSize, buffer + bufferSize, readSize);
	
	if (readResult < 0)
		throw (status_t)readResult;
	
	bufferSize += readResult;
}

// #pragma mark -

namespace BPrivate {

class ScanError : public Error {
public:
	ScanError(const char *message, int32 offset)
		:	Error(message, offset)
		{}

	ScanError(const char *message, const Sniffer::RuleScanner *scanner)
		:	Error(message, scanner->Offset())
		{}
};

}

Sniffer::RuleScanner::RuleScanner(const char *stream, size_t streamLength)
	:	stream(stream),
		streamLength(streamLength),
		offset(-1)
{
}

Sniffer::RuleScanner::TokenType 
Sniffer::RuleScanner::ReturnToken(int32 tokenOffset, int32 tokenEnd)
{
	if (deescapingToken) {
		tokenValue = deescapedToken.String();
		length = deescapedToken.Length();
		return currentToken;
	}
	
	tokenValue = &stream[tokenOffset];
	length = tokenEnd - tokenOffset;
	return currentToken;
}

Sniffer::RuleScanner::TokenType 
Sniffer::RuleScanner::ReturnToken(TokenType token, int32 tokenOffset, int32 tokenEnd)
{
	currentToken = token;
	return ReturnToken(tokenOffset, tokenEnd);
}

void 
Sniffer::RuleScanner::SkipComment()
{
	for (;;) {
		++offset;
		if (offset >= (int32)streamLength || stream[offset] == EOF)
			return;
		
		if (stream[offset] == '\n')
			return;
	}
}

// string literals can be either optionally quoted strings with
// octal non-printing characters or hexadecimal streams

Sniffer::RuleScanner::TokenType 
Sniffer::RuleScanner::GetNextToken() throw (Error)
{
	// toss white space
	for (;;) {
		++offset;
		if (offset >= (int32)streamLength || stream[offset] == EOF)
			return ReturnToken(kEnd, offset, offset);
		
		if (stream[offset] == '#') 
			SkipComment();

		if (stream[offset] != ' ' && stream[offset] != '\t' && stream[offset] != '\n')
			break;		
	}
	
	bool sawSingleQuote = false;
	bool sawDoubleQuote = false;
	bool sawBackSlash = false;
	int32 scanningOctalDigit = 0;
	int32 scanningHexDigit = 0;
	int32 currentHexDigit = -1;
	uchar octalDigit = 0;

	deescapingToken = false;
	tokenStart = offset;
	currentToken = kControlToken;

	if (stream[offset] == '\'') {
		sawSingleQuote = true;
		tokenStart++;
		offset++;
		currentToken = kStringLiteral;
	} else if (stream[offset] == '\"') {
		sawDoubleQuote = true;
		tokenStart++;
		offset++;
		currentToken = kStringLiteral;
	} else if (stream[offset] == '0' && stream[offset + 1] == 'x') {
		scanningHexDigit = true;
		if (!deescapingToken)
			deescapedToken = "";
		deescapingToken = true;
		offset += 2;
	}
	
	for (;;) {

		char ch;
		if (offset >= (int32)streamLength)
			ch = EOF;
		else
			ch = stream[offset];
		
		if (scanningHexDigit) {

			// handle pattern as hexadecimal stream
			
			int32 thisNibble;

			switch (ch) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					thisNibble = ch - '0';
					break;
				
				case 'a':
				case 'b':
				case 'c':
				case 'd':
				case 'e':
				case 'f':
					thisNibble = ch + 10 - 'a';
					break;

				case 'A':
				case 'B':
				case 'C':
				case 'D':
				case 'E':
				case 'F':
					thisNibble = ch + 10 - 'A';
					break;
				
				default:
					if (currentHexDigit >= 0)
						// got first nibble, need the second one!
						throw ScanError("bad hex literal", offset);
					
					--offset;
					return ReturnToken(kStringLiteral, offset, offset);
			}
			
			if (currentHexDigit < 0)
				currentHexDigit = thisNibble;
			else {
				deescapedToken += currentHexDigit * 16 + thisNibble;
				currentHexDigit = -1;
			}
			
		} else { 

			if (scanningOctalDigit)
				// handle character escaped as an octal number
				switch (ch) {
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
						break;
	
					default:
						deescapedToken += octalDigit;
						scanningOctalDigit = 0;
						break;
				}
	
	
			switch (ch) {
				case EOF:
					return ReturnToken(tokenStart, offset);
				
				case '\\':
					if (!sawBackSlash) {
						sawBackSlash = true;
						currentToken = kStringLiteral;
						if (!deescapingToken) {
							deescapingToken = true;
							deescapedToken.SetTo(&stream[tokenStart], offset - tokenStart);
						}
					} else if (deescapingToken) {
						deescapedToken += ch;
						sawBackSlash = false;
					}
					break;
	
				case '\n':
					if (sawBackSlash)
						// ignore newline if preceded by backslash
						break;
					
					if (sawSingleQuote || sawDoubleQuote)
						throw ScanError("unterminated quote", offset);
	
					// else terminate scanning
					return ReturnToken(tokenStart, offset);
				
				case '\t':
				case ' ':
					if (!sawBackSlash && !sawSingleQuote && !sawDoubleQuote)
						// white space terminates the token 
						return ReturnToken(tokenStart, offset);
	
					if (deescapingToken) 
						deescapedToken += ch;
	
					break;
					
				case '\'':
					if (!sawBackSlash && sawSingleQuote)
						return ReturnToken(tokenStart, offset);
	
					if (!sawBackSlash && !sawDoubleQuote)
						throw ScanError("misplaced single quote", offset);
					
					if (deescapingToken) 
						deescapedToken += ch;
					
					break;
	
				case '\"':
					if (!sawBackSlash && sawDoubleQuote)
						return ReturnToken(tokenStart, offset);
	
					if (!sawBackSlash && !sawSingleQuote)
						throw ScanError("misplaced double quote", offset);
					
					if (deescapingToken) 
						deescapedToken += ch;
	
					break;
	
					
				case '&':
				case '|':
				case ':':
				case '[':
				case '(':
				case ']':
				case ')':
					if (sawBackSlash || sawDoubleQuote || sawSingleQuote) {
						if (deescapingToken) 
							deescapedToken += ch;
					} else if (offset == tokenStart) 
						// return paren, etc. itself
						return ReturnToken(kControlToken, offset, offset + 1);
					else {
						// paren, etc. terminates previous token, back off
						--offset;
						return ReturnToken(tokenStart, offset + 1);
					}
					break;
	
				case '-':
					if (sawBackSlash || sawDoubleQuote || sawSingleQuote) {
						if (deescapingToken) 
							deescapedToken += ch;
					} else if (offset == tokenStart) {
						// return option (for now only single character -i supported for now)
						++offset;
						return ReturnToken(kOptionToken, tokenStart, offset + 1);
					} else {
						// paren, etc. terminates previous token, back off
						--offset;
						return ReturnToken(tokenStart, offset + 1);
					}
					break;
					
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
	
					if (sawBackSlash) {
						// start escaping an octal digit
	
						octalDigit = ch - '0';
						scanningOctalDigit++;
	
						if (!deescapingToken) {
							deescapingToken = true;
							deescapedToken.SetTo(&stream[tokenStart], offset - tokenStart);
						}
						
					} else if (scanningOctalDigit) {
						// continue escaping an octal digit
						octalDigit = 8*octalDigit + (ch - '0');
						scanningOctalDigit++;
	
						if (scanningOctalDigit >= 3) {
							// we are done, push octal digit
							deescapedToken += octalDigit;
							scanningOctalDigit = 0;
						}
						
					} else {
						// normal digit
						if (deescapingToken) 
							deescapedToken += ch;
					}
					break;
					
				default:
					if (deescapingToken) 
						deescapedToken += ch;
					break;
	
			}
		}

		if (sawBackSlash && ch != '\\')
			sawBackSlash = false;


		offset++;
	}
	TRESPASS();
	return ReturnToken(kEnd, offset, offset);
}

Sniffer::RuleScanner::TokenType 
Sniffer::RuleScanner::ExpectNextToken(const char *missingMessage) throw (Error)
{
	TokenType result = GetNextToken();
	if (result == kEnd)
		throw ScanError(missingMessage, offset);

	return result;
}

void 
Sniffer::RuleScanner::ExpectNextToken(RuleScanner::TokenType type,
	const char *errorMessage) throw (Error)
{
	if (GetNextToken() != type)
		throw ScanError(errorMessage, offset);
}

void 
Sniffer::RuleScanner::ExpectNextToken(RuleScanner::TokenType type, const char *expectedValue,
	const char *errorMessage) throw (Error)
{
	if (GetNextToken() != type
		|| strncmp(TokenValue(), expectedValue, strlen(expectedValue)) != 0)
		throw ScanError(errorMessage, offset);
}

int32 
Sniffer::RuleScanner::ExpectInt32(const char *missingMessage, const char *badFormatMessage) throw (Error)
{
	ExpectNextToken(missingMessage);
	int32 result;
	if (sscanf(TokenValue(), "%ld", &result) != 1)
		throw ScanError(badFormatMessage, offset);
	
	return result;
}

void 
Sniffer::RuleScanner::PrintScanError(const Error *error) const
{
	const char *stream = Stream();
	printf("%s", stream);
	if (stream[strlen(stream) - 1] != '\n')
		printf("\n");

	for (int32 count = error->Offset(); count > 0; count--)
		printf(" ");	
	printf("^    Sniffer pattern error: %s\n", error->ErrorMessage());
}

void 
Sniffer::RuleScanner::PrintScanError(const Error *error, BString &buffer) const
{
	const char *stream = Stream();
	buffer << stream;
	if (stream[strlen(stream) - 1] != '\n')
		buffer << "\n";

	buffer.Append(' ', error->Offset());
	buffer << "^    Sniffer pattern error: " << error->ErrorMessage() << "\n";
}


// #pragma mark -

#define kSpaces "                                                                       "

static BString &
PadStringTo(BString &string, int32 size)
{

	if (string.Length() < size) {
		int32 padLength = size - string.Length();
		if (padLength > (ssize_t)sizeof(kSpaces))
			padLength = sizeof(kSpaces);

		string.Append(kSpaces, padLength);
	}
	return string;
}

static BString &
PrePadStringTo(BString &string, int32 size)
{

	if (string.Length() < size) {
		int32 padLength = size - string.Length();
		if (padLength > (ssize_t)sizeof(kSpaces))
			padLength = sizeof(kSpaces);

#if 0
		if (!string.Length())
			string.Append(kSpaces, padLength);
			// temporary workaround for a BString bug
		else
#endif
			string.Prepend(kSpaces, padLength);
	}
	return string;
}

static const char *
EscapeNonPrintingToOctal(const char *source, int32 length, BString &result)
{
	result = "";
	for (; length > 0; source++, length--) {
		uchar tmp = (uchar)(*source);
		if (tmp == '\\' || tmp == '\"' || tmp == '\"'/* || tmp == ' '*/)
			result << "\\" << *source;			
		else if (tmp >= ' ' && tmp <= 127)
			result += *source;
		else {
			result << '\\'
				<< (char)((tmp / 64) + '0')
				<< (char)(((tmp / 8) & 7) + '0')
				<< (char)((tmp & 7) + '0');
		}
	}
	return result.String();
}

static const char *
HexDump(const uchar *source, int32 length, BString &result)
{
	result = "0x";
	for (int32 index = 0; index < length; index++)
	{
		result << "0123456789abcdef"[source[index] >> 4];
		result << "0123456789abcdef"[source[index] & 0xf];
	}
	return result.String();
}

void 
SnifferRuleBase::Dump(BString &dumpBuffer, bool ruleOnly) const
{
	if (!ruleOnly) {
		BString tmp;
		tmp << '\"' << type.String() << '\"';
		PadStringTo(tmp, 36).String();
		dumpBuffer << tmp << " ";
	}
	dumpBuffer << matchLevel;
}

// #pragma mark -

SimpleSnifferRule::SimpleSnifferRule(const char *type, float matchLevel, const BString &pattern,
	off_t sniffOffsetStart)
	:	SnifferRuleBase(type, matchLevel),
		pattern(pattern),
		sniffOffsetStart(sniffOffsetStart)
{
}

bool 
SimpleSnifferRule::Match(const SniffBuffer *buffer) const
{
	return buffer->CaseSensitiveMatch(pattern, sniffOffsetStart);
}

const int32 kRangePrepadValue = 8;

void 
SimpleSnifferRule::Dump(BString &dumpBuffer, bool ruleOnly) const
{
	SnifferRuleBase::Dump(dumpBuffer, ruleOnly);
	BString tmpBuffer;
	
	if (sniffOffsetStart != 0) 
		tmpBuffer << '[' << sniffOffsetStart << ']';
	
	BString tmpPattern;
	dumpBuffer << " " << PrePadStringTo(tmpBuffer, kRangePrepadValue)
		<< " (\"" << EscapeNonPrintingToOctal(pattern.String(), pattern.Length(), tmpPattern)
		<< "\")";
}

// #pragma mark -

RangeSnifferRule::RangeSnifferRule(const char *type, float matchLevel, const BString &pattern,
	off_t newSniffOffsetStart, off_t newSniffOffsetEnd)
	:	SimpleSnifferRule(type, matchLevel, pattern, newSniffOffsetStart),
		sniffOffsetEnd(newSniffOffsetEnd)
{
}

bool 
RangeSnifferRule::Match(const SniffBuffer *buffer) const
{
	for (off_t offset = sniffOffsetStart; offset <= sniffOffsetEnd; offset++)
		if (buffer->CaseSensitiveMatch(pattern, offset))
			return true;
	
	return false;
}


void 
RangeSnifferRule::Dump(BString &dumpBuffer, bool ruleOnly) const
{
	SnifferRuleBase::Dump(dumpBuffer, ruleOnly);
	BString tmpBuffer;
	
	if (sniffOffsetStart != 0) {
		tmpBuffer << '[' << sniffOffsetStart;
		if (sniffOffsetEnd > sniffOffsetStart)
			tmpBuffer << ':' << sniffOffsetEnd;
		tmpBuffer << ']';
	}
	
	BString tmpPattern;
	dumpBuffer << " " << PrePadStringTo(tmpBuffer, kRangePrepadValue)
		<< " (\"" << EscapeNonPrintingToOctal(pattern.String(), pattern.Length(), tmpPattern)
		<< "\")";
}

// #pragma mark -

RangeSnifferRuleWithMask::RangeSnifferRuleWithMask(const char *type,
	float matchLevel, const BString &newPattern, const BString &mask,
	off_t sniffOffsetStart, off_t sniffOffsetEnd)
	:	RangeSnifferRule(type, matchLevel, newPattern, sniffOffsetStart, sniffOffsetEnd),
		mask(mask)
{
	ASSERT(mask.Length() == pattern.Length());

	// make sure the pattern is masked out by the mask so that we don't have
	// to do it during matching
	int32 length = pattern.Length();
	for (int32 index = 0; index < length; index++)
		pattern[index] = (pattern[index] & mask[index]);
}

bool 
RangeSnifferRuleWithMask::Match(const SniffBuffer *buffer) const
{
	for (off_t offset = sniffOffsetStart; offset <= sniffOffsetEnd; offset++)
		if (buffer->CaseSensitiveMatchWithMask(pattern, mask, offset))
			return true;
	
	return false;
}

void 
RangeSnifferRuleWithMask::Dump(BString &dumpBuffer, bool ruleOnly) const
{
	SnifferRuleBase::Dump(dumpBuffer, ruleOnly);
	BString tmpBuffer;
	
	if (sniffOffsetStart != 0) {
		tmpBuffer << '[' << sniffOffsetStart;
		if (sniffOffsetEnd > sniffOffsetStart)
			tmpBuffer << ':' << sniffOffsetEnd;
		tmpBuffer << ']';
	}

	// replace the masked out bits of the pattern with a '-', rather than printing
	// and escaped \000
	Pattern tmpPattern(pattern);
	int32 length = tmpPattern.Length();
	for (int32 index = 0; index < length; index++)
		if (mask[index] == 0)
			tmpPattern[index] = '-';
	
	
	BString escapedPattern;
	BString tmpMask;
	dumpBuffer << " " << PrePadStringTo(tmpBuffer, kRangePrepadValue)
		<< " (\""
		<< EscapeNonPrintingToOctal(tmpPattern.String(), tmpPattern.Length(), escapedPattern)
		<< "\""
		<< " & " << HexDump((const uchar *)mask.String(), mask.Length(), tmpMask)
		<< " )";
}

// #pragma mark -

SingleRangeMultiplePatternSnifferRule::SingleRangeMultiplePatternSnifferRule(const char *type,
	float matchLevel, BObjectList<Pattern> *patternList, off_t sniffOffsetStart,
	off_t sniffOffsetEnd, bool caseInsensitive)
	:	SnifferRuleBase(type, matchLevel),
		patternList(patternList),
		sniffOffsetStart(sniffOffsetStart),
		sniffOffsetEnd(sniffOffsetEnd),
		caseInsensitive(caseInsensitive)
{
}


SingleRangeMultiplePatternSnifferRule::~SingleRangeMultiplePatternSnifferRule()
{
	delete patternList;
}

bool 
SingleRangeMultiplePatternSnifferRule::Match(const SniffBuffer *buffer) const
{
	for (off_t offset = sniffOffsetStart; offset <= sniffOffsetEnd; offset++) {
		int32 count = patternList->CountItems();
		for (int32 index = 0; index < count; index++)
			if (caseInsensitive) {
				if (buffer->CaseInsensitiveMatch(*patternList->ItemAt(index), offset))
					return true;
			} else if (buffer->CaseSensitiveMatch(*patternList->ItemAt(index), offset))
				return true;
	}
	
	return false;
}

void 
SingleRangeMultiplePatternSnifferRule::Dump(BString &dumpBuffer, bool ruleOnly) const
{
	SnifferRuleBase::Dump(dumpBuffer, ruleOnly);
	BString tmpPattern;
	BString tmpBuffer;
	
	if (sniffOffsetStart != 0) {
		tmpBuffer << '[' << sniffOffsetStart;
		if (sniffOffsetEnd > sniffOffsetStart)
			tmpBuffer << ':' << sniffOffsetEnd;
		tmpBuffer << ']';
	}
	
	dumpBuffer << " " << PrePadStringTo(tmpBuffer, kRangePrepadValue)
		<< " (" << (caseInsensitive ? " -i " : "");
	
	int32 count = patternList->CountItems();
	bool first = true;
	for (int32 index = 0; index < count; index++) {
		if (!first)
			dumpBuffer << " | ";
		first = false;
		dumpBuffer << "\"" << EscapeNonPrintingToOctal(patternList->ItemAt(index)->String(),
			patternList->ItemAt(index)->Length(), tmpPattern) << "\"";
	}
	dumpBuffer << ")";
}

// #pragma mark -

OptimizedSingleRangeMultiplePatternSnifferRule::OptimizedSingleRangeMultiplePatternSnifferRule(
	const char *type, float matchLevel, BObjectList<Pattern> *patternList,
	off_t sniffOffsetStart, off_t sniffOffsetEnd, bool caseInsensitive)
	:	SingleRangeMultiplePatternSnifferRule(type, matchLevel, patternList,
			sniffOffsetStart, sniffOffsetEnd, caseInsensitive)
{
	firstPatternCharacter = patternList->ItemAt(0)->String()[0];
}

bool 
OptimizedSingleRangeMultiplePatternSnifferRule::Match(const SniffBuffer *buffer) const
{
	for (off_t offset = sniffOffsetStart; offset <= sniffOffsetEnd; offset++) {
		if (!buffer->CaseSensitiveMatchFirst(firstPatternCharacter, offset))
			continue;
		int32 count = patternList->CountItems();
		for (int32 index = 0; index < count; index++)
			if (caseInsensitive) {
				if (buffer->CaseInsensitiveMatch(*patternList->ItemAt(index), offset))
					return true;
			} else if (buffer->CaseSensitiveMatch(*patternList->ItemAt(index), offset))
				return true;
	}
	
	return false;
}

// #pragma mark -


MultiplePatternSnifferRule::MultiplePatternSnifferRule(const char *type, float matchLevel,
	BObjectList<PatternAtOffsetRange> *patternList, bool caseInsensitive)
	:	SnifferRuleBase(type, matchLevel),
		patternList(patternList),
		caseInsensitive(caseInsensitive)
{
}


MultiplePatternSnifferRule::~MultiplePatternSnifferRule()
{
	delete patternList;
}

bool 
MultiplePatternSnifferRule::Match(const SniffBuffer *buffer) const
{
	int32 count = patternList->CountItems();
	for (int32 index = 0; index < count; index++) {
		PatternAtOffsetRange *pattern = patternList->ItemAt(index);
		for (int32 offset = pattern->Start(); offset <= pattern->End(); offset++)
			if (caseInsensitive) {
				if (buffer->CaseInsensitiveMatch(pattern->PatternValue(), offset))
					return true;
			} else if (buffer->CaseSensitiveMatch(pattern->PatternValue(), offset))
				return true;
	}

	return false;
}

void 
MultiplePatternSnifferRule::Dump(BString &dumpBuffer, bool ruleOnly) const
{
	SnifferRuleBase::Dump(dumpBuffer, ruleOnly);
	BString tmpBuffer;

	dumpBuffer << " " << PrePadStringTo(tmpBuffer, kRangePrepadValue)
		<< " (" << (caseInsensitive ? " -i " : "");
	
	int32 count = patternList->CountItems();
	bool first = true;
	for (int32 index = 0; index < count; index++) {
		if (!first)
			dumpBuffer << " | ";
		first = false;

		PatternAtOffsetRange *pattern = patternList->ItemAt(index);

		tmpBuffer = "";
		tmpBuffer << pattern->Start();
		if (pattern->End() > pattern->Start())
			tmpBuffer << ':' << pattern->End();

		BString tmpPattern;
		dumpBuffer << "[" << tmpBuffer << "]\""
			<< EscapeNonPrintingToOctal(pattern->PatternValue().String(),
				pattern->PatternValue().Length(), tmpPattern) << "\"";
	}
	dumpBuffer << ")";
}

// #pragma mark -


AlgorithmicSniffer::AlgorithmicSniffer(const char *type, float matchLevel,
	bool (*matcher)(const SniffBuffer *, off_t , off_t),
	off_t sniffOffsetStart, off_t sniffOffsetEnd)
	:	SnifferRuleBase(type, matchLevel),
		matcher(matcher),
		rangeStart(sniffOffsetStart),
		rangeEnd(sniffOffsetEnd)
{
}

bool 
AlgorithmicSniffer::Match(const SniffBuffer *buffer) const
{
	return (matcher)(buffer, rangeStart, rangeEnd);
}

void 
AlgorithmicSniffer::Dump(BString &dumpBuffer, bool ruleOnly) const
{
	SnifferRuleBase::Dump(dumpBuffer, ruleOnly);
	dumpBuffer << "    # algorithmic sniffer";
}

bool
Sniffer::LooksLikeText(const SniffBuffer *buffer, off_t rangeStart, off_t rangeEnd)
{		
	buffer->FillBufferIfNeeded(rangeEnd);

	off_t offset = rangeStart;
	for (; offset < rangeEnd; offset++) {

		if (offset >= buffer->Size())
			return true;
			
		uchar ch = (*buffer)[offset];
		
		if (!isprint(ch) && !isspace(ch) && ch != '\t') {
			// if it's not plain ascii, then try to see if it's UTF-8

			if ((ch & 0xc0) != 0xc0)
				return false;
			
			if (offset + 1 >= buffer->Size()) 
				return true;

			if ((ch & 0x20) == 0) {
				// 2-byte utf
				if (((*buffer)[offset + 1] & 0xc0) != 0x80)
					return false;
				
				offset++;
			} else if ((ch & 0x30) == 0x20) {
				 // 3 byte utf
				if (offset + 2 >= buffer->Size()) 
					return true;

				if (((*buffer)[offset + 1] & 0xc0) != 0x80
					|| ((*buffer)[offset + 2] & 0xc0) != 0x80)
					return false;

				offset += 2;
			} else if ((ch & 0x30) == 0x30) {
				 // 4 byte utf
				if ((ch & 0x08) != 0)
					return false;
				
				if (offset + 3 >= buffer->Size()) 
					return true;

						
				if (((*buffer)[offset + 1] & 0xc0) != 0x80
					|| ((*buffer)[offset + 2] & 0xc0) != 0x80
					|| ((*buffer)[offset + 3] & 0xc0) != 0x80)
					return false;

				offset += 3;
			}
		}
	}
	
	return true;
}

static bool
LooksLikeTarga(const SniffBuffer *buffer, off_t rangeStart, off_t rangeEnd)
{
	if (rangeEnd < rangeStart + 17 || buffer->Size() < rangeStart + 17)
		return false;

    int pixelSize = ((*buffer)[rangeStart + 16]) >> 3;

	uchar ch1 = (*buffer)[rangeStart + 1];
	uchar ch2 = (*buffer)[rangeStart + 2];
	
	return ((ch1 == 1 && (ch2 == 1 || ch2 == 9))
			|| (ch1 == 0 && (ch2 == 2 || ch2 == 3 || ch2 == 10 || ch2 == 11))
		&& pixelSize >= 1 && pixelSize <= 4 && ((*buffer)[rangeStart + 16] & 0x7) == 0);
}

static bool
LooksLikeTrueType(const SniffBuffer *buffer, off_t rangeStart, off_t rangeEnd)
{
	struct SFNTOffsetTable {
		int32	version;
		uint16	numOffsets;
		uint16	searchRange;
		uint16	entrySelector;
		uint16	rangeShift;
	};

	if (rangeEnd < rangeStart + sizeof(SFNTOffsetTable)
		|| buffer->Size() < rangeStart + sizeof(SFNTOffsetTable))
		return false;

	SFNTOffsetTable *table = (SFNTOffsetTable *)buffer->Buffer(sizeof(SFNTOffsetTable));

	return (B_BENDIAN_TO_HOST_INT32(table->version) == 0x00010000)
		&& (B_BENDIAN_TO_HOST_INT16(table->rangeShift)
			== (B_BENDIAN_TO_HOST_INT16(table->numOffsets) * 16)
				- B_BENDIAN_TO_HOST_INT16(table->searchRange)) ;
}

static bool
LooksLikeMP3(const SniffBuffer *buffer, off_t rangeStart, off_t rangeEnd)
{
	for (off_t offset = rangeStart; offset <= rangeEnd; offset++) {

		if (buffer->Size() < offset + 4)
			return false;
	
		if ((*buffer)[rangeStart] != 0xff)			// sync field
			continue;
	
		uchar ch1 = (*buffer)[rangeStart + 1];
		if ((ch1 & 0xf6) != 0xf2 					// layer != 3
			&& (ch1 & 0xf6) != 0xf4)				// layer != 2
			continue;
	
		uchar ch2 = (*buffer)[rangeStart + 2];
		
		if ((ch2 & 0xf0) == 0xf0)	// bitrate == 15
			continue;
	
		if ((ch2 & 0x0c) == 0x0c)	// sampling rate index == 3
			continue;
	
		if (((*buffer)[rangeStart + 3] & 3) == 2)			// emphasis == 2
			continue;
		
		return true;
	}
	
	return false;
}

static bool
LooksLikeELFApp(const SniffBuffer *buffer, off_t rangeStart, off_t rangeEnd)
{
	if (rangeEnd < rangeStart + sizeof(Elf32_Ehdr) || buffer->Size() < sizeof(Elf32_Ehdr))
		return false;

	if (!buffer->CaseSensitiveMatch(ELFMAG, rangeStart))
		return false;
	
	Elf32_Ehdr *header = (Elf32_Ehdr *)buffer->Buffer(sizeof(Elf32_Ehdr));
	return header->e_type == B_HOST_TO_LENDIAN_INT16(ET_DYN)
		|| header->e_type == B_HOST_TO_LENDIAN_INT16(ET_EXEC);
}

static bool
LooksLikeELFObj(const SniffBuffer *buffer, off_t rangeStart, off_t rangeEnd)
{
	if (rangeEnd < rangeStart + sizeof(Elf32_Ehdr) || buffer->Size() < sizeof(Elf32_Ehdr))
		return false;

	if (!buffer->CaseSensitiveMatch(ELFMAG, rangeStart))
		return false;
	
	Elf32_Ehdr *header = (Elf32_Ehdr *)buffer->Buffer(sizeof(Elf32_Ehdr));
	return header->e_type == B_HOST_TO_LENDIAN_INT16(ET_REL);
}

// #pragma mark -

Sniffer::Sniffer()
{
	// this is a private constructor used just by the CheckRule call,
	// we don't need to set up the whole object here
}

Sniffer::Sniffer(bool readRulesFromMIMEDatabase)
	:	rules(100, true)
{
	if (readRulesFromMIMEDatabase) 
		AddRulesFromMIMEDatabase();
	
	AddRuleUnique(new AlgorithmicSniffer("image/x-targa", 0.15, &LooksLikeTarga, 0, 64));
	AddRuleUnique(new AlgorithmicSniffer("application/x-truetype", 0.15, &LooksLikeTrueType, 0, 64));
	AddRuleUnique(new AlgorithmicSniffer("application/x-vnd.Be-elfexecutable", 0.6, &LooksLikeELFApp, 0, 64));
	AddRuleUnique(new AlgorithmicSniffer("application/x-vnd.Be.ELF-object", 0.6, &LooksLikeELFObj, 0, 64));
	AddRuleUnique(new AlgorithmicSniffer("audio/x-mpeg", 0.15, &LooksLikeMP3, 0, 0));
	
}

Sniffer::Sniffer(const entry_ref *ruleFile)
	:	rules(100, true)
{
	// parse rules from a rule file
	AddRules(ruleFile);
	
	// add algorithmic sniffers
	AddRuleUnique(new AlgorithmicSniffer("text/plain", 0.1, &Sniffer::LooksLikeText, 0, 64));
	AddRuleUnique(new AlgorithmicSniffer("image/x-targa", 0.15, &LooksLikeTarga, 0, 64));
	AddRuleUnique(new AlgorithmicSniffer("application/x-truetype", 0.15, &LooksLikeTrueType, 0, 64));
	AddRuleUnique(new AlgorithmicSniffer("application/x-vnd.Be-elfexecutable", 0.6, &LooksLikeELFApp, 0, 64));
	AddRuleUnique(new AlgorithmicSniffer("application/x-vnd.Be.ELF-object", 0.6, &LooksLikeELFObj, 0, 64));
	AddRuleUnique(new AlgorithmicSniffer("audio/x-mpeg", 0.15, &LooksLikeMP3, 0, 256));
}

void
Sniffer::AddRulesFromMIMEDatabase()
{
	BMessage types;
	BMimeType::GetInstalledTypes(&types);
	for (int32 index = 0; ;) {
		const char *type;
		if (types.FindString("types", index++, &type) != B_OK)
			break;
		
		BMimeType mimeType(type);
		if (!mimeType.IsValid())
			continue;
		
		BString ruleLine;
		if (mimeType.GetSnifferRule(&ruleLine) != B_OK)
			continue;
		
		RuleScanner scanner(ruleLine.String(), ruleLine.Length());
		
		SnifferRuleBase *rule = GetRule(type, &scanner);
		if (rule) {
			if (!AddRuleUnique(rule)) {
				delete rule;
				printf("rule %s for type %s not added - already in table\n",
					ruleLine.String(), type);
			}
		}
	}
}

const int32 kBufferSize = 20 * 1024;

void 
Sniffer::AddRules(const entry_ref *ruleFile)
{
	BEntry entry(ruleFile);
	BPath path;
	entry.GetPath(&path);
	
	FILE *file = fopen(path.Path(), "r");
	if (!file) {
		PRINT(("Error opening %s\n", path.Path()));
		return;
	}
	
	BString buffer;
	// use a BString as an autoptr
	char *line = buffer.LockBuffer(kBufferSize);

	for (;;) {

		if (fgets(line, kBufferSize, file) == NULL)
			break;

		RuleScanner scanner(line, strlen(line));
		
		SnifferRuleBase *rule = GetRule(&scanner);
		if (rule) {
			if (!AddRuleUnique(rule)) {
				delete rule;
				printf("rule %s not added - already in table\n", line);
			}
		}
	}
	
	fclose(file);
}

SnifferRuleBase *
Sniffer::GetRule(Sniffer::RuleScanner *scanner)
{
	try {
		RuleScanner::TokenType token = scanner->GetNextToken();
		if (token == RuleScanner::kEnd)
			return NULL;

		if (token != RuleScanner::kStringLiteral)
			throw ScanError("expecting MIME type", scanner);
		
		BString typeString;
		typeString.SetTo(scanner->TokenValue(), scanner->TokenLength());
		BMimeType type(typeString.String());
		if (!type.IsValid())
			throw ScanError("valid MIME type expected", scanner);
		
		return GetRule(typeString.String(), scanner);
	}

	catch (Error error) {
		scanner->PrintScanError(&error);
	}

	catch (...) {
		printf("unknown error\n");
	}

	return NULL;
}

struct MatchByTypePredicate : UnaryPredicate<SnifferRuleBase> {
	MatchByTypePredicate(const char *type)
		:	type(type)
		{}
	
	virtual int operator()(const SnifferRuleBase *item) const
		{ return strcmp(type, item->Type()); }
	
	const char *type;
};

void
Sniffer::RuleChanged(const char *type, const char *rule)
{
	try {
		
		SnifferRuleBase *oldRule = rules.FindIf(MatchByTypePredicate(type));
		RuleScanner scanner(rule, strlen(rule));
		SnifferRuleBase *newRule = GetRuleCommon(type, &scanner);
		
		if (oldRule) 
			rules.RemoveItem(oldRule);
		
		AddRuleUnique(newRule);
		
	} catch (...) {
		printf("error replacing rule\n");
	}
}

SnifferRuleBase *
Sniffer::GetRule(const char *type, RuleScanner *scanner)
{
	try {
		return GetRuleCommon(type, scanner);
	}

	catch (Error error) {
		scanner->PrintScanError(&error);
	}

	catch (...) {
		printf("unknown error\n");
	}

	return NULL;
}

bool 
Sniffer::CheckRule(const char *rule, BString *errorBuffer)
{
	RuleScanner scanner(rule, strlen(rule));
	try {
		delete Sniffer().GetRuleCommon("", &scanner);
		return true;
	}
	
	catch (Error error) {
		if (errorBuffer)
			scanner.PrintScanError(&error, *errorBuffer);
	}

	catch (...) {
		*errorBuffer << "unknown error\n";
	}

	return false;
}


void 
Sniffer::CheckCurrentToken(const Sniffer::RuleScanner *scanner, RuleScanner::TokenType expectedType,
	const char *expectedValue, const char *error) const throw (Error)
{
	if (scanner->Token() != expectedType
		|| strncmp(scanner->TokenValue(), expectedValue, scanner->TokenLength()) != 0)
		throw ScanError(error, scanner);
}

bool 
Sniffer::ParseSniffRange(RuleScanner *scanner, off_t &start, off_t &end) const throw (Error)
{
	if (scanner->Token() != RuleScanner::kControlToken
		|| strncmp(scanner->TokenValue(), "[", scanner->TokenLength()) != 0)
		return false;

	// handle start offset or range
	// [13]
	// [25:28]
	start = scanner->ExpectInt32("missing pattern offset",
		"pattern offset expected");
	
	scanner->ExpectNextToken("unterminated sniff rule");

	if (strncmp(scanner->TokenValue(), ":", scanner->TokenLength()) == 0) {
		end = scanner->ExpectInt32("missing pattern range end",
			"pattern range end expected");
		scanner->GetNextToken();
	
	}
	CheckCurrentToken(scanner, RuleScanner::kControlToken, "]", "missing ']'");
	scanner->ExpectNextToken("missing sniff pattern");

	return true;
}

SnifferRuleBase *
Sniffer::GetRuleCommon(const char *type, RuleScanner *scanner) const throw (Error)
{
	// get the match level
	scanner->ExpectNextToken("missing match level");		
	
	float matchLevel;
	if (sscanf(scanner->TokenValue(), "%f", &matchLevel) != 1)
		throw ScanError("match level expected", scanner);

	scanner->ExpectNextToken("missing sniff rule");
	
	off_t rangeStart = 0;
	off_t rangeEnd = 0;
	bool caseInsensitive = false;

	BObjectList<PatternAtOffsetRange> *multiplePatternList = NULL;
	BObjectList<Pattern> *singleRangeMultiplePaternList = NULL;
	
	bool possibleMultiplePatternRule = !ParseSniffRange(scanner, rangeStart, rangeEnd);
	
	if (scanner->Token() == RuleScanner::kControlToken
		&& strncmp(scanner->TokenValue(), "(", scanner->TokenLength()) != 0)
		throw ScanError("bad token", scanner);
		
	scanner->ExpectNextToken("missing sniff pattern");
	for (;;) {
		
		// check for the '-i' option
		if (scanner->Token() == RuleScanner::kOptionToken
			&& strncmp(scanner->TokenValue(), "-i", scanner->TokenLength()) == 0) {
			caseInsensitive = true;
			scanner->ExpectNextToken("missing sniff pattern");
		}
		
		// check for possible range that would lead to a multiple range multiple pattern rule
		if (possibleMultiplePatternRule) {
			if (ParseSniffRange(scanner, rangeStart, rangeEnd)) {
				if (!multiplePatternList)
					// we are scanning a multiple pattern rule, allocate the list
					multiplePatternList = new BObjectList<PatternAtOffsetRange>(5, true);
			} else if (!multiplePatternList)
				// first item didn't have a range, can't be a multiple pattern rule
				possibleMultiplePatternRule = false;

		}
		
		// allocate singleRangeMultiplePaternList even if we don't know that we
		// will have multiple patterns
		if (!multiplePatternList && !singleRangeMultiplePaternList) 
			singleRangeMultiplePaternList = new BObjectList<Pattern>(5, true);

		if (scanner->Token() != RuleScanner::kStringLiteral)
			throw ScanError("missing pattern", scanner);
		
		Pattern pattern(scanner->TokenValue(), scanner->TokenLength());
		if (!pattern.Length())
			throw ScanError("illegal empty pattern", scanner);
		
		if (rangeEnd < rangeStart)
			rangeEnd = rangeStart;

		scanner->ExpectNextToken("unterminated rule");

		if (scanner->Token() == RuleScanner::kControlToken
			&& strncmp(scanner->TokenValue(), "&", scanner->TokenLength()) == 0) {
			// pattern with a mask
			if (multiplePatternList)
				throw ScanError("Only single pattern with a mask supported", scanner);

			scanner->ExpectNextToken(RuleScanner::kStringLiteral, "missing mask");
			Pattern mask(scanner->TokenValue(), scanner->TokenLength());

			if (!mask.Length())
				throw ScanError("illegal empty mask", scanner);

			if (mask.Length() != pattern.Length())
				throw ScanError("pattern and mask lengths do not match", scanner);

			// we have all we need at this point, return with a rule

			scanner->ExpectNextToken(RuleScanner::kControlToken, ")", "unterminated rule");
			delete singleRangeMultiplePaternList;
			
			return new RangeSnifferRuleWithMask(type, matchLevel, pattern, mask,
				rangeStart, rangeEnd);
		}

		// stash a pattern in one of the two pattern lists
		if (multiplePatternList)
			multiplePatternList->AddItem(
				new PatternAtOffsetRange(pattern, rangeStart, rangeEnd));
		else
			singleRangeMultiplePaternList->AddItem(new Pattern(pattern));

		if (scanner->Token() == RuleScanner::kControlToken
			&& strncmp(scanner->TokenValue(), ")", scanner->TokenLength()) == 0)
			// terminating paren
			break;

		if (scanner->Token() != RuleScanner::kControlToken
			|| strncmp(scanner->TokenValue(), "|", scanner->TokenLength()) != 0)
			throw ScanError("expecting \'|\' or \'&\'", scanner);
		
		scanner->ExpectNextToken("missing sniff pattern");
	}
		
	if (multiplePatternList)
		return new MultiplePatternSnifferRule(type, matchLevel, multiplePatternList,
			caseInsensitive);
	
	int32 patternCount = singleRangeMultiplePaternList->CountItems();
	if (patternCount > 1 || caseInsensitive) {
		// figure out if we can get a multiple pattern rule where the first
		// character of each pattern matches; for now 
		
		char first = singleRangeMultiplePaternList->ItemAt(0)->String()[0];
		bool canOptimize = true;

		// if the rule is case insensitive, can only optimize if first characted
		// is non-alpha (tailored for html tags)
		if (caseInsensitive && tolower(first) != toupper(first))
			canOptimize = false;
			
		if (canOptimize)
			// now check if all the first characters in all the patterns match
			for (int32 index = 1; index < patternCount; index++)
				if (singleRangeMultiplePaternList->ItemAt(index)->String()[0] != first) {
					// can't do it, just create an un-optimized version
					canOptimize = false;
					break;
				}
		
		if (canOptimize) {
			PRINT(("optimized multi-pattern rule for %s\n", type));
			return new OptimizedSingleRangeMultiplePatternSnifferRule(type, matchLevel,
				singleRangeMultiplePaternList, rangeStart, rangeEnd, caseInsensitive);			
		}

		return new SingleRangeMultiplePatternSnifferRule(type, matchLevel,
			singleRangeMultiplePaternList, rangeStart, rangeEnd, caseInsensitive);
	}
	
	// we only got one pattern return single pattern rules
	SnifferRuleBase *result;
	if (rangeEnd > rangeStart)
		result = new RangeSnifferRule(type, matchLevel,
			*singleRangeMultiplePaternList->ItemAt(0), rangeStart, rangeEnd);
	else
		result = new SimpleSnifferRule(type, matchLevel,
			*singleRangeMultiplePaternList->ItemAt(0), rangeStart);

	// the list didn't get consumed by the new rules, delete it
	delete singleRangeMultiplePaternList;
	return result;
}

static int
MatchMatchLevel(const SnifferRuleBase *rule1, const SnifferRuleBase *rule2)
{
	if (rule1->MatchLevel() < rule2->MatchLevel())
		return 1;
	else if (rule1->MatchLevel() > rule2->MatchLevel())
		return -1;

	return 0;
}

bool 
Sniffer::AddRuleUnique(SnifferRuleBase *newRule)
{
	if (rules.FindIf(MatchByTypePredicate(newRule->Type())))
		// rule for type already exists, don't add a new one
		return false;

	rules.BinaryInsert(newRule, MatchMatchLevel);
	return true;
}

static int
SortByType(const SnifferRuleBase *rule1, const SnifferRuleBase *rule2)
{
	return strcasecmp(rule1->Type(), rule2->Type());
}

void 
Sniffer::DumpRules()
{
	// sort rules by mimetype before dumping
	BObjectList<SnifferRuleBase> sortedRules(rules.CountItems());
	int32 count = rules.CountItems();
	for (int32 index = 0; index < count; index++) 
		sortedRules.BinaryInsert(rules.ItemAt(index), SortByType);

	for (int32 index = 0; index < count; index++) {
		BString buffer;
		sortedRules.ItemAt(index)->Dump(buffer, false);
		printf("%s\n", buffer.String());
	}
}

const SnifferRuleBase *
Sniffer::SniffFile(const entry_ref *ref) const
{
	BEntry entry(ref);
	InitCheck( &entry );
	
	BFile file(&entry, O_RDONLY);
	InitCheck( &file );
	
	SniffBuffer buffer(&file);
	int32 count = rules.CountItems();
	for (int32 index = 0; index < count; index++) {
		SnifferRuleBase *rule = rules.ItemAt(index);
		if (rule->Match(&buffer))
			return rule;
	}
	return NULL;
}

const SnifferRuleBase *
Sniffer::SniffMemory(void *, size_t) const
{
	return NULL;
}

const SnifferRuleBase *
Sniffer::Sniff(SniffBuffer *sniffBuffer) const
{
	int32 count = rules.CountItems();
	for (int32 index = 0; index < count; index++) {
		SnifferRuleBase *rule = rules.ItemAt(index);
		if (rule->Match(sniffBuffer))
			return rule;
	}
	return NULL;
}

void 
Sniffer::DumpRule(const char *type, BString &buffer)
{
	const SnifferRuleBase *rule = rules.FindIf(MatchByTypePredicate(type));
	if (!rule)
		return;
	
	rule->Dump(buffer, false);
}

#endif	/* _SUPPORTS_FEATURE_BACKGROUND_MIME_SNIFFING */
