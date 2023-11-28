
//******************************************************************************
//
//	File:		bezier.cpp
//
//	Description:	Tesselation of cubic bezier curves
//	
//	Written by:	George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#include <math.h>
#include <render2/Path.h>
#include <render2/Rect.h>
#include <render2_p/PointInlines.h>

namespace B {
namespace Render2 {

#define BEZIER_LEFT 1
#define BEZIER_RIGHT 2
#define BEZIER_DONE 3

inline bool Colinear(BPoint p0, BPoint p1, BPoint p2, BPoint p3, float maxError2)
{
	BPoint d,p,_d;
	float len;
	
	d.x = p3.x-p0.x;
	d.y = p3.y-p0.y;
	
	_d.x = -d.y;
	_d.y = d.x;
	FindIntersection(p0,d,p1,_d,p);
	p.x = p1.x-p.x;
	p.y = p1.y-p.y;
	len = p.x*p.x + p.y*p.y;
	
	if (len < maxError2) {
		FindIntersection(p0,d,p2,_d,p);
		p.x = p2.x-p.x;
		p.y = p2.y-p.y;
		len = p.x*p.x + p.y*p.y;
		if (len < maxError2)
			return true;
	};
	
	return false;
};

struct BezierStackFrame {
	BPoint center,l1,r1,l2,r2,p0,p1,p2,p3;
	int8 state,needClipping;
};

#define BEZIER_STACK_SIZE 16
#define BEZIER_CACHE_SIZE 16

void DecomposeBezier(const BPoint *p, BRect *clip, IPath &path, float maxError)
{
	BezierStackFrame stackFrames[BEZIER_STACK_SIZE],*stack,*oldStack;
	BPoint cache[BEZIER_CACHE_SIZE];
	int32 cacheCount=0;
	BPoint tmp;
	BRect bound;
	float maxError2 = maxError*maxError;
	int32 stackCount=0;
	int8 needClipping,clipped;

	/*	Here's how this recursion works.
	
		p0 through p3 define the control points at the current
		level.  l1,l2,r1,r2 define the middle two control points of,
		respectively, the left half and right half beziers.  center
		is the rightmost control point for the left half and the leftmost
		for the right half, as well as being the point on the curve that
		this recursion contributes.  "state" is the three-valued state of
		recursion.  If it is LEFT, then we should recurse left.  If it is
		RIGHT, recurse right.  If it is DONE, we should go up a level.

		Note that this methodology means that p0 through p3 for a given
		stackPtr=n should be set by level n-1.  Function arguments, if
		you will.
	*/

	stack = stackFrames;
	
	stack->p0 = p[0];
	stack->p1 = p[1];
	stack->p2 = p[2];
	stack->p3 = p[3];
	stack->state = BEZIER_LEFT;
	stack->needClipping = clip?1:0;

loop:

	clipped = 0;
	needClipping = stack->needClipping;
	if (needClipping) {
		bound.left = bound.right = stack->p0.x;
		bound.top = bound.bottom = stack->p0.y;
		if (stack->p1.x < bound.left) bound.left = stack->p1.x;
		if (stack->p2.x < bound.left) bound.left = stack->p2.x;
		if (stack->p3.x < bound.left) bound.left = stack->p3.x;
		if (stack->p1.x > bound.right) bound.right = stack->p1.x;
		if (stack->p2.x > bound.right) bound.right = stack->p2.x;
		if (stack->p3.x > bound.right) bound.right = stack->p3.x;
		if (stack->p1.y < bound.top) bound.top = stack->p1.y;
		if (stack->p2.y < bound.top) bound.top = stack->p2.y;
		if (stack->p3.y < bound.top) bound.top = stack->p3.y;
		if (stack->p1.y > bound.bottom) bound.bottom = stack->p1.y;
		if (stack->p2.y > bound.bottom) bound.bottom = stack->p2.y;
		if (stack->p3.y > bound.bottom) bound.bottom = stack->p3.y;
		needClipping = 0;
		if (clip->left > bound.left) { needClipping = 1; bound.left = clip->left; };
		if (clip->top > bound.top) { needClipping = 1; bound.top = clip->top; };
		if (clip->right < bound.right) { needClipping = 1; bound.right = clip->right; };
		if (clip->bottom < bound.bottom) { needClipping = 1; bound.bottom = clip->bottom; };
		clipped = (bound.left >= bound.right) || (bound.top >= bound.bottom);
	};
	
	/*	We want to stop the recursion if the control points are
		approximately colinear.  Just check if p1 and p2 are "close" to
		being on the line connecting p0 and p3. */

	if ((stackCount > (BEZIER_STACK_SIZE-1)) ||
		Colinear(stack->p0,stack->p1,stack->p2,stack->p3,maxError2) ||
		clipped) {
		stackCount--;
		stack--;
		goto unrecurse;
	};

	/*	We need more precision.  First compute the point-on-curve
		and left and right half control points and store them in the
		stack. */
	tmp				= MidPoint(stack->p1,stack->p2);
	stack->l1		= MidPoint(stack->p0,stack->p1);
	stack->r1 		= MidPoint(stack->p2,stack->p3);
	stack->l2		= MidPoint(stack->l1,tmp);
	stack->r2		= MidPoint(tmp,stack->r1);
	stack->center	= MidPoint(stack->l2,stack->r2);
	
	/*	Now we're ready to recurse left. */
	oldStack = stack++;
	stackCount++;
	stack->p0 = oldStack->p0;		// Leftmost point doesn't change
	stack->p1 = oldStack->l1;		// l1 and l2 define the middle two
	stack->p2 = oldStack->l2;		//    control points for the left half
	stack->p3 = oldStack->center;	// center is the rightmost point of left half
	oldStack->state = BEZIER_RIGHT;	// Remind ourselves to go right next
	stack->state = BEZIER_LEFT;		// Tell the child to start by going left
	stack->needClipping = needClipping;
	goto loop;

unrecurse:
			
	/*	If we're here we've just come back up from a child recursion.
		We have to check the stack state to see if we still have to recurse
		right or if we're done. */

	if ((stackCount<0) || (stack->state==BEZIER_DONE)) {
		/*	We're done with this level.  If this is the top level,
			the whole curve is done except for the last point, so
			exit the loop.  Otherwise, unrecurse. */
		if (stackCount<=0)
			goto doneRecursion;
		stackCount--;
		stack--;
		goto unrecurse;
	};
	
	/*	Before we recurse right, we have to add our center point to the curve. */
	cache[cacheCount++] = stack->center;
	if (cacheCount == BEZIER_CACHE_SIZE) {
		path.LinesTo(cache,cacheCount);
		cacheCount = 0;
	}

	/*	Recurse right */
	oldStack = stack++;
	stackCount++;
	stack->p0 = oldStack->center;	// center is the leftmost point of right half
	stack->p1 = oldStack->r2;		// r1 and r2 define the middle two
	stack->p2 = oldStack->r1;		//    control points for the right half
	stack->p3 = oldStack->p3;		// rightmost point doesn't change
	oldStack->state = BEZIER_DONE;	// We're done after this recursion
	stack->state = BEZIER_LEFT;		// Tell the child to start by going left
	goto loop;

doneRecursion:
	
	cache[cacheCount++] = p[3];
	path.LinesTo(cache,cacheCount);
};

} }
