/***************************************************************************
//
//	File:			media2/IBufferSource.h
//
//	Description:	Interface that provides access to an abstract buffer source.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _MEDIA2_BUFFERSOURCE_INTERFACE_
#define _MEDIA2_BUFFERSOURCE_INTERFACE_

#include <media2/MediaDefs.h>
#include <media2/Buffer.h>
#include <support2/IInterface.h>
#include <support2/Binder.h>

namespace B {
namespace Media2 {

using namespace Support2;

class IBufferSource : public IInterface
{
public:
	B_DECLARE_META_INTERFACE(BufferSource);

	virtual	status_t				AcquireBuffer(
										BBuffer * outBuffer,
										int32 id = BBuffer::ANY_BUFFER,
										bigtime_t timeout = B_INFINITE_TIMEOUT) = 0;

	virtual ssize_t					ListBuffers(
										BVector<BBuffer> * outBuffers) = 0;
};

class LBufferSource : public LInterface<IBufferSource>
{
public:
	virtual status_t	Called(BValue &in, const BValue &outBindings, BValue &out);
};

} } // namespace B::Media2
#endif //_MEDIA2_BUFFERSOURCE_INTERFACE_
