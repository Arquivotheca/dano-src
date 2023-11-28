#include "context.h"
#include "global.h"
#include "rasRaster.h"
#include "rasTriangle.h"
#include "rasScanline.h"
#include "rasBuffers.h"
#include "fog.h"
#include "mathLib.h"

/**********************************************************************************/
/***                                                                            ***/
/***                       Move this to the driver code!                        ***/
/***                                                                            ***/
/**********************************************************************************/

#define COLOR_SCALE_CONST (65536)
#define TEXTURE_SCALE_CONST (65536)

static GLfloat consts[] = { 0.5, 1<<16 };


GLfloat rasGetPolyOffset( __glContext *gc, GLuint A, GLuint B, GLuint c )
{
	GLfloat v1[4],v2[4],res[4];
	GLfloat dzdx, dzdy, max;

	v1[0] = gc->vertices.WindowX[A] - gc->vertices.WindowX[B];
	v1[1] = gc->vertices.WindowY[A] - gc->vertices.WindowY[B];
	v1[2] = gc->vertices.WindowZ[A] - gc->vertices.WindowZ[B];
	v2[0] = gc->vertices.WindowX[c] - gc->vertices.WindowX[B];
	v2[1] = gc->vertices.WindowY[c] - gc->vertices.WindowY[B];
	v2[2] = gc->vertices.WindowZ[c] - gc->vertices.WindowZ[B];
	mathVectorCross3( v1, v2, res );

	dzdx = fabs( res[0] / (res[2]+0.0000001) );
	dzdy = fabs( res[1] / (res[2]+0.0000001) );
	
	if( dzdx > dzdy )
		max = dzdx;
	else
		max = dzdy;

//printf( "\nmax1 %f \n", max );
	max *= 1.0 / ((double)(1 << 11));
//printf( "max2 %f \n", max );
	max *= gc->state.poly.OffsetFactor;
//printf( "max3 %f \n", max );
	max += gc->state.poly.OffsetR * gc->state.poly.OffsetUnit;
//printf( "max4 %f \n", max );
	return max;
}


extern void scanline_init( __glContext *gc, const __glFragment *start, __glShade *shade, GLint w );

