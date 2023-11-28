/******************************************************************************
/
/	File:			BinderStreams.h
/
/	Copyright 1993-98, Be Incorporated
/
******************************************************************************/

#ifndef	_SUPPORT2_BINDERSTREAMS_H
#define	_SUPPORT2_BINDERSTREAMS_H

#include <sys/uio.h>
#include <support2/SupportDefs.h>
#include <support2/ByteStream.h>

namespace B {
namespace Support2 {

/*---------------------------------------------------------------------*/

class BBinderOStr : public LByteOutput
{
	public:
								BBinderOStr(int32 descriptor);
		virtual					~BBinderOStr();
		
		virtual	ssize_t			WriteV(const struct iovec *vector, ssize_t count);
		virtual	status_t		End();
		virtual	status_t		Sync();

	private:

				int32			m_descriptor;
};

/*-------------------------------------------------------------*/

class BBinderIStr : public LByteInput
{
	public:
								BBinderIStr(int32 descriptor);
		virtual					~BBinderIStr();
		
		virtual	ssize_t			ReadV(const struct iovec *vector, ssize_t count);

	private:

				int32			m_descriptor;
};

/*-------------------------------------------------------------*/


BBinderOStr::BBinderOStr(int32 descriptor) : m_descriptor(descriptor)
{
}

BBinderOStr::~BBinderOStr()
{
}

ssize_t 
BBinderOStr::WriteV(const struct iovec *vector, ssize_t /*count*/)
{
#warning BBinderOStr::WriteV() does not support iovec
	if (m_descriptor != -1)
		return write_binder(m_descriptor,vector->iov_base,vector->iov_len);
	else
		return B_ERROR;
}

status_t 
BBinderOStr::End()
{
	close(m_descriptor);
	m_descriptor = -1;
	return B_OK;
}

status_t 
BBinderOStr::Sync()
{
	return B_OK;
}

BBinderIStr::BBinderIStr(int32 descriptor) : m_descriptor(descriptor)
{
}

BBinderIStr::~BBinderIStr()
{
}

ssize_t 
BBinderIStr::ReadV(const struct iovec *vector, ssize_t /*count*/)
{
#warning BBinderOStr::ReadV() does not support iovec
	return read_binder(m_descriptor,vector->iov_base,vector->iov_len);
}

} } // namespace B::Support2

#endif /* _SUPPORT2_KERNELSTREAMS_H */
