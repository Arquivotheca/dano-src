#include <assert.h>
#include <stdlib.h>
#include <kernel/OS.h>
#include <support/TLS.h>
#include <support/memadviser.h>
#include <os_p/priv_runtime.h>


typedef struct madviser_lock_t {
	int32	lck_count;
	int32	lck_sem;
	int32	lck_tlsrecptr;
	bool	lck_reclaiming_memory;
} madviser_lock_t;

typedef struct function_record {
	void	*function;
	void	*cookie;
	int32	identifier;
	int32	class;

	struct function_record	*next;
} function_record;

#define MADV_FREE_FUNC 0
#define MADV_FAIL_FUNC 1

static int32 next_identifier= 1;
static madviser_lock_t madviser_lock = { -1, -1, -1, false };
static function_record *registered_functions[2] = { 0, 0};


#if _SUPPORTS_FEATURE_MEMORY_ADVISER
void
_initialize_madviser(void)
{
	if(madviser_lock.lck_count>= 0) {
		return;
	}
	if(madviser_lock.lck_sem>= 0) {
		return;
	}
	if(madviser_lock.lck_tlsrecptr>= 0) {
		return;
	}

	madviser_lock.lck_count= 0;
	madviser_lock.lck_sem= create_sem(0, "madviser_lock");
	madviser_lock.lck_tlsrecptr= TLS_MADV_RECURSE;
	madviser_lock.lck_reclaiming_memory= false;

	tls_set(madviser_lock.lck_tlsrecptr, 0);
}

static
bool
REC_LOCK(madviser_lock_t *lck)
{
	int32 tls_rec= (int32)(tls_get(lck->lck_tlsrecptr));
	
	if(tls_rec== 0) {
		if(atomic_add(&lck->lck_count, 1)!= 0) {
			acquire_sem(lck->lck_sem);
		}
	}

	tls_rec++;

	tls_set(lck->lck_tlsrecptr, (void*)(tls_rec));

	return (tls_rec> 1);	// reentering ???
}

static
void
REC_UNLOCK(madviser_lock_t *lck)
{
	int32 tls_rec= (int32)(tls_get(lck->lck_tlsrecptr));

	assert(tls_rec> 0);

	tls_rec--;

	if(tls_rec== 0) {
		if(atomic_add(&lck->lck_count, -1)> 1) {
			release_sem(lck->lck_sem);
		}
	}

	tls_set(lck->lck_tlsrecptr, (void*)(tls_rec));
}
#endif

static
int32
register_function(void *function, void *cookie, int32 class)
{
#if _SUPPORTS_FEATURE_MEMORY_ADVISER
	function_record *curr= (function_record *)malloc(sizeof(function_record));

	if(!curr) {
		return ENOMEM;
	}

	curr->function = function;
	curr->cookie = cookie;
	curr->identifier = atomic_add(&next_identifier, 1);
	curr->class = class;

	REC_LOCK(&madviser_lock);
		curr->next = registered_functions[class];
		registered_functions[class] = curr;
	REC_UNLOCK(&madviser_lock);

	return curr->identifier;
#else
	return 0;
#endif
}

static
void
unregister_function(int32 free_func_token, int32 class)
{
#if _SUPPORTS_FEATURE_MEMORY_ADVISER
	REC_LOCK(&madviser_lock);
	do {
		function_record *list = registered_functions[class];

		if(!list) {
			break;
		}

		if(list->identifier== free_func_token) {
			registered_functions[class] = list->next;
			free(list);
			break;
		}

		while(list->next) {
			if(list->next->identifier== free_func_token) {

				function_record *byebye = list->next;
				list->next = byebye->next;
				free(byebye);

				break;
			}
		}
	} while(0);
	REC_UNLOCK(&madviser_lock);
#endif
}

int32 
register_free_func(memory_adviser_free_func func, void *cookie)
{
	return register_function(func, cookie, MADV_FREE_FUNC);
}

void
unregister_free_func(int32 free_func_token)
{
	return unregister_function(free_func_token, MADV_FREE_FUNC);
}

int32
register_failure_func(memory_adviser_failure_func func, void *cookie)
{
	return register_function(func, cookie, MADV_FAIL_FUNC);
}

void
unregister_failure_func(int32 failure_func_token)
{
	return unregister_function(failure_func_token, MADV_FAIL_FUNC);
}



/*
 * Here comes the actual code --
 *
 * For not slowing down the system a lot when doing
 * small allocations we try to save some calls into
 * the kernel for getting the number of free pages.
 * We'll only call if:
 *
 *		* allocation is bigger the ALLOCATION_WATERMARK
 *		* current allocation plus previous allocations
 *		  will render us with less than RECHECK_RATIO
 *		  of free pages since we last asked the kernel.
 *
 * After that, we'll go round all registered free
 * free_function_hooks if:
 *
 *		* reserved pages + current allocation + MEMORY_RED_ZONE
 *		  is greater than the number of available pages.
 *
 * The reserved pages parameter is calculated as
 * ONGOING_RESERVE_RATIO of previous calls to this function.
 * 
 * After going round all free_function_hooks we'll
 * recheck with the kernel for available pages. If
 * it succeed it's ok, if we fail we retry with
 * B_FREE_ALL.
 *
 * NOTE: before doing all this tests, the current request
 *       is rounded up to two pages
 */
