#ifndef _PRIV_RUNTIME_H
#define _PRIV_RUNTIME_H

#include <BeBuild.h>
#include <OS.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	TLS_BASE_PTR		0
#define	TLS_THREAD_ID		1
#define	TLS_ERRNO			2
#define	TLS_EXIT_NOTIFY		3
#define TLS_MADV_RECURSE	4
#define TLS_MALLOC_RECURSE  5
#define	TLS_FIRST_AVAIL		10

extern int		_call_init_routines_();
extern int		_call_term_routines_();

extern void	_thread_do_exit_notification();

extern thread_id	__main_thread_id;
extern char **		argv_save;

void	_init_system_time_(void);
void	_init_fast_syscall(void);
void	_init_sbrk_(void);
void	__fork_addon_();
void	__fork_sbrk_(void);
void	_init_kernel_export_data(void);

#ifdef __cplusplus
}
#endif

#endif
