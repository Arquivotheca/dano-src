/* :ts=8 bk=0
 *
 * debug.h:	Debugging macros
 *
 * $Id:$
 *
 * Leo L. Schwab					9806.02
 *  Pulled from multiple identical instances in each source file.
 */
#ifndef	__DEBUG_H
#define	__DEBUG_H


#if 1 //(DEBUG  ||  FORCE_DEBUG)

#if 0

#define	dprintf(x)		printf x
#define set_dprintf_enabled(x)

#else

#ifdef	__cplusplus
extern "C" {
#endif

extern void	_kdprintf_ (const char *format, ...);
extern status_t _kset_dprintf_enabled_(int);

#ifdef	__cplusplus
}
#endif

#ifdef COMPILING_ACCELERANT
#define	set_dprintf_enabled	_kset_dprintf_enabled_
#define	dprintf(x)		_kdprintf_ x
#else
#define	dprintf(x)		dprintf x
#endif

#endif

#else

#define	set_dprintf_enabled(x)
#define	dprintf(x)

#endif


#endif	/*  __DEBUG_H  */
