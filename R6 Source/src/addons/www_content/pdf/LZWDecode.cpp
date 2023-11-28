#include "LZWDecode.h"

#include <Debug.h>
#include <stdio.h>
#include <OS.h>

LZWDecode::LZWDecode(Pusher *sink, uint32 earlyChange)
	: Pusher(new PusherBuffer(sink)), fEarlyChange(earlyChange)
{
	// allocate the first table
	dict_entry *e = fTable = new dict_entry[0x1000];
	// init the first 256 entries
	for (int i = 0; i < 256; i++, e++)
	{
		e->this_code = (uint8)i;
		e->last_code = -1;
	}
	PrivateReset();
}


LZWDecode::~LZWDecode()
{
	// clean up the table
	delete [] fTable;
}

ssize_t 
LZWDecode::Write(const uint8 *buffer, ssize_t length, bool finish)
{
	int16 this_code = -2;
	ssize_t result;
	ssize_t origLength = length;

	while (length)
	{
		// make one 9-12 bit integer, depending on the current fCodeBits value
		while ((length != 0) && (fBufferedBits < fCodeBits))
		{
			fBitBuffer <<= 8;
			fBitBuffer |= *buffer++; length--;
			fBufferedBits += 8;
		}
		// bail out if not enough bits at end of input
		if (fBufferedBits < fCodeBits) break;
		fBufferedBits -= fCodeBits;
		this_code = (fBitBuffer >> fBufferedBits) & fCodeMask;

		// special codes?
		if (this_code == 256)
		{
			// reset the tables
			ResetTable();
			continue;
		}
		if (this_code == 257)
		{
			// end of input
			fLastCode = this_code;
			break;
		}

		// first time through or after table reset?
		if (fLastCode < 0)
		{
			result = ChaseCodes(fLastCode = this_code, false);
			if (result < 0) return result;
			continue;
		}
		//printf("got code %d, fMaxCode: %d\n", this_code, fMaxCode);
		// normal codes
		if (this_code > fMaxCode)
		{
			if (this_code - fMaxCode > 1)
			{
				printf("this: %d, max: %d\n", this_code, fMaxCode);
				debugger("oops");
			}
			// find first char of previous code
			// add new entry
			AddCode(fLastCode, FirstCode(fLastCode));
		}
		else
			// add new code to table
			//if (this_code < fMaxCode)
			AddCode(fLastCode, FirstCode(this_code));

		// output the string
		result = ChaseCodes(fLastCode = this_code, false);
		if (result < 0) return result;
	}
	// finish on EOF
	if (
		((this_code == 257) && (fLastCode == 257)) ||
		((length == 0) && (finish == true)))
	{
		Pusher::Write((uint8*)&this_code, 0, true);
		length = 0;  // lie, and tell them we took everything
	}
	// reset condition
	if (finish && (fLastCode == 257)) PrivateReset();
	// report bytes written
	return origLength - length;
}

void
LZWDecode::ResetTable(void)
{
	// restart the fCodeBits counter
	fCodeBits = 9;
	fCodeMask = (1 << fCodeBits) - 1;
	fStackTop = -1;
	fMaxCode = 257;
	fLastCode = -1;
}

void
LZWDecode::PrivateReset()
{
	// prep for first Read()
	ResetTable();
	fBufferedBits = 0;
	fBitBuffer = 0;
}

uint8
LZWDecode::FirstCode(int16 start_code)
{
	// return the first terminal code of this sequence
	while (fTable[start_code].last_code >= 0)
		start_code = fTable[start_code].last_code;
	return fTable[start_code].this_code;
}

ssize_t
LZWDecode::ChaseCodes(int16 start_code, bool finish)
{
	ssize_t result = Pusher::ERROR;

	// while more codes
	while (start_code >= 0)
	{
		// stack up the last byte
		fTable[++fStackTop].stack = fTable[start_code].this_code;
		// nest code
		start_code = fTable[start_code].last_code;
	}
#if 0
	if (fStackTop > 0xfff)
	{
		printf("\n\n\n****************\nChaseCodes() stack overflow\n*********************\n");
	}
#endif
	// finish stack
	while (fStackTop >= 0)
	{
		// output byte
		result = Pusher::Write(&(fTable[fStackTop].stack), 1, finish && (fStackTop == 0));
		fStackTop--;
		if (result != 1) break;
	}
	// return status of write
	ASSERT(result >= 0);
	return result;
}

#if 0
void dump_buffer(uint8 *buffer, size_t bytes)
{
	printf("-->>");
	for (int i = 0; i < bytes; i++)
	{
		switch (buffer[i])
		{
			case '\\':
				printf("\\\\");
				break;
			case 0x0a:
				printf("\\n");
				break;
			case 0x0d:
				printf("\\r");
				break;
			default:
				if ((buffer[i] < ' ') || (buffer[i] > 126))
				{
					printf("\\x%.2x", buffer[i]);
				}
				else printf("%c", buffer[i]);
				break;
		}
	}
	printf("<<--\n");
}
#endif

void
LZWDecode::AddCode(int16 last_code, uint8 this_code)
{
	fMaxCode++;
	fTable[fMaxCode].last_code = last_code;
	fTable[fMaxCode].this_code = this_code;
#if 0
	{
	uint8 buffer[2048];
	size_t bytes = ChaseCodes(fMaxCode, buffer, sizeof(buffer));
	printf("added code ");
	dump_buffer(buffer, bytes);
	}
#endif
	if ((fMaxCode == (fCodeMask - (fEarlyChange ? 1 : 0))) && fCodeBits < 12)
	{
		// more bits!
		fCodeBits++;
		fCodeMask = (1 << fCodeBits) - 1;
	}
}
