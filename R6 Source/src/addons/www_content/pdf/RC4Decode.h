#if !defined(__RC4DECODE_H_)
#include "Pusher.h"
#include "crypto.h"

namespace Pushers {

class RC4Decode : public Pusher {
RC4codec			fDecoder;

public:
					RC4Decode(Pusher *sink, const uint8 *key, size_t key_length = 10);
virtual				~RC4Decode();

virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false);

};

}; // namespace Pushers

using namespace Pushers;

#endif
