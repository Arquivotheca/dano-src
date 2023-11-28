#include <Atom.h>

#include <OS.h>
#include <atomic.h>
#include <TypeConstants.h>
#include <Message.h>

#include <stdio.h>

#if SUPPORTS_OLD_ATOM_DEBUG
#define DEBUG_ATOMS 1
#define CHECK_LEAKS 1
#else
#define DEBUG_ATOMS 0
#define CHECK_LEAKS 0
#endif

#if DEBUG_ATOMS || CHECK_LEAKS
#include <Autolock.h>
#include <Locker.h>
#include <StreamIO.h>
#include <StringIO.h>
#include <CallStack.h>
#include <String.h>
#include <malloc.h>
#include <typeinfo>
#include <stdlib.h>

namespace BPrivate {
	// This custom IO stream class is only to ensure that the stream
	// still exists while our static objects are being destroyed.
	// (Since we are in the same library as StreamIO.h, there is no
	// guarantee about whose destructors will be called first.)
	class AtomDataIOStub : public BDataIO {
	public:
		AtomDataIOStub(FILE* fh)
			: fFile(fh)
		{
		}
		
		virtual ~AtomDataIOStub()
		{
		}
		
		virtual ssize_t Read(void *buffer, size_t size);
		virtual ssize_t Write(const void *buffer, size_t size);
	
	private:
		FILE* fFile;
	};
	
	ssize_t AtomDataIOStub::Read(void *buffer, size_t size)
	{
		ssize_t iosize = fread(buffer, size, 1, fFile);
		status_t err = ferror(fFile);
		if (err != B_OK) return err;
		return iosize;
	}
		
	ssize_t AtomDataIOStub::Write(const void *buffer, size_t size)
	{
		ssize_t iosize = fwrite(buffer, size, 1, fFile);
		status_t err = ferror(fFile);
		if (err != B_OK) return err;
		return iosize;
	}
	static AtomDataIOStub AtomIO(stderr);

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
				if (gDebugLevel < 1)
					gDebugLevel = 1;
			}
			atomic_or(&gHasDebugLevel, 2);
		}
		while ((gHasDebugLevel&2) == 0) sleep(2000);
		return gDebugLevel;
	}
	static int32 AtomReportLevel() {
		if ((gHasReportLevel&2) != 0) return gReportLevel;
		if (atomic_or(&gHasReportLevel, 1) == 0) {
			const char* env = getenv("ATOM_REPORT");
			if (env) {
				gReportLevel = atoi(env);
				if (gReportLevel < 1)
					gReportLevel = 1;
			}
			atomic_or(&gHasReportLevel, 2);
		}
		while ((gHasReportLevel&2) == 0) sleep(2000);
		return gReportLevel;
	}
}

#endif

#if CHECK_LEAKS
#include <map>
#include <set>

namespace BPrivate {
	class AtomLeakChecker {
	public:
		AtomLeakChecker() : fGone(false), fWatching(false), fCreated(0), fDeleted(0), fCurMark(0)
		{
		}
		~AtomLeakChecker()
		{
			BAutolock _l(fAccess);
			fGone = true;
			if (!fActiveAtoms.empty()) {
				PrintActive(AtomIO);
			}
			if (fCreated || fDeleted) {
				AtomIO << "BAtom Statistics: "
						<< "Created " << fCreated
						<< ", Deleted " << fDeleted << endl;
			}
		}
		
		void NoteCreate(BAtom* a)
		{
			atomic_add(&fCreated, 1);
			if (AtomDebugLevel() > 0) {
				BAutolock _l(fAccess);
				if (fGone) return;
				fActiveAtoms[a] = fCurMark;
			}
		}
		void NoteDelete(BAtom* a)
		{
			atomic_add(&fDeleted, 1);
			if (AtomDebugLevel() > 0) {
				BAutolock _l(fAccess);
				if (fGone) return;
				fActiveAtoms.erase(a);
			}
		}
		
		int32 Mark()
		{
			return atomic_add(&fCurMark, 1) + 1;
		}
		
