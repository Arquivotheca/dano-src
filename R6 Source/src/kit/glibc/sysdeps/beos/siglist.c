#include <stddef.h>


/* This is a list of all known signal numbers.  */

const char *const _sys_siglist[] =
  {
    "unknown signal",
    "Hangup",
    "Interrupt",
    "Quit",
    "Illegal Instruction",
    "Child exited",
    "Aborted",
    "Broken pipe",
    "Floating point exception",
    "Killed",
    "Stopped (signal)",
    "Segmentation fault",
    "Continued",
    "Stopped",
    "Alarm clock",
    "Terminated",
    "Stopped (tty input)",
    "Stopped (tty output)",
    "User defined signal 1",
    "User defined signal 2",
    "Window changed",
    "Thread Killed",
    NULL
  };

weak_alias (_sys_siglist, sys_siglist)
