#include "Transform2D.h"
#include <math.h>
#include <Point.h>
#include <Rect.h>
#include <stdio.h>

Transform2D::Transform2D(void)
{
	m[0][0] = 1; // a
	m[0][1] = 0; // b
	m[0][2] = 0; // g
	m[1][0] = 0; // c
	m[1][1] = 1; // d
	m[1][2] = 0; // h
	m[2][0] = 0; // e
	m[2][1] = 0; // f
	m[2][2] = 1; // i
}


Transform2D::Transform2D(float A, float B, float C, float D, float E, float F)
{
	m[0][0] = A; // a
	m[0][1] = B; // b
	m[0][2] = 0; // g
	m[1][0] = C; // c
	m[1][1] = D; // d
	m[1][2] = 0; // h
	m[2][0] = E; // e
	m[2][1] = F; // f
	m[2][2] = 1; // i
}

Transform2D &
Transform2D::Set(float A, float B, float C, float D, float E, float F)
{
	m[0][0] = A; // a
	m[0][1] = B; // b
	m[0][2] = 0; // g
	m[1][0] = C; // c
	m[1][1] = D; // d
	m[1][2] = 0; // h
	m[2][0] = E; // e
	m[2][1] = F; // f
	m[2][2] = 1; // i
	return *this;
}

float 
Transform2D::A() const
{
	return m[0][0];
}

float 
Transform2D::B() const
{
	return 	m[0][1];
}

float 
Transform2D::C() const
{
	return m[1][0];
}

float 
Transform2D::D() const
{
	return m[1][1];
}

float 
Transform2D::E() const
{
	return m[2][0];
}

float 
Transform2D::F() const
{
	return 	m[2][1];
}

float 
Transform2D::G() const
{
	return m[0][2];
}

float 
Transform2D::H() const
{
	return m[1][2];
}

float 
Transform2D::I() const
{
	return m[2][2];
}



void
Transform2D::Transform(BPoint *list, uint32 count) const
{
	// transform each of the points, in place
	float x, y;
	BPoint *p = list;
	//printf("CTM [ %f %f %f %f %f %f ]\n", a, b, c, d, e, f);
	for (uint32 i = 0; i < count; i++)
	{
		x = p->x; y = p->y;
		p->x = m[0][0] * x + m[1][0] * y + m[2][0];
		p->y = m[0][1] * x + m[1][1] * y + m[2][1];
		p++;
	}
}

Transform2D &
Transform2D::operator*=(const Transform2D &rhs)
{
	float ta = m[0][0], tb = m[0][1];
	float tc = m[1][0], td = m[1][1];
	float te = m[2][0], tf = m[2][1];

	/* this = rhs * this */
	m[0][0] = rhs.m[0][0] * ta + rhs.m[0][1] * tc /* + G * te */;
	m[0][1] = rhs.m[0][0] * tb + rhs.m[0][1] * td /* + G * tf */;
	/* g */
	m[1][0] = rhs.m[1][0] * ta + rhs.m[1][1] * tc /* + H * te */;
	m[1][1] = rhs.m[1][0] * tb + rhs.m[1][1] * td /* + H * tf */;
	/* h */
	m[2][0] = rhs.m[2][0] * ta + rhs.m[2][1] * tc + /* 1 * */ te;
	m[2][1] = rhs.m[2][0] * tb + rhs.m[2][1] * td + /* 1 * */ tf;
	/* i */
	return *this;
}

Transform2D &
Transform2D::MultiplyBy(const Transform2D &rhs)
{
	float ta = m[0][0], tb = m[0][1];
	float tc = m[1][0], td = m[1][1];
	float te = m[2][0], tf = m[2][1];

	m[0][0] = ta * rhs.m[0][0] + tb * rhs.m[1][0];
	m[0][1] = ta * rhs.m[0][1] + tb * rhs.m[1][1];
	m[1][0] = tc * rhs.m[0][0] + td * rhs.m[1][0];
	m[1][1] = tc * rhs.m[0][1] + td * rhs.m[1][1];
	m[2][0] = te * rhs.m[0][0] + tf * rhs.m[1][0] + rhs.m[2][0];
	m[2][1] = te * rhs.m[0][1] + tf * rhs.m[1][1] + rhs.m[2][1];
	
	return *this;
}

Transform2D &
Transform2D::Rotate(float deg)
{
	deg *= PI;
	deg /= 180;
	float cf = cosf(deg);
	float sf = sinf(deg);
	return *this *= Transform2D(cf, sf, -sf, cf, 0, 0);
}

