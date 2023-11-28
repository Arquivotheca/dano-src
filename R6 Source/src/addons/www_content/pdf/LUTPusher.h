#if !defined(__LUTPUSHER_H_)
#define __LUTPUSHER_H_

#include "Pusher.h"

namespace Pushers {

class LUTPusher : public Pusher {

uint32				fRowBytes;
uint32				fComponentsPerSample;
uint8				*fLUT;
uint32				fComponent;
uint8				*fRowBuffer;
uint32				fOneToMany;

public:
					LUTPusher(Pusher *sink, bool oneToMany, uint32 samplesPerLine, uint32 componentsPerSample, uint8 *lut);
virtual				~LUTPusher();

virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false);

};

};

using namespace Pushers;
#endif
