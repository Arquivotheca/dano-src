#if !defined(__CMYKTORGB_H_)
#define __CMYKTORGB_H_

#include "Pusher.h"

namespace Pushers {

class CMYKtoRGB : public Pusher {

uint32				fRowBytes;
uint32				fInComponent;
uint8				*fRowBuffer;
uint32				fOutComponent;
uint16				fSample[4];

enum				{ CYAN, MAGENTA, YELLOW, BLACK };
public:
					CMYKtoRGB(Pusher *sink, uint32 samplesPerLine);
virtual				~CMYKtoRGB();

virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false);

};

};

using namespace Pushers;
#endif
