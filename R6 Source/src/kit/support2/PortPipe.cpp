
#include <support2/PortPipe.h>

#include <malloc.h>
#include <memory.h>
#include <stdio.h>

namespace B {
namespace Support2 {

BPortPipe::BPortPipe(port_id port, int32 id)
	: m_port(port), m_id(id), m_portBuffer(NULL), m_portBufferSize(0)
{
}

BPortPipe::~BPortPipe()
{
	if (m_portBuffer) free(m_portBuffer);
}

port_id 
BPortPipe::Port()
{
	return m_port;
}

int32 
BPortPipe::ID()
{
	return m_id;
}

uint8 *
BPortPipe::Buffer(int32 assertSize)
{
	if (assertSize > m_portBufferSize) {
		m_portBufferSize = assertSize;
		m_portBuffer = (uint8*)realloc(m_portBuffer,m_portBufferSize);
	}
	
	return m_portBuffer;
}

/******************************************************************/

BPortInputPipe::BPortInputPipe(port_id inputPort, int32 id)
	: BPortPipe(inputPort,id)
{
}

BPortInputPipe::~BPortInputPipe()
{
}

void 
BPortInputPipe::RenewBuffer(uint8 *&buffer, int32 &size, int32 &filled)
{
	int32 err,incomingID;
	int32 leftovers = size-filled;
	fprintf(stdout,"waiting for buffer on port %ld ... ",Port());
	fflush(stdout);
	int32 newSize = port_buffer_size(Port());

	if (leftovers < 0) leftovers = 0;
	buffer = Buffer(newSize+leftovers);
	
	if (leftovers) memmove(buffer,buffer+filled,leftovers);

	fprintf(stdout,"reading %ld byte buffer @%p from port %ld... ",newSize,buffer+leftovers,Port());
	fflush(stdout);
	err = read_port(Port(), &incomingID, buffer+leftovers, newSize);
	fprintf(stdout,"result was %ld\n",err);

	if (err > 0) {
		memmove(buffer+leftovers,buffer+leftovers+4,err-4);
		size = err-4+leftovers;
	} else {
		size = 0;
	}
}

/******************************************************************/


BPortOutputPipe::BPortOutputPipe(port_id outputPort, int32 id, int32 bufferSize)
	: BPortPipe(outputPort,id), m_bufferSize(bufferSize)
{
}

BPortOutputPipe::~BPortOutputPipe()
{
}

void 
BPortOutputPipe::RenewBuffer(uint8 *&buffer, int32 &size, int32 &filled)
{
	if (filled) {
		memmove(buffer+4,buffer,filled);
		filled += 4;
		*((int32*)buffer) = filled;
		fprintf(stdout,"writing %ld byte buffer @%p to port %ld with ID-%ld... ",filled,buffer,Port(),ID());
		fflush(stdout);
		int32 err = write_port(Port(), ID(), buffer, filled);
		fprintf(stdout,"result was %ld (%s)\n",err,strerror(err));
	}
	buffer = Buffer(m_bufferSize);
	size = m_bufferSize-4;
}

} }	// namespace B::Support2
