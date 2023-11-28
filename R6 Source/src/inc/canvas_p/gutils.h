#ifndef GUTILS_H
#define GUTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define DEGTORAD(deg) (M_PI*deg/180.0)
#define RADTODEG(rad) (180.0*rad/M_PI)

double const gmEPSILON =   1.0e-10;



inline double gmAbs(const double f)
{
  return (f >= 0) ? f : -f;
}

inline double gmInv(const double f)
{
  return 1.0 / f;
}

inline bool gmIsZero(const double f)
{
  return (gmAbs(f) < gmEPSILON);
}

/*
	These are things that are good for procedural texture generation
	and a couple more general routines.
	
	These are primarily provided by examples from Darwyn Peachey
	from his articles on precedural texture generation.
*/

#define LOG05  -0.693147180559945  /* log(0.5) */

#define FLOOR(x) ((int)(x) - ((x) < 0 && (x) != (int)(x)))
#define CEIL(x) ((int)(x) + ((x) > 0 && (x) != (int)(x)))
#define CLAMP(x,a,b) ((x) =< (a) ? (a) : ((x) >= (b) ? (b) : (x)))
#define LERP(t,x0,x1)  ((x0) + (t)*((x1)-(x0)))

#define PULSE(a,b,x) (step((a),(x)) - step((b),(x)))
#define boxstep(a,b,x) clamp(((x)-(a))/((b)-(a)),0,1)

extern float spline(float x, int nknots, float *knot);



inline float Abs(float x)
{
    return (x < 0 ? -x : x);
}

inline float bias(const float b, const float x)
{
    return powf(x, logf(b)/LOG05);
}

inline float clamp(float x, float a, float b)
{
	return (x < a ? a : (x > b ? b : x));
}

inline float gain(const float g, const float x)
{
    if (x < 0.5)
        return bias(1-g, 2*x)/2;
    else
        return 1 - bias(1-g, 2 - 2*x)/2;
}

inline float gammacorrect(const float gamma, const float x)
{
    return powf(x, 1/gamma);
}

inline float max(const float a, const float b)
{
    return (a < b ? b : a);
}

inline float min(const float a, const float b)
{
    return (a < b ? a : b);
}

inline float mod(const float A, const float b)
{
	float a = A;
	
    int n = (int)(a/b);

    a -= n*b;
    if (a < 0)
        a += b;

    return a;
}

inline float smoothstep(float a, float b, float x)
{
    if (x < a)
        return 0;
    if (x >= b)
        return 1;
    x = (x - a)/(b - a); /* normalize to [0:1] */

    return (x*x * (3 - 2*x));
}

inline float step(const float a, const float x)
{
    return (float)(x >= a);
}


#endif
