/*****************************************************************************

     File: TextStream.cpp

	 Written By: Dianne Hackborn

     Copyright (c) 2000 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#include <support2/TextStream.h>
#include <support2/Autolock.h>
#include <support2/Locker.h>
#include <support2/Debug.h>
#include <support2/String.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <os_p/priv_syscalls.h>

#include <string.h>

// If set, the serial port stream will lock between each line break,
// so that different thread's output is not intermingled.
//#define SYNCHRONIZE_LINES 1

namespace B {
namespace Support2 {

/* ---------------------------------------------------------------- */

BTextOutput::BTextOutput(IByteOutput::arg stream)
	:	m_stream(stream.ptr()), m_currentIndent(0), m_front(1)
{
	m_stream->Acquire(this);
}

BTextOutput::BTextOutput(IByteOutput *This)
	:	m_stream(This), m_currentIndent(0), m_front(1)
{
}

void BTextOutput::Print(const char *debugText, int32 len)
{
	const char *start,*end;
	start = end = debugText;
	// Loop while either the next current is not '0' -or- we have
	// been given an exact number of bytes to write.
	while (len && *start) {
		// Look for end of text or next newline.
		while (len && *end && (*end != '\n')) { len--; end++; };
		
		// If we are going to write the start of a line, first
		// insert an indent.
		if (m_front && atomic_and(&m_front, 0))
			WriteIndent();
		
		// Skip ahead to include all newlines in this section.
		while (len && *end == '\n') {
			len--;
			end++;
			atomic_or(&m_front, 1);
		}
			
		// Write this line of text and get ready to process the next.
		m_stream->Write(start,end-start);
		start = end;
	}
}

void BTextOutput::WriteIndent()
{
	static const char space[] =
	"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
	int32 off = sizeof(space) - m_currentIndent - 1;
	if( off < 0 ) off = 0;
	m_stream->Write(space+off,m_currentIndent);
}

void BTextOutput::BumpIndentLevel(int32 delta)
{
	m_currentIndent += delta;
	if (m_currentIndent < 0) m_currentIndent = 0;
}

BTextOutput::~BTextOutput()
{
	m_stream->AttemptRelease(this);
}

/* ---------------------------------------------------------------- */

static inline int isident(int c)
{
	return isalnum(c) || c == '_';
}

static inline bool isasciitype(char c)
{
	if( c >= ' ' && c < 127 && c != '\'' && c != '\\' ) return true;
	return false;
}

static inline char makehexdigit(uint32 val)
{
	return "0123456789ABCDEF"[val&0xF];
}

static char* appendhexnum(uint32 val, char* out)
{
	for( int32 i=7; i>=0; i-- ) {
		*out++ = makehexdigit( val );
		val >>= 4;
	}
	*out = 0;
	return out;
}

static char* TypeToString(type_code type, char* out,
						  bool fullContext = true,
						  bool strict = false)
{
	char* pos = out;
	const char c1 = (char)((type>>24)&0xFF);
	const char c2 = (char)((type>>16)&0xFF);
	const char c3 = (char)((type>>8)&0xFF);
	const char c4 = (char)(type&0xFF);
	bool valid;
	if( !strict ) {
		valid = isasciitype(c1) && isasciitype(c2) && isasciitype(c3) && isasciitype(c4);
	} else {
		valid = isident(c1) && isident(c2) && isident(c3) && isident(c4);
	}
	if( valid && (!fullContext || c1 != '0' || c2 != 'x') ) {
		if( fullContext ) *pos++ = '\'';
		*pos++ = c1;
		*pos++ = c2;
		*pos++ = c3;
		*pos++ = c4;
		if( fullContext ) *pos++ = '\'';
		*pos = 0;
		return pos;
	}
	
	if( fullContext ) {
		*pos++ = '0';
		*pos++ = 'x';
	}
	return appendhexnum(type, pos);
}
	
/* ---------------------------------------------------------------- */

