#if !defined(__SAMPLEEXPANDER_H_)
#define __SAMPLEEXPANDER_H_

#include "Pusher.h"

namespace Pushers {

class SampleExpander : public Pusher {

uint32				fRowBytes;
uint32				fBitsPerComponent;
uint32				fBitBuffer;
uint32				fBitsInBuffer;
uint32				fComponent;
uint8				*fRowBuffer;
uint8				fScale;

public:
					SampleExpander(Pusher *sink, uint32 componentsPerLine, uint32 bitsPerComponent, bool scale);
virtual				~SampleExpander();

virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false);

};

};

using namespace Pushers;
#endif
