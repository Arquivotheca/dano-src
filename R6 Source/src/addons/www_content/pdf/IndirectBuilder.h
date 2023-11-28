#if !defined(__INDIRECTBUILDER_H_)
#define __INDIRECTBUILDER_H_

#include "ArrayBuilder.h"

class RC4codec;

namespace BPrivate {

class IndirectBuilder : public ArrayBuilder {

PDFDocument				*fDoc;
RC4codec				*fDecoder;
bool					fDone;

						IndirectBuilder();
protected:
virtual status_t		DispatchKeyword(PDFObject *o);

public:
						IndirectBuilder(PDFDocument *doc);
virtual					~IndirectBuilder();

virtual	ssize_t			Write(const uint8 *buffer, ssize_t length, bool finish = false);
virtual ssize_t			Write(BPrivate::PDFObject *obj);

};

}; // namespace BPrivate

using namespace BPrivate;
#endif
