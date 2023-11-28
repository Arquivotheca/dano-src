#if !defined(__FLATEDECODE_H__)
#define __FLATEDECODE_H__

#include "Pusher.h"

namespace Pushers {

class FlateDecode : public Pusher {

typedef struct private_data;
private_data		*m_pd;

public:
					FlateDecode(Pusher *sink);
					~FlateDecode();
virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false);

};

}; // namespace Pushers

using namespace Pushers;

#endif
