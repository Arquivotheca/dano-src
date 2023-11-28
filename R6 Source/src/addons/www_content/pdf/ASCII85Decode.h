#if !defined(__ASCII85DECODE_H__)
#define __ASCII85DECODE_H__

#include "Pusher.h"

namespace Pushers {

class ASCII85Decode : public Pusher {

uint32				char_buffer;
int					bytes_to_consume;
int					buffered_chars;
bool				end_of_stream;

public:
					ASCII85Decode(Pusher *sink);
virtual				~ASCII85Decode();

virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false);

};

}; // namespace Pushers

using namespace Pushers;
#endif
