
#include <support2/atomic.h>
#include <support2/Binder.h>
#include <support2/Parcel.h>
#include <support2/StdIO.h>
#include <support2/SupportDefs.h>
#include <support2/TLS.h>
#include <support2/HashTable.h>
#include <support2/Bitfield.h>
#include <support2/TokenSpace.h>
#include <support2/Looper.h>
#include <support2/StringIO.h>
#include <support2_p/BinderKeys.h>

#include <errno.h>
#include <stdio.h>

//#define RATOM_DEBUG_MSGS 1
//#define BINDER_DEBUG_MSGS 1

namespace B {
namespace Support2 {

using namespace B::Private;


struct EffectCache {
	const BValue *in;
	const BValue *inBindings;
	const BValue *outBindings;
	BValue *out;
};

#define EFFECT_CACHE ((EffectCache*)tls_get(gBinderTLS))

/**************************************************************************************/

IBinder::ptr
IInterface::AsBinder()
{
	return this ? AsBinderImpl() : NULL;
}

IBinder::ptr
IInterface::AsBinderImpl()
{
	return NULL;
}

IBinder::const_ptr
IInterface::AsBinder() const
{
	return this ? AsBinderImpl() : NULL;
}

IBinder::const_ptr
IInterface::AsBinderImpl() const
{
	return NULL;
}

/**************************************************************************************/

status_t 
IBinder::Put(const BValue &in)
{
	return Effect(in,BValue::null,BValue::undefined,NULL);
}

BValue 
IBinder::Get(const BValue &bindings) const
{
	BValue out;
	const_cast<IBinder*>(this)->Effect(BValue::undefined,BValue::undefined,bindings,&out);
#if BINDER_DEBUG_MSGS
	berr << "IBinder::Get " << out << endl;
#endif
	return out;
}

BValue 
IBinder::Get() const
{
	return Get(BValue::null);
}

BValue 
IBinder::Invoke(const BValue &in)
{
	BValue out;
	Effect(in,BValue::null,BValue::null,&out);
	return out;
}

BValue 
IBinder::Invoke(const BValue &in, const BValue &binding)
{
	BValue out;
	Effect(BValue(binding,in),BValue::null,binding,&out);
	return out;
}

/**************************************************************************************/

BBinder::BBinder()
{
	m_extensions = NULL;
}

BBinder::~BBinder()
{
	if (m_extensions) delete m_extensions;
}

void 
BBinder::BeginEffectContext(const EffectCache &cache)
{
	tls_set(gBinderTLS,const_cast<EffectCache*>(&cache));
}

void
BBinder::EndEffectContext()
{
	tls_set(gBinderTLS,NULL);
}

BValue 
BBinder::Inspect(const BValue &, uint32 flags)
{
	return BValue::undefined;
}

status_t
BBinder::Link(const IBinder::ptr &node, const BValue &binding)
{
	status_t err = B_OK;
	extensions *e;

	if (!m_extensions) {
		e = new extensions;
		if (!compare_and_swap32(reinterpret_cast<volatile int32*>(&m_extensions),
								0,
								reinterpret_cast<int32>(e))) {
			delete e;
		}
	}
	
	e = m_extensions;
	e->lock.Lock();
	
	BValue senderKey, targetKey;
	void * cookie = NULL;
	bool found;
	while (B_OK == binding.GetNextItem(&cookie, &senderKey, &targetKey)) {
		BVector<links_rec> &linkRec = e->links.EditValueFor(senderKey, &found);
		if (found) {
			linkRec.AddItem(links_rec(targetKey, node));
		} else {
			BVector<links_rec> vec;
			vec.AddItem(links_rec(targetKey, node));
			e->links.AddItem(senderKey, vec);
		}
	}
	
	e->lock.Unlock();
	
	return err;
	
}

status_t 
BBinder::Unlink(const IBinder::ptr &node, const BValue &binding)
{
	if (!m_extensions) return B_ERROR;
	status_t err = B_OK;
	uint32 i, count;

	extensions *e = m_extensions;
	e->lock.Lock();

	BValue senderKey, targetKey;
	void * cookie = NULL;
	bool found;
	while (B_OK == binding.GetNextItem(&cookie, &senderKey, &targetKey)) {
		BVector<links_rec> &linkRec = e->links.EditValueFor(senderKey, &found);
		if (found) {
			links_rec r(targetKey, node);
			count = linkRec.CountItems();
			for (i=count; i>0; i--) {
				if (linkRec[i-1] == r) {
					linkRec.RemoveItemsAt(i-1);
				}
			}
			if (linkRec.CountItems() == 0) {
				e->links.RemoveItemsAt(e->links.IndexOf(senderKey));
			}
		}
	}
	
	e->lock.Unlock();
	
	return err;
}

status_t 
BBinder::Effect(const BValue &in, const BValue &inBindings, const BValue &outBindings, BValue *out)
{
	EffectCache cache;

	// Remember any previous effect cache context.
	EffectCache *lastCache = (EffectCache*)tls_get(gBinderTLS);
	
	if (out) {
		// Start a new effect cache context where Push() should go.
		cache.in = &in;
		cache.inBindings = &inBindings;
		cache.outBindings = &outBindings;
		cache.out = out;
		BeginEffectContext(cache);
	} else {
		// We are not expecting any outputs to Push() in to, but we don't
		// want a Push() call here to modify the caller's effect cache.
		EndEffectContext();
	}
	
	BValue told = inBindings * in;
	if (told) {
		Told(told);
		if (told) {
			if (m_extensions) {
				m_extensions->lock.Lock();
				berr << "searching for inheritance for tell" << endl;
				// ... tell inherited nodes
				m_extensions->lock.Unlock();
			}
		}
	}

	if (told && out) {
		if (outBindings - *out) {
			Called(told,outBindings,*out);
			if (outBindings - *out) {
				if (m_extensions) {
					m_extensions->lock.Lock();
					berr << "searching for inheritance for call" << endl;
					m_extensions->lock.Unlock();
				}
			}
		}
	}

#if BINDER_DEBUG_MSGS
	berr << "BBinder::Effect " << endl
		<< indent
			<< outBindings << endl
			<< (out?*out:BValue::null) << endl
		<< dedent;
#endif

	if (out) {
		if (outBindings - *out) {
			Asked(outBindings,*out);
			if (outBindings - *out) {
				if (m_extensions) {
					m_extensions->lock.Lock();
					berr << "searching for inheritance for ask" << endl;
					m_extensions->lock.Unlock();
				}
			}
		}
	}

	// Restore the previous effect context.
	if (lastCache) BeginEffectContext(*lastCache);
	else EndEffectContext();
	
	return B_OK;
}

BBinder*
BBinder::LocalBinder()
{
	return this;
}

Private::RBinder*
BBinder::RemoteBinder()
{
	return NULL;
}

status_t
BBinder::Transact(uint32 code, BParcel& data, BParcel* reply, uint32 /*flags*/)
{
	if (code == B_EFFECT_TRANSACTION) {
		BValue values[3];
		BValue replyValue;
		BValue val;
		
		ssize_t status = data.GetValues(3, values);
		if (status < B_OK) return status;
		
#if BINDER_DEBUG_MSGS
		BStringIO::ptr msg(new BStringIO);
		msg << "BBinder::Transact " << this << " {" << endl << indent;
		if (status >= 1) msg << "Value 1: " << values[0] << endl;
		if (status >= 2) msg << "Value 2: " << values[1] << endl;
		if (status >= 3) msg << "Value 3: " << values[2] << endl;
		msg << dedent << "}" << endl;
		msg->PrintAndReset(berr);
#endif

		if ((val=values[0][g_keySysInspect]).IsDefined())
			replyValue = Inspect(val[g_keyWhich], val[g_keyFlags].AsInt32());
		else if (status == 1)
			status = Effect(values[0],BValue::null,BValue::undefined,&replyValue);
		else if (status == 2)
			status = Effect(values[0],values[1],BValue::undefined,&replyValue);
		else if (status == 3)
			status = Effect(values[0],values[1],values[2],&replyValue);
	
		if (status >= B_OK && reply) {
			status = reply->SetValues(&replyValue, NULL);
#if BINDER_DEBUG_MSGS
			msg << "BBinder::Transact reply: " << endl << indent;
			msg << replyValue << endl << reply << endl << dedent;
			msg->PrintAndReset(berr);
#endif
			if (status >= B_OK) status = reply->Reply();
		}
		data.Free();
		return status >= B_OK ? B_OK : status;
	}
	
	return B_OK;
}

status_t 
BBinder::Told(BValue &)
{
	#warning Implement BBinder::Told()
	return B_UNSUPPORTED;
}

status_t 
BBinder::Asked(const BValue &, BValue &)
{
	#warning Implement BBinder::Asked()
	return B_UNSUPPORTED;
}

status_t
BBinder::Called(BValue &, const BValue &, BValue &)
{
	#warning Implement BBinder::Called()
	return B_UNSUPPORTED;
}

status_t 
BBinder::Push(const BValue &out)
{
	// If we're in Effect, add what we're pushing into our effect
	EffectCache *cache = (EffectCache*)tls_get(gBinderTLS);
	if (cache && cache->out) {
#if BINDER_DEBUG_MSGS
		berr << "BBinder::out " << out << endl;
		berr << "BBinder::outbindings " << cache->outBindings << " " << *cache->outBindings << endl;
#endif
		if (*cache->outBindings == BValue::null) {
			*cache->out += out;
#if BINDER_DEBUG_MSGS
			berr << "BBinder::Push " << out << endl;
			berr << "BBinder::Push2 " << *cache->out << endl;
#endif
		} else {
			*cache->out += *cache->outBindings * out;
#if BINDER_DEBUG_MSGS
			berr << "BBinder::Push3 " << *cache->out << endl;
#endif
		}
	}
	
	// Notify our links about this Push()
	// This really wants to be made asynchronous, because
	// the idea here is that you get a notification when something
	// happens or changes, but at this point, it might not be done
	// happening or changing, and it probably has some locks held
	// and deadlock will probably happen when the things that are
	// notified try to acquire these same locks.
	if (m_extensions) {
		BKeyedVector<IBinder::ptr, BValue> sendThese;
		
		m_extensions->lock.Lock();
		if (m_extensions->links.CountItems() > 0) {
			
			BValue senderKey, senderValue;
			void * cookie = NULL;
			bool found;
			while (B_OK == out.GetNextItem(&cookie, &senderKey, &senderValue)) {
				BVector<links_rec> & rec = m_extensions->links.EditValueFor(senderKey, &found);
				if (found) {
					size_t i, count = rec.CountItems();
					for (i=0; i<count; i++) {
						BValue & targetOut = sendThese.EditValueFor(rec[i].target, &found);
						if (found) {
							targetOut.Overlay(rec[i].value, senderValue);
						} else {
							sendThese.AddItem(rec[i].target, BValue(rec[i].value, senderValue));
						}
					}
				}
			}
		}
		m_extensions->lock.Unlock();
		
		size_t i, count = sendThese.CountItems();
		for (i=0; i<count; i++) {
			BValue dummiResult;
			IBinder::ptr target = sendThese.KeyAt(i);
			const BValue & value = sendThese.ValueAt(i);
			target->Effect(value, BValue::null, BValue::undefined, &dummiResult);
		}
		
	}
		
	return B_OK;
}

status_t 
BBinder::Pull(BValue *)
{
	#warning Implement BBinder::Pull()
	return B_UNSUPPORTED;
}

/**************************************************************************************/

enum {
	// This is used to transfer ownership of the remote binder from
	// the RAtom object holding it (when it is constructed), to the
	// owner of the RAtom object when it first acquires that RAtom.
	kRemoteAcquired = 0x00000001
};

RAtom::RAtom(IBinder::arg o)
	:	m_remote(o.ptr()), m_state(0)
{
	if (m_remote) {
#if RATOM_DEBUG_MSGS
		printf("*** RAtom(): Acquire() and IncRefs() remote %p\n", m_remote);
#endif
		m_remote->Acquire(this);	// Removed on first Acquire().
		m_remote->IncRefs(this);	// Held for life of RAtom.
	}
}

RAtom::~RAtom()
{
	if (m_remote) {
		if (!(m_state&kRemoteAcquired)) {
#if RATOM_DEBUG_MSGS
			printf("*** ~RAtom(): Release() remote %p\n", m_remote);
#endif
			m_remote->Release(this);
		}
#if RATOM_DEBUG_MSGS
		printf("*** ~RAtom(): DecRefs() remote %p\n", m_remote);
#endif
		m_remote->DecRefs(this);
	}
}

status_t RAtom::Acquired(const void* /*id*/)
{
#if RATOM_DEBUG_MSGS
	printf("*** RAtom::Acquired(): Transfering acquire ownership from %p\n", m_remote);
#endif
	atomic_or(&m_state, kRemoteAcquired);
	return B_OK;
}

status_t RAtom::Released(const void* /*id*/)
{
	if (m_remote) {
#if RATOM_DEBUG_MSGS
		printf("*** RAtom::Released(): Calling Release() on %p\n", m_remote);
#endif
		m_remote->Release(this);
	}
	return B_ERROR;
}

status_t RAtom::AcquireAttempted(const void* /*id*/)
{
#if RATOM_DEBUG_MSGS
	printf("*** RAtom::AcquireAttempted() of %p\n", m_remote);
#endif
	return m_remote ? (m_remote->AttemptAcquire(this) ? B_OK : B_NOT_ALLOWED)
					: B_NOT_ALLOWED;
}

status_t RAtom::DeleteAtom(const void* /*id*/)
{
	return B_OK;
}

} }	// namespace B::Support2
