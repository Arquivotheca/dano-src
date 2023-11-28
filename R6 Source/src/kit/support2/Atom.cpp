
#include <support2/Atom.h>

#include <support2/atomic.h>
#include <kernel/OS.h>
#include <support2/SupportDefs.h>
#include <support2/StdIO.h>
#include <support2/TLS.h>
#include <support2/TypeConstants.h>
#include <support2/Debug.h>
#include <support2_p/SupportMisc.h>

#include <memory>
#include <stdio.h>
#include <typeinfo>

#define INITIAL_PRIMARY_VALUE (1<<28)

#if !SUPPORTS_ATOM_DEBUG

/*---------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------*/

/* No atom debugging -- just run as fast as possible. */

namespace B {
namespace Support2 {
	void BAtom::Report(ITextOutput::arg, uint32) const				{ }
	int32 BAtom::MarkLeakReport()									{ return 0; }
	void BAtom::LeakReport(ITextOutput::arg, int32, int32, uint32)	{ }
	void BAtom::LeakReport(int32, int32, uint32)					{ }

	bool BAtom::ExistsAndAcquire(BAtom*)							{ return false; }
	bool BAtom::ExistsAndIncRefs(BAtom*)							{ return false; }

	void BAtom::StartWatching(const std::type_info*)				{ }
	void BAtom::StopWatching(const std::type_info*)					{ }

	inline void BAtom::init_atom()									{ }
	inline void BAtom::term_atom()									{ }

	inline void BAtom::lock_atom() const							{ }
	inline void BAtom::unlock_atom() const							{ }
	
	inline int32* BAtom::primary_addr() const						{ return &fPrimary; }
	inline int32* BAtom::secondary_addr() const						{ return &fSecondary; }
	inline int32 BAtom::primary_count() const						{ return fPrimary; }
	inline int32 BAtom::secondary_count() const						{ return fSecondary; }
	
	inline void BAtom::watch_action(const char*) const				{ }
	inline void BAtom::do_report(ITextOutput::arg, uint32) const	{ }
	
	inline void BAtom::add_acquire(const void*) const				{ }
	inline void BAtom::add_release(const void*) const				{ }
	inline void BAtom::add_increfs(const void*) const				{ }
	inline void BAtom::add_decrefs(const void*) const				{ }
} }	// namespace B::Support2

#define NOTE_CREATE()
#define NOTE_DESTROY()
#define NOTE_FREE()
	
#else

/*---------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------*/

/* Implement BAtom debugging infrastructure */

#include <support2/Autolock.h>
#include <support2/Locker.h>
#include <support2/String.h>
#include <support2/StringIO.h>
#include <support2/CallStack.h>
#include <set>

namespace B {
namespace Private {
#if 0	
	// This custom IO stream class is only to ensure that the stream
	// still exists while our static objects are being destroyed.
	// (Since we are in the same library as StreamIO.h, there is no
	// guarantee about whose destructors will be called first.)
	class AtomDataIO : public CByteOutput, public BTextOutput {
	public:
		AtomDataIO(FILE* fh) : BTextOutput(this), fFile(fh)
		{
		}
		
		virtual ~AtomDataIO()
		{
		}
		
		virtual ssize_t WriteV(const void *buffer, size_t size);
		virtual void Print(const char *buffer, int32 len) {
			Write(buffer,len);
		}
	
	private:
		FILE* fFile;
	};
	
	ssize_t AtomDataIO::Write(const void *buffer, size_t size)
	{
		ssize_t iosize = fwrite(buffer, size, 1, fFile);
		status_t err = ferror(fFile);
		if (err != B_OK) return err;
		return iosize;
	}
	static atom_ptr<AtomDataIO> AtomIO(new AtomDataIO(stderr));
	#endif
	#define AtomIO berr
	static int32 gHasDebugLevel = 0;
	static int32 gDebugLevel = 0;
	static int32 gHasReportLevel = 0;
	static int32 gReportLevel = 0;
	static int32 AtomDebugLevel() {
		if ((gHasDebugLevel&2) != 0) return gDebugLevel;
		if (atomic_or(&gHasDebugLevel, 1) == 0) {
			const char* env = getenv("ATOM_DEBUG");
			if (env) {
				gDebugLevel = atoi(env);
				if (gDebugLevel < 0)
					gDebugLevel = 0;
			}
			atomic_or(&gHasDebugLevel, 2);
		}
		while ((gHasDebugLevel&2) == 0) sleep(2000);
		return gDebugLevel;
	}
	static int32 AtomReportLevel(uint32 flags) {
		switch (flags&B_ATOM_REPORT_FORCE_MASK) {
			case B_ATOM_REPORT_FORCE_LONG:		return 10;
			case B_ATOM_REPORT_FORCE_SHORT:		return 5;
			case B_ATOM_REPORT_FORCE_SUMMARY:	return 0;
			default:						break;
		}
		
		if ((gHasReportLevel&2) != 0) return gReportLevel;
		if (atomic_or(&gHasReportLevel, 1) == 0) {
			const char* env = getenv("ATOM_REPORT");
			if (env) {
				gReportLevel = atoi(env);
				if (gReportLevel < 0)
					gReportLevel = 0;
			}
			atomic_or(&gHasReportLevel, 2);
		}
		while ((gHasReportLevel&2) == 0) sleep(2000);
		return gReportLevel;
	}
	
