/*
 * This include file declares all types internal to both the regular
 * expression compiler and the pattern matcher.  These declarations should
 * not be used just to use the pattern matching facility.
 */
#include "reg.h"


#define	MAXPAR	10			/* maximum no. of parens */
#define	EOS	'\0'			/* end of string */
#define	BPC	8			/* bits per character */
#define	CSIZE	(1<<BPC)		/* character set size */
#define	CVECL	((CSIZE + BPC-1) /BPC)	/* bytes per character set */


/*
 * The assert macro is used only for sanity checks.
 */
#ifdef	DEBUG
#define	assert(p)	if (not (p)) abort(); else
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
		uchar	n_char;		/* character to match */
	}	n_mch;
	struct	nmcl {			/* match character class */
		uchar	n_type;		/* NMCL */
		uchar	*n_cls;		/* character class to match */
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
		r_inst[1];		/* nfa instructions */
};
