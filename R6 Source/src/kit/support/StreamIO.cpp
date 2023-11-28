/*****************************************************************************

     File: StreamIO.cpp

	 Written By: Dianne Hackborn

     Copyright (c) 2000 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#include <StreamIO.h>

#include <Autolock.h>
#include <Locker.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <Debug.h>
#include <DataIO.h>
#include <String.h>

#include <priv_syscalls.h>

#include <ctype.h>
#include <string.h>

// If set, the serial port stream will lock between each line break,
// so that different thread's output is not intermingled.
//#define SYNCHRONIZE_LINES 1

/*-----------------------------------------------------------------*/
/*------- BStreamFormat Class -------------------------------------*/

// Not used yet.  I'd like this to be something that can be attached
// to a BDataIO to control how output is formatted.

class BStreamFormat {
public:
					BStreamFormat();
virtual				~BStreamFormat();

virtual	const char*	IndentForLevel(int32 level) const;

/*----- Private or reserved ---------------*/
private:

virtual	void		_ReservedStreamFormat1();
virtual	void		_ReservedStreamFormat2();
virtual	void		_ReservedStreamFormat3();
virtual	void		_ReservedStreamFormat4();
virtual	void		_ReservedStreamFormat5();
virtual	void		_ReservedStreamFormat6();
virtual	void		_ReservedStreamFormat7();
virtual	void		_ReservedStreamFormat8();
virtual	void		_ReservedStreamFormat9();
virtual	void		_ReservedStreamFormat10();
virtual	void		_ReservedStreamFormat11();
virtual	void		_ReservedStreamFormat12();

					BStreamFormat(const BStreamFormat &);
	BStreamFormat	&operator=(const BStreamFormat &);

		int32		_reserved[16];
};

BStreamFormat::BStreamFormat()
{
}

BStreamFormat::~BStreamFormat()
{
}

const char*	BStreamFormat::IndentForLevel(int32 level) const
{
	static const char space[] =
	"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
	int32 off = sizeof(space) - level - 1;
	if( off < 0 ) off = 0;
	return &space[off];
}

/* ---------------------------------------------------------------- */

BStreamFormat::BStreamFormat(const BStreamFormat &) {}
BStreamFormat &BStreamFormat::operator=(const BStreamFormat &) { return *this; }

/* ---------------------------------------------------------------- */

void 
BStreamFormat::_ReservedStreamFormat1()
{
}

void 
BStreamFormat::_ReservedStreamFormat2()
{
}

void 
BStreamFormat::_ReservedStreamFormat3()
{
}

void 
BStreamFormat::_ReservedStreamFormat4()
{
}

void 
BStreamFormat::_ReservedStreamFormat5()
{
}

void 
BStreamFormat::_ReservedStreamFormat6()
{
}

void 
BStreamFormat::_ReservedStreamFormat7()
{
}

void 
BStreamFormat::_ReservedStreamFormat8()
{
}

void 
BStreamFormat::_ReservedStreamFormat9()
{
}

void 
BStreamFormat::_ReservedStreamFormat10()
{
}

void 
BStreamFormat::_ReservedStreamFormat11()
{
}

void 
BStreamFormat::_ReservedStreamFormat12()
{
}

// ----------------------------------------------------------------- //
// ----------------------------------------------------------------- //
// ----------------------------------------------------------------- //

namespace BPrivate {
	class StdDataIO : public BDataIO {
	public:
		StdDataIO(FILE** fh)
			: fFile(fh)
		{
		}
		
		virtual ~StdDataIO()
		{
		}
		
		virtual ssize_t Read(void *buffer, size_t size);
		virtual ssize_t Write(const void *buffer, size_t size);
	
	private:
		FILE** fFile;
	};
	
	ssize_t StdDataIO::Read(void *buffer, size_t size)
	{
		ssize_t iosize = fread(buffer, size, 1, *fFile);
		status_t err = ferror(*fFile);
		if (err != B_OK) return err;
		return iosize;
	}
		
	ssize_t StdDataIO::Write(const void *buffer, size_t size)
	{
		ssize_t iosize = fwrite(buffer, size, 1, *fFile);
		status_t err = ferror(*fFile);
		if (err != B_OK) return err;
		return iosize;
	}
	
