//******************************************************************************
//
//	File:			2dTransform.cpp
//
//	Description:	Matrix transformation in 2-space.
//	
//	Written by:		Dianne Hackborn
//
//	Copyright 2000, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#include <render2/Rect.h>
#include <render2/2dTransform.h>
#include <support2/StdIO.h>
#include <support2_p/SupportMisc.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

namespace B {
namespace Render2 {

const char* B2dTransform::gElementNames[9] = {
	"XX", "YX", "XY", "YY", "XT", "YT", "TX", "TY", "TT"
};

// Private bits in fFlags.
enum {
	// Error status bits.
	kOk					= 0x00000000,
	kInvertFailed		= 0x01000000,
	kErrorMask			= 0xFF000000,
	
	// Flatten-only information.
	kHostEndianFlag		= 0x00800000,
	kOtherEndianFlag	= 0x00008000,
	kEndianMask			= kHostEndianFlag|kOtherEndianFlag
};

struct flat_transform_2d {
	uint32 flags;			// error code packed in to upper byte.
	coord matrix[6];
};

static const coord identity_matrix[6] = { 1, 0, 0, 1, 0, 0 };

const B2dTransform B2dTransform::gIdentity(identity_matrix, B_TRANSFORM_IDENTITY);

inline void B2dTransform::multiply(coord* d, const coord* m1, const coord* m2)
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

inline B2dTransform::B2dTransform(bool)
{
}

//------------------------------------------------------------------------------

B2dTransform::B2dTransform()
{
	memcpy(fMatrix, identity_matrix, sizeof(fMatrix));
	fFlags = B_TRANSFORM_IDENTITY;
}

//------------------------------------------------------------------------------

B2dTransform::B2dTransform(	coord xx, coord yx,
							coord xy, coord yy,
							coord xt, coord yt,
							uint8 operations)
{
	fMatrix[XX] = xx;	fMatrix[YX] = yx;
	fMatrix[XY] = xy;	fMatrix[YY] = yy;
	fMatrix[XT] = xt;	fMatrix[YT] = yt;
	fFlags = operations;
}

//------------------------------------------------------------------------------

B2dTransform::B2dTransform(const B2dTransform& t)
	:	BFlattenable()
{
	memcpy(this, &t, sizeof(B2dTransform));
}

//------------------------------------------------------------------------------

B2dTransform::B2dTransform(const B2dTransform& pre, const B2dTransform& post)
{
	multiply(fMatrix, pre.fMatrix, post.fMatrix);
	fFlags = pre.fFlags | post.fFlags;
}

//------------------------------------------------------------------------------

B2dTransform::B2dTransform(const BValue& o, status_t *result)
{
	status_t r;
	value_ref ref(o);
	if ((r = Unflatten(ref.type, ref.data, ref.length)) != B_OK)
		SetTo(MakeIdentity());
	if (result)
		*result =r;
		
}

B2dTransform::B2dTransform(const value_ref& o, status_t *result)
{
	status_t r;
	if ((r = Unflatten(o.type, o.data, o.length)) != B_OK)
		SetTo(MakeIdentity());
	if (result)
		*result = r;
}

//------------------------------------------------------------------------------

B2dTransform::B2dTransform(const coord* matrix, uint8 operations)
{
	memcpy(fMatrix, matrix, sizeof(fMatrix));
	fFlags = operations;
}

//------------------------------------------------------------------------------

void B2dTransform::SetTo(const B2dTransform& t)
{
	memcpy(this, &t, sizeof(B2dTransform));
}

//------------------------------------------------------------------------------

void B2dTransform::SetTo(const B2dTransform& pre, const B2dTransform& post)
{
	multiply(fMatrix, pre.fMatrix, post.fMatrix);
	fFlags = pre.fFlags | post.fFlags;
}

//------------------------------------------------------------------------------

void B2dTransform::SetTo(const coord* matrix, uint8 operations)
{
	memcpy(fMatrix, matrix, sizeof(fMatrix));
	fFlags = operations;
}

//------------------------------------------------------------------------------

status_t B2dTransform::Status() const
{
	return (fFlags&kErrorMask) == 0 ? B_OK : B_ERROR;
}

//------------------------------------------------------------------------------

void B2dTransform::Swap(B2dTransform& with)
{
	char buffer[sizeof(B2dTransform)];
	memcpy(buffer, this, sizeof(B2dTransform));
	memcpy(this, &with, sizeof(B2dTransform));
	memcpy(&with, buffer, sizeof(B2dTransform));
}

//------------------------------------------------------------------------------

BValue B2dTransform::AsValue() const
{
	flat_transform_2d f2d;
	memcpy(f2d.matrix, fMatrix, sizeof(fMatrix));
	f2d.flags = (fFlags&~kEndianMask) | kHostEndianFlag;
	return BValue(B_TRANSFORM_2D_TYPE, &f2d, sizeof(flat_transform_2d));
}

//------------------------------------------------------------------------------

const coord* B2dTransform::Matrix() const
{
	return fMatrix;
}

//------------------------------------------------------------------------------

void B2dTransform::Get3dMatrix(coord* target) const
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

uint8 B2dTransform::Operations() const
{
	return (uint8)fFlags;
}

//------------------------------------------------------------------------------

BPoint B2dTransform::Origin() const
{
	return BPoint(fMatrix[XT], fMatrix[YT]);
}

//------------------------------------------------------------------------------

coord B2dTransform::Determinant(void) const
{
	return fMatrix[XX] * fMatrix[YY] - fMatrix[YX] * fMatrix[XY];
}


//------------------------------------------------------------------------------

coord B2dTransform::Cofactor(element e) const
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
	static const coord matrix_pad[3] = { 0, 0, 1 };
	
