/*	tr_debug.cpp	*/

#include <OS.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "tr_debug.h"


#if DEBUG_BUFFER_REFS

struct buf_info {
	sem_id p_sem;
	int32	request_count[64];
	int32	got_count[64];
	int32	sent_count[64];
	int32	recycled_count[64];
};
static buf_info * g_ptr;

void debug_attach()
{
	area_id id = find_area("tr_debug_buf_data");
	if (id < 0) {
		dlog("create_area %s", "tr_debug_buf_data");
		id = create_area("tr_debug_buf_data", (void**)&g_ptr, B_ANY_ADDRESS, 
			B_PAGE_SIZE, B_NO_LOCK, B_READ_AREA|B_WRITE_AREA);
		if (id < 0) {
			dlog("can't create buf debug area: %x", id);
			return;
		}
		memset(g_ptr, 0, sizeof(*g_ptr));
		g_ptr->p_sem = create_sem(1, "debug print sem");
	}
	else {
		char name[32];
		sprintf(name, "tr_debug_buf_data:%d", find_thread(NULL));
		dlog("clone_area %s", name);
		id = clone_area(name, (void**)&g_ptr, B_ANY_ADDRESS, 
			B_READ_AREA|B_WRITE_AREA, id);
		if (id < 0) {
			dlog("can't clone buf debug area: %x", id);
			return;
		}
	}
}

void debug_request_buf(int id)
{
	if (!g_ptr) return;
	if (atomic_add(&g_ptr->request_count[id&63], 1) != g_ptr->recycled_count[id&63]) {
		printf("Buffer count discrepancy seen in request_buf (id %d)!!!\n", id); 
		debug_dump_bufs();
		DEBUGGER("buffer count error\n");
	}
}

void debug_got_buf(int id)
{
	if (!g_ptr) return;
	if (atomic_add(&g_ptr->got_count[id&63], 1) != g_ptr->sent_count[id&63]-1) {
		printf("Buffer got/sent count discrepancy seen in got_buf (id %d)!!!\n", id); 
		debug_dump_bufs();
		DEBUGGER("buffer count error\n");
	}
}

void debug_sent_buf(int id)
{
	if (!g_ptr) return;
	if (atomic_add(&g_ptr->sent_count[id&63], 1) != g_ptr->got_count[id&63]) {
		printf("Buffer got/sent count discrepancy seen in sent_buf (id %d)!!!\n", id); 
		debug_dump_bufs();
		DEBUGGER("buffer count error\n");
	}
}

void debug_recycled_buf(int id)
{
	if (!g_ptr) return;
	int32 i = atomic_add(&g_ptr->recycled_count[id&63], 1);
	if (g_ptr->got_count[id&63] != g_ptr->sent_count[id&63]) {
		printf("Buffer got/sent count discrepancy seen in recycled_buf (id %d)!!!\n", id); 
		debug_dump_bufs();
		DEBUGGER("buffer count error\n");
	}
	else if (i != g_ptr->request_count[id&63]-1) {
		printf("Buffer request count discrepancy seen in recycled_buf (id %d)!!!\n", id); 
		debug_dump_bufs();
		DEBUGGER("buffer count error\n");
	}
}

void debug_dump_bufs()
{
	if (!g_ptr) return;
	acquire_sem(g_ptr->p_sem);
	thread_info tinfo;
	thread_id t = find_thread(NULL);
	get_thread_info(t, &tinfo);
	fprintf(stderr, "thread %d: %s\n", t, tinfo.name);
	fprintf(stderr, "id  send  recv  requ  recy\n");
	for (int ix=0; ix<64; ix++) {
		fprintf(stderr, "%2d: %4d  %4d  %4d  %4d\n", ix, g_ptr->sent_count[ix], 
			g_ptr->got_count[ix], g_ptr->request_count[ix], g_ptr->recycled_count[ix]);
	}
	release_sem(g_ptr->p_sem);
}

#endif


port_id _dlogger::log_port = 0;

_dlogger::_dlogger(const char * f, int l)
{
	if (log_port < 0) return;	/* short-circuit no-logger case */
	when = system_time();
	thread = find_thread(NULL);
	line = l;
	strncpy(file, f, 32);
}


void _dlogger::message(const char * fmt, ...)
{
	if (log_port < 0) return;	/* short-circuit no-logger case */
	va_list l;
	va_start(l, fmt);
	vsprintf(msg, fmt, l);
	va_end(l);
	if (log_port == 0) {
		log_port = find_port("tr_debug dlogger port");
	}
	if (log_port > 0) {
		if (write_port_etc(log_port, 1, this, sizeof(*this), B_TIMEOUT, 3000) == 
			B_TIMED_OUT) {
			fprintf(stderr, "ERROR: timed out in dlog() write_port() [%ld %s@%ld]\n", thread, file, line);
		}
	}
}

bool
_dlogger::_check_fail(
	const char *f, 
	int l, 
	const char *x)
{
	_dlogger(f, l).message("DCHECK FAILED: %s", x);
	return false;
}

