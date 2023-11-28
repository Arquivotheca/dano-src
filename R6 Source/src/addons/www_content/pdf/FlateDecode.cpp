#include "FlateDecode.h"
#include <zlib.h>
#include <malloc.h>

/* alloc/free wrapper functions needed by zlib */
static voidpf zalloc(voidpf , uInt items, uInt size)
{
	return calloc(items, size);
}

static void zfree(voidpf , voidpf address)
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
Byte		m_buffer[BUFFER_SIZE];
Bytef		*m_next_to_write;
z_stream	m_zstream;
status_t	m_zstatus;
};
typedef FlateDecode::private_data FPD;

FlateDecode::private_data::private_data()
{
	m_zstream.zalloc = (alloc_func)zalloc;
	m_zstream.zfree = (free_func)zfree;
	m_zstream.opaque = 0;

	reset_data();
	
	m_zstatus = inflateInit(&m_zstream);
}

void 
FlateDecode::private_data::reset_data(void)
{
	m_zstream.next_in = 0;
	m_zstream.avail_in = 0;
	m_zstream.total_in = 0;
	
	m_zstream.next_out = m_next_to_write = m_buffer;
	m_zstream.avail_out = 0;
	m_zstream.total_out = 0;
}


FlateDecode::private_data::~private_data()
{
	inflateEnd(&m_zstream);
}

FlateDecode::FlateDecode(Pusher *sink)
	: Pusher(sink), m_pd(new FPD())
{
}


FlateDecode::~FlateDecode()
{
	delete m_pd;
}

ssize_t 
FlateDecode::Write(const uint8 *buffer, ssize_t length, bool finish)
{
	ssize_t origLength = length;
	ssize_t count;
	z_stream *m_zstream = &(m_pd->m_zstream);
	ssize_t result = 0;

	// any left over from the last time?
	count = m_zstream->next_out - m_pd->m_next_to_write;
	while (length && (result >= 0))
	{
		// move anything at the end of the output buffer to the front
		if (count)
		{
			memcpy(m_pd->m_buffer, m_pd->m_next_to_write, count);
		}
		// note where we start writing uncompressed data from
		m_pd->m_next_to_write = m_pd->m_buffer;
		// note available room in output buffer
		m_zstream->next_out = m_pd->m_buffer + count;
		m_zstream->avail_out = FPD::BUFFER_SIZE - count;
		// load up parms for inflate
		m_zstream->next_in = const_cast<uint8 *>(buffer);
		m_zstream->avail_in = length;
		// fill the output buffer
		m_pd->m_zstatus = inflate(&(m_pd->m_zstream), Z_SYNC_FLUSH);
		// check for errors
		if (m_pd->m_zstatus < Z_OK) m_pd->m_zstatus = Z_STREAM_END;
		// if ((m_pd->m_zstatus != Z_OK) && (m_pd->m_zstatus != Z_STREAM_END)) break;
		// adjust input source
		count = length - m_zstream->avail_in;
		length -= count;
		buffer += count;
		// write as much as possible to our sink
		count = m_zstream->next_out - m_pd->m_buffer;
		while (count)
		{
			result = Pusher::Write(m_pd->m_next_to_write, count, (finish && (length == 0)) || (m_pd->m_zstatus == Z_STREAM_END));
			if (result <= 0) break;
			m_pd->m_next_to_write += result;
			count -= result;
		}
		if (m_pd->m_zstatus == Z_STREAM_END) length = 0;
	}
	if (!length) result = origLength;
	return result;
}

