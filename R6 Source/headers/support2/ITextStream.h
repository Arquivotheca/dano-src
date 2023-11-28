/***************************************************************************
//
//	File:			support2/ITextStream.h
//
//	Description:	Abstract interface for a formatted text output
//					stream, with C++ iostream-like operators.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef	_SUPPORT2_TEXTSTREAM_INTERFACE_H
#define	_SUPPORT2_TEXTSTREAM_INTERFACE_H

#include <support2/IInterface.h>

namespace B {
namespace Support2 {

/*-----------------------------------------------------------------*/

class ITextOutput : public IInterface
{
	public:

		// Note: binder remote interface not yet implemented.
		B_STANDARD_ATOM_TYPEDEFS(ITextOutput)

		virtual	void					Print(const char *debugText, int32 len = -1) = 0;
		virtual void					BumpIndentLevel(int32 delta) = 0;
};

/*-----------------------------------------------------------------*/

// Generic manipulator function for the stream.
typedef ITextOutput::arg (*ITextOutputManipFunc)(ITextOutput::arg);

ITextOutput::arg endl(ITextOutput::arg io);
ITextOutput::arg indent(ITextOutput::arg io);
ITextOutput::arg dedent(ITextOutput::arg io);

ITextOutput::arg operator<<(ITextOutput::arg io, const char* str);
ITextOutput::arg operator<<(ITextOutput::arg io, char);		// writes raw character

ITextOutput::arg operator<<(ITextOutput::arg io, bool);
ITextOutput::arg operator<<(ITextOutput::arg io, int);
ITextOutput::arg operator<<(ITextOutput::arg io, unsigned int);
ITextOutput::arg operator<<(ITextOutput::arg io, uint32);
ITextOutput::arg operator<<(ITextOutput::arg io, int32);
ITextOutput::arg operator<<(ITextOutput::arg io, uint64);
ITextOutput::arg operator<<(ITextOutput::arg io, int64);
ITextOutput::arg operator<<(ITextOutput::arg io, float);
ITextOutput::arg operator<<(ITextOutput::arg io, double);

ITextOutput::arg operator<<(ITextOutput::arg io, ITextOutputManipFunc func);

ITextOutput::arg operator<<(ITextOutput::arg io, const void*);

template <class TYPE>
inline ITextOutput::arg operator<<(ITextOutput::arg io, const atom_ptr<TYPE>& a)
{
	return io << a.ptr();
}

template <class TYPE>
inline ITextOutput::arg operator<<(ITextOutput::arg io, const atom_ref<TYPE>& a)
{
	// cough, cough.
	return io << (*(void**)&a);
}

/*-----------------------------------------------------------------*/

inline ITextOutput::arg operator<<(ITextOutput::arg io, ITextOutputManipFunc func)
{
	return (*func)(io);
}

/*-----------------------------------------------------------------*/
/*------- Formatting type_code values -----------------------------*/

class BTypeCode {
public:
	BTypeCode(type_code type);
	~BTypeCode();

	type_code TypeCode() const;
	
private:
	type_code fType;
	int32 _reserved;
};

ITextOutput::arg operator<<(ITextOutput::arg io, const BTypeCode& type);

/*-----------------------------------------------------------------*/
/*------- Dumping raw bytes as hex --------------------------------*/

class BHexDump {
public:
	BHexDump(const void *buf, size_t length, size_t bytesPerLine=16);
	~BHexDump();
	
	BHexDump& SetBytesPerLine(size_t bytesPerLine);
	BHexDump& SetSingleLineCutoff(int32 bytes);
	BHexDump& SetAlignment(size_t alignment);
	
	const void* Buffer() const;
	size_t Length() const;
	size_t BytesPerLine() const;
	int32 SingleLineCutoff() const;
	size_t Alignment() const;

private:
	const void* fBuffer;
	size_t fLength;
	size_t fBytesPerLine;
	int32 fSingleLineCutoff;
	size_t fAlignment;
	
	uint32 _reserved[8];
};

ITextOutput::arg operator<<(ITextOutput::arg io, const BHexDump& buffer);

/*-----------------------------------------------------------------*/

} } // namespace B::Support2

#endif /* _SUPPORT2_TEXTSTREAM_INTERFACE_H */
