#if !defined(__PUSHER_H__)
#define __PUSHER_H__

#include <SupportDefs.h>

class Pusher {
	Pusher	*fSink;

					Pusher(const Pusher &p);
protected:
					Pusher(void);
					Pusher(Pusher *p);
public:

virtual				~Pusher();

virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false);

// Set the Pusher's sink to the new_sink.  Caller takes ownership
// of the returned previous sink.
Pusher				*SetSink(Pusher *new_sink) { Pusher *p = fSink; fSink = new_sink; return p; };

// Get the Pusher's sink.
Pusher				*GetSink(void) { return fSink; };

enum {
	OK = 0,
	ERROR = -1,
	SINK_FULL = -2
	};

};

class PusherBuffer : public Pusher {

uint8				*fBuffer;
size_t				fMaxSize;
size_t				fBufSize;
size_t				fNextByte;

public:
					PusherBuffer(Pusher *sink, ssize_t bufferSize = 1024);
virtual				~PusherBuffer();

virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false);

};

#endif
