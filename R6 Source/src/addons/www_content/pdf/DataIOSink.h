#if !defined(__DATAIOSINK_H_)
#define __DATAIOSINK_H_

#include "Pusher.h"
#include "DataIO.h"

class DataIOSink : public Pusher {
	BDataIO	*fIO;

public:
					DataIOSink(BDataIO *sink);
virtual				~DataIOSink();
virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false);

BDataIO				*GetDataIO(void) { return fIO; };
};

#endif
