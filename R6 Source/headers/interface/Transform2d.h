/*******************************************************************************
/
/	File:			Transform2d.h
/
/   Description:    BTransform2d represents a 2d transformation matrix.
/
/	Copyright 2000, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef	_TRANSFORM_2D_H
#define	_TRANSFORM_2D_H

#include <BeBuild.h>
#include <Point.h>

class BDataIO;
class BRect;

/*----------------------------------------------------------------*/
/*----- BTransform2d class --------------------------------------------*/

enum {
	B_TRANSFORM_IDENTITY	= 0x00,
	
	B_TRANSFORM_TRANSLATE	= 0x01,
	B_TRANSFORM_SCALE		= 0x02,
	B_TRANSFORM_SHEAR		= 0x04,
	B_TRANSFORM_ROTATE		= 0x08,
	B_TRANSFORM_OTHER		= 0x80,
	
	B_TRANSFORM_ALL			= 0xFF
};

class BTransform2d {
public:
		enum element {
				XX = 0,		XY = 2,		XT = 4,
				YX = 1,		YY = 3,		YT = 5
			/*	0,			0,			1			*/
		};
		
							BTransform2d();
							BTransform2d(	float xx, float yx,
											float xy, float yy,
											float xt, float yt,
											uint8 operations = B_TRANSFORM_ALL);
							BTransform2d(	const float* matrix,
											uint8 operations = B_TRANSFORM_ALL);
							BTransform2d(	const BTransform2d& pt);
							BTransform2d(	const BTransform2d& m1,
											const BTransform2d& m2);
		
		void				SetTo(	const BTransform2d& from);
		void				SetTo(	const BTransform2d& m1,
									const BTransform2d& m2);
		void				SetTo(	const float* matrix,
									uint8 operations = B_TRANSFORM_ALL);
		BTransform2d&		operator=(const BTransform2d& from);

		const float*		Matrix() const;
		void				Get3dMatrix(float* target) const;
		
		uint8				Operations() const;
		
		BPoint				Origin() const;
		
		float				Determinant() const;
		float				Cofactor(element e) const;
		
static	const BTransform2d&	MakeIdentity();
static	BTransform2d		MakeTranslate(const float dx, const float dy);
static	BTransform2d		MakeScale(const float sx, const float sy);
static	BTransform2d		MakeShear(const float sx, const float sy);
static	BTransform2d		MakeSkew(const float radiansx, const float radiansy);
static	BTransform2d		MakeRotate(const float radians);
static	BTransform2d		MakeRotate(const float radians, const BPoint& center);

		BTransform2d&		Translate(const float dx, const float dy);
		BTransform2d&		Scale(const float amount);
		BTransform2d&		Scale(const float sx, const float sy);
		BTransform2d&		Shear(const float sx, const float sy);
		BTransform2d&		Skew(const float radiansx, const float radiansy);
		BTransform2d&		Rotate(const float radians);
		BTransform2d&		Rotate(const float radians, const BPoint& center);
		
		BTransform2d&		Concatenate(	const BTransform2d& m2);
		void				Concatenate(	const BTransform2d& m2,
											BTransform2d* dest) const;
		BTransform2d&		PreConcatenate(	const BTransform2d& m1);
		void				PreConcatenate(	const BTransform2d& m1,
											BTransform2d* dest) const;
		
		BTransform2d		operator*(const BTransform2d&) const;
		BTransform2d&		operator*=(const BTransform2d&);

		status_t			Invert();
		
		bool				operator!=(const BTransform2d&) const;
		bool				operator==(const BTransform2d&) const;

		BPoint				Transform(const BPoint& from) const;
		void				Transform(BPoint* target) const;
		void				Transform(BPoint* target, size_t num) const;
		void				Transform(BPoint* dest, const BPoint* src, size_t num) const;
		void				TransformBounds(BRect* target) const;
		
		BPoint				DeltaTransform(const BPoint& from) const;
		void				DeltaTransform(BPoint* target) const;
		void				DeltaTransform(BPoint* target, size_t num) const;
		void				DeltaTransform(BPoint* dest, const BPoint* src, size_t num) const;
		void				DeltaTransformBounds(BRect* target) const;
		
		void				PrintToStream() const;

private:
							BTransform2d(bool no_init);

static	void				multiply(float* d, const float* m1, const float* m2);

static	const char*			gElementNames[9];

		// NOTE: This variable is made public by the inlined MakeIdentity().
static	const BTransform2d	gIdentity;

		float				fMatrix[6];
		uint8				fOperations;
		uint8				_reservedBytes[3];
		float				_reserved;
};

BPoint operator*(const BTransform2d& t, const BPoint& p);
BPoint operator*(const BPoint& p, const BTransform2d& t);

BDataIO& operator<<(BDataIO& io, const BTransform2d& matrix);

/*----------------------------------------------------------------*/
/*----- inline definitions ---------------------------------------*/

inline BTransform2d& BTransform2d::operator=(const BTransform2d& from)
{
	SetTo(from);
	return *this;
}

inline const BTransform2d& BTransform2d::MakeIdentity()
{
	return gIdentity;
}

inline BPoint operator*(const BTransform2d& t, const BPoint& p)
{
	return t.Transform(p);
}

inline BPoint operator*(const BPoint& p, const BTransform2d& t)
{
	return t.Transform(p);
}

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _POINT_H */
