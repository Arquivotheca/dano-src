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

#define COLOR_SCALE_CONST (255)

static GLfloat consts[] = { 0.5, 1<<16 };

static void FillSubTriangle (rasState *state, __glShade * sh);


GLfloat rasGetPolyOffset( rasState *state, rasVertex *a, rasVertex *b, rasVertex *c )
{
	GLfloat v1[4],v2[4],res[4];
	GLfloat dzdx, dzdy, max;

	v1[0] = a->window.x - b->window.x;
	v1[1] = a->window.y - b->window.y;
	v1[2] = a->window.z - b->window.z;
	v2[0] = c->window.x - b->window.x;
	v2[1] = c->window.y - b->window.y;
	v2[2] = c->window.z - b->window.z;
	mathVectorCross3( v1, v2, res );

	dzdx = res[0] / (res[2]+0.0000001);
	dzdy = res[1] / (res[2]+0.0000001);
	if( dzdx < 0 )
		dzdx = -dzdx;
	if( dzdy < 0 )
		dzdy = -dzdy;
	
	if( dzdx > dzdy )
		max = dzdx;
	else
		max = dzdy;

	max = max * state->polyOffsetFactor + state->polyOffsetR * state->polyOffsetUnit;
	return max;
}

static void FillSubTriangle (rasState *state, __glShade * sh)
{
	GLint yBottom, yTop, ixLeft, ixRight;
	GLint spanWidth;
	__glFixedCoord xLeft, xRight;
	__glFixedCoord dxdyLeft, dxdyRight;
	GLfloat r, g, b, a, s, t, qw, f;
	GLfloat z;
	__glFragment frag;

	xLeft = sh->xLeftFixed;
	xRight = sh->xRightFixed;

#if __INTEL__ && __GNUC__
	__asm__ __volatile__ (
	"flds (%%eax) \n\t"
	"fmuls 4(%%ecx) \n\t"
	"fistpl (%%edx) \n\t"
	::"a"(&sh->dxdyLeft),"d"(&dxdyLeft),"c"(&consts[0]) : "memory" );
	__asm__ __volatile__ (
	"flds (%%eax) \n\t"
	"fmuls 4(%%ecx) \n\t"
	"fistpl (%%edx) \n\t"
	::"a"(&sh->dxdyRight),"d"(&dxdyRight),"c"(&consts[0]) : "memory" );
#else
	dxdyLeft = __GL_COORD_SIGNED_FLOAT_TO_FIXED (sh->dxdyLeft);
	dxdyRight = __GL_COORD_SIGNED_FLOAT_TO_FIXED (sh->dxdyRight);
#endif
	
	if (state->polySmoothEnabled)
	{
		frag.color.r = r = sh->r0;
		frag.color.g = g = sh->g0;
		frag.color.b = b = sh->b0;
		frag.color.a = a = sh->a0;
	}
	else
	{
		frag.color.r = sh->r0;
		frag.color.g = sh->g0;
		frag.color.b = sh->b0;
		frag.color.a = sh->a0;
	}
	z = sh->z0;
	s = sh->s0;
	t = sh->t0;
	qw = sh->qw0;
	f = sh->f0;

	yBottom = sh->iyBottom;
	yTop = sh->iyTop;
//    fp = gc->methods.span0;
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
			   ** multiplying by the change in the parameter in x.
			 */
			frag.x = ixLeft;
			frag.y = yBottom;
			if (state->polySmoothEnabled)
			{
				frag.color.r = r + dx * sh->drdx;
				frag.color.g = g + dx * sh->dgdx;
				frag.color.b = b + dx * sh->dbdx;
				frag.color.a = a + dx * sh->dadx;
			}
			frag.z = z + dx * sh->dzdx;
			if (state->textureEnabled)
			{
				frag.qw = qw + dx * sh->dqwdx;
				frag.s = s + dx * sh->dsdx;
				frag.t = t + dx * sh->dtdx;
			}
			if (state->fogEnabled)
			{
				frag.f = f + dx * sh->dfdx;
				sh->f0 = f + dx * sh->dfdx;
			}


			/*
			   ** draw a scanline here
			 */
			if ((frag.y >= state->transformClipY0) && (frag.y < state->transformClipY1))
			{
				state->scanlineBuffer.y = frag.y;
				rasProcessScanline (state, &frag, sh, spanWidth);
				//(*fp)(cfb, &frag, sh, spanWidth);
			}
		}

		/*
		   ** Advance edge walking parameters
		 */
		__GL_COORD_ADD (xLeft, xLeft, dxdyLeft);
		__GL_COORD_ADD (xRight, xRight, dxdyRight);
		if (state->polySmoothEnabled)
		{
			r += sh->drdxdy;
			g += sh->dgdxdy;
			b += sh->dbdxdy;
			a += sh->dadxdy;
		}
		z += sh->dzdxdy;
		if (state->textureEnabled)
		{
			s += sh->dsdxdy;
			t += sh->dtdxdy;
			qw += sh->dqwdxdy;
		}
		if (state->fogEnabled)
		{
			f += sh->dfdxdy;
		}
		yBottom++;
	}

	sh->xLeftFixed = xLeft;
	sh->xRightFixed = xRight;

	if (state->polySmoothEnabled)
	{
		sh->r0 = r;
		sh->g0 = g;
		sh->b0 = b;
		sh->a0 = a;
	}
	sh->z0 = z;
	sh->s0 = s;
	sh->t0 = t;
	sh->qw0 = qw;
	sh->f0 = f;
}