	static int32 gCurMark = 0;
	static int32 IncrementMark() { return atomic_add(&gCurMark, 1) + 1; }
	
	class AtomLeakChecker {
	public:
		AtomLeakChecker() : fCreated(0), fDestroyed(0), fFreed(0)
		{
		}
		~AtomLeakChecker()
		{
		}
		
		void Shutdown()
		{
			if (AtomDebugLevel() > 0 && (fCreated || fDestroyed || fFreed)) {
				AtomIO << "BAtom: "
					 << (fCreated-fFreed) << " leaked. ("
					 << fCreated << " created, "
					 << fDestroyed << " destroyed, "
					 << fFreed << " freed)" << endl;
			}
		}
		
		void NoteCreate()
		{
			if (AtomDebugLevel() > 0) atomic_add(&fCreated, 1);
		}
		void NoteDestroy()
		{
			if (AtomDebugLevel() > 0) atomic_add(&fDestroyed, 1);
		}
		void NoteFree()
		{
			if (AtomDebugLevel() > 0) atomic_add(&fFreed, 1);
		}
	
	private:
		int32 fCreated;
		int32 fDestroyed;
		int32 fFreed;
	};
	
	struct atom_ref_info {
		const void*		id;
		BCallStack		stack;
		int32			count;
		thread_id		thid;
		bigtime_t		when;
		char*			note;
		atom_ref_info*	next;
		
						atom_ref_info()
			: id(NULL), count(0), thid(B_ERROR), when(0), note(NULL), next(NULL)
		{
		}
						~atom_ref_info()
		{
			if (note) free(note);
		}
		
		bool			operator==(const atom_ref_info& o) const
		{
			if (id == o.id && stack == o.stack) return true;
			return false;
		}
		
		void			report(ITextOutput::arg io, bool longForm=false) const
		{
			const atom_ref_info* ref = this;
			while (ref) {
				if (longForm) {
					io << "ID: " << (void*)(ref->id)
						<< ", Thread: " << ref->thid
						<< ", When:" << ref->when << endl;
					io->BumpIndentLevel(1);
					ref->stack.LongPrint(io, NULL);
					io->BumpIndentLevel(-1);
				} else {
					io << (void*)(ref->id) << " " << ref->thid
						 << " " << ref->when << "us -- ";
					ref->stack.Print(io);
					io << endl;
				}
				ref = ref->next;
			}
		}
	
		int32 			refcount() const
		{
			int32 num = 0;
			const atom_ref_info* ref = this;
			while (ref) {
				ref = ref->next;
				num++;
			}
			return num;
		}

	};
	
	struct atom_debug : public BLocker {
		BAtom*				atom;
		int32				primary;
		int32				secondary;
		int32				mark;
		atom_ref_info*		acquires;
		atom_ref_info*		releases;
		atom_ref_info*		incRefs;
		atom_ref_info*		decRefs;
		
						atom_debug(BAtom* inAtom, int32 inMark)
			:	atom(inAtom),
				primary(INITIAL_PRIMARY_VALUE), secondary(0),
				mark(inMark),
				acquires(NULL), releases(NULL), incRefs(NULL), decRefs(NULL)
		{
		}
		
						~atom_debug()
		{
			atom_ref_info *ref;
			while ((ref=acquires) != NULL) {
				acquires = ref->next;
				delete ref;
			}
			while ((ref=releases) != NULL) {
				releases = ref->next;
				delete ref;
			}
			while ((ref=incRefs) != NULL) {
				incRefs = ref->next;
				delete ref;
			}
			while ((ref=decRefs) != NULL) {
				decRefs = ref->next;
				delete ref;
			}
			atom = NULL;
		}
		
