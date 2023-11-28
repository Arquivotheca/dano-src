#ifndef _MEDIA2_MEDIAPRODUCER_INTERFACE_H_
#define _MEDIA2_MEDIAPRODUCER_INTERFACE_H_

#include <media2/IMediaProducer.h>
#include <support2/Binder.h>

namespace B {
namespace Media2 {

using namespace Support2;

class LMediaProducer : public LInterface<IMediaProducer>
{
public:
	virtual status_t				Called (BValue &in, const BValue &, BValue &);
};

class BMediaProducer : public LMediaProducer
{
public:
	B_STANDARD_ATOM_TYPEDEFS(BMediaProducer)
	
						BMediaProducer();
	virtual status_t	Start(bool usethisthread = false);
	virtual status_t	Stop();
	virtual status_t	PushBuffer() = 0;


private:
	thread_id			looper;
	static int32		startloop(void *arg);
	int32				RunLoop();
	volatile bool		fRunning;
};

}; }; // namespace B::Media2
#endif //_MEDIA2_MEDIAPRODUCER_INTERFACE_
