#include <media2/MediaProducer.h>

#include <OS.h>
#include <support2/Autolock.h>
#include <support2/Debug.h>
#include <support2/StdIO.h>

#define checkpoint \
berr << "thid " << find_thread(0) << " LMediaProducer(" << this << ") -- " << __FILE__ << ":" << __LINE__ << " -- " << __FUNCTION__ << endl;

using B::Support2::BAutolock;

namespace B {
namespace Media2 {

class RMediaProducer : public RInterface<IMediaProducer>
{
	friend class LMediaProducer;
	
	static const BValue kKeyStart;
	static const BValue kKeyStartUseThisThread;
	static const BValue kKeyStartResult;
	
	static const BValue kKeyStop;
	static const BValue kKeyStopResult;

	static const BValue kKeyPushBuffer;
	static const BValue kKeyPushBufferResult;
	
public:
	RMediaProducer(const IBinder::ptr & binder) : RInterface<IMediaProducer>(binder) {}

	virtual status_t Start(bool usethisthread = false)
	{
		return Remote()->Invoke(BValue()
			.Overlay(kKeyStart,BValue::Bool(true))
			.Overlay(kKeyStartUseThisThread,BValue::Bool(usethisthread)))
			[kKeyStartResult].AsInt32();
	}
	
	virtual status_t Stop()
	{
		return Remote()->Invoke(BValue()
			.Overlay(kKeyStop,BValue::Bool(true)))
			[kKeyStopResult].AsInt32();
	}
	
	virtual status_t PushBuffer()
	{
		return Remote()->Invoke(BValue()
			.Overlay(kKeyPushBuffer,BValue::Bool(true)))
			[kKeyPushBufferResult].AsInt32();
	}
};

const BValue RMediaProducer::kKeyStart("Start");
const BValue RMediaProducer::kKeyStartUseThisThread("StartUseThisThread");
const BValue RMediaProducer::kKeyStartResult("StartResult");

const BValue RMediaProducer::kKeyStop("Stop");
const BValue RMediaProducer::kKeyStopResult("StopResult");

const BValue RMediaProducer::kKeyPushBuffer("PushBuffer");
const BValue RMediaProducer::kKeyPushBufferResult("PushBufferResult");

B_IMPLEMENT_META_INTERFACE(MediaProducer);

/* ******************************************************************* */

status_t 
LMediaProducer::Called(BValue &in, const BValue &outBindings, BValue &out)
{
	if (in[RMediaProducer::kKeyStart].IsDefined())
	{
		status_t err = Start(in[RMediaProducer::kKeyStartUseThisThread].AsBool());
		out += outBindings * BValue(RMediaProducer::kKeyStartResult, BValue::Int32(err));
	} 
	
	if (in[RMediaProducer::kKeyStop].IsDefined())
	{
		status_t err = Stop();
		out += outBindings * BValue(RMediaProducer::kKeyStopResult, BValue::Int32(err));
	}

	if (in[RMediaProducer::kKeyPushBuffer].IsDefined())
	{
		status_t err = PushBuffer();
		out += outBindings * BValue(RMediaProducer::kKeyPushBufferResult, BValue::Int32(err));
	}
	
	return B_OK;
}

/* ******************************************************************* */

BMediaProducer::BMediaProducer() :
	fRunning(false)
{
}


status_t BMediaProducer::Start(bool usethisthread)
{
	if(fRunning)
		return B_NOT_ALLOWED;

	if(usethisthread)
	{
		looper=find_thread(NULL);
		fRunning = true;
		return RunLoop();
	}
	else
	{
		looper=spawn_thread(startloop,"producer-looper",B_REAL_TIME_PRIORITY,this);
		if(looper>=0)
		{
			fRunning = true;
			resume_thread(looper);
			return B_OK;
		}
		return looper;
	}
}

int32 BMediaProducer::startloop(void *arg)
{
	return ((BMediaProducer*)arg)->RunLoop();
}

int32 BMediaProducer::RunLoop()
{
	while(fRunning)
		PushBuffer();
		
	return B_OK;
}

status_t BMediaProducer::Stop()
{
	if(!fRunning)
		return B_NOT_ALLOWED;

	fRunning = false;
	if(looper != find_thread(NULL))
	{
		status_t status;
		wait_for_thread(looper,&status);
	}
	return B_OK;
}

} } // B::Media2