		void PrintActive(BDataIO& io, int32 mark = 0, int32 last = -1) const
		{
			if (AtomDebugLevel() > 0) {
				BAutolock _l(fAccess);
				if (!fActiveAtoms.empty()) {
					io << "Active atoms:" << endl;
					std::map<const BAtom*, int32>::const_iterator i;
					const std::map<const BAtom*, int32>::const_iterator end(fActiveAtoms.end());
					for (i=fActiveAtoms.begin(); i != end; ++i) {
						if (i->second >= mark && (last < 0 || i->second <= last)) {
							io << "Atom " << i->first << ": '"
								<< typeid(*(i->first)).name() << "'"
								<< " mark #" << i->second << endl;
							i->first->Report(io, "  ", B_ATOM_REPORT_REMOVE_HEADER);
						}
					}
				} else {
					io << "No active atoms." << endl;
				}
			}
		}
		
		bool HasAtom(BAtom* a, bool primary)
		{
			if (AtomDebugLevel() > 0) {
				BAutolock _l(fAccess);
				std::map<const BAtom*, int32>::const_iterator i = fActiveAtoms.find(a);
				// Note that there are race conditions here between the last
				// reference going away, the destructor being called, and the
				// atom being unregistered.  But this is just for debugging,
				// so I don't really care that much.
				if (i != fActiveAtoms.end() && a->IncRefsCount() > 0) {
					if (primary) a->Acquire();
					else a->IncRefs();
					return true;
				}
			}
			return false;
		}
		
		void StartWatching(const std::type_info* type)
		{
			BAutolock _l(fAccess);
			fWatchTypes.insert(type);
			fWatching = true;
		}
		
		void StopWatching(const std::type_info* type)
		{
			BAutolock _l(fAccess);
			fWatchTypes.erase(type);
			if (fWatchTypes.empty()) fWatching = false;
		}
		
		void WatchAction(const BAtom* which, const char* action)
		{
			BAutolock _l(fAccess);
			if (fGone || !fWatching) return;
			if (fWatchTypes.find(&(typeid(*which))) != fWatchTypes.end()) {
				BStringIO sio;
				sio << "Action " << which << " "
					<< typeid(*which).name() << "::" << action;
				BCallStack stack;
				stack.Update(0);
				sio << endl;
				stack.LongPrint(sio, NULL, "  ");
				//stack.Print(AtomIO);
				//AtomIO << endl;
				which->Report(sio);
				AtomIO << sio.String();
			}
		}
		
	private:
		mutable BLocker fAccess;
		bool fGone;
		bool fWatching;
		std::map<const BAtom*, int32> fActiveAtoms;
		std::set<const std::type_info*> fWatchTypes;
		int32 fCreated;
		int32 fDeleted;
		int32 fCurMark;
	};
	
	static AtomLeakChecker LeakChecker;
}

#define NOTE_CREATE(who)			BPrivate::LeakChecker.NoteCreate(who)
#define NOTE_DELETE(who)			BPrivate::LeakChecker.NoteDelete(who)
#define LEAK_MARK()					BPrivate::LeakChecker.Mark();
#define LEAK_REPORT(io, mark, last)	BPrivate::LeakChecker.PrintActive(io, mark, last)
#define HAS_ATOM(who, primary)		BPrivate::LeakChecker.HasAtom(who, primary)
#define START_WATCHING(type)		BPrivate::LeakChecker.StartWatching(type)
#define STOP_WATCHING(type)			BPrivate::LeakChecker.StopWatching(type)
#define WATCH_ACTION(atom, action)	BPrivate::LeakChecker.WatchAction(atom, action);

#else

#define NOTE_CREATE(who)
#define NOTE_DELETE(who)
#define LEAK_MARK()					0
#define LEAK_REPORT(io, mark, last)
#define HAS_ATOM(who, primary)		false
#define START_WATCHING(type)
#define STOP_WATCHING(type)
#define WATCH_ACTION(atom, action)

#endif