	class SerialDataIO : public BDataIO {
	public:
		SerialDataIO()
			: fLineSync("SerialDataIO Line Sync"),
			  fAccess("SerialDataIO Access"),
			  fPos(0)
		{
		}
		
		virtual ~SerialDataIO()
		{
			flush_buffer();
		}
		
		virtual ssize_t Read(void */*buffer*/, size_t /*size*/);
		virtual ssize_t Write(const void *buffer, size_t size);
	
	private:
		void append_buffer(const void* data, size_t size);
		void flush_buffer();
		
		BLocker fLineSync;
		BLocker fAccess;
		char fBuffer[1024];
		size_t fPos;
	};
	
	ssize_t SerialDataIO::Read(void */*buffer*/, size_t /*size*/)
	{
		return EPERM;
	}
	
	ssize_t SerialDataIO::Write(const void *buffer, size_t size)
	{
		if (!buffer || size == 0) return 0;
		
		#if SYNCHRONIZE_LINES
		if (fPos == 0 || fBuffer[fPos-1] == '\n') {
			fLineSync.Lock();
			while (fPos != 0 && fBuffer[fPos-1] != '\n') {
				fLineSync.Unlock();
				fLineSync.Lock();
			}
		}
		#endif
		
		BAutolock l(fAccess);
		
		const size_t origSize = size;
		
		char* c = ((char*)buffer) + size - 1;
		while (c >= buffer && *c != '\n') c--;
		if (c >= buffer) {
			size_t len = (size_t)(c-(char*)buffer) + 1;
			append_buffer(buffer, len);
			size -= len;
			buffer = c + 1;
			flush_buffer();
		}
		
		if (size > 0) append_buffer(buffer, size);
		
		#if SYNCHRONIZE_LINES
		if (fPos == 0 || fBuffer[fPos-1] == '\n') {
			fLineSync.Unlock();
		}
		#endif
		
		return origSize;
	}

	void SerialDataIO::append_buffer(const void* data, size_t size)
	{
		while (size > 0) {
			if (size+fPos < sizeof(fBuffer)-1) {
				memcpy(fBuffer+fPos, data, size);
				fPos += size;
				size = 0;
			} else {
				size_t avail = sizeof(fBuffer)-fPos-1;
				memcpy(fBuffer+fPos, data, avail);
				fPos += avail;
				data = ((char*)data) + avail;
				size -= avail;
			}
			
			if (fPos >= sizeof(fBuffer)-1) flush_buffer();
		}
	}
	
	void SerialDataIO::flush_buffer()
	{
		if (fPos == 0) return;
		if (fPos >= sizeof(fBuffer)) fPos = sizeof(fBuffer)-1;
		fBuffer[fPos] = 0;
		_kdprintf_("%s", fBuffer);
		fPos = 0;
	}
	
	StdDataIO StandardOutIO(&stdout);
	StdDataIO StandardInIO(&stdin);
	StdDataIO StandardErrIO(&stderr);
	SerialDataIO StandardSerialIO;
}

BDataIO& BOut(BPrivate::StandardOutIO);
BDataIO& BIn(BPrivate::StandardInIO);
BDataIO& BErr(BPrivate::StandardErrIO);
BDataIO& BSer(BPrivate::StandardSerialIO);

// ----------------------------------------------------------------- //
// ----------------------------------------------------------------- //
// ----------------------------------------------------------------- //

static void append_float(BDataIO& io, double value)
{
	char buffer[64];
	sprintf(buffer, "%g", value);
	if( !strchr(buffer, '.') && !strchr(buffer, 'e') &&
		!strchr(buffer, 'E') ) {
		strncat(buffer, ".0", sizeof(buffer)-1);
	}
	if (&io) io.Write(buffer, strlen(buffer));
}

/* ---------------------------------------------------------------- */

BDataIO& endl(BDataIO& io)
{
	if (&io) io.Write("\n", 1);
	return io;
}

BDataIO& ends(BDataIO& io)
{
	if (&io) io.Write("", 1);
	return io;
}

BDataIO& operator<<(BDataIO& io, const char* str)
{
	if (&io)  {
		if (str) io.Write(str, strlen(str));
		else io.Write("(nil)", 5);
	}
	return io;
}

BDataIO& operator<<(BDataIO& io, char c)
{
	if (&io) io.Write(&c, sizeof(c));
	return io;
}

