#include "pdf_filters.h"

#include <zlib.h>
#include <malloc.h>
#include "rc4.h"

#include <stdio.h>
#include <memory.h>
#include <OS.h>


ASCIIHexDecode::ASCIIHexDecode(BInputFilter *source)
	: BInputFilter(source), next_byte(0), bytes_in_buffer(0), hex(0), nibbles(0), end_of_stream(false)
{
}

ASCIIHexDecode::~ASCIIHexDecode()
{
}

ssize_t 
ASCIIHexDecode::Read(void *buffer, size_t size)
{
	uint8 *buf = (uint8 *)buffer;
	uint8 byte = 0;

	while (size && !end_of_stream)
	{
		while (next_byte >= bytes_in_buffer)
		{
			next_byte = 0;
			bytes_in_buffer = BInputFilter::Read(byte_buffer, sizeof(byte_buffer));
			if (bytes_in_buffer < 0) goto exit0;
		}
		switch (byte_buffer[next_byte++])
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
				end_of_stream = true;
				// assume trailing zero if odd number of hex digits
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
				return BInputFilter::ERROR;
		}
		if (nibbles == 2)
		{
			*buf++ = hex;
			hex = nibbles = 0;
			size--;
		}
	}
exit0:
	// any bytes buffered?
	if (buf != buffer) return (buf - (uint8 *)buffer);
	return BInputFilter::END_OF_INPUT;
}

void 
ASCIIHexDecode::Reset(void)
{
	next_byte =
	bytes_in_buffer = 0;
	hex =
	nibbles = 0;
	end_of_stream = false;
	BInputFilter::Reset();
}


ASCII85Decode::ASCII85Decode(BInputFilter *source)
	: BInputFilter(source), next_byte(0), bytes_in_buffer(0),
		char_buffer(0), buffered_chars(0), end_of_stream(false)
{
}


ASCII85Decode::~ASCII85Decode()
{
}

ssize_t
ASCII85Decode::ReadFive()
{
	size_t bytes_to_consume = 5;
	bool eof_code = false;
	uint8 a_byte;
	// decode the bytes
	char_buffer = 0;
	buffered_chars = 4;
	
	while (bytes_to_consume)
	{
		if (bytes_in_buffer == next_byte)
		{
			// get some data
#if 0
			// this is byte-at-a-time because inline image filters don't
			// necessarily provide a byte-count to limit the source.  Grrrr.
			while (!(bytes_in_buffer = BInputFilter::Read(byte_buffer, 1)))
				;
#else
			bytes_in_buffer = BInputFilter::Read(byte_buffer, sizeof(byte_buffer));
#endif
			// end of input?
			if (bytes_in_buffer < 0)
			{
				// should never happen
				debugger("ASCII85Decode - out of data!\n");
				return bytes_in_buffer;
			}
			// reset pointer to first byte
			next_byte = 0;
		}
		// get byte to decode
		a_byte = byte_buffer[next_byte++];
		// out of range
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
			break;
		}
		if ((a_byte == '>') && eof_code)
		{
			switch(bytes_to_consume)
			{
				case 4:
					// an error
					debugger("ASCII85Decode exception");
					return -2;
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
			break;
		}
		else eof_code = false;
		char_buffer *= 85;
		char_buffer += a_byte - 33;
		bytes_to_consume--;
	}
	return buffered_chars;
}

ssize_t 
ASCII85Decode::Read(void *buffer, size_t size)
{
	ssize_t bytes_read = 0;
	uint8 *buf = (uint8 *)buffer;
	// why someone would pass a zero-length buffer, I don't know, but...
	if (!size) return 0;
	if (end_of_stream) return BInputFilter::END_OF_INPUT;

	do
	{
		// prime the input pump, if req'd
		if (!buffered_chars) bytes_read = ReadFive();
		// more bytes to write out?
		while (buffered_chars && size)
		{
			*buf++ = (char_buffer >> 24) & 0xff;
			char_buffer <<= 8;
			buffered_chars--;
			size--;
		}
	}
	// while there is room
	while (size && (bytes_read >= 0) && !end_of_stream);

	// no more bytes to read and nothing in buffer? bail
	if ((bytes_read < 0) && (buf == buffer)) return bytes_read;
	// return bytes written
	return (buf - (uint8 *)buffer);
}

