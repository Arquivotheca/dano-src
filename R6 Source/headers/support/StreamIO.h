/******************************************************************************
/
/	File:			StreamIO.h
/
/	Description:	Output stream operators for BDataIO.
/
/	Copyright 2000, Be Incorporated
/
******************************************************************************/

#ifndef	_STREAM_IO_H
#define	_STREAM_IO_H

#include <BeBuild.h>
#include <SupportDefs.h>

class BDataIO;

/*-----------------------------------------------------------------*/
/*------- Standard Streams ----------------------------------------*/

extern BDataIO& BOut;
extern BDataIO& BIn;
extern BDataIO& BErr;
extern BDataIO& BSer;

/*-----------------------------------------------------------------*/
/*------- Stream Operators ----------------------------------------*/

typedef BDataIO& (*BDataIOManipFunc)(BDataIO&);

BDataIO& endl(BDataIO& io);
BDataIO& ends(BDataIO& io);

BDataIO& operator<<(BDataIO& io, const char* str);
BDataIO& operator<<(BDataIO& io, char);		// writes raw character

BDataIO& operator<<(BDataIO& io, bool);
BDataIO& operator<<(BDataIO& io, int);
BDataIO& operator<<(BDataIO& io, unsigned int);
BDataIO& operator<<(BDataIO& io, uint32);
BDataIO& operator<<(BDataIO& io, int32);
BDataIO& operator<<(BDataIO& io, uint64);
BDataIO& operator<<(BDataIO& io, int64);
BDataIO& operator<<(BDataIO& io, float);
BDataIO& operator<<(BDataIO& io, double);
BDataIO& operator<<(BDataIO& io, const void*);

inline BDataIO& operator<<(BDataIO& io, BDataIOManipFunc func) { return (*func)(io); }

class BHexDump {
public:
	BHexDump(const void *buf, size_t length,
			 size_t bytesPerLine=16, const char* prefix="");
	~BHexDump();
	
	BHexDump& SetBytesPerLine(size_t bytesPerLine);
	BHexDump& SetSingleLineCutoff(int32 bytes);
	BHexDump& SetAlignment(size_t alignment);
	BHexDump& SetPrefix(const char* prefix);
	
	const void* Buffer() const;
	size_t Length() const;
	size_t BytesPerLine() const;
	int32 SingleLineCutoff() const;
	size_t Alignment() const;
	const char* Prefix() const;

private:
	const void* fBuffer;
	size_t fLength;
	size_t fBytesPerLine;
	int32 fSingleLineCutoff;
	size_t fAlignment;
	const char* fPrefix;
	
	uint32 _reserved[7];
};

BDataIO& operator<<(BDataIO& io, const BHexDump& buffer);

class BTypeCode {
public:
	BTypeCode(type_code type);
	~BTypeCode();

	type_code TypeCode() const;
	
private:
	type_code fType;
	int32 _reserved;
};

BDataIO& operator<<(BDataIO& io, const BTypeCode& type);

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _DATA_IO_H */
