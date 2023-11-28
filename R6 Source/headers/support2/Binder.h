
#ifndef _SUPPORT2_BINDER_H_
#define _SUPPORT2_BINDER_H_

#include <support2/IBinder.h>
#include <support2/SupportDefs.h>
#include <support2/Value.h>
#include <support2/KeyedVector.h>
#include <support2/Autolock.h>

namespace B {
namespace Support2 {

/**************************************************************************************/

enum {
	B_SYNCHRONOUS_TRANSACTION = 0x00000001
};

class BBinder : public IBinder
{
	public:
		virtual	BValue					Inspect(const BValue &which,
												uint32 flags = 0);
		virtual	status_t				Link(	const IBinder::ptr &to,
												const BValue &bindings);
		virtual	status_t				Unlink(	const IBinder::ptr &from,
												const BValue &bindings);
		virtual	status_t				Effect(	const BValue &in,
												const BValue &inBindings,
												const BValue &outBindings,
												BValue *out);

		virtual	status_t				Transact(	uint32 code,
													BParcel& data,
													BParcel* reply = NULL,
													uint32 flags = 0);
		
		virtual	BBinder*				LocalBinder();
		virtual	B::Private::RBinder*	RemoteBinder();
		
	protected:
										BBinder();
		virtual							~BBinder();

		// These functions are called in response to Put(), Get(), and
		// Invoke(), respectively.
		virtual	status_t				Told(BValue &in);
		virtual	status_t				Asked(const BValue &outBindings, BValue &out);
		virtual	status_t				Called(	BValue &in,
												const BValue &outBindings,
												BValue &out);
		
		virtual	status_t				Push(const BValue &out);
		virtual	status_t				Pull(BValue *inMaskAndDefaultsOutData);

	private:
										BBinder(const BBinder& o);

				void					BeginEffectContext(const struct EffectCache &cache);
				void					EndEffectContext();

		static	int32					gBinderTLS;
		
		struct links_rec {
			links_rec() {};
			links_rec(const links_rec &orig) :value(orig.value), target(orig.target) {};
			links_rec(BValue & k, const IBinder::ptr &t) :value(k), target(t) {};
			bool operator==(const links_rec & other) const {return value == other.value && target == other.target;}
			
			BValue value;
			IBinder::ptr target;
		};

		struct extensions {
			BLocker lock;
			BKeyedVector<BValue,BVector<links_rec> > links;
		};

		extensions *	m_extensions;
};

/**************************************************************************************/

// This is the base implementation for a local IInterface.
template<class INTERFACE>
class LInterface : public INTERFACE, public BBinder
{
	public:
		virtual	BValue					Inspect(const BValue &which, uint32 flags = 0)
			{ (void)flags; return which * BValue(descriptor,BValue::Binder(this)); };
	
	protected:
		inline							LInterface() { }
		inline virtual					~LInterface() { }
		
		virtual	IBinder::ptr			AsBinderImpl()			{ return this; }
		virtual	IBinder::const_ptr		AsBinderImpl() const	{ return this; }
};

/**************************************************************************************/

// This is the BAtom protocol implementation for a remote interface.  You
// don't normally use this directly -- it is included as part of the
// RInterface<> implementation below.
class RAtom : public virtual BAtom
{
	protected:
										RAtom(IBinder::arg o);
		virtual							~RAtom();
		virtual	status_t				Acquired(const void* id);
		virtual	status_t				Released(const void* id);
		virtual	status_t				AcquireAttempted(const void* id);
		virtual	status_t				DeleteAtom(const void* id);
		
		inline	IBinder*				Remote()				{ return m_remote; }
		// NOTE: This removes constness from the remote binder.
		inline	IBinder*				Remote() const			{ return m_remote; }
				
	private:
				IBinder* const			m_remote;
				int32					m_state;
				size_t					_reserved;
};

/**************************************************************************************/

// This is the base implementation for a remote IInterface.
template<class INTERFACE>
class RInterface : public INTERFACE, public RAtom
{
	protected:
		inline							RInterface(IBinder::arg o)	: RAtom(o) { }
		inline virtual					~RInterface() { }
		
		virtual	IBinder::ptr			AsBinderImpl()			{ return IBinder::ptr(Remote()); }
		virtual	IBinder::const_ptr		AsBinderImpl() const	{ return IBinder::const_ptr(Remote()); }
};

/**************************************************************************************/

} } // namespace B::Support2

#endif	/* _SUPPORT2_BINDER_H_ */