void 
ASCII85Decode::Reset(void)
{
	buffered_chars = 
	bytes_in_buffer =
	next_byte = 0;
	end_of_stream = false;
	BInputFilter::Reset();
}


int16
LZWDecode::NextByte(void)
{
	int16 a_byte;

	// fill the buffer
	while (buffered_bytes == 0)
	{
		bytes_in_buffer = buffered_bytes = BInputFilter::Read(byte_buffer, sizeof(byte_buffer));
		//if (buffered_bytes > 0) printf("\n\n****\nGot %ld buffered_bytes\n****\n", buffered_bytes);
	}
	if (buffered_bytes < 0) return -2;
	a_byte = byte_buffer[bytes_in_buffer - buffered_bytes--];
	return a_byte;
}

int16
LZWDecode::NextCode(void)
{
	int16 a_byte;

	// return one 9-12 bit integer, depending on the current code_bits value
	while (buffered_bits < code_bits)
	{
		a_byte = NextByte();
		// unexpected end of input
		if (a_byte < 0) return a_byte;
		bit_buffer <<= 8;
		bit_buffer |= a_byte & 0xff;
		buffered_bits += 8;
	}
	buffered_bits -= code_bits;
	a_byte = (bit_buffer >> buffered_bits) & code_mask;
	//bit_buffer &= (1 << buffered_bits) - 1;
	return a_byte;
}

void
LZWDecode::ResetTable(void)
{
	// restart the code_bits counter
	code_bits = 9;
	code_mask = (1 << code_bits) - 1;
	stack_top = -1;
	max_code = 257;
}

void
LZWDecode::PrivateReset()
{
	// prep for first Read()
	last_code = -1;
	ResetTable();
	buffered_bytes = 0;
	buffered_bits = 0;
	bit_buffer = 0;
}

LZWDecode::LZWDecode(BInputFilter *source, uint32 earlyChange)
	: BInputFilter(source), early_change(earlyChange)
{
	// allocate the first table
	dict_entry *e = table = new dict_entry[0x1000];
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
	delete [] table;
}

uint8
LZWDecode::FirstCode(int16 start_code)
{
	// return the first terminal code of this sequence
	while (table[start_code].last_code >= 0)
		start_code = table[start_code].last_code;
	return table[start_code].this_code;
}

size_t
LZWDecode::ChaseCodes(int16 start_code, uint8 *buffer, size_t bytes_max)
{
	size_t bytes_free = bytes_max;

	// while more codes
	while (start_code >= 0)
	{
		// stack up the last byte
		table[++stack_top].stack = table[start_code].this_code;
		// nest code
		start_code = table[start_code].last_code;
	}
#if 0
	if (stack_top > 0xfff)
	{
		printf("\n\n\n****************\nChaseCodes() stack overflow\n*********************\n");
	}
#endif
	// flush stack
	while ((stack_top >= 0) && bytes_free)
	{
		// output byte
		*buffer++ = table[stack_top--].stack;
		// decrement bytes free
		bytes_free--;
	}
	// return number of bytes written
	return bytes_max - bytes_free;
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
	max_code++;
	table[max_code].last_code = last_code;
	table[max_code].this_code = this_code;
#if 0
	{
	uint8 buffer[2048];
	size_t bytes = ChaseCodes(max_code, buffer, sizeof(buffer));
	printf("added code ");
	dump_buffer(buffer, bytes);
	}
#endif
	if ((max_code == (code_mask - (early_change ? 1 : 0))) && code_bits < 12)
	{
		// more bits!
		code_bits++;
		code_mask = (1 << code_bits) - 1;
	}
}

