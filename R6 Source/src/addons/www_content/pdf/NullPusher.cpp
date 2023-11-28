#include "NullPusher.h"

NullPusher::NullPusher()
	: Pusher()
{
}


NullPusher::~NullPusher()
{
}

ssize_t 
NullPusher::Write(const uint8 *buffer, ssize_t length, bool finish)
{
	return length;
}

