

#include <stdlib.h>
#include "rico.h"
#include "reg.h"
#include "utype.h"


#define	MAXPAR	10			/* maximum no. of parens */
#define	EOS	0			/* end of string */
#define	BPC	16			/* bits per character */
#define	CSIZE	(1<<BPC)		/* character set size */
#define	CVECL	((CSIZE + BPC-1) /BPC)	/* bytes per character set */


/*
 * The assert macro is used only for sanity checks.
 */
#ifdef	DEBUG
#define	assert(p)	if (!(p)) abort(); else
#else
#define	assert(p)
#endif


/*
 * NFA transition types (n_type).
 */
#define	NMCH	0			/* match literal character */
#define	NMCL	1			/* match character class */
#define	NMANY	2			/* match any character */
#define	NMEOS	3			/* match end of string */
#define	NMEW	4			/* match end of word */
#define	NMSUBS	5			/* match previous substring */
#define	NSAVE	6			/* save current position */
#define	NCLO	7			/* match any number of following */
#define	NFINI	8			/* end of NFA */


/*
 * The typedef `nfa' defines what are effectively instructions for a
 * non-deterministic pattern matching machine.
 * N_indx members are subscripts in [0, 2*MAXPAR) used to index into an array
 * of pointer to chars.  The n'th parenthesis pair are stored in positions
 * 2*n and 2*n+1.
 * Note, an nfa that has a NMEOS or NMEW transition ends at that point (and
 * does not have an NFINI transition).
 */
typedef union	nfa {
	struct	comm {			/* common to all nfa's */
		uchar	n_type;		/* NFA transition type */
	}	n_comm;
	struct	nmch {			/* match literal character */
		uchar	n_type;		/* NMCH */
		ushort	n_char;		/* character to match */
	}	n_mch;
	struct	nmcl {			/* match character class */
		uchar	n_type;		/* NMCL */
		ushort	*n_cls;		/* character class to match */
	}	n_mcl;
	struct	nmsubs {		/* match previously found sub-string */
		uchar	n_type,		/* NMSUBS */
			n_indx;		/* sub-string index */
	}	n_msubs;
	struct	nsave {			/* save current position */
		uchar	n_type,		/* NSAVE */
			n_indx;		/* sub-string index */
	}	n_save;
}	nfa;


/*
 * Anchor types (r_anchor).
 */
#define	RANY	0			/* totally unanchored */
#define RBOS	1			/* anchored to start of string */
#define	RBOW	2			/* anchored to start of word */


/*
 * A struct regprog hold all the data associated with a compiled regular
 * expression.
 * Note, r_npars includes the implicit pair around the entire expression.
 */
struct	regprog {
	uchar	r_npars,		/* number of \(-\) pairs */
		r_anchor;		/* initial anchoring */
	nfa	*r_start,		/* first real instruction */
		r_inst[0];		/* nfa instructions */
};


#define	NQ	20			/* NFA allocation quantum */

int	regerror;


/*
 * Local variables.
 */
static ushort		*nxtch;		/* next char in reg. exp. string */
static struct regprog	*prog;		/* current regprog */
static nfa		*nfanxt,	/* next slot in prog */
			*nfalim;	/* limit of prog */
static bool		pdone[MAXPAR];	/* iff `(' and `)' seen */


/*
 * Local functions.
 */
static bool	init(),			/* initialize for compile */
		compile(),		/* compile regular expression */
		comp1(),		/* compile paren-enclosed piece */
		makecls(),		/* make character class */
		addnst(),		/* add NFA state */
		addlp(),		/* add \( transition */
		addrp(),		/* add \) transition */
		adjust(),		/* change prog size */
		syntax( ),
		nomem( );
static void	cleanup(),		/* de-allocate after error */
		optimize();		/* optimize regprog */


/*
 * Regfree free()'s all space used by a compiled regular expression.
 */
void
regfree(rp)
struct regprog	*rp;
{
	register nfa	*np;

	for (np = rp->r_inst;; ++np)
		switch (np->n_comm.n_type) {
		case NMEW:
		case NMEOS:
		case NFINI:
			free((uchar *)rp);
			return;
		case NMCL:
			free((uchar *)np->n_mcl.n_cls);
			break;
		default:
			break;
		}
}


/*
 * Regcomp compiles a string which represents a regular expression into an
 * nfa.  It returns a pointer to the first transition (casted to a pointer
 * to a regprog).  If some problem occurs, it issues an error message and
 * returns NULL.
 */
struct regprog	*
regcomp(str)
ushort	*str;
{
	if (!init(str))
		return (NULL);
	if (!compile()) {
		cleanup();
		return (NULL);
	}
	optimize();
	return (prog);
}


