#if !defined(__RUNLENGTHDECODE_H__)
#define __RUNLENGTHDECODE_H__

#include "Pusher.h"

namespace Pushers {

class RunLengthDecode : public Pusher {

uint16				rle_code;
uint8				rle_value;
bool				end_of_stream;

public:
					RunLengthDecode(Pusher *sink);
					~RunLengthDecode();
virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false);

};

}; // namespace Pushers

using namespace Pushers;

#endif
