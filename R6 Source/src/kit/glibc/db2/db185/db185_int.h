/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 1998
 *	Sleepycat Software.  All rights reserved.
 */
/*
 * Copyright (c) 1990, 1993, 1994, 1995, 1996
 *	Keith Bostic.  All rights reserved.
 */
/*
 * Copyright (c) 1990, 1993, 1994
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
 *
 *	@(#)db185_int.h	8.7 (Sleepycat) 4/10/98
 */

#ifndef _DB185_H_
#define	_DB185_H_

/* Routine flags. */
#define	R_CURSOR	1		/* del, put, seq */
#define	__R_UNUSED	2		/* UNUSED */
#define	R_FIRST		3		/* seq */
#define	R_IAFTER	4		/* put (RECNO) */
#define	R_IBEFORE	5		/* put (RECNO) */
#define	R_LAST		6		/* seq (BTREE, RECNO) */
#define	R_NEXT		7		/* seq */
#define	R_NOOVERWRITE	8		/* put */
#define	R_PREV		9		/* seq (BTREE, RECNO) */
#define	R_SETCURSOR	10		/* put (RECNO) */
#define	R_RECNOSYNC	11		/* sync (RECNO) */

typedef struct {
	void	*data;			/* data */
	size_t	 size;			/* data length */
} DBT185;

/* Access method description structure. */
typedef struct __db185 {
	DBTYPE type;			/* Underlying db type. */
	int (*close)	__P((struct __db185 *));
	int (*del)	__P((const struct __db185 *, const DBT185 *, u_int));
	int (*get)
	    __P((const struct __db185 *, const DBT185 *, DBT185 *, u_int));
	int (*put)
	    __P((const struct __db185 *, DBT185 *, const DBT185 *, u_int));
	int (*seq)
	    __P((const struct __db185 *, DBT185 *, DBT185 *, u_int));
	int (*sync)	__P((const struct __db185 *, u_int));
	void *internal;			/* Access method private. */
	int (*fd)	__P((const struct __db185 *));

	/*
	 * !!!
	 * Added to the end of the DB 1.85 DB structure, it's needed to
	 * hold the DB 2.0 cursor used for DB 1.85 sequential operations.
	 */
	DBC *dbc;			/* DB 1.85 sequential cursor. */
} DB185;

/* Structure used to pass parameters to the btree routines. */
typedef struct {
#define	R_DUP		0x01	/* duplicate keys */
	u_int32_t flags;
	u_int32_t cachesize;	/* bytes to cache */
	u_int32_t maxkeypage;	/* maximum keys per page */
	u_int32_t minkeypage;	/* minimum keys per page */
	u_int32_t psize;	/* page size */
	int	(*compare)	/* comparison function */
	    __P((const DBT *, const DBT *));
	size_t	(*prefix)	/* prefix function */
	    __P((const DBT *, const DBT *));
	int	lorder;		/* byte order */
} BTREEINFO;

/* Structure used to pass parameters to the hashing routines. */
typedef struct {
	u_int32_t bsize;	/* bucket size */
	u_int32_t ffactor;	/* fill factor */
	u_int32_t nelem;	/* number of elements */
	u_int32_t cachesize;	/* bytes to cache */
	u_int32_t		/* hash function */
		(*hash) __P((const void *, size_t));
	int	lorder;		/* byte order */
} HASHINFO;

/* Structure used to pass parameters to the record routines. */
typedef struct {
#define	R_FIXEDLEN	0x01	/* fixed-length records */
#define	R_NOKEY		0x02	/* key not required */
#define	R_SNAPSHOT	0x04	/* snapshot the input */
	u_int32_t flags;
	u_int32_t cachesize;	/* bytes to cache */
	u_int32_t psize;	/* page size */
	int	lorder;		/* byte order */
	size_t	reclen;		/* record length (fixed-length records) */
	u_char	bval;		/* delimiting byte (variable-length records */
	char	*bfname;	/* btree file name */
} RECNOINFO;

#if defined(__cplusplus)
extern "C" {
#endif
DB185 *__dbopen __P((const char *, int, int, DBTYPE, const void *));
DB185 *dbopen __P((const char *, int, int, DBTYPE, const void *));
#if defined(__cplusplus)
};
#endif
#endif /* !_DB185_H_ */