		void			report(ITextOutput::arg io, uint32 flags) const
		{
			BStringIO::ptr sio(new BStringIO);
			
			if (!(flags&B_ATOM_REPORT_REMOVE_HEADER)) {
				sio << "Report for atom " << atom
					<< " (" << typeid(*atom).name() << ") at mark #"
					<< mark << ":" << endl;
			}
			const int32 level = AtomReportLevel(flags);
			if (level > 0) {
				const bool longForm = level > 5;
				sio << primary << " Acquires Remain:" << endl;
				sio->BumpIndentLevel(1);
				acquires->report(sio, longForm);
				sio->BumpIndentLevel(-1);
				sio << secondary << " IncRefs Remain:" << endl;
				sio->BumpIndentLevel(1);
				incRefs->report(sio, longForm);
				sio->BumpIndentLevel(-1);
				sio << "DecRefs:" << endl;
				sio->BumpIndentLevel(1);
				decRefs->report(sio, longForm);
				sio->BumpIndentLevel(-1);
				sio << "Releases:" << endl;
				sio->BumpIndentLevel(1);
				releases->report(sio, longForm);
				sio->BumpIndentLevel(-1);
			} else {
				sio << primary << " Acquires Remain, "
					<< secondary << " IncRefs Remain. ("
					<< acquires->refcount() << " Acquires, "
					<< incRefs->refcount() << " IncRefs, "
					<< decRefs->refcount() << " DecRefs, "
					<< releases->refcount() << " Releases)" << endl;
			}
			
			io << sio->String();
		}
	};
	
	class BAtomTracker
	{
	public:
		BAtomTracker()
			: fGone(false), fWatching(false)
		{
		}
		
		~BAtomTracker()
		{
		}
		
		void Shutdown()
		{
			if (AtomDebugLevel() > 5) {
				BAutolock _l(fAccess.Lock());
				if (!fActiveAtoms.empty()) {
					AtomIO << "Leaked atoms:" << endl;
					AtomIO->BumpIndentLevel(1);
					PrintActive(AtomIO, 0, -1, 0);
					AtomIO->BumpIndentLevel(-1);
				}
				fActiveAtoms.clear();
				fWatchTypes.clear();
			}
			fGone = true;
		}
		
		void AddAtom(atom_debug* info)
		{
			BAutolock _l(fAccess.Lock());
			if (fGone) return;
			fActiveAtoms.insert(info);
		}
		
		void RemoveAtom(atom_debug* info)
		{
			BAutolock _l(fAccess.Lock());
			if (fGone) return;
			fActiveAtoms.erase(info);
		}
		
		void PrintActive(ITextOutput::arg io, int32 mark, int32 last, uint32 flags) const
		{
			BAutolock _l(fAccess.Lock());
			if (!fActiveAtoms.empty()) {
				std::set<atom_debug*>::iterator i;
				for (i=fActiveAtoms.begin(); i != fActiveAtoms.end(); i++) {
					(*i)->Lock();
					if ((*i)->mark >= mark && (last < 0 || (*i)->mark <= last))
						(*i)->report(io, flags);
					(*i)->Unlock();
				}
			} else {
				io << "No active atoms." << endl;
			}
		}
		
		bool HasAtom(atom_debug* a, bool primary)
		{
			BAutolock _l(fAccess.Lock());
			std::set<atom_debug*>::const_iterator i = fActiveAtoms.find(a);
			// Note that there are race conditions here between the last
			// reference going away, the destructor being called, and the
			// atom being unregistered.  But this is just for debugging,
			// so I don't really care that much.
			if (i != fActiveAtoms.end() && a->atom->IncRefsCount() > 0) {
				if (primary) a->atom->Acquire();
				else a->atom->IncRefs();
				return true;
			}
			return false;
		}
	
		void StartWatching(const std::type_info* type)
		{
			BAutolock _l(fAccess.Lock());
			fWatchTypes.insert(type);
			fWatching = true;
		}
		
		void StopWatching(const std::type_info* type)
		{
			BAutolock _l(fAccess.Lock());
			fWatchTypes.erase(type);
			if (fWatchTypes.empty()) fWatching = false;
		}
		
		void WatchAction(const BAtom* which, const char* action)
		{
			BAutolock _l(fAccess.Lock());
			if (fGone || !fWatching) {
				return;
			}
			if (fWatchTypes.find(&(typeid(*which))) != fWatchTypes.end()) {
				BStringIO::ptr sio(new BStringIO);
				sio << "Action " << which << " "
					<< typeid(*which).name() << "::" << action;
				BCallStack stack;
				stack.Update(2);
				sio << endl;
				sio->BumpIndentLevel(1);
				stack.LongPrint(sio, NULL);
				sio->BumpIndentLevel(-1);
				which->Report(sio);
				AtomIO << sio->String();
			}
		}
		
