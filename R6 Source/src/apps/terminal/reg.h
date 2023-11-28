/*
 * This include file declares all of the externally visible objects from
 * the regular expression compiler/matcher.
 * Note, the actual layout of a compiled regular expression (a struct regprog)
 * is not externally visible.
 * The functions having to do with compiling a regular expression are:
 *
 *	struct regprog	*
 *	regcomp(regexp)
 *	uchar	*regexp;
 * 		Compile the regular expression `regexp' into a regprog (returns
 *		NULL if something goes wrong).
 *
 *	void
 *	regfree(rp)
 *	struct regprog	*rp;
 *		Release all space occupied by the regprog `rp'.
 *
 * The functions having to do with regular expression matching are:
 *
 *	bool
 *	regfind(rp, str)
 *	struct regprog	*rp;
 *	uchar		*str,
 *			*start;
 *		Look for the first occurance of the regular expression
 *		compiled in `rp' in the string starting at `str'.
 *
 *	bool
 *	regstep()
 *		Look for the next disjoint match of the same regular expression
 *		in the same string as the last call to regfind() or regstep().
 *		Note, it is only legal to call regstep() if the last call to
 *		either regfind() or regstep() returned TRUE.
 *
 *	uchar	*
 *	reglp(pnum)
 *	uint	pnum;
 *		Return a pointer (in the string passed in the last call to
 *		regfind()) to the first character that matched the `pnum'th
 *		parenthesized regular expression.  Note, the parenthesized
 *		expressions are numbered according to the order of occurance
 *		of the left parentheses.  Also, the zero'th set of parentheses
 *		always enclose the entire regular expression.
 *		If there were not atleast `pnum' parentheses the regular
 *		expression passed in the last call to regfind, then NULL is
 *		returned.
 *		Note, it is only legal to call reglp() if the last call to
 *		either regfind() or regstep() returned TRUE.
 *
 *	uchar	*
 *	regrp(pnum)
 *	uint	pnum;
 *		Exactly the same as reglp() except that instead of a pointer
 *		to the first character that matched, a pointer to just past
 *		the last character that matched is returned.
 */
extern struct regprog	*regcomp( ushort *);			/* compile regular expression */
extern void		regfree( struct regprog *);		/* free space used by compiled exp. */
extern bool		regfind( struct regprog	*, ushort *),	/* search for first occurance */
			regstep( );				/* search for next occurance */
extern ushort		*reglp( uint),				/* return start of match */
			*regrp( uint);				/* return end of match */
extern int		regerror;