#define ALLOCATION_WATERMARK	(16)
#define RECHECK_RATIO	(2.0/3.0)
#define MEMORY_RED_ZONE	(64)
#define ONGOING_RESERVE_RATIO (1.0/2.0)

#if _SUPPORTS_FEATURE_MEMORY_ADVISER
static int32  reservations= 0;
static int32  cached_free_pages= 0;
static int32  allocated_since_last_check= 0;

static
uint32
count_pages(void)
{
	return cached_free_pages - MEMORY_RED_ZONE - allocated_since_last_check - (int32)(ONGOING_RESERVE_RATIO*reservations);
}

static
uint32
check_pages(void)
{
	system_info sinfo;

	get_system_info(&sinfo);
	cached_free_pages= (sinfo.max_pages - sinfo.used_pages);
	allocated_since_last_check= 0;

	return count_pages();
}

static
bool
trim_pages(uint32 pages, free_level level)
{
	size_t trimmed = 0;
	size_t size_to_trim = pages*B_PAGE_SIZE;

	function_record *list= registered_functions[MADV_FREE_FUNC];
	function_record *iter;

	for(iter= list; (iter) && (trimmed< size_to_trim); iter= iter->next) {
		memory_adviser_free_func func_ptr = iter->function;
		trimmed += (*func_ptr)(iter->cookie, size_to_trim - trimmed, level);
	}

	return trimmed/B_PAGE_SIZE >= pages;
}

static
void
notify_error(size_t bytes, char const *requester)
{
	function_record *list= registered_functions[MADV_FAIL_FUNC];
	function_record *iter;

	for(iter= list; iter; iter= iter->next) {
		memory_adviser_failure_func func_ptr = iter->function;
		(*func_ptr)(iter->cookie, bytes, requester);
	}
}
#endif

bool
madv_reserve_memory(size_t bytes, char const *requester)
{
#if _SUPPORTS_FEATURE_MEMORY_ADVISER
	bool  result= false;
	int32 req_pages= (bytes + 2*B_PAGE_SIZE - 1) / (B_PAGE_SIZE);

	if(!registered_functions[MADV_FREE_FUNC]) {
		/*
		 * if no handlers no work to do
		 */
		return true;
	}

	REC_LOCK(&madviser_lock);
	if(madviser_lock.lck_reclaiming_memory) {
		result= true;
	} else {
		madviser_lock.lck_reclaiming_memory= true;
		do {
			bool  recheck_kernel  = false;
			int32 req_pages_sofar = req_pages+allocated_since_last_check;
			int32 actually_available;


			if(req_pages> ALLOCATION_WATERMARK) {
				recheck_kernel= true;
			}
			if((cached_free_pages - req_pages_sofar)< RECHECK_RATIO*cached_free_pages) {
				recheck_kernel= true;
			}

			if(recheck_kernel) {
				check_pages();
			}

			actually_available= count_pages();

			if(actually_available> req_pages) {
				/*
				 * there is enough for satisfying the request so our work is done.
				 */
				result= true;
				break;
			}

			trim_pages(req_pages-actually_available, B_FREE_CACHED);

			/*
			 * we are paranoid, so we don't trust whatever the hooks
			 * have said they have freed and check again.
			 */
			actually_available= check_pages();

			if(actually_available> req_pages) {
				/*
				 * there is enough for satisfying the request so our work is done.
				 */
				result= true;
				break;
			}

			trim_pages(req_pages-actually_available, B_FREE_ALL);


			actually_available= check_pages();

			if(actually_available> req_pages) {
				/*
				 * there is enough for satisfying the request so our work is done.
				 */
				result= true;
				break;
			}
		} while(0);

		madviser_lock.lck_reclaiming_memory= false;
	}
	if(result) {
		allocated_since_last_check+= req_pages;
		reservations+= req_pages;
	} else {
		notify_error(bytes, requester);
	}
	REC_UNLOCK(&madviser_lock);

	return result;
#else
	return 1;
#endif
}

void
madv_finished_allocating(size_t size_reserved)
{
#if _SUPPORTS_FEATURE_MEMORY_ADVISER
	int32 reserved_pages= (size_reserved + 2*B_PAGE_SIZE -1) / B_PAGE_SIZE;

	REC_LOCK(&madviser_lock);
	{
		reservations-= reserved_pages;

		if(reservations< 0) {
			reservations= 0;	
		}
	}
	REC_UNLOCK(&madviser_lock);
#endif
}


bool
madv_lock(void)
{
#if _SUPPORTS_FEATURE_MEMORY_ADVISER
	return REC_LOCK(&madviser_lock);
#else
	return true;
#endif
}

void
madv_unlock(void)
{
#if _SUPPORTS_FEATURE_MEMORY_ADVISER
	return REC_UNLOCK(&madviser_lock);
#else
	return;
#endif
}
