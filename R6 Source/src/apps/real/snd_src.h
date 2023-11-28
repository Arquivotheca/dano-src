#include <math.h>

#ifndef	SND_SRC
#define	SND_SRC

/*------------------------------------------------------------*/

class	SndSrc {
public:
						SndSrc(char *path);
virtual					~SndSrc();
virtual		float		GetValue(float i, float dt);

			short		*buffer;
			long		length;
};

/*------------------------------------------------------------*/

#endif
