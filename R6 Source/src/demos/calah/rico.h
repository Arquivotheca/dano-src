

#define	uchar	unsigned char
#define	ushort	unsigned short
#define	uint	unsigned
#define	ulong	unsigned long

#ifndef __cplusplus
#define	bool		uchar
#endif
#ifndef TRUE
#define	TRUE		(0 == 0)
#define	FALSE		(not TRUE)
#endif
#define	not		!
#define	and		&&
#define	or		||
#define	loop		for (; ; )
#define	unless(expr)	if (not (expr))
#define	until(expr)	while (not (expr))

#define	nel( a)		(sizeof( a) / sizeof( (a)[0]))
#define	endof( a)	(&(a)[nel( a)])
#define	bitsof( a)	(8 * sizeof( a))
#define	ctrl( c)	((c) - 0100)
#define	tab( col)	(((col)|7) + 1)
#define	streq( s0, s1)	(strcmp( s0, s1) == 0)
#define	trunc( n, r)	((n) - (n)%(r))
#define	roundup( n, r)	((n) - 1 - ((n)+(r)-1)%(r) + (r))
#define	hibyte( w)	((uint)(w) >> 8)
#define	lobyte( w)	((w) & 0xFF)
#define	toascii( c)	((c) & 0x7F)
