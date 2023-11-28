/*
 * This file contains all routines having to do with regular expression
 * matching.
 */
#include "ali.h"
#include "regi.h"


/*
 * Local variables.
 * Note, saved and nsaved are only valid if the last call to regfind or
 * regstep returned TRUE.
 */
static uchar	*saved[2*MAXPAR];	/* saved locations */
static struct regprog	*prog;		/* current regular expression */


/*
 * Local functions.
 */
static bool	run(),			/* match controller */
		amatch(),		/* anchored match routine */
		inclass();		/* test if uchar in class */
static uchar	*isprefix();		/* test for string prefix */


/*
 * Reglp returns a pointer (in the string last passed to regfind())
 * to the first character that matched the `pnum'th parenthesized
 * regular expression (where parenthesis pairs are numbered by the
 * order of occurance of the left parentheses).  If there were not
 * atleast `pnum' parentheses then reglp() returns NULL.
 * Note, it is an error to call reglp if the last call to regfind or
 * regstep did not return TRUE.
 */
uchar	*
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
uchar	*
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
uchar		*str;
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
	register uchar	*str;

	str = regrp(0);
	if ((str == reglp(0))
	and (*str++ == EOS))
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
register uchar	*str;
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
			if ((bol || not isalnum(str[-1]))
			and (amatch(pc, str)))
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
				and (amatch(pc, str)))
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
register uchar	*cp;
{
	uchar	*cls,
		**pos;

	for (;; ++np)
		switch (np->n_comm.n_type) {
		case NMCH:	/* match literal character */
			if (*cp++ != np->n_mch.n_char)
				return (FALSE);
			break;
		case NMCL:	/* match character class */
			if (not inclass(*cp++, np->n_mcl.n_cls))
				return (FALSE);
			break;
		case NMANY:	/* match any character */
			if (*cp++ == EOS)
				return (FALSE);
			break;
		case NMEOS:	/* match end of string */
			return (*cp == EOS);
		case NMEW:	/* match end of word */
			return (not isalnum(*cp));
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
			case NMCH:	/* closure of match literal uchar */
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
register uchar	*cls;
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
static uchar	*
isprefix(str, fwa, lim)
register uchar	*str,
		*fwa,
		*lim;
{
	while (fwa != lim)
		if (*str++ != *fwa++)
			return (NULL);
	return (str);
}
