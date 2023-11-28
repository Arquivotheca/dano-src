/* ++++++++++
	FILE:	BG_Goureaud.cpp
	REVS:	$Revision$
	NAME:	pierre
	DATE:	Tue Jun 17 21:42:13 PDT 1997
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#include "BG_Goureaud.h"
#include <Debug.h>

extern short    tabgauche[];
extern short    tabdroite[];
extern long     OffScreen_largeur,OffScreen_baseAddr;
extern short    tabgauche2[];
extern short    tabdroite2[];
extern long     OffScreen_largeur2,OffScreen_baseAddr2;

void BG_EraseTampon (uint32 *dst, int rowbyte, int rowsize, int nbline, uint32 color) {
	int      i;

	rowbyte >>= 2;
	rowsize >>= 2;
	for (; nbline>0; nbline--) {
		for (i=0; i<rowsize; i++)
			dst[i] = color;
		dst += rowbyte;
	}
}

void BG_Goureaud(BG_GoureaudPt *Tab,unsigned char *Color) {
	int32           i, min, max, y1, y2, x1, x2, l1, tmp;
	int32           delta_x, delta_y, posx, hmin, lmin, delta;
	int32           pash2, pash, pasv, L21, L31, H21, H31, V21, V31;
	int32           largeur, lum;
	int16           *tableau;
	int16           *tabg, *tabd;
	uchar           *Color2;
	uchar           *drawline;

/*	__sPrintf("(%d,%d) (%d,%d) (%d,%d)\n",
			 Tab[0].H, Tab[0].V,
			 Tab[1].H, Tab[2].V,
			 Tab[2].H, Tab[2].V);*/

	Tab[3].H = Tab[0].H;
	Tab[3].V = Tab[0].V;

	min = 1999;
	max = 0;

	for (i=0; i<3; i++) {
		y1 = Tab[i].V;
		y2 = Tab[i+1].V;
		if (y1 == y2) continue;
		if (y1 < y2) {
			x1 = Tab[i].H;
			x2 = Tab[i+1].H;
			l1 = Tab[i].Level;
			tableau = tabdroite;
		}
		else {
			tmp = y1;
			y1 = y2;
			y2 = tmp;
			x1 = Tab[i+1].H;
			x2 = Tab[i].H;
			l1 = Tab[i+1].Level;
			tableau = tabgauche;
		}
		if (y1 < min) {
			min = y1;
			hmin = x1;
			lmin = l1;
		}
		if (y2 > max)
			max = y2;
		delta_x = x2-x1;
		delta_y = y2-y1;
		delta = (delta_x<<16)/delta_y;
		posx = (x1<<16) | (1<<15);
		for (; y1<y2; y1++) {
			tableau[y1] = posx>>16;
			posx += delta;
		}
	}

	if (min >= max) return;

	L21 = Tab[1].Level-Tab[0].Level;
	L31 = Tab[2].Level-Tab[0].Level;
	H21 = Tab[1].H-Tab[0].H;
	H31 = Tab[2].H-Tab[0].H;
	V21 = Tab[1].V-Tab[0].V;
	V31 = Tab[2].V-Tab[0].V;
	pash = L21*V31 - L31*V21;
	pasv = L31*H21 - L21*H31;
	tmp = H21*V31 - H31*V21;
	if (tmp == 0) return;
	pash = (pash<<12)/tmp;
	pasv = (pasv<<12)/tmp;
	pash2 = pash<<1;
	lmin <<= 12;

	largeur = OffScreen_largeur;
	drawline = (uchar*)OffScreen_baseAddr + largeur*min;
	lmin -= hmin*pash;
	tabg = tabgauche;
	tabd = tabdroite;
	Color2 = Color+512;
	
	if (min & 1) goto odd;
	while (TRUE) {
		x2 = tabg[min];
		x1 = tabd[min];
		x1 = x1-x2;
		lum = lmin+pash*x2;
		if (x1&1) {
			drawline[x2] = Color[lum>>11];
			lum += pash;
			x2++;
		}
		for (; x1>1; x1-=2) {
			*(uint16*)(drawline+x2) = *(uint16*)(Color+(lum>>11));
			lum += pash2;
			x2 += 2;
		}
		min++;
		if (min == max) break;
		drawline += largeur;
		lmin += pasv;
	odd:
		x2 = tabg[min];
		x1 = tabd[min];
		x1 = x1-x2;
		lum = lmin+pash*x2;
		if (x1&1) {
			drawline[x2] = Color2[lum>>11];
			lum += pash;
			x2++;
		}
		for (; x1>1; x1-=2) {
			*(uint16*)(drawline+x2) = *(uint16*)(Color2+(lum>>11));
			lum += pash2;
			x2 += 2;
		}
		min++;
		if (min == max) break;
		drawline += largeur;
		lmin += pasv;
	}
}