	private:
		mutable BNestedLocker fAccess;
		bool fGone;
		bool fWatching;
		int32 fCurMark;
		std::set<atom_debug*> fActiveAtoms;
		std::set<const std::type_info*> fWatchTypes;
	};
	
	// Globals
	static int32 gHasLeakChecker = 0;
	static AtomLeakChecker* gLeakChecker = NULL;
	static int32 gHasTracker = 0;
	static BAtomTracker* gTracker = NULL;
	
	static AtomLeakChecker* LeakChecker() {
		if ((gHasLeakChecker&2) != 0) return gLeakChecker;
		if (atomic_or(&gHasLeakChecker, 1) == 0) {
			gLeakChecker = new AtomLeakChecker;
			atomic_or(&gHasLeakChecker, 2);
		} else {
			while ((gHasLeakChecker&2) == 0) sleep(2000);
		}
		return gLeakChecker;
	}
	static BAtomTracker* Tracker() {
		if ((gHasTracker&2) != 0) return gTracker;
		if (atomic_or(&gHasTracker, 1) == 0) {
			gTracker = new BAtomTracker;
			atomic_or(&gHasTracker, 2);
		} else {
			while ((gHasTracker&2) == 0) sleep(2000);
		}
		return gTracker;
	}
	
	struct atom_cleanup
	{
		~atom_cleanup()
		{
			if (gTracker) gTracker->Shutdown();
			if (gLeakChecker) gLeakChecker->Shutdown();
		}
	};
	static atom_cleanup gCleanup;
	
}	//namespace Private
using namespace Private;

namespace Support2 {
	
	/*	\note Debugging only.  This function only works if are linking
		with a library that contains debugging code and have set the
		ATOM_DEBUG environment variable.  Choices for ATOM_DEBUG are:
			-	<= 0: Debugging disabled.
			-	<= 5: Simple debugging -- create/delete statistics only.
			-	<= 10: Full BAtom debugging enabled.
		In addition, you can modify the default Report() format with the
		ATOM_REPORT environment variable:
			-	<= 0: Short summary report.
			-	<= 5: One-line call stacks.
			-	<= 10: Call stacks with full symbols.
	*/
	void BAtom::Report(ITextOutput::arg io, uint32 flags) const {
		lock_atom();
		do_report(io, flags);
		unlock_atom();
	}
	
	/*!	\note Debugging only.  Set the ATOM_DEBUG and ATOM_REPORT
		environment variables to use.
		\sa Report()
		\sa LeakReport()
	*/
	int32 BAtom::MarkLeakReport() {
		if (AtomDebugLevel() > 5) return IncrementMark();
		return 0;
	}
	
	/*!	This function prints information about all of the currently
		active atoms created during the leak context \a mark up to and
		including the context \a last.
		A \a mark context of 0 is the first context; a \a last context
		of -1 means the current context.
		\note Debugging only.  Set the ATOM_DEBUG and ATOM_REPORT
		environment variables to use.
		\sa Report()
		\sa MarkLeakReport()
	*/
	void BAtom::LeakReport(ITextOutput::arg io, int32 mark, int32 last, uint32 flags) {
		if (AtomDebugLevel() > 5) {
			if (last < 0)
				io << "Active atoms since mark " << mark << ":" << endl;
			else
				io << "Active atoms from mark " << mark << " to " << last << ":" << endl;
			io->BumpIndentLevel(1);
			Tracker()->PrintActive(io, mark, last, flags);
			io->BumpIndentLevel(-1);
		}
	}
	
	void BAtom::LeakReport(int32 mark, int32 last, uint32 flags) {
		LeakReport(AtomIO, mark, last, flags);
	}
	
	/*!	Check whether the given atom currently exists, and acquire a
		primary or secondary reference if so.  These only work when the
		leak checker is turned on; otherwise, false is always returned.
		\note Debugging only.  Set the ATOM_DEBUG and ATOM_REPORT
		environment variables to use.
		\sa Report()
		\sa MarkLeakReport()
	*/
	bool BAtom::ExistsAndAcquire(BAtom* atom) {
		if (AtomDebugLevel() > 5)
			return Tracker()->HasAtom((*((atom_debug**)(&atom->fPrimary))), true);
		return false;
	}

	bool BAtom::ExistsAndIncRefs(BAtom* atom) {
		if (AtomDebugLevel() > 5)
			return Tracker()->HasAtom((*((atom_debug**)(&atom->fPrimary))), false);
		return false;
	}

