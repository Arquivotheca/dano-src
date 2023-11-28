

#include	<stdio.h>
#include	"rico.h"


extern bool	gflag;
extern uint	sdepth,
		ecount;

bool		nomem( ),
		complain( char *, ...);
void		*malloc( );