status_t 
BMessage::AddAtom(const char *name, const atom<BAtom> &a)
{
	status_t r = AddAtom(name, (BAtom*)a);
	return r;
}

status_t 
BMessage::AddAtomRef(const char *name, const atom<BAtom> &a)
{
	status_t r = AddAtomRef(name, (BAtom*)a);
	return r;
}

status_t 
BMessage::AddAtomRef(const char *name, const atomref<BAtom> &a)
{
	status_t r = AddAtomRef(name, (BAtom*)a);
	return r;
}

status_t 
BMessage::FindAtom(const char *name, int32 index, atom<BAtom> &a) const
{
	status_t e;
	BAtom *obj;
	if ((e = FindAtom(name, index, &obj)) != B_OK) obj = NULL;
	a = obj;
	return e;
}

status_t 
BMessage::FindAtom(const char *name, atom<BAtom> &a) const
{
	return FindAtom(name, 0, a);
}

status_t 
BMessage::FindAtomRef(const char *name, int32 index, atomref<BAtom> &a) const
{
	status_t e;
	BAtom *obj;
	if ((e = FindAtomRef(name, index, &obj)) != B_OK) obj = NULL;
	a = obj;
	return e;
}

status_t 
BMessage::FindAtomRef(const char *name, atomref<BAtom> &a) const
{
	return FindAtomRef(name, 0, a);
}

status_t 
BMessage::ReplaceAtom(const char *name, int32 index, const atom<BAtom> &a)
{
	return ReplaceAtom(name, index, (BAtom*)a);
}

status_t 
BMessage::ReplaceAtom(const char *name, const atom<BAtom> &a)
{
	return ReplaceAtom(name,0,a);
}

status_t 
BMessage::ReplaceAtomRef(const char *name, int32 index, const atomref<BAtom> &a)
{
	return ReplaceAtomRef(name, index, (BAtom*)a);
}

status_t 
BMessage::ReplaceAtomRef(const char *name, const atomref<BAtom> &a)
{
	return ReplaceAtomRef(name,0,a);
}

/*---------------------------------------------------------------------------------*/

#define INITIAL_PRIMARY_VALUE (1<<28)

int32 BAtom::MarkLeakReport()
{
	return LEAK_MARK();
}

void BAtom::LeakReport(BDataIO& io, int32 mark, int32 last)
{
	(void)io;
	(void)mark;
	(void)last;
	return LEAK_REPORT(io, mark, last);
}

void BAtom::LeakReport(int32 mark, int32 last)
{
	(void)mark;
	(void)last;
	return LEAK_REPORT(AtomIO, mark, last);
}

bool BAtom::ExistsAndAcquire(BAtom* atom)
{
	(void)atom;
	return HAS_ATOM(atom, true);
}

bool BAtom::ExistsAndIncRefs(BAtom* atom)
{
	(void)atom;
	return HAS_ATOM(atom, false);
}

void BAtom::StartWatching(const std::type_info* type)
{
	(void)type;
	START_WATCHING(type);
}

void BAtom::StopWatching(const std::type_info* type)
{
	(void)type;
	STOP_WATCHING(type);
}

#if !defined(DEBUG_ATOMS) || DEBUG_ATOMS == 0

void BAtom::_delete()
{
	WATCH_ACTION(this, "_delete()");
	delete this;
};

void BAtom::Cleanup()
{
	if (m_primary != 0) debugger("BAtom: Cleanup() called with non-zero primary references!");
};

void BAtom::Acquired()
{
};

BAtom::BAtom() : m_primary(INITIAL_PRIMARY_VALUE), m_secondary(0)
{
	NOTE_CREATE(this);
};

BAtom::~BAtom()
{
	if (m_secondary != 0) debugger("BAtom: deleted with non-zero references!");
	// This currently crashes Wagner.
	//if (m_primary == INITIAL_PRIMARY_VALUE) debugger("BAtom: deleted before being acquired!");
	NOTE_DELETE(this);
};

void BAtom::Report(BDataIO&, const char*, uint32) const
{
}

