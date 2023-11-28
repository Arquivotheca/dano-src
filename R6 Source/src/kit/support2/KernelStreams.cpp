
#include <support2/KernelStreams.h>

#include <unistd.h>

namespace B {
namespace Support2 {

/*-------------------------------------------------------------*/

BKernelOStr::BKernelOStr(int32 descriptor) : m_descriptor(descriptor)
{
}

BKernelOStr::~BKernelOStr()
{
	if (m_descriptor >= 0)
		close(m_descriptor);
}

ssize_t 
BKernelOStr::Write(const void *buffer, size_t size)
{
	if (m_descriptor >= 0)
		return write(m_descriptor,buffer,size);
	else
		return B_ERROR;
}

ssize_t 
BKernelOStr::WriteV(const struct iovec *vector, ssize_t count)
{
	if (m_descriptor >= 0)
		return writev(m_descriptor,vector,count);
	else
		return B_ERROR;
}

status_t 
BKernelOStr::End()
{
	close(m_descriptor);
	m_descriptor = -1;
	return B_OK;
}

status_t 
BKernelOStr::Sync()
{
#warning implement BKernelOStr::Sync()
	return B_OK;
}

BKernelIStr::BKernelIStr(int32 descriptor) : m_descriptor(descriptor)
{
}

BKernelIStr::~BKernelIStr()
{
	close(m_descriptor);
}

ssize_t 
BKernelIStr::Read(void* buffer, size_t size)
{
	return read(m_descriptor,buffer,size);
}

ssize_t 
BKernelIStr::ReadV(const struct iovec *vector, ssize_t count)
{
	return readv(m_descriptor,vector,count);
}

} }	// namespace B::Support2
