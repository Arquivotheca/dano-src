#ifndef GMATRIX_H
#define GMATRIX_H

#include <math.h>
#include <assert.h>

enum ESRTComponents
{
	B_SRT_SCALE = 0x1,
	B_SRT_ROTATE = 0x2,
	B_SRT_TRANSLATE = 0x4
};

/*
	Interface: BMatrix2D
	
	This is the interface to a 2D matrix object.  It
	encapsulates typical 2D matrix operations for
	Scale, Rotate, Translate.
*/

class BPoint;


class BMatrix2D
{
public:
	double m_[3][3];	// storage for a 3x3 martrix

					BMatrix2D();
					BMatrix2D(const BMatrix2D &);
					BMatrix2D(const double a00, const double a01, const double a02,
					 const double a10, const double a11, const double a12,
					 const double a20, const double a21, const double a22);
					BMatrix2D(const double scaleX, const double scaleY,
							const double transX, const double transY,
							const double rotation);

					  // array access
					double* operator [](int);
					const double* operator [](int) const;

 			BMatrix2D operator *(double) const;
			
			// Main routine of interest
			void	Transform(float *x, float *y);
			void	Transform(BPoint *aPoint);
			void	Untransform(float *x, float *y);
			void	Untransform(BPoint *aPoint);
			
			// Utility functions
			BMatrix2D & Assign(const double a00, const double a01, const double a02,
					 const double a10, const double a11, const double a12,
					 const double a20, const double a21, const double a22);
			void	Set(const double scaleX, const double scaleY,
							const double transX, const double transY,
							const double rotation);
			void	MakeIdentity();
			BMatrix2D	Transpose() const;
			BMatrix2D	Inverse() const;
			void	Invert();
			double	Determinant() const;
			void	Clear();
			BMatrix2D Adjoint() const;
	
			bool	IsIdentity()const {return fIsIdentity;};
			bool	IsClear() const {return fIsClear;};
			bool	IsSingular() const;
			
			void	Print();
			
protected:

private:
	bool	fIsIdentity;
	bool	fIsClear;
};



// ARRAY ACCESS

inline double* BMatrix2D::operator [](int i)
{
  assert(i == 0 || i == 1 || i == 2 || i == 3);
  return &m_[i][0];
}

inline const double* BMatrix2D::operator [](int i) const
{
  assert(i == 0 || i == 1 || i == 2 || i == 3);
  return &m_[i][0];
}

#endif