static void append_float(ITextOutput::arg io, double value)
{
	char buffer[64];
	sprintf(buffer, "%g", value);
	if( !strchr(buffer, '.') && !strchr(buffer, 'e') &&
		!strchr(buffer, 'E') ) {
		strncat(buffer, ".0", sizeof(buffer)-1);
	}
	if (&io) io->Print(buffer, strlen(buffer));
}

/* ---------------------------------------------------------------- */

ITextOutput::arg endl(ITextOutput::arg io)
{
	if (io != NULL) io->Print("\n");
	return io;
}

ITextOutput::arg indent(ITextOutput::arg io)
{
	if (io != NULL) io->BumpIndentLevel(1);
	return io;
}

ITextOutput::arg dedent(ITextOutput::arg io)
{
	if (io != NULL) io->BumpIndentLevel(-1);
	return io;
}

ITextOutput::arg operator<<(ITextOutput::arg io, const char* str)
{
	if (io != NULL)  {
		if (str) io->Print(str, strlen(str));
		else io->Print("(nil)", 5);
	}
	return io;
}

ITextOutput::arg operator<<(ITextOutput::arg io, char c)
{
	if (io != NULL) io->Print(&c, sizeof(c));
	return io;
}

ITextOutput::arg operator<<(ITextOutput::arg io, bool b)
{
	if (io != NULL) {
		if (b) io->Print("true", 4);
		else io->Print("false", 5);
	}
	return io;
}

ITextOutput::arg operator<<(ITextOutput::arg io, int num)
{
	char buffer[64];
	sprintf(buffer, "%d", num);
	if (io != NULL) io->Print(buffer, strlen(buffer));
	return io;
}

ITextOutput::arg operator<<(ITextOutput::arg io, unsigned int num)
{
	char buffer[64];
	sprintf(buffer, "%u", num);
	if (io != NULL) io->Print(buffer, strlen(buffer));
	return io;
}

ITextOutput::arg operator<<(ITextOutput::arg io, int32 num)
{
	char buffer[64];
	sprintf(buffer, "%ld", num);
	if (io != NULL) io->Print(buffer, strlen(buffer));
	return io;
}

ITextOutput::arg operator<<(ITextOutput::arg io, uint32 num)
{
	char buffer[64];
	sprintf(buffer, "%lu", num);
	if (io != NULL) io->Print(buffer, strlen(buffer));
	return io;
}

ITextOutput::arg operator<<(ITextOutput::arg io, int64 num)
{
	char buffer[64];
	sprintf(buffer, "%Ld", num);
	if (io != NULL) io->Print(buffer, strlen(buffer));
	return io;
}

ITextOutput::arg operator<<(ITextOutput::arg io, uint64 num)
{
	char buffer[64];
	sprintf(buffer, "%Lu", num);
	if (io != NULL) io->Print(buffer, strlen(buffer));
	return io;
}

ITextOutput::arg operator<<(ITextOutput::arg io, float num)
{
	append_float(io, num);
	return io;
}

ITextOutput::arg operator<<(ITextOutput::arg io, double num)
{
	append_float(io, num);
	return io;
}

ITextOutput::arg operator<<(ITextOutput::arg io, const void* ptr)
{
	char buffer[64];
	sprintf(buffer, "%p", ptr);
	if (io != NULL) io->Print(buffer, strlen(buffer));
	return io;
}

/* ---------------------------------------------------------------- */

BTypeCode::BTypeCode(type_code type)
	:	fType(type)
{
}

BTypeCode::~BTypeCode()
{
}

type_code BTypeCode::TypeCode() const
{
	return fType;
}

ITextOutput::arg operator<<(ITextOutput::arg io, const BTypeCode& type)
{
	char buffer[32];
	const int32 len = TypeToString(type.TypeCode(), buffer) - buffer;
	buffer[len] = 0;
	io->Print(buffer);
	return io;
}

/* ---------------------------------------------------------------- */

