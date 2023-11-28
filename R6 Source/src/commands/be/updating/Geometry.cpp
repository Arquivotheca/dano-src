#include <math.h>
#include "Geometry.h"

// ***
// class BVector3D
// ***

BVector3D BVector3D::operator+( const BVector3D& v ) const
{
	BVector3D V;
	
	V.x = x + v.x;
	V.y = y + v.y;
	V.z = z + v.z;
	
	return V;
}

BVector3D BVector3D::operator-( const BVector3D& v ) const
{
	BVector3D V;
	
	V.x = x - v.x;
	V.y = y - v.y;
	V.z = z - v.z;
	
	return V;
}

BVector3D& BVector3D::operator+=( const BVector3D& v )
{
	x += v.x;
	y += v.y;
	z += v.z;
	
	return *this;
}

BVector3D& BVector3D::operator-=( const BVector3D& v )
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
	
	return *this;
}

bool BVector3D::operator!=( const BVector3D& v ) const
{
	return ((x != v.x)||(y != v.y)||(z != v.z));
}

bool BVector3D::operator==( const BVector3D& v ) const
{
	return((x == v.x)&&(y == v.y)&&(z == v.z) );
}

float BVector3D::Magnitude( void )
{
	return sqrt( x*x + y*y + z*z );
}

void BVector3D::Normalize( void )
{
	float	mag = Magnitude();
	
	x /= mag;
	y /= mag;
	z /= mag;
}

float BVector3D::DotProduct( const BVector3D &v )
{
	return (x * v.x) + (y * v.y) + (z * v.z);
}

void BVector3D::CrossProduct( const BVector3D &v, BVector3D &normal )
{
	normal.x = (y * v.z) - (z * v.y);
	normal.y = (z * v.x) - (x * v.z);
	normal.z = (x * v.y) - (y * v.x);
}

void BVector3D::Scale( float s )
{
	x *= s;
	y *= s;
	z *= s;
}

BPoint BVector3D::Project( float scale, BPoint origin )
{
	BPoint pt( ((x * scale) / z) + origin.x, ((y * scale) / z) + origin.y );
	return pt;
}

// ***
// class BVector2D
// ***

float BVector2D::Magnitude( void )
{
	return sqrt( x*x + y*y );
}

void BVector2D::Normalize( void )
{
	float	mag = Magnitude();
	
	x /= mag;
	y /= mag;
}

float BVector2D::DotProduct( const BVector2D &v )
{
	return (x * v.x) + (y * v.y);
}

void BVector2D::CrossProduct( const BVector2D &v, BVector3D &normal )
{
	normal.x = 0;
	normal.y = 0;
	normal.z = (x * v.y) - (y * v.x);
}

void BVector2D::Scale( float s )
{
	x *= s;
	y *= s;
}

// ***
// class BMatrix3D
// ***

/*BMatrix3D &BMatrix3D::operator=( const BMatrix3D &M )
{
	for( int32 i=0; i<3; i++ )
	{
		m[i][0] = M.m[i][0];
		m[i][1] = M.m[i][1];
		m[i][2] = M.m[i][2];
		m[i][3] = M.m[i][3];
	}
	return *this;
}*/

BVector3D BMatrix3D::Transform( const BVector3D &src )
{
	BVector3D dst;
	
	dst.x = m[0][3] + src.x*m[0][0] + src.y*m[0][1] + src.z*m[0][2];
	dst.y = m[1][3] + src.x*m[1][0] + src.y*m[1][1] + src.z*m[1][2];
	dst.z = m[2][3] + src.x*m[2][0] + src.y*m[2][1] + src.z*m[2][2];
	return dst;
}

BMatrix3D& BMatrix3D::operator*=( const BMatrix3D &b )
{
	*this = *this * b;
	return *this;
}

BMatrix3D BMatrix3D::operator*( const BMatrix3D &b ) const
{
	BMatrix3D M;
	
	for( int32 i = 0; i < 3; i++ )
	{
		M.m[i][0] = m[0][0] * b.m[i][0] +
					m[1][0] * b.m[i][1] +
					m[2][0] * b.m[i][2];
							
		M.m[i][1] = m[0][1] * b.m[i][0] +
					m[1][1] * b.m[i][1] +
					m[2][1] * b.m[i][2];
							
		M.m[i][2] = m[0][2] * b.m[i][0] +
					m[1][2] * b.m[i][1] +
					m[2][2] * b.m[i][2];
							
		M.m[i][3] = m[i][3] +
					m[0][3] * b.m[i][0] +
					m[1][3] * b.m[i][1] +
					m[2][3] * b.m[i][2];
	}
	
	return M;
}

