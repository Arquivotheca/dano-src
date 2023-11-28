/*
   Private definitions and extern's for the kdebug driver
*/   

#ifndef _KDBG_PRIVATE_H_
#define _KDBG_PRIVATE_H_


/*
   This is for use by kdebug functions that want to look at the
   current interrupt frame that got us into the debugger (for
   example to look at the PC)
*/   
extern iframe *kdebug_cur_iframe;


/*
   this is a simple linked list of external commands added by
   the various subsystems to dump out state info.  The list
   is maintained over in src/kernel/ppc/dbprocs.c which is also
   where some of the other kdebug glue routines are.
*/
typedef struct kdbg_cmd 
{
  char *name;
  int (*func)(int argc, char **argv);
  char *help;
  struct kdbg_cmd *next;
}kdbg_cmd;

extern kdbg_cmd *kdbg_external_cmds;

#endif /* _KDBG_PRIVATE_H_ */