/*
   ** Take y coordinates and convert them to integer form, taking into
   ** account the rules that determine which pixel centers are in and which
   ** are not by using the "ALMOST_HALF" constant.
 */
static GLboolean SnapYToPixelCenter (__glShade * sh)
{
#if __INTEL__ && __GNUC__
	__asm__ __volatile__ (
	"flds (%%eax) \n\t"
	"fistpl (%%edx) \n\t"
	::"a"(&sh->yBottom),"d"(&sh->iyBottom) );
	__asm__ __volatile__ (
	"flds (%%eax) \n\t"
	"fistpl (%%edx) \n\t"
	::"a"(&sh->yTop),"d"(&sh->iyTop) );
#else
	__glFixedCoord temp;

	temp = __GL_COORD_FLOAT_TO_FIXED (sh->yBottom);
	__GL_COORD_ADD (temp, temp, __GL_COORD_ALMOST_HALF);
	sh->iyBottom = (GLint) __GL_COORD_FIXED_TO_INT (temp);

	temp = __GL_COORD_FLOAT_TO_FIXED (sh->yTop);
	__GL_COORD_ADD (temp, temp, __GL_COORD_ALMOST_HALF);
	sh->iyTop = (GLint) __GL_COORD_FIXED_TO_INT (temp);
#endif

	/*
	   ** Compute how far we moved in y.  This value is needed so that the
	   ** other parameters can be adjusted by this motion.
	 */
	sh->dy = (sh->iyBottom + __glHalf) - sh->yBottom;
//    assert(sh->dy < __glOne);

	return sh->iyBottom == sh->iyTop;
}

