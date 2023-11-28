/***************************************************************************
//
//	File:			support2/BAtom.h
//
//	Description:	BAtom is a reference counted base class.
//					atom_ptr<> is a template class that looks like a
//					pointer to an atom and holds a primary reference on it.
//					atom_ref<> is a template class that looks like a
//					pointer to an atom and holds a secondary reference on it.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _SUPPORT2_ATOM_H
#define _SUPPORT2_ATOM_H

#include <support2/SupportDefs.h>
#include <typeinfo>

namespace std {
	struct nothrow_t;
}

namespace B { namespace Private { struct atom_debug; } }

namespace B {
namespace Support2 {

class ITextOutput;
class BAtomTracker;

/**************************************************************************************/

//!	Use this macro inside of a class to create standard atom_ptr<> and
//!	atom_ref<> typedefs.
#define B_STANDARD_ATOM_TYPEDEFS(cname)												\
		typedef	::B::Support2::atom_ref<cname>	ref;								\
		typedef	::B::Support2::atom_ref<const cname>			const_ref;			\
		typedef	::B::Support2::atom_ptr<cname>					ptr;				\
		typedef	::B::Support2::atom_ptr<const cname>			const_ptr;			\
		typedef	const ::B::Support2::atom_ptr<cname> &			arg;				\
		typedef	const ::B::Support2::atom_ptr<const cname> &	const_arg;			\

/**************************************************************************************/

//!	Flags for BAtom debugging reports
enum {
	B_ATOM_REPORT_FORCE_LONG		= 0x0001,	//!< Print full stack crawls.
	B_ATOM_REPORT_FORCE_SHORT		= 0x0002,	//!< Print one-line stack crawls.
	B_ATOM_REPORT_FORCE_SUMMARY		= 0x0003,	//!< Print only holder summary.
	B_ATOM_REPORT_FORCE_MASK		= 0x000f,	//!< Mask for report type.
	
	B_ATOM_REPORT_REMOVE_HEADER		= 0x0010	//!< Don't print atom header.
};

//!	Base class for a reference-counted object.
class BAtom
{
public:
			//!	BAtom must use its own new and delete operators.
			//@{
			void*			operator new(size_t size);
			void*			operator new(size_t size, const std::nothrow_t&);
			void			operator delete(void* ptr, size_t size);
			//@}
			
			//!	Acquire and release the atom's primary reference.
			//@{
			int32			Acquire(const void* id = NULL) const;
			int32			Release(const void* id = NULL) const;
			//@}
			
			//! Increment and decrement the secondary reference count.
			//@{
			int32		 	IncRefs(const void* id = NULL) const;
			int32		 	DecRefs(const void* id = NULL) const;
			//@}
			
			//!	Try to acquire a primary reference on this atom.
			bool			AttemptAcquire(const void* id = NULL) const;
	
			//!	Perform a Release() if there an primary references remaining.
			bool			AttemptRelease(const void* id = NULL) const;
			
			//!	Acquire an atom's primary reference, even if it doesn't have one.
			int32			ForceAcquire(const void* id = NULL) const;
			
			//	************************ DEBUGGING SUPPORT ***********************
			//	******* DO NOT CALL THE REMAINING FUNCTIONS IN NORMAL CODE *******
			
			//!	Return the number of primary/secondary references currently
			//!	on the atom.
			//@{
			int32			AcquireCount() const;
			int32			IncRefsCount() const;
			//@}
			
			//	The remaining functions are only available when BAtom is built for
			//	debugging.
			
			//!	Print information state and references on this atom to \a io.
			void			Report(const atom_ptr<ITextOutput>& io, uint32 flags=0) const;
			
			//	BAtom leak tracking.
			
			//!	Start a new BAtom leak context and returns its identifier.
	static	int32			MarkLeakReport();
	
			//!	Print information about atoms in a leak context.
			//@{
	static	void			LeakReport(	const atom_ptr<ITextOutput>& io, int32 mark=0, int32 last=-1,
										uint32 flags=0);
	static	void			LeakReport(	int32 mark=0, int32 last=-1,
										uint32 flags=0);
			//@}
			