#if 1
void __glFillSubTriangle ( __glContext *gc )
{
	GLint yBottom, yTop, ixLeft, ixRight;
	GLint spanWidth;
	__glFixedCoord xLeft, xRight;
	__glFixedCoord dxdyLeft, dxdyRight;
	GLfloat R=0.f, G=0.f, B=0.f, A=0.f, s, t, qw, f;
	GLfloat z;
	__glFragment frag;

	xLeft = gc->softScanProcs.shade.xLeftFixed;
	xRight = gc->softScanProcs.shade.xRightFixed;

#if __PROCESSOR_P5__
	__asm__ __volatile__ (
	"flds (%%eax) \n\t"
	"fmuls 4(%%ecx) \n\t"
	"fistpl (%%edx) \n\t"
	::"A"(&gc->softScanProcs.shade.dxdyLeft),"d"(&dxdyLeft),"c"(&consts[0]) : "memory" );
	__asm__ __volatile__ (
	"flds (%%eax) \n\t"
	"fmuls 4(%%ecx) \n\t"
	"fistpl (%%edx) \n\t"
	::"A"(&gc->softScanProcs.shade.dxdyRight),"d"(&dxdyRight),"c"(&consts[0]) : "memory" );
#else
	dxdyLeft = __GL_COORD_SIGNED_FLOAT_TO_FIXED (gc->softScanProcs.shade.dxdyLeft);
	dxdyRight = __GL_COORD_SIGNED_FLOAT_TO_FIXED (gc->softScanProcs.shade.dxdyRight);
#endif
	
	if (gc->state.light.ShadingModel == GL_SMOOTH)
	{
		frag.color.R = R = gc->softScanProcs.shade.r0;
		frag.color.G = G = gc->softScanProcs.shade.g0;
		frag.color.B = B = gc->softScanProcs.shade.b0;
		frag.color.A = A = gc->softScanProcs.shade.a0;
	}
	else
	{
		frag.color.R = gc->softScanProcs.shade.r0;
		frag.color.G = gc->softScanProcs.shade.g0;
		frag.color.B = gc->softScanProcs.shade.b0;
		frag.color.A = gc->softScanProcs.shade.a0;
	}
	z = gc->softScanProcs.shade.z0;
	s = gc->softScanProcs.shade.s0;
	t = gc->softScanProcs.shade.t0;
	qw = gc->softScanProcs.shade.qw0;
	f = gc->softScanProcs.shade.f0;

	yBottom = gc->softScanProcs.shade.iyBottom;
	yTop = gc->softScanProcs.shade.iyTop;
	while (yBottom < yTop)
	{
		ixLeft = (GLint) __GL_COORD_FIXED_TO_INT (xLeft);
		ixRight = (GLint) __GL_COORD_FIXED_TO_INT (xRight);
		spanWidth = ixRight - ixLeft;
		if (spanWidth > 0)
		{
			__glFixedCoord fixeddx;
			GLfloat dx;

			/*
			   ** Figure out how far the left edge of the triangle is from
			   ** the first pixel center in the triangle.  The first part
			   ** of this calculation is carefully done in fixed point:
			   **
			   **   fixeddx = ixLeft + 0.5 - (xLeft - 0.499999...)
			   **
			   ** The addition of one half is to move the ixLeft to the
			   ** pixel center.  The subtraction of almost one half is
			   ** to remove the pre-adjustment done above when xLeft
			   ** was originaly computed from its floating point counterpart.
			 */
			fixeddx = __GL_COORD_INT_TO_FIXED (ixLeft);
			__GL_COORD_SUB (fixeddx, fixeddx, xLeft);
			__GL_COORD_ADD (fixeddx, fixeddx, __GL_COORD_ALMOST_ONE);
//          assert(__GL_COORD_FIXED_TO_INT(fixeddx) == 0);

			/*
			   ** fixeddx contains the delta in fixed point.  Carefully
			   ** convert this to floating point for the parameter
			   ** calculations.  This can be easily done because the
			   ** maximum distance is just less than one, so we can
			   ** assert that the fixeddx integer value
			 */
			dx = __GL_COORD_FIXED_TO_FLOAT (fixeddx);

			/*
			   ** Setup initial fragment state from position on the left
			   ** edge of the triangle.  Move coordinate and parameter
			   ** values to the pixel center by computing the distance
			   ** between the left edge and the pixel center, and then
			   ** multiplying by the change in the parameter in X.
			 */
			frag.x = ixLeft;
			frag.y = yBottom;
			if (gc->state.light.ShadingModel == GL_SMOOTH)
			{
				frag.color.R = R + dx * gc->softScanProcs.shade.drdx;
				frag.color.G = G + dx * gc->softScanProcs.shade.dgdx;
				frag.color.B = B + dx * gc->softScanProcs.shade.dbdx;
				frag.color.A = A + dx * gc->softScanProcs.shade.dadx;
			}
			frag.z = z + dx * gc->softScanProcs.shade.dzdx;
			if (gc->texture.Enabled[0] || gc->texture.Enabled[1])
			{
				frag.qw = qw + dx * gc->softScanProcs.shade.dqwdx;
				frag.s = s + dx * gc->softScanProcs.shade.dsdx;
				frag.t = t + dx * gc->softScanProcs.shade.dtdx;
			}
			if (gc->state.fog.Enabled)
			{
				frag.f = f + dx * gc->softScanProcs.shade.dfdx;
				gc->softScanProcs.shade.f0 = f + dx * gc->softScanProcs.shade.dfdx;
			}


			/*
			   ** draw A scanline here
			 */
			if ((frag.y >= gc->state.transformClipY0) && (frag.y < gc->state.transformClipY1))
			{
				gc->buffer.current->ScanlineY = frag.y;
				rasProcessScanline (gc, &frag, spanWidth);
				//(*gc->softScanProcs.processorFunc) (gc, &frag, spanWidth);
			}
		}

		/*
		   ** Advance edge walking parameters
		 */
		__GL_COORD_ADD (xLeft, xLeft, dxdyLeft);
		__GL_COORD_ADD (xRight, xRight, dxdyRight);
		if (gc->state.light.ShadingModel == GL_SMOOTH)
		{
			R += gc->softScanProcs.shade.drdxdy;
			G += gc->softScanProcs.shade.dgdxdy;
			B += gc->softScanProcs.shade.dbdxdy;
			A += gc->softScanProcs.shade.dadxdy;
		}
		z += gc->softScanProcs.shade.dzdxdy;
		if (gc->texture.Enabled[0] || gc->texture.Enabled[1])
		{
			s += gc->softScanProcs.shade.dsdxdy;
			t += gc->softScanProcs.shade.dtdxdy;
			qw += gc->softScanProcs.shade.dqwdxdy;
		}
		if (gc->state.fog.Enabled)
		{
			f += gc->softScanProcs.shade.dfdxdy;
		}
		yBottom++;
	}

	gc->softScanProcs.shade.xLeftFixed = xLeft;
	gc->softScanProcs.shade.xRightFixed = xRight;

	if (gc->state.light.ShadingModel == GL_SMOOTH)
	{
		gc->softScanProcs.shade.r0 = R;
		gc->softScanProcs.shade.g0 = G;
		gc->softScanProcs.shade.b0 = B;
		gc->softScanProcs.shade.a0 = A;
	}
	gc->softScanProcs.shade.z0 = z;
	gc->softScanProcs.shade.s0 = s;
	gc->softScanProcs.shade.t0 = t;
	gc->softScanProcs.shade.qw0 = qw;
	gc->softScanProcs.shade.f0 = f;
}
#endif

/*
   ** Take y coordinates and convert them to integer form, taking into
   ** account the rules that determine which pixel centers are in and which
   ** are not by using the "ALMOST_HALF" constant.
 */
