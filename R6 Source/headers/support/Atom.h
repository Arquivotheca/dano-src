
#ifndef _ATOM_H
#define _ATOM_H

#include <Gehnaphore.h>
#include <SupportDefs.h>

namespace std {
class type_info;
}
class BDataIO;

enum {
	B_ATOM_REPORT_REMOVE_HEADER		= 0x0001
};

class BAtom
{

	private:

				int32	m_primary;
				int32	m_secondary;

		virtual	void	_delete();

	protected:

		virtual	void	Cleanup();
		virtual	void	Acquired();

						BAtom();
		virtual			~BAtom();

	public:
				void	Report(BDataIO& io, const char* prefix="  ", uint32 flags=0) const;
				void	Report(const char* prefix="  ", uint32 flags=0) const;
				
		static	int32	MarkLeakReport();
		static	void	LeakReport(BDataIO& io, int32 mark=0, int32 last=-1);
		static	void	LeakReport(int32 mark=0, int32 last=-1);
		
		static	bool	ExistsAndAcquire(BAtom* atom);
		static	bool	ExistsAndIncRefs(BAtom* atom);
		
		static	void	StartWatching(const std::type_info* type);
		static	void	StopWatching(const std::type_info* type);

		const BAtom &	operator =(const BAtom &p) { return p; };

				int32	Acquire(void *id = NULL);
				int32	Release(void *id = NULL);
				int32 	IncRefs(void *id = NULL);
				int32 	DecRefs(void *id = NULL);
				
				bool	AttemptAcquire(void *id = NULL);
				
				// For debugging ONLY!
				int32	AcquireCount();
				int32	IncRefsCount();
				
				// Return true if Cleanup() has been (or is about
				// to be) called.
				bool	CleanupCalled() const;
};

template <class TYPE> class atom;
template <class TYPE> class atomref;

template <class TYPE>
class AtomPtr {
	private:
		TYPE *ptr;
		Gehnaphore lock;
		
		friend class atom<TYPE>;
		friend class atomref<TYPE>;

		TYPE * Acquire(void *id) {
			lock.Lock();
			TYPE *t = ptr;
			if (t) t->Acquire(id);
			lock.Unlock();
			return t;
		};

		TYPE * AcquireRef(void *id) {
			lock.Lock();
			TYPE *t = ptr;
			if (t) t->Acquire(id);
			lock.Unlock();
			return t;
		};

	public:
		AtomPtr() { ptr = NULL; };
		AtomPtr(TYPE *p) { ptr = p; ptr->Acquire(this); };
		~AtomPtr() { if (ptr) ptr->Release(this); };

		atom<TYPE> Ptr() {
			atom<TYPE> a = *this;
			return a;
		}

		AtomPtr<TYPE> &operator =(AtomPtr<TYPE> &p) {
			TYPE *safe = NULL;
			p.lock.Lock();
			TYPE *t = p.ptr;
			if (t) t->Acquire(this);
			p.lock.Unlock();
			lock.Lock();
			if (ptr!=t) {
				safe = ptr;
				ptr = t;
			} else if (t)
				safe = t;
			lock.Unlock();
			if (safe) safe->Release(this);
			return *this;
		};

		AtomPtr<TYPE> &operator =(TYPE *p) {
			TYPE *safe = NULL;
			lock.Lock();
			if (ptr!=p) {
				safe = ptr;
				ptr = p;
				if (ptr) ptr->Acquire(this);
			};
			lock.Unlock();
			if (safe) safe->Release(this);
			return *this;
		};
};

template <class TYPE>
class AtomRef {
	private:
		TYPE *ptr;
		Gehnaphore lock;

		friend class atom<TYPE>;
		friend class atomref<TYPE>;

		TYPE * AcquireRef(void *id) {
			lock.Lock();
			TYPE *t = ptr;
			if (t) t->IncRefs(id);
			lock.Unlock();
			return t;
		};

	public:
		AtomRef() { ptr = NULL; };
		AtomRef(TYPE *p) { ptr = p; ptr->IncRefs(this); };
		~AtomRef() { if (ptr) ptr->DecRefs(this); };

