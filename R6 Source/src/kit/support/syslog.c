/*
 * syslog.c --
 *
 * 	BeOS version of the 4.3BSD system logging facility.
 *
 *  Copyright (c) 1996, Be Inc. All rights reserved.
 *
 *  Copyright (c) 1983, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>

#include <OS.h>
#include <syslogd.h>
#include <syslog.h>
#include <stdlib.h>

#define TAGLEN	50
#define TEAM_LOGGING 	10
#define THREAD_LOGGING	20

struct log_options {
	char 		tag[TAGLEN + 1];
	int			flags;
	int			facility;
	int			priority_mask;
	long 		openlog_called;
	thread_id	thid;
	team_id		tmid;
	void		*listelem;
};

typedef struct optionslist {
	struct log_options	options;
	struct optionslist	*prev;
	struct optionslist	*next;
} optionslist;

static optionslist *PerThreadOptionsList = (optionslist *) NULL;
static struct log_options TeamOptions;
static thread_id getlogoptions (struct log_options *, int);
static void setoptions (struct log_options *optionsPtr, const char *ident, int flags, int facility);

typedef struct Benaphore {
    int32       count;
    sem_id      sem;
    int32       sems_created;
    thread_id   owner;
    int32       owner_count;
    char        *name;
} Benaphore;

static Benaphore TeamOptionsLock = { 0, 0, 0, -1, 0, "syslog team options lock" };
static Benaphore OptionsListLock = { 0, 0, 0, -1, 0, "syslog options list lock" };

static bool
benaphore_lock(Benaphore *mutex)
{
    status_t    error = B_NO_ERROR;
    int32       old;
    thread_id   owner = find_thread(NULL);

    if (owner == mutex->owner) {
        mutex->owner_count++;
        goto done;   
    }

    old = atomic_add(&mutex->count, 1);
    if (old > 0) {
        while (!atomic_or(&mutex->sems_created, 0))
			;
        do {
            error = acquire_sem(mutex->sem);
        } while (error == B_INTERRUPTED);
    } else {
        if (!mutex->sems_created) {
            mutex->sem = create_sem(0, mutex->name);
            atomic_or(&mutex->sems_created, 1);
        }
    }

    if (error == B_NO_ERROR) {
        mutex->owner = owner;
        mutex->owner_count = 1;
    }

 done:
	return (error == B_NO_ERROR);
}

static void
benaphore_unlock(Benaphore *mutex)
{
    int32 old;

    mutex->owner_count--;

    if (mutex->owner_count == 0) {
        mutex->owner = -1;
        old = atomic_add(&mutex->count, -1);
        if (old > 1) {
            release_sem(mutex->sem);
        }
    }
}

static void
do_logging (int team_or_thread, int logflags, char *format, va_list args)
{
	char		*o, *b, *f, *afterdate;
	char		buf[MAXLINE+1];
	time_t		now;
	int			c;
	int 		olderrno = errno;
	long		code;
	struct log_options	options;
	syslog_message msg;
	int			facility, priority;
	int			optionflags;
	thread_id	pid;
	
	/* get time first */
	
	time (&now);
	pid = getlogoptions (&options, team_or_thread);

	/* see if we should just throw out this message. */

	priority = LOG_PRI (logflags);
	facility = LOG_FAC (logflags);
	
	if (priority < 0  ||  priority >= LOG_NPRIORITIES  ||
		facility < 0  ||  facility >= LOG_NFACILITIES  ||
		(priority & options.priority_mask) == 0) {
		return;
	}

	/* build the message */

	o = &msg.buf[0];
#if 0
#ifdef SYSLOG_NAMES
	sprintf (o , "<%s,%s>", facilitynames[facility], prioritynames[priority]);
#else
	sprintf (o, "<%d,%d>", facility, priority);
#endif
#endif

	sprintf (o, "<%d>", priority);
	
	o += strlen (o);
	sprintf (o, "%.15s ", ctime (&now) + 4);
	o += strlen (o);

	afterdate = o;

	if (options.tag[0]) {
		sprintf (o, "'%s'", options.tag);
		o += strlen (o);
	}

	optionflags = LOG_OPTIONS (logflags);

	if ((options.flags | optionflags) & LOG_PID) {
		if (team_or_thread == TEAM_LOGGING) {
			sprintf (o, "[tm%ld]", pid);
		} else {
			sprintf (o, "[%ld]", pid);
		}			
		o += strlen (o);
	}
	if (options.tag[0]  ||
		(options.flags | optionflags) & LOG_PID) {
		strcpy (o, ": ");
		o += 2;
	}

	b = buf;
	f = format;
	/* allow %m specifier for errno that may have caused this logging */

	while ((c = *f++) != '\0' && c != '\n' && b <= &buf[MAXLINE]) {
		if (c != '%') {
			*b++ = c;
			continue;
		}
		if ((c = *f++) != 'm') {
			*b++ = '%';
			*b++ = c;
			continue;
		}

		strncpy (b, strerror (olderrno), &buf[MAXLINE] - b);
		b += strlen(b);
	}

	*b++ = '\n';
	*b = '\0';

	vsprintf(o, &buf[0], args);
	
	c = strlen(msg.buf);
	if (c > MAXLINE)
		c = MAXLINE;

	/* output the message to the local logging daemon */

	msg.len = c;
	code = SYSLOG_PRINT;
	
	while (write_port (find_port ("syslog_daemon_port"), code, &msg, sizeof (msg)) == B_INTERRUPTED)
		;
			
	if ((options.flags | optionflags) & LOG_PERROR) {
		fputs (afterdate, stderr);
	}
	if ((options.flags | optionflags) & LOG_SERIAL) {
		/* XXX fix this! */
	}
}


