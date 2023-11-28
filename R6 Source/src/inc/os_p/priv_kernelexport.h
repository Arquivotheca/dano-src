#ifndef _PRIV_KERNELEXPORT_H
#include <BeBuild.h>

extern bool		interrupts_enabled();
extern void		enable_interrupts();

extern bool		debug_output_enabled();
extern void		force_trace_pd();
extern void		do_control_alt_del();

#endif