ssize_t 
LZWDecode::Read(void *buffer, size_t size)
{
	uint8 *buf = (uint8 *)buffer;
	ssize_t written;
	int16 this_code;

	// why someone would pass a zero-length buffer, I don't know, but...
	if (!size) return 0;
	// end of input?
	if (last_code == 257) return BInputFilter::END_OF_INPUT;
	//printf("buffer is %ld bytes long\n", size);
	// decompress any remaining code from last time
	if (stack_top >= 0)
	{
		written = ChaseCodes(-1, buf, size);
		//if (written == size) return written;
		buf += written;
		size -= written;
	}
	// bail out if can't complete chasing codes
	if (stack_top >= 0)
	{
		//printf("incomplete ChaseCodes()\n");
		goto exit0;
	}
	// first time through?
	if (last_code < 0)
	{
		// get first code
		last_code = NextCode();
		// if it's the clear table code, try again
		if (last_code == 256) last_code = NextCode();
		// process this code
		written = ChaseCodes(last_code, buf, size);
		buf += written;
		size -= written;
	}
	// _in theory_, there should be room enough for 1 byte in the buffer, as that's all
	// the previous statements could possibly generate

	// while there are more codes to process
	while (size && ((this_code = NextCode()) >= 0))
	{
		// special codes?
		if (this_code == 256)
		{
			// reset the tables
			ResetTable();
			//printf("\n\n*******\nGot TableReset code\n******\n\n");
			this_code = NextCode();
			if (this_code < 0) goto exit0;
			if (this_code != 257) goto bypass_add;
			// odd, clear table followed by end of data marker
		}
		if (this_code == 257)
		{
			// end of input
			last_code = this_code;
			goto exit0;
		}
		//printf("got code %d, max_code: %d\n", this_code, max_code);
		// normal codes
		if (this_code > max_code)
		{
			if (this_code - max_code > 1)
			{
				printf("this: %d, max: %d\n", this_code, max_code);
				debugger("oops");
			}
			// find first char of previous code
			// add new entry
			AddCode(last_code, FirstCode(last_code));
		}
		else
			// add new code to table
			//if (this_code < max_code)
			AddCode(last_code, FirstCode(this_code));
bypass_add:
		// output the string
		written = ChaseCodes(this_code, buf, size);
		// adjust buffer and size
		buf += written;
		size -= written;
		// save current code for next trip through
		last_code = this_code;
		// bail out if there was no more room in the output buffer
		if (stack_top >= 0) break;
	}
exit0:
	//dump_buffer((uint8 *)buffer, (buf - (uint8 *)buffer));
	// return bytes written
	return (buf - (uint8 *)buffer);
}

void 
LZWDecode::Reset(void)
{
	PrivateReset();
	BInputFilter::Reset();
}


RunLengthDecode::RunLengthDecode(BInputFilter *source)
	: BInputFilter(source), rle_code(128), rle_value(0), end_of_stream(false)
{
}


RunLengthDecode::~RunLengthDecode()
{
}

ssize_t 
RunLengthDecode::Read(void *buffer, size_t size)
{
	uint8 *buf = (uint8 *)buffer;
	uint8 a_byte;
	if (end_of_stream) return BInputFilter::END_OF_INPUT;

	while (size)
	{
		if (rle_code == 128)
		{
			// next command code
			if (BInputFilter::Read(&a_byte, sizeof(rle_code)) < 0) return BInputFilter::ERROR;
			rle_code = (uint16)a_byte;
			if (rle_code == 128)
			{
				end_of_stream = true;
				break;
			}
			if (rle_code > 128)
			{
				if (BInputFilter::Read(&rle_value, sizeof(rle_value)) < 0) return BInputFilter::ERROR;
			}
		}
		if (rle_code > 128)
		{
			// expand value byte more times
			ssize_t expansion_count = (ssize_t)max_c(size, 257 - (size_t)rle_code);
			memset(buf, rle_value, expansion_count);
			size -= expansion_count;
			buf += expansion_count;
			rle_code += expansion_count;
			if (rle_code > 256) rle_code = 128;
		}
		if (rle_code < 128)
		{
			// read raw data from input
			ssize_t bytes_to_read = (ssize_t)max_c(size, (size_t)rle_code+1);
			ssize_t bytes_read = BInputFilter::Read(buf, bytes_to_read);
			if (bytes_read < 0) return BInputFilter::ERROR;
			size -= bytes_read;
			buf += bytes_read;
			if (bytes_read > rle_code) rle_code = 128;
			else rle_code -= bytes_read;
		}
	}
	return (buf - (uint8*)buffer);
}

void 
RunLengthDecode::Reset(void)
{
	rle_code = 128;
	rle_value = 0;
	end_of_stream = false;
	BInputFilter::Reset();
}


CCITTFaxDecode::CCITTFaxDecode(BInputFilter *source)
	: BInputFilter(source)
{
}


CCITTFaxDecode::~CCITTFaxDecode()
{
}

