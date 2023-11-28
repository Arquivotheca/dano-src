/* ++++++++++

   FILE:  Mapper32.cpp
   REVS:  $Revision: 1.3 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#include <stdlib.h>
#include <string.h>
#include <Debug.h>

#ifndef _3D_RENDERED_H
#include "3dRenderer.h"
#endif

void MapTriangle(long h1, long v1, ulong col1, float x1, float y1, float z1,
				 long h2, long v2, ulong col2, float x2, float y2, float z2,
				 long h3, long v3, ulong col3, float x3, float y3, float z3,
				 void *bits, long row,
				 void *map, long Hsize, long Vsize) {
	long			Dh1,Dv1,Dh2,Dv2,Dh3,Dv3;
	long			dh1,dh2,dh3;
	long			r0b,g0b,b0b,Dh0,inv_Dh0;
	long            inv_Dv1,inv_Dv2,inv_Dv3;
	long			r1,g1,b1,r2,g2,b2,r3,g3,b3,r0,g0,b0;
	long			dr1,dr2,dg1,dg2,db1,db2;
	long			curR,curG,curB,curR0,curG0,curB0;
	uchar	    	*draw,*draw2;
	long			H1,H2,H1b,H2b;
	long            Size, Count;
	ulong           index;
	uchar           plot;
	ulong			temp,col,offset,Hmask,Vmask;
	long            H0,V0,H,V,dH,dV;
	float           x0b,y0b,z0b;
	float           dx1,dx2,dy1,dy2,dz1,dz2;
	float			curX,curY,curZ,curX0,curY0,curZ0;
	float           ftemp,scale;

//	_sPrintf("base : %x\n", map);
	// reordonne les points	
	if (v2 < v3) {
		if (v2 >= v1)
			goto lab1;
	}
	else if (v3 < v1) {
		// v3 le plus haut
		temp = col1;
		col1 = col2;
		col2 = col3;
		col3 = temp;
		temp = h1;
		h1 = h2;
		h2 = h3;
		h3 = temp;
		temp = v1;
		v1 = v2;
		v2 = v3;
		v3 = temp;
		ftemp = x1;
		x1 = x2;
		x2 = x3;
		x3 = ftemp;
		ftemp = y1;
		y1 = y2;
		y2 = y3;
		y3 = ftemp;
		ftemp = z1;
		z1 = z2;
		z2 = z3;
		z3 = ftemp;
	}
	else {
		// v1 le plus haut
lab1:  	temp = col1;
		col1 = col3;
		col3 = col2;
		col2 = temp;
		temp = h1;
		h1 = h3;
		h3 = h2;
		h2 = temp;
		temp = v1;
		v1 = v3;
		v3 = v2;
		v2 = temp;
		ftemp = x1;
		x1 = x3;
		x3 = x2;
		x2 = ftemp;
		ftemp = y1;
		y1 = y3;
		y3 = y2;
		y2 = ftemp;
		ftemp = z1;
		z1 = z3;
		z3 = z2;
		z2 = ftemp;
	}
	Hmask = ((1<<Hsize)-1)<<2;
	Vmask = ((1<<Vsize)-1)<<(Hsize+2);
	// extrait les composantes
	r1 = ((col1>>ROTR)&0xff);
	g1 = ((col1>>ROTG)&0xff);
	b1 = ((col1>>ROTB)&0xff);
	r2 = ((col2>>ROTR)&0xff);
	g2 = ((col2>>ROTG)&0xff);
	b2 = ((col2>>ROTB)&0xff);
	r3 = ((col3>>ROTR)&0xff);
	g3 = ((col3>>ROTG)&0xff);
	b3 = ((col3>>ROTB)&0xff);
	// calcule les vecteurs lateraux
	Dh1 = h1-h2;
	Dh2 = h3-h2;
	Dv1 = v1-v2;
	Dv2 = v3-v2;
	// preprocess the x and y parameters.
	x1 *= z1;
	x2 *= z2;
	x3 *= z3;
	y1 *= z1;
	y2 *= z2;
	y3 *= z3;
	// separe les cas horizontaux et les inclinaisons
	r0 = 0x80;
	g0 = 0x80;
	b0 = 0x80;
	if (Dv1 < Dv2) {
		if (Dv1 == 0) {
			// cas 1 = 0 > 2
			inv_Dv2 = invert[Dv2];
			dh2 = Dh2*inv_Dv2;
			if (Dh1 >= 0) return;
			dr1 = (r3-r2)*inv_Dv2;
			dg1 = (g3-g2)*inv_Dv2;
			db1 = (b3-b2)*inv_Dv2;
		scale = ((float)inv_Dv2)*(1.0/FLOAT_EXPO);
		dx1 = (x3-x2)*scale;
		dy1 = (y3-y2)*scale;
		dz1 = (z3-z2)*scale;
			Dh3 = Dh2-Dh1;
			Dv3 = Dv2-Dv1;
			inv_Dv3 = invert[Dv3];
			dh1 = Dh3*inv_Dv3;
			if (Dh1 == 0) Dh1 = -1;
			inv_Dh0 = invert[-Dh1];
			dr2 = (r2-r1)*inv_Dh0;
			dg2 = (g2-g1)*inv_Dh0;
			db2 = (b2-b1)*inv_Dh0;
		scale = ((float)inv_Dh0)*(1.0/FLOAT_EXPO);
		dx2 = (x2-x1)*scale;
		dy2 = (y2-y1)*scale;
		dz2 = (z2-z1)*scale;
			// debut de ligne haute
			draw = (uchar*)((long)bits+row*(v2-1));
			H2 = (h2<<EXPO)|(1<<(EXPO-1));
			H1 = (h1<<EXPO)|(1<<(EXPO-1));
			Dv1 = -1;
			curR0 = (r2<<EXPO)|(1<<(EXPO-1));
			curG0 = (g2<<EXPO)|(1<<(EXPO-1));
			curB0 = (b2<<EXPO)|(1<<(EXPO-1));
		curX0 = x2;
		curY0 = y2;
		curZ0 = z2;
			goto end1;
		}
		else {
			// cas 1 > 0 > 2
			inv_Dv1 = invert[Dv1];
			inv_Dv2 = invert[Dv2];
			dh1 = Dh1*inv_Dv1;
			dh2 = Dh2*inv_Dv2;
			if (dh1 >= dh2) return;
			dr1 = (r3-r2)*inv_Dv2;
			dg1 = (g3-g2)*inv_Dv2;
			db1 = (b3-b2)*inv_Dv2;
		scale = ((float)inv_Dv2)*(1.0/FLOAT_EXPO);
		dx1 = (x3-x2)*scale;
		dy1 = (y3-y2)*scale;
		dz1 = (z3-z2)*scale;
			Dh3 = Dh2-Dh1;
			Dv3 = Dv2-Dv1;
			inv_Dv3 = invert[Dv3];
			dh3 = Dh3*inv_Dv3;
			r0b = ((dr1*Dv1+(1<<(EXPO-1)))>>EXPO)+r2;
			g0b = ((dg1*Dv1+(1<<(EXPO-1)))>>EXPO)+g2;
			b0b = ((db1*Dv1+(1<<(EXPO-1)))>>EXPO)+b2;
			Dh0 = ((dh2*Dv1+(1<<(EXPO-1)))>>EXPO)-Dh1;
		scale = (float)Dv1;
		x0b = dx1*scale+x2;
		y0b = dy1*scale+y2;
		z0b = dz1*scale+z2;
			if (Dh0 == 0) Dh0 = 1;
			inv_Dh0 = invert[Dh0];
			dr2 = r0b-r1;
			dg2 = g0b-g1;
			db2 = b0b-b1;
			dr2 *= inv_Dh0;
			dg2 *= inv_Dh0;
			db2 *= inv_Dh0;
		scale = ((float)inv_Dh0)*(1.0/FLOAT_EXPO);
		dx2 = (x0b-x1)*scale;
		dy2 = (y0b-y1)*scale;
		dz2 = (z0b-z1)*scale;
			// debut de ligne haute
end0:	    draw = (uchar*)((long)bits+row*v2);
			H2 = (h2<<EXPO)|(1<<(EXPO-1));
			H1 = H2+dh1;
			H2 += dh2;
			Dv1 = Dv2-Dv1;
			Dv2--;
			// couleur du pointeur de trace
			curR0 = (r2<<EXPO)|(1<<(EXPO-1));
			curG0 = (g2<<EXPO)|(1<<(EXPO-1));
			curB0 = (b2<<EXPO)|(1<<(EXPO-1));
			curR0 += dr1;
			curG0 += dg1;
			curB0 += db1;
		curX0 = x2+dx1;
		curY0 = y2+dy1;
		curZ0 = z2+dz1;
end1:		// offset by one for backward rendering
			curR0 -= dr2;
			curG0 -= dg2;
			curB0 -= db2;
		curX0 -= dx2;
		curY0 -= dy2;
		curZ0 -= dz2;
			// mapping step
		dx2 *= PROJECT_STEP_FACT;
		dy2 *= PROJECT_STEP_FACT;
		dz2 *= PROJECT_STEP_FACT;
			// traceur
			do {
				if (Dv2 == Dv1) {
					dh1 = dh3;
					H1 &= ((1<<31)-1)<<EXPO;
					H1 |= (1<<(EXPO-1));
				}
				H1b = (H1>>EXPO)&((1<<(32-EXPO))-1);
				H2b = (H2>>EXPO)&((1<<(32-EXPO))-1);
				Dv2--;
				H1 += dh1;
				H2 += dh2;
				Size = H2b-H1b;
				draw = (uchar*)((long)draw+row);
				if (Size > 0) {
				curX = curX0;
				curY = curY0;
				curZ = curZ0;
				scale = FACTOR/curZ;
					Count = 0;
					curR = curR0;
					curG = curG0;
					curB = curB0;
					draw2 = (uchar*)((long)draw+H2b);
				H0 = (long)(curX*scale);
				V0 = (long)(curY*scale);
				
					do {
						if ((Count & PROJECT_STEP_MASK) == 0) {
							curZ -= dz2;
							scale = FACTOR/curZ;
							curX -= dx2;
							curY -= dy2;
							H = H0;
							V = V0;
							H0 = (long)(curX*scale);
							V0 = (long)(curY*scale);
							dH = (H0-H)>>PROJECT_STEP_EXP;
							dV = (V0-V)>>PROJECT_STEP_EXP;
						}
						offset = (H>>(EXPOFAC-2))&Hmask;
						offset |= ((V>>(EXPOFAC-2))<<Hsize)&Vmask;
						col = *((ulong*)((long)map+offset));
						Count += 1;
						r1 = ((col>>ROTR)&0xff);
						g1 = ((col>>ROTG)&0xff);
						b1 = ((col>>ROTB)&0xff);
						r0 += r1*(curR>>EXPO);
						if (r0 > 0xffff)
							r0 = 0xffff;
						if (r0 < 0)
							r0 = 0;
						index = (r0>>1)&0x7c00;
						g0 += g1*(curG>>EXPO);
						if (g0 > 0xffff)
							g0 = 0xffff;
						if (g0 < 0)
							g0 = 0;
						index |= (g0>>6)&0x03e0;
						b0 += b1*(curB>>EXPO);
						if (b0 > 0xffff)
							b0 = 0xffff;
						if (b0 < 0)
							b0 = 0;
						index |= b0>>11;
						plot = index_map[index];
						H += dH;
						V += dV;
						col = color_list[plot];
						*--draw2 = plot;
						curR -= dr2;
						curG -= dg2;
						curB -= db2;
#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
                        r0 -= (col<<8)&0xff00;
                        g0 -= (col)&0xff00;
                        b0 -= (col>>8)&0xff00;
#else
						r0 -= (col>>16)&0xff00;
						g0 -= (col>>8)&0xff00;
						b0 -= (col)&0xff00;
#endif
					} while (Count < Size);
					
				}
				curR0 += dr1;
				curG0 += dg1;
				curB0 += db1;
			curX0 += dx1;
			curY0 += dy1;
			curZ0 += dz1;
			} while (Dv2 > 0);			
		}
	}
	else if (Dv2 < Dv1) {
		if (Dv2 == 0) {
			// cas 1 = 2 > 0
			inv_Dv1 = invert[Dv1];
			dh1 = Dh1*inv_Dv1;
			if (Dh2 < 0) return;
			dr1 = (r1-r2)*inv_Dv1;
			dg1 = (g1-g2)*inv_Dv1;
			db1 = (b1-b2)*inv_Dv1;
		scale = ((float)inv_Dv1)*(1.0/FLOAT_EXPO);
		dx1 = (x1-x2)*scale;
		dy1 = (y1-y2)*scale;
		dz1 = (z1-z2)*scale;
			Dh3 = Dh1-Dh2;
			Dv3 = Dv1-Dv2;
			inv_Dv3 = invert[Dv3];
			dh2 = Dh3*inv_Dv3;
			if (Dh2 == 0) Dh2 = 1;
			inv_Dh0 = invert[Dh2];
			dr2 = (r2-r3)*inv_Dh0;
			dg2 = (g2-g3)*inv_Dh0;
			db2 = (b2-b3)*inv_Dh0;
		scale = ((float)inv_Dh0)*(1.0/FLOAT_EXPO);
		dx2 = (x2-x3)*scale;
		dy2 = (y2-y3)*scale;
		dz2 = (z2-z3)*scale;
			// debut de ligne haute
			draw = (uchar*)((long)bits+row*(v2-1)-1);
			H2 = (h3<<EXPO)|(1<<(EXPO-1));
			H1 = (h2<<EXPO)|(1<<(EXPO-1));
			Dv2 = -1;
			curR0 = (r2<<EXPO)|(1<<(EXPO-1));
			curG0 = (g2<<EXPO)|(1<<(EXPO-1));
			curB0 = (b2<<EXPO)|(1<<(EXPO-1));
		curX0 = x2;
		curY0 = y2;
		curZ0 = z2;
			goto end2;
		}
		else {
			// cas 1 > 2 > 0
			inv_Dv1 = invert[Dv1];
			inv_Dv2 = invert[Dv2];
			dh1 = Dh1*inv_Dv1;
			dh2 = Dh2*inv_Dv2;
			if (dh1 >= dh2) return;
			dr1 = (r1-r2)*inv_Dv1;
			dg1 = (g1-g2)*inv_Dv1;
			db1 = (b1-b2)*inv_Dv1;
		scale = ((float)inv_Dv1)*(1.0/FLOAT_EXPO);
		dx1 = (x1-x2)*scale;
		dy1 = (y1-y2)*scale;
		dz1 = (z1-z2)*scale;
			Dh3 = Dh1-Dh2;
			Dv3 = Dv1-Dv2;
			inv_Dv3 = invert[Dv3];
			dh3 = Dh3*inv_Dv3;
			r0b = ((dr1*Dv2+(1<<(EXPO-1)))>>EXPO)+r2;
			g0b = ((dg1*Dv2+(1<<(EXPO-1)))>>EXPO)+g2;
			b0b = ((db1*Dv2+(1<<(EXPO-1)))>>EXPO)+b2;
		scale = (float)Dv2;
		x0b = dx1*scale+x2;
		y0b = dy1*scale+y2;
		z0b = dz1*scale+z2;
			Dh0 = Dh2-((dh1*Dv2-(1<<(EXPO-1)))>>EXPO);
			if (Dh0 == 0) Dh0 = 1;
			inv_Dh0 = invert[Dh0];
			dr2 = r0b-r3;
			dg2 = g0b-g3;
			db2 = b0b-b3;
			dr2 *= inv_Dh0;
			dg2 *= inv_Dh0;
			db2 *= inv_Dh0;
		scale = ((float)inv_Dh0)*(1.0/FLOAT_EXPO);
		dx2 = (x0b-x3)*scale;
		dy2 = (y0b-y3)*scale;
		dz2 = (z0b-z3)*scale;
			// debut de ligne haute
			draw = (uchar*)((long)bits+row*v2-1);
			H2 = (h2<<EXPO)|(1<<(EXPO-1));
			H1 = H2+dh1;
			H2 += dh2;
			Dv2 = Dv1-Dv2;
			Dv1--;
			// couleur du pointeur de trace
			curR0 = (r2<<EXPO)|(1<<(EXPO-1));
			curG0 = (g2<<EXPO)|(1<<(EXPO-1));
			curB0 = (b2<<EXPO)|(1<<(EXPO-1));
			curR0 += dr1;
			curG0 += dg1;
			curB0 += db1;
		curX0 = x2+dx1;
		curY0 = y2+dy1;
		curZ0 = z2+dz1;
end2:		// mapping step
		dx2 *= PROJECT_STEP_FACT;
		dy2 *= PROJECT_STEP_FACT;
		dz2 *= PROJECT_STEP_FACT;
			// traceur
			do {
				if (Dv1 == Dv2) {
					dh2 = dh3;
					H2 &= ((1<<31)-1)<<EXPO;
					H2 |= (1<<(EXPO-1));
				}
				H1b = (H1>>(EXPO))&((1<<(32-EXPO))-1);
				H2b = (H2>>(EXPO))&((1<<(32-EXPO))-1);
				Dv1--;
				H1 += dh1;
				H2 += dh2;
				Size = H2b-H1b;
				draw = (uchar*)((long)draw+row);
				if (Size > 0) {
				curX = curX0;
				curY = curY0;
				curZ = curZ0;
				scale = FACTOR/curZ;
					Count = 0;
					curR = curR0;
					curG = curG0;
					curB = curB0;
					draw2 = (uchar*)((long)draw+H1b);
				H0 = (long)(curX*scale);
				V0 = (long)(curY*scale);

					do {
						if ((Count & PROJECT_STEP_MASK) == 0) {
							curZ -= dz2;
							scale = FACTOR/curZ;
							curX -= dx2;
							curY -= dy2;
							H = H0;
							V = V0;
							H0 = (long)(curX*scale);
							V0 = (long)(curY*scale);
							dH = (H0-H)>>PROJECT_STEP_EXP;
							dV = (V0-V)>>PROJECT_STEP_EXP;
						}
						offset = (H>>(EXPOFAC-2))&Hmask;
						offset |= ((V>>(EXPOFAC-2))<<Hsize)&Vmask;
						col = *((ulong*)((long)map+offset));
						Count += 1;
						r1 = ((col>>ROTR)&0xff);
						g1 = ((col>>ROTG)&0xff);
						b1 = ((col>>ROTB)&0xff);
						r0 += r1*(curR>>EXPO);
						if (r0 > 0xffff)
							r0 = 0xffff;
						if (r0 < 0)
							r0 = 0;
						index = (r0>>1)&0x7c00;
						g0 += g1*(curG>>EXPO);
						if (g0 > 0xffff)
							g0 = 0xffff;
						if (g0 < 0)
							g0 = 0;
						index |= (g0>>6)&0x03e0;
						b0 += b1*(curB>>EXPO);
						if (b0 > 0xffff)
							b0 = 0xffff;
						if (b0 < 0)
							b0 = 0;
						index |= b0>>11;
						plot = index_map[index];
						H += dH;
						V += dV;
						col = color_list[plot];
						*++draw2 = plot;

						curR -= dr2;
						curG -= dg2;
						curB -= db2;
#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
                        r0 -= (col<<8)&0xff00;
                        g0 -= (col)&0xff00;
                        b0 -= (col>>8)&0xff00;
#else
						r0 -= (col>>16)&0xff00;
						g0 -= (col>>8)&0xff00;
						b0 -= (col)&0xff00;
#endif
					} while (Count < Size);
					
				}
				curR0 += dr1;
				curG0 += dg1;
				curB0 += db1;
			curX0 += dx1;
			curY0 += dy1;
			curZ0 += dz1;
			} while (Dv1 > 0);			
		}
	}
	else if (Dv1 > 0) {
		// cas 1 > 0 = 2
		inv_Dv1 = invert[Dv1];
		inv_Dv2 = invert[Dv2];
		dh1 = Dh1*inv_Dv1;
		dh2 = Dh2*inv_Dv2;
		if (dh1 >= dh2) return;
		dr1 = (r3-r2)*inv_Dv2;
		dg1 = (g3-g2)*inv_Dv2;
		db1 = (b3-b2)*inv_Dv2;
	scale = ((float)inv_Dv2)*(1.0/FLOAT_EXPO);
	dx1 = (x3-x2)*scale;
	dy1 = (y3-y2)*scale;
	dz1 = (z3-z2)*scale;
		Dh3 = Dh2-Dh1;
		if (Dh3 == 0) Dh3 = 1;
		inv_Dh0 = invert[Dh3];
		dr2 = (r3-r1)*inv_Dh0;
		dg2 = (g3-g1)*inv_Dh0;
		db2 = (b3-b1)*inv_Dh0;
	scale = ((float)inv_Dh0)*(1.0/FLOAT_EXPO);
	dx2 = (x3-x1)*scale;
	dy2 = (y3-y1)*scale;
	dz2 = (z3-z1)*scale;
		goto end0;
	}
}














