#if !defined(__ARRAYBUILDER_H_)
#define __ARRAYBUILDER_H_

#include "ObjectSink.h"
#include "Object2.h"

namespace BPrivate {

class ArrayBuilder : public ObjectSink {

typedef vector<int32>	index_array;
index_array				fIndicies;

protected:
object_array			fStack;
virtual status_t		DispatchKeyword(PDFObject *o);
virtual status_t		UnknownKeyword(PDFObject *o);
vector<int32>::size_type
						NestDepth() { return fIndicies.size(); };
void					FlushStacks(void);

public:
					ArrayBuilder();
virtual				~ArrayBuilder();

virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false) = 0;
virtual ssize_t		Write(BPrivate::PDFObject *obj) = 0;
};

}; // namespace BPrivate

using namespace BPrivate;

#endif