/*
 * Init initializes all local variables to begin compiling the regular
 * expression contained in the string `str'.
 */
static bool
init(str)
ushort	*str;
{
	register uint	n;

	nxtch = str;
	n = sizeof(*prog) + NQ * sizeof(*prog->r_inst);
	prog = (struct regprog *)malloc(n);
	if (prog == NULL)
		return (nomem());
	prog->r_npars = 0;
	prog->r_anchor = RANY;
	nfanxt = prog->r_inst;
	nfalim = &nfanxt[NQ];
	for (n=0; n != MAXPAR; ++n)
		pdone[n] = FALSE;
	return (TRUE);
}


/*
 * Compile a regular expression.
 */
static bool
compile()
{
	switch (*nxtch) {
	case '^':
		prog->r_anchor = RBOS;
		++nxtch;
		break;
	case '~':
		prog->r_anchor = RBOW;
		++nxtch;
		break;
	}
	if (!comp1())
		return (FALSE);
	switch (*nxtch) {
	case '$':
		if (*++nxtch != EOS)
			return (syntax());
		if (!addnst(NMEOS))
			return (FALSE);
		break;
	case '~':
		if (*++nxtch != EOS)
			return (syntax());
		if (!addnst(NMEW))
			return (FALSE);
		break;
	case EOS:
		if (!addnst(NFINI))
			return (FALSE);
		break;
	default:
		return (syntax());
	}
	return (adjust(nfanxt - prog->r_inst));
}


/*
 * Comp1 compiles a regular expression with an assumed left and
 * right parenthesis surrounding the entire expression.  It recursively
 * calls itself for parenthesized sub-expressions.  When it returns TRUE
 * (indicating all is ok), the next character will either be EOS, `$' or
 * `\)'.
 */
static bool
comp1()
{
	register int	ch;
	register bool	clsok;
	int		level;

	if (!addlp(&level))
		return (FALSE);
	clsok = FALSE;
	loop
		switch (ch = *nxtch++) {
		case EOS:
			--nxtch;
			return (addrp(level));
		case '^':
			return (syntax());
		case '$':
		case '~':
			--nxtch;
			return (addrp(level));
		case '*':
			if (!clsok)
				return (syntax());
			if (!addnst(NCLO))
				return (FALSE);
			nfanxt[-1] = nfanxt[-2];
			nfanxt[-2].n_comm.n_type = NCLO;
			clsok = FALSE;
			break;
		case '.':
			if (!addnst(NMANY))
				return (FALSE);
			clsok = TRUE;
			break;
		case '[':
			if ((!addnst(NMCL))
			|| (!makecls(&nfanxt[-1].n_mcl.n_cls)))
				return (FALSE);
			clsok = TRUE;
			break;
		case '\\':
			switch (ch = *nxtch++) {
			case EOS:
				return (syntax());
			case '(':
				if (!comp1())
					return (FALSE);
				if (*nxtch++ != '\\' || *nxtch++ != ')')
					return (syntax());
				clsok = FALSE;
				break;
			case ')':
				nxtch -= 2;
				return (addrp(level));
			default:
				if ('1' <= ch && ch <= '9') {
					ch -= '0';
					if (!pdone[ch])
						return (syntax());
					if (!addnst(NMSUBS))
						return (FALSE);
					nfanxt[-1].n_msubs.n_indx = 2*ch;
					clsok = FALSE;
				} else {
					if (!addnst(NMCH))
						return (FALSE);
					nfanxt[-1].n_mch.n_char = ch;
					clsok = TRUE;
				}
				break;
			}
			break;
		default:
			if (!addnst(NMCH))
				return (FALSE);
			nfanxt[-1].n_mch.n_char = ch;
			clsok = TRUE;
			break;
		}
}


/*
 * Makecls makes a character class.
 */
static bool
makecls(cpp)
ushort	**cpp;
{
	register ushort	*cp;
	register int	ch,
			n;
	bool		negate;

	*cpp = cp = (ushort *)malloc(CVECL * sizeof(*cp));
	if (cp == NULL)
		return (nomem());
	for (n=0; n != CVECL; ++n)
		cp[n] = 0;
	negate = FALSE;
	if (*nxtch == '^') {
		++nxtch;
		negate = TRUE;
	}
	loop {
		ch = *nxtch++;
		switch (ch) {
		case '\\':
			ch = *nxtch++;
			if (ch == EOS)
				return (syntax());
			break;
		case EOS:
			return (syntax());
		case ']':
			if (negate) {
				for (n=0; n != CVECL; ++n)
					cp[n] = ~cp[n];
				cp[EOS/BPC] &= ~ (1 << EOS%BPC);
			}
			return (TRUE);
		}
		ch = (uint)ch % CSIZE;
		n = ch;
		if (*nxtch == '-') {
			++nxtch;
			ch = *nxtch++;
			switch (ch) {
			case '\\':
				ch = *nxtch++;
				if (ch == EOS)
					return (syntax());
				break;
			case EOS:
				return (syntax());
			}
			ch = (uint)ch % CSIZE;
			if (n > ch)
				return (syntax());
		}
		do {
			cp[n/BPC] |= 1 << n%BPC;
		} while (n++ != ch);
	}
}


