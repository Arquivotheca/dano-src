
#include <malloc.h>
#include <memory.h>
#include <support2/StdIO.h>
#include <support2/ITextStream.h>
#include <support2/StreamPipe.h>

namespace B {
namespace Support2 {

BStreamPipe::BStreamPipe(size_t bufferSize)
{
	m_bufferSize = bufferSize;
	m_buffer = (uint8*)malloc(bufferSize);
}

BStreamPipe::~BStreamPipe()
{
	free(m_buffer);
}

uint8 *
BStreamPipe::Buffer(int32 assertSize)
{
	if (assertSize > m_bufferSize) {
		m_bufferSize = assertSize;
		m_buffer = (uint8*)realloc(m_buffer,m_bufferSize);
	}
	
	return m_buffer;
}

int32 
BStreamPipe::BufferSize()
{
	return m_bufferSize;
}

BStreamInputPipe::BStreamInputPipe(IByteInput:: arg stream, size_t bufferSize)
	: BStreamPipe(bufferSize), m_stream(stream)
{
}

BStreamInputPipe::~BStreamInputPipe()
{
}

void 
BStreamInputPipe::RenewBuffer(uint8 *&buffer, int32 &size, int32 &filled)
{
	int32 err;
	int32 leftovers = size-filled;
	int32 currentBufferSize = BufferSize();

	if (leftovers < 0) leftovers = 0;
	if ((leftovers + (currentBufferSize/2)) > currentBufferSize) {
		currentBufferSize = leftovers + (currentBufferSize/2);
		buffer = Buffer(currentBufferSize);
	} else
		buffer = Buffer();
	
	if (leftovers) memmove(buffer,buffer+filled,leftovers);
	filled = 0;

/*
	berr
		<< "reading "
		<< (currentBufferSize-leftovers)
		<< " byte buffer @"
		<< (buffer+leftovers)
		<< " from descriptor "
		<< Descriptor()
		<< " ... "
		<< endl;
*/
	err = m_stream->Read(buffer+leftovers,currentBufferSize-leftovers);

#if BINDER_DEBUG_MSGS
	berr
		<< "read "<< err <<" bytes from stream: " << endl
		<< indent << BHexDump(buffer+leftovers,err) << dedent << endl;
#endif

/*
	berr
		<< "result was "
		<< err
		<< endl;
*/
	if (err > 0) {
		size = err+leftovers;
	} else if (err == 0) {
		buffer = Buffer(BufferSize()*2);
		size = BufferSize();
	} else {
		SetStatus(err);
		filled = 0;
	}
}

BStreamOutputPipe::BStreamOutputPipe(IByteOutput:: arg stream, size_t bufferSize)
	: BStreamPipe(bufferSize), m_stream(stream.ptr())
{
	m_stream->Acquire(this);
}

BStreamOutputPipe::BStreamOutputPipe(IByteOutput *stream, size_t bufferSize)
	: BStreamPipe(bufferSize), m_stream(stream)
{
}

BStreamOutputPipe::~BStreamOutputPipe()
{
	m_stream->AttemptRelease(this);
}

void 
BStreamOutputPipe::RenewBuffer(uint8 *&buffer, int32 &size, int32 &filled)
{
/*
	berr
		<< "writing "
		<< filled
		<< " byte buffer @"
		<< buffer
		<< " to descriptor "
		<< Descriptor()
		<< " ... " << endl;
*/

	int32 err = 0;
	if (filled) {
#if BINDER_DEBUG_MSGS
		berr
			<< "writing "<< filled <<" bytes to binder: "
			<< indent << BHexDump(buffer,filled) << dedent << endl;
#endif
	
		err = m_stream->Write(buffer,filled);
	}
#if BINDER_DEBUG_MSGS
	berr
		<< "result was "
		<< err
		<< endl;
#endif

	if (err > 0) {
		if (err < filled) memmove(buffer,buffer+err,filled-err);
		filled -= err;
		size = BufferSize();
	} else if (err == 0) {
		size = BufferSize();
		if (!size) size = 256;
		buffer = Buffer(size*2);
		size = BufferSize();
	} else {
		SetStatus(err);
		filled = 0;
	}
}

} }	// namespace B::Support2
