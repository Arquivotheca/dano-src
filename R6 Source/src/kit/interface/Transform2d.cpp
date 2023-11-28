//******************************************************************************
//
//	File:			Transform2d.cpp
//
//	Description:	Matrix transformation in 2-space.
//	
//	Written by:		Dianne Hackborn
//
//	Copyright 2000, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#ifndef _TRANSFORM_2D_H
#include "Transform2d.h"
#endif

#ifndef _RECT_H
#include "Rect.h"
#endif

#ifdef B_BEOS_VERSION_DANO
#ifndef _STREAM_IO_H
#include <StreamIO.h>
#endif
#endif

#include <math.h>
#include <stdio.h>
#include <string.h>

// optimization for gcc
#if __GNUC__
#define DECLARE_RETURN(x) return x
#else
#define DECLARE_RETURN(x)
#endif

const char* BTransform2d::gElementNames[9] = {
	"XX", "YX", "XY", "YY", "XT", "YT", "TX", "TY", "TT"
};

static const float identity_matrix[6] = { 1, 0, 0, 1, 0, 0 };

const BTransform2d BTransform2d::gIdentity(identity_matrix, B_TRANSFORM_IDENTITY);

inline void BTransform2d::multiply(float* d, const float* m1, const float* m2)
{
	d[XX] = m1[XX]*m2[XX] + m1[YX]*m2[XY] /*+ m1[TX]*m2[XT]*/ ;
	d[XY] = m1[XY]*m2[XX] + m1[YY]*m2[XY] /*+ m1[TY]*m2[XT]*/ ;
	d[XT] = m1[XT]*m2[XX] + m1[YT]*m2[XY] + /*m1[TT]**/ m2[XT];
	
	d[YX] = m1[XX]*m2[YX] + m1[YX]*m2[YY] /*+ m1[TX]*m2[YT]*/ ;
	d[YY] = m1[XY]*m2[YX] + m1[YY]*m2[YY] /*+ m1[TY]*m2[YT]*/ ;
	d[YT] = m1[XT]*m2[YX] + m1[YT]*m2[YY] + /*m1[TT]**/ m2[YT] ;
	
	/*d[TX] = m1[XX]*m2[TX] + m1[YX]*m2[TY] + m1[TX]*m2[TT]*/
	/*d[TY] = m1[XY]*m2[TX] + m1[YY]*m2[TY] + m1[TY]*m2[TT]*/
	/*d[TT] = m1[XT]*m2[TX] + m1[YT]*m2[TY] + m1[TT]*m2[TT]*/
}

//------------------------------------------------------------------------------

inline BTransform2d::BTransform2d(bool)
{
}

//------------------------------------------------------------------------------

BTransform2d::BTransform2d()
{
	memcpy(fMatrix, identity_matrix, sizeof(fMatrix));
	fOperations = B_TRANSFORM_IDENTITY;
}

//------------------------------------------------------------------------------

BTransform2d::BTransform2d(float xx, float yx,
							float xy, float yy,
							float xt, float yt,
							uint8 operations)
{
	fMatrix[XX] = xx;	fMatrix[YX] = yx;
	fMatrix[XY] = xy;	fMatrix[YY] = yy;
	fMatrix[XT] = xt;	fMatrix[YT] = yt;
	fOperations = operations;
}

//------------------------------------------------------------------------------

BTransform2d::BTransform2d(const BTransform2d& t)
{
	memcpy(fMatrix, t.fMatrix, sizeof(fMatrix));
	fOperations = t.fOperations;
}

//------------------------------------------------------------------------------

BTransform2d::BTransform2d(const BTransform2d& pre, const BTransform2d& post)
{
	multiply(fMatrix, pre.fMatrix, post.fMatrix);
	fOperations = pre.fOperations | post.fOperations;
}

//------------------------------------------------------------------------------

BTransform2d::BTransform2d(const float* matrix, uint8 operations)
{
	memcpy(fMatrix, matrix, sizeof(fMatrix));
	fOperations = operations;
}

//------------------------------------------------------------------------------

