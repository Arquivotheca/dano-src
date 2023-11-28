
#include <textencoding/BTextEncoding.h>
#include <add-ons/textencoding/BTextCodec.h>
#include "BTextEncodingAddOn.h"

#include <stdio.h>

using namespace B::TextEncoding;

// #pragma mark -

BTextEncoding::BTextEncoding(const char *encodingName) :
	fName(NULL),
	fCodec(NULL)
{
	SetTo(encodingName);
}


BTextEncoding::BTextEncoding(uint32 encodingType) :
	fName(NULL),
	fCodec(NULL)
{
	SetTo(encodingType);
}



BTextEncoding::BTextEncoding(BTextCodec *codec) :
	fName(NULL),
	fCodec(codec)
{
}

BTextEncoding::~BTextEncoding()
{
	if (fCodec)
		delete fCodec;
}

status_t 
BTextEncoding::SetTo(const char *encodingName)
{
	if (encodingName == fName) {
		return B_OK;
	}
	if (fCodec) {
		delete fCodec;
		fCodec = NULL;
	}
	
	fCodec = BTextEncodingManager::Default().MakeCodec(encodingName);
	fName = encodingName;
	
	return B_OK;
}

status_t 
BTextEncoding::SetTo(uint32 encodingType)
{
	return SetTo(name_for_encoding_enum(encodingType));
}

bool 
BTextEncoding::HasCodec() const
{
	return (fCodec != NULL);
}

const char *
BTextEncoding::Name() const
{
	return fName.String();
}

void 
BTextEncoding::SetName(const char *name)
{
	fName = name;
}

status_t 
BTextEncoding::ConvertToUnicode(const char *src, int32 *srcLen, char *dst, int32 *dstLen, conversion_info &info) const
{
	if (fCodec)
		return fCodec->DeccodeString(src, srcLen, dst, dstLen, info);
	else
		return B_NO_INIT;
}

status_t 
BTextEncoding::ConvertFromUnicode(const char *src, int32 *srcLen, char *dst, int32 *dstLen, conversion_info &info) const
{
	if (fCodec)
		return fCodec->EncodeString(src, srcLen, dst, dstLen, info);
	else
		return B_NO_INIT;
}

uint16 
BTextEncoding::UnicodeFor(uint16 charcode) const
{
	uint16 unicode = 0xffff;
	if (fCodec)
		unicode = fCodec->Decode(charcode);
	
	return unicode; 
}

uint16 
BTextEncoding::CharcodeFor(uint16 unicode) const
{
	uint16 charcode = 0xffff;
	if (fCodec)
		charcode = fCodec->Encode(unicode);
	return charcode;
}