BHexDump::BHexDump(const void *buf, size_t length, size_t bytesPerLine)
	: fBuffer(buf), fLength(length),
	  fBytesPerLine(bytesPerLine), fSingleLineCutoff(bytesPerLine),
	  fAlignment(4)
{
	if (bytesPerLine >= 16) fAlignment = 4;
	else if (bytesPerLine >= 8) fAlignment = 2;
	else fAlignment = 1;
}

BHexDump::~BHexDump()
{
}

BHexDump& BHexDump::SetBytesPerLine(size_t bytesPerLine)
{
	fBytesPerLine = bytesPerLine;
	return *this;
}

BHexDump& BHexDump::SetSingleLineCutoff(int32 bytes)
{
	fSingleLineCutoff = bytes;
	return *this;
}

BHexDump& BHexDump::SetAlignment(size_t alignment)
{
	fAlignment = alignment;
	return *this;
}

const void* BHexDump::Buffer() const		{ return fBuffer; }
size_t BHexDump::Length() const				{ return fLength; }
size_t BHexDump::BytesPerLine() const		{ return fBytesPerLine; }
int32 BHexDump::SingleLineCutoff() const	{ return fSingleLineCutoff; }
size_t BHexDump::Alignment() const			{ return fAlignment; }

static const char* hexdigits = "0123456789abcdef";

ITextOutput::arg operator<<(ITextOutput::arg io, const BHexDump& data)
{
	size_t offset;
	
	size_t length = data.Length();
	unsigned char *pos = (unsigned char *)data.Buffer();
	size_t bytesPerLine = data.BytesPerLine();
	const size_t alignment = data.Alignment();
	
	if (pos == NULL) {
		if (data.SingleLineCutoff() < 0) io << endl;
		io << "(NULL)";
		return io;
	}
	
	if (length == 0) {
		if (data.SingleLineCutoff() < 0) io << endl;
		io << "(empty)";
		return io;
	}
	
	if ((int32)length < 0) {
		if (data.SingleLineCutoff() < 0) io << endl;
		io << "(bad length: " << (int32)length << ")";
		return io;
	}
	
	char buffer[256];
	static const size_t maxBytesPerLine = (sizeof(buffer)-1-11-4)/(3+1);
	
	if (bytesPerLine > maxBytesPerLine) bytesPerLine = maxBytesPerLine;
	
	const bool oneLine = (int32)length <= data.SingleLineCutoff();
	if (!oneLine) io << endl;
	
	for (offset = 0; ; offset += bytesPerLine, pos += bytesPerLine) {
		long remain = length;
		size_t index;

		char* c = buffer;
		if (!oneLine) {
			sprintf(c, "0x%06x: ", (int)offset);
			c += 10;
		}

		size_t inner = 0;
		
		for (index = 0; index < bytesPerLine; index++, inner++) {

			if (inner == alignment && alignment > 0) {
				*c++ = ' ';
				inner = 0;
			}
			
			if (remain-- > 0) {
				const unsigned char val = pos[index];
				*c++ = hexdigits[val>>4];
				*c++ = hexdigits[val&0xf];
			} else if (!oneLine) {
				*c++ = ' ';
				*c++ = ' ';
			}
		}

		remain = length;
		*c++ = ' ';
		*c++ = '\'';
		for (index = 0; index < bytesPerLine; index++) {

			if (remain-- > 0) {
				const unsigned char val = pos[index];
				*c++ = (val >= ' ' && val < 127) ? val : '.';
			} else if (!oneLine) {
				*c++ = ' ';
			}
		}
		
		*c++ = '\'';
		if (length > bytesPerLine) *c++ = '\n';
		
//		if (!oneLine) io << indent;
		io->Print(buffer, (size_t)(c-buffer));
		
		*c++ = 0;
		
		if (length <= bytesPerLine) break;
		length -= bytesPerLine;
	}
	
	return io;
}

} }	// namespace B::Support2

// ----------------------------------------------------------------- //
// ----------------------------------------------------------------- //
// ----------------------------------------------------------------- //
