#include "ASCII85Decode.h"

#include <OS.h>

ASCII85Decode::ASCII85Decode(Pusher *sink)
	: Pusher(new PusherBuffer(sink)), char_buffer(0), bytes_to_consume(5), buffered_chars(4), end_of_stream(false)
{
}

ASCII85Decode::~ASCII85Decode()
{
}

ssize_t 
ASCII85Decode::Write(const uint8 *buffer, ssize_t length, bool finish)
{
	ssize_t origLength = length;
	bool eof_code = false;
	uint8 a_byte;

	while (!end_of_stream && length)
	{
		length--;
		a_byte = *buffer++;
		if (a_byte == '~')
		{
			eof_code = true;
			continue;
		}
		if ((a_byte < '!') || (a_byte > 'z'))
		{
			//debugger("ASCII85Decode exception");
			// probably white space
			continue;
		}
		// all zeros special case
		if (a_byte == 'z')
		{
			bytes_to_consume = 0;
			goto output_bytes;
		}
		if ((a_byte == '>') && eof_code)
		{
			switch(bytes_to_consume)
			{
				case 5:
					buffered_chars = 0;
					break;
				case 4:
					// an error
					debugger("ASCII85Decode exception");
					return Pusher::ERROR;
				case 3:
					char_buffer *= 85;
					buffered_chars--;
				case 2:
					char_buffer *= 85;
					buffered_chars--;
				case 1:
					char_buffer *= 85;
					buffered_chars--;
					break;
			}
			end_of_stream = true;
			bytes_to_consume = 0;
			goto output_bytes;
		}
		else eof_code = false;
		char_buffer *= 85;
		char_buffer += a_byte - 33;
		bytes_to_consume--;
output_bytes:
		if (bytes_to_consume == 0)
		{
			ssize_t result;
			uint8 buffer[4];
			int tmp = buffered_chars;
			while (tmp)
			{
				buffer[buffered_chars-tmp] = (char_buffer >> 24) & 0xff;
				char_buffer <<= 8;
				tmp--;
			}
			result = Pusher::Write(buffer, buffered_chars, finish && (length == 0));
			// reset
			bytes_to_consume = 5;
			buffered_chars = 4;
		}
	}
	// finish on EOF
	if (end_of_stream) Pusher::Write(&a_byte, 0, true);
	// reset condition
	//if (finish && end_of_stream) end_of_stream = false;
	// report bytes written
	return origLength - length;
}

