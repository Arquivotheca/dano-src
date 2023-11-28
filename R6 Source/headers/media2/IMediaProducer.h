#ifndef _MEDIA2_MEDIAPRODUCER_INTERFACE_
#define _MEDIA2_MEDIAPRODUCER_INTERFACE_

#include <support2/IInterface.h>
#include <support2/IBinder.h>
#include <media2/MediaDefs.h>

namespace B {
namespace Media2 {

using namespace Support2;

class IMediaProducer : public IInterface
{
public:
	B_DECLARE_META_INTERFACE(MediaProducer);

	virtual status_t Start(bool usethisthread = false) = 0;
	virtual status_t Stop() = 0;
	virtual status_t PushBuffer() = 0;
};

}; }; // namespace B::Media2
#endif //_MEDIA2_MEDIAPRODUCER_INTERFACE_
