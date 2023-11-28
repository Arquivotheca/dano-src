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


#if (DEBUG > 0)

#if 0

#define	dprintf(x)		printf x
#define set_dprintf_enabled(x)

#else

#ifdef	__cplusplus
extern "C" {
#endif

extern void	_kdprintf_ (const char *format, ...);
extern status_t _kset_dprintf_enabled_(int);
extern void	_synclog_ (uint32 code, uint32 val);

#ifdef	__cplusplus
}
#endif

#if (_KERNEL_MODE > 0)
#define	dprintf(x)		dprintf x
#else
#define	set_dprintf_enabled(x)	_kset_dprintf_enabled_ x
#define	dprintf(x)		_kdprintf_ x
#endif

#if DEBUG_SYNCLOG
#define	synclog(x)		_synclog_ x
#else
#define	synclog(x)
#endif

#endif

#else

#define	set_dprintf_enabled(x)
#define	dprintf(x)
#define	synclog(x)

#endif


#endif	/*  __DEBUG_H  */