static void SetInitialParameters ( rasState *state, __glShade * sh, const rasVertex * a,
								  const rasColor * ac, GLfloat aFog)
{
	GLfloat dy = sh->dy;
	GLfloat dxdyLeft = sh->dxdyLeft;

	if (state->polySmoothEnabled)
	{
		sh->drdxdy = sh->drdy + dxdyLeft * sh->drdx;
		sh->dgdxdy = sh->dgdy + dxdyLeft * sh->dgdx;
		sh->dbdxdy = sh->dbdy + dxdyLeft * sh->dbdx;
		sh->dadxdy = sh->dady + dxdyLeft * sh->dadx;
		sh->r0 = ac->r * COLOR_SCALE_CONST + dy * sh->drdxdy;
		sh->g0 = ac->g * COLOR_SCALE_CONST + dy * sh->dgdxdy;
		sh->b0 = ac->b * COLOR_SCALE_CONST + dy * sh->dbdxdy;
		sh->a0 = ac->a * COLOR_SCALE_CONST + dy * sh->dadxdy;
	}

	sh->dzdxdy = sh->dzdy + dxdyLeft * sh->dzdx;
	sh->z0 = a->window.z + dy * sh->dzdxdy;

	if (state->textureEnabled)
	{
		GLfloat oneOverW = a->window.w;
		sh->dsdxdy = sh->dsdy + dxdyLeft * sh->dsdx;
		sh->dtdxdy = sh->dtdy + dxdyLeft * sh->dtdx;
		sh->dqwdxdy = sh->dqwdy + dxdyLeft * sh->dqwdx;
		sh->s0 = a->texture.x * oneOverW + dy * sh->dsdxdy;
		sh->t0 = a->texture.y * oneOverW + dy * sh->dtdxdy;
		sh->qw0 = a->texture.w * oneOverW + dy * sh->dqwdxdy;
	}

	if (state->fogEnabled)
	{
		sh->dfdxdy = sh->dfdy + dxdyLeft * sh->dfdx;
		sh->f0 = aFog + dy * sh->dfdxdy;
	}
}

void rasDrawTriangleLine( rasState * state, rasVertex * a, rasVertex * b, rasVertex * c)
{
	__glContext *gc = (__glContext *)state->glReserved;
	GLfloat az,bz,cz, offset;
	
	if( state->polyOffsetLineEnabled )
	{
		offset = rasGetPolyOffset( state, a, b, c );
		az = a->window.z;
		bz = b->window.z;
		cz = c->window.z;
		a->window.z += offset;
		b->window.z += offset;
		c->window.z += offset;
	}
	
	if (a->boundaryEdge)
		(*gc->rasterState->procs.line)(state, a, b);
	if (b->boundaryEdge)
		(*gc->rasterState->procs.line)(state, b, c);
	if (c->boundaryEdge)
		(*gc->rasterState->procs.line)(state, c, a);

	if( state->polyOffsetLineEnabled )
	{
		a->window.z = az;
		b->window.z = bz;
		c->window.z = cz;
	}

}

void rasDrawTrianglePoint( rasState * state, rasVertex * a, rasVertex * b, rasVertex * c)
{
	__glContext *gc = (__glContext *)state->glReserved;
	GLfloat az,bz,cz, offset;
	
	if( state->polyOffsetPointEnabled )
	{
		offset = rasGetPolyOffset( state, a, b, c );
		az = a->window.z;
		bz = b->window.z;
		cz = c->window.z;
		a->window.z += offset;
		b->window.z += offset;
		c->window.z += offset;
	}
	if (a->boundaryEdge)
		(*gc->rasterState->procs.point)(state, a);
	if (b->boundaryEdge)
		(*gc->rasterState->procs.point)(state, b);
	if (c->boundaryEdge)
		(*gc->rasterState->procs.point)(state, c);

	if( state->polyOffsetPointEnabled )
	{
		a->window.z = az;
		b->window.z = bz;
		c->window.z = cz;
	}
}