			//!	Check for atom existance and acquire primary reference.
	static	bool			ExistsAndAcquire(BAtom* atom);
	static	bool			ExistsAndIncRefs(BAtom* atom);
			
			//!	Register a particular class type for watching atom operations.
			//@{
	static	void			StartWatching(const std::type_info* type);
	static	void			StopWatching(const std::type_info* type);
			//@}
	
protected:
							BAtom();
	virtual					~BAtom();
	
			//!	Called the first time an atom is acquired.
	virtual	status_t		Acquired(const void* id);
	
			//!	Called the last time an atom is released.
	virtual	status_t		Released(const void* id);
	
			//!	Called during AttemptAcquire() after an atom has been released.
	virtual	status_t		AcquireAttempted(const void* id);
	
			//!	Called after last DecRefs() when the life of an atom is extended.
	virtual	status_t		DeleteAtom(const void* id);

private:
			friend class	BAtomTracker;
			
							BAtom(const BAtom&);
			
			void			destructor_impl();
			void			delete_impl(size_t size);
			
			// ----- private debugging support -----
			
			void			init_atom();
			void			term_atom();
			
			void			lock_atom() const;
			void			unlock_atom() const;
			
			int32*			primary_addr() const;
			int32*			secondary_addr() const;
			
			int32			primary_count() const;
			int32			secondary_count() const;
			
			void			watch_action(const char* description) const;
			void			do_report(const atom_ptr<class ITextOutput>& io, uint32 flags) const;
			
			void			add_acquire(const void* id) const;
			void			add_release(const void* id) const;
			void			add_increfs(const void* id) const;
			void			add_decrefs(const void* id) const;
	
			struct base_data {
				BAtom* atom;
				size_t size;
			};
			
			base_data*		fBase;
			union {
				mutable	int32					fPrimary;
				mutable	Private::atom_debug*	fDebugPtr;
			};
	mutable	int32			fSecondary;
};

/**************************************************************************************/

// forward reference
template <class TYPE> class atom_ref;
template <class TYPE> class safe_ptr;
template <class TYPE> class safe_ref;

//!	This is a smart-pointer template for BAtom-derived classes that
//!	maintains a primary reference on the object.
template <class TYPE>
class atom_ptr {
public:
		atom_ptr();
		atom_ptr(const atom_ptr<TYPE>& p);
		atom_ptr(TYPE* p);
		~atom_ptr();

		// Conversion to other atom types.
		template <class NEWTYPE> operator atom_ptr<NEWTYPE>() const;
		template <class NEWTYPE> operator atom_ref<NEWTYPE>() const;
		
		// Access to raw pointer.
		TYPE& operator *() const;
		TYPE* operator ->() const;
		
		TYPE* ptr() const;
		TYPE* detach() const;
		
		// Assignment.
		const atom_ptr<TYPE> &operator =(TYPE* p);
		const atom_ptr<TYPE> &operator =(const atom_ptr<TYPE>& p);

		// Give comparison operators access to our pointer.
		#define COMPARE_FRIEND(op)									\
			bool operator op (const TYPE* p2) const					\
				{ return fPtr op p2; }								\
			friend bool operator op <>(	const atom_ptr<TYPE>& p,	\
										const atom_ptr<TYPE>& p);	\
			friend bool operator op <>(	const atom_ref<TYPE>& p,	\
										const atom_ptr<TYPE>& p);	\
			friend bool operator op <>(	const atom_ptr<TYPE>& p,	\
										const atom_ref<TYPE>& p);	\
		
		COMPARE_FRIEND(==);
		COMPARE_FRIEND(!=);
		COMPARE_FRIEND(<=);
		COMPARE_FRIEND(<);
		COMPARE_FRIEND(>);
		COMPARE_FRIEND(>=);
		