ssize_t 
CCITTFaxDecode::Read(void *buffer, size_t size)
{
	return BInputFilter::END_OF_INPUT;
}

void 
CCITTFaxDecode::Reset(void)
{
}


DCTDecode::DCTDecode(BInputFilter *source)
	: BInputFilter(source)
{
}


DCTDecode::~DCTDecode()
{
}

ssize_t 
DCTDecode::Read(void *buffer, size_t size)
{
	return BInputFilter::END_OF_INPUT;
}

void 
DCTDecode::Reset(void)
{
}

/* alloc/free wrapper functions needed by zlib */
static voidpf zalloc(voidpf opaque, uInt items, uInt size)
{
	return calloc(items, size);
}

static void zfree(voidpf opaque, voidpf address)
{
	free(address);
}

struct FlateDecode::private_data {
			private_data();
			~private_data();

void		reset_data(void);

			enum {
				BUFFER_SIZE = 4096
			};
Bytef		*m_buffer;
z_stream	m_zstream;
status_t	m_zstatus;
};
typedef FlateDecode::private_data FPD;

FlateDecode::private_data::private_data()
{
	m_zstream.zalloc = (alloc_func)zalloc;
	m_zstream.zfree = (free_func)zfree;
	m_zstream.opaque = 0;

	m_buffer = new uint8 [BUFFER_SIZE];

	reset_data();
	
	m_zstatus = inflateInit(&m_zstream);
}

void 
FlateDecode::private_data::reset_data(void)
{
	m_zstream.next_in = m_buffer;
	m_zstream.avail_in = 0;
	m_zstream.total_in = 0;
	
	m_zstream.next_out = 0;
	m_zstream.avail_out = 0;
	m_zstream.total_out = 0;
}


FlateDecode::private_data::~private_data()
{
	inflateEnd(&m_zstream);
	delete [] m_buffer;
}

FlateDecode::FlateDecode(BInputFilter *source)
	:	BInputFilter(source), m_pd(new FPD())
{
}


FlateDecode::~FlateDecode()
{
	delete m_pd;
}

ssize_t 
FlateDecode::Read(void *buffer, size_t size)
{
	if (m_pd->m_zstatus == Z_STREAM_END) return BInputFilter::END_OF_INPUT;
	bool end_of_input = false;
	ssize_t bytesRead;
	m_pd->m_zstream.avail_out = size;

	// while there is more room in the output buffer
	while (m_pd->m_zstream.avail_out > 0)
	{
		// fill the input buffer
		if (m_pd->m_zstream.avail_in == 0)
		{
			bytesRead = BInputFilter::Read(m_pd->m_buffer, FPD::BUFFER_SIZE);
			if (bytesRead < 0)
			{
				end_of_input = true;
				bytesRead = 0;
			}
			m_pd->m_zstream.next_in = m_pd->m_buffer;
			m_pd->m_zstream.avail_in = bytesRead;
		}
		// inflate the input
		m_pd->m_zstream.next_out = (uint8 *)buffer + (size - m_pd->m_zstream.avail_out);
		m_pd->m_zstatus = inflate(&(m_pd->m_zstream), Z_NO_FLUSH);
		// end of uncompressed data?
		if ((m_pd->m_zstatus == Z_STREAM_END) || (m_pd->m_zstatus < Z_OK)) break;
	}
	// return the number of bytes read
	bytesRead = size - m_pd->m_zstream.avail_out;
	return bytesRead ? bytesRead : BInputFilter::END_OF_INPUT;
}

void 
FlateDecode::Reset(void)
{
	m_pd->reset_data();
	m_pd->m_zstatus = inflateReset(&(m_pd->m_zstream));
}

struct RC4Decode::private_data {
	uint8	m_raw_key[10];
	RC4_KEY	m_key;
};

RC4Decode::RC4Decode(BInputFilter *source, const uint8 *key)
	: BInputFilter(source), m_pd(new RC4Decode::private_data)
{
	// set the key
	SetKey(key);
	// setup RC4
	RC4_set_key(&(m_pd->m_key), 10, m_pd->m_raw_key);
}


RC4Decode::~RC4Decode()
{
	delete m_pd;
}

