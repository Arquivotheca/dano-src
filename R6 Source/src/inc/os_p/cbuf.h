/* ++++++++++
	cbuf.h
	Copyright (C) 1991-95 Be Inc.  All Rights Reserved.
	Circular buffer management.
+++++ */

#ifndef _CBUF_H
#define _CBUF_H

#include <BeBuild.h>
#include <KernelExport.h>

typedef struct cbuf_rec {
	spinlock	lock;			/* spinlock  for buffer access */
	char		*ip;			/* input ptr */
	char		*op;			/* output pointer */
	char 		*buf;			/* -> start of buffer */
	char 		*max;			/* -> end of buffer */
	int			size;			/* size of buffer */
} cbuf;

extern cbuf			*cbuf_init (int size);
extern int			cbuf_delete (cbuf *cbp);

extern unsigned char	cbuf_get (cbuf *cbp);
extern int 			cbuf_mt (cbuf *cbp);
extern int 			cbuf_full (cbuf *cbp);
extern int 			cbuf_put (cbuf *cbp, char c);
extern int 			cbuf_unput (cbuf *cbp);
extern int 			cbuf_flush (cbuf *cbp);
extern int 			cbuf_size (cbuf *cbp);
extern int 			cbuf_avail (cbuf *cbp);
extern int 			cbuf_free (cbuf *cbp);
extern int 			cbuf_putn (cbuf *cbp, char *buf, int count);
extern int			cbuf_getn (cbuf *cbp, char *buf, int count);
extern int 			cbuf_putn_no_lock (cbuf *cbp, char *buf, int count);
extern int			cbuf_getn_no_lock (cbuf *cbp, char *buf, int count);
extern cpu_status	cbuf_lock (cbuf *cbp);
extern void			cbuf_unlock (cbuf *cbp, cpu_status pstate);

#define	CBUF_LOCK(p)	acquire_spinlock(&(p)->lock)
#define	CBUF_UNLOCK(p)	release_spinlock(&(p)->lock)
#define	CBUF_FULL(p)	((((p)->ip+1 == (p)->max) && \
								((p)->op == (p)->buf)) || \
								((p)->ip+1 == (p)->op))
#define	CBUF_MT(p)		((p)->ip == (p)->op)
#define	CBUF_FREE(p)	((p)->ip == (p)->op ? (p)->max - (p)->buf - 1 : \
								((p)->ip < (p)->op ? (p)->op - (p)->ip - 1 : \
								((p)->max-(p)->ip) + ((p)->op-(p)->buf) - 1))
#define	CBUF_AVAIL(p)	((p)->max - (p)->buf - 1 - CBUF_FREE(p))
#define	CBUF_GETN(p,b,n)	cbuf_getn_no_lock(p, b, n)
#define	CBUF_PUTN(p,b,n)	cbuf_putn_no_lock(p, b, n)

#endif