		#undef COMPARE_FRIEND

private:
		friend class atom_ref<TYPE>;
		friend class safe_ptr<TYPE>;
		friend class safe_ref<TYPE>;
		
		TYPE *fPtr;
};

/**************************************************************************************/

//!	This is a smart-pointer template for BAtom-derived classes that
//!	maintains a secondary reference on the object.
template <class TYPE>
class atom_ref {
public:
		atom_ref();
		atom_ref(const atom_ptr<TYPE>& p);
		atom_ref(const atom_ref<TYPE>& p);
		atom_ref(TYPE* p);
		~atom_ref();
		
		// Conversion to other atom types.
		template <class NEWTYPE> operator atom_ref<NEWTYPE>() const;
		
		// Assignment.
		const atom_ref<TYPE> &operator =(TYPE* p);
		const atom_ref<TYPE> &operator =(const atom_ptr<TYPE>& p);
		const atom_ref<TYPE> &operator =(const atom_ref<TYPE>& p);

		// Attempt to promote this secondary reference to a primary reference.
		// The returned atom_ptr<> will be NULL if it failed.
		const atom_ptr<TYPE> promote() const;
		
		// Give comparison operators access to our pointer.
		#define COMPARE_FRIEND(op)									\
			bool operator op (const TYPE* p2) const					\
				{ return fPtr op p2; }								\
			friend bool operator op <>(	const atom_ref<TYPE>& p,	\
										const atom_ref<TYPE>& p);	\
			friend bool operator op <>(	const atom_ref<TYPE>& p,	\
										const atom_ptr<TYPE>& p);	\
			friend bool operator op <>(	const atom_ptr<TYPE>& p,	\
										const atom_ref<TYPE>& p);	\
		
		COMPARE_FRIEND(==);
		COMPARE_FRIEND(!=);
		COMPARE_FRIEND(<=);
		COMPARE_FRIEND(<);
		COMPARE_FRIEND(>);
		COMPARE_FRIEND(>=);
		
		#undef COMPARE_FRIEND
		
		// Explicitly increment and decrement reference count of
		// this atom.  Should very rarely be used.
		int32 inc_refs() const;
		int32 dec_refs() const;
		
private:
		friend class atom_ptr<TYPE>;
		friend class safe_ptr<TYPE>;
		friend class safe_ref<TYPE>;
		
		TYPE *fPtr;
};

/**************************************************************************************/

// A zillion kinds of comparison operators.
#define COMPARE(op)														\
template<class TYPE> inline												\
bool operator op(const atom_ptr<TYPE>& p1, const atom_ptr<TYPE>& p2)	\
	{ return p1.fPtr op p2.fPtr; }										\
template<class TYPE> inline												\
bool operator op(const atom_ref<TYPE>& p1, const atom_ref<TYPE>& p2)	\
	{ return p1.fPtr op p2.fPtr; }										\
template<class TYPE> inline												\
bool operator op(const atom_ptr<TYPE>& p1, const atom_ref<TYPE>& p2)	\
	{ return p1.fPtr op p2.fPtr; }										\
template<class TYPE> inline												\
bool operator op(const atom_ref<TYPE>& p1, const atom_ptr<TYPE>& p2)	\
	{ return p1.fPtr op p2.fPtr; }										\

COMPARE(==);
COMPARE(!=);
COMPARE(<=);
COMPARE(<);
COMPARE(>);
COMPARE(>=);

#undef COMPARE

/*-------------------------------------------------------------*/
/*---- No user serviceable parts after this -------------------*/

/* ----------------- atom_ptr Implementation ------------------*/

template<class TYPE> inline
atom_ptr<TYPE>::atom_ptr()								{ fPtr = NULL; }
template<class TYPE> inline
atom_ptr<TYPE>::atom_ptr(const atom_ptr<TYPE>& p)		{ fPtr = p.fPtr; if (fPtr) fPtr->Acquire(this); }
template<class TYPE> inline
atom_ptr<TYPE>::atom_ptr(TYPE* p)						{ fPtr = p; if (fPtr) fPtr->Acquire(this); }
template<class TYPE> inline
atom_ptr<TYPE>::~atom_ptr()								{ if (fPtr) fPtr->Release(this); }

