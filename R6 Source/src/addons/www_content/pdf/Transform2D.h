#if !defined(_TRANSFORM2D_H_)
#define _TRANSFORM2D_H_

#include <SupportDefs.h>
#include <stdio.h>
#include <Point.h>

class Transform2D {
#if 0
float	a, b; // g(0)
float	c, d; // h(0)
float	e, f; // i(1)
#endif

float	m[3][3];

public:
				Transform2D(void);
				Transform2D(float A, float B, float C, float D, float E, float F);
#if 0
				Transform2D(float *fp) :
					a(fp[0]), b(fp[1]), c(fp[2]), d(fp[3]), e(fp[4]), f(fp[5]) { };
				Transform2D(const Transform2D& rhs) :
					a(rhs.a), b(rhs.b), c(rhs.c), d(rhs.d), e(rhs.e), f(rhs.f) { };
#endif
				~Transform2D() {};

#if 0
Transform2D&	operator=(const Transform2D& rhs) {
					if (this == &rhs) return *this;
					a = rhs.a; b = rhs.b;
					c = rhs.c; d = rhs.d;
					e = rhs.e; f = rhs.f;
					return *this;
				};
#endif
Transform2D&	Set(float A, float B, float C, float D, float E, float F);
float			A() const;
float			B() const;
float			C() const;
float			D() const;
float			E() const;
float			F() const;
float			G() const;
float			H() const;
float			I() const;

				/* repace this with rhs * this */
Transform2D&	operator*=(const Transform2D& rhs);
Transform2D&	operator*=(float f);
Transform2D&	operator/=(float f) { return *this *= (1.0/f); };
Transform2D&	Rotate(float deg);
Transform2D&	Translate(float x, float y);
Transform2D&	Scale(float x, float y);
Transform2D&	Skew(float degx, float degy);
Transform2D&	Transpose(void);
Transform2D&	Invert(void);
Transform2D&	RoundBy(float factor);
float			Determinant(void);
float			Cofactor(uint i, uint j);

void			Origin(BPoint *o) const { o->x = m[2][0]; o->y = m[2][1]; };
void			Transform(BPoint *list, uint32 count) const;
BRect			TransformedBounds(const BRect &rect) const;

void			PrintToStream(int level = 0, FILE *f = stdout);

Transform2D&	MultiplyBy(const Transform2D& rhs);

};

inline const Transform2D operator*(const Transform2D& lhs, const Transform2D& rhs)
{
	return Transform2D(rhs) *= lhs;
}
#endif