void BG_Goureaud2(BG_GoureaudPt *Tab,unsigned char *Color) {
	int32           i, min, max, y1, y2, x1, x2, l1, tmp;
	int32           delta_x, delta_y, posx, hmin, lmin, delta;
	int32           pash2, pash, pasv, L21, L31, H21, H31, V21, V31;
	int32           largeur, lum;
	int16           *tableau;
	int16           *tabg, *tabd;
	uchar           *Color2;
	uchar           *drawline;

	Tab[3].H = Tab[0].H;
	Tab[3].V = Tab[0].V;

	min = 1999;
	max = 0;

	for (i=0; i<3; i++) {
		y1 = Tab[i].V;
		y2 = Tab[i+1].V;
		if (y1 == y2) continue;
		if (y1 < y2) {
			x1 = Tab[i].H;
			x2 = Tab[i+1].H;
			l1 = Tab[i].Level;
			tableau = tabdroite2;
		}
		else {
			tmp = y1;
			y1 = y2;
			y2 = tmp;
			x1 = Tab[i+1].H;
			x2 = Tab[i].H;
			l1 = Tab[i+1].Level;
			tableau = tabgauche2;
		}
		if (y1 < min) {
			min = y1;
			hmin = x1;
			lmin = l1;
		}
		if (y2 > max)
			max = y2;
		delta_x = x2-x1;
		delta_y = y2-y1;
		delta = (delta_x<<16)/delta_y;
		posx = (x1<<16) | (1<<15);
		for (; y1<y2; y1++) {
			tableau[y1] = posx>>16;
			posx += delta;
		}
	}

	if (min >= max) return;

	L21 = Tab[1].Level-Tab[0].Level;
	L31 = Tab[2].Level-Tab[0].Level;
	H21 = Tab[1].H-Tab[0].H;
	H31 = Tab[2].H-Tab[0].H;
	V21 = Tab[1].V-Tab[0].V;
	V31 = Tab[2].V-Tab[0].V;
	pash = L21*V31 - L31*V21;
	pasv = L31*H21 - L21*H31;
	tmp = H21*V31 - H31*V21;
	if (tmp == 0) return;
	pash = (pash<<12)/tmp;
	pasv = (pasv<<12)/tmp;
	pash2 = pash<<1;
	lmin <<= 12;

	largeur = OffScreen_largeur2;
	drawline = (uchar*)OffScreen_baseAddr2+largeur*min;
	lmin -= hmin*pash;
	tabg = tabgauche2;
	tabd = tabdroite2;
	Color2 = Color+512;
	
	if (min & 1) goto odd;
	while (TRUE) {
		x2 = tabg[min];
		x1 = tabd[min];
		x1 = x1-x2;
		lum = lmin+pash*x2;
		if (x1&1) {
			drawline[x2] = Color[lum>>11];
			lum += pash;
			x2++;
		}
		for (; x1>1; x1-=2) {
			*(uint16*)(drawline+x2) = *(uint16*)(Color+(lum>>11));
			lum += pash2;
			x2 += 2;
		}
		min++;
		if (min == max) break;
		drawline += largeur;
		lmin += pasv;
	odd:
		x2 = tabg[min];
		x1 = tabd[min];
		x1 = x1-x2;
		lum = lmin+pash*x2;
		if (x1&1) {
			drawline[x2] = Color2[lum>>11];
			lum += pash;
			x2++;
		}
		for (; x1>1; x1-=2) {
			*(uint16*)(drawline+x2) = *(uint16*)(Color2+(lum>>11));
			lum += pash2;
			x2 += 2;
		}
		min++;
		if (min == max) break;
		drawline += largeur;
		lmin += pasv;
	}
}