	coord d[2][2];
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
	coord cf = (d[0][0] * d[1][1]) - (d[0][1] * d[1][0]);
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

B2dTransform B2dTransform::MakeTranslate(const coord dx, const coord dy)
{
	return B2dTransform(1, 0, 0, 1, dx, dy, B_TRANSFORM_TRANSLATE);
}

B2dTransform B2dTransform::MakeTranslate(const BPoint& translate)
{
	return MakeTranslate(translate.x, translate.y);
}

//------------------------------------------------------------------------------

B2dTransform B2dTransform::MakeScale(const coord sx, const coord sy)
{
	return B2dTransform(sx, 0, 0, sy, 0, 0, B_TRANSFORM_SCALE);
}

//------------------------------------------------------------------------------

B2dTransform B2dTransform::MakeShear(const coord sx, const coord sy)
{
	return B2dTransform(1, sx, sy, 1, 0, 0, B_TRANSFORM_SHEAR);
}

//------------------------------------------------------------------------------

B2dTransform B2dTransform::MakeSkew(const coord radiansx, const coord radiansy)
{
	return MakeShear(tanf(radiansx), tanf(radiansy));
}

//------------------------------------------------------------------------------

B2dTransform B2dTransform::MakeRotate(const coord radians)
{
	const coord s = sinf(radians);
	const coord c = cosf(radians);
	return B2dTransform(c, s, -s, c, 0, 0, B_TRANSFORM_ROTATE);
}

//------------------------------------------------------------------------------

B2dTransform B2dTransform::MakeRotate(const coord radians, const BPoint& center)
{
	const coord s = sinf(radians);
	const coord c = cosf(radians);
	return B2dTransform(c, s, -s, c,
						center.x-center.x*c+center.y*s,
						center.y-center.x*s+center.y*c,
						B_TRANSFORM_ROTATE);
}

//------------------------------------------------------------------------------

B2dTransform& B2dTransform::Translate(const coord dx, const coord dy)
{
	fMatrix[XT] += dx;
	fMatrix[YT] += dy;
	fFlags |= B_TRANSFORM_TRANSLATE;
	return *this;
}

B2dTransform& B2dTransform::Translate(const BPoint& translate)
{
	return Translate(translate.x, translate.y);
}

//------------------------------------------------------------------------------

B2dTransform& B2dTransform::Scale(const coord amount)
{
	if (amount != 1.0) {
		fMatrix[XX] *= amount;
		fMatrix[XY] *= amount;
		fMatrix[XT] *= amount;
		fMatrix[YX] *= amount;
		fMatrix[YY] *= amount;
		fMatrix[YT] *= amount;
		fFlags |= B_TRANSFORM_SCALE;
	}
	return *this;
}

//------------------------------------------------------------------------------

B2dTransform& B2dTransform::Scale(const coord sx, const coord sy)
{
	if (sx != 1.0) {
		fMatrix[XX] *= sx;
		fMatrix[XY] *= sx;
		fMatrix[XT] *= sx;
		fFlags |= B_TRANSFORM_SCALE;
	}
	if (sy != 1.0) {
		fMatrix[YX] *= sy;
		fMatrix[YY] *= sy;
		fMatrix[YT] *= sy;
		fFlags |= B_TRANSFORM_SCALE;
	}
	return *this;
}

//------------------------------------------------------------------------------

B2dTransform& B2dTransform::Shear(const coord sx, const coord sy)
{
	coord src[6];
	memcpy(src, fMatrix, sizeof(fMatrix));
	if (sx != 0) {
		fMatrix[XX] = src[XX] + src[YX]*sx;
		fMatrix[XY] = src[XY] + src[YY]*sx;
		fMatrix[XT] = src[XT] + src[YT]*sx;
		fFlags |= B_TRANSFORM_SHEAR;
	}
	if (sy != 0) {
		fMatrix[YX] = src[XX]*sy + src[YX];
		fMatrix[YY] = src[XY]*sy + src[YY];
		fMatrix[YT] = src[XT]*sy + src[YT];
		fFlags |= B_TRANSFORM_SHEAR;
	}
	return *this;
}

//------------------------------------------------------------------------------

B2dTransform& B2dTransform::Skew(const coord radiansx, const coord radiansy)
{
	return Shear(tanf(radiansx), tanf(radiansy));
}

//------------------------------------------------------------------------------

B2dTransform& B2dTransform::Rotate(const coord radians)
{
	const coord s = sinf(radians);
	const coord c = cosf(radians);
	coord src[6];
	memcpy(src, fMatrix, sizeof(fMatrix));
	fMatrix[XX] = src[XX]*c + src[YX]*(-s);
	fMatrix[XY] = src[XY]*c + src[YY]*(-s);
	fMatrix[XT] = src[XT]*c + src[YT]*(-s);
	fMatrix[YX] = src[XX]*s + src[YX]*c;
	fMatrix[YY] = src[XY]*s + src[YY]*c;
	fMatrix[YT] = src[XT]*s + src[YT]*c;
	fFlags |= B_TRANSFORM_ROTATE;
	return *this;
}

//------------------------------------------------------------------------------

B2dTransform& B2dTransform::Rotate(const coord radians, const BPoint& center)
{
	const coord s = sinf(radians);
	const coord c = cosf(radians);
	coord src[6];
	memcpy(src, fMatrix, sizeof(fMatrix));
	fMatrix[XX] = src[XX]*c + src[YX]*(-s);
	fMatrix[XY] = src[XY]*c + src[YY]*(-s);
	fMatrix[XT] = src[XT]*c + src[YT]*(-s) + (center.x-center.x*c+center.y*s);
	fMatrix[YX] = src[XX]*s + src[YX]*c;
	fMatrix[YY] = src[XY]*s + src[YY]*c;
	fMatrix[YT] = src[XT]*s + src[YT]*c + (center.y-center.x*s+center.y*c);
	fFlags |= B_TRANSFORM_TRANSLATE|B_TRANSFORM_ROTATE;
	return *this;
}

//------------------------------------------------------------------------------

B2dTransform& B2dTransform::Concatenate(const B2dTransform& t)
{
	if (t.fFlags != B_TRANSFORM_IDENTITY) {
		B2dTransform src(*this);
		multiply(fMatrix, src.fMatrix, t.fMatrix);
		fFlags |= t.fFlags;
	}
	return *this;
}

//------------------------------------------------------------------------------

void B2dTransform::Concatenate(const B2dTransform& t, B2dTransform* dest) const
{
	multiply(dest->fMatrix, fMatrix, t.fMatrix);
	dest->fFlags = fFlags | t.fFlags;
}

//------------------------------------------------------------------------------

B2dTransform& B2dTransform::PreConcatenate(const B2dTransform& t)
{
	if (t.fFlags != B_TRANSFORM_IDENTITY) {
		B2dTransform src(*this);
		multiply(fMatrix, t.fMatrix, src.fMatrix);
		fFlags |= t.fFlags;
	}
	return *this;
}

//------------------------------------------------------------------------------

void B2dTransform::PreConcatenate(const B2dTransform& t, B2dTransform* dest) const
{
	multiply(dest->fMatrix, t.fMatrix, fMatrix);
	dest->fFlags = fFlags | t.fFlags;
}

//------------------------------------------------------------------------------

B2dTransform B2dTransform::Invert(void) const DECLARE_RETURN(dest)
{
	B2dTransform dest(true);
	
	const coord D = Determinant();
	if (D != 0.0) {
		const coord factor = 1.0 / D;
		
		// Commented out code is the less-optimized version that calls
		// Cofactor().  Instead, we just do those computations inline.
		
		dest.fMatrix[XX] = (fMatrix[YY]) /*Cofactor(XX)*/ * factor;
		dest.fMatrix[XY] = (-fMatrix[XY]) /*Cofactor(XY)*/ * factor;
		dest.fMatrix[XT] = (fMatrix[XY]*fMatrix[YT] - fMatrix[XT]*fMatrix[YY]) /*Cofactor(XT)*/ * factor;
		dest.fMatrix[YX] = (-fMatrix[YX]) /*Cofactor(YX)*/ * factor;
		dest.fMatrix[YY] = (fMatrix[XX]) /*Cofactor(YY)*/ * factor;
		dest.fMatrix[YT] = (fMatrix[XT]*fMatrix[YX] - fMatrix[XX]*fMatrix[YT]) /*Cofactor(YT)*/ * factor;
		dest.fFlags = fFlags;
		
	} else {
		dest = *this;
		dest.fFlags |= kInvertFailed;
	}
	
	return dest;
}

//------------------------------------------------------------------------------

B2dTransform B2dTransform::operator*(const B2dTransform& t) const DECLARE_RETURN(dest)
{
	B2dTransform dest(true);
	multiply(dest.fMatrix, fMatrix, t.fMatrix);
	dest.fFlags = fFlags | t.fFlags;
	return dest;
}

//------------------------------------------------------------------------------

B2dTransform& B2dTransform::operator*=(const B2dTransform& t)
{
	if (t.fFlags != B_TRANSFORM_IDENTITY) {
		B2dTransform src(*this);
		multiply(fMatrix, src.fMatrix, t.fMatrix);
		fFlags |= t.fFlags;
	}
	return *this;
}

//------------------------------------------------------------------------------

int32 B2dTransform::Compare(const B2dTransform& t) const
{
	if ((fFlags&kErrorMask) != (t.fFlags&kErrorMask)) {
		return (fFlags&kErrorMask) < (t.fFlags&kErrorMask) ? -1 : 1;
	}
	if (fMatrix[XX] != t.fMatrix[XX]) return fMatrix[XX] < t.fMatrix[XX] ? -1 : 1;
	if (fMatrix[YX] != t.fMatrix[YX]) return fMatrix[YX] < t.fMatrix[YX] ? -1 : 1;
	if (fMatrix[XY] != t.fMatrix[XY]) return fMatrix[XY] < t.fMatrix[XY] ? -1 : 1;
	if (fMatrix[YY] != t.fMatrix[YY]) return fMatrix[YY] < t.fMatrix[YY] ? -1 : 1;
	if (fMatrix[XT] != t.fMatrix[XT]) return fMatrix[XT] < t.fMatrix[XT] ? -1 : 1;
	if (fMatrix[YT] != t.fMatrix[YT]) return fMatrix[YT] < t.fMatrix[YT] ? -1 : 1;
	return 0;
}

//------------------------------------------------------------------------------

bool B2dTransform::operator!=(const B2dTransform& t) const
{
	return	(fMatrix[XX] != t.fMatrix[XX]) || (fMatrix[YX] != t.fMatrix[YX]) ||
			(fMatrix[XY] != t.fMatrix[XY]) || (fMatrix[YY] != t.fMatrix[YY]) ||
			(fMatrix[XT] != t.fMatrix[XT]) || (fMatrix[YT] != t.fMatrix[YT]);
}

//------------------------------------------------------------------------------

bool B2dTransform::operator==(const B2dTransform& t) const
{
	return	(fMatrix[XX] == t.fMatrix[XX]) && (fMatrix[YX] == t.fMatrix[YX]) &&
			(fMatrix[XY] == t.fMatrix[XY]) && (fMatrix[YY] == t.fMatrix[YY]) &&
			(fMatrix[XT] == t.fMatrix[XT]) && (fMatrix[YT] == t.fMatrix[YT]);
}

//------------------------------------------------------------------------------

BPoint B2dTransform::Transform(const BPoint &from) const
{
	return BPoint(from.x*fMatrix[XX] + from.y*fMatrix[XY] + fMatrix[XT],
				  from.x*fMatrix[YX] + from.y*fMatrix[YY] + fMatrix[YT]);
}

//------------------------------------------------------------------------------

void B2dTransform::Transform(BPoint* target) const
{
	if (fFlags != B_TRANSFORM_IDENTITY) {
		const coord x = target->x;
		target->x = x*fMatrix[XX] + target->y*fMatrix[XY] + fMatrix[XT];
		target->y = x*fMatrix[YX] + target->y*fMatrix[YY] + fMatrix[YT];
	}
}

//------------------------------------------------------------------------------

void B2dTransform::Transform(BPoint* target, size_t num) const
{
	if (fFlags == B_TRANSFORM_IDENTITY)
		return;
	
	if (fFlags == B_TRANSFORM_TRANSLATE) {
		while (num) {
			target->x += fMatrix[XT];
			target->y += fMatrix[YT];
			target++;
			num--;
		}
	
	} else {
		while (num) {
			const coord x = target->x;
			target->x = x*fMatrix[XX] + target->y*fMatrix[XY] + fMatrix[XT];
			target->y = x*fMatrix[YX] + target->y*fMatrix[YY] + fMatrix[YT];
			target++;
			num--;
		}
	}
}

//------------------------------------------------------------------------------

void B2dTransform::Transform(BPoint* dest, const BPoint* src, size_t num) const
{
	if (fFlags == B_TRANSFORM_IDENTITY)
		return;
	
	if (fFlags == B_TRANSFORM_TRANSLATE) {
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

void B2dTransform::TransformBounds(BRect* target) const
{
	if (fFlags == B_TRANSFORM_IDENTITY)
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

BPoint B2dTransform::DeltaTransform(const BPoint &from) const
{
	return BPoint(from.x*fMatrix[XX] + from.y*fMatrix[XY],
				  from.x*fMatrix[YX] + from.y*fMatrix[YY]);
}

//------------------------------------------------------------------------------

void B2dTransform::DeltaTransform(BPoint* target) const
{
	const coord x = target->x;
	target->x = x*fMatrix[XX] + target->y*fMatrix[XY];
	target->y = x*fMatrix[YX] + target->y*fMatrix[YY];
}

//------------------------------------------------------------------------------

void B2dTransform::DeltaTransform(BPoint* target, size_t num) const
{
	if (fFlags <= B_TRANSFORM_TRANSLATE)
		return;
	
	while (num) {
		const coord x = target->x;
		target->x = x*fMatrix[XX] + target->y*fMatrix[XY];
		target->y = x*fMatrix[YX] + target->y*fMatrix[YY];
		target++;
		num--;
	}
}

//------------------------------------------------------------------------------

void B2dTransform::DeltaTransform(BPoint* dest, const BPoint* src, size_t num) const
{
	if (fFlags <= B_TRANSFORM_TRANSLATE)
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

void B2dTransform::DeltaTransformBounds(BRect* target) const
{
	if (fFlags <= B_TRANSFORM_TRANSLATE)
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

void B2dTransform::PrintToStream(ITextOutput::arg io, uint32 flags) const
{
	static uint8 opCodes[] = {
		B_TRANSFORM_TRANSLATE, B_TRANSFORM_SCALE, B_TRANSFORM_SHEAR,
		B_TRANSFORM_ROTATE, B_TRANSFORM_OTHER
	};
	static const char* opNames[] = {
		"Tr", "Sc", "Sh", "Ro", "Ot"
	};
	
	if ((fFlags&kErrorMask) == 0) {
		if (flags&B_PRINT_STREAM_HEADER) io << "B2dTransform([";
		else io << "[";
		io	<< fMatrix[XX] << ", " << fMatrix[XY] << ", " << fMatrix[XT] << "], ["
			<< fMatrix[YX] << ", " << fMatrix[YY] << ", " << fMatrix[YT] << "] {";
		if (fFlags == B_TRANSFORM_IDENTITY)
			io << "Id";
		else {
			bool has = false;
			for (size_t i=0; i<(sizeof(opCodes)/sizeof(opCodes[0])); i++) {
				if (fFlags&opCodes[i]) {
					if (has) io << "+";
					io << opNames[i];
					has = true;
				}
			}
		}
		if (flags&B_PRINT_STREAM_HEADER) io << "})";
		else io << "}";
	} else {
		const char* err = strerror(Status());
		if (flags&B_PRINT_STREAM_HEADER) io << "B2dTransform(" << err << ")";
		else io << "(" << err << ")";
	}
}

status_t B2dTransform::printer(ITextOutput::arg io, const value_ref& val, uint32 flags)
{
	status_t result;
	B2dTransform obj(val, &result);
	if (result == B_OK) obj.PrintToStream(io, flags);
	return result;
}

/*----------------------------------------------------------------*/

bool B2dTransform::IsFixedSize() const
{
	return true;
}

type_code B2dTransform::TypeCode() const
{
	return B_TRANSFORM_2D_TYPE;
}

ssize_t B2dTransform::FlattenedSize() const
{
	return sizeof(flat_transform_2d);
}

status_t B2dTransform::Flatten(void *buffer, ssize_t size) const
{
	if ((size_t)size >= sizeof(flat_transform_2d)) {
		flat_transform_2d* f2d = static_cast<flat_transform_2d*>(buffer);
		memcpy(f2d->matrix, fMatrix, sizeof(fMatrix));
		f2d->flags = (fFlags&~kEndianMask) | kHostEndianFlag;
		return B_OK;
	} else {
		return B_BAD_VALUE;
	}
}

bool B2dTransform::AllowsTypeCode(type_code code) const
{
	return code == B_TRANSFORM_2D_TYPE;
}

status_t B2dTransform::Unflatten(type_code c, const void *buf, ssize_t size)
{
	if (c == B_TRANSFORM_2D_TYPE) {
		if ((size_t)size >= sizeof(flat_transform_2d)) {
			const flat_transform_2d& f2d = *static_cast<const flat_transform_2d*>(buf);
			if ((f2d.flags&kHostEndianFlag) != 0) {
				memcpy(fMatrix, f2d.matrix, sizeof(fMatrix));
				fFlags = f2d.flags&~kEndianMask;
			} else {
				for (int32 i=0; i<6; i++) fMatrix[i] = B_BYTE_SWAP_FLOAT(f2d.matrix[i]);
				fFlags = B_BYTE_SWAP_INT32(f2d.flags)&~kEndianMask;
			}
			return B_OK;
		} else {
			return B_BAD_VALUE;
		}
	} else {
		return B_BAD_TYPE;
	}
}

//------------------------------------------------------------------------------

void BMoveBefore(B2dTransform* to, B2dTransform* from, size_t count)
{
	memcpy(to, from, sizeof(B2dTransform)*count);
}

void BMoveAfter(B2dTransform* to, B2dTransform* from, size_t count)
{
	memmove(to, from, sizeof(B2dTransform)*count);
}

ITextOutput::arg operator<<(ITextOutput::arg io, const B2dTransform& t)
{
	t.PrintToStream(io, B_PRINT_STREAM_HEADER);
	return io;
}

} }	// namespace B::Render2