/*
 * Addnst adds an nfa transition.
 */
static bool
addnst(type)
int	type;
{
	if ((nfanxt == nfalim)
	&& (!adjust(nfalim - prog->r_inst + NQ)))
		return (FALSE);
	nfanxt++->n_comm.n_type = type;
	return (TRUE);
}


/*
 * Addlp adds a left-parenthesis transition, and sets the integer pointed
 * to by `pnp' to the parnthesis level.
 */
static bool
addlp(pnp)
int	*pnp;
{
	register int	pc;

	pc = prog->r_npars++;
	if (pc >= MAXPAR)
		return (syntax());
	*pnp = pc;
	if (!addnst(NSAVE))
		return (FALSE);
	nfanxt[-1].n_save.n_indx = 2*pc;
	return (TRUE);
}


/*
 * Addrp adds a right-parenthesis transition for parenthesis number `pn'.
 */
static bool
addrp(pn)
register int	pn;
{
	assert(pn < MAXPAR);
	if (!addnst(NSAVE))
		return (FALSE);
	pdone[pn] = TRUE;
	nfanxt[-1].n_save.n_indx = 2*pn + 1;
	return (TRUE);
}


/*
 * Adjust the size of prog.
 */
static bool
adjust(nlen)
register uint	nlen;
{
	register uint	olen,
			nsize;

	olen = nfanxt - prog->r_inst;
	nsize = sizeof(*prog) + nlen*sizeof(*prog->r_inst);
	prog = (struct regprog *)realloc(prog, nsize);
	if (prog == NULL)
		return (nomem());	/* character class memory lost */
	nfanxt = &prog->r_inst[olen];
	nfalim = &prog->r_inst[nlen];
	return (TRUE);
}


/*
 * Cleanup de-allocates anything allocated in case an error is detected.
 */
static void
cleanup()
{
	register nfa	*np;

	if (prog == NULL)
		return;
	for (np = prog->r_inst; np != nfanxt; ++np)
		if ((np->n_comm.n_type == NMCL)
		&& (np->n_mcl.n_cls != NULL))
			free((uchar *)np->n_mcl.n_cls);
	free((uchar *)prog);
}


/*
 * Optimize is used to speed up matches by noting what any match must start
 * with.
 */
static void
optimize()
{
	register nfa	*np;

	for (np = prog->r_inst; np->n_comm.n_type == NSAVE; ++np)
		;
	if ((np[0].n_comm.n_type == NCLO)
	&& (np[1].n_comm.n_type == NMANY))
		prog->r_anchor = RBOS;	/* /.*junk/ can only match at start */
	prog->r_start = np;
}


/*
 * Local variables.
 * Note, saved and nsaved are only valid if the last call to regfind or
 * regstep returned TRUE.
 */
static ushort	*saved[2*MAXPAR];	/* saved locations */


/*
 * Local functions.
 */
static bool	run(),			/* match controller */
		amatch(),		/* anchored match routine */
		inclass();		/* test if ushort in class */
static ushort	*isprefix();		/* test for string prefix */


/*
 * Reglp returns a pointer (in the string last passed to regfind())
 * to the first character that matched the `pnum'th parenthesized
 * regular expression (where parenthesis pairs are numbered by the
 * order of occurance of the left parentheses).  If there were not
 * atleast `pnum' parentheses then reglp() returns NULL.
 * Note, it is an error to call reglp if the last call to regfind or
 * regstep did not return TRUE.
 */
ushort	*
reglp(pnum)
uint	pnum;
{
	if (pnum >= prog->r_npars)
		return (NULL);
	pnum = 2*pnum;
	assert(pnum < 2*MAXPAR);
	return (saved[pnum]);
}


/*
 * Regrp is just like reglp except that it returns a pointer to just
 * past the last character that matched.
 */
ushort	*
regrp(pnum)
uint	pnum;
{
	if (pnum >= prog->r_npars)
		return (NULL);
	pnum = 2*pnum + 1;
	assert(pnum < 2*MAXPAR);
	return (saved[pnum]);
}


/*
 * Regfind searches for the first match of `rp' in the string which starts
 * at `str'.  If it returns TRUE, successive matches can be found by calls
 * to regstep.
 */
