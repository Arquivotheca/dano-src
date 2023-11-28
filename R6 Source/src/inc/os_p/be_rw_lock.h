//	be_rw_lock.h

#include <OS.h>

typedef struct rw_lock {
	sem_id		op_sem;
	long		num_readers;
	long		num_writers;
} *rw_lock_id;

#ifdef __cplusplus
extern "C" {
#endif

#if 0

extern rw_lock_id create_rwlock(const char *name);
extern long delete_rwlock(rw_lock_id lock);
extern long acquire_read(rw_lock_id lock);
extern long acquire_write(rw_lock_id lock);
extern long release_rwlock(rw_lock_id lock);
extern long acquire_read_etc(rw_lock_id lock, ulong flags, bigtime_t timeout);
extern long acquire_write_etc(rw_lock_id lock, ulong flags, bigtime_t timeout);

#endif

extern void acquire_spinlock (volatile long *addr);
extern void release_spinlock (volatile long *addr);
extern long test_and_set (long *addr, long val);


#ifdef __cplusplus
}
#endif
