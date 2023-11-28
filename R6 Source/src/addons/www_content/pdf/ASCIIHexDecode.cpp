#include "ASCIIHexDecode.h"

ASCIIHexDecode::ASCIIHexDecode(Pusher *sink)
	: Pusher(new PusherBuffer(sink)), end_of_stream(false)
{
}


ASCIIHexDecode::~ASCIIHexDecode()
{
}


ssize_t 
ASCIIHexDecode::Write(const uint8 *buffer, ssize_t length, bool finish)
{
	// stop accepting data after end of stream marker
	if (end_of_stream && !finish) return Pusher::ERROR;

	uint8 byte;
	ssize_t origLength = length;
	ssize_t result;
	while (!end_of_stream && (length != 0))
	{
		length--;
		switch (byte = *buffer++)
		{
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				hex *= 16;
				hex += byte - '0';
				nibbles++;
				break;
			case 'a': case 'b': case 'c':
			case 'd': case 'e': case 'f':
				hex *= 16;
				hex += 10 + byte - 'a';
				nibbles++;
				break;
			case 'A': case 'B': case 'C':
			case 'D': case 'E': case 'F':
				hex *= 16;
				hex += 10 + byte - 'A';
				nibbles++;
				break;
			case '>':
				// assume trailing zero if odd number of hex digits
				end_of_stream = true;
				if (nibbles == 1) {
					hex *= 16;
					nibbles = 2;
				}
				break;

			case ' ':
			case '\n':
			case '\r':
			case '\f':
			case '\t':
				// ignore white space
				continue;

			default:
				// anything else is an error
				return Pusher::ERROR;
		}
		if (nibbles == 2)
		{
			result = Pusher::Write(&hex, sizeof(uint8), finish && (length == 0));
			if (result != 1) return result;
			hex = nibbles = 0;
		}
	}
	// finish on EOF
	if (!finish && end_of_stream) Pusher::Write(&hex, 0, true);
	// reset condition
	if (finish && end_of_stream) end_of_stream = false;
	// report bytes written
	return origLength - length;
}