void BG_Goureaud24(BG_GoureaudPt *Tab,unsigned char *Color) {
	int32           i, min, max, y1, y2, x1, x2, l1, tmp;
	int32           delta_x, delta_y, posx, hmin, lmin, delta;
	int32           pash, pasv, L21, L31, H21, H31, V21, V31;
	int32           largeur, lum;
	int16           *tableau;
	int16           *tabg, *tabd;
	uchar           *drawline;

	Tab[3].H = Tab[0].H;
	Tab[3].V = Tab[0].V;

	min = 1999;
	max = 0;

	for (i=0; i<3; i++) {
		y1 = Tab[i].V;
		y2 = Tab[i+1].V;
		if (y1 == y2) continue;
		if (y1 < y2) {
			x1 = Tab[i].H;
			x2 = Tab[i+1].H;
			l1 = Tab[i].Level;
			tableau = tabdroite;
		}
		else {
			tmp = y1;
			y1 = y2;
			y2 = tmp;
			x1 = Tab[i+1].H;
			x2 = Tab[i].H;
			l1 = Tab[i+1].Level;
			tableau = tabgauche;
		}
		if (y1 < min) {
			min = y1;
			hmin = x1;
			lmin = l1;
		}
		if (y2 > max)
			max = y2;
		delta_x = x2-x1;
		delta_y = y2-y1;
		delta = (delta_x<<16)/delta_y;
		posx = (x1<<16) | (1<<15);
		for (; y1<y2; y1++) {
			tableau[y1] = posx>>16;
			posx += delta;
		}
	}

	if (min >= max) return;

	L21 = Tab[1].Level-Tab[0].Level;
	L31 = Tab[2].Level-Tab[0].Level;
	H21 = Tab[1].H-Tab[0].H;
	H31 = Tab[2].H-Tab[0].H;
	V21 = Tab[1].V-Tab[0].V;
	V31 = Tab[2].V-Tab[0].V;
	pash = L21*V31 - L31*V21;
	pasv = L31*H21 - L21*H31;
	tmp = H21*V31 - H31*V21;
	if (tmp == 0) return;
	pash = (pash<<12)/tmp;
	pasv = (pasv<<12)/tmp;
	lmin <<= 12;

	largeur = OffScreen_largeur;
	drawline = (uchar*)OffScreen_baseAddr+largeur*min;
	lmin -= hmin*pash;
	tabg = tabgauche;
	tabd = tabdroite;
	
	for (; min<max; min++) {
		x2 = tabg[min];
		x1 = tabd[min];
		lum = lmin+pash*x2;
		for (; x2<x1; x2++) {
			((uint32*)drawline)[x2] = *(uint32*)(Color+((lum>>12)<<2));
			lum += pash;
		}
		drawline += largeur;
		lmin += pasv;
	}
}

void BG_Goureaud242(BG_GoureaudPt *Tab,unsigned char *Color) {
	int32           i, min, max, y1, y2, x1, x2, l1, tmp;
	int32           delta_x, delta_y, posx, hmin, lmin, delta;
	int32           pash, pasv, L21, L31, H21, H31, V21, V31;
	int32           largeur, lum;
	int16           *tableau;
	int16           *tabg, *tabd;
	uchar           *drawline;

	Tab[3].H = Tab[0].H;
	Tab[3].V = Tab[0].V;

	min = 1999;
	max = 0;

	for (i=0; i<3; i++) {
		y1 = Tab[i].V;
		y2 = Tab[i+1].V;
		if (y1 == y2) continue;
		if (y1 < y2) {
			x1 = Tab[i].H;
			x2 = Tab[i+1].H;
			l1 = Tab[i].Level;
			tableau = tabdroite2;
		}
		else {
			tmp = y1;
			y1 = y2;
			y2 = tmp;
			x1 = Tab[i+1].H;
			x2 = Tab[i].H;
			l1 = Tab[i+1].Level;
			tableau = tabgauche2;
		}
		if (y1 < min) {
			min = y1;
			hmin = x1;
			lmin = l1;
		}
		if (y2 > max)
			max = y2;
		delta_x = x2-x1;
		delta_y = y2-y1;
		delta = (delta_x<<16)/delta_y;
		posx = (x1<<16) | (1<<15);
		for (; y1<y2; y1++) {
			tableau[y1] = posx>>16;
			posx += delta;
		}
	}

	if (min >= max) return;

	L21 = Tab[1].Level-Tab[0].Level;
	L31 = Tab[2].Level-Tab[0].Level;
	H21 = Tab[1].H-Tab[0].H;
	H31 = Tab[2].H-Tab[0].H;
	V21 = Tab[1].V-Tab[0].V;
	V31 = Tab[2].V-Tab[0].V;
	pash = L21*V31 - L31*V21;
	pasv = L31*H21 - L21*H31;
	tmp = H21*V31 - H31*V21;
	if (tmp == 0) return;
	pash = (pash<<12)/tmp;
	pasv = (pasv<<12)/tmp;
	lmin <<= 12;

	largeur = OffScreen_largeur2;
	drawline = (uchar*)OffScreen_baseAddr2+largeur*min;
	lmin -= hmin*pash;
	tabg = tabgauche2;
	tabd = tabdroite2;
	
	for (; min<max; min++) {
		x2 = tabg[min];
		x1 = tabd[min];
		lum = lmin+pash*x2;
		for (; x2<x1; x2++) {
			((uint32*)drawline)[x2] = *(uint32*)(Color+((lum>>12)<<2));
			lum += pash;
		}
		drawline += largeur;
		lmin += pasv;
	}
}









