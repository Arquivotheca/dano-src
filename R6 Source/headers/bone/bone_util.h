/*
	bone_util.h
	
	networking utilities module
	
	Copyright 1999-2001, Be Incorporated, All Rights Reserved.
*/

#ifndef H_BONE_UTIL
#define H_BONE_UTIL

#include <BeBuild.h>
#include <SupportDefs.h>
#include <KernelExport.h>
#include <module.h>
#include <bone_data.h>
#include <ByteOrder.h>
#include <profiler.h>
#include <bone_endpoint_api.h>
#include <bone_observer.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * benaphore
 */
typedef struct bone_benaphore
{
	int32	atom;
	sem_id	sem;
} bone_benaphore_t;


/*
 * multi-reader/single-writer lock
 */
typedef struct bone_rwlock
{
	uint32 		counts;
	thread_id 	writer;
	uint32		level;
	
	sem_id readerSem;
	sem_id writerSem1;
	sem_id writerSem2;
} bone_rwlock_t;


typedef struct bone_fifo
{
	bone_benaphore_t    lock;
	sem_id				sync;
	volatile int32		waiting;
	volatile int32		interrupt;
	
	ssize_t 			max_bytes;
	size_t				current_bytes;
	
	bone_data_t 		*head;
	bone_data_t 		*tail;
} bone_fifo_t;

struct bone_proto_info;
struct bone_dl_proto_info;
struct bone_endpoint;

struct bone_timer;
struct mempool;
struct bone_dl;
struct address_spec;

typedef struct bone_timer bone_timer_t;
typedef void (*bone_timer_hook)(struct bone_timer *, void *);

