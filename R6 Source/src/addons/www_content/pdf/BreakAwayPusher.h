#if !defined(__BREAKAWAYPUSHER_H_)
#define __BREAKAWAYPUSHER_H_

#include "Pusher.h"

class BreakAwayPusher : public Pusher {
public:
					BreakAwayPusher(Pusher *sink);
virtual				~BreakAwayPusher();
};

#endif