void BTransform2d::SetTo(const BTransform2d& t)
{
	memcpy(fMatrix, t.fMatrix, sizeof(fMatrix));
	fOperations = t.fOperations;
}

//------------------------------------------------------------------------------

void BTransform2d::SetTo(const BTransform2d& pre, const BTransform2d& post)
{
	multiply(fMatrix, pre.fMatrix, post.fMatrix);
	fOperations = pre.fOperations | post.fOperations;
}

//------------------------------------------------------------------------------

void BTransform2d::SetTo(const float* matrix, uint8 operations)
{
	memcpy(fMatrix, matrix, sizeof(fMatrix));
	fOperations = operations;
}

//------------------------------------------------------------------------------

const float* BTransform2d::Matrix() const
{
	return fMatrix;
}

//------------------------------------------------------------------------------

void BTransform2d::Get3dMatrix(float* target) const
{
	target[0] = fMatrix[XX];
	target[1] = fMatrix[YX];
	target[2] = 0;
	target[3] = fMatrix[XY];
	target[4] = fMatrix[YY];
	target[5] = 0;
	target[6] = 0;
	target[7] = 0;
	target[8] = 1;
	target[9] = fMatrix[XT];
	target[10] = fMatrix[YT];
	target[11] = 0;
}

//------------------------------------------------------------------------------

uint8 BTransform2d::Operations() const
{
	return fOperations;
}

//------------------------------------------------------------------------------

BPoint BTransform2d::Origin() const
{
	return BPoint(fMatrix[XT], fMatrix[YT]);
}

//------------------------------------------------------------------------------

float BTransform2d::Determinant(void) const
{
	return fMatrix[XX] * fMatrix[YY] - fMatrix[YX] * fMatrix[XY];
}


//------------------------------------------------------------------------------

float BTransform2d::Cofactor(element e) const
{
	switch (e) {
		case XX:	return fMatrix[YY];
		case YX:	return -fMatrix[YX];
		case XY:	return -fMatrix[XY];
		case YY:	return fMatrix[XX];
		case XT:	return fMatrix[XY]*fMatrix[YT] - fMatrix[XT]*fMatrix[YY];
		case YT:	return fMatrix[XT]*fMatrix[YX] - fMatrix[XX]*fMatrix[YT];
	}
	
	return 0;
	
#if 0
	// Here is the "real" algorithm to compute the above...
	
	static const int32 matrix_map[3][3] = {
		{ XX, XY, XT },
		{ YX, YY, YT },
		{ 6,  7,  8  }
	};
	static const float matrix_pad[3] = { 0, 0, 1 };
	
	float d[2][2];
	//int32 l[2][2];
	uint r = uint(e/2);
	uint c = uint(e%2);
	uint rr = 0, cc = 0;
	for (uint i = 0; i < 3; i++)
	{
		if (i != r)
		{
			cc = 0;
			for (uint j = 0; j < 3; j++)
			{
				if (j != c)
				{
					int32 entry = matrix_map[i][j];
					//l[rr][cc] = entry;
					if (entry < 6) d[rr][cc] = fMatrix[entry];
					else d[rr][cc] = matrix_pad[entry-6];
					cc++;
				}
			}
			rr++;
		}
	}
	float cf = (d[0][0] * d[1][1]) - (d[0][1] * d[1][0]);
	if ((r+c) & 0x01) cf *= -1;
	//printf("Co %s = (%s*%s) - (%s*%s)%s\n",
	//		gElementNames[e],
	//		gElementNames[l[0][0]], gElementNames[l[1][1]],
	//		gElementNames[l[0][1]], gElementNames[l[1][0]],
	//		((r+c) & 0x01) ? " * -1" : "");
	
	return cf;
#endif
}

//------------------------------------------------------------------------------

BTransform2d BTransform2d::MakeTranslate(const float dx, const float dy)
{
	return BTransform2d(1, 0, 0, 1, dx, dy, B_TRANSFORM_TRANSLATE);
}

//------------------------------------------------------------------------------

BTransform2d BTransform2d::MakeScale(const float sx, const float sy)
{
	return BTransform2d(sx, 0, 0, sy, 0, 0, B_TRANSFORM_SCALE);
}