void rasDrawTriangleFill (rasState * state, rasVertex * a, rasVertex * b, rasVertex * c)
{
	__glShade sh;
	rasVertex *temp;

	GLfloat oneOverArea, t1, t2, t3, t4, px;
	GLfloat dxAB, dxAC, dxBC, dyAB, dyBC, dyAC;
	GLfloat dzAC, dzBC;
	GLfloat dxdyAB, dxdyBC, dxdyAC;
	GLfloat aFog, bFog, cFog;
	rasColor *ac, *bc, *cc;
	GLboolean firstSubFilled, abHorizontal, bcHorizontal;
	GLfloat az,bz,cz;
	
	/*
	   ** Sort verticies in y.  Keep track if a reversal of the
	   ** winding occurs.  Adjust the boundary edge flags as
	   ** well.
	 */
	if ( a->window.y > b->window.y )
	{
		temp = a;
		a = b;
		b = temp;
	}
	if ( b->window.y > c->window.y )
	{
		temp = b;
		b = c;
		c = temp;
		if ( a->window.y > b->window.y )
		{
			temp = a;
			a = b;
			b = temp;
		}
	}
	
	if( state->polyOffsetFillEnabled )
	{
		GLfloat offset = rasGetPolyOffset( state, a, b, c );
		az = a->window.z;
		bz = b->window.z;
		cz = c->window.z;
		a->window.z += offset;
		b->window.z += offset;
		c->window.z += offset;
	}

	memset (&sh, 0, sizeof (__glShade));
	sh.dxAC = a->window.x - c->window.x;
	sh.dxBC = b->window.x - c->window.x;
	sh.dyAC = a->window.y - c->window.y;
	sh.dyBC = b->window.y - c->window.y;
	sh.area = sh.dxAC * sh.dyBC - sh.dxBC * sh.dyAC;
	sh.ccw = (sh.area > __glZero);

	/* Toss out zero area triangles */
	if (sh.area == 0)
	{
		if( state->polyOffsetFillEnabled )
		{
			a->window.z = az;
			b->window.z = bz;
			c->window.z = cz;
		}
		return;
	}
//  __glFillTriangle( gc, a, b, c, &sh);


	dxAC = sh.dxAC;
	dxBC = sh.dxBC;
	dyAC = sh.dyAC;
	dyBC = sh.dyBC;

	/*
	   ** dx/dy values are needed for each of the three edges of the
	   ** triangle so that x can be recomputed after each unit step of y.
	   ** Horizontal edges will have a dx/dy that is "large".
	 */
	dxAB = a->window.x - b->window.x;
	dyAB = a->window.y - b->window.y;
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
	   ** iteratively in fixed point.  To iterate the parameters across a
	   ** span, dpdx values are needed (where p represents a parameter
	   ** being iterated; e.g. red).  These values are found by computing
	   ** the derivative of the parameter with respect to x.
	   **
	   ** To step the parameter values in Y...
	 */
	oneOverArea = __glOne / sh.area;
	t1 = dyAC * oneOverArea;
	t2 = dyBC * oneOverArea;
	t3 = dxAC * oneOverArea;
	t4 = dxBC * oneOverArea;
	ac = a->color;
	bc = b->color;
	cc = c->color;
	
	if ( state->polySmoothEnabled )
	{
		GLfloat drAC, dgAC, dbAC, daAC;
		GLfloat drBC, dgBC, dbBC, daBC;

		drAC = ac->r - cc->r;
		drBC = bc->r - cc->r;
		sh.drdx = (drAC * t2 - drBC * t1) * COLOR_SCALE_CONST;
		sh.mmxDR = sh.drdx * 64;
		sh.drdy = (drBC * t3 - drAC * t4) * COLOR_SCALE_CONST;
		dgAC = ac->g - cc->g;
		dgBC = bc->g - cc->g;
		sh.dgdx = (dgAC * t2 - dgBC * t1) * COLOR_SCALE_CONST;
		sh.mmxDG = sh.dgdx * 64;
		sh.dgdy = (dgBC * t3 - dgAC * t4) * COLOR_SCALE_CONST;
		dbAC = ac->b - cc->b;
		dbBC = bc->b - cc->b;
		sh.dbdx = (dbAC * t2 - dbBC * t1) * COLOR_SCALE_CONST;
		sh.mmxDB = sh.dbdx * 64;
		sh.dbdy = (dbBC * t3 - dbAC * t4) * COLOR_SCALE_CONST;
		daAC = ac->a - cc->a;
		daBC = bc->a - cc->a;
		sh.dadx = (daAC * t2 - daBC * t1) * COLOR_SCALE_CONST;
		sh.mmxDA = sh.dadx * 64;
		sh.dady = (daBC * t3 - daAC * t4) * COLOR_SCALE_CONST;
	}
	else
	{
		sh.r0 = a->color->r * COLOR_SCALE_CONST;
		sh.g0 = a->color->g * COLOR_SCALE_CONST;
		sh.b0 = a->color->b * COLOR_SCALE_CONST;
		sh.a0 = a->color->a * COLOR_SCALE_CONST;
	}

	if( state->depthTestEnabled )
	{
		dzAC = a->window.z - c->window.z;
		dzBC = b->window.z - c->window.z;
		sh.dzdx = dzAC * t2 - dzBC * t1;
		sh.dzdy = dzBC * t3 - dzAC * t4;
	}

	if (state->textureEnabled)
	{
		GLfloat awinv, bwinv, cwinv, scwinv, tcwinv, qwcwinv;
		GLfloat dsAC, dsBC, dtAC, dtBC, dqwAC, dqwBC;

		awinv = a->window.w;
		bwinv = b->window.w;
		cwinv = c->window.w;
		scwinv = c->texture.x * cwinv;
		tcwinv = c->texture.y * cwinv;
		qwcwinv = c->texture.w * cwinv;

		dsAC = a->texture.x * awinv - scwinv;
		dsBC = b->texture.x * bwinv - scwinv;
		sh.dsdx = dsAC * t2 - dsBC * t1;
		sh.dsdy = dsBC * t3 - dsAC * t4;
		dtAC = a->texture.y * awinv - tcwinv;
		dtBC = b->texture.y * bwinv - tcwinv;
		sh.dtdx = dtAC * t2 - dtBC * t1;
		sh.dtdy = dtBC * t3 - dtAC * t4;
		dqwAC = a->texture.w * awinv - qwcwinv;
		dqwBC = b->texture.w * bwinv - qwcwinv;
		sh.dqwdx = dqwAC * t2 - dqwBC * t1;
		sh.dqwdy = dqwBC * t3 - dqwAC * t4;
	}

	if (state->fogEnabled)
	{
		GLfloat dfAC, dfBC;

		if (state->fogHint == GL_NICEST)
		{
			/* Use eyeZ for interpolation value */
			aFog = a->eye.z;
			bFog = b->eye.z;
			cFog = c->eye.z;
		}
		else
		{
			/* Use fog(eyeZ) for interpolation value */
			aFog = calcFog (state, a->eye.z );
			bFog = calcFog (state, b->eye.z );
			cFog = calcFog (state, c->eye.z );
		}
		dfAC = aFog - cFog;
		dfBC = bFog - cFog;
		sh.dfdx = dfAC * t2 - dfBC * t1;
		sh.dfdy = dfBC * t3 - dfAC * t4;
	}

	/* Fill first sub triangle */
	firstSubFilled = GL_FALSE;
	if (!abHorizontal)
	{
		sh.xLeft = a->window.x;
		sh.xRight = a->window.x;
		sh.yBottom = a->window.y;
		sh.yTop = b->window.y;
		if (sh.ccw)
		{
			sh.dxdyLeft = dxdyAC;
			sh.dxdyRight = dxdyAB;
		}
		else
		{
			sh.dxdyLeft = dxdyAB;
			sh.dxdyRight = dxdyAC;
		}

		if (!SnapYToPixelCenter (&sh))
		{
			firstSubFilled = GL_TRUE;

			SetInitialParameters (state, &sh, a, ac, aFog);
			/*
			   ** Adjust the xLeft and xRight coordinate by the amount of change
			   ** that y's moving to the pixel center caused.
			 */
#if __INTEL__ && __GNUC__
			{
				float t = sh.xLeft + sh.dy * sh.dxdyLeft;
				__asm__ __volatile__ (
				"flds (%%eax) \n\t"
				"fmuls 4(%%ecx) \n\t"
				"fistpl (%%edx) \n\t"
				::"a"(&t),"d"(&sh.xLeftFixed),"c"(&consts[0]) : "memory" );

				t = sh.xRight + sh.dy * sh.dxdyRight;
				__asm__ __volatile__ (
				"flds (%%eax) \n\t"
				"fmuls 4(%%ecx) \n\t"
				"fistpl (%%edx) \n\t"
				::"a"(&t),"d"(&sh.xRightFixed),"c"(&consts[0]) : "memory" );
			}
#else
			sh.xLeftFixed = __GL_COORD_FLOAT_TO_FIXED (sh.xLeft + sh.dy * sh.dxdyLeft);
			sh.xRightFixed = __GL_COORD_FLOAT_TO_FIXED (sh.xRight + sh.dy * sh.dxdyRight);
#endif

			/*
			   ** Bias xLeft and xRight fixed point numbers by the ALMOST_HALF
			   ** constant so that when the integer forms are computed a simple
			   ** integerization is all that is needed.  This moves the computation
			   ** out of the scan line loop
			 */
			__GL_COORD_ADD (sh.xLeftFixed, sh.xLeftFixed, __GL_COORD_ALMOST_HALF);
			__GL_COORD_ADD (sh.xRightFixed, sh.xRightFixed, __GL_COORD_ALMOST_HALF);
			FillSubTriangle (state, &sh);
		}
	}

	/* Fill second sub triangle */
	if (!bcHorizontal)
	{
		px = (dyBC / dyAC) * dxAC + c->window.x;
		sh.yBottom = b->window.y;
		sh.yTop = c->window.y;
		if (!SnapYToPixelCenter (&sh))
		{
			if (sh.ccw)
			{
				sh.xLeft = px;
				sh.xRight = b->window.x;
				sh.dxdyLeft = dxdyAC;
				sh.dxdyRight = dxdyBC;
				if (!firstSubFilled)
				{
					GLfloat dySave = sh.dy;
					/* Interpolate initial parameters from a along ac */
					sh.dy += b->window.y - a->window.y;
					SetInitialParameters (state, &sh, a, ac, aFog);
					/* Restore dy which is used below to set x values */
					sh.dy = dySave;
				}
			}
			else
			{
				sh.xLeft = b->window.x;
				sh.xRight = px;
				sh.dxdyLeft = dxdyBC;
				sh.dxdyRight = dxdyAC;
				/* Have to set initial parameters when moving to b & bc */
				SetInitialParameters (state, &sh, b, bc, bFog);
			}
			/* Set the x values *only* if they haven't been set already */
			if (!sh.ccw || !firstSubFilled)
			{
#if __INTEL__ && __GNUC__
				float t = sh.xLeft + sh.dy * sh.dxdyLeft;
				__asm__ __volatile__ (
				"flds (%%eax) \n\t"
				"fmuls 4(%%ecx) \n\t"
				"fistpl (%%edx) \n\t"
				::"a"(&t),"d"(&sh.xLeftFixed),"c"(&consts[0]) : "memory" );
#else
				sh.xLeftFixed = __GL_COORD_FLOAT_TO_FIXED (sh.xLeft + sh.dy * sh.dxdyLeft);
#endif
				__GL_COORD_ADD (sh.xLeftFixed, sh.xLeftFixed, __GL_COORD_ALMOST_HALF);
			}
			if (sh.ccw || !firstSubFilled)
			{
#if __INTEL__ && __GNUC__
				float t = sh.xRight + sh.dy * sh.dxdyRight;
				__asm__ __volatile__ (
				"flds (%%eax) \n\t"
				"fmuls 4(%%ecx) \n\t"
				"fistpl (%%edx) \n\t"
				::"a"(&t),"d"(&sh.xRightFixed),"c"(&consts[0]) : "memory" );
#else
				sh.xRightFixed = __GL_COORD_FLOAT_TO_FIXED (sh.xRight + sh.dy * sh.dxdyRight);
#endif
				__GL_COORD_ADD (sh.xRightFixed, sh.xRightFixed, __GL_COORD_ALMOST_HALF);
			}
			FillSubTriangle (state, &sh);
		}
	}

	if( state->polyOffsetFillEnabled )
	{
		a->window.z = az;
		b->window.z = bz;
		c->window.z = cz;
	}

}
