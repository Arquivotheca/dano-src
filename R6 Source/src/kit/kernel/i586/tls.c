#include <OS.h>
#include <TLS.h>

#include "priv_syscalls.h"
#include "priv_runtime.h"


static int32	next_item = TLS_FIRST_AVAIL;

int32
tls_allocate(void)
{
	return atomic_add(&next_item, 1);
}

void
_init_tls(void)
{
	area_info	info;
	void		**base;

	get_area_info(area_for(&info), &info);
	base = (void **)(info.address + 0x1000);
	tls_set(TLS_BASE_PTR, base);
	tls_set(TLS_THREAD_ID, (void *)_kfind_thread_(NULL));
	tls_set(TLS_MADV_RECURSE, 0);
	tls_set(TLS_MALLOC_RECURSE, 0);
}

/*
 * left for backward compatiblity, as it is referenced from
 * the start function of old (4.5 or older) code.
 */

int32		_data_offset_main_;
