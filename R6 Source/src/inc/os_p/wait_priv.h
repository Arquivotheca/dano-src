/* 
 * wait_for_team() options and return values..
 */

#ifndef _WAIT_PRIV_H
#define _WAIT_PRIV_H

/* team wait options */

#define	B_WAIT_NO_BLOCK		0x01		/* Do not hang if nothing exited */
#define	B_WAIT_STOPPED		0x02		/* Returned any stopped childern */

/* team wait reasons */

#define	B_THREAD_EXITED		1		/* Team simply exited */
#define	B_THREAD_SIGNALED	2		/* Team was signaled */
#define	B_THREAD_FAULTED	3		/* Team faulted */
#define	B_THREAD_STOPPED_	4		/* Team was suspended */
									/* trailing _ fixes conflict w/another name */
#define	B_THREAD_KILLED		5		/* Team was killed */

#endif