		AtomRef<TYPE> &operator =(AtomRef<TYPE> &p) {
			TYPE *safe = NULL;
			p.lock.Lock();
			TYPE *t = p.ptr;
			if (t) t->IncRefs(this);
			p.lock.Unlock();
			lock.Lock();
			if (ptr!=t) {
				safe = ptr;
				ptr = t;
			} else if (t)
				safe = t;
			lock.Unlock();
			if (safe) safe->DecRefs(this);
			return *this;
		};

		AtomRef<TYPE> &operator =(TYPE *p) {
			TYPE *safe = NULL;
			lock.Lock();
			if (ptr!=p) {
				safe = ptr;
				ptr = p;
				if (ptr) ptr->IncRefs(this);
			};
			lock.Unlock();
			if (safe) safe->DecRefs(this);
			return *this;
		};
};

template <class TYPE>
class atomref {
	private:
		TYPE *ptr;
	public:
		bool operator !=(void *p) { return ptr != p; };
		bool operator ==(const atomref<TYPE> &p) { return ptr == p.ptr; };
		operator TYPE*() const { return ptr; };
		operator atomref<BAtom>() const { return atomref(ptr); };
		TYPE * operator ->() const { return ptr; };
		const atomref<TYPE> &operator =(TYPE *p) {
			if (ptr!=p) {
				if (p) p->IncRefs(this);
				if (ptr) ptr->DecRefs(this);
				ptr = p;
			};
			return *this;
		};
		const atomref<TYPE> &operator =(const atom<TYPE> &p) {
			return ((*this) = (TYPE*)p);
		};
		const atomref<TYPE> &operator =(const atomref<TYPE> &p) {
			return ((*this) = p.ptr);
		};

		const atomref<TYPE> &operator =(AtomPtr<TYPE> &p) {
			TYPE *t = p.AcquireRef(this);
			if (ptr) ptr->DecRefs(this);
			ptr = t;
			return *this;
		};
		const atomref<TYPE> &operator =(AtomRef<TYPE> &p) {
			TYPE *t = p.AcquireRef(this);
			if (ptr) ptr->DecRefs(this);
			ptr = t;
			return *this;
		};

		atomref() { ptr = NULL; };
		atomref(AtomPtr<TYPE> &p) { ptr = NULL; *this = p; };
		atomref(AtomRef<TYPE> &p) { ptr = NULL; *this = p; };
		atomref(const atom<TYPE> &p) { ptr = (TYPE*)p; if (ptr) ptr->IncRefs(this); };
		atomref(const atomref<TYPE> &p) { ptr = p.ptr; if (ptr) ptr->IncRefs(this); };
		atomref(TYPE *p) { ptr = p; if (ptr) ptr->IncRefs(this); };
		~atomref() { if (ptr) ptr->DecRefs(this); };
};

template <class TYPE>
class atom {
	private:
		TYPE *ptr;
	public:
		bool operator !=(int p) { return ((int)ptr) != p; };
		bool operator ==(int p) { return ((int)ptr) == p; };
		operator TYPE*() const { return ptr; };
		operator atom<BAtom>() const { return atom<BAtom>(ptr); };
		operator atomref<BAtom>() const { return atomref<BAtom>(ptr); };
		TYPE * operator ->() const { return ptr; };
		const atom<TYPE> &operator =(TYPE *p) {
			if (ptr!=p) {
				if (p) p->Acquire(this);
				if (ptr) ptr->Release(this);
				ptr = p;
			};
			return *this;
		};
		const atom<TYPE> &operator =(const atom<TYPE> &p) {
			return ((*this) = p.ptr);
		};
		const atom<TYPE> &operator =(AtomPtr<TYPE> &p) {
			TYPE *t = p.Acquire(this);
			if (ptr) ptr->Release(this);
			ptr = t;
			return *this;
		};

		atom() { ptr = NULL; };
		atom(AtomPtr<TYPE> &p) { ptr = NULL; *this = p; };
		atom(const atom<TYPE> &p) { ptr = p.ptr; if (ptr) ptr->Acquire(this); };
		atom(TYPE *p) { ptr = p; if (ptr) ptr->Acquire(this); };
		~atom() { if (ptr) ptr->Release(this); };
};

#endif
