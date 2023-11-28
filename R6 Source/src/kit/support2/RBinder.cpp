
#include <stdio.h>
#include <support2/Looper.h>
#include <support2/StdIO.h>
#include <support2/StringIO.h>
#include <support2/Parcel.h>
#include <support2_p/BinderIPC.h>
#include <support2_p/BinderKeys.h>

//#define RBINDER_DEBUG_MSGS 1
//#define BINDER_DEBUG_MSGS 1

namespace B {
namespace Support2 {

using namespace B::Private;

RBinder::RBinder(int32 handle)
{
	m_handle = handle;
#if RBINDER_DEBUG_MSGS
	printf("*** RBinder(): IncRefs %p descriptor %ld\n", this, m_handle);
#endif
	BLooper::This()->IncrefsHandle(m_handle);
}

RBinder::~RBinder()
{
#if RBINDER_DEBUG_MSGS
	printf("*** ~RBinder(): DecRefs %p descriptor %ld\n", this, m_handle);
#endif
	BLooper::ExpungeHandle(m_handle);
	BLooper::This()->DecrefsHandle(m_handle);
}

status_t 
RBinder::Link(const IBinder::ptr &, const BValue &)
{
	#warning Implement RBinder::Link()
	return B_UNSUPPORTED;
}

status_t 
RBinder::Unlink(const IBinder::ptr &, const BValue &)
{
	#warning Implement RBinder::Unlink()
	return B_UNSUPPORTED;
}

BValue 
RBinder::Inspect(const BValue &which, uint32 flags)
{
	BValue args(g_keyWhich, which);
	if (flags) args.Overlay(g_keyFlags, flags);
	return Invoke(args, g_keySysInspect);
}

status_t 
RBinder::Effect(const BValue &in, const BValue &inBindings, const BValue &outBindings, BValue *out)
{
	BParcel buffer;
	BParcel reply;
	
	const BValue *outBindingsP = outBindings ? &outBindings : NULL;
	const BValue *inBindingsP = (outBindingsP || (!inBindings.IsNull())) ? &inBindings : NULL;
	ssize_t result = buffer.SetValues(&in,inBindingsP,outBindingsP,NULL);
	if (result >= B_OK) {
#if BINDER_DEBUG_MSGS
		BStringIO::ptr msg(new BStringIO);
		msg << "RBinder::Effect " << buffer << endl;
		msg->PrintAndReset(berr);
#endif
		result = Transact(B_EFFECT_TRANSACTION,buffer,out ? &reply : NULL,0);
		if (result >= B_OK && out) reply.GetValues(1, out);
#if BINDER_DEBUG_MSGS
		msg << "RBinder::Effect reply {" << endl << indent
			<< reply << endl;
		if (out) msg << *out << endl;
		msg << dedent;
		msg->PrintAndReset(berr);
#endif
	}
	return result;
}

status_t
RBinder::Transact(uint32 code, BParcel& data, BParcel* reply, uint32 flags)
{
	return BLooper::This()->Transact(m_handle, code, data, reply, flags);
}

BBinder*
RBinder::LocalBinder()
{
	return NULL;
}

RBinder*
RBinder::RemoteBinder()
{
	return this;
}

status_t
RBinder::Acquired(const void* )
{
#if RBINDER_DEBUG_MSGS
	printf("*** RBinder::Acquired(): Acquire %p descriptor %ld\n", this, m_handle);
#endif
	BLooper::This()->AcquireHandle(m_handle);
	return B_OK;
}

status_t
RBinder::Released(const void* )
{
#if RBINDER_DEBUG_MSGS
	printf("*** RBinder::Released(): Release %p descriptor %ld\n", this, m_handle);
#endif
	BLooper::This()->ReleaseHandle(m_handle);
	return B_ERROR;
}

status_t
RBinder::AcquireAttempted(const void* )
{
#if RBINDER_DEBUG_MSGS
	printf("*** RBinder::AcquireAttempted(): AttemptAcquire %p descriptor %ld\n", this, m_handle);
#endif
	return BLooper::This()->AttemptAcquireHandle(m_handle);
}

} }	// namespace B::Support2
