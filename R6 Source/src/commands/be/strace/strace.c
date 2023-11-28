#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>

#include <kernel/image.h>
#include <kernel/OS.h>
#include <kernel/debugger.h>

#define ARG_NONE		0
#define ARG_UINT32		1
#define ARG_ADDR		2
#define ARG_UINT64_LO	3
#define ARG_UINT64_HI	4
#define ARG_STRING		5

#define BLACK			(opt_colorize ? "\033[0m" : "")
#define THID_COLOR		(opt_colorize ? "\033[34m" : "")
#define FUNCTION_COLOR	(opt_colorize ? "\033[31m" : "")
#define ELAPSED_COLOR	(opt_colorize ? "\033[35m" : "")

#define MAXSTRINGSIZE	128		/* includes NULL */

extern status_t _kstrace_init_(thread_id thid, uint32 flags);

struct thread_list {
    struct thread_list *next;
    struct thread_list *prev;
	thread_id thid;
	port_id nub_to_debugger;
	uint32 syscall;
	uint32 args[8];
	char strings[8][MAXSTRINGSIZE];
};

typedef struct thread_list thread_list;

static thread_list *ThreadList;

/*
 * options 
 */
static int opt_trace_team;
static int opt_fast_mode;
static int opt_print_args = 1;
static int opt_trace_through_spawns;
static int opt_colorize = 1;
static int opt_return_val = 1;

