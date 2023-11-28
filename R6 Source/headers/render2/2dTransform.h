/*******************************************************************************
/
/	File:			2dTransform.h
/
/   Description:    B2dTransform represents a 2d transformation matrix.
/
/	Copyright 2000, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef	_RENDER2_2DTRANSFORM_H_
#define	_RENDER2_2DTRANSFORM_H_

#include <render2/RenderDefs.h>
#include <render2/Point.h>
#include <support2/Flattenable.h>
#include <support2/Value.h>

namespace B {
namespace Render2 {

using namespace Support2;

/*----------------------------------------------------------------*/
/*----- B2dTransform class --------------------------------------------*/

enum {
	B_TRANSFORM_IDENTITY	= 0x00,
	
	B_TRANSFORM_TRANSLATE	= 0x01,
	B_TRANSFORM_SCALE		= 0x02,
	B_TRANSFORM_SHEAR		= 0x04,
	B_TRANSFORM_ROTATE		= 0x08,
	B_TRANSFORM_OTHER		= 0x80,
	
	B_TRANSFORM_ALL			= 0xFF
};

class B2dTransform : public BFlattenable {
public:
		enum element {
				XX = 0,		XY = 2,		XT = 4,
				YX = 1,		YY = 3,		YT = 5
			/*	0,			0,			1			*/
		};
		
							B2dTransform();
							B2dTransform(	coord xx, coord yx,
											coord xy, coord yy,
											coord xt, coord yt,
											uint8 operations = B_TRANSFORM_ALL);
							B2dTransform(	const coord* matrix,
											uint8 operations = B_TRANSFORM_ALL);
							B2dTransform(	const B2dTransform& pt);
							B2dTransform(	const B2dTransform& m1,
											const B2dTransform& m2);
							B2dTransform(	const BValue& value,
											status_t* result = NULL);
							B2dTransform(	const value_ref& value,
											status_t* result = NULL);
		
		void				SetTo(	const B2dTransform& from);
		void				SetTo(	const B2dTransform& m1,
									const B2dTransform& m2);
		void				SetTo(	const coord* matrix,
									uint8 operations = B_TRANSFORM_ALL);
		B2dTransform&		operator=(const B2dTransform& from);

		status_t			Status() const;
		
		void				Swap(B2dTransform& with);
			
		BValue				AsValue() const;
inline						operator BValue() const				{ return AsValue(); }

		const coord*		Matrix() const;
		void				Get3dMatrix(coord* target) const;
		
		uint8				Operations() const;
		
		BPoint				Origin() const;
		
		coord				Determinant() const;
		coord				Cofactor(element e) const;
		
static	const B2dTransform&	MakeIdentity();
static	B2dTransform		MakeTranslate(const coord dx, const coord dy);
static	B2dTransform		MakeTranslate(const BPoint& translate);
static	B2dTransform		MakeScale(const coord sx, const coord sy);
static	B2dTransform		MakeShear(const coord sx, const coord sy);
static	B2dTransform		MakeSkew(const coord radiansx, const coord radiansy);
static	B2dTransform		MakeRotate(const coord radians);
static	B2dTransform		MakeRotate(const coord radians, const BPoint& center);

		B2dTransform&		Translate(const coord dx, const coord dy);
		B2dTransform&		Translate(const BPoint& translate);
		B2dTransform&		Scale(const coord amount);
		B2dTransform&		Scale(const coord sx, const coord sy);
		B2dTransform&		Shear(const coord sx, const coord sy);
		B2dTransform&		Skew(const coord radiansx, const coord radiansy);
		B2dTransform&		Rotate(const coord radians);
		B2dTransform&		Rotate(const coord radians, const BPoint& center);
		
		B2dTransform&		Concatenate(	const B2dTransform& m2);
		void				Concatenate(	const B2dTransform& m2,
											B2dTransform* dest) const;
		B2dTransform&		PreConcatenate(	const B2dTransform& m1);
		void				PreConcatenate(	const B2dTransform& m1,
											B2dTransform* dest) const;
		
