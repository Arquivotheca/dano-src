/*****************************************************************************

     File: support/DebugLock.cpp

	 Written By: Dianne Hackborn

     Copyright (c) 2001 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#if SUPPORTS_LOCK_DEBUG

#include <support_p/DebugLock.h>

#include <kernel/OS.h>
#include <support/atomic.h>
#include <support/StringIO.h>
#include <support/AssociativeVector.h>
#include <support_p/CallStack.h>
#include <support/OrderedVector.h>
#include <support/String.h>
#include <support/TLS.h>

#include <stdio.h>
#include <stdlib.h>
#include <new>

namespace B {
namespace Support {

#define berr BErr

class StaticStringIO : public BStringIO
{
public:
	inline StaticStringIO() : m_currentIndent(0), m_front(1) { }
	inline ~StaticStringIO() { }
	
	inline StaticStringIO* operator->() { return this; }

	ssize_t WriteIndent()
	{
		static const char space[] =
		"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
		int32 off = sizeof(space) - m_currentIndent - 1;
		if( off < 0 ) off = 0;
		return BStringIO::Write(space+off,m_currentIndent);
	}
	
	void BumpIndentLevel(int32 delta)
	{
		m_currentIndent += delta;
		if (m_currentIndent < 0) m_currentIndent = 0;
	}

	virtual ssize_t Write(const void *buffer, size_t len)
	{
		const char *start,*end;
		ssize_t total = 0, written;
		start = end = reinterpret_cast<const char*>(buffer);
		// Loop while either the next current is not '0' -or- we have
		// been given an exact number of bytes to write.
		while (total >= 0 && len && *start) {
			// Look for end of text or next newline.
			while (len && *end && (*end != '\n')) { len--; end++; };
			
			// If we are going to write the start of a line, first
			// insert an indent.
			if (m_front && atomic_and(&m_front, 0)) {
				written = WriteIndent();
				total = written >= 0 ? (total+written) : written;
			}
			
			// Skip ahead to include all newlines in this section.
			while (len && *end == '\n') {
				len--;
				end++;
				atomic_or(&m_front, 1);
			}
			
			// Write this line of text and get ready to process the next.
			if (total >= 0) {
				written = BStringIO::Write(start,end-start);
				total = written >= 0 ? (total+written) : written;
			}
			start = end;
		}
		return total;
	}

private:
	int32 m_currentIndent;
	int32 m_front;
};

struct link_info
{
	inline link_info()
	{
	}
	inline link_info(DebugLockNode* in_target, DebugLockNode* in_source)
		:	target(in_target), source(in_source), count(1)
	{
		thread = find_thread(NULL);
		stack.Update(3);
	}
	inline link_info(const link_info& o)
		:	target(o.target), source(o.source),
			thread(o.thread), stack(o.stack),
			count(o.count)
	{
	}
	
	DebugLockNode*		target;		// next lock being acquired
	DebugLockNode*		source;		// last lock held while 'target' was acquired
	thread_id			thread;		// thread that created this link
	BCallStack			stack;		// stack of this acquire of 'target'
	int32				count;		// number of times link has occurred.
};

B_IMPLEMENT_SIMPLE_TYPE_FUNCS(link_info);

struct block_links
{
	inline block_links()
		:	targets(link_info(NULL, NULL))
	{
	}
	
	BAssociativeVector<DebugLockNode*, link_info>	targets;
	BOrderedVector<DebugLockNode*>					sources;
};

// ----- Lock debugging level
//#pragma mark -

static int32 gHasLockDebugLevel = 0;
int32 gLockDebugLevel = -1;

int32 LockDebugLevelSlow() {
	if (atomic_or(&gHasLockDebugLevel, 1) == 0) {
		const char* env = getenv("LOCK_DEBUG");
		if (env) {
			gLockDebugLevel = atoi(env);
			if (gLockDebugLevel < 1)
				gLockDebugLevel = 1;
			printf("LOCK DEBUGGING ENABLED!  Debugging level is %ld\n", gLockDebugLevel);
		} else {
			gLockDebugLevel = 0;
		}
		atomic_or(&gHasLockDebugLevel, 2);
	}
	while ((gHasLockDebugLevel&2) == 0) snooze(2000);
	return gLockDebugLevel;
}

// ----- Graph management and access protection
//#pragma mark -

static int32 gGraphLock = 0;
static sem_id gGraphSem = B_BAD_SEM_ID;
static thread_id gGraphHolder = B_BAD_THREAD_ID;
static int32 gGraphNesting = 0;
static int32 gHasGraphSem = 0;
static int32 gLocksHeldTLS = -1;

static void AllocGraphLock();

static inline void LockGlobalGraph() {
	if ((gHasGraphSem&2) == 0) AllocGraphLock();
	const thread_id me = find_thread(NULL);
	if (gGraphHolder != me && atomic_add(&gGraphLock, 1) >= 1)
		acquire_sem(gGraphSem);
	gGraphNesting++;
	gGraphHolder = me;
}

// These functions are used to keep track of all the locks held by
// a running thread.
static inline BOrderedVector<DebugLockNode*>* LocksHeld()
{
	return reinterpret_cast<BOrderedVector<DebugLockNode*>*>(tls_get(gLocksHeldTLS));
}
static inline void AddLockHeld(DebugLockNode* lock)
{
	BOrderedVector<DebugLockNode*>* held = LocksHeld();
	if (!held) {
		held = new(std::nothrow) BOrderedVector<DebugLockNode*>;
		tls_set(gLocksHeldTLS, held);
	}
	if (held) held->InsertValue(lock);
}
static inline void RemLockHeld(DebugLockNode* lock)
{
	BOrderedVector<DebugLockNode*>* held = LocksHeld();
	if (held) {
		held->RemoveValue(lock);
		if (held->CountItems() == 0) {
			delete held;
			tls_set(gLocksHeldTLS, NULL);
		}
	}
}

// Look through the lock graph for a lock 'source' that is connected as
// a source to 'base'.  (That is, if in the past 'base' has been acquired
// while 'source' was held.)
static BVector<const link_info*>* FindSourceLock(	DebugLockNode* source,
													DebugLockNode* base)
{
	if (base->Links()) {
		BVector<const link_info*>* shortest = NULL;
		BVector<const link_info*>* v;
		int32 i = base->Links()->sources.CountItems();
		while (i > 0) {
			--i;
			DebugLockNode* s = base->Links()->sources.ItemAt(i);
			if (s == source) {
				v = new(std::nothrow) BVector<const link_info*>;
				if (!v) debugger("Potential deadlock, but no memory to report it!");
			} else {
				v = FindSourceLock(source, s);
			}
			if (v) {
				v->AddItem(&(s->Links()->targets.LookupValue(base)));
				if (!shortest) {
					shortest = v;
				} else if (shortest->CountItems() > v->CountItems()) {
					delete shortest;
					shortest = v;
				} else {
					delete v;
				}
			}
		}
		
		return shortest;
	}
	return NULL;
}

static inline void UnlockGlobalGraph() {
	if (--gGraphNesting == 0) {
		gGraphHolder = B_BAD_THREAD_ID;
		if (atomic_add(&gGraphLock, -1) > 1)
			release_sem(gGraphSem);
	}
}

static void AllocGraphLock() {
	if (atomic_or(&gHasGraphSem, 1) == 0) {
		gGraphSem = create_sem(0, "lock graph sem");
		gLocksHeldTLS = tls_allocate();
		atomic_or(&gHasGraphSem, 2);
	}
	while ((gHasGraphSem&2) == 0) snooze(2000);
}

// ----- Lock contention tracking
//#pragma mark -

static int32 gContentionLock = 0;
static sem_id gContentionSem = B_BAD_SEM_ID;
static int32 gHasContentionSem = 0;
static BVector<DebugLockNode*>* gHighContention = NULL;
static int32 gNumContention = -1;
static int32 gMinContention = 1;

static void AllocContentionLock();

static inline bool LockContention() {
	if ((gHasContentionSem&2) == 0) AllocContentionLock();
	if (gNumContention == 0) return false;
	if (atomic_add(&gContentionLock, 1) >= 1)
		acquire_sem(gContentionSem);
	return true;
}

static bool RegisterContention(DebugLockNode* who)
{
	if (gHighContention) {
		const int32 C = who->Contention();
		if (C >= gMinContention) {
			const int32 N = gHighContention->CountItems();
			DebugLockNode* node;
			
			// Determine where this item should go.
			ssize_t mid, low = 0, high = N-1;
			while (low <= high) {
				mid = (low + high)/2;
				node = gHighContention->ItemAt(mid);
				if (node == NULL || node->Contention() < C) {
					high = mid-1;
				} else if (node->Contention() > C) {
					low = mid+1;
				} else {
					do {
						low++;
						if (low < N) node = gHighContention->ItemAt(low);
						else node = NULL;
					} while (node && node->Contention() == C);
					high = low-1;
				}
			}
			
			if (low < N) {
				if (N >= gNumContention) {
					node = gHighContention->ItemAt(N-1);
					node->DecRefs();
					gHighContention->RemoveItem(N-1);
				}
				gHighContention->InsertItem(who, low);
				return true;
			}
		}
	}
	return false;
}

static inline void UnlockContention() {
	if (gNumContention == 0) return;
	if (atomic_add(&gContentionLock, -1) > 1)
		release_sem(gContentionSem);
}

static void AllocContentionLock() {
	if (atomic_or(&gHasContentionSem, 1) == 0) {
		const char* env = getenv("LOCK_CONTENTION");
		if (env) {
			gNumContention = atoi(env);
			if (gNumContention < 1)
				gNumContention = 1;
			gHighContention = new(std::nothrow) BVector<DebugLockNode*>;
			if (gHighContention) {
				gHighContention->Reserve(gNumContention);
				for (int32 i=0; i<gNumContention; i++)
					gHighContention->AddItem(NULL);
			}
		}
		if (gNumContention > 0) gContentionSem = create_sem(0, "lock contention sem");
		atomic_or(&gHasContentionSem, 2);
	}
	while ((gHasContentionSem&2) == 0) snooze(2000);
}

// ----- Locking implementation
//#pragma mark -

DebugLockNode::DebugLockNode(	const char* type, const void* addr,
								const char* name, uint32 flags)
	:	m_refs(1), m_type(type), m_addr(addr), m_name(name), m_globalFlags(flags),
		m_createStack(LockDebugLevel() >= 5 ? new(std::nothrow) BCallStack : NULL),
		m_links(LockDebugLevel() >= 10 ? new(std::nothrow) block_links : NULL)
{
	if (m_createStack) m_createStack->Update(2);
}

DebugLockNode::~DebugLockNode()
{
	if (LockGraph()) {
		// Remove associations with all other locks.
		const int32 NT = m_links->targets.CountItems();
		const int32 NS = m_links->sources.CountItems();
		int32 i;
		for (i=0; i<NT; i++) {
			DebugLockNode* target = m_links->targets.KeyAt(i);
			if (target && target->m_links)
				target->m_links->sources.RemoveValue(this);
		}
		for (i=0; i<NS; i++) {
			DebugLockNode* source = m_links->sources.ItemAt(i);
			if (source && source->m_links)
				source->m_links->targets.RemoveKey(this);
		}
		RemLockHeld(this);
		UnlockGraph();
	}
	
	delete m_links;
	delete m_createStack;
}

void DebugLockNode::Delete()
{
	const bool taken = LockContention() ? RegisterContention(this) : false;
	UnlockContention();
	
	if (!taken) DecRefs();
}

int32 DebugLockNode::IncRefs() const
{
	return atomic_add(&m_refs, 1);
}

int32 DebugLockNode::DecRefs() const
{
	const int32 c = atomic_add(&m_refs, 1);
	if (c == 1) delete const_cast<DebugLockNode*>(this);
	return c;
}

const char* DebugLockNode::Type() const
{
	return m_type.String();
}

const void* DebugLockNode::Addr() const
{
	return m_addr;
}

const char* DebugLockNode::Name() const
{
	return m_name.String();
}

uint32 DebugLockNode::GlobalFlags() const
{
	return m_globalFlags;
}

const BCallStack* DebugLockNode::CreationStack() const
{
	return m_createStack;
}

inline bool DebugLockNode::LockGraph() const
{
	if (!m_links) return false;
	LockGlobalGraph();
	return true;
}

inline void DebugLockNode::UnlockGraph() const
{
	UnlockGlobalGraph();
}

bool DebugLockNode::AddToGraph()
{
	bool succeeded = true;
	
	// Add this block to the block graph, checking to see if it creates
	// a cycle.
	BOrderedVector<DebugLockNode*>* held = LocksHeld();
	if (held) {
		const int32 N = held->CountItems();
		for (int32 i=0; i<N; i++) {
			DebugLockNode* source = held->ItemAt(i);
			if (source && source->m_links) {
			
				const int32 idx = source->m_links->targets.FindKey(this);
				BVector<const link_info*>* deadlock = NULL;
				if (idx < 0) {
					// This dependency does not currently exist.  We need
					// to add it, but before doing so make sure it won't create
					// a deadlock -- we never want to have a cyclic graph.
					deadlock = FindSourceLock(this, source);
					if (!deadlock) {
						link_info info(this, source);
						source->m_links->targets.InsertValue(this, info);
					} else {
						// NOTE THAT THERE ARE MANY POTENTIAL DEADLOCKS HERE!
						// We really shouldn't do anything that might acquire another
						// lock while holding the graph lock...  however, we're just
						// doing this to -report- a friggin' deadlock, so who cares??
						StaticStringIO msg;
						msg << "Thread " << find_thread(NULL)
							<< " created a potential deadlock!" << endl << endl
							<< "While holding " << *source << endl;
						msg->BumpIndentLevel(1);
						source->PrintStacksToStream(msg);
						msg->BumpIndentLevel(-1);
						msg	<< "attempting to acquire " << *this << endl << endl
							<< "Previously these were acquired as:" << endl;
						msg->BumpIndentLevel(1);
						const int32 N = deadlock->CountItems();
						for (int32 i=0; i<N; i++) {
							const link_info* info = deadlock->ItemAt(i);
							msg << "Holding " << (*info->source) << endl
								<< "acquired " << (*info->target) << endl;
							msg->BumpIndentLevel(1);
							msg << "Occurred " << info->count
								<< " times; first seen in thread "
								<< info->thread << ":" << endl;
							info->stack.LongPrint(msg);
							msg->BumpIndentLevel(-1);
						}
						msg->BumpIndentLevel(-1);
						berr << msg->String();
						delete deadlock;
						UnlockGlobalGraph();
						debugger(msg->String());
						LockGlobalGraph();
						succeeded = false;
					}
				} else {
					source->m_links->targets.EditValueAt(idx).count++;
				}
				if (deadlock == NULL && m_links->sources.FindValue(source) < 0) {
					m_links->sources.InsertValue(source);
				}
			}
		}
	}
	
	return succeeded;
}

void DebugLockNode::RegisterAsHeld()
{
	AddLockHeld(this);
}

void DebugLockNode::UnregisterAsHeld()
{
	RemLockHeld(this);
}

int32 DebugLockNode::Contention() const
{
	return 0;
}

void DebugLockNode::PrintToStream(BDataIO& io) const
{
	io << m_type << "(" << m_addr << " \"" << m_name << "\"";
	PrintSubclassToStream(io);
	io << ")";
}

void DebugLockNode::PrintContentionToStream(BDataIO& io)
{
	bool found = false;
	if (LockContention() && gHighContention) {
		const int32 N = gHighContention->CountItems();
		StaticStringIO msg;
		for (int32 i=0; i<N; i++) {
			DebugLockNode* node = gHighContention->ItemAt(i);
			if (node) {
				if (!found) {
					msg << "Locks with highest contention:" << endl;
					found = true;
				}
				msg->BumpIndentLevel(1);
				msg << *node << endl;
				if (node->CreationStack()) {
					msg << "Created at:" << endl;
					msg->BumpIndentLevel(1);
					node->CreationStack()->LongPrint(msg);
					msg->BumpIndentLevel(-1);
				}
				msg->BumpIndentLevel(-1);
			}
		}
		io << msg->String();
	}
	UnlockContention();
}

void DebugLockNode::PrintStacksToStream(StaticStringIO& io) const
{
	if (CreationStack()) {
		io << "Created at:" << endl;
		io->BumpIndentLevel(1);
		CreationStack()->LongPrint(io);
		io->BumpIndentLevel(-1);
	}
}

void DebugLockNode::PrintSubclassToStream(BDataIO&) const
{
}

DebugLock::DebugLock(	const char* type, const void* addr,
						const char* name, uint32 debug_flags)
	:	DebugLockNode(type, addr, name, debug_flags),
		m_sem(create_sem(1, name)), m_owner(B_ERROR),
		m_ownerStack(CreationStack() ? new(std::nothrow) BCallStack : NULL),
		m_contention(0), m_maxContention(0), m_deleted(false)
{
}

DebugLock::~DebugLock()
{
	if (!m_deleted) delete_sem(m_sem);
}

void DebugLock::Delete()
{
	// When the lock is held, deleting it is an error if:
	// * The LOCK_ANYONE_CAN_DELETE flag is not set; and
	//   * This thread is not the current owner of the lock; or
	//   * The LOCK_CAN_DELETE_WHILE_HELD flag is not set.
	if (m_owner >= B_OK) {
		if ((GlobalFlags()&LOCK_ANYONE_CAN_DELETE) == 0
				&& ((GlobalFlags()&LOCK_CAN_DELETE_WHILE_HELD) == 0
					|| m_owner != find_thread(NULL))) {
			StaticStringIO msg;
			msg << "Thread " << find_thread(NULL) << " deleting " << *this << endl
				<< "Currently held by thread " << m_owner << endl;
			PrintStacksToStream(msg);
			berr << msg->String();
			debugger(msg->String());
		}
		if (m_owner == find_thread(NULL)) {
			if (LockGraph()) {
				UnregisterAsHeld();
				UnlockGraph();
			}
		} else {
			debugger("DebugLock: need to implement LOCK_ANYONE_CAN_DELETE!");
		}
	}
	
	m_deleted = true;
	delete_sem(m_sem);
	
	DebugLockNode::Delete();
}

int32 DebugLock::Contention() const
{
	return m_maxContention;
}

status_t DebugLock::Lock(uint32 flags, bigtime_t timeout, uint32 debug_flags)
{
	if (m_owner == find_thread(NULL)) {
		StaticStringIO msg;
		msg << "Thread " << m_owner << " attempted to acquire " << *this
			<< " multiple times" << endl;
		PrintStacksToStream(msg);
		berr << msg->String();
		debugger(msg->String());
	}
	
	if (timeout != 0
			&& ((GlobalFlags()|debug_flags)&LOCK_SKIP_DEADLOCK_CHECK) == 0
			&& LockGraph()) {
		AddToGraph();
		UnlockGraph();
	}
	
	const int32 contention = atomic_add(&m_contention, 1);
	
	status_t err;
	while ((err=acquire_sem_etc(m_sem, 1, flags, timeout)) == B_INTERRUPTED) ;
	
	if (err == B_OK) {
		if (((GlobalFlags()|debug_flags)&LOCK_DO_NOT_REGISTER_HELD) == 0
				&& LockGraph()) {
			RegisterAsHeld();
			UnlockGraph();
		}
		m_owner = find_thread(NULL);
		m_ownerFlags = debug_flags;
		if (m_ownerStack) m_ownerStack->Update(2);
	} else if (err != B_TIMED_OUT && (timeout != 0 || err != B_WOULD_BLOCK)) {
		StaticStringIO msg;
		if (err != B_BAD_SEM_ID) msg << "Error \"";
		else msg << "Warning \"";
		msg << strerror(err) << "\" attempting to acquire " << *this << endl;
		PrintStacksToStream(msg);
		berr << msg->String();
		if (err != B_BAD_SEM_ID) debugger(msg->String());
	}
	
	if (contention >= m_maxContention) m_maxContention = contention+1;
	
	return err;
}

status_t DebugLock::Unlock()
{
	if (m_owner < B_OK) {
		StaticStringIO msg;
		msg << "Thread " << find_thread(NULL) << " releasing "
			<< *this << " that is not held" << endl;
		PrintStacksToStream(msg);
		berr << msg->String();
		debugger(msg->String());
	} else if (m_owner != find_thread(NULL)) {
		if (((GlobalFlags()|m_ownerFlags)&LOCK_ANYONE_CAN_UNLOCK) == 0) {
			StaticStringIO msg;
			msg << "Thread " << find_thread(NULL) << " releasing " << *this << endl
				<< "Held by thread " << m_owner << endl;
			PrintStacksToStream(msg);
			berr << msg->String();
			debugger(msg->String());
		}
	}
	
	m_owner = B_ERROR;
	
	if (LockGraph()) {
		UnregisterAsHeld();
		UnlockGraph();
	}
	
	status_t err;
	while ((err=release_sem(m_sem)) == B_INTERRUPTED) ;
	
	atomic_add(&m_contention, -1);
	
	if (err != B_OK) {
		StaticStringIO msg;
		msg << "Error \"" << strerror(err) << "\" attempting to release " << *this;
		PrintStacksToStream(msg);
		debugger(msg->String());
	}
	
	return err;
}

const BCallStack* DebugLock::OwnerStack() const
{
	return m_ownerStack;
}

sem_id DebugLock::Semaphore() const
{
	return m_sem;
}

void DebugLock::PrintStacksToStream(StaticStringIO& io) const
{
	DebugLockNode::PrintStacksToStream(io);
	if (OwnerStack() && m_owner >= B_OK) {
		io << "Locked by thread " << m_owner << " at:" << endl;
		io->BumpIndentLevel(1);
		OwnerStack()->LongPrint(io);
		io->BumpIndentLevel(-1);
	}
}

void DebugLock::PrintSubclassToStream(BDataIO& io) const
{
	io << ", sem=" << m_sem;
}

} }	// namespace B::Support

#endif	// SUPPORTS_LOCK_DEBUG