bool
regfind(rp, str)
struct regprog	*rp;
ushort		*str;
{
	prog = rp;
	return (run(str, TRUE));
}


/*
 * Regstep searches for the next disjoint substring which matches.
 * Both the string being searched and the regular expression being
 * searched for are those passed to the last call to regfind.
 * Note, it is an error to call regstep if the last call to regfind
 * or regstep did not return TRUE.
 */
bool
regstep( )
{
	register ushort	*str;

	str = regrp(0);
	if ((str == reglp(0))
	&& (*str++ == EOS))
		return (FALSE);
	return (run(str, FALSE));
}


/*
 * Run tries to match the string starting at str against the regular
 * expression prog.  It basically scans for a possible starting point
 * for the match and then calls amatch to attempt an anchored match.
 */
static bool
run(str, bol)
register ushort	*str;
bool		bol;
{
	register nfa	*sp,
			*pc;

	pc = prog->r_inst;
	switch (prog->r_anchor) {
	case RBOS:
		return (bol && amatch(pc, str));
	case RBOW:
		do {
			if ((bol || !isuword(str[-1]))
			&& (amatch(pc, str)))
				return (TRUE);
			bol = FALSE;
		} while (*str++ != EOS);
		return (FALSE);
	case RANY:
		sp = prog->r_start;
		if (sp->n_comm.n_type == NMCH) {	/* optimization */
			for (;; ++str)
				if (*str == EOS)
					return (FALSE);
				else if ((*str == sp->n_mch.n_char)
				&& (amatch(pc, str)))
					return (TRUE);
		} else {
			do {
				if (amatch(pc, str))
					return (TRUE);
			} while (*str++ != EOS);
			return (FALSE);
		}
	default:
		assert(FALSE);
	}
}


/*
 * Amatch is the work horse of regular expression matching.  It only
 * performs anchored matches, and recursively calls itself to handle
 * closures.
 */
static bool
amatch(np, cp)
register nfa	*np;
register ushort	*cp;
{
	ushort	*cls,
		**pos;

	for (;; ++np)
		switch (np->n_comm.n_type) {
		case NMCH:	/* match literal character */
			if (*cp++ != np->n_mch.n_char)
				return (FALSE);
			break;
		case NMCL:	/* match character class */
			if (!inclass(*cp++, np->n_mcl.n_cls))
				return (FALSE);
			break;
		case NMANY:	/* match any character */
			if (*cp++ == EOS)
				return (FALSE);
			break;
		case NMEOS:	/* match end of string */
			return (*cp == EOS);
		case NMEW:	/* match end of word */
			return (!isuword(*cp));
		case NMSUBS:	/* match previous string */
			assert(np->n_msubs.n_indx+1 < 2*MAXPAR);
			pos = &saved[np->n_msubs.n_indx];
			cp = isprefix(cp, pos[0], pos[1]);
			if (cp == NULL)
				return (FALSE);
			break;
		case NSAVE:	/* save current location */
			assert(np->n_save.n_indx < 2*MAXPAR);
			saved[np->n_save.n_indx] = cp;
			break;
		case NCLO:	/* closure */
			cls = cp;
			++np;
			switch (np->n_comm.n_type) {
			case NMCH:	/* closure of match literal ushort */
				while (*cp == np->n_mch.n_char)
					++cp;
				break;
			case NMCL:	/* closuere of match class */
				while (inclass(*cp, np->n_mcl.n_cls))
					++cp;
				break;
			case NMANY:	/* closure of match any */
				while (*cp != EOS)
					++cp;
				break;
			default:
				assert(FALSE);
			}
			++np;
			loop {
				if (amatch(np, cp))
					return (TRUE);
				if (cp == cls)
					return (FALSE);
				--cp;
			}
		case NFINI:	/* done */
			return (TRUE);
		default:
			assert(FALSE);
		}
}


/*
 * Inclass returns TRUE iff `ch' is in the character class `cls'.
 */
static bool
inclass(ch, cls)
register uint	ch;
register ushort	*cls;
{
	ch %= CSIZE;
	return ((cls[ch/BPC] & (1 << ch%BPC)) != 0);
}


/*
 * Isprefix tests to see if the string [`fwa', `lim') is a prefix
 * to the string `str'.  If it is, it returns a pointer to the next
 * character in the string `str' after this prefix.  If not, it
 * returns NULL.
 */
static ushort	*
isprefix(str, fwa, lim)
register ushort	*str,
		*fwa,
		*lim;
{
	while (fwa != lim)
		if (*str++ != *fwa++)
			return (NULL);
	return (str);
}


static bool
syntax( )
{

	regerror = 's';
	return (FALSE);
}


static bool
nomem( )
{

	regerror = 'm';
	return (FALSE);
}
