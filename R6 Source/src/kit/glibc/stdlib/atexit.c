/* Copyright (C) 1991, 1996 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <bits/libc-lock.h>
#include <stdlib.h>
#include "exit.h"


/* Register FUNC to be executed by `exit'.  */
int
atexit (void (*func) (void))
{
  struct exit_function *new = __new_exitfn ();

  if (new == NULL)
    return -1;

  new->flavor = ef_at;
  new->func.at = func;
  return 0;
}


/* We change global data, so we need locking.  */
__libc_lock_define_initialized (static, lock)


static struct exit_function_list fnlist = { NULL, 0, };
struct exit_function_list *__exit_funcs = &fnlist;

struct exit_function *
__new_exitfn (void)
{
  struct exit_function_list *l;
  size_t i = 0;

  __libc_lock_lock (lock);

  for (l = __exit_funcs; l != NULL; l = l->next)
    {
      for (i = 0; i < l->idx; ++i)
	if (l->fns[i].flavor == ef_free)
	  break;
      if (i < l->idx)
	break;

      if (l->idx < sizeof (l->fns) / sizeof (l->fns[0]))
	{
	  i = l->idx++;
	  break;
	}
    }

  if (l == NULL)
    {
      l = (struct exit_function_list *)
	malloc (sizeof (struct exit_function_list));
      if (l != NULL)
	{
	  l->next = __exit_funcs;
	  __exit_funcs = l;

	  l->idx = 1;
      	  i = 0;
	}
    }

  /* Mark entry as used, but we don't know the flavor now.  */
  if (l != NULL)
    l->fns[i].flavor = ef_us;

  __libc_lock_unlock (lock);

  return l == NULL ? NULL : &l->fns[i];
}


#if __BEOS__

/* called when a container is unloaded, before the container term routine.
   it calls all the atexit-registered functions that live in the container. */

void
exit_container(char *base, size_t size)
{
	struct exit_function_list	*funcs;
	size_t						i;
	struct exit_function		*f;
	void						(*ptr)(void);
	char						foundone;

	/*
	regarding locking: the lock is held until a atexit function to call is found.
	then the lock is release. The entry cannot be left in the ef_at state because
	of reentrancy. An atexit() function calling exit() would end up calling itself
	another time. Instead, the entry is marked ef_us, not ef_free. This ensures
	that the exit_function_list structure pointed to by funcs is still valid upon return
	from the atexit function, in case free exit_function_list are garbage collected.
	Then, upon return from the atexit() function, the entry is marked free.
	regarding the do while outer loop: it is there to handle the case of atexit functions
	registering other atexit functions.	We have to scan the list again if we called at
	least one atexit function.
	*/

	__libc_lock_lock (lock);
	do {
		foundone = 0;
		for (funcs = __exit_funcs; funcs; funcs = funcs->next)
			for (i=0; i<funcs->idx; i++) {
				f = &funcs->fns[i];
				if (f->flavor == ef_at) {
					ptr = f->func.at;
					if (((char *)ptr >= base) && ((char *)ptr < base + size)) {
						f->flavor = ef_us;
						__libc_lock_unlock (lock);
						(*ptr)();
						__libc_lock_lock (lock);
						f->flavor = ef_free;
						foundone = 1;
					}
				}
		    }
	} while (foundone);
	__libc_lock_unlock (lock);
}

#endif
