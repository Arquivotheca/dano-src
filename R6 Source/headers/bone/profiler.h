/*
	profiler.h
	
	kernel profiler, originally for BONE
	
	Copyright 1999, Be Incorporated, All Rights Reserved.
*/


#ifndef _H_PROFILE
#define _H_PROFILE


/*
 *  To use the profiler, place PROF_IN as the first line in your functions
 *  after variable declaration, and PROF_OUT before each return.  Then, to
 *  enable profiler output, build with -DPROFILER=<num>, where num is a
 *  combo of the flags below or'ed together.  Run using the newly built
 *  module and examine the profiler data using the profiler debugger commands.
 *
 *  The profiler MUST NOT be directly used to profile the following calls (or anything called
 *  by them).  Funcs that call these calls are (of course) unaffected, so writing a
 *  simple wrapper around these calls to profile them is acceptable. 
 *  
 *  malloc, calloc, free, strcpy, strcmp, _get_thread_info, create_sem, acquire_sem,
 *  release_sem, any SkipList function, set_sem_owner, kprintf, qsort,
 *  kernel_debugger 
 *
 */
extern void _enter_prof_func(const char *file, const char *func, int flags);
extern void _exit_prof_func(const char *file, const char *func, int flags);

#define PROFILER_PROFILE 0x1 /* compile profiling info */
#define PROFILER_PRINT   0x2 /* print func enter/exit info on serial out */
#define PROFILER_DEBUG   0x4 /* break into kernel debugger at each profile point */
         
#ifdef PROFILER
/*
 * this is BONE-specific, we can change if this is linked globally somewhere.
 */
#define PROF_IN  gUtil->enter_prof_func(__FILE__, __FUNCTION__, PROFILER);
#define PROF_OUT gUtil->exit_prof_func(__FILE__, __FUNCTION__, PROFILER); 

#else

#define PROF_IN
#define PROF_OUT

#endif

#endif
