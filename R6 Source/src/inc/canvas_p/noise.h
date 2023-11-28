
/*
 * Declarations and preprocessor definitions used in the various noise
 * functions.
 * Darwyn Peachey, June, 1994.
 */

#ifndef _NOISE_H_
#define _NOISE_H_ 1

#define TABSIZE          256
#define TABMASK          (TABSIZE-1)
#define PERM(x)          perm[(x)&TABMASK]
#define INDEX(ix,iy,iz)  PERM((ix)+PERM((iy)+PERM(iz)))

#define RANDMASK  0x7fffffff
#define RANDNBR   ((random() & RANDMASK)/(double) RANDMASK)

extern unsigned char perm[TABSIZE];	

extern float catrom2(float d);
extern float gnoise(float x, float y, float z);
extern float gvnoise(float x, float y, float z);
extern float scnoise(float x, float y, float z);
extern float vcnoise(float x, float y, float z);
extern float vnoise(float x, float y, float z);

#endif /* _NOISE_H_ */