	/*!	Information will be printed as operations are performed on all
		atoms of the given type.  Note that they must be -exactly- this
		type -- subclasses are not included.
		\note Debugging only.  Set the ATOM_DEBUG and ATOM_REPORT
		environment variables to use.
		\sa Report()
		\sa MarkLeakReport()
	*/
	void BAtom::StartWatching(const std::type_info* type) {
		Tracker()->StartWatching(type);
	}
	
	void BAtom::StopWatching(const std::type_info* type) {
		Tracker()->StopWatching(type);
	}
	
	void BAtom::init_atom() {
		if (AtomDebugLevel() > 5) {
			fDebugPtr = new(std::nothrow) atom_debug(this, gCurMark);
			if (!fDebugPtr) debugger("Out of memory!");
			Tracker()->AddAtom(fDebugPtr);
		}
	}
	void BAtom::term_atom() {
		if (AtomDebugLevel() > 5) {
			Tracker()->RemoveAtom(fDebugPtr);
			delete fDebugPtr;
			fDebugPtr = NULL;
		}
	}

	void BAtom::lock_atom() const {
		if (AtomDebugLevel() > 5) fDebugPtr->Lock();
	}
	void BAtom::unlock_atom() const {
		if (AtomDebugLevel() > 5) fDebugPtr->Unlock();
	}
	
	int32* BAtom::primary_addr() const {
		if (AtomDebugLevel() > 5) return &(fDebugPtr->primary);
		return &fPrimary;
	}
	int32* BAtom::secondary_addr() const {
		if (AtomDebugLevel() > 5) return &(fDebugPtr->secondary);
		return &fSecondary;
	}
	
	int32 BAtom::primary_count() const {
		if (AtomDebugLevel() > 5) return fDebugPtr ? fDebugPtr->primary : 0;
		return fPrimary;
	} 
	int32 BAtom::secondary_count() const {
		if (AtomDebugLevel() > 5) return fDebugPtr ? fDebugPtr->secondary : 0;
		return fSecondary;
	} 

	void BAtom::watch_action(const char* description) const {
		if (AtomDebugLevel() > 5) Tracker()->WatchAction(this, description);
	}
	
	void BAtom::do_report(ITextOutput::arg io, uint32 flags) const {
		if (AtomDebugLevel() > 5) fDebugPtr->report(io, flags);
	}
	
	void BAtom::add_acquire(const void* id) const {
		if (AtomDebugLevel() > 5) {
			atom_ref_info *ref = new(std::nothrow) atom_ref_info;
			if (ref) {
				ref->id = id;
				ref->thid = find_thread(NULL);
				ref->when = system_time();
				ref->note = NULL;
				ref->stack.Update(1);
				ref->next = fDebugPtr->acquires;
				fDebugPtr->acquires = ref;
			}
		}
	}
	
	void BAtom::add_release(const void* id) const {
		if (AtomDebugLevel() > 5) {
			bool found = false;
			if (id) {
				atom_ref_info *p,**ref;
				for (ref = &fDebugPtr->acquires; *ref;ref = &(*ref)->next) {
					if ((*ref)->id == id) {
						p = *ref;
						*ref = (*ref)->next;
						delete p;
						found = true;
						break;
					}
				}
			}
		
			if (!found) {
				atom_ref_info *ref = new(std::nothrow) atom_ref_info;
				if (ref) {
					ref->id = id;
					ref->thid = find_thread(NULL);
					ref->when = system_time();
					ref->stack.Update(1);
					ref->next = fDebugPtr->releases;
					fDebugPtr->releases = ref;
				}
				
				if (id) {
					BStringIO::ptr io(new BStringIO);
					io->BumpIndentLevel(1);
					io << "BAtom: No Acquire() found for Release() id=" << id << endl;
					do_report(io, B_ATOM_REPORT_FORCE_LONG);
					io->BumpIndentLevel(-1);
					debugger(io->String());
				}
			}
		}
	}
	
	void BAtom::add_increfs(const void* id) const {
		if (AtomDebugLevel() > 5) {
			atom_ref_info *ref = new(std::nothrow) atom_ref_info;
			if (ref) {
				ref->id = id;
				ref->thid = find_thread(NULL);
				ref->when = system_time();
				ref->note = NULL;
				ref->stack.Update(1);
				ref->next = fDebugPtr->incRefs;
				fDebugPtr->incRefs = ref;
			}
		}
	}
	
