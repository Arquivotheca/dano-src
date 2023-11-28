#if !defined(__ASCIIHEXDECODE_H__)
#define __ASCIIHEXDECODE_H__

#include "Pusher.h"

namespace Pushers {

class ASCIIHexDecode : public Pusher {

uint8				hex;
uint8				nibbles;
bool				end_of_stream;

public:
					ASCIIHexDecode(Pusher *sink);
virtual				~ASCIIHexDecode();

virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false);

};

}; // namespace Pushers
using namespace Pushers;

#endif
