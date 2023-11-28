/* filechain.h*/

#if !defined(FILECHAIN_H)
#define FILECHAIN_H


#include <stdio.h>

typedef struct filechain {
	struct filechain	*prev;
	FILE				*file;
	int					line;
	char			*curname;
} filechain;
//new %%% mved from mwbres.l

#endif /* FILECHAIN_H */
