#if !defined(__GREYTORGB_H_)
#define __GREYTORGB_H_

#include "Pusher.h"

namespace Pushers {

class GreyToRGB : public Pusher {

uint32				fRowBytes;
uint32				fComponent;
uint8				*fRowBuffer;

public:
					GreyToRGB(Pusher *sink, uint32 samplesPerLine);
virtual				~GreyToRGB();

virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false);

};

};

using namespace Pushers;
#endif
