#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <Point.h>

class BVector3D
{
	public:
		float x, y, z;
		
		BVector3D( void );
		BVector3D( float X, float Y, float Z );
		BVector3D( const BPoint &pt );
		BVector3D( const BVector3D &v );
		
		BVector3D &operator=( const BVector3D &from );
		BVector3D &operator=( const BPoint &from );
		BVector3D operator+( const BVector3D& ) const;
		BVector3D operator-( const BVector3D& ) const;
		BVector3D& operator+=( const BVector3D& );
		BVector3D& operator-=( const BVector3D& );

		bool operator!=( const BVector3D& ) const;
		bool operator==( const BVector3D& ) const;
		
		inline void Set( float X, float Y, float Z );
		void Scale( float s );
		float Magnitude( void );
		void Normalize( void );
		
		// The dot product = cos( theta ) between two vectors
		float DotProduct( const BVector3D &v );
		// The cross product = normal vector of plane defined by two vectors
		void CrossProduct( const BVector3D &v, BVector3D &normal );
		
		// Project 3D vector onto a 2D plane
		BPoint Project( float scale, BPoint origin );
		inline float ProjectX( float scale, float dx );
		inline float ProjectY( float scale, float dy );
};

class BVector2D : public BPoint
{
	public:
		BVector2D( void );
		BVector2D( float X, float Y );
		BVector2D( const BPoint &pt );
		BVector2D( const BVector2D &v );
		
		BVector2D &operator=( const BVector2D &from );
		BVector2D&operator=( const BPoint &from );
		
		void Scale( float s );
		float Magnitude( void );
		void Normalize( void );
		
		// The dot product = cos( theta ) between two vectors
		float DotProduct( const BVector2D &v );
		// The cross product = normal vector of plane defined by two vectors
		void CrossProduct( const BVector2D &v, BVector3D &normal );
};

class BMatrix3D
{
	public:
		float m[3][4];
		
		BMatrix3D( void );
		BMatrix3D( const BMatrix3D &M );
		
		BVector3D Transform( const BVector3D &v );
		
		BMatrix3D &operator*=( const BMatrix3D &b );
		BMatrix3D operator*( const BMatrix3D &b ) const;
		//BMatrix3D &operator=( const BMatrix3D &M );
		
		BMatrix3D &Identity( void );
		BMatrix3D &ReflectX( void );
		BMatrix3D &ReflectY( void );
		BMatrix3D &ReflectZ( void );
		BMatrix3D &Scale( float sx, float sy, float sz );
		BMatrix3D &Translation( float dx, float dy, float dz );
		BMatrix3D &RotationX( float rx );
		BMatrix3D &RotationY( float ry );
		BMatrix3D &RotationZ( float rz );
};

class BMatrix2D
{
	public:
		float m[2][3];
		
		BMatrix2D( void );
		BMatrix2D( const BMatrix2D &M );
		
		BPoint Transform( const BPoint &v );
		
		BMatrix2D &operator*=( const BMatrix2D &b );
		BMatrix2D operator*( const BMatrix2D &b ) const;
		//BMatrix2D &operator=( const BMatrix2D &M );
		
		BMatrix2D &Identity( void );
		BMatrix2D &ReflectX( void );
		BMatrix2D &ReflectY( void );
		BMatrix2D &Scale( float sx, float sy );
		BMatrix2D &Translation( float dx, float dy );
		BMatrix2D &Rotation( float r );
		
		BMatrix2D &Inverse( void );
};

/*----------------------------------------------------------------*/
/*----- inline definitions ---------------------------------------*/

inline BVector3D::BVector3D()
{
}

inline BVector3D::BVector3D( float X, float Y, float Z )
{
	x = X;
	y = Y;
	z = Z;
}

inline BVector3D::BVector3D( const BPoint &pt )
{
	x = pt.x;
	y = pt.y;
	z = 0;
}

inline BVector3D::BVector3D( const BVector3D &v )
{
	x = v.x;
	y = v.y;
	z = v.z;
}

inline BVector3D &BVector3D::operator=( const BVector3D &from )
{
	x = from.x;
	y = from.y;
	z = from.z;
	return *this;
}

inline BVector3D &BVector3D::operator=( const BPoint &from )
{
	x = from.x;
	y = from.y;
	z = 0;
	return *this;
}

inline void BVector3D::Set( float X, float Y, float Z )
{
	x = X;
	y = Y;
	z = Z;
}

inline float BVector3D::ProjectX( float scale, float dx )
{
	return (x * scale) / z + dx;
}

inline float BVector3D::ProjectY( float scale, float dy )
{
	return (y * scale) / z + dy;
}

inline BVector2D::BVector2D( void )
{
}

inline BVector2D::BVector2D( float X, float Y )
{
	x = X;
	y = Y;
}

inline BVector2D::BVector2D( const BPoint &pt )
{
	x = pt.x;
	y = pt.y;
}

inline BVector2D::BVector2D( const BVector2D &v )
{
	x = v.x;
	y = v.y;
}

inline BVector2D &BVector2D::operator=( const BVector2D &from )
{
	x = from.x;
	y = from.y;
	return *this;
}

inline BVector2D &BVector2D::operator=( const BPoint &from )
{
	x = from.x;
	y = from.y;
	return *this;
}

inline BMatrix3D::BMatrix3D( void )
{
}

inline BMatrix3D::BMatrix3D( const BMatrix3D &M )
{
	*this = M;
}

inline BMatrix2D::BMatrix2D( void )
{
}

inline BMatrix2D::BMatrix2D( const BMatrix2D &M )
{
	*this = M;
}

#endif
