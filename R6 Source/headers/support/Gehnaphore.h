#ifndef GEHNAPHORE_H
#define GEHNAPHORE_H

#include <OS.h>
#include <SupportDefs.h>
#include <OS.h>

class NestedGehnaphore;

void init_gehnaphore(int32* value);
void lock_gehnaphore(int32* value);
void unlock_gehnaphore(int32* value);

class NestedGehnaphore;

class Gehnaphore {

	private:
	
				friend class 	NestedGehnaphore;
				int32					m_lockValue;
	
	public:
	
										Gehnaphore();
										Gehnaphore(const char *name);
										~Gehnaphore();

				void					Lock();
				void					Yield();
				void					Unlock();
};

class NestedGehnaphore : protected Gehnaphore {

	private:
	
				int32					m_nesting;
				int32					m_owner;

	public:
	
										NestedGehnaphore();
										~NestedGehnaphore();

				bool					IsLocked();
				void					Lock();
				void					Yield();
				void					Unlock();
};

class RWGehnaphore {

	private:
	
				struct rw_data {
					unsigned int readerThread 		: 31;
					unsigned int writerActive 		: 1;
					unsigned int writerWaitingOn 	: 16;
					unsigned int readerCount 		: 16;
				};

				Gehnaphore				m_write;
				int32					m_writerThread;
				rw_data					m_extraValue;
	
	public:
	
										RWGehnaphore() { m_writerThread = 0; *((int64*)&m_extraValue) = 0; };
										~RWGehnaphore() {};

				void					ReadLock();
				void					ReadUnlock();

				void					WriteLock();
				void					WriteUnlock();
};

/*
class RWGehnaphore : public Gehnaphore {

	public:
	
										RWGehnaphore() {};
										~RWGehnaphore() {} ;

				void					ReadLock() { Lock(); };
				void					ReadUnlock() { Unlock(); };

				void					WriteLock() { Lock(); };
				void					WriteUnlock() { Unlock(); };
};
*/

class GehnaphoreAutoLock {

	private:

				Gehnaphore &			m_lock;
	public:
	
										GehnaphoreAutoLock(Gehnaphore &lock) : m_lock(lock) { m_lock.Lock(); }
										~GehnaphoreAutoLock() { m_lock.Unlock(); };
};

class NestedGehnaphoreAutoLock {

	private:

				NestedGehnaphore &		m_lock;
	public:
	
										NestedGehnaphoreAutoLock(NestedGehnaphore &lock) : m_lock(lock) { m_lock.Lock(); }
										~NestedGehnaphoreAutoLock() { m_lock.Unlock(); };
};

class RWGehnaphoreAutoReadLock {

	private:

				RWGehnaphore &			m_lock;
	public:
	
										RWGehnaphoreAutoReadLock(RWGehnaphore &lock) : m_lock(lock) { m_lock.ReadLock(); }
										~RWGehnaphoreAutoReadLock() { m_lock.ReadUnlock(); };
};

class RWGehnaphoreAutoWriteLock {

	private:

				RWGehnaphore &			m_lock;
	public:
	
										RWGehnaphoreAutoWriteLock(RWGehnaphore &lock) : m_lock(lock) { m_lock.WriteLock(); }
										~RWGehnaphoreAutoWriteLock() { m_lock.WriteUnlock(); };
};

#endif