	void BAtom::add_decrefs(const void* id) const {
		if (AtomDebugLevel() > 5) {
			bool found = false;
			if (id) {
				atom_ref_info *p,**ref;
				for (ref = &fDebugPtr->incRefs; *ref;ref = &(*ref)->next) {
					if ((*ref)->id == id) {
						p = *ref;
						*ref = (*ref)->next;
						delete p;
						found = true;
						break;
					}
				}
			}
		
			if (!found) {
				atom_ref_info *ref = new(std::nothrow) atom_ref_info;
				if (ref) {
					ref->id = id;
					ref->thid = find_thread(NULL);
					ref->when = system_time();
					ref->stack.Update(1);
					ref->next = fDebugPtr->decRefs;
					fDebugPtr->decRefs = ref;
				}
			}
		}
	}
} }	// namespace B::Support2

#define NOTE_CREATE() { if (AtomDebugLevel() > 0) LeakChecker()->NoteCreate(); }
#define NOTE_DESTROY() { if (AtomDebugLevel() > 0) LeakChecker()->NoteDestroy(); }
#define NOTE_FREE() { if (AtomDebugLevel() > 0) LeakChecker()->NoteFree(); }

#endif

/*---------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------*/

/* Common BAtom functionality. */

namespace B {
namespace Support2 {

// This is the TLS slot in which we store the base address of
// an atom class when allocating it.  Doing this allows us to
// get that information up to the BAtom constructor.
static int32 gAtomBaseIndex = -1;
static int32 gAtomBaseAllocated = 0;

/*!	BAtom overrides its new and delete operators for its implementation.
	You \em must use these operators and instantiating classes derived
	from BAtom.  Thus a subclass can not implement its own new or delete
	operators, nor can you use inplace-new on a BAtom class.
*/
void* BAtom::operator new(size_t size)
{
	// Allocate extra space before the atom, rounded to 8 bytes.
	base_data* ptr = (base_data*)::operator new(size + sizeof(base_data));
	
	// Stash this pointer in the thread's slot, first creating
	// the slot if it doesn't already exist.
	if (!atomic_or(&gAtomBaseAllocated, 1)) {
		int32 tls = tls_allocate();
		tls_set(tls, NULL);
		gAtomBaseIndex = tls;
	} else {
		while (!gAtomBaseIndex) snooze(2000);
	}
	
	if (tls_get(gAtomBaseIndex) != NULL) {
		debugger("BAtom must be the first base class when using multiple inheritance.");
		return NULL;
	}
	
	PRINT(("Allocate atom base=%p, tls=%ld, off=%p\n",
				ptr, gAtomBaseIndex, ptr+1));
	
	tls_set(gAtomBaseIndex, ptr);
	return ptr+1;
}

void* BAtom::operator new(size_t size, const std::nothrow_t&)
{
	// Allocate extra space before the atom, rounded to 8 bytes.
	base_data* ptr = (base_data*)::operator new(size + sizeof(base_data));
	if (!ptr) return ptr;
	
	// Stash this pointer in the thread's slot, first creating
	// the slot if it doesn't already exist.
	if (!atomic_or(&gAtomBaseAllocated, 1)) {
		int32 tls = tls_allocate();
		tls_set(tls, NULL);
		gAtomBaseIndex = tls;
	} else {
		while (!gAtomBaseIndex) snooze(2000);
	}
	
	if (tls_get(gAtomBaseIndex) != NULL) {
		debugger("BAtom must be the first base class when using multiple inheritance.");
		return NULL;
	}
	
	PRINT(("Allocate atom base=%p, tls=%ld, off=%p\n",
				ptr, gAtomBaseIndex, ptr+1));
	
	tls_set(gAtomBaseIndex, ptr);
	return ptr+1;
}

void BAtom::operator delete(void* ptr, size_t size)
{
	if (ptr) {
		// Go back to find the pointer into the atom instance.
		(((base_data*)ptr)-1)->atom->delete_impl(size);
	}
}

void BAtom::delete_impl(size_t size)
{
	if (secondary_count() == 0) {
		// The atom has already been destroyed, goodbye.
		PRINT(("Freeing atom memory %p directly\n", this));
		::operator delete(fBase);
		NOTE_FREE();
		return;
	}
	
	const int32 c = primary_count();
	if (c != 0 && c != INITIAL_PRIMARY_VALUE) {
		debugger("BAtom: deleted with non-zero primary references");
	}
	
	// Cache object size to later free its memory.
	fBase->size = size;
}

void BAtom::destructor_impl()
{
	if (secondary_count()) debugger("BAtom: deleted with non-zero references");
	term_atom();
}

BAtom::BAtom()
	:	fPrimary(INITIAL_PRIMARY_VALUE), fSecondary(0)
{
	NOTE_CREATE();
	
	fBase = (base_data*)tls_get(gAtomBaseIndex);
	if (gAtomBaseIndex < 0 || fBase == NULL) {
		debugger("Do not override operator new() or operator delete() when inheriting from BAtom");
	}
	fBase->atom = this;
	fBase->size = 0;
	tls_set(gAtomBaseIndex, NULL);
	
	init_atom();
	
	PRINT(("Creating BAtom %p\n", this));
}

BAtom::~BAtom()
{
	NOTE_DESTROY();
	
	PRINT(("Destroying BAtom %p\n", this));
	if (secondary_count() == 0) {
		// Whoops, someone is really deleting us.  Do it all.
		destructor_impl();
	}
}

/*!	This keeps an atom from actually being freed after its
	last primary reference is removed.  If you are only
	holding a secondary reference on the object, you know
	that the memory it points to still exists, but don't
	know what state the object is in.  The optional \a id
	parameter is the memory address of the object holding
	this reference.  It is only used for debugging.
	\sa atom_ref<>
*/
int32 BAtom::IncRefs(const void *id) const DECLARE_RETURN(r)
{
	lock_atom();
	const int32 r = atomic_add(secondary_addr(), 1);
	add_increfs(id);
	unlock_atom();

	return r;
}

int32 BAtom::DecRefs(const void *id) const DECLARE_RETURN(r)
{
	lock_atom();
	const int32 r = atomic_add(secondary_addr(), -1);
	add_decrefs(id);
	unlock_atom();

	if (r == 1)	{
		watch_action("DecRefs() [last]");
		if (fBase->size > 0) {
			const_cast<BAtom*>(this)->destructor_impl();
			PRINT(("Freeing atom memory %p after last ref\n", this));
			::operator delete(fBase);
			NOTE_FREE();
		} else {
			if (const_cast<BAtom*>(this)->DeleteAtom(id) == B_OK)
				delete this;
		}
	} else if (r < 1) {
		debugger("BAtom: DecRefs() called more times than IncRefs()");
	}
	return r;
}

/*!	This is the standard reference count -- once it
	transitions to zero, the atom will become invalid.
	An atom starts with a reference count of zero, and
	gets invalidated the first time it transitions from one
	to zero.  The optional \a id parameter is the memory address
	of the object holding this reference.  It is only used
	for debugging.
	\sa atom_ptr<>
*/
int32 BAtom::Acquire(const void *id) const DECLARE_RETURN(r)
{
	IncRefs(id);
	
	lock_atom();
	int32 r = atomic_add(primary_addr(), 1);
	if (r == INITIAL_PRIMARY_VALUE)
		atomic_add(primary_addr(), -INITIAL_PRIMARY_VALUE);
	
	if (r == 0) {
		// The acquire count was at zero, this is an error.
		atomic_add(primary_addr(), -1);
		unlock_atom();
		debugger("BAtom: atom acquired after final release");
		DecRefs();
		return 0;
	}
	
	add_acquire(id);

	unlock_atom();

	if (r == INITIAL_PRIMARY_VALUE) {
		watch_action("Acquired()");
		const_cast<BAtom*>(this)->Acquired(id);
		r -= INITIAL_PRIMARY_VALUE;
	}
	
	return r;
}

int32 BAtom::Release(const void *id) const DECLARE_RETURN(r)
{
	lock_atom();
	const int32 r = atomic_add(primary_addr(), -1);
	add_release(id);
	unlock_atom();
	
	if (r == 1)	{
		watch_action("Released()");
		if (const_cast<BAtom*>(this)->Released(id) == B_OK)
			delete this;
	} else if (r < 1) {
		debugger("BAtom: Release() called more times than Acquire()");
	}
	DecRefs(id);
	return r;
}

/*!	You must already have a secondary reference on the atom.
	This function will attempt to add a new primary reference
	to the atom.  It returns true on success, in which case
	a new primary reference has been acquired which you must
	later remove with Release().  Otherwise, the atom is
	left as-is.  Failure only occurs after the atom has already
	been acquired and then completely released.  That is,
	AttemptAcquire() will succeeded on a newly created BAtom
	that has never been acquired.
	\sa Acquire()
*/
bool BAtom::AttemptAcquire(const void *id) const
{
	IncRefs(id);
	
	lock_atom();
	
	// Attempt to increment the reference count, without
	// disrupting it if it has already gone to zero.
	int32 current = primary_count();
	while (current > 0 && !cmpxchg32(primary_addr(), &current, current+1))
		snooze(200);
	
	if (current <= 0) {
		unlock_atom();
		// The primary count has gone to zero; if the object hasn't yet
		// been deleted, give it a chance to renew the atom.
		const bool die = fBase->size > 0
						|| (const_cast<BAtom*>(this)->AcquireAttempted(id) < B_OK);
		if (die) {
			DecRefs(id);
			return false;
		}
		
		// AcquireAttempted() has allowed us to revive the atom, so increment
		// the reference count and continue with a success.
		lock_atom();
		current = atomic_add(primary_addr(), 1);
		// If the primary references count has already been incremented by
		// someone else, the implementor of AcquireAttempted() is holding
		// an unneeded reference.  So call Released() here to remove it.
		// (No, this is not pretty.)
		if (current > 0) {
			if (const_cast<BAtom*>(this)->Released(id) == B_OK)
				debugger("Released() must not return B_OK if you implement AcquireAttempted()");
		}
	}
	
	if (current == INITIAL_PRIMARY_VALUE)
		atomic_add(primary_addr(), -INITIAL_PRIMARY_VALUE);
		
	add_acquire(id);

	unlock_atom();
	
	if (current == INITIAL_PRIMARY_VALUE) {
		watch_action("Acquired()");
		const_cast<BAtom*>(this)->Acquired(id);
	}
	return true;
}

/*!	If this atom has any outstanding primary references, this
	function will remove one of them and return true.  Otherwise
	it leaves the atom as-is and returns false.  Trust me, it
	is useful.
	\sa Release()
*/
bool BAtom::AttemptRelease(const void *id) const
{
	lock_atom();

	int32 r = primary_count();
	while (r > 0 && !cmpxchg32(primary_addr(), &r, r-1))
		snooze(200);

	if (r > 0) {
		add_release(id);
		unlock_atom();
		
		if (r == 1)	{
			watch_action("Released()");
			if (const_cast<BAtom*>(this)->Released(id) == B_OK)
				delete this;
		}
		DecRefs(id);
		return true;
	}
	unlock_atom();
	
	return false;
}

/*!	This is just like Acquire(), except that it is not an error to
	call on a BAtom that currently does not have a primary reference.
	If you think you need to use this, think again.
	\sa Acquire<>
*/
int32 BAtom::ForceAcquire(const void *id) const DECLARE_RETURN(r)
{
	IncRefs(id);
	
	lock_atom();
	int32 r = atomic_add(primary_addr(), 1);
	if (r == INITIAL_PRIMARY_VALUE)
		atomic_add(primary_addr(), -INITIAL_PRIMARY_VALUE);
	
	add_acquire(id);

	unlock_atom();

	if (r == INITIAL_PRIMARY_VALUE || r == 0) {
		watch_action("Acquired()");
		const_cast<BAtom*>(this)->Acquired(id);
		r -= INITIAL_PRIMARY_VALUE;
	}
	
	return r;
}

/*!	\note Debugging only.  The returned value is no longer valid
	as soon as you receive it.
*/
int32 BAtom::AcquireCount() const DECLARE_RETURN(r)
{
	lock_atom();
	const int32 r = primary_count();
	unlock_atom();
	return r;
}

int32 BAtom::IncRefsCount() const DECLARE_RETURN(r)
{
	lock_atom();
	const int32 r = secondary_count();
	unlock_atom();
	return r;
}

/*!	You can override it and do any setup you
	need.  Note that you do not need to call the BAtom
	implementation.  (So you can derive from two different
	BAtom implementations and safely call down to both
	of their Acquire() methods.) For now, always return B_OK.
	\sa Acquire()
*/
status_t BAtom::Acquired(const void*)
{
	return B_OK;
}

/*!	The default implementation returns B_OK,
	which indicates that the object should now be deleted.
	You can override this to return an error code if you
	would like to delay its destruction.  Like Acquired(),
	you do not need to call the BAtom implementation.
	\sa Release()
*/
status_t BAtom::Released(const void*)
{
	return B_OK;
}

/*!	By default it returns B_ERROR, which makes the attempted
	acquire fail.  You can override this to return B_OK when
	you would like an atom to continue allowing primary references.
	\sa AttemptAcquire()
*/
status_t BAtom::AcquireAttempted(const void*)
{
	return B_NOT_ALLOWED;
}

/*!	If you override Released() to not call into BAtom
	(and thus extend the life of your object), then this
	method will be called when its last reference goes
	away.  The default implementation returns B_OK to have
	the object deleted.
	\sa DecRefs();
	\sa Released()
*/
status_t BAtom::DeleteAtom(const void*)
{
	return B_OK;
}

} }	// namespace B::Support2