//------------------------------------------------------------------------------

BTransform2d BTransform2d::MakeShear(const float sx, const float sy)
{
	return BTransform2d(1, sx, sy, 1, 0, 0, B_TRANSFORM_SHEAR);
}

//------------------------------------------------------------------------------

BTransform2d BTransform2d::MakeSkew(const float radiansx, const float radiansy)
{
	return MakeShear(tanf(radiansx), tanf(radiansy));
}

//------------------------------------------------------------------------------

BTransform2d BTransform2d::MakeRotate(const float radians)
{
	const float s = sinf(radians);
	const float c = cosf(radians);
	return BTransform2d(c, s, -s, c, 0, 0, B_TRANSFORM_ROTATE);
}

//------------------------------------------------------------------------------

BTransform2d BTransform2d::MakeRotate(const float radians, const BPoint& center)
{
	const float s = sinf(radians);
	const float c = cosf(radians);
	return BTransform2d(c, s, -s, c,
						center.x-center.x*c+center.y*s,
						center.y-center.x*s+center.y*c,
						B_TRANSFORM_ROTATE);
}

//------------------------------------------------------------------------------

BTransform2d& BTransform2d::Translate(const float dx, const float dy)
{
	fMatrix[XT] += dx;
	fMatrix[YT] += dy;
	fOperations |= B_TRANSFORM_TRANSLATE;
	return *this;
}

//------------------------------------------------------------------------------

BTransform2d& BTransform2d::Scale(const float amount)
{
	if (amount != 1.0) {
		fMatrix[XX] *= amount;
		fMatrix[XY] *= amount;
		fMatrix[XT] *= amount;
		fMatrix[YX] *= amount;
		fMatrix[YY] *= amount;
		fMatrix[YT] *= amount;
		fOperations |= B_TRANSFORM_SCALE;
	}
	return *this;
}

//------------------------------------------------------------------------------

BTransform2d& BTransform2d::Scale(const float sx, const float sy)
{
	if (sx != 1.0) {
		fMatrix[XX] *= sx;
		fMatrix[XY] *= sx;
		fMatrix[XT] *= sx;
		fOperations |= B_TRANSFORM_SCALE;
	}
	if (sy != 1.0) {
		fMatrix[YX] *= sy;
		fMatrix[YY] *= sy;
		fMatrix[YT] *= sy;
		fOperations |= B_TRANSFORM_SCALE;
	}
	return *this;
}

//------------------------------------------------------------------------------

BTransform2d& BTransform2d::Shear(const float sx, const float sy)
{
	float src[6];
	memcpy(src, fMatrix, sizeof(fMatrix));
	if (sx != 0) {
		fMatrix[XX] = src[XX] + src[YX]*sx;
		fMatrix[XY] = src[XY] + src[YY]*sx;
		fMatrix[XT] = src[XT] + src[YT]*sx;
		fOperations |= B_TRANSFORM_SHEAR;
	}
	if (sy != 0) {
		fMatrix[YX] = src[XX]*sy + src[YX];
		fMatrix[YY] = src[XY]*sy + src[YY];
		fMatrix[YT] = src[XT]*sy + src[YT];
		fOperations |= B_TRANSFORM_SHEAR;
	}
	return *this;
}

//------------------------------------------------------------------------------

BTransform2d& BTransform2d::Skew(const float radiansx, const float radiansy)
{
	return Shear(tanf(radiansx), tanf(radiansy));
}

//------------------------------------------------------------------------------

BTransform2d& BTransform2d::Rotate(const float radians)
{
	const float s = sinf(radians);
	const float c = cosf(radians);
	float src[6];
	memcpy(src, fMatrix, sizeof(fMatrix));
	fMatrix[XX] = src[XX]*c + src[YX]*(-s);
	fMatrix[XY] = src[XY]*c + src[YY]*(-s);
	fMatrix[XT] = src[XT]*c + src[YT]*(-s);
	fMatrix[YX] = src[XX]*s + src[YX]*c;
	fMatrix[YY] = src[XY]*s + src[YY]*c;
	fMatrix[YT] = src[XT]*s + src[YT]*c;
	fOperations |= B_TRANSFORM_ROTATE;
	return *this;
}

