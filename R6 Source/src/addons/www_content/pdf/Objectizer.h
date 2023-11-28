#if !defined(__OBJECTIZER_H_)
#define __OBJECTIZER_H_

#include "ArrayBuilder.h"

namespace BPrivate {

class Objectizer : public ArrayBuilder {

PDFDocument			*fDoc;
BPositionIO			*fSrc;
uint8				fBuffer[64];
int					bytesInBuffer;

protected:
virtual status_t	DispatchKeyword(PDFObject *o);
virtual status_t	UnknownKeyword(PDFObject *o);

public:
					Objectizer(PDFDocument *doc);
virtual				~Objectizer();
virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false);
virtual ssize_t		Write(BPrivate::PDFObject *obj);

status_t			Read(uint8 *buffer, size_t size);
off_t				Seek(off_t position, uint32 seek_mode);
off_t				Position();

PDFObject			*GetObject(void);

};

};

using namespace BPrivate;

#endif