void BAtom::Report(const char*, uint32) const
{
}

int32 BAtom::IncRefs(void *)
{
	int32 r = atomic_add(&m_secondary,1);
	if (!m_primary) {
		// This whole condition will be optimized out, right?
		WATCH_ACTION(this, "IncRefs after Cleanup!");
	}
	return r;
};

int32 BAtom::DecRefs(void *)
{
	if (!m_primary) {
		// This whole condition will be optimized out, right?
		WATCH_ACTION(this, "DecRefs() after Cleanup...");
	}
	int32 r = atomic_add(&m_secondary,-1);
	if (r == 1)	_delete();
	// This currently crashes Wagner.
	//else if (r < 1) debugger("BAtom: DecRefs() called more times than IncRefs()!");
	return r;
};

int32 BAtom::Acquire(void *id)
{
	IncRefs(id);
	int32 r = atomic_add(&m_primary,1);
	if (r == INITIAL_PRIMARY_VALUE) {
		atomic_add(&m_primary,-INITIAL_PRIMARY_VALUE);
		Acquired();
	} else if (r > INITIAL_PRIMARY_VALUE)
		r -= INITIAL_PRIMARY_VALUE;
	return r;
};

bool BAtom::AttemptAcquire(void *id)
{
	IncRefs(id);
	
	// Attempt to increment the reference count, without
	// disrupting it if it has already gone to zero.
	int32 current = m_primary;
	while (current > 0 && !cmpxchg32(&m_primary, &current, current+1))
		;
	
	if (current <= 0) {
		DecRefs(id);
		return false;
	}
		
	if (current == INITIAL_PRIMARY_VALUE) {
		atomic_add(&m_primary, -INITIAL_PRIMARY_VALUE);
		Acquired();
	}
	
	return true;
}

int32 BAtom::Release(void *id)
{
	int32 r = atomic_add(&m_primary,-1);
	if (r == 1)	{
		WATCH_ACTION(this, "Cleanup()");
		Cleanup();
	}
	else if (r < 1) debugger("BAtom: Release() called more times than Acquire()!");
	DecRefs(id);
	return r;
};

int32 BAtom::AcquireCount()
{
	const int32 p = m_primary;
	if (p == INITIAL_PRIMARY_VALUE) return 0;
	return p;
}

int32 BAtom::IncRefsCount()
{
	return m_secondary;
}

bool BAtom::CleanupCalled() const
{
	return m_primary == 0;
}

#else

#define debugPtr (*((atom_debug**)(&m_primary)))

struct atom_ref {
	void *id;
	thread_id thid;
	bigtime_t when;
	char *note;
	BCallStack stack;
	atom_ref *next;
	
	atom_ref() {
		note = NULL;
	};
	~atom_ref() {
		if (note) free(note);
	};
};

struct atom_debug : public Gehnaphore {
	int32			primary;
	int32			secondary;
	atom_ref *		acquires;
	atom_ref *		releases;
	atom_ref *		incRefs;
	atom_ref *		decRefs;
};

void BAtom::_delete()
{
	WATCH_ACTION(this, "_delete()");
	delete this;
};

void BAtom::Cleanup()
{
};

void BAtom::Acquired()
{
};

BAtom::BAtom() : m_primary(0), m_secondary(0)
{
	debugPtr = new atom_debug;
	debugPtr->primary = INITIAL_PRIMARY_VALUE;
	debugPtr->secondary = 0;
	debugPtr->incRefs = NULL;
	debugPtr->decRefs = NULL;
	debugPtr->acquires = NULL;
	debugPtr->releases = NULL;
	NOTE_CREATE(this);
};

BAtom::~BAtom()
{
	atom_ref *ref;
	while ((ref = debugPtr->acquires)) {
		debugPtr->acquires = ref->next;
		delete ref;
	};
	while ((ref = debugPtr->releases)) {
		debugPtr->releases = ref->next;
		delete ref;
	};
	while ((ref = debugPtr->incRefs)) {
		debugPtr->incRefs = ref->next;
		delete ref;
	};
	while ((ref = debugPtr->decRefs)) {
		debugPtr->decRefs = ref->next;
		delete ref;
	};
	delete debugPtr;
	NOTE_DELETE(this);
};