BDataIO& operator<<(BDataIO& io, bool b)
{
	if (&io) {
		if (b) io.Write("true", 4);
		else io.Write("false", 5);
	}
	return io;
}

BDataIO& operator<<(BDataIO& io, int num)
{
	char buffer[64];
	sprintf(buffer, "%d", num);
	if (&io) io.Write(buffer, strlen(buffer));
	return io;
}

BDataIO& operator<<(BDataIO& io, unsigned int num)
{
	char buffer[64];
	sprintf(buffer, "%u", num);
	if (&io) io.Write(buffer, strlen(buffer));
	return io;
}

BDataIO& operator<<(BDataIO& io, int32 num)
{
	char buffer[64];
	sprintf(buffer, "%ld", num);
	if (&io) io.Write(buffer, strlen(buffer));
	return io;
}

BDataIO& operator<<(BDataIO& io, uint32 num)
{
	char buffer[64];
	sprintf(buffer, "%lu", num);
	if (&io) io.Write(buffer, strlen(buffer));
	return io;
}

BDataIO& operator<<(BDataIO& io, int64 num)
{
	char buffer[64];
	sprintf(buffer, "%Ld", num);
	if (&io) io.Write(buffer, strlen(buffer));
	return io;
}

BDataIO& operator<<(BDataIO& io, uint64 num)
{
	char buffer[64];
	sprintf(buffer, "%Lu", num);
	if (&io) io.Write(buffer, strlen(buffer));
	return io;
}

BDataIO& operator<<(BDataIO& io, float num)
{
	append_float(io, num);
	return io;
}

BDataIO& operator<<(BDataIO& io, double num)
{
	append_float(io, num);
	return io;
}

BDataIO& operator<<(BDataIO& io, const void* ptr)
{
	char buffer[64];
	sprintf(buffer, "%p", ptr);
	if (&io) io.Write(buffer, strlen(buffer));
	return io;
}

/* ---------------------------------------------------------------- */

BHexDump::BHexDump(const void *buf, size_t length,
					size_t bytesPerLine, const char* prefix)
	: fBuffer(buf), fLength(length),
	  fBytesPerLine(bytesPerLine), fSingleLineCutoff(bytesPerLine),
	  fAlignment(4), fPrefix(prefix)
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

BHexDump& BHexDump::SetPrefix(const char* prefix)
{
	fPrefix = prefix;
	return *this;
}

const void* BHexDump::Buffer() const		{ return fBuffer; }
size_t BHexDump::Length() const				{ return fLength; }
size_t BHexDump::BytesPerLine() const		{ return fBytesPerLine; }
int32 BHexDump::SingleLineCutoff() const	{ return fSingleLineCutoff; }
size_t BHexDump::Alignment() const			{ return fAlignment; }
const char* BHexDump::Prefix() const		{ return fPrefix; }

static const char* hexdigits = "0123456789abcdef";

BDataIO& operator<<(BDataIO& io, const BHexDump& data)
{
	size_t offset;
	
	size_t length = data.Length();
	unsigned char *pos = (unsigned char *)data.Buffer();
	size_t bytesPerLine = data.BytesPerLine();
	const size_t alignment = data.Alignment();
	
	if (pos == NULL) {
		if (data.SingleLineCutoff() < 0) io << endl << data.Prefix();
		io << data.Prefix() << "(NULL)";
		return io;
	}
	
	if (length == 0) {
		if (data.SingleLineCutoff() < 0) io << endl << data.Prefix();
		io << data.Prefix() << "(empty)";
		return io;
	}
	
	if ((int32)length < 0) {
		if (data.SingleLineCutoff() < 0) io << endl << data.Prefix();
		io << data.Prefix() << "(bad length: " << (int32)length << ")";
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
		
		if (!oneLine) io << data.Prefix();
		io.Write(buffer, (size_t)(c-buffer));
		
		if (length <= bytesPerLine) break;
		length -= bytesPerLine;
	}
	
	return io;
}

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
	
BDataIO& operator<<(BDataIO& io, const BTypeCode& type)
{
	char buffer[32];
	const int32 len = TypeToString(type.TypeCode(), buffer) - buffer;
	io.Write(buffer, len);
	return io;
}

// ----------------------------------------------------------------- //
// ----------------------------------------------------------------- //
// ----------------------------------------------------------------- //