static GLboolean SnapYToPixelCenter (__glContext *gc)
{
#if __PROCESSOR_P5__
	__asm__ __volatile__ (
	"flds (%%eax) \n\t"
	"fistpl (%%edx) \n\t"
	::"A"(&gc->softScanProcs.shade.yBottom),"d"(&gc->softScanProcs.shade.iyBottom) );
	__asm__ __volatile__ (
	"flds (%%eax) \n\t"
	"fistpl (%%edx) \n\t"
	::"A"(&gc->softScanProcs.shade.yTop),"d"(&gc->softScanProcs.shade.iyTop) );
#else
	__glFixedCoord temp;

	temp = __GL_COORD_FLOAT_TO_FIXED (gc->softScanProcs.shade.yBottom);
	__GL_COORD_ADD (temp, temp, __GL_COORD_ALMOST_HALF);
	gc->softScanProcs.shade.iyBottom = (GLint) __GL_COORD_FIXED_TO_INT (temp);

	temp = __GL_COORD_FLOAT_TO_FIXED (gc->softScanProcs.shade.yTop);
	__GL_COORD_ADD (temp, temp, __GL_COORD_ALMOST_HALF);
	gc->softScanProcs.shade.iyTop = (GLint) __GL_COORD_FIXED_TO_INT (temp);
#endif

	/*
	   ** Compute how far we moved in y.  This value is needed so that the
	   ** other parameters can be adjusted by this motion.
	 */
	gc->softScanProcs.shade.dy = (gc->softScanProcs.shade.iyBottom + __glHalf) - gc->softScanProcs.shade.yBottom;
//    assert(gc->softScanProcs.shade.dy < __glOne);

	return gc->softScanProcs.shade.iyBottom == gc->softScanProcs.shade.iyTop;
}

static void SetInitialParameters ( __glContext *gc, GLuint a, const __glColor * ac, GLfloat aFog)
{
	GLfloat dy = gc->softScanProcs.shade.dy;
	GLfloat dxdyLeft = gc->softScanProcs.shade.dxdyLeft;

	if (gc->state.light.ShadingModel == GL_SMOOTH)
	{
		gc->softScanProcs.shade.drdxdy = gc->softScanProcs.shade.drdy + dxdyLeft * gc->softScanProcs.shade.drdx;
		gc->softScanProcs.shade.dgdxdy = gc->softScanProcs.shade.dgdy + dxdyLeft * gc->softScanProcs.shade.dgdx;
		gc->softScanProcs.shade.dbdxdy = gc->softScanProcs.shade.dbdy + dxdyLeft * gc->softScanProcs.shade.dbdx;
		gc->softScanProcs.shade.dadxdy = gc->softScanProcs.shade.dady + dxdyLeft * gc->softScanProcs.shade.dadx;
		gc->softScanProcs.shade.r0 = ac->R * COLOR_SCALE_CONST + dy * gc->softScanProcs.shade.drdxdy;
		gc->softScanProcs.shade.g0 = ac->G * COLOR_SCALE_CONST + dy * gc->softScanProcs.shade.dgdxdy;
		gc->softScanProcs.shade.b0 = ac->B * COLOR_SCALE_CONST + dy * gc->softScanProcs.shade.dbdxdy;
		gc->softScanProcs.shade.a0 = ac->A * COLOR_SCALE_CONST + dy * gc->softScanProcs.shade.dadxdy;
	}

	gc->softScanProcs.shade.dzdxdy = gc->softScanProcs.shade.dzdy + dxdyLeft * gc->softScanProcs.shade.dzdx;
	gc->softScanProcs.shade.z0 = gc->vertices.WindowZ[a] + dy * gc->softScanProcs.shade.dzdxdy;

	if (gc->texture.Enabled[0] || gc->texture.Enabled[1])
	{
		GLfloat oneOverW = gc->vertices.WindowW[a];
		gc->softScanProcs.shade.dsdxdy = gc->softScanProcs.shade.dsdy + dxdyLeft * gc->softScanProcs.shade.dsdx;
		gc->softScanProcs.shade.dtdxdy = gc->softScanProcs.shade.dtdy + dxdyLeft * gc->softScanProcs.shade.dtdx;
		gc->softScanProcs.shade.dqwdxdy = gc->softScanProcs.shade.dqwdy + dxdyLeft * gc->softScanProcs.shade.dqwdx;
		gc->softScanProcs.shade.s0 = gc->vertices.TextureX[a] * TEXTURE_SCALE_CONST * oneOverW + dy * gc->softScanProcs.shade.dsdxdy;
		gc->softScanProcs.shade.t0 = gc->vertices.TextureY[a] * TEXTURE_SCALE_CONST * oneOverW + dy * gc->softScanProcs.shade.dtdxdy;
		gc->softScanProcs.shade.qw0 = gc->vertices.TextureW[a] * TEXTURE_SCALE_CONST * oneOverW + dy * gc->softScanProcs.shade.dqwdxdy;
	}

	if (gc->state.fog.Enabled)
	{
		gc->softScanProcs.shade.dfdxdy = gc->softScanProcs.shade.dfdy + dxdyLeft * gc->softScanProcs.shade.dfdx;
		gc->softScanProcs.shade.f0 = aFog + dy * gc->softScanProcs.shade.dfdxdy;
	}
}