static thread_id
getlogoptions (struct log_options *optionsPtr, int team_or_thread)
{
	if (team_or_thread == TEAM_LOGGING) {
		if (atomic_or (&TeamOptions.openlog_called, 0) == 0) {
			openlog_team (NULL, 0, LOG_USER);
		}

		benaphore_lock (&TeamOptionsLock);
		*optionsPtr = TeamOptions;
		benaphore_unlock (&TeamOptionsLock);

		return (thread_id) optionsPtr->tmid;
	} else {
		optionslist *elemPtr = PerThreadOptionsList;
		thread_id thid = find_thread (NULL);

		benaphore_lock (&OptionsListLock);
		
		/* this code should match openlog_thread, in essence */

		while (elemPtr) {
			if (elemPtr->options.thid == thid) {
				*optionsPtr = elemPtr->options;

				benaphore_unlock (&OptionsListLock);
				return thid;
			}

			elemPtr = elemPtr->next;
		}

		/* not found, so add to head of options list */

		elemPtr = (optionslist *) malloc (sizeof (optionslist));
		elemPtr->prev = NULL;
		elemPtr->next = PerThreadOptionsList;
		if (elemPtr->next) {
			elemPtr->next->prev = elemPtr;
		}
		PerThreadOptionsList = elemPtr;
		setoptions (&elemPtr->options, NULL, 0, LOG_USER);
		elemPtr->options.listelem = (void *) elemPtr;
		elemPtr->options.thid = thid;
		*optionsPtr = elemPtr->options;
	
		benaphore_unlock (&OptionsListLock);

		return thid;
	}
}
			 

void
log_thread (int logflags, char *format, ...)
{
	va_list args;
	
	va_start (args, format);
	do_logging (THREAD_LOGGING, logflags, format, args);
	va_end (args);
}

void
log_team (int logflags, char *format, ...)
{
	va_list args;
	
	va_start (args, format);
	do_logging (TEAM_LOGGING, logflags, format, args);
	va_end (args);
}

void
syslog (int logflags, const char *format, ...)
{
	va_list args;

	va_start (args, format);
	do_logging (THREAD_LOGGING, logflags, format, args);
	va_end (args);
}

void
openlog_team (char *ident, int flags, int facility)
{
	thread_info thinfo;
	
	get_thread_info (find_thread(NULL), &thinfo);

	benaphore_lock (&TeamOptionsLock);

	setoptions (&TeamOptions, ident, flags, facility);
	TeamOptions.tmid = thinfo.team;
	
	benaphore_unlock (&TeamOptionsLock);

	atomic_or (&TeamOptions.openlog_called, 1);
}


void
openlog_thread (char *ident, int flags, int facility)
{
	optionslist *elemPtr = PerThreadOptionsList;
	thread_id thid = find_thread (NULL);
	
	benaphore_lock (&OptionsListLock);

	while (elemPtr) {

		if (elemPtr->options.thid == thid) {
			setoptions (&elemPtr->options, ident, flags, facility);

			benaphore_unlock (&OptionsListLock);
			return;
		}

		elemPtr = elemPtr->next;
	}

	/* not found, so add to options list */

	elemPtr = (optionslist *) malloc (sizeof (optionslist));
	elemPtr->prev = NULL;
	elemPtr->next = PerThreadOptionsList;
	if (elemPtr->next) {
		elemPtr->next->prev = elemPtr;
	}
	PerThreadOptionsList = elemPtr;
	setoptions (&elemPtr->options, ident, flags, facility);
	elemPtr->options.listelem = (void *) elemPtr;
	elemPtr->options.thid = thid;
	
	benaphore_unlock (&OptionsListLock);
}


void
openlog (const char *ident, int options, int facility)
{
	openlog_thread (ident, options, facility);
}


void
closelog_team (void)
{
	return;
}


void
closelog_thread (void)
{
	struct log_options options;
	optionslist *elemPtr;
	
	getlogoptions (&options, THREAD_LOGGING);
	elemPtr = (optionslist *) options.listelem;

	benaphore_lock (&OptionsListLock);
	if (elemPtr->prev)
		elemPtr->prev->next = elemPtr->next;
	if (elemPtr->next)
		elemPtr->next->prev = elemPtr->prev;
	benaphore_unlock (&OptionsListLock);

	free (elemPtr);
}

void
closelog (void)
{
	closelog_thread ();
}
	

int
setlogmask_team (int pmask)
{
	int omask;

	benaphore_lock (&TeamOptionsLock);
	omask = TeamOptions.priority_mask;
	if (pmask != 0) {
		TeamOptions.priority_mask = pmask;
	}
	benaphore_unlock (&TeamOptionsLock);

	return omask;
}


int
setlogmask_thread (int pmask)
{
	int omask;
	struct log_options options;
	optionslist *elemPtr;
	
	getlogoptions (&options, THREAD_LOGGING);
	elemPtr = (optionslist *) options.listelem;
	
	omask = elemPtr->options.priority_mask;
	if (pmask != 0) {
		elemPtr->options.priority_mask = pmask;
	}

	return omask;
}

int
setlogmask (int pmask)
{
	return setlogmask_thread (pmask);
}


static void
setoptions (struct log_options *optionsPtr, const char *ident, int flags, int facility)
{
	if (ident != NULL) {
		strncpy (optionsPtr->tag, ident, TAGLEN);
	} else {
		optionsPtr->tag[0] = 0;
	}
	
	optionsPtr->flags = flags;

	if (facility != 0) {
		optionsPtr->facility = facility & LOG_FACMASK;
	} else {
		optionsPtr->facility = LOG_USER;
	}

	optionsPtr->priority_mask = LOG_PRIMASK;
}


