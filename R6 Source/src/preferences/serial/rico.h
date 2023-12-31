

#define	uchar	unsigned char
#define	ushort	unsigned short
#define	uint	unsigned
#define	ulong	unsigned long

#ifndef TRUE
#define	bool		char
#define	TRUE		(0 == 0)
#define	FALSE		(not TRUE)
#endif
#define	not		!
#define	and		&&
#define	or		||
#define	loop		while (TRUE)
#define	until(expr)	while (not (expr))
#define	unless(expr)	if (not (expr))

#define	nel( a)		(sizeof( a) / sizeof( (a)[0]))
#define	endof( a)	((a) + nel( a))
#define	bitsof( a)	(8 * sizeof( a))
#define	ctrl( c)	((c) - 0100)
#define	tab( col)	(((col)|7) + 1)
#define	streq( s0, s1)	(strcmp( s0, s1) == 0)
#define	trunc( n, r)	((n) - (n)%(r))
#define	roundup( n, r)	((n) - 1 - ((n)+(r)-1)%(r) + (r))
#define	min( a, b)	((a)<(b)? (a): (b))
#define	max( a, b)	((a)<(b)? (b): (a))