void rasDrawTriangleLine( __glContext *gc, GLuint A, GLuint B, GLuint c, GLboolean side )
{
	GLfloat az=0.f,bz=0.f,cz=0.f;

	if( gc->state.poly.OffsetLineEnabled )
	{
		GLfloat offset = rasGetPolyOffset( gc, A, B, c );
		az = gc->vertices.WindowZ[A];
		bz = gc->vertices.WindowZ[B];
		cz = gc->vertices.WindowZ[c];
		gc->vertices.WindowZ[A] += offset;
		gc->vertices.WindowZ[B] += offset;
		gc->vertices.WindowZ[c] += offset;
	}
	
	if (gc->vertices.Edge[A])
		(*gc->procs.lineUnordered)(gc, A, B);
	if (gc->vertices.Edge[B])
		(*gc->procs.lineUnordered)(gc, B, c);
	if (gc->vertices.Edge[c])
		(*gc->procs.lineUnordered)(gc, c, A);

	if( gc->state.poly.OffsetLineEnabled )
	{
		gc->vertices.WindowZ[A] = az;
		gc->vertices.WindowZ[B] = bz;
		gc->vertices.WindowZ[c] = cz;
	}
}

void rasDrawTrianglePoint( __glContext *gc, GLuint A, GLuint B, GLuint c, GLboolean side )
{
	GLfloat az=0.f,bz=0.f,cz=0.f;

	if( gc->state.poly.OffsetPointEnabled )
	{
		GLfloat offset = rasGetPolyOffset( gc, A, B, c );
		az = gc->vertices.WindowZ[A];
		bz = gc->vertices.WindowZ[B];
		cz = gc->vertices.WindowZ[c];
		gc->vertices.WindowZ[A] += offset;
		gc->vertices.WindowZ[B] += offset;
		gc->vertices.WindowZ[c] += offset;
	}

	if (gc->vertices.Edge[A])
		(*gc->procs.pointUnordered)(gc, A);
	if (gc->vertices.Edge[B])
		(*gc->procs.pointUnordered)(gc, B);
	if (gc->vertices.Edge[c])
		(*gc->procs.pointUnordered)(gc, c);

	if( gc->state.poly.OffsetLineEnabled )
	{
		gc->vertices.WindowZ[A] = az;
		gc->vertices.WindowZ[B] = bz;
		gc->vertices.WindowZ[c] = cz;
	}
}

extern void validateSoftScanProcs( __glContext * );