static void RefReport(BDataIO& io, atom_ref *ref, const char *prefix)
{
	if (AtomReportLevel() > 1) {
		BString newPrefix(prefix);
		newPrefix += "  ";
		while (ref) {
			io << prefix << ref->id << " " << ref->thid << " " << ref->when << endl;
			ref->stack.LongPrint(io, NULL, newPrefix.String());
			ref = ref->next;
		}
	} else {
		while (ref) {
			io << prefix << ref->id << " " << ref->thid << " " << ref->when << " -- ";
			ref->stack.Print(io);
			io << endl;
			ref = ref->next;
		}
	}
}

static int32 RefCount(atom_ref *ref)
{
	int32 num = 0;
	while (ref) {
		ref = ref->next;
		num++;
	}
	return num;
}

void BAtom::Report(BDataIO& io, const char* prefix, uint32 flags) const
{
	BStringIO str;
	
	debugPtr->Lock();
	if (!(flags&B_ATOM_REPORT_REMOVE_HEADER))
		str << "Report for atom " << this << ": '" << typeid(*this).name() << "'" << endl;
	if (AtomReportLevel() > 0) {
		str << prefix << "Acquires:\n";
		RefReport(str,debugPtr->acquires,"    ");
		str << prefix << "IncRefs:\n";
		RefReport(str,debugPtr->incRefs,"    ");
		str << prefix << "DecRefs:\n";
		RefReport(str,debugPtr->decRefs,"    ");
		str << prefix << "Releases:\n";
		RefReport(str,debugPtr->releases,"    ");
	} else {
		str << prefix << RefCount(debugPtr->acquires) << " Acquires, "
			<< RefCount(debugPtr->incRefs) << " IncRefs, "
			<< RefCount(debugPtr->decRefs) << " DecRefs, "
			<< RefCount(debugPtr->releases) << " Releases" << endl;
	}
	debugPtr->Unlock();
	
	io << str.String();
}

void BAtom::Report(const char* prefix, uint32 flags) const
{
	Report(BOut, prefix, flags);
}

int32 BAtom::IncRefs(void *id)
{
	debugPtr->Lock();

	int32 r = debugPtr->secondary++;

	if (AtomDebugLevel() > 0) {
		atom_ref *ref = new atom_ref;
		ref->id = id;
		ref->thid = find_thread(NULL);
		ref->when = system_time();
		ref->note = NULL;
		ref->stack.Update(1);
		ref->next = debugPtr->incRefs;
		debugPtr->incRefs = ref;
	}

	debugPtr->Unlock();

	if (!debugPtr->primary) {
		WATCH_ACTION(this, "IncRefs after Cleanup!");
	}

	return r;
};

int32 BAtom::DecRefs(void *id)
{
	debugPtr->Lock();
	int32 r = debugPtr->secondary--;
	if (r == 0)
		debugger("DecRefs() called too many times");
	
	if (AtomDebugLevel() > 0) {
		bool found = false;
		if (id) {
			atom_ref *p,**ref;
			for (ref = &debugPtr->incRefs; *ref;ref = &(*ref)->next) {
				if ((*ref)->id == id) {
					p = *ref;
					*ref = (*ref)->next;
					delete p;
					found = true;
					break;
				};
			};
		};
	
		if (!found) {
			atom_ref *ref = new atom_ref;
			ref->id = id;
			ref->thid = find_thread(NULL);
			ref->when = system_time();
			ref->stack.Update(1);
			ref->next = debugPtr->decRefs;
			debugPtr->decRefs = ref;
		};
	}
	
	debugPtr->Unlock();

	if (!debugPtr->primary) {
		WATCH_ACTION(this, "DecRefs() after Cleanup...");
/*
		Wagner::ContentInstance *asInstance = dynamic_cast<Wagner::ContentInstance*>(this);
		if (asInstance) {
			int32 count = asInstance->GetContent()->GetResource()->fQueuedCreateRequests.CountItems();
			if (count) printf("Still has %d listeners!\n",count);
		};
		if (debugPtr->incRefs && debugPtr->incRefs->note) {
			void *p = debugPtr->incRefs->id;
			Wagner::Resource *res = (Wagner::Resource*)p;
			Wagner::URL &url = res->GetURL();
			int32 count = res->fQueuedCreateRequests.CountItems();
			printf("Listeners: %d\n",count);
			printf("Content type: '%s'\n",res->GetContentType());
			printf("State: %d\n",res->GetState());
			url.PrintToStream();
		};
*/
	}

	if (r == 1)	_delete();
	return r;
};

