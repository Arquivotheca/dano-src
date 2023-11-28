#ifndef _CINEPAK_H_
#define _CINEPAK_H_

#include <media2/MediaNode.h>

namespace B {
namespace Private {

class CinepakDecoder : public B::Media2::BMediaNode
{
	::B::Media2::BMediaInput::ptr mInput;
	::B::Media2::BMediaOutput::ptr mOutput;

	size_t mOutputSize;
	
	void CreateInput();

public:
	CinepakDecoder();

	virtual status_t Acquired (const void *id);
	virtual status_t Released (const void *id);

	virtual	void Connected (::B::Media2::BMediaEndpoint::arg localEndpoint,
							::B::Media2::IMediaEndpoint::arg remoteEndpoint,
							const ::B::Media2::BMediaFormat &format);

	virtual	void Disconnected (::B::Media2::BMediaEndpoint::arg localEndpoint,
								::B::Media2::IMediaEndpoint::arg remoteEndpoint);		

	virtual	status_t HandleBuffer (::B::Media2::BMediaInput::arg receiver,
									::B::Media2::BBuffer *buffer);		

private:
	void                    *dptr;
};

} } // B::Private

#endif /* _CINEPAK_H_ */
