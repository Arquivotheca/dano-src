#if !defined(__NULLPUSHER_H_)
#define __NULLPUSHER_H_

#include "Pusher.h"

namespace Pushers {

class NullPusher : public Pusher {

public:
					NullPusher();
virtual				~NullPusher();

virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false);

};

};

using namespace Pushers;

#endif