Transform2D &
Transform2D::Translate(float x, float y)
{
	return *this *= Transform2D(1, 0, 0, 1, x, y);
}

Transform2D &
Transform2D::Scale(float x, float y)
{
	return *this *= Transform2D(x, 0, 0, y, 0, 0);
}

Transform2D &
Transform2D::Skew(float degx, float degy)
{
	return *this *= Transform2D(1, tanf(degx * PI / 180), tanf(degy * PI / 180), 1, 0, 0);
}

void 
Transform2D::PrintToStream(int level, FILE *fd)
{
	fprintf(fd, "%*s[\n", level, "");
	for (uint i = 0; i < 3; i++)
		fprintf(fd, "%*s %f %f %f\n", level, "", m[i][0], m[i][1], m[i][2]);
	fprintf(fd, "%*s]\n", level, "");
}

Transform2D &
Transform2D::Transpose(void)
{
	float tmp;
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			if (i != j)
			{
				tmp = m[i][j];
				m[i][j] = m[j][i];
				m[j][i] = tmp;
			}
	return *this;
}

Transform2D &
Transform2D::operator*=(float f)
{
	for (uint i = 0; i < 3; i++)
		for (uint j = 0; j < 3; j++)
			m[i][j] *= f;
	return *this;
}


float 
Transform2D::Determinant(void)
{
	return
		m[0][0] * ((m[1][1] * m[2][2]) - (m[2][1] * m[1][2])) -
		m[0][1] * ((m[1][0] * m[2][2]) - (m[2][0] * m[1][2])) +
		m[0][2] * ((m[1][0] * m[2][1]) - (m[2][0] * m[1][1]));
}


float 
Transform2D::Cofactor(uint r, uint c)
{
	float d[2][2];
	uint rr = 0, cc = 0;
	//printf("Cofactor(%d,%d)\n", r, c);
	for (uint i = 0; i < 3; i++)
	{
		if (i != r)
		{
			cc = 0;
			for (uint j = 0; j < 3; j++)
			{
				if (j != c)
				{
					d[rr][cc] = m[i][j];
					//printf("d[%d,%d] = m[%d,%d] = %f\n", rr, cc, i, j, m[i][j]);
					cc++;
				}
			}
			rr++;
		}
	}
	float cf = (d[0][0] * d[1][1]) - (d[0][1] * d[1][0]);
	if ((r+c) & 0x01) cf *= -1;
	//printf("Cofactor is %f\n", cf);
	return cf;
}

Transform2D &
Transform2D::Invert(void)
{
	Transform2D m2(*this);

	// make *this the pre-transposed cofactor matrix
	for (uint i = 0; i < 3; i++)
		for (uint j = 0; j < 3; j++)
			m[j][i] = m2.Cofactor(i,j);
	// return us, inverted
	return *this /= m2.Determinant();
}

Transform2D &
Transform2D::RoundBy(float factor)
{
	for (uint i = 0; i < 3; i++)
		for (uint j = 0; j < 3; j++)
			m[j][i] = (int)(m[j][i] * factor) / factor;
	return *this;
}

/*

		a		b		g
		c		d		h
		e		f		i
	
		Aa		Ab		Ag
A B G	Bc		Bd		Bh
		Ge		Gf		Gi

		Ca		Cb		Cg
C D H	Dc		Dd		Dh
		He		Hf		Hi

		Ea		Eb		Eg
E F I	Fc		Fd		Fh
		Ie		If		Ii


*/

BRect 
Transform2D::TransformedBounds(const BRect &rect) const
{
	BPoint r[4];
	r[0] = rect.LeftTop();
	r[1] = rect.RightTop();
	r[2] = rect.RightBottom();
	r[3] = rect.LeftBottom();
	// transformed
	Transform(r, 4);
	// determine the boundaries
	BPoint tl(FLT_MAX, FLT_MAX), br(-FLT_MAX, -FLT_MAX);
	for (int i = 0; i < 4; i++)
	{
		//printf("[%d] %f,%f\n", i, r[i].x, r[i].y);
		if (r[i].x < tl.x) tl.x = r[i].x;
		if (r[i].y < tl.y) tl.y = r[i].y;
		if (r[i].x > br.x) br.x = r[i].x;
		if (r[i].y > br.y) br.y = r[i].y;
	}
	return BRect(tl, br);
}