void rasDrawTriangleFill (__glContext *gc, GLuint a, GLuint b, GLuint c, GLboolean side)
{
	GLfloat oneOverArea, t1, t2, t3, t4, px;
	GLfloat dxAB, dxAC, dxBC, dyAB, dyBC, dyAC;
	GLfloat dzAC, dzBC;
	GLfloat dxdyAB=0.f, dxdyBC=0.f, dxdyAC=0.f;
	GLfloat aFog=0.f, bFog=0.f, cFog;
	__glColor ac, bc, cc;
	GLboolean firstSubFilled, abHorizontal, bcHorizontal;
	GLfloat az=0.0,bz=0.0,cz=0.0;

	if( !gc->softScanProcs.valid )
		validateSoftScanProcs( gc );

#if 0	
printf( "ABC= %i %i %i \n", a, b, c );
printf( "A = %f %f, %f %f %f \n", gc->vertices.WindowX[a], gc->vertices.WindowY[a],
	gc->vertices.FrontColorR[a], gc->vertices.FrontColorG[a], gc->vertices.FrontColorB[a] );
printf( "B = %f %f, %f %f %f \n", gc->vertices.WindowX[b], gc->vertices.WindowY[b],
	gc->vertices.FrontColorR[b], gc->vertices.FrontColorG[b], gc->vertices.FrontColorB[b] );
printf( "C = %f %f, %f %f %f \n", gc->vertices.WindowX[c], gc->vertices.WindowY[c],
	gc->vertices.FrontColorR[c], gc->vertices.FrontColorG[c], gc->vertices.FrontColorB[c] );
#endif

	/*
	   ** Sort verticies in y.  Keep track if A reversal of the
	   ** winding occurs.  Adjust the boundary edge flags as
	   ** well.
	 */
	if ( gc->vertices.WindowY[a] > gc->vertices.WindowY[b] )
	{
		GLuint temp;
		temp = a;
		a = b;
		b = temp;
	}
	if ( gc->vertices.WindowY[b] > gc->vertices.WindowY[c] )
	{
		GLuint temp;
		temp = b;
		b = c;
		c = temp;
		if ( gc->vertices.WindowY[a] > gc->vertices.WindowY[b] )
		{
			temp = a;
			a = b;
			b = temp;
		}
	}

	if( gc->state.poly.OffsetFillEnabled )
	{
		GLfloat offset = rasGetPolyOffset( gc, a, b, c );
		az = gc->vertices.WindowZ[a];
		bz = gc->vertices.WindowZ[b];
		cz = gc->vertices.WindowZ[c];
		gc->vertices.WindowZ[a] += offset;
		gc->vertices.WindowZ[b] += offset;
		gc->vertices.WindowZ[c] += offset;
	}

	memset (&gc->softScanProcs.shade, 0, sizeof (__glShade));
	gc->softScanProcs.shade.dxAC = gc->vertices.WindowX[a] - gc->vertices.WindowX[c];
	gc->softScanProcs.shade.dxBC = gc->vertices.WindowX[b] - gc->vertices.WindowX[c];
	gc->softScanProcs.shade.dyAC = gc->vertices.WindowY[a] - gc->vertices.WindowY[c];
	gc->softScanProcs.shade.dyBC = gc->vertices.WindowY[b] - gc->vertices.WindowY[c];
	gc->softScanProcs.shade.area = gc->softScanProcs.shade.dxAC * gc->softScanProcs.shade.dyBC - gc->softScanProcs.shade.dxBC * gc->softScanProcs.shade.dyAC;
	gc->softScanProcs.shade.ccw = (gc->softScanProcs.shade.area > 0.0);

	/* Toss out zero area triangles */
	if (gc->softScanProcs.shade.area == 0)
	{
		if( gc->state.poly.OffsetFillEnabled )
		{
			gc->vertices.WindowZ[a] = az;
			gc->vertices.WindowZ[b] = bz;
			gc->vertices.WindowZ[c] = cz;
		}
		return;
	}

	dxAC = gc->softScanProcs.shade.dxAC;
	dxBC = gc->softScanProcs.shade.dxBC;
	dyAC = gc->softScanProcs.shade.dyAC;
	dyBC = gc->softScanProcs.shade.dyBC;

	/*
	   ** dx/dy values are needed for each of the three edges of the
	   ** triangle so that X can be recomputed after each unit step of y.
	   ** Horizontal edges will have A dx/dy that is "large".
	 */
	dxAB = gc->vertices.WindowX[a] - gc->vertices.WindowX[b];
	dyAB = gc->vertices.WindowY[a] - gc->vertices.WindowY[b];
	abHorizontal = GL_TRUE;
	bcHorizontal = GL_TRUE;
	if (dyAB)
	{
		dxdyAB = dxAB / dyAB;
		abHorizontal = GL_FALSE;
	}
	if (dyBC)
	{
		dxdyBC = dxBC / dyBC;
		bcHorizontal = GL_FALSE;
	}
	dxdyAC = dxAC / dyAC;

	/*
	   ** To scan convert the triangle, this code will make unit steps in
	   ** y.  For each scan line, the left and right edges will be computed
	   ** iteratively in fixed point.  To iterate the parameters across A
	   ** span, dpdx values are needed (where p represents A parameter
	   ** being iterated; e.G. red).  These values are found by computing
	   ** the derivative of the parameter with respect to X.
	   **
	   ** To step the parameter values in Y...
	 */
	oneOverArea = __glOne / gc->softScanProcs.shade.area;
	t1 = dyAC * oneOverArea;
	t2 = dyBC * oneOverArea;
	t3 = dxAC * oneOverArea;
	t4 = dxBC * oneOverArea;
	
	if ( gc->state.light.ShadingModel == GL_SMOOTH )
	{
		GLfloat drAC, dgAC, dbAC, daAC;
		GLfloat drBC, dgBC, dbBC, daBC;

		if( side )
		{
			ac.R = gc->vertices.BackColorR[a];
			ac.G = gc->vertices.BackColorG[a];
			ac.B = gc->vertices.BackColorB[a];
			ac.A = gc->vertices.BackColorA[a];
			bc.R = gc->vertices.BackColorR[b];
			bc.G = gc->vertices.BackColorG[b];
			bc.B = gc->vertices.BackColorB[b];
			bc.A = gc->vertices.BackColorA[b];
			cc.R = gc->vertices.BackColorR[c];
			cc.G = gc->vertices.BackColorG[c];
			cc.B = gc->vertices.BackColorB[c];
			cc.A = gc->vertices.BackColorA[c];
		}
		else
		{
			ac.R = gc->vertices.FrontColorR[a];
			ac.G = gc->vertices.FrontColorG[a];
			ac.B = gc->vertices.FrontColorB[a];
			ac.A = gc->vertices.FrontColorA[a];
			bc.R = gc->vertices.FrontColorR[b];
			bc.G = gc->vertices.FrontColorG[b];
			bc.B = gc->vertices.FrontColorB[b];
			bc.A = gc->vertices.FrontColorA[b];
			cc.R = gc->vertices.FrontColorR[c];
			cc.G = gc->vertices.FrontColorG[c];
			cc.B = gc->vertices.FrontColorB[c];
			cc.A = gc->vertices.FrontColorA[c];
		}
	
		drAC = ac.R - cc.R;
		drBC = bc.R - cc.R;
		gc->softScanProcs.shade.drdx = (drAC * t2 - drBC * t1) * COLOR_SCALE_CONST;
		gc->softScanProcs.shade.drdy = (drBC * t3 - drAC * t4) * COLOR_SCALE_CONST;
		dgAC = ac.G - cc.G;
		dgBC = bc.G - cc.G;
		gc->softScanProcs.shade.dgdx = (dgAC * t2 - dgBC * t1) * COLOR_SCALE_CONST;
		gc->softScanProcs.shade.dgdy = (dgBC * t3 - dgAC * t4) * COLOR_SCALE_CONST;
		dbAC = ac.B - cc.B;
		dbBC = bc.B - cc.B;
		gc->softScanProcs.shade.dbdx = (dbAC * t2 - dbBC * t1) * COLOR_SCALE_CONST;
		gc->softScanProcs.shade.dbdy = (dbBC * t3 - dbAC * t4) * COLOR_SCALE_CONST;
		daAC = ac.A - cc.A;
		daBC = bc.A - cc.A;
		gc->softScanProcs.shade.dadx = (daAC * t2 - daBC * t1) * COLOR_SCALE_CONST;
		gc->softScanProcs.shade.dady = (daBC * t3 - daAC * t4) * COLOR_SCALE_CONST;
	}
	else
	{
		GLint p = gc->primitive.Provoking;
		if( side )
		{
			ac.R = gc->vertices.BackColorR[p];
			ac.G = gc->vertices.BackColorG[p];
			ac.B = gc->vertices.BackColorB[p];
			ac.A = gc->vertices.BackColorA[p];
			bc.R = gc->vertices.BackColorR[p];
			bc.G = gc->vertices.BackColorG[p];
			bc.B = gc->vertices.BackColorB[p];
			bc.A = gc->vertices.BackColorA[p];
			cc.R = gc->vertices.BackColorR[p];
			cc.G = gc->vertices.BackColorG[p];
			cc.B = gc->vertices.BackColorB[p];
			cc.A = gc->vertices.BackColorA[p];
		}
		else
		{
			ac.R = gc->vertices.FrontColorR[p];
			ac.G = gc->vertices.FrontColorG[p];
			ac.B = gc->vertices.FrontColorB[p];
			ac.A = gc->vertices.FrontColorA[p];
			bc.R = gc->vertices.FrontColorR[p];
			bc.G = gc->vertices.FrontColorG[p];
			bc.B = gc->vertices.FrontColorB[p];
			bc.A = gc->vertices.FrontColorA[p];
			cc.R = gc->vertices.FrontColorR[p];
			cc.G = gc->vertices.FrontColorG[p];
			cc.B = gc->vertices.FrontColorB[p];
			cc.A = gc->vertices.FrontColorA[p];
		}

		gc->softScanProcs.shade.r0 = ac.R * COLOR_SCALE_CONST;
		gc->softScanProcs.shade.g0 = ac.G * COLOR_SCALE_CONST;
		gc->softScanProcs.shade.b0 = ac.B * COLOR_SCALE_CONST;
		gc->softScanProcs.shade.a0 = ac.A * COLOR_SCALE_CONST;
	}

	if( gc->state.depth.TestEnabled )
	{
		dzAC = gc->vertices.WindowZ[a] - gc->vertices.WindowZ[c];
		dzBC = gc->vertices.WindowZ[b] - gc->vertices.WindowZ[c];
		gc->softScanProcs.shade.dzdx = dzAC * t2 - dzBC * t1;
		gc->softScanProcs.shade.dzdy = dzBC * t3 - dzAC * t4;
	}

	if (gc->texture.Enabled[0] || gc->texture.Enabled[1] )
	{
		GLfloat awinv, bwinv, cwinv, scwinv, tcwinv, qwcwinv;
		GLfloat dsAC, dsBC, dtAC, dtBC, dqwAC, dqwBC;

		awinv = gc->vertices.WindowW[a];
		bwinv = gc->vertices.WindowW[b];
		cwinv = gc->vertices.WindowW[c];
		scwinv = gc->vertices.TextureX[c] * TEXTURE_SCALE_CONST * cwinv;
		tcwinv = gc->vertices.TextureY[c] * TEXTURE_SCALE_CONST * cwinv;
		qwcwinv = gc->vertices.TextureW[c] * TEXTURE_SCALE_CONST * cwinv;

		dsAC = gc->vertices.TextureX[a] * TEXTURE_SCALE_CONST * awinv - scwinv;
		dsBC = gc->vertices.TextureX[b] * TEXTURE_SCALE_CONST * bwinv - scwinv;
		gc->softScanProcs.shade.dsdx = dsAC * t2 - dsBC * t1;
		gc->softScanProcs.shade.dsdy = dsBC * t3 - dsAC * t4;
		dtAC = gc->vertices.TextureY[a] * TEXTURE_SCALE_CONST * awinv - tcwinv;
		dtBC = gc->vertices.TextureY[b] * TEXTURE_SCALE_CONST * bwinv - tcwinv;
		gc->softScanProcs.shade.dtdx = dtAC * t2 - dtBC * t1;
		gc->softScanProcs.shade.dtdy = dtBC * t3 - dtAC * t4;
		dqwAC = gc->vertices.TextureW[a] * TEXTURE_SCALE_CONST * awinv - qwcwinv;
		dqwBC = gc->vertices.TextureW[b] * TEXTURE_SCALE_CONST * bwinv - qwcwinv;
		gc->softScanProcs.shade.dqwdx = dqwAC * t2 - dqwBC * t1;
		gc->softScanProcs.shade.dqwdy = dqwBC * t3 - dqwAC * t4;
	}

	if (gc->state.fog.Enabled)
	{
		GLfloat dfAC, dfBC;
/*
printf( "Obj a  %f %f %f \n", gc->vertices.ObjX[a], gc->vertices.ObjY[a], gc->vertices.ObjZ[a] );
printf( "Obj b  %f %f %f \n", gc->vertices.ObjX[b], gc->vertices.ObjY[b], gc->vertices.ObjZ[b] );
printf( "Obj c  %f %f %f \n", gc->vertices.ObjX[c], gc->vertices.ObjY[c], gc->vertices.ObjZ[c] );

mathPrintMatrix( "Model", gc->transform.modelView->matrix.matrix );
mathPrintMatrixSIMD( "Model", (float *)&gc->model );

printf( "Eye a  %f %f %f \n", gc->vertices.EyeX[a], gc->vertices.EyeY[a], gc->vertices.EyeZ[a] );
printf( "Eye b  %f %f %f \n", gc->vertices.EyeX[b], gc->vertices.EyeY[b], gc->vertices.EyeZ[b] );
printf( "Eye c  %f %f %f \n", gc->vertices.EyeX[c], gc->vertices.EyeY[c], gc->vertices.EyeZ[c] );
*/
		if (gc->state.hint.Fog == GL_NICEST)
		{
			/* Use eyeZ for interpolation value */
			aFog = gc->vertices.EyeZ[a];
			bFog = gc->vertices.EyeZ[b];
			cFog = gc->vertices.EyeZ[c];
		}
		else
		{
			/* Use fog(eyeZ) for interpolation value */
			aFog = calcFog (gc, gc->vertices.EyeZ[a] );
			bFog = calcFog (gc, gc->vertices.EyeZ[b] );
			cFog = calcFog (gc, gc->vertices.EyeZ[c] );
		}
		dfAC = aFog - cFog;
		dfBC = bFog - cFog;
		gc->softScanProcs.shade.dfdx = dfAC * t2 - dfBC * t1;
		gc->softScanProcs.shade.dfdy = dfBC * t3 - dfAC * t4;
	}

	/* Fill first sub triangle */
	DRIVERPROC_BUFFER_LOCK( gc );
	firstSubFilled = GL_FALSE;
	if (!abHorizontal)
	{
		gc->softScanProcs.shade.xLeft = gc->vertices.WindowX[a];
		gc->softScanProcs.shade.xRight = gc->vertices.WindowX[a];
		gc->softScanProcs.shade.yBottom = gc->vertices.WindowY[a];
		gc->softScanProcs.shade.yTop = gc->vertices.WindowY[b];
		if (gc->softScanProcs.shade.ccw)
		{
			gc->softScanProcs.shade.dxdyLeft = dxdyAC;
			gc->softScanProcs.shade.dxdyRight = dxdyAB;
		}
		else
		{
			gc->softScanProcs.shade.dxdyLeft = dxdyAB;
			gc->softScanProcs.shade.dxdyRight = dxdyAC;
		}

		if (!SnapYToPixelCenter (gc))
		{
			firstSubFilled = GL_TRUE;

			SetInitialParameters (gc, a, &ac, aFog);
			/*
			   ** Adjust the xLeft and xRight coordinate by the amount of change
			   ** that y's moving to the pixel center caused.
			 */
#if __PROCESSOR_P5__
			{
				float t = gc->softScanProcs.shade.xLeft + gc->softScanProcs.shade.dy * gc->softScanProcs.shade.dxdyLeft;
				__asm__ __volatile__ (
				"flds (%%eax) \n\t"
				"fmuls 4(%%ecx) \n\t"
				"fistpl (%%edx) \n\t"
				::"A"(&t),"d"(&gc->softScanProcs.shade.xLeftFixed),"c"(&consts[0]) : "memory" );

				t = gc->softScanProcs.shade.xRight + gc->softScanProcs.shade.dy * gc->softScanProcs.shade.dxdyRight;
				__asm__ __volatile__ (
				"flds (%%eax) \n\t"
				"fmuls 4(%%ecx) \n\t"
				"fistpl (%%edx) \n\t"
				::"A"(&t),"d"(&gc->softScanProcs.shade.xRightFixed),"c"(&consts[0]) : "memory" );
			}
#else
			gc->softScanProcs.shade.xLeftFixed = __GL_COORD_FLOAT_TO_FIXED (gc->softScanProcs.shade.xLeft + gc->softScanProcs.shade.dy * gc->softScanProcs.shade.dxdyLeft);
			gc->softScanProcs.shade.xRightFixed = __GL_COORD_FLOAT_TO_FIXED (gc->softScanProcs.shade.xRight + gc->softScanProcs.shade.dy * gc->softScanProcs.shade.dxdyRight);
#endif

			/*
			   ** Bias xLeft and xRight fixed point numbers by the ALMOST_HALF
			   ** constant so that when the integer forms are computed A simple
			   ** integerization is all that is needed.  This moves the computation
			   ** out of the scan line loop
			 */
			__GL_COORD_ADD (gc->softScanProcs.shade.xLeftFixed, gc->softScanProcs.shade.xLeftFixed, __GL_COORD_ALMOST_HALF);
			__GL_COORD_ADD (gc->softScanProcs.shade.xRightFixed, gc->softScanProcs.shade.xRightFixed, __GL_COORD_ALMOST_HALF);
			(*gc->softScanProcs.fillSubProc)/*FillSubTriangle*/ (gc);
		}
	}

	/* Fill second sub triangle */
	if (!bcHorizontal)
	{
		px = (dyBC / dyAC) * dxAC + gc->vertices.WindowX[c];
		gc->softScanProcs.shade.yBottom = gc->vertices.WindowY[b];
		gc->softScanProcs.shade.yTop = gc->vertices.WindowY[c];
		if (!SnapYToPixelCenter (gc))
		{
			if (gc->softScanProcs.shade.ccw)
			{
				gc->softScanProcs.shade.xLeft = px;
				gc->softScanProcs.shade.xRight = gc->vertices.WindowX[b];
				gc->softScanProcs.shade.dxdyLeft = dxdyAC;
				gc->softScanProcs.shade.dxdyRight = dxdyBC;
				if (!firstSubFilled)
				{
					GLfloat dySave = gc->softScanProcs.shade.dy;
					/* Interpolate initial parameters from A along ac */
					gc->softScanProcs.shade.dy += gc->vertices.WindowY[b] - gc->vertices.WindowY[a];
					SetInitialParameters (gc, a, &ac, aFog);
					/* Restore dy which is used below to set X values */
					gc->softScanProcs.shade.dy = dySave;
				}
			}
			else
			{
				gc->softScanProcs.shade.xLeft = gc->vertices.WindowX[b];
				gc->softScanProcs.shade.xRight = px;
				gc->softScanProcs.shade.dxdyLeft = dxdyBC;
				gc->softScanProcs.shade.dxdyRight = dxdyAC;
				/* Have to set initial parameters when moving to B & bc */
				SetInitialParameters (gc, b, &bc, bFog);
			}
			/* Set the x values *only* if they haven't been set already */
			if (!gc->softScanProcs.shade.ccw || !firstSubFilled)
			{
#if __PROCESSOR_P5__
				float t = gc->softScanProcs.shade.xLeft + gc->softScanProcs.shade.dy * gc->softScanProcs.shade.dxdyLeft;
				__asm__ __volatile__ (
				"flds (%%eax) \n\t"
				"fmuls 4(%%ecx) \n\t"
				"fistpl (%%edx) \n\t"
				::"A"(&t),"d"(&gc->softScanProcs.shade.xLeftFixed),"c"(&consts[0]) : "memory" );
#else
				gc->softScanProcs.shade.xLeftFixed = __GL_COORD_FLOAT_TO_FIXED (gc->softScanProcs.shade.xLeft + gc->softScanProcs.shade.dy * gc->softScanProcs.shade.dxdyLeft);
#endif
				__GL_COORD_ADD (gc->softScanProcs.shade.xLeftFixed, gc->softScanProcs.shade.xLeftFixed, __GL_COORD_ALMOST_HALF);
			}
			if (gc->softScanProcs.shade.ccw || !firstSubFilled)
			{
#if __PROCESSOR_P5__
				float t = gc->softScanProcs.shade.xRight + gc->softScanProcs.shade.dy * gc->softScanProcs.shade.dxdyRight;
				__asm__ __volatile__ (
				"flds (%%eax) \n\t"
				"fmuls 4(%%ecx) \n\t"
				"fistpl (%%edx) \n\t"
				::"A"(&t),"d"(&gc->softScanProcs.shade.xRightFixed),"c"(&consts[0]) : "memory" );
#else
				gc->softScanProcs.shade.xRightFixed = __GL_COORD_FLOAT_TO_FIXED (gc->softScanProcs.shade.xRight + gc->softScanProcs.shade.dy * gc->softScanProcs.shade.dxdyRight);
#endif
				__GL_COORD_ADD (gc->softScanProcs.shade.xRightFixed, gc->softScanProcs.shade.xRightFixed, __GL_COORD_ALMOST_HALF);
			}
			(*gc->softScanProcs.fillSubProc)/*FillSubTriangle*/ (gc);
		}
	}
	DRIVERPROC_BUFFER_UNLOCK( gc );

	if( gc->state.poly.OffsetFillEnabled )
	{
		gc->vertices.WindowZ[a] = az;
		gc->vertices.WindowZ[b] = bz;
		gc->vertices.WindowZ[c] = cz;
	}
}


void rasDrawTriangleFillFrontUnordered( __glContext *gc, GLuint a, GLuint b, GLuint c )
{
	rasDrawTriangleFill( gc, a, b, c, 0 );
}

void rasDrawTriangleFillBackUnordered( __glContext *gc, GLuint a, GLuint b, GLuint c )
{
	rasDrawTriangleFill( gc, a, b, c, 1 );
}

void rasDrawTriangleLineFrontUnordered( __glContext *gc, GLuint a, GLuint b, GLuint c )
{
	rasDrawTriangleLine( gc, a, b, c, 0 );
}

void rasDrawTriangleLineBackUnordered( __glContext *gc, GLuint a, GLuint b, GLuint c )
{
	rasDrawTriangleLine( gc, a, b, c, 1 );
}

void rasDrawTrianglePointFrontUnordered( __glContext *gc, GLuint a, GLuint b, GLuint c )
{
	rasDrawTrianglePoint( gc, a, b, c, 0 );
}

void rasDrawTrianglePointBackUnordered( __glContext *gc, GLuint a, GLuint b, GLuint c )
{
	rasDrawTrianglePoint( gc, a, b, c, 1 );
}