//------------------------------------------------------------------------------

BTransform2d& BTransform2d::Rotate(const float radians, const BPoint& center)
{
	const float s = sinf(radians);
	const float c = cosf(radians);
	float src[6];
	memcpy(src, fMatrix, sizeof(fMatrix));
	fMatrix[XX] = src[XX]*c + src[YX]*(-s);
	fMatrix[XY] = src[XY]*c + src[YY]*(-s);
	fMatrix[XT] = src[XT]*c + src[YT]*(-s) + (center.x-center.x*c+center.y*s);
	fMatrix[YX] = src[XX]*s + src[YX]*c;
	fMatrix[YY] = src[XY]*s + src[YY]*c;
	fMatrix[YT] = src[XT]*s + src[YT]*c + (center.y-center.x*s+center.y*c);
	fOperations |= B_TRANSFORM_TRANSLATE|B_TRANSFORM_ROTATE;
	return *this;
}

//------------------------------------------------------------------------------

BTransform2d& BTransform2d::Concatenate(const BTransform2d& t)
{
	if (t.fOperations != B_TRANSFORM_IDENTITY) {
		BTransform2d src(*this);
		multiply(fMatrix, src.fMatrix, t.fMatrix);
		fOperations |= t.fOperations;
	}
	return *this;
}

//------------------------------------------------------------------------------

void BTransform2d::Concatenate(const BTransform2d& t, BTransform2d* dest) const
{
	multiply(dest->fMatrix, fMatrix, t.fMatrix);
	dest->fOperations = fOperations | t.fOperations;
}

//------------------------------------------------------------------------------

BTransform2d& BTransform2d::PreConcatenate(const BTransform2d& t)
{
	if (t.fOperations != B_TRANSFORM_IDENTITY) {
		BTransform2d src(*this);
		multiply(fMatrix, t.fMatrix, src.fMatrix);
		fOperations |= t.fOperations;
	}
	return *this;
}

//------------------------------------------------------------------------------

void BTransform2d::PreConcatenate(const BTransform2d& t, BTransform2d* dest) const
{
	multiply(dest->fMatrix, t.fMatrix, fMatrix);
	dest->fOperations = fOperations | t.fOperations;
}

//------------------------------------------------------------------------------

status_t BTransform2d::Invert(void)
{
	const float D = Determinant();
	if (D == 0.0)
		return B_ERROR;
	
	const float factor = 1.0 / D;
	
	// Commented out code is the less-optimized version that calls
	// Cofactor().  Instead, we just do those computations inline.
	
	/*BTransform2d src(*this);*/
	float m[6];
	memcpy(m, fMatrix, sizeof(fMatrix));
	fMatrix[XX] = (m[YY]) /*src.Cofactor(XX)*/ * factor;
	fMatrix[XY] = (-m[XY]) /*src.Cofactor(XY)*/ * factor;
	fMatrix[XT] = (m[XY]*m[YT] - m[XT]*m[YY]) /*src.Cofactor(XT)*/ * factor;
	fMatrix[YX] = (-m[YX]) /*src.Cofactor(YX)*/ * factor;
	fMatrix[YY] = (m[XX]) /*src.Cofactor(YY)*/ * factor;
	fMatrix[YT] = (m[XT]*m[YX] - m[XX]*m[YT]) /*src.Cofactor(YT)*/ * factor;
	fOperations |= B_TRANSFORM_SCALE;
	
	return B_OK;
}

//------------------------------------------------------------------------------

BTransform2d BTransform2d::operator*(const BTransform2d& t) const DECLARE_RETURN(dest)
{
	BTransform2d dest(true);
	multiply(dest.fMatrix, fMatrix, t.fMatrix);
	dest.fOperations = fOperations | t.fOperations;
	return dest;
}

//------------------------------------------------------------------------------

BTransform2d& BTransform2d::operator*=(const BTransform2d& t)
{
	if (t.fOperations != B_TRANSFORM_IDENTITY) {
		BTransform2d src(*this);
		multiply(fMatrix, src.fMatrix, t.fMatrix);
		fOperations |= t.fOperations;
	}
	return *this;
}