struct sctbl {
	const char *name;
	int argcount;
	uint32 args[8];
} scfuncs[] = {
/* 0 */		{ "user_open",  5, { ARG_UINT32, ARG_STRING, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "user_close",  1, { ARG_UINT32 } },
			{ "user_read",  4, { ARG_UINT32, ARG_ADDR, ARG_UINT32, ARG_UINT32 } },
			{ "user_write",  4, { ARG_UINT32, ARG_ADDR, ARG_UINT32, ARG_UINT32 } },
			{ "user_ioctl",  4, { ARG_UINT32, ARG_UINT32, ARG_ADDR, ARG_UINT32 } },
			{ "user_lseek",  4, { ARG_UINT32, ARG_UINT64_LO, ARG_UINT64_HI, ARG_UINT32 } },
			{ "copy_sighandlers",  2, { ARG_UINT32, ARG_UINT32 } },
			{ "real_time_clock",  0, { ARG_NONE } },
			{ "resize_area",  2, { ARG_UINT32, ARG_UINT32 } },
			{ "set_real_time_clock",  1, { ARG_UINT32 } },
/* 10 */	{ "load_user_addon",  3, { ARG_STRING, ARG_STRING, ARG_STRING } },
			{ "user_fcntl",  3, { ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "user_opendir",  3, { ARG_UINT32, ARG_STRING, ARG_UINT32 } },
			{ "exit_thread",  1, { ARG_UINT32 } },
			{ "user_snooze",  2, { ARG_UINT64_LO, ARG_UINT64_HI } },
			{ "user_closedir",  1, { ARG_UINT32 } },
			{ "kill_thread",  1, { ARG_UINT32 } },
			{ "resume_thread",  1, { ARG_UINT32 } },
			{ "user_acquire_sem",  1, { ARG_UINT32 } },
			{ "unload_user_addon",  1, { ARG_UINT32 } },
/* 20 */	{ "user_create_area",  6, { ARG_STRING, ARG_ADDR, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "delete_area",  1, { ARG_UINT32 } },
			{ "user_get_image_symbol",  4, { ARG_UINT32, ARG_STRING, ARG_UINT32, ARG_ADDR } },
			{ "user_rewinddir",  1, { ARG_UINT32 } },
			{ "quit",  1, { ARG_UINT32 } },
			{ "user_find_area",  1, { ARG_STRING } },
			{ "user_rename_thread",  2, { ARG_UINT32, ARG_STRING } },
			{ "user_get_nth_image_symbol",  6, { ARG_UINT32, ARG_UINT32, ARG_ADDR, ARG_ADDR, ARG_ADDR, ARG_ADDR } },
			{ "user_readdir",  4, { ARG_UINT32, ARG_ADDR, ARG_UINT32, ARG_UINT32 } },
			{ "_user_get_thread_info",  3, { ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
/* 30 */	{ "user_mkdir",  3, { ARG_UINT32, ARG_STRING, ARG_UINT32 } },
			{ "user_symlink",  3, { ARG_STRING, ARG_UINT32, ARG_STRING } },
			{ "set_cpu_state",  2, { ARG_UINT32, ARG_UINT32 } },
			{ "get_cpu_state",  1, { ARG_UINT32 } },
			{ "user_wait_for_thread",  2, { ARG_UINT32, ARG_UINT32 } },
			{ "user_readlink",  4, { ARG_UINT32, ARG_STRING, ARG_ADDR, ARG_UINT32 } },
			{ "set_dprintf_enabled",  1, { ARG_UINT32 } },
			{ "read_config_item",  2, { ARG_UINT32, ARG_UINT32 } },
			{ "user_rename",  4, { ARG_UINT32, ARG_STRING, ARG_UINT32, ARG_STRING } },
			{ "user_unlink",  2, { ARG_UINT32, ARG_STRING } },
/* 40 */	{ "_user_get_port_info",  3, { ARG_UINT32, ARG_UINT32, ARG_UINT32 } }, 
			{ "has_data",  1, { ARG_UINT32 } },
			{ "_user_get_sem_info",  3, { ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "user_release_sem",  1, { ARG_UINT32 } },
			{ "user_delete_sem",  1, { ARG_UINT32 } },
			{ "spawn_thread",  4, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "install_default_debugger",  1, { ARG_UINT32 } },
			{ "user_find_thread",  1, { ARG_UINT32 } },
			{ "user_rstat",  4, { ARG_UINT32, ARG_STRING, ARG_ADDR, ARG_UINT32 } },
			{ "user_mount",  7, { ARG_STRING, ARG_UINT32, ARG_STRING, ARG_STRING, ARG_UINT32, ARG_ADDR, ARG_UINT32 } },
/* 50 */	{ "set_sem_owner",  2, { ARG_UINT32, ARG_UINT32 } },
			{ "set_port_owner",  2, { ARG_UINT32, ARG_UINT32 } },
			{ "user_load_image",  6, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "install_team_debugger",  2, { ARG_UINT32, ARG_UINT32 } },
			{ "remove_team_debugger",  1, { ARG_UINT32 } },
			{ "suspend_thread",  1, { ARG_UINT32 } },
			{ "_to_debugger",  1, { ARG_UINT32 } },		/* a debugger call */
			{ "force_trace",  1, { ARG_UINT32 } },
			{ "kill_team",  1, { ARG_UINT32 } },
			{ "_user_get_team_info",  3, { ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
/* 60 */	{ "_user_get_next_image_info",  4, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } }, 
			{ "_user_get_image_info",  3, { ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "write_config_item",  2, { ARG_UINT32, ARG_UINT32 } },	/* used to be timedelay */
			{ "exit_team",  1, { ARG_UINT32 } },
			{ "user_unmount",  2, { ARG_UINT32, ARG_STRING } },
			{ "user_create_port",  2, { ARG_UINT32, ARG_STRING } },		/* port create */
			{ "user_write_port",  4, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },	/* port send */
			{ "user_read_port",  4, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },	/* port receive */
			{ "port_count",  1, { ARG_UINT32 } },		/* port count */
			{ "free_memory",  2, { ARG_UINT32, ARG_UINT32 } },
/* 70 */	{ "start_event_log",  0, { ARG_NONE } },
			{ "stop_event_log",  2, { ARG_UINT32, ARG_UINT32 } },
			{ "set_thread_priority",  2, { ARG_UINT32, ARG_UINT32 } },	/* change process priority */
			{ "delete_port",  1, { ARG_UINT32 } },			/* port delete */
			{ "user_dup2",  3, { ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "user_wstat",  5, { ARG_UINT32, ARG_STRING, ARG_ADDR, ARG_UINT32, ARG_UINT32 } },
			{ "read_pci_config",  5, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },		/* read pci config space */
			{ "write_pci_config",  6, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },		/* write pci config space */
			{ "set_area_protection",  2, { ARG_UINT32, ARG_UINT32 } },	/* change the protection of an area */
			{ "user_clone_area",  6, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },		/* share an area */
/* 80 */	{ "get_nth_pci_info",  2, { ARG_UINT32, ARG_UINT32 } },		/* get pci info */
			{ "_user_get_next_team_info",  3, { ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "_user_get_next_area_info",  4, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "user_read_port_etc",  7, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT64_LO, ARG_UINT64_HI } },
			{ "_user_get_area_info",  3, { ARG_UINT32, ARG_UINT32, ARG_UINT32 } },	/* get area's info */
			{ "area_for",  1, { ARG_ADDR } },				/* find area given its address */
			{ "_user_get_system_info",  2, { ARG_UINT32, ARG_UINT32 } },	/* return kernel stats */
			{ "user_chdir",  2, { ARG_UINT32, ARG_STRING } },
			{ "user_access",  3, { ARG_UINT32, ARG_STRING, ARG_UINT32 } },
			{ "user_send_data",  4, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },		/* Send msg variable. */
/* 90 */	{ "user_receive_data",  3, { ARG_UINT32, ARG_UINT32, ARG_UINT32 } },	/* receive msg variable. */
			{ "user_release_sem_etc",  3, { ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "user_get_sem_count",  2, { ARG_UINT32, ARG_UINT32 } },
			{ "user_create_sem",  2, { ARG_UINT32, ARG_STRING } },
			{ "user_write_port_etc",  7, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT64_LO, ARG_UINT64_HI } },
			{ "user_rfsstat",  5, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "user_rmdir",  2, { ARG_UINT32, ARG_UINT32 } },
			{ "is_computer_on",  0, { ARG_NONE } },
			{ "user_openwd",  4, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },			/* XXX 3? */
			{ "user_copy_area",  5, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
/* 100 */	{ "_user_get_next_thread_info",  4, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "_user_get_next_port_info",  4, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "_user_get_next_sem_info",  4, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "find_port",  1, { ARG_UINT32 } },
			{ "dprintf",  8, { ARG_STRING, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } }, 			/* PROBLEM -- DOES NOT KNPW THE NUMBER OF PARAMETERS */
			{ "user_port_buffer_size",  1, { ARG_UINT32 } },		/* get next port buffer size */
			{ "user_acquire_sem_etc",  5, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT64_LO, ARG_UINT64_HI } },
			{ "user_port_buffer_size_etc",  4, { ARG_UINT32, ARG_UINT32, ARG_UINT64_LO, ARG_UINT64_HI } },
			{ "user_openwd_vn",  6, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "user_closewd",  1, { ARG_UINT32 } },
/* 110 */	{ "user_link",  4, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "sync",  0, { ARG_NONE } },
			{ "user_sigaction",  3, { ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "user_sigpending",  1, { ARG_UINT32 } },
			{ "user_sigsuspend",  1, { ARG_UINT32 } },
			{ "user_sigprocmask",  3, { ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "send_signal",  2, { ARG_UINT32, ARG_UINT32 } },
			{ "getegid",  0, { ARG_NONE } },
			{ "geteuid",  0, { ARG_NONE } },
			{ "getgid",  0, { ARG_NONE } },
/* 120 */	{ "getpgrp",  0, { ARG_NONE } },
			{ "set_alarm",  3, { ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "getppid",  0, { ARG_NONE } },
			{ "getuid",  0, { ARG_NONE } },
			{ "setgid",  1, { ARG_UINT32 } },
			{ "setpgid",  2, { ARG_UINT32, ARG_UINT32 } },
			{ "setsid",  0, { ARG_NONE } },
			{ "setuid",  1, { ARG_UINT32 } },
			{ "tcgetpgrp",  1, { ARG_UINT32 } },
			{ "tcsetpgrp",  2, { ARG_UINT32, ARG_UINT32 } },
/* 130 */	{ "set_parent",  2, { ARG_UINT32, ARG_UINT32 } },
			{ "syslog_initialize",  1, { ARG_UINT32 } },
			{ "user_get_thread_stacks",  2, { ARG_UINT32, ARG_UINT32 } },
			{ "user_read_attr",  8, { ARG_UINT32, ARG_STRING, ARG_UINT32, ARG_UINT64_LO, ARG_UINT64_HI, ARG_ADDR, ARG_UINT32 } },
			{ "user_write_attr",  8, { ARG_UINT32, ARG_STRING, ARG_UINT32, ARG_UINT64_LO, ARG_UINT64_HI, ARG_ADDR, ARG_UINT32 } },
			{ "user_start_watching_vnode",  6, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "user_stop_watching_vnode",  5, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "user_read_pos",  6, { ARG_UINT32, ARG_UINT64_LO, ARG_UINT64_HI, ARG_ADDR, ARG_UINT32, ARG_UINT32 } },
			{ "user_write_pos",  6, { ARG_UINT32, ARG_UINT64_LO, ARG_UINT64_HI, ARG_ADDR, ARG_UINT32, ARG_UINT32 } },
			{ "get_default_screen_info",  1, { ARG_UINT32 } },
/* 140 */	{ "user_fsync",  1, { ARG_UINT32 } },
			{ "is_computer_on_fire",  0, { ARG_NONE } },
			{ "user_lock_node",  1, { ARG_UINT32 } },
			{ "user_unlock_node",  1, { ARG_UINT32 } },
			{ "user_remove_attr",  2, { ARG_UINT32, ARG_STRING } },
			{ "user_stat_attr",  3, { ARG_UINT32, ARG_STRING, ARG_ADDR } },
			{ "user_read_attr_dir",  4, { ARG_UINT32, ARG_ADDR, ARG_UINT32, ARG_UINT32 } },
			{ "user_rewind_attr_dir",  1, { ARG_UINT32 } },
			{ "user_open_attr_dir",  3, { ARG_UINT32, ARG_STRING, ARG_UINT32 } },
			{ "user_close_attr_dir",  1, { ARG_UINT32 } },
/* 150 */	{ "user_stop_notifying",  2, { ARG_UINT32, ARG_UINT32 } },
			{ "user_open_index_dir",  2, { ARG_UINT32, ARG_UINT32 } },
			{ "user_close_index_dir",  1, { ARG_UINT32 } },
			{ "user_read_index_dir",  4, { ARG_UINT32, ARG_ADDR, ARG_UINT32, ARG_UINT32 } },
			{ "user_rewind_index_dir",  1, { ARG_UINT32 } },
			{ "user_create_index",  4, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "user_remove_index",  2, { ARG_UINT32, ARG_UINT32 } },
			{ "user_stat_index",  3, { ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "user_open_query",  6, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "user_close_query",  1, { ARG_UINT32 } },
/* 160 */	{ "user_read_query",  4, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "user_fork", 0, { ARG_NONE } },
			{ "exec_image",  4, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "user_wait_for_team",  5, { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "clear_caches",  3, { ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "get_system_time_parms",  2, { ARG_UINT32, ARG_UINT32 } },
			{ "real_time_clock_usecs",  0, { ARG_NONE } },
			{ "user_set_thread_name",  1, { ARG_UINT32 } },
			{ "user_sigactionvec",  3, { ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "user_wfsstat",  3, { ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
/* 170 */	{ "map_page",  2, { ARG_UINT32, ARG_UINT32 } },
			{ "user_rename_attr",  3, { ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "user_rename_index",  3, { ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "set_tzspecs",  2, { ARG_UINT32, ARG_UINT32 } },
			{ "set_tzfilename",  3, { ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "get_tzfilename",  1, { ARG_UINT32 } },
			{ "get_rtc_info",  1, { ARG_UINT32 } },
			{ "get_thread_registers",  2, { ARG_UINT32, ARG_UINT32 } },
			{ "set_thread_register_flag",  2, { ARG_UINT32, ARG_UINT32 } },
			{ "read_isa_io",  3 , { ARG_UINT32, ARG_UINT32, ARG_UINT32 } }, /* HACK FIXME XXX */
/* 180 */	{ "write_isa_io",  4 , { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } }, /* HACK FIXME XXX */
			{ "lock_memory",  3 , { ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "unlock_memory",  3 , { ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "set_signal_stack",  2 , { ARG_UINT32, ARG_UINT32 } },
			{ "disable_debugger",  1 , { ARG_UINT32 } },
			{ "user_snooze_until",  3 , { ARG_UINT64_LO, ARG_UINT64_HI, ARG_UINT32 } },
			{ "set_fd_limit",  1 , { ARG_UINT32 } },
			{ "set_mon_limit",  1 , { ARG_UINT32 } },
			{ "map_physical_memory",  6 , { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "apm_control",  1 , { ARG_UINT32 } },
/* 190 */	{ "select",  6 , { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "user_readv_pos",  6, { ARG_UINT32, ARG_UINT64_LO, ARG_UINT64_HI, ARG_ADDR, ARG_UINT32, ARG_UINT32 } },
			{ "user_writev_pos",  6, { ARG_UINT32, ARG_UINT64_LO, ARG_UINT64_HI, ARG_ADDR, ARG_UINT32, ARG_UINT32 } },
			{ "user_get_safemode_option",  3 , { ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "_get_team_usage_info",  3 , { ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "close_port",  1 , { ARG_UINT32 } },
			{ "user_load_image_etc",  6 , { ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32, ARG_UINT32 } },
			{ "generic_syscall",  3, { ARG_UINT32, ARG_ADDR, ARG_UINT32 } },
			{ "strace_init", 2, { ARG_UINT32, ARG_UINT32 } },
			{ "grant", 3, { ARG_UINT32, ARG_UINT32, ARG_UINT32 }},
/* 200 */	{ "instantiate_image",  5 , { ARG_UINT32, ARG_ADDR, ARG_ADDR, ARG_UINT32, ARG_UINT32 } },
			{ "map_kernel_export_data", 1, { ARG_ADDR } },
			{ "user_snooze_etc", 4 , { ARG_UINT64_LO, ARG_UINT64_HI, ARG_UINT32, ARG_UINT32 } },
			{ "user_kprof_get_info", 4, {ARG_ADDR, ARG_UINT32, ARG_ADDR, ARG_UINT32 }  }
};

typedef struct sctbl sctbl;

/*
 * This is used to determine if a syscall is valid.  Note that it works
 * differently than it does on PPC.
 */   
static int num_scfuncs = sizeof(scfuncs) / sizeof(scfuncs[0]);
 
struct {
	db_why_stopped why;
	const char *s;
} why_stopped_str[] = {
	{ B_THREAD_NOT_RUNNING, "Thread not running" },
	{ B_DEBUGGER_CALL, "debugger() call" },
	{ B_BREAKPOINT_HIT, "breakpoint hit" },
	{ B_SNGLSTP, "trace exception" },
	{ B_NMI, "Got NMI" },
	{ B_MACHINE_CHECK_EXCEPTION, "Machine Check Exception" },
	{ B_SEGMENT_VIOLATION, "Segment Violation" },
	{ B_ALIGNMENT_EXCEPTION, "Alignment Exception" },
	{ B_DIVIDE_ERROR, "Divide Error" },
	{ B_OVERFLOW_EXCEPTION, "Overflow Exception" },
	{ B_BOUNDS_CHECK_EXCEPTION, "Bounds Check Exception" },
	{ B_INVALID_OPCODE_EXCEPTION, "Invalid Opcode Exception" },
	{ B_SEGMENT_NOT_PRESENT, "Segment Not Present" },
	{ B_STACK_FAULT, "Stack Fault" },
	{ B_GENERAL_PROTECTION_FAULT, "General Protection Fault" },
	{ B_FLOATING_POINT_EXCEPTION, "Floating Point Exception" },  
	{ B_GET_PROFILING_INFO, "Profiling Info" },
	{ B_WATCHPOINT_HIT, "Watchpoint Hit" },
	{ B_SYSCALL_HIT, "Syscall Hit" }
};

static status_t 
fetch_string(char *s, void *addr, port_id port_u2k, port_id port_k2u)
{
	long what;
	status_t ret;
	nub_read_memory_msg m;

	if (opt_fast_mode)
		return B_ERROR;
			
	m.reply_port = port_k2u;
	m.addr = addr;
	m.count = MAXSTRINGSIZE;
	write_port(port_u2k, B_READ_MEMORY, &m, sizeof(m));

	ret = read_port(port_k2u, &what, s, MAXSTRINGSIZE);
	if (ret < B_OK)
		return ret;
	else if (what < B_OK)
		return what;

	return B_OK;
}

static status_t
install_team_debugger_for_thread(thread_id thid, port_id dbport)
{
	thread_info thinfo;
	uint32 flags = DEBUG_syscall_tracing_only;

	if (get_thread_info(thid, &thinfo) < B_OK) {
		perror("strace calling get_thread_info");
		return -1;
	}
	if (install_team_debugger(thinfo.team, dbport) < 0) {
		perror("strace calling install_team_debugger");
		return -1;
	}
	if (opt_fast_mode)
		flags |= DEBUG_syscall_fast_trace;
	if (opt_trace_through_spawns)
		flags |= DEBUG_syscall_trace_through_spawns;
	if (opt_trace_team)
		flags |= DEBUG_syscall_trace_whole_team;
	if (_kstrace_init_(thid, flags) < 0) {
		perror("strace calling strace_init");
		return -1;
	}
	return B_OK;
}

static thread_list *
new_elem(thread_id thid)
{
	thread_list *elem;

	elem = (thread_list *) calloc(1, sizeof(thread_list));
	elem->thid = thid;

	if (!opt_fast_mode) {
		elem->nub_to_debugger = create_port(32, "strace port: reply from nub to debugger");
	}
	return elem;
}

static void
delete_elem(thread_list *elem)
{
	if (elem->prev) {
		elem->prev->next = elem->next;
	}
	if (elem->next) {
		elem->next->prev = elem->prev;
	}
	if (!opt_fast_mode)
		delete_port(elem->nub_to_debugger);
	if (elem == ThreadList)
		ThreadList = elem->next;
	free(elem);
} 

static void
delete_elem_by_thid(thread_id thid)
{
	thread_list *elem;
	for (elem = ThreadList; elem != NULL; elem = elem->next) {
		if (elem->thid == thid) {
			delete_elem(elem);
			return;
		}
	}
}

static void
delete_thread_list(void)
{
	thread_list *elem = ThreadList;

	while (elem != NULL) {
		thread_list *save = elem->next;
		delete_elem(elem);
		elem = save;
	}
}

static thread_list *
get_elem_for_thread(thread_id thid)
{
	thread_list *elem = ThreadList;

	if (!ThreadList) {
		return (ThreadList = new_elem(thid));
	}

	while (elem != NULL) {
		if (elem->thid == thid) {
			return elem;
		}
		elem = elem->next;
	}

	/* need to create new element */

	elem = new_elem(thid);
	ThreadList->prev = elem;
	elem->next = ThreadList; 
	ThreadList = elem;
	return elem;
}

static status_t
do_thread_stopped(db_thread_stopped_msg *m)
{
	uint32 syscall = m->cpu.eax;
	nub_run_thread_msg reply;
	nub_read_memory_msg read_memory;
	uint32 what;
	status_t ret;
	struct sctbl *s;
	thread_list *elem;
	char name[MAXSTRINGSIZE];

	assert(m->why == B_SYSCALL_HIT);

	elem = get_elem_for_thread(m->thread);
	elem->syscall = syscall;

	if (syscall < 0  || syscall > num_scfuncs) {
		goto done;
	}

	/* get arguments */

	s = scfuncs + syscall;

	read_memory.reply_port = elem->nub_to_debugger;
	read_memory.addr = (void *) (m->cpu.uesp + 4);
	read_memory.count = (4 * s->argcount);

	write_port(m->nub_port, B_READ_MEMORY, &read_memory, sizeof(read_memory));
   	ret = read_port(elem->nub_to_debugger, &what, elem->args, 4 * s->argcount);

	if (ret < B_OK)
		return ret;
	else if (what < B_OK)
		return what;

	if (opt_print_args  &&  !opt_fast_mode) {
		int i;
		for(i = 0; i < s->argcount; i++) {
			if (!s->args[i])
				continue;

			switch (s->args[i]) {
			case ARG_STRING:
				if (elem->args[i] == (int) NULL) {
					break;
				} else if (fetch_string(name, (void *) elem->args[i], 
										m->nub_port, elem->nub_to_debugger) == B_OK) {
					strncpy(elem->strings[i], name, MAXSTRINGSIZE);
				}
				break;
			}
		}
	}

 done:
	reply.thread = m->thread;
	reply.cpu = m->cpu;
	write_port(m->nub_port, B_RUN_THREAD, &reply, sizeof(reply));

	return B_OK;
}

static status_t
do_syscall_post(db_syscall_post_msg *m)
{
	char *errstr = "";
	thread_list *elem = NULL;
	uint32 *args;
	sctbl *s = scfuncs + m->syscall;
	const char *funcname;
	bool comma = false;

	if (!opt_fast_mode) {
		elem = get_elem_for_thread(m->thread);
		args = elem->args;
		if (m->syscall != elem->syscall) {
			/* mismatch, for some reason; let's try to resync */
			return B_OK;
		}
	} else {
		args = m->args;
	}

	if (opt_trace_through_spawns)
		printf("%s[%d] ", THID_COLOR, (int) m->thread);

#if 0
	if (!strncmp(s->name, "user_", 5)) {
		funcname = s->name + 5;
	} else if (!strncmp(s->name, "_user_", 6))
		funcname = s->name + 6;
	else
#endif
		funcname = s->name;

	printf("%s%s%s(", FUNCTION_COLOR, funcname, BLACK);

	if (opt_print_args) {
		int i;
		for(i = 0; i < s->argcount; i++) {
			if (!s->args[i])
				continue;

			if (comma) 
				printf(", ");

			switch (s->args[i]) {
			case ARG_UINT32 :
				printf("0x%lx", args[i]);
				break;
			case ARG_ADDR :
				printf("0x%8.8lx", args[i]);
				break;
			case ARG_UINT64_LO :
				printf("0x%Lx", args[i] + (1LL << 32) * args[i+1]);
				i++;
				break;
			case ARG_STRING:
				if (!opt_fast_mode && args[i])
					printf("0x%x \"%s\"", (int) args[i], elem->strings[i]);
				else
					printf("0x%x", (int) args[i]);
				break;
			}
			comma = true;
		}
	}
	
	putc(')', stdout);

	if (opt_return_val) {
		if (m->retlo > 0x80000000) {
			errstr = strerror(m->retlo);
		}
		printf(" = 0x%x %s %s(%Ld us)%s", 
			   (int) m->retlo, errstr, ELAPSED_COLOR, m->end_time - m->start_time, BLACK);
	}
	putc('\n', stdout);
	
	if (opt_trace_through_spawns) {
		/* free resources if kill_thread or exit_thread seen */  
		if (m->syscall == 16 && m->retlo == B_OK) {
			delete_elem_by_thid(m->args[0]);
		} else if (m->syscall == 13  &&  m->retlo == B_OK) {
			delete_elem_by_thid(m->thread);
		}
	}

	return B_OK;
}	

static status_t 
strace_thread(thread_id thid)
{
	port_id port_k2u;
	int32 op;
	status_t ret = B_OK;
	int dummymsg;
	to_debugger_msg m;

	port_k2u = create_port(32, "strace k2u (debugging port)");
	if (port_k2u < 0) {
		perror("strace error when creating debugging port");
		ret = port_k2u;
		goto err0;
	}

	if (install_team_debugger_for_thread(thid, port_k2u) < 0) {
		goto err1;
	}

	resume_thread(thid);

	while (1) {
		ret = read_port(port_k2u, &op, &m, sizeof(m));
		if (ret < 0)
			goto err1;

		switch (op) {
		case B_THREAD_STOPPED:
			if (opt_fast_mode) {
				/* XXX do some handling here */
			}
			if (m.thread_stopped.why != B_SYSCALL_HIT) {
				fflush(stdout);
				fprintf(stderr, "\nstrace: %s in application. Can't continue stracing!\n\n", 
						why_stopped_str[m.thread_stopped.why].s);
				goto err1;
			} else if (do_thread_stopped((db_thread_stopped_msg *) &m) < 0)
				goto err1;
			break;

		case B_TEAM_CREATED:
		case B_TEAM_DELETED:
			/* XXX */
			printf("strace doesn't yet trace through fork/exit\n");
			break;

		case B_ELF_IMAGE_CREATED:
		{
			nub_acknowlege_image_created_msg msg;
			msg.token = m.pef_image_created.reply_token;
			write_port(m.pef_image_created.nub_port,
					B_ACKNOWLEGE_IMAGE_CREATED, &msg, sizeof(msg));
			break;
		}

		case B_SYSCALL_POST:
			if (do_syscall_post((db_syscall_post_msg *) &m) < 0)
				goto err1;
			break;

		default:
			assert(0);
		case B_THREAD_CREATED:
		case B_THREAD_DELETED:
			break;
		}

		if (op == B_SYSCALL_POST  &&  m.syscall_post.syscall == 63) {	/* exit_team */
			break;
		}
	}
	
 err1:
	delete_port(port_k2u);
 err0:
	if (opt_trace_through_spawns)
		delete_thread_list();
	write_port(m.thread_stopped.nub_port, -1, &dummymsg, sizeof(dummymsg));
	kill_team(m.thread_stopped.team);
	return ret;
}

static void
load_and_run(int argc, char **argv)
{
	int			thid;
	const char	*fn;
	char		*epath;
	char		*dir;
	char		*path = NULL;
	char		*ebuf = NULL;

	fn = argv[0];
	
	/* first try in current directory */
	thid = load_image(argc, (const char **) argv, (const char **)environ);
	if (thid >= 0)
		goto found;

	/* now try in our search path */

	if (!(epath = (getenv("PATH"))))
		goto err;
	if (!(ebuf = (char *) malloc(strlen (epath) + 1)))
		goto err;
	strcpy(ebuf, epath);		/* get a copy, cuz we trash it */

	if (!(path = (char *) malloc(strlen (ebuf) + strlen (fn) + 2)))
		goto err;
	argv[0] = path;

	dir = strtok (ebuf, ":");

	while (dir) {
		strcpy (path, dir);
		strcat (path, "/");
		strcat (path, fn);
	
		thid = load_image(argc, (const char **) argv, (const char **)environ);
		if (thid >= 0)
			goto found;

		dir = strtok (NULL, ":");
	}
err:
	printf ("strace: could not find something named %s to run\n", fn);
	goto exit;

found:
	strace_thread(thid);
exit:
	if (ebuf) free (ebuf);
	if (path) free (path);
	return;
}

static void
help(void)
{
	printf("usage: strace [-Tfscarh] [ thread_id | team_id | executable ]\n");
	printf("\n");
	printf("   -T: trace the entire team, not just one thread.\n");
	printf("       Required if passing a team_id to strace.\n");
	printf("   -f: fast mode. Doesn't retrieve arguments contents. Useful for timing system calls\n");
	printf("   -s: Trace all threads spawned by this thread.\n");
	printf("       Redundant if -T is also passed.\n");
	printf("   -c: don't colorize output\n");
	printf("   -a: don't print arguments\n"); 
	printf("   -r: don't print return value\n"); 
	printf("   -h: print this help message\n");
}

int 
main(int argc, char **argv)
{
	thread_id tid;
	char opt;	

	if (argc == 1) {
		help();
		exit(0);
	}

	while ((opt = getopt(argc, argv, "+Tsfarhc")) != EOF) {
		switch (opt) {
		case 'T':
			opt_trace_team = 1;
			opt_trace_through_spawns = 1;
			break;
		case 'f':
			opt_fast_mode = 1;
			break;
		case 'a':
			opt_print_args = 0;
			break;
		case 's':
			opt_trace_through_spawns = 1;
			break;
		case 'h':
			help();
			exit(1);
		case 'c':
			opt_colorize = 0;
			break;
		case 'r':
			opt_return_val = 0;
			break;
		default:
			help();
			exit(-1);
		}
	}

	if (argv[optind] == NULL) {
		help();
		exit(-2);
	}

	if (!isatty(1)) {
		opt_colorize = 0;
	}
	/* if argument was a number, strace that */

	if (isdigit(argv[optind][0])) {
		tid = strtol(argv[optind], 0, 0);
		if (opt_trace_team) { 
			team_info tminfo;
			thread_info self, thinfo;
			uint32 cookie = 0;

			if (get_team_info(tid, &tminfo) < B_OK) {
				fprintf(stderr, "strace: %d is not a valid team id\n", (int) tid);
				exit(-6);
			}
			get_thread_info(getpid(), &self);
			if (self.team == tminfo.team) {
				fprintf(stderr, "strace: Sorry, can't strace myself\n");
				exit(-7);
			}
			do {
				if (get_next_thread_info(tminfo.team, &cookie, &thinfo) < 0) {
					fprintf(stderr, "strace: couldn't strace team %d\n", (int) tid);
				}
			} while (strace_thread(thinfo.thread) < 0); 
		} else if (tid == getpid()) {
			fprintf(stderr, "strace: Sorry, can't strace myself\n");
			exit(-3);
		} else {
			thread_info thinfo;
			if (get_thread_info(tid, &thinfo) < 0 || strace_thread(tid) < 0) {
				fprintf(stderr, "strace: %d is not a valid thread id\n", (int) tid);
				exit(-4);
			}
		}
	} 
#if 0
		/* argument was a name - look for process by that name */
	else if ((tid = find_thread(argv[optind])) >= B_OK  &&  tid != getpid()) {
		if (strace_thread(tid) < 0) {				/* running */
			fprintf(stderr, "strace: Could not stop '%s'\n", (char *) argv[optind]);
			exit(-5);
		}
	}
#endif 
	else {
		/* named process not found: look for object file, run it */
		load_and_run(argc - optind, &argv[optind]);
	}
	return 0;
}

