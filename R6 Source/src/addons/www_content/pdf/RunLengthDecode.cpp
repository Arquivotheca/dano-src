#include "RunLengthDecode.h"

RunLengthDecode::RunLengthDecode(Pusher *sink)
	: Pusher(new PusherBuffer(sink)), rle_code(128), rle_value(0), end_of_stream(false)
{
}


RunLengthDecode::~RunLengthDecode()
{
}

ssize_t 
RunLengthDecode::Write(const uint8 *buffer, ssize_t length, bool finish)
{
	return Pusher::SINK_FULL;
}

