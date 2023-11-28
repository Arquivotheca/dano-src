//******************************************************************************
//	From File:		main.cpp,Screen Saver application.
//	Written by:	Eric Knight
//	Copyright 1993, Be Incorporated
//******************************************************************************

#include "BG_Erreur.h"
#include "BG_Types3D.h"
#include "BG_Memory.h"
#include "BG_Graphic.h"
#include "BG_Fractal.h"
#include "BG_Ground.h"
#include <time.h> 
#include <stdlib.h>
#include <math.h>
#include <Debug.h>

enum	{ ITERATIONS, CONTOUR, SMOOTHNESS };

char	menuCk[4];
long	wx,wy,wd,nIter,mapS,mapD;
long	*map = 0L;
double	xTh = -1.396;
double	tm[3][3];
double	contour,roughness,xc,sc,lx;
int32	roughness_table[maxIter+2];

// Make mountains a standard height and apply contour transform
void NormalizeMap()
{
	long n, m, k;
	long* a;
	double i, z;

	a = map;
	m = -0x80000000;

	// Find highest point in map
	for (n = mapS * mapS - 1; n >= 0; n--)
		if (*(a + n) > m)
			m = *(a + n);
	z = (double)m;
	z *= sqrt(z);
//	z = pow(z, contour);

	// Apply contour transform to it
	z = (double)10000 / z;

	// Find normalizing coefficient such that the range will be the chosen
	// height
	for (n = mapS * mapS - 1; n >= 0; n--) {
		k = *(a + n);
		if (k > 0) {
			i = (double)k;
//			i = pow(i, contour) * z;
			i = i * sqrt(i) * z;
			*(a + n) = (long)i;
		}
		else {
			i = (double)(-k);
//			i = pow(i, contour) * z;
			i = i * sqrt(i) * z;
			*(a + n) = -((long)i);
		}
	}
}

/*------------------------------------------------------------*/

// Return the max deviation a point could attain given an
// iteration depth (and roughness)
inline long MaxDeviation(long ic)
{
	return roughness_table[ic];
}

/*------------------------------------------------------------*/

// Deviates a point by a random amount in a given range
inline void DeviatePoint(long o, long ic)
{
	long v = MaxDeviation(ic);
	map[o] += random_prr(-v, v);
}

/*------------------------------------------------------------*/

// Passed a triangle with side (sx1, sy1) - (sx2, sy2) and apex
// (ax, ay), calculate midpoints and recurse. "c" is the iter.
// count
void IterCalc(long s1, long s2, long a, long c)
{
	long ns1, ns2, na;

	// Decrement iter count
	c--;

	// Find midpoints
	ns1 = (s1 + a) >> 1;
	ns2 = (s2 + a) >> 1;
	na = (s1 + s2) >> 1;

	// For each midpoint, if not already set, set to average of endpoints
	// and deviate
	if (map[ns1] == 0x7fffffff) {
		map[ns1] = (map[s1] + map[a]) >> 1;
		DeviatePoint(ns1, c);
	}
	if (map[ns2] == 0x7fffffff) {
		map[ns2] = (map[s2] + map[a]) >> 1;
		DeviatePoint(ns2, c);
	}
	if (map[na] == 0x7fffffff) {
		map[na] = (map[s1] + map[s2]) >> 1;
		DeviatePoint(na, c);
	}

	// Iterate calculations on sub-triangles if we haven't yet reached
	// max resolution of the map
	if (ns1 + 1 != ns2) {
		IterCalc(s1, na, ns1, c);
		IterCalc(na, s2, ns2, c);
		IterCalc(ns1, ns2, na, c);
		IterCalc(ns1, ns2, a, c);
	}
}

/*------------------------------------------------------------*/

// Initialize entire map to 0x7fffffff (not set value)
void InitMap()
{
	long n;
	long* a;

	a = map;
	for (n = mapS * mapS - 1; n >= 0; n--)
		*(a + n) = 0x7fffffff;
}

/*------------------------------------------------------------*/

void InitMountains()
	{
	int32		i;
	float		val;
	
	nIter = menuCk[ITERATIONS];
	mapD = 1 << nIter;
	mapS = mapD + 1;

/*
	contour = 0.25 * menuCk[CONTOUR];
	if (menuCk[CONTOUR] == 9)
		contour = 3.0;
	else if (menuCk[CONTOUR] == 10)
		contour = 5.0;
*/
	contour = 1.5;

	roughness = 0.75 + 0.25 * menuCk[SMOOTHNESS];
	val = 1.0;
	for (i=0; i<maxIter+2; i++) {
		roughness_table[i] = (int32)(val*8.0+0.5);
		val *= roughness;
	}

	lx = -1.0 / sqrt(3.0);

	map = (long*)BG_GetMemLow(sizeof(long)*mapS*mapS);

	InitMap();
	}

/*------------------------------------------------------------*/

// Calculate mountain range using current params, profile, etc...
void CalcMountains()
{
	long		i;

	InitMountains();
// initialise le point de reference
	Map(0, 0) = 0;
	Map(mapD, 0) = 0;
	Map(0, mapD) = 0;
	Map(mapD, mapD) = 0;
// Generate each main triangle recursively
	IterCalc(0, mapS - 1, mapS * mapS - 1, maxIter + 1);
// recopie les bords superieurs et droite en bas et a gauche
	for (i=0;i<mapS;i++)
		map[i+mapS*(mapS-1)] = map[i];
	for (i=0;i<mapS*mapS;i+=mapS)
		map[i] = map[i+mapS-1];
// Generate each main triangle recursively
	IterCalc(mapS * (mapS - 1), mapS * mapS - 1, 0, maxIter + 1);
	NormalizeMap();
	}

/***********************************************************
* What :	Init Ground model	
*			
* State : 	In progress
* Last Modif :	16/10/95 (Pierre)
*					
***********************************************************/
void BG_CalculMountains(float *Altitude,int taille,long RandomKey)
	{
	long		min,max,i,j,water,ground;
	float		base,zmax;
	
// regle les parametres aleatoires
	srand(RandomKey);
	if (taille == 32)
		{
		menuCk[ITERATIONS] = 5;
		zmax = 4.0;
		}
	else if (taille == 64)
		{
		menuCk[ITERATIONS] = 6;
		zmax = 8.0;
		}
	else if (taille == 128)
		{
		menuCk[ITERATIONS] = 7;
		zmax = 16.0;
		}
	else Erreur();
	BG_AltFact = 4.0/zmax;
	menuCk[CONTOUR] = 6;
	menuCk[SMOOTHNESS] = 5;
// reitere la generation aussi longtemps qu'il y a trop d'eau
	while (TRUE)
		{
		water = ground = 0L;
	// genere le terrain
		CalcMountains();
	// min and max processing
		min = 100000000;
		max = -100000000;
		for (i=0;i<mapS*mapS;i++)
			{
			if (map[i] < min)
				min = map[i];
			if (map[i] > max)
				max = map[i];
			}
	// put water in place
		min += (max-min)/3;
		for (i=0;i<mapS*mapS;i++)
			{
			if (map[i] < min)
				{
				map[i] = 0;
				water++;
				}
			else
				{
				map[i] -= min;
				ground++;
				}
			}
	// conversion
		base = zmax/(float)(max-min);
		for (i=0;i<mapS-1;i++)
			for (j=0;j<mapS-1;j++)
				Altitude[i+j*taille] = base*(float)map[i+j*mapS];
	// end
		BG_FreeMemLow((char*)map);
	// test de sortie
		if (water < ground) break;
		}
	}