ssize_t 
RC4Decode::Read(void *buffer, size_t size)
{
	ssize_t bytes_read = BInputFilter::Read(buffer, size);
	// any data read?
	if (bytes_read > 0)
		// RC4 can en/de-code in-place
		RC4(&(m_pd->m_key), bytes_read, (uchar *)buffer, (uchar *)buffer);
	return bytes_read;
}

void 
RC4Decode::Reset(void)
{
	// reset using our parent's code
	BInputFilter::Reset();
	// setup RC4
	RC4_set_key(&(m_pd->m_key), 10, m_pd->m_raw_key);
}

void 
RC4Decode::SetKey(const uint8 *new_key)
{
	// save the key for reset purposes
	for (int i = 0; i < 10; i++)
		m_pd->m_raw_key[i] = new_key[i];
}


PredictedDecode::PredictedDecode(BInputFilter *source, uint32 predictor, uint32 columns, uint32 colors, uint32 bitsPerComponent)
	: BInputFilter(source), m_predictor(predictor), m_columns(columns), m_colors(colors), m_bitsPerComponent(bitsPerComponent),
		m_bytes_in_buffer(0)
{
	m_rowBytes = ((m_columns * m_colors * m_bitsPerComponent) + 7) >> 3;
	m_bpp = ((m_colors * m_bitsPerComponent) + 7) >> 3;
	if (m_predictor == 2)
	{
		// TIFF
		// previous pixel, no line buffering requirements, but..
		m_row = new uint8[m_rowBytes];
	}
	else if ((m_predictor >= 10) || (m_predictor <= 15))
	{
		// PNG "fitlers"
		m_row = new uint8[m_rowBytes];
		m_prev_row = new uint8[m_rowBytes];
		memset(m_row, 0, m_rowBytes);
		memset(m_prev_row, 0, m_rowBytes);
	}
	else
	{
		// 1 or unknown: unchanged
		// we'll just pass the data through un-changed
	}
}


PredictedDecode::~PredictedDecode()
{
	delete [] m_prev_row;
	delete [] m_row;
}

void
PredictedDecode::process_filter_row(uint8 the_predictor)
{
   switch (the_predictor)
   {
      case PNG_FILTER_VALUE_NONE:
         break;
      case PNG_FILTER_VALUE_SUB:
      {
         uint32 i;
         uint32 istop = m_rowBytes;
         uint8p rp = m_row + m_bpp;
         uint8p lp = m_row;

         for (i = m_bpp; i < istop; i++)
         {
            *rp = (uint8)(((int)(*rp) + (int)(*lp++)) & 0xff);
            rp++;
         }
         break;
      }
      case PNG_FILTER_VALUE_UP:
      {
         uint32 i;
         uint32 istop = m_rowBytes;
         uint8p rp = m_row;
         uint8p pp = m_prev_row;

         for (i = 0; i < istop; i++)
         {
            *rp = (uint8)(((int)(*rp) + (int)(*pp++)) & 0xff);
            rp++;
         }
         break;
      }
      case PNG_FILTER_VALUE_AVG:
      {
         uint32 i;
         uint8p rp = m_row;
         uint8p pp = m_prev_row;
         uint8p lp = m_row;
         uint32 istop = m_rowBytes - m_bpp;

         for (i = 0; i < m_bpp; i++)
         {
            *rp = (uint8)(((int)(*rp) +
               ((int)(*pp++) / 2)) & 0xff);
            rp++;
         }

         for (i = 0; i < istop; i++)
         {
            *rp = (uint8)(((int)(*rp) +
               (int)(*pp++ + *lp++) / 2) & 0xff);
            rp++;
         }
         break;
      }
      case PNG_FILTER_VALUE_PAETH:
      {
         uint32 i;
         uint8p rp = m_row;
         uint8p pp = m_prev_row;
         uint8p lp = m_row;
         uint8p cp = m_prev_row;
         uint32 istop = m_rowBytes - m_bpp;

         for (i = 0; i < m_bpp; i++)
         {
            *rp = (uint8)(((int)(*rp) + (int)(*pp++)) & 0xff);
            rp++;
         }

         for (i = 0; i < istop; i++)   /* use leftover rp,pp */
         {
            int a, b, c, pa, pb, pc, p;

            a = *lp++;
            b = *pp++;
            c = *cp++;

            p = b - c;
            pc = a - c;

            pa = p < 0 ? -p : p;
            pb = pc < 0 ? -pc : pc;
            pc = (p + pc) < 0 ? -(p + pc) : p + pc;

            p = (pa <= pb && pa <=pc) ? a : (pb <= pc) ? b : c;

            *rp = (uint8)(((int)(*rp) + p) & 0xff);
            rp++;
         }
         break;
      }
      default:
         break;
   }
}

