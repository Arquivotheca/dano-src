#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <byteorder.h>
#include <Alert.h>
#include "util.h"

//--------------------------------------------------------------
// used by color mixing function.
// Will take two components and mix them using the formulae
// 0.5*c1 + 0.8 * c2
// clip for overflow.
//--------------------------------------------------------------

long	xcmix(long v1, long v2)
{
	long	tmp;
	
	tmp = (int)(v2 + v1*1.6);
	tmp = tmp / 2;
	if (tmp > 255) tmp = 255;
	return tmp;
}

//--------------------------------------------------------------
// mixing function used for the blending of polygons in the 3d
// view.
// includes a specular component.
//--------------------------------------------------------------

long	cmix(long v1, long v2)
{
	long	tmp;
	

	tmp = v1 * v2;							//transparency component
	tmp = tmp / 512;
	tmp += (int)(v2/1.9);						//add a fraction of the background for non perfect filter effect
	tmp += (int)(v1 * 0.95);				//add forward illumination.
	tmp = (int)(tmp*0.8);
	
	if (tmp > 255) tmp = 255;
	return tmp;
}

//--------------------------------------------------------------
// fast square root function.
// good aproximation for 3d work.
//--------------------------------------------------------------

float b_sqrt(float x) {
	unsigned long	val;
	float			y,z,t;
	float	        flottant, tampon;
	
	flottant = x;
	val = *((unsigned long*)&flottant);
	val >>= 1;
	val += 0x1FC00000L;
	*((unsigned long*)&tampon) = val;
	y = tampon;
	z = y*y+x;
	t = y*y-x;
	y *= (float)4.0;
	x = z*z;
	t = t*t;
	y = z*y;
	t = (float)2.0*x-t;
	return t/y;
}

//--------------------------------------------------------------
// IO wrapper with endiannes conversion
//--------------------------------------------------------------

int32	read32(int ref)
{
	ulong	tmp;

	read(ref, &tmp, sizeof(tmp));

#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
	tmp = __swap_int32(tmp);
#endif

	return tmp;
}

//-------------------------------------------------------------------------
// IO wrapper with endiannes conversion
//--------------------------------------------------------------

void	write32(int ref, int32 v)
{

#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
	v = __swap_int32(v);
#endif
	
	write(ref, &v, sizeof(v));
}


//-------------------------------------------------------------------------
// IO wrapper with endiannes conversion
//--------------------------------------------------------------

float	readf(int ref)
{
	float	tmp;

	read(ref, &tmp, sizeof(tmp));

#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
	tmp = __swap_float(tmp);
#endif

	return tmp;
}


//-------------------------------------------------------------------------
// IO wrapper with endiannes conversion
//--------------------------------------------------------------

void	writef(int ref, float tmp)
{

#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
	tmp = __swap_float(tmp);
#endif

	write(ref, &tmp, sizeof(tmp));
}

//--------------------------------------------------------------
// alert for near out of memory conditions
//--------------------------------------------------------------
		
void	NotifyMemory(char *s1, char *s2)
{
		BAlert   *alert;
		
		alert = new BAlert("", s1,
						   s2, NULL, NULL);
		alert->Go();
}