//------------------------------------------------------------------------------

bool BTransform2d::operator!=(const BTransform2d& t) const
{
	return	(fMatrix[XX] != t.fMatrix[XX]) || (fMatrix[YX] != t.fMatrix[YX]) ||
			(fMatrix[XY] != t.fMatrix[XY]) || (fMatrix[YY] != t.fMatrix[YY]) ||
			(fMatrix[XT] != t.fMatrix[XT]) || (fMatrix[YT] != t.fMatrix[YT]);
}

//------------------------------------------------------------------------------

bool BTransform2d::operator==(const BTransform2d& t) const
{
	return	(fMatrix[XX] == t.fMatrix[XX]) && (fMatrix[YX] == t.fMatrix[YX]) &&
			(fMatrix[XY] == t.fMatrix[XY]) && (fMatrix[YY] == t.fMatrix[YY]) &&
			(fMatrix[XT] == t.fMatrix[XT]) && (fMatrix[YT] == t.fMatrix[YT]);
}

//------------------------------------------------------------------------------

BPoint BTransform2d::Transform(const BPoint &from) const
{
	return BPoint(from.x*fMatrix[XX] + from.y*fMatrix[XY] + fMatrix[XT],
				  from.x*fMatrix[YX] + from.y*fMatrix[YY] + fMatrix[YT]);
}

//------------------------------------------------------------------------------

void BTransform2d::Transform(BPoint* target) const
{
	if (fOperations != B_TRANSFORM_IDENTITY) {
		const float x = target->x;
		target->x = x*fMatrix[XX] + target->y*fMatrix[XY] + fMatrix[XT];
		target->y = x*fMatrix[YX] + target->y*fMatrix[YY] + fMatrix[YT];
	}
}

//------------------------------------------------------------------------------

void BTransform2d::Transform(BPoint* target, size_t num) const
{
	if (fOperations == B_TRANSFORM_IDENTITY)
		return;
	
	if (fOperations == B_TRANSFORM_TRANSLATE) {
		while (num) {
			target->x += fMatrix[XT];
			target->y += fMatrix[YT];
			target++;
			num--;
		}
	
	} else {
		while (num) {
			const float x = target->x;
			target->x = x*fMatrix[XX] + target->y*fMatrix[XY] + fMatrix[XT];
			target->y = x*fMatrix[YX] + target->y*fMatrix[YY] + fMatrix[YT];
			target++;
			num--;
		}
	}
}

//------------------------------------------------------------------------------

void BTransform2d::Transform(BPoint* dest, const BPoint* src, size_t num) const
{
	if (fOperations == B_TRANSFORM_IDENTITY)
		return;
	
	if (fOperations == B_TRANSFORM_TRANSLATE) {
		while (num) {
			dest->x = src->x + fMatrix[XT];
			dest->y = src->x + fMatrix[YT];
			dest++;
			src++;
			num--;
		}
	
	} else {
		while (num) {
			dest->x = src->x*fMatrix[XX] + src->y*fMatrix[XY] + fMatrix[XT];
			dest->y = src->x*fMatrix[YX] + src->y*fMatrix[YY] + fMatrix[YT];
			dest++;
			src++;
			num--;
		}
	}
}

//------------------------------------------------------------------------------

void BTransform2d::TransformBounds(BRect* target) const
{
	if (fOperations == B_TRANSFORM_IDENTITY)
		return;
	
	BPoint r[4] = {
		BPoint(target->left, target->top),
		BPoint(target->right, target->top),
		BPoint(target->left, target->bottom),
		BPoint(target->right, target->bottom),
	};
	// transform
	Transform(r, 4);
	// determine the boundaries
	target->left = target->top = r[0].x;
	target->right = target->bottom = r[0].y;
	for (int i = 1; i < 4; i++)
	{
		if (r[i].x < target->left) target->left = r[i].x;
		if (r[i].y < target->top) target->top = r[i].y;
		if (r[i].x > target->right) target->right = r[i].x;
		if (r[i].y > target->bottom) target->bottom = r[i].y;
	}
}