ssize_t 
PredictedDecode::FillRow(uint8p a_row)
{
	// read data into m_prev_row
	ssize_t result;
	ssize_t bytes_read = 0;
	while (bytes_read != (ssize_t)m_rowBytes)
	{
		result = BInputFilter::Read(a_row + bytes_read, m_rowBytes - bytes_read);
		if (result < 0)
		{
			// note an error
			return result;
		}
		bytes_read += result;
	}
	return bytes_read;
}

ssize_t
PredictedDecode::ReadPNGRow(void)
{
	// read the tag value
	uint8 the_predictor;
	ssize_t result = BInputFilter::Read(&the_predictor, 1);
	if (result <= 0) return BInputFilter::END_OF_INPUT;;

	// read data into m_prev_row
	result = FillRow(m_prev_row);
	if (result < 0) return result;

	// swap prev and current rows
	uint8p tmp = m_prev_row;
	m_prev_row = m_row;
	m_row = tmp;
	// process the row
	process_filter_row(the_predictor);
	// return bytes read, setting buffer size
	return (m_bytes_in_buffer = m_rowBytes);
}

void
PredictedDecode::tiff_process_row(void)
{
	switch (m_bitsPerComponent)
	{
	case 8:
		{
			uint32 count = m_rowBytes - m_colors;
			uint8 prev = *m_row;
			uint8p curr = m_row + m_colors;
			while (count--)
			{
				prev = *curr += prev;
				curr++;
			}
		}
		break;
	case 4:
		{
		uint8 last[4]; // indexed by color component
		switch (m_colors)
		{
			case 4:
			case 2:
				{
				}
				break;
			case 3:
				{
				}
				break;
			case 1:
				{
				}
				break;
			}
		}
		break;
	case 2:
		{
		}
		break;
	case 1:
		{
		}
		break;
	}
}

ssize_t 
PredictedDecode::ReadTIFFRow(void)
{
	// read data into m_row
	ssize_t result = FillRow(m_row);
	if (result < 0) return result;

	// process the row
	tiff_process_row();
	return (m_bytes_in_buffer = m_rowBytes);
}

ssize_t 
PredictedDecode::Read(void *buffer, size_t size)
{
	uint8 *buf = (uint8p)buffer;
	while (size)
	{
		// any more data left in output buffer?
		uint32 bytes_to_copy = min_c(size, m_bytes_in_buffer);
		if (bytes_to_copy)
		{
			// squeeze it out
			memcpy(buf, m_row + (m_rowBytes - m_bytes_in_buffer), bytes_to_copy);
			m_bytes_in_buffer -= bytes_to_copy;
			size -= bytes_to_copy;
			buf += bytes_to_copy;
		}
		else
		{
			// reload the output buffer
			if (m_predictor == TIFF_PREDICTOR)
			{
				// TIFF
				ssize_t result = ReadTIFFRow();
				if (result <= 0)
				{
					// stop reading
					if (buf != (uint8p)buffer) break;
					else return result;
				}
			}
			else if ((m_predictor >= PNG_FILTER_VALUE_NONE) || (m_predictor <= PNG_FILTER_VALUE_OPTIMUM))
			{
				// PNG "fitlers"
				ssize_t result = ReadPNGRow();
				if (result <= 0)
				{
					// stop reading
					if (buf != (uint8p)buffer) break;
					else return result;
				}
			}
			else
			{
				// 1 or unknown: unchanged
				ssize_t result = BInputFilter::Read(buf, size);
				if (result <= 0)
				{
					if (buf != (uint8p)buffer) break;
					else return result;
				}
				buf += result;
				size -= result;
				m_bytes_in_buffer = 0;				
			}
		}
	}
	return (buf - (uint8p)buffer);
}

void 
PredictedDecode::Reset(void)
{
	if (m_row) memset(m_row, 0, m_rowBytes);
	if (m_prev_row) memset(m_prev_row, 0, m_rowBytes);
	m_bytes_in_buffer = 0;
	BInputFilter::Reset();
}


