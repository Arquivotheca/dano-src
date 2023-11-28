

#include	"calah.h"
#include	"board.h"


#define	E		(1 << BCALAH+0)		/* empty move */
#define	F		(1 << BCALAH+1)		/* end-of-game move */


mgenerate( bp)
board	*bp;
{
	uint	m,
		i;

	if (bp->mustpass)
		return (E);
	m = 0;
	for (i=0; i<BCALAH; ++i)
		if (bp->holes[i])
			m |= 1 << i;
	unless (m)
		for (i=0; i<BCALAH; ++i)
			if (bp->holes[BHI+i])
				return (F);
	return (m);
}


mscan( line)
char	line[];
{
	uint	i;

	if ((sscanf( line, "%d", &i) == 1)
	and (--i < BCALAH))
		return (1 << i);
	return (0);
}


bool
mlegal( bp, m)
board	*bp;
{
	uint	a;

	if ((a = mgenerate( bp))
	and (m)) {
		uint i = 0;
		until (m & 1<<i)
			++i;
		if (a & 1<<i)
			return (TRUE);
	}
	return (FALSE);
}


mcount( m)
{
	uint	n,
		i;

	unless (m)
		return (0);
	if (m & E)
		return (1);
	if (m & F)
		return (1);
	n = 0;
	for (i=0; i<BCALAH; ++i)
		if (m & 1<<i)
			++n;
	return (n);
}


bool
mempty( m)
{

	return (m&E && TRUE);
}


bool
mfini( m)
{

	return (m&F && TRUE);
}


#if 0
mnext( bp, m)
board	*bp;
{
	uint	i;

	unless ((m & E)
	or (m & F)
	or (not m))
		for (i=0; i<BCALAH; ++i)
			if (m & 1<<i)
				return (1 << i);
	return (m);
}
#else
mnext( bp, m)
board	*bp;
{
	uint	i;

	unless ((m & E)
	or (m & F)
	or (not m)) {
		i = BCALAH; do {
			--i;
			if ((m & 1<<i)
			and (i+bp->holes[i] == BCALAH))
				return (1 << i);
		} while (i);
		i = BCALAH; do {
			--i;
			if ((m & 1<<i)
			and (i+bp->holes[i] < BCALAH)
			and (not bp->holes[i+bp->holes[i]])
			and (bp->holes[BHI+i+bp->holes[i]]))
				return (1 << i);
		} while (i);
		i = BCALAH; do {
			--i;
			if (m & 1<<i)
				return (1 << i);
		} while (i);
	}
	return (m);
}
#endif


mclear( m, m2)
{

	return (m & ~m2);
}


move( bp, m)
board	*bp;
uint	m;
{
	uint	i;

	bp->mustpass = FALSE;
	if (m & F)
		for (i=0; i<BCALAH; ++i) {
			bp->holes[BLEFTCALAH] += bp->holes[BHI+i];
			bp->holes[BHI+i] = 0;
		}
	else unless (m & E)
		for (i=0; i<BCALAH; ++i)
			if (m & 1<<i) {
				uint n = bp->holes[i];
				bp->holes[i] = 0;
				while (n) {
					i = binext( i);
					++bp->holes[i];
					--n;
				}
				if ((i < BCALAH)
				and (bp->holes[i] == 1)
				and (bp->holes[bireflect( i)])) {
					bp->holes[BRIGHTCALAH] += 1 + bp->holes[bireflect( i)];
					bp->holes[i] = 0;
					bp->holes[bireflect( i)] = 0;
				}
				else if (i == BCALAH)
					bp->mustpass = TRUE;
				break;
			}
	bp->move = m;
	bswap( bp);
}


mprint( m)
{
	uint	i;

	for (i=0; i<BCALAH; ++i)
		if (m & 1<<i)
			printf( " %d", i+1);
	if (m & E)
		printf( " e");
	if (m & F)
		printf( " f");
}


mprintother( m)
{
	uint	i;

	for (i=0; i<BCALAH; ++i)
		if (m & 1<<i)
			printf( " %c", "89abcd"[i]);
	if (m & E)
		printf( " e");
	if (m & F)
		printf( " f");
}