//------------------------------------------------------------------------------

BPoint BTransform2d::DeltaTransform(const BPoint &from) const
{
	return BPoint(from.x*fMatrix[XX] + from.y*fMatrix[XY],
				  from.x*fMatrix[YX] + from.y*fMatrix[YY]);
}

//------------------------------------------------------------------------------

void BTransform2d::DeltaTransform(BPoint* target) const
{
	const float x = target->x;
	target->x = x*fMatrix[XX] + target->y*fMatrix[XY];
	target->y = x*fMatrix[YX] + target->y*fMatrix[YY];
}

//------------------------------------------------------------------------------

void BTransform2d::DeltaTransform(BPoint* target, size_t num) const
{
	if (fOperations == B_TRANSFORM_IDENTITY || fOperations == B_TRANSFORM_TRANSLATE)
		return;
	
	while (num) {
		const float x = target->x;
		target->x = x*fMatrix[XX] + target->y*fMatrix[XY];
		target->y = x*fMatrix[YX] + target->y*fMatrix[YY];
		target++;
		num--;
	}
}

//------------------------------------------------------------------------------

void BTransform2d::DeltaTransform(BPoint* dest, const BPoint* src, size_t num) const
{
	if (fOperations == B_TRANSFORM_IDENTITY || fOperations == B_TRANSFORM_TRANSLATE)
		return;
	
	while (num) {
		dest->x = src->x*fMatrix[XX] + src->y*fMatrix[XY];
		dest->y = src->x*fMatrix[YX] + src->y*fMatrix[YY];
		dest++;
		src++;
		num--;
	}
}

//------------------------------------------------------------------------------

void BTransform2d::DeltaTransformBounds(BRect* target) const
{
	if (fOperations == B_TRANSFORM_IDENTITY || fOperations == B_TRANSFORM_TRANSLATE)
		return;
	
	BPoint r[4] = {
		BPoint(target->left, target->top),
		BPoint(target->right, target->top),
		BPoint(target->left, target->bottom),
		BPoint(target->right, target->bottom),
	};
	// transform
	DeltaTransform(r, 4);
	// determine the boundaries
	target->left = target->top = r[0].x;
	target->right = target->bottom = r[0].y;
	for (int i = 1; i < 4; i++)
	{
		if (r[i].x < target->left) target->left = r[i].x;
		if (r[i].y < target->top) target->top = r[i].y;
		if (r[i].x > target->right) target->right = r[i].x;
		if (r[i].y > target->bottom) target->bottom = r[i].y;
	}
}

//------------------------------------------------------------------------------

void BTransform2d::PrintToStream() const
{
#if SUPPORTS_STREAM_IO
	BOut << *this << endl;
#endif
}

//------------------------------------------------------------------------------

BDataIO& operator<<(BDataIO& io, const BTransform2d& t)
{
#if SUPPORTS_STREAM_IO
	static uint8 opCodes[] = {
		B_TRANSFORM_TRANSLATE, B_TRANSFORM_SCALE, B_TRANSFORM_SHEAR,
		B_TRANSFORM_ROTATE, B_TRANSFORM_OTHER
	};
	static const char* opNames[] = {
		"Tr", "Sc", "Sh", "Ro", "Ot"
	};
	
	const float* m = t.Matrix();
	const uint8 op = t.Operations();
	io << "BTransform2d([" << m[BTransform2d::XX] << ", "
						   << m[BTransform2d::XY] << ", "
						   << m[BTransform2d::XT] << "], ["
						   << m[BTransform2d::YX] << ", "
						   << m[BTransform2d::YY] << ", "
						   << m[BTransform2d::YT] << "] {";
	if (op == B_TRANSFORM_IDENTITY)
		io << "Id";
	else {
		bool has = false;
		for (size_t i=0; i<(sizeof(opCodes)/sizeof(opCodes[0])); i++) {
			if (op&opCodes[i]) {
				if (has) io << "+";
				io << opNames[i];
				has = true;
			}
		}
	}
	io << "})";
#else
	(void)t;
#endif
	
	return io;
}