typedef struct bone_util_info
{
	struct module_info info;
	
	/*
	 * bone_data utilities
	 */
	
	/* creation and destruction */
	bone_data_t	*	(*new_data)(void);
	void 			(*delete_data)(bone_data_t *bdt);
	void 			(*isafe_delete_data)(bone_data_t *bdt);
	int				(*add_free_element)(bone_data_t *bdt, void *arg1, void *arg2, void *free_func, free_flags_t flags);
	
	/* data-to-data replication */
	int				(*copy_data)(bone_data_t *to, bone_data_t *from, uint32 offset, uint32 bytes);
	int				(*clone_data)(bone_data_t *to, bone_data_t *from, uint32 offset, uint32 bytes);
	int				(*coalesce_data)(bone_data_t *bdt, uint32 offset, uint32 bytes);
	
	/* data insertion and removal */
	int				(*prepend_data)(bone_data_t *bdt, const void *data, uint32 length, freefunc freethis);			 
	int 			(*append_data)(bone_data_t *bdt, const void *data, uint32 length, freefunc freethis);
	int 			(*insert_data)(bone_data_t *bdt, uint32 offset, const void *data, uint32 length, freefunc freethis);
	int 			(*trim_data)(bone_data_t *bdt, uint32 offset, uint32 bytes);
	
	/* data retrieval */
	int				(*get_data_memory_map)(bone_data_t *bdt, struct iovec *iov, uint32 numiov, bool cache_only);
	uint32 			(*get_data_iovecs)(const bone_data_t *bdt, struct iovec *iovs, int32 numiov);
	uint32 			(*count_data_iovecs)(const bone_data_t *bdt);
	
	uint32 			(*copy_from_data)(const bone_data_t *bdt, uint32 offset, void *copyinto, uint32 bytes);
	int 			(*map_to_data)(bone_data_t *bdt, uint32 offset, void **mystruct, uint32 mystructlen);
	int				(*disassociate_data)(bone_data_t *bdt, uint32 offset, uint32 bytes);
	
	/* data checksums */
	uint16 			(*checksum_data)(const bone_data_t *bdt, uint32 offset, uint32 bytes);
	uint16			(*copy_and_checksum_data)(const bone_data_t *bdt, uint32 offset, void *copyinto, uint32 bytes);
	uint16      	(*checksum)(void *buf, size_t len, uint16 prev_sum);
	uint16      	(*incremental_checksum)(uint16 oldsum, uint16 oldval, uint16 newval);
	
	/* data memory locking */
	struct bone_dl *(*lock_data)(bone_data_t *bdt);
	void			(*unlock_data)(struct bone_dl *lock);
	
	/*
	 *	benaphore utilities
	 */
	
	int 		(*create_benaphore)(bone_benaphore_t *bbt, const char *name);
	void		(*delete_benaphore)(bone_benaphore_t *bbt);
	
	status_t	(*lock_benaphore)(bone_benaphore_t *bbt);
	status_t	(*unlock_benaphore)(bone_benaphore_t *bbt);
	
	/*
	 *	timer utilities
	 */
	
	struct bone_timer *(*create_timer)(bone_timer_hook hook, void *arg);
	void 		(*delete_timer)(struct bone_timer *timer);
	void 		(*set_timer_hook)(struct bone_timer *timer, bone_timer_hook hook, void *arg);

	void		(*set_timer)(struct bone_timer *t, bigtime_t delay, bool cancel);
	void 		(*cancel_timer)(struct bone_timer *t);
	bigtime_t	(*timer_appointment)(struct bone_timer *t);
	
	/*
	 * read-write lock utilities
 	 */

	status_t	(*create_rwlock)(struct bone_rwlock *out, const char *name);
	void 		(*delete_rwlock)(struct bone_rwlock *lock);
	
	status_t 	(*read_lock)(struct bone_rwlock *lock);
	status_t 	(*read_unlock)(struct bone_rwlock *lock);
	status_t 	(*write_lock)(struct bone_rwlock *lock);
	status_t	(*write_unlock)(struct bone_rwlock *lock);
	status_t	(*upgrade_to_write)(struct bone_rwlock *lock);
	status_t	(*downgrade_to_read)(struct bone_rwlock *lock);
	
	/*
	 * fifo utilities
	 */
	
	status_t	(*create_fifo)(bone_fifo_t *out, const char *name, ssize_t max_bytes);
	void		(*delete_fifo)(struct bone_fifo *bft);
	status_t	(*enqueue_fifo_data)(struct bone_fifo *bft, bone_data_t *datum);
	ssize_t    	(*dequeue_fifo_data)(struct bone_fifo *bft, bone_data_t **data_ptr, bigtime_t timeout, bool peek);
	status_t	(*clear_fifo)(struct bone_fifo *bft);
	
	/*
	 *	masked compare and copy
	 */
	
	int 		(*masked_cmp)(const char *buf1, const char *buf2, const char *mask, int32 len);
	void		(*masked_copy)(char *to, const char *from, const char *mask, int32 len);
	
	/*
	 * module roster functions
	 */
	status_t (*get_outbound_protos)(int family, int type, int proto, struct bone_proto_info ***outStack, int *outNum);
	status_t (*get_datalink_protos)(int family, int ifType, int flavor, struct bone_dl_proto_info ***outStack, int *outNum);
	status_t (*get_inbound_proto)(int family, int proto, struct bone_proto_info **outProto);
	status_t (*get_family_ref)(int family);
	status_t (*release_family_ref)(int family);

	/*
	 * config file functions
	 */

	int32		(*get_config_int32)(const char *key, int32 def);
	bool		(*get_config_bool)(const char *key, bool def);
	
	/*
	 * allocator utilities
	 */
	
	void		*(*falloc)(size_t size); /* fast mempool allocater; expected life < 1000 us */
	void		*(*zalloc)(size_t size); /* zero'd mempool allocater */
	void		*(*malloc)(size_t size); /* zero'd general malloc() */
	void		(*free)(void *mem);
	
	/*
	 * mempool utilities
	 */
	
	struct mempool 	*(*new_mempool)(void *base, size_t node_size, int32 node_count);
	void 			(*delete_mempool)(struct mempool *pool);
	void 			*(*mp_alloc)(struct mempool *pool);
	void 			(*mp_free)(struct mempool *pool, void *ptr);
	
	/*
	 * virtual-to-physical address cache utilities
	 */
	
	struct address_spec 	*(*cache_address)(const void *address, uint32 length, thread_id id);
	void					(*uncache_address)(struct address_spec *spec);
	int						(*get_cached_memory_map)(const void *address, uint32 length, struct iovec *iovs, int32 numiov);
	
	/*
	 * profiler
	 */
	
	void 		(*enter_prof_func)(const char *file, const char *func, int flags);
	void 		(*exit_prof_func)(const char *file, const char *func, int flags);
	void 		(*log_data_path)(bone_data_t *data, const char *what);
	void		(*print_memuse)(void);
	
	/*
	 *	endpoint utilities
	 */
	
	status_t			(*new_endpoint)(struct bone_endpoint **ep_ptr, int family, int type, int proto);
	void 				(*delete_endpoint)(struct bone_endpoint *ep);
	struct bone_endpoint *(*spawn_endpoint)(struct bone_endpoint *parent);
	struct bone_endpoint *(*dequeue_endpoint)(struct bone_endpoint *parent);
	void				(*ready_endpoint)(struct bone_endpoint *child);
	void				(*set_backlog)(struct bone_endpoint *ep, int backlog);
	void				(*notify_endpoint)(struct bone_endpoint *ep, notify_event_t what, int32 value);

	/*
	 * Async notification utilities
	 */

	status_t 	(*notify)(uint32 who, uint32 what, const void *data, size_t len);
	
	
} bone_util_info_t;

#define BONE_UTIL_MOD_NAME "network/bone_util"
#define BONE_PEEPER_MOD_NAME "network/bone_peeper"


#ifdef __cplusplus
}
#endif


#endif /* H_BONE_UTIL */       