BMatrix3D &BMatrix3D::Identity( void )
{
	m[0][1] = m[0][2] = m[0][3] = 0;
	m[1][0] = m[1][2] = m[1][3] = 0;
	m[2][0] = m[2][1] = m[2][3] = 0;
	m[0][0] = m[1][1] = m[2][2] = 1;
	return *this;
}

BMatrix3D &BMatrix3D::ReflectX( void )
{
	Identity();
	m[0][0] = -1;
	return *this;
}

BMatrix3D &BMatrix3D::ReflectY( void )
{
	Identity();
	m[1][1] = -1;
	return *this;
}

BMatrix3D &BMatrix3D::ReflectZ( void )
{
	Identity();
	m[2][2] = -1;
	return *this;
}

BMatrix3D &BMatrix3D::Scale( float sx, float sy, float sz )
{
	Identity();
	m[0][0] = sx;
	m[1][1] = sy;
	m[2][2] = sz;
	return *this;
}

BMatrix3D &BMatrix3D::Translation( float dx, float dy, float dz )
{
	Identity();
	m[0][3] = dx;
	m[1][3] = dy;
	m[2][3] = dz;
	return *this;
}

BMatrix3D &BMatrix3D::RotationX( float rx )
{
	Identity();
	float	cosx = cos( rx );
	float	sinx = sin( rx );
	
	m[1][1] = cosx;
	m[1][2] = -sinx;
	m[2][1] = sinx;
	m[2][2] = cosx;
	return *this;
}

BMatrix3D &BMatrix3D::RotationY( float ry )
{
	Identity();
	float	cosx = cos( ry );
	float	sinx = sin( ry );
	
	m[0][0] = cosx;
	m[0][2] = sinx;
	m[2][0] = -sinx;
	m[2][2] = cosx;
	return *this;
}

BMatrix3D &BMatrix3D::RotationZ( float rz )
{
	Identity();
	float	cosx = cos( rz );
	float	sinx = sin( rz );
	
	m[0][0] = cosx;
	m[0][1] = -sinx;
	m[1][0] = sinx;
	m[1][1] = cosx;
	return *this;
}

// ***
// class BMatrix2D
// ***

/*
BMatrix2D &BMatrix2D::operator=( const BMatrix2D &M )
{
	for( int32 i=0; i<2; i++ )
	{
		m[i][0] = M.m[i][0];
		m[i][1] = M.m[i][1];
		m[i][2] = M.m[i][2];
	}
	return *this;
}
*/
	
BPoint BMatrix2D::Transform( const BPoint &src )
{
	BPoint dst;
	
	dst.x = m[0][2] + src.x*m[0][0] + src.y*m[0][1];
	dst.y = m[1][2] + src.x*m[1][0] + src.y*m[1][1];
	return dst;
}

BMatrix2D& BMatrix2D::operator*=( const BMatrix2D &b )
{
	*this = *this * b;
	return *this;
}

BMatrix2D BMatrix2D::operator*( const BMatrix2D &b ) const
{
	BMatrix2D M;
	
	for( int32 i = 0; i < 2; i++ )
	{
		M.m[i][0] = m[0][0] * b.m[i][0] +
					m[1][0] * b.m[i][1];
							
		M.m[i][1] = m[0][1] * b.m[i][0] +
					m[1][1] * b.m[i][1];
							
		M.m[i][2] = b.m[i][2] +
					m[0][2] * b.m[i][0] +
					m[1][2] * b.m[i][1];
	}
	
	return M;
}

BMatrix2D &BMatrix2D::Identity( void )
{
	m[0][1] = m[0][2] = 0;
	m[1][0] = m[1][2] = 0;
	m[0][0] = m[1][1] = 1;
	return *this;
}

BMatrix2D &BMatrix2D::ReflectX( void )
{
	Identity();
	m[0][0] = -1;
	return *this;
}

BMatrix2D &BMatrix2D::ReflectY( void )
{
	Identity();
	m[1][1] = -1;
	return *this;
}

BMatrix2D &BMatrix2D::Scale( float sx, float sy )
{
	Identity();
	m[0][0] = sx;
	m[1][1] = sy;
	return *this;
}

BMatrix2D &BMatrix2D::Translation( float dx, float dy )
{
	Identity();
	m[0][2] = dx;
	m[1][2] = dy;
	return *this;
}

BMatrix2D &BMatrix2D::Rotation( float r )
{
	Identity();
	float	cosx = cos( r );
	float	sinx = sin( r );
	
	m[0][0] = cosx;
	m[0][1] = -sinx;
	m[1][0] = sinx;
	m[1][1] = cosx;
	return *this;
}

BMatrix2D &BMatrix2D::Inverse( void )
{
	
	return *this;
}

