
#ifndef _MWREGEXP_H
#define _MWREGEXP_H

#include "IDEConstants.h"

/**/
/* * Definitions etc. for regexp(3) routines.*/
/* **/
/* * Caveat:  this is V8 regexp(3) [actually, a reimplementation thereof],*/
/* * not the System V one.*/
/* */

#define NSUBEXP  10
typedef struct regexp  {
	char *startp[NSUBEXP];
	char *endp[NSUBEXP];
	char regstart;			/* Internal use only. */
	char reganch;			/* Internal use only. */
	char *regmust;			/* Internal use only. */
	int regmlen;			/* Internal use only. */
	char lastStr[255];		//	last-used expression. (?)
	char* match_text;		//	copy of the matched text
//	Handle match_text;		//	copy of the matched text
	char program[1];		/* Unwarranted chumminess with compiler. */
} regexp;

regexp *regcomp(char *exp);
long regexec(regexp *prog, char *string, char direction);
void regsub(regexp *prog, char *source, char *dest);

//	error codes from regcomp/regexec
enum  {
	no_error,

	/* if these ever happen, there's real trouble */
	invalid_args,
	invalid_pgm,
	corrupted_mem,
	corrupted_ptrs,
	internal_error,
	corrupted_opcode,
	damaged_match_str,
	junk_on_end,
	internal_urp,			/*	"internal urp", should never happen */
	internal_disaster,		/*	"internal disaster" */

	expr_too_complicated,
	out_of_memory,
	too_many_subexpr,
	unmatched_parens,
	empty_star_plus,		/* "*+ operand could be empty" */
	nested_repeater,		/* "nested *?+" */
	invalid_range,			/* "invalid [] range" */
	unmatched_bracket,
	bad_repeater,			/*	"?+* follows nothing" */
	trailing_backslash, 	/*	"trailing \\" */
	
	too_much_recursion,		/* We have passed our safe limit on recursion while
							   trying to evaluate an expression */

	last_error
};

extern regexp *regstruct;

typedef pascal void (*REErrorProc)(short code);

pascal bool REPrep(bool case_sensitive, bool reverse_search,
					  Str255 pattern, REErrorProc error_proc);

extern bool DoRESearch(bool reverse_search, bool match_words, Ptr text, 
                          long search_start, long search_limit, long *match_start, 
                          long *match_end);
#if 0
pascal bool RESearch(bool reverse_search, bool match_words,
						Handle text, long search_start, long search_limit,
						long *match_start, long *match_end);
#endif

char* REReplace(const char *repl_str);

#endif