		B2dTransform		operator*(const B2dTransform&) const;
		B2dTransform&		operator*=(const B2dTransform&);

		// NOTE: Invert() can fail!  Be sure to check Status() sometime after
		// using it.
		B2dTransform		Invert() const;

		int32				Compare(const B2dTransform&) const;
		bool				operator!=(const B2dTransform&) const;
		bool				operator==(const B2dTransform&) const;
		bool				operator<(const B2dTransform&) const;
		bool				operator<=(const B2dTransform&) const;
		bool				operator>=(const B2dTransform&) const;
		bool				operator>(const B2dTransform&) const;

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
		
		void				PrintToStream(ITextOutput::arg io, uint32 flags=0) const;
static	status_t			printer(ITextOutput::arg io, const value_ref& val, uint32 flags);

		// Implement BFlattenable API.
		virtual	bool		IsFixedSize() const;
		virtual	type_code	TypeCode() const;
		virtual	ssize_t		FlattenedSize() const;
		virtual	status_t	Flatten(void *buffer, ssize_t size) const;
		virtual	bool		AllowsTypeCode(type_code code) const;
		virtual	status_t	Unflatten(type_code c, const void *buf, ssize_t size);

private:
							B2dTransform(bool no_init);
		
static	void				multiply(coord* d, const coord* m1, const coord* m2);

static	const char*			gElementNames[9];

		// NOTE: This variable is made public by the inlined MakeIdentity().
static	const B2dTransform	gIdentity;

		coord				fMatrix[6];
		uint32				fFlags;
		coord				_reserved;
};

/*----- Mathematical operators --------------------------------------*/
BPoint			operator*(const B2dTransform& t, const BPoint& p);
BPoint			operator*(const BPoint& p, const B2dTransform& t);

/*----- Type and STL utilities --------------------------------------*/
void			BMoveBefore(B2dTransform* to, B2dTransform* from, size_t count = 1);
void			BMoveAfter(B2dTransform* to, B2dTransform* from, size_t count = 1);
void			BSwap(B2dTransform& t1, B2dTransform& t2);
int32			BCompare(const B2dTransform& t1, const B2dTransform& t2);
void			swap(B2dTransform& x, B2dTransform& y);

ITextOutput::arg	operator<<(ITextOutput::arg io, const B2dTransform& matrix);

/*----------------------------------------------------------------*/
/*----- inline definitions ---------------------------------------*/

inline B2dTransform& B2dTransform::operator=(const B2dTransform& from)
{
	SetTo(from);
	return *this;
}

inline const B2dTransform& B2dTransform::MakeIdentity()
{
	return gIdentity;
}

inline BPoint operator*(const B2dTransform& t, const BPoint& p)
{
	return t.Transform(p);
}

inline BPoint operator*(const BPoint& p, const B2dTransform& t)
{
	return t.Transform(p);
}

inline bool B2dTransform::operator<(const B2dTransform& other) const
{
	return Compare(other) < 0;
}

inline bool B2dTransform::operator<=(const B2dTransform& other) const
{
	return Compare(other) <= 0;
}

inline bool B2dTransform::operator>=(const B2dTransform& other) const
{
	return Compare(other) >= 0;
}

inline bool B2dTransform::operator>(const B2dTransform& other) const
{
	return Compare(other) > 0;
}

inline void BSwap(B2dTransform& x, B2dTransform& y)
{
	x.Swap(y);
}

inline int32 BCompare(const B2dTransform& t1, const B2dTransform& t2)
{
	return t1.Compare(t2);
}

inline void swap(B2dTransform& x, B2dTransform& y)
{
	x.Swap(y);
}

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

} } // namespace B::Render2

#endif /* _RENDER2_2DTRANSFORM_H_ */