int32 BAtom::Acquire(void *id)
{
	IncRefs(id);

	debugPtr->Lock();

	int32 r = debugPtr->primary++;
	if (r >= INITIAL_PRIMARY_VALUE)
		debugPtr->primary -= INITIAL_PRIMARY_VALUE;

	if (AtomDebugLevel() > 0) {
		atom_ref *ref = new atom_ref;
		ref->id = id;
		ref->thid = find_thread(NULL);
		ref->when = system_time();
		ref->note = NULL;
		ref->stack.Update(1);
		ref->next = debugPtr->acquires;
		debugPtr->acquires = ref;
	}
	
	debugPtr->Unlock();

	if (r >= INITIAL_PRIMARY_VALUE) {
		if (r == INITIAL_PRIMARY_VALUE) Acquired();
		r -= INITIAL_PRIMARY_VALUE;
	}
	
	return r;
};

bool BAtom::AttemptAcquire(void *id)
{
	IncRefs(id);
	
	debugPtr->Lock();

	if (debugPtr->primary <= 0) {
		debugPtr->Unlock();
		DecRefs(id);
		return false;
	}
	
	int32 r = debugPtr->primary++;
	if (r >= INITIAL_PRIMARY_VALUE)
		debugPtr->primary -= INITIAL_PRIMARY_VALUE;

	if (AtomDebugLevel() > 0) {
		atom_ref *ref = new atom_ref;
		ref->id = id;
		ref->thid = find_thread(NULL);
		ref->when = system_time();
		ref->note = NULL;
		ref->stack.Update(1);
		ref->next = debugPtr->acquires;
		debugPtr->acquires = ref;
	}
	
	debugPtr->Unlock();
	
	if (r == INITIAL_PRIMARY_VALUE) Acquired();
	
	return true;
}

int32 BAtom::Release(void *id)
{
	debugPtr->Lock();
	int32 r = debugPtr->primary--;
	if (r == 0)
		debugger("Release() called too many times");

	if (AtomDebugLevel() > 0) {
		bool found = false;
		if (id) {
			atom_ref *p,**ref;
			for (ref = &debugPtr->acquires; *ref;ref = &(*ref)->next) {
				if ((*ref)->id == id) {
					p = *ref;
					*ref = (*ref)->next;
					delete p;
					found = true;
					break;
				};
			};
		};
	
		if (!found) {
			atom_ref *ref = new atom_ref;
			ref->id = id;
			ref->thid = find_thread(NULL);
			ref->when = system_time();
			ref->stack.Update(1);
			ref->next = debugPtr->releases;
			debugPtr->releases = ref;
		};
	}
	
	debugPtr->Unlock();

	if (r == 1)	{
		WATCH_ACTION(this, "Cleanup()");
		Cleanup();
	};
	DecRefs(id);
	return r;
};

int32 BAtom::AcquireCount()
{
	const int32 p = debugPtr->primary;
	if (p == INITIAL_PRIMARY_VALUE) return 0;
	return p;
}

int32 BAtom::IncRefsCount()
{
	return debugPtr->secondary;
}

bool BAtom::CleanupCalled() const
{
	debugPtr->Lock();
	bool andTheAnswerIs = (debugPtr->primary == 0);
	debugPtr->Unlock();
	return andTheAnswerIs;
}

#endif
