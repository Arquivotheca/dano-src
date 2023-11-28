
//******************************************************************************
//
//	File:		Stroke.cpp
//
//	Description:	Utility class to generate thick line shapes
//	
//	Written by:	George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#include <math.h>
#include <support2/Debug.h>
#include <render2_p/Stroke.h>
#include <render2_p/PointInlines.h>

namespace B {
namespace Render2 {

#define MAX_ROUND_CAP_ERROR 0.4
#define PI M_PI

BStroker::LineCapFunc BStroker::cap_funcs[5] =
{
	BStroker::GenerateRoundCap,
	BStroker::GenerateButtCap,
	BStroker::GenerateButtCap,
	BStroker::GenerateButtCap,
	BStroker::GenerateSquareCap
};

BStroker::LineJoinFunc BStroker::join_funcs[5] =
{
	BStroker::GenerateRoundCorner,
	BStroker::GenerateMiterCorner,
	BStroker::GenerateBevelCorner,
	BStroker::GenerateBevelCorner,
	BStroker::GenerateBevelCorner
};

BPoint ComputeDelta(BPoint p0, BPoint p1, float penSize)
{
	BPoint d,delta;
	float len,_len;
		
	d = p1 - p0;
	len = sqrt(d.x*d.x + d.y*d.y);
	_len = 1.0/len;
	d.x *= _len;
	d.y *= _len;
	delta.x = d.y*penSize;
	delta.y = -d.x*penSize;

	return delta;
};

void BStroker::GenerateRoundCap(
	BStroker *shape,
	BPoint p, BPoint delta)
{
	float thetaCount = 0,c,s;
	BPoint newDelta;
	
	thetaCount += shape->m_theta;

	shape->m_destination->LineTo(p + delta);

	int steps=0,maxSteps=shape->m_numThetaSteps;
	while ((thetaCount < PI) && (steps < maxSteps)) {
		c = shape->m_cosThetaSteps[steps];
		s = shape->m_sinThetaSteps[steps];
		newDelta.x = c*delta.x - s*delta.y;
		newDelta.y = c*delta.y + s*delta.x;
		shape->m_destination->LineTo(p + newDelta);

		thetaCount += shape->m_theta;
		steps++;
	};

	shape->m_destination->LineTo(p - delta);
};

void BStroker::GenerateSquareCap(
	BStroker *shape,
	BPoint p, BPoint delta)
{	
	BPoint halfPen;
	
	halfPen.x = -delta.y;
	halfPen.y = delta.x;
	halfPen = p + halfPen;
	
	shape->m_destination->LineTo(halfPen + delta);
	shape->m_destination->LineTo(halfPen - delta);
};

void BStroker::GenerateButtCap(
	BStroker *shape,
	BPoint p, BPoint delta)
{

	BPoint halfPen;
	float len = 1.0/sqrt((delta.x*delta.x) + (delta.y*delta.y));
	halfPen.x = (-delta.y * len) * 0.5;
	halfPen.y = (delta.x * len) * 0.5;
	halfPen = p + halfPen;
		
	shape->m_destination->LineTo(halfPen+ delta);
	shape->m_destination->LineTo(halfPen - delta);
};

void BStroker::GenerateRoundCorner(
	BStroker *shape,
	BPoint corner, BPoint delta1, BPoint delta2)
{
	float targetCos,c,s;
	BPoint newDelta;
	
	targetCos = (delta1.x*delta2.x + delta1.y*delta2.y)*shape->m_penSize2_;
	int steps = shape->m_numThetaSteps;
	
	while (steps && (targetCos > shape->m_cosThetaSteps[steps-1])) steps--;		
	shape->m_destination->LineTo(corner + delta1);

	if (steps) {
		for (int i=0;i<steps;i++) {
			c = shape->m_cosThetaSteps[i];
			s = shape->m_sinThetaSteps[i];
			newDelta.x = c*delta1.x - s*delta1.y;
			newDelta.y = c*delta1.y + s*delta1.x;
			shape->m_destination->LineTo(corner + newDelta);	
		};
	};

	shape->m_destination->LineTo(corner + delta2);
};

void BStroker::GenerateMiterCorner(
	BStroker *shape,
	BPoint corner, BPoint delta1, BPoint delta2)
{
	int32 err;
	float c;
	BPoint p0,p1,d0,d1,p;

	c = (delta1.x*delta2.x + delta1.y*delta2.y)*shape->m_penSize2_;
	
	if (c < shape->m_cosMiterLimit)
		goto hum;
		
	p0 = corner + delta1;
	p1 = corner + delta2;
	d0.x = -delta1.y;
	d0.y = delta1.x;
	d1.x = -delta2.y;
	d1.y = delta2.x;
	
	err = FindIntersection(p0,d0,p1,d1,p);
	if (!err) {
		shape->m_destination->LineTo(p);
	} else {
		hum:
		shape->m_destination->LineTo(corner + delta2);
	};
};

void BStroker::GenerateBevelCorner(
	BStroker *shape,
	BPoint corner, BPoint /*delta1*/, BPoint delta2)
{
	shape->m_destination->LineTo(corner + delta2);
};

void BStroker::Stroke(bool closed)
{
	int32 vc,limit;
	uint32 i;
	if (!m_cache.CountItems()) return;
	BPoint lastDelta,delta;
	BPoint *sp=&m_cache.EditItemAt(0)+1,*dp=sp;
	for (vc=i=1;i<m_cache.CountItems();i++) {
		if ((sp->x != (sp-1)->x) ||
			(sp->y != (sp-1)->y)) {
			*dp++ = *sp;
			vc++;
		};
		sp++;
	};
	sp = &m_cache.EditItemAt(0);
	dp = sp+vc-1;
	if (close && ((sp->x == dp->x) && (sp->y == dp->y))) vc--;

	BPoint *curve = sp;
	int32 curveLen = vc;
	
	delta = ComputeDelta(curve[0],curve[1],m_halfPen);
	m_destination->MoveTo(curve[0] + delta);
	lastDelta = delta;

	limit = curveLen;
	if (closed) limit++;
	
	for (int32 i=2;i<limit;i++) {
		delta = ComputeDelta(curve[i-1],curve[i%curveLen],m_halfPen);
		m_destination->LineTo(curve[i-1] + lastDelta);
		if ((delta.x*lastDelta.y - delta.y*lastDelta.x) > 1) {
			m_destination->LineTo(curve[i-1] + delta);
		} else {
			m_joinFunc(this,curve[i-1],lastDelta,delta);
		};
		lastDelta = delta;
	};

	if (closed) {
		delta = ComputeDelta(curve[0],curve[1],m_halfPen);
		m_destination->LineTo(curve[0] + lastDelta);
		if ((delta.x*lastDelta.y - delta.y*lastDelta.x) > 1) {
			m_destination->LineTo(curve[0] + delta);
		} else {
			m_joinFunc(this,curve[0],lastDelta,delta);
		};
		m_destination->Close();
		delta = ComputeDelta(curve[curveLen-1],curve[curveLen-2],m_halfPen);
		m_destination->MoveTo(curve[curveLen-1] + delta);
	} else {
		delta = ComputeDelta(curve[curveLen-2],curve[curveLen-1],m_halfPen);
		m_capFunc(this,curve[curveLen-1],delta);
		delta.x = -delta.x;
		delta.y = -delta.y;
	};

	lastDelta = delta;
	
	limit = 0;
	if (closed) limit--;

	for (int32 i=curveLen-3;i>=limit;i--) {
		delta = ComputeDelta(curve[i+1],curve[(i<0)?(curveLen+i):i],m_halfPen);
		m_destination->LineTo(curve[i+1] + lastDelta);
		if ((delta.x*lastDelta.y - delta.y*lastDelta.x) > 1) {
			m_destination->LineTo(curve[i+1] + delta);
		} else {
			m_joinFunc(this,curve[i+1],lastDelta,delta);
		};
		lastDelta = delta;
	};

	if (closed) {
		delta = ComputeDelta(curve[curveLen-1],curve[curveLen-2],m_halfPen);
		m_destination->LineTo(curve[curveLen-1] + lastDelta);
		if ((delta.x*lastDelta.y - delta.y*lastDelta.x) > 1) {
			m_destination->LineTo(curve[curveLen-1] + delta);
		} else {
			m_joinFunc(this,curve[curveLen-1],lastDelta,delta);
		};
	} else {
		delta = ComputeDelta(curve[1],curve[0],m_halfPen);
		m_capFunc(this,curve[0],delta);
	};

	m_destination->Close();
	m_cache.MakeEmpty();
};

BStroker::BStroker(IPath::arg destination, float penSize, cap_mode cap, join_mode join, float miter)
	:	m_destination(destination),
		m_penSize(penSize),
		m_cap(cap),
		m_join(join),
		m_miter(miter)
{
	m_numThetaSteps = 0;
	m_thetaStepsHighWater = 0;
	m_cosThetaSteps = NULL;
	m_sinThetaSteps = NULL;
	LineDependencies();
};

BStroker::~BStroker()
{
	if (m_cosThetaSteps) free(m_cosThetaSteps);
};

void 
BStroker::SetPenSize(float size)
{
	m_penSize = size;
	LineDependencies();
}

void 
BStroker::SetCapping(cap_mode capping)
{
	m_cap = capping;
	LineDependencies();
}

void 
BStroker::SetJoining(join_mode joining)
{
	m_join = joining;
	LineDependencies();
}

void 
BStroker::SetMiter(float miter)
{
	m_miter = miter;
	LineDependencies();
}

void BStroker::LineDependencies()
{
	if ((m_join == B_ROUND_JOIN) || (m_cap == B_ROUND_CAP)) {
		/*	This computes theta such that drawing a circle of diameter penWidth
			with a polygonal approximation computed by stepping by theta around
			it's edge ensures a "flatness" error for each segment of no more than 
			MAX_ROUND_CAP_ERROR. */
		m_theta = 2*acos(1.0 - 2.0*(MAX_ROUND_CAP_ERROR/m_penSize));
		
		/* Precompute terms of the rotation */
		m_numThetaSteps = (int32)floor(PI/m_theta);
		if (m_numThetaSteps > m_thetaStepsHighWater) {
			if (m_cosThetaSteps) free(m_cosThetaSteps);
			m_cosThetaSteps = (float*)malloc(2*m_numThetaSteps*sizeof(float));
			m_sinThetaSteps = m_cosThetaSteps + m_numThetaSteps;
			m_thetaStepsHighWater = m_numThetaSteps;
		};
		for (int i=0;i<m_numThetaSteps;i++) {
			m_cosThetaSteps[i] = cos(m_theta*(i+1));
			m_sinThetaSteps[i] = sin(m_theta*(i+1));
		};
	};
	
	if (m_join == B_MITER_JOIN) {
		float sinTheta_2 = 1.0/m_miter;
		float theta = asin(sinTheta_2) * 2;
		m_cosMiterLimit = -cos(theta);
	};

	m_halfPen = m_penSize*0.5;
	m_penSize2_ = 1.0/(m_halfPen*m_halfPen);
	
	cap_mode cap = m_cap;
	if ((cap == B_ROUND_CAP) && (m_penSize < 3.0)) cap = B_BUTT_CAP;
	m_capFunc = cap_funcs[cap];
	m_internalCapFunc = cap_funcs[m_join];
	m_joinFunc = join_funcs[m_join];
};

void 
BStroker::MoveTo(const BPoint &pt)
{
	if (m_cache.CountItems()) {
		Stroke(false);
		m_cache.MakeEmpty();
	}
	m_cache.AddItem(pt);
}

void 
BStroker::LinesTo(const BPoint *points, int32 lineCount)
{
	while (lineCount--) m_cache.AddItem(*points++);
}

void 
BStroker::Close()
{
	if (m_cache.CountItems()) {
		Stroke(true);
		m_cache.MakeEmpty();
	}
}

} }
