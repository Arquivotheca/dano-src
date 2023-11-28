#include <OS.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "tr_debug.h"
#include <signal.h>


#define THREAD_PRIO 30


static void
format_msg(
	_dlogger & msg,
	char * output)
{
	msg.file[31] = 0;
	msg.msg[79] = 0;
	sprintf(output, "%012Lx:%d\t%s@%d\t%s\n", msg.when, msg.thread, msg.file, 
		msg.line, msg.msg);
}


int32 front_cnt = 0;
int32 back_cnt = 0;
sem_id free_sem;
int32 free_ben = 1024;
_dlogger msgs[1024];
bool g_running;
sem_id wake_sem;



static void
do_quit(
	int signal)
{
	g_running = false;
	release_sem(wake_sem);
}


static void
setup_record()
{
	free_sem = create_sem(0, "record_area throttle");
	wake_sem = create_sem(0, "record_thread wakeup");
}


char big_buf[70000];


static status_t
record_thread(
	void * data)
{
	signal(SIGINT, do_quit);
	signal(SIGHUP, do_quit);
	bool tty = isatty(1);
	fprintf(stderr, "printing to a %s\n", tty ? "Terminal window" : "file");
	data = data;
	bigtime_t sn_time = 100000;
	char * ptr = big_buf;
	while (g_running) {
		acquire_sem_etc(wake_sem, 1, B_TIMEOUT, sn_time);
		int32 diff = front_cnt-back_cnt;
		if (diff > 32) {
			sn_time /= 2;
		}
		else if (diff < 2) {
			if (sn_time < 400000) {
				sn_time *= 2;
			}
			else if (tty && (ptr > big_buf)) {
				write(1, big_buf, ptr-big_buf);
				ptr = big_buf;
			}
		}
		while (back_cnt < front_cnt) {
			format_msg(msgs[back_cnt & 1023], ptr);
			ptr += strlen(ptr);
			if (ptr >= &big_buf[65536]) {
				write(1, big_buf, 65536);
				memcpy(big_buf, &big_buf[65536], (ptr-big_buf)-65536);
				ptr -= 65536;
			}
			back_cnt++;
		}
		if (diff > 0) {
			int32 n = atomic_add(&free_ben, diff);
			if (n < 0) {
				release_sem_etc(free_sem, -n, B_DO_NOT_RESCHEDULE);
			}
		}
	}
	if (ptr > big_buf) {
		write(1, big_buf, ptr-big_buf);
	}
	return 0;
}


static void
record_msg(
	_dlogger & msg)
{
	if (atomic_add(&free_ben, -1) <= 0) {
		acquire_sem(free_sem);
	}
	msgs[front_cnt&1023] = msg;
	front_cnt++;
	int diff = front_cnt-back_cnt;
	if (!(diff & 0x3f)) {
		release_sem(wake_sem);
	}
}


int
main()
{
	signal(SIGINT, do_quit);
	signal(SIGHUP, do_quit);

	port_id p = find_port("tr_debug dlogger port");
	if (p >= 0) {
		fprintf(stderr, "Logger port already exists; exiting\n");
		exit(1);
	}
	if (p <= 0) {
		p = create_port(20, "tr_debug dlogger port");
	}
	if (p <= 0) {
		fprintf(stderr, "Cannot create logger port; exiting\n");
		exit(1);
	}
	g_running = true;
	setup_record();
	thread_id tr;
	resume_thread((tr = spawn_thread(record_thread, "record_thread", THREAD_PRIO/2, NULL)));
	set_thread_priority(find_thread(NULL), THREAD_PRIO);
	int32 code;
	_dlogger msg("", 0);
	while (read_port(p, &code, &msg, sizeof(msg)) > 0) {
		record_msg(msg);
	}
	g_running = false;
	release_sem(wake_sem);
	status_t status;
	wait_for_thread(tr, &status);
	return 0;
}


_dlogger::_dlogger()
{
}