#ifdef TEST_TRANSFORM2D
int
main(int argc, char **argv)
{
#if 0
	float w = 100; //72 * 8.5;
	float h = 100; //72 * 11;
	Transform2D d;
	d.PrintToStream(); printf("\n");
	d.Translate(0,h);
	d.PrintToStream(); printf("\n");
	//d.Scale(1,-1);
	d.PrintToStream(); printf("\n\n");
	Transform2D m;
	float s = 10;
	m.Translate(w/2, h/2);
	m.PrintToStream(); printf("\n");
	m.Translate(-s/2, s/2);
	m.PrintToStream(); printf("\n");
#if 1
	m.Rotate(45);
	m.PrintToStream(); printf("\n");
#endif
#if 0
	m.Translate(s/2, -s/2);
	m.PrintToStream(); printf("\n");
#endif
#if 0
	m.Scale(s, s);
	m.PrintToStream(); printf("\n");
#endif
#if 1
	m.Scale(1, -1);
	m.PrintToStream(); printf("\n");
#endif

	//m *= d;
	m.PrintToStream(); printf("\n\n");

	Transform2D mi(m);
	mi.Invert();
	mi.PrintToStream(); printf("\n\n");

	BPoint p;
	BPoint r[4];
	r[0].Set(0,0);
	r[1].Set(s-1,0);
	r[2].Set(s-1,s-1);
	r[3].Set(0,s-1);
	m.Transform(r, 4);
	BPoint tl(100000, 100000), br(-10000, -10000);
	for (int i = 0; i < 4; i++)
	{
		printf("[%d] %f,%f\n", i, r[i].x, r[i].y);
		if (r[i].x < tl.x) tl.x = r[i].x;
		if (r[i].y < tl.y) tl.y = r[i].y;
		if (r[i].x > br.x) br.x = r[i].x;
		if (r[i].y > br.y) br.y = r[i].y;
	}
	printf("\n");
	printf("tl: %f,%f\n", tl.x, tl.y);
	printf("br: %f,%f\n", br.x, br.y);
	float ylimit = ceil(br.y);
	float xlimit = ceil(br.x);
	float ystart = floor(tl.y);
	float xstart = floor(tl.x);
	printf("tl: %f,%f br: %f,%f\n", xstart, ystart, xlimit, ylimit);
#else
	Transform2D scaleskew;
	Transform2D skewscale;
	scaleskew.Scale(2, 2);
	scaleskew.Rotate(10);
	skewscale.Rotate(10);
#if 0
	scaleskew.Skew(10, 22);
	skewscale.Skew(10, 22);
#endif
	skewscale.Scale(2, 2);
	printf("scale skew: "); scaleskew.PrintToStream(); printf("\n");
	printf("skew scale: "); skewscale.PrintToStream(); printf("\n");
	skewscale.Invert();
	printf("skew scale: "); skewscale.PrintToStream(); printf("\n");
#endif
	//return 0;
#if 0
	for (float y = ystart; y <= ylimit; y++)
	{
		for (float x = xstart; x <= xlimit; x++)
		{
			p.Set(x,y);
			m.UnTransform(&p, 1);
			if ((p.x < 0) || (p.y < 0))
			{
				printf("    ");
				continue;
				p.Set(0,0);
			}
			if ((p.x > 9) || (p.y > 9))
			{
				printf("    ");
				continue;
				p.Set(0,0);
			}
			//printf("%+05.3f,%+05.3f ", p.x, p.y);
			printf("%d,%d ", (int)p.x, (int)p.y);
		}
		printf("\n");
	}
#else
#if 0
	printf("   ");
	for (float x = xstart; x <= xlimit; x++)
		printf("   %2d   ", (int)x);
	printf("\n");
	for (float y = ylimit; y >= ystart; y--)
	{
		printf("%2d ", (int)y);
		for (float x = xstart; x <= xlimit; x++)
		{
			p.Set(x,y);
			mi.Transform(&p, 1);
#if 1
			if ((p.x < 0) || (p.y < 0))
			{
				printf("        ");
				continue;
				p.Set(0,0);
			}
			if ((p.x > s-1) || (p.y > s-1))
			{
				printf("        ");
				continue;
				p.Set(0,0);
			}
#endif
			//printf("%+05.3f,%+05.3f ", p.x, p.y);
			printf("%3d,%-3d ", (int)p.x, (int)p.y);
		}
		printf("\n");
	}
#endif
#endif
}
#endif
