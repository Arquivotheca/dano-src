#if !defined(__OBJECTSINK_H_)
#define __OBJECTSINK_H_

#include "Pusher.h"

namespace BPrivate {

class PDFObject;

class ObjectSink : public Pusher {

protected:

					ObjectSink();
virtual				~ObjectSink();

public:

virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false) = 0;
virtual ssize_t		Write(BPrivate::PDFObject *obj) = 0;

	enum {
		OK = 0,			// continue passing objects
		WANT_RAW_DATA,	// switch to raw data writes
		WANT_OBJECTS = -10	// switch to object writes
	};
};

}; // namespace BPrivate

using namespace BPrivate;
#endif
