#include "RC4Decode.h"
#include "Debug.h"

RC4Decode::RC4Decode(Pusher *sink, const uint8 *key, size_t key_length)
	: Pusher(new PusherBuffer(sink)), fDecoder(key, key_length)
{
}


RC4Decode::~RC4Decode()
{
}

ssize_t 
RC4Decode::Write(const uint8 *buffer, ssize_t length, bool finish)
{
	uint8 *crypt_buffer = 0;
	// dup the buffer (RC4codec does in-place conversion)
	if (length)
	{
		crypt_buffer = new uint8[length];
		if (!crypt_buffer) return B_NO_MEMORY;
		memcpy(crypt_buffer, buffer, length);
		fDecoder.Encode(crypt_buffer, length);
	}
	// we depend on PusherBuffer always taking all of our data
	status_t result = Pusher::Write(crypt_buffer, length, finish);
	ASSERT((result < 0) || (result == length));
	delete crypt_buffer;
	if (finish) fDecoder.Reset();
	return result;
}