template<class TYPE> template<class NEWTYPE> inline
atom_ptr<TYPE>::operator atom_ptr<NEWTYPE>() const		{ return atom_ptr<NEWTYPE>(fPtr); }
template<class TYPE> template<class NEWTYPE> inline
atom_ptr<TYPE>::operator atom_ref<NEWTYPE>() const		{ return atom_ref<NEWTYPE>(fPtr); }
template<class TYPE> inline
TYPE & atom_ptr<TYPE>::operator *() const				{ return *fPtr; }
template<class TYPE> inline
TYPE * atom_ptr<TYPE>::operator ->() const				{ return fPtr; }
template<class TYPE> inline
TYPE * atom_ptr<TYPE>::ptr() const						{ return fPtr; }
template<class TYPE> inline
TYPE * atom_ptr<TYPE>::detach() const					{ return fPtr; fPtr = NULL; }

template<class TYPE> inline
const atom_ptr<TYPE> & atom_ptr<TYPE>::operator =(TYPE* p)
{
	if (p) p->Acquire(this);
	if (fPtr) fPtr->Release(this);
	fPtr = p;
	return *this;
}

template<class TYPE> inline
const atom_ptr<TYPE> & atom_ptr<TYPE>::operator =(const atom_ptr<TYPE>& p)
{
	return ((*this) = p.fPtr);
}

/* ----------------- atom_ref Implementation ------------------*/

template<class TYPE> inline
atom_ref<TYPE>::atom_ref()								{ fPtr = NULL; }
template<class TYPE> inline
atom_ref<TYPE>::atom_ref(const atom_ptr<TYPE>& p)		{ fPtr = p.fPtr; if (fPtr) fPtr->IncRefs(this); }
template<class TYPE> inline
atom_ref<TYPE>::atom_ref(const atom_ref<TYPE>& p)		{ fPtr = p.fPtr; if (fPtr) fPtr->IncRefs(this); }
template<class TYPE> inline
atom_ref<TYPE>::atom_ref(TYPE* p)						{ fPtr = p; if (fPtr) fPtr->IncRefs(this); }
template<class TYPE> inline
atom_ref<TYPE>::~atom_ref()								{ if (fPtr) fPtr->DecRefs(this); }

template<class TYPE> template<class NEWTYPE> inline
atom_ref<TYPE>::operator atom_ref<NEWTYPE>() const		{ return atom_ref<NEWTYPE>(fPtr); }

template<class TYPE> inline
const atom_ref<TYPE> & atom_ref<TYPE>::operator =(TYPE *p)
{
	if (p) p->IncRefs(this);
	if (fPtr) fPtr->DecRefs(this);
	fPtr = p;
	return *this;
}

template<class TYPE> inline
const atom_ref<TYPE> & atom_ref<TYPE>::operator =(const atom_ptr<TYPE> &p)
{
	return ((*this) = p.fPtr);
}

template<class TYPE> inline
const atom_ref<TYPE> & atom_ref<TYPE>::operator =(const atom_ref<TYPE> &p)
{
	return ((*this) = p.fPtr);
}

template<class TYPE> inline
const atom_ptr<TYPE> atom_ref<TYPE>::promote() const
{
	atom_ptr<TYPE> a;
	if (fPtr && fPtr->AttemptAcquire(this)) {
		a = fPtr;
		fPtr->Release(this);
	}
	return a;
}

template<class TYPE> inline
int32 atom_ref<TYPE>::inc_refs() const
{
	return fPtr ? fPtr->IncRefs() : 0;
}

template<class TYPE> inline
int32 atom_ref<TYPE>::dec_refs() const
{
	return fPtr ? fPtr->DecRefs() : 0;
}

} }	// namespace B::Support2

#endif /* _SUPPORT2_ATOM_H */
