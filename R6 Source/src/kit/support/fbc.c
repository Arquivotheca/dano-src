#include "fbc.h"

/* This function replaces an old function pointer with a new function */
/* pointer in a vtable. You can only call it once per class, and you */
/* should beware that you're passing correct parameters into it, because */
/* it CANNOT check your parameters for validity! */

void _patch_vtable_(void * vtab, const void * old_func, const void * new_func)
{
	const void ** t = (const void **)vtab;
	/* this is the maximum amount of sanity checking we can do :-( */
	if (!old_func || !vtab || !new_func) return;
	while (*t != old_func) t++;
	*t = new_func;
}

