/*	Macros and prototypes for solutions to the fragile base class problem	*/
/*	$Id$	*/

#if !defined(_FBC_H)
#define _FBC_H

/* Update the vtable entry of this class to point from a reserved function */
/* to a new, improved member function that's available in a newer kit version */
/* This function will need casts when you call it. The arguments are: */
/* vtab -- the actual vtable of the object. If the vtable is the first */
/* member of the object (often, but not always, the case) you can get it */
/* by doing ((void**)object)[0] */
/* old_func -- the address of the "reserved" function; just pass the name */
/* new_func -- the address of the new, improved function; just pass the name */

/* Chances are that this function will freeze, crash, or subtly corrupt your */
/* program if you call it with the wrong arguments, SO CHECK YOUR ARGS!! */

#if defined(__cplusplus)
extern "C"
#endif
void _patch_vtable_(void * vtab, const void * old_func, const void * new_func);

#endif	/*	_FBC_H	*/
