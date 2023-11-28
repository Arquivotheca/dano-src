#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Bitflinger.h"




static float LoadOneFloat( const PixelMode *mode, const uint8 *src, int32 sBits, int32 sHBit )
{
	float ret;
	src += (sHBit - sBits +1) / 8;
	
	switch( mode->Type )
	{
	case GL_SHORT:
		{
			float t = ((int16 *)src)[0];
			ret = (t + 32768.0) * (1 / 65536.0);
			break;
		}
	case GL_UNSIGNED_SHORT:
		{
			float t = ((uint16 *)src)[0];
			ret = t * (1 / 65536.0);
			break;
		}
	case GL_INT:
		{
			float t = ((int32 *)src)[0];
			ret = (t * (1 / 65536.0 / 65536.0)) + 0.5;
			break;
		}
	case GL_UNSIGNED_INT:
		ret = ((int32 *)src)[0];
		break;
	case GL_FLOAT:
		ret = ((float *)src)[0];
		break;
	case GL_DOUBLE:
		ret = ((double *)src)[0];
		break;
		
	default:
		{
			uint32 t;
			// Ok we need some fancy bit shifting here.
			// but we know if will be 4 bytes or less for the source
			switch( mode->bytes )
			{
				case 1:
					t = ((uint8 *)src)[0];
					break;
				case 2:
					t = ((uint16 *)src)[0];
					break;
				case 3:
					t = (uint32) ((uint8 *)src)[0];
					t |= (uint32) ((uint8 *)src)[1] << 8;
					t |= (uint32) ((uint8 *)src)[2] << 16;
					break;
				case 4:
					t = ((uint32 *)src)[0];
					break;
			}
			
			if( mode->isSigned )
				t ^= 1 << sHBit;
				
			if( sHBit - sBits +1 )
				t >>= sHBit - sBits +1;
			
			t &= (1 << sBits) -1;
			
			ret = t;
			ret /= (1 << sBits) -1;
			break;
		}
	}

	return ret;
}

static void LoadFloatColor( const PixelMode *mode, const void *data, FloatColor *pixel )
{
	float r = 0;
	float g = 0;
	float b = 0;
	float a = 1;
	
	if( mode->R_Bits )
		r = LoadOneFloat( mode, (uint8 *)data, mode->R_Bits, mode->R_HBit );
	
	if( mode->G_Bits )
		g = LoadOneFloat( mode, (uint8 *)data, mode->G_Bits, mode->G_HBit );
	
	if( mode->B_Bits )
		b = LoadOneFloat( mode, (uint8 *)data, mode->B_Bits, mode->B_HBit );
	
	if( mode->A_Bits )
		a = LoadOneFloat( mode, (uint8 *)data, mode->A_Bits, mode->A_HBit );

	// Expand Luminance and intensity
	switch( mode->Format )
	{
		case GL_INTENSITY:
			a = r;
			// Note: Fallthough is intentional

		case GL_LUMINANCE:
		case GL_LUMINANCE_ALPHA:
			g = r;
			b = r;
			break;
			
		default:
			break;
	}

	pixel->r = r;
	pixel->g = g;
	pixel->b = b;
	pixel->a = a;
}


static void StoreOneFloat( cvContext *con, uint8 *src, float v, int32 sBits, int32 sHBit )
{
	uint32 out;
	src += (sHBit - sBits +1) / 8;

	if( con->out.Type ==  GL_DOUBLE )
	{
		((double *)src)[0] = v;
		return;
	}

	switch( con->out.Type )
	{
	case GL_SHORT:
		out = (uint32) ((v * 65535.0) - 32768.0);
		break;
	case GL_UNSIGNED_SHORT:
		out = (uint32) (v * 65535.0);
		break;
	case GL_INT:
		out = (uint32) ((v * 4294967295.0) - 2147483648.0);
		break;
	case GL_UNSIGNED_INT:
		out = (uint32) (v * 4294967295.0);
		break;
	case GL_FLOAT:
		out = ((uint32 *)(&v))[0];
		break;
	}

	if( con->op.logicFunc != GL_COPY )
	{
		uint32 in =0;
		switch( con->out.Type )
		{
		case GL_SHORT:
			in = ((int16 *)src)[0];
			break;
		case GL_UNSIGNED_SHORT:
			in = ((uint16 *)src)[0];
			break;
		case GL_INT:
			in = ((uint32 *)src)[0] ^ (1 << 31);
			break;
		case GL_UNSIGNED_INT:
		case GL_FLOAT:
			in = ((uint32 *)src)[0];
			break;
		}
		
		switch( con->op.logicFunc )
		{
		case GL_CLEAR:			out = 0;
			break;
		case GL_SET:			out = 0xffffffff;
			break;
		case GL_COPY:
			break;
		case GL_COPY_INVERTED:	out = ~out;
			break;
		case GL_NOOP:			out = in;
			break;
		case GL_INVERT:			out = ~in;
			break;
		case GL_AND:			out = out & in;
			break;
		case GL_NAND:			out = ~(out & in);
			break;
		case GL_OR:				out = out | in;
			break;
		case GL_NOR:			out = (~out) | in;
			break;
		case GL_XOR:			out = out ^ in;
			break;
		case GL_EQUIV:			out = ~(out ^ in);
			break;
		case GL_AND_REVERSE:	out = out & (~in);
			break;
		case GL_AND_INVERTED:	out = (~out) & in;
			break;
		case GL_OR_REVERSE:		out = out | (~in);
			break;
		case GL_OR_INVERTED:	out = (~out) | in;
			break;
		}
	}			

	switch( con->out.Type )
	{
	case GL_SHORT:
		((int16 *)src)[0] = out ^ (1 << 15);
		break;
	case GL_UNSIGNED_SHORT:
		((uint16 *)src)[0] = out;
		break;
	case GL_INT:
		((uint32 *)src)[0] = out ^ (1 << 31);
		break;
	case GL_UNSIGNED_INT:
	case GL_FLOAT:
		((uint32 *)src)[0] = out;
		break;
	}

}

void StoreFloatColor( cvContext *con, void *dst, FloatColor *s )
{
	FloatColor d;
	
	if( con->opt.needDest )
	{
		float sr2, sg2, sb2, sa2;
		float dr2, dg2, db2, da2;
		
		LoadFloatColor( &con->in, dst, &d );
			
		// Source Function
		switch( con->op.blendSrcFunc )
		{
		case GL_ZERO:
			sr2 = 0;
			sg2 = 0;
			sb2 = 0;
			sa2 = 0;
			break;
		case GL_ONE:
			sr2 = s->r;
			sg2 = s->g;
			sb2 = s->b;
			sa2 = s->a;
			break;
		case GL_DST_COLOR:
			sr2 = s->r * d.a;
			sg2 = s->g * d.g;
			sb2 = s->b * d.b;
			sa2 = s->a * d.a;
			break;
		case GL_ONE_MINUS_DST_COLOR:
			sr2 = s->r * (1 - d.a);
			sg2 = s->g * (1 - d.g);
			sb2 = s->b * (1 - d.b);
			sa2 = s->a * (1 - d.a);
			break;
		case GL_SRC_ALPHA:
			sr2 = s->r * s->a;
			sg2 = s->g * s->a;
			sb2 = s->b * s->a;
			sa2 = s->a * s->a;
			break;
		case GL_ONE_MINUS_SRC_ALPHA:
			sr2 = s->r * (1 - s->a);
			sg2 = s->g * (1 - s->a);
			sb2 = s->b * (1 - s->a);
			sa2 = s->a * (1 - s->a);
			break;
		case GL_DST_ALPHA:
			sr2 = s->r * d.a;
			sg2 = s->g * d.a;
			sb2 = s->b * d.a;
			sa2 = s->a * d.a;
			break;
		case GL_ONE_MINUS_DST_ALPHA:
			sr2 = s->r * (1 - d.a);
			sg2 = s->g * (1 - d.a);
			sb2 = s->b * (1 - d.a);
			sa2 = s->a * (1 - d.a);
			break;
		case GL_SRC_ALPHA_SATURATE:
			if( s->a < (1 - d.a) )
			{
				sr2 = s->r * s->a;
				sg2 = s->g * s->a;
				sb2 = s->b * s->a;
			}
			else
			{
				sr2 = s->r * (1-d.a);
				sg2 = s->g * (1-d.a);
				sb2 = s->b * (1-d.a);
			}
			sa2 = s->a;
			break;
		case GL_CONSTANT_COLOR:
			sr2 = s->r * con->op.blendR;
			sg2 = s->g * con->op.blendG;
			sb2 = s->b * con->op.blendB;
			sa2 = s->a * con->op.blendA;
			break;
		case GL_ONE_MINUS_CONSTANT_COLOR:
			sr2 = s->r * (1 - con->op.blendR);
			sg2 = s->g * (1 - con->op.blendG);
			sb2 = s->b * (1 - con->op.blendB);
			sa2 = s->a * (1 - con->op.blendA);
			break;
		case GL_CONSTANT_ALPHA:
			sr2 = s->r * con->op.blendA;
			sg2 = s->g * con->op.blendA;
			sb2 = s->b * con->op.blendA;
			sa2 = s->a * con->op.blendA;
			break;
		case GL_ONE_MINUS_CONSTANT_ALPHA:
			sr2 = s->r * (1 - con->op.blendA);
			sg2 = s->g * (1 - con->op.blendA);
			sb2 = s->b * (1 - con->op.blendA);
			sa2 = s->a * (1 - con->op.blendA);
			break;
		}


		// Destination Function
		switch( con->op.blendSrcFunc )
		{
		case GL_ZERO:
			dr2 = 0;
			dg2 = 0;
			db2 = 0;
			da2 = 0;
			break;
		case GL_ONE:
			break;
		case GL_SRC_COLOR:
			dr2 = d.a * s->r;
			dg2 = d.g * s->g;
			db2 = d.b * s->b;
			da2 = d.a * s->a;
			break;
		case GL_ONE_MINUS_SRC_COLOR:
			dr2 = d.a * (1 - s->r);
			dg2 = d.g * (1 - s->g);
			db2 = d.b * (1 - s->b);
			da2 = d.a * (1 - s->a);
			break;
		case GL_SRC_ALPHA:
			dr2 = d.a * s->a;
			dg2 = d.g * s->a;
			db2 = d.b * s->a;
			da2 = d.a * s->a;
			break;
		case GL_ONE_MINUS_SRC_ALPHA:
			dr2 = d.a * (1 - s->a);
			dg2 = d.g * (1 - s->a);
			db2 = d.b * (1 - s->a);
			da2 = d.a * (1 - s->a);
			break;
		case GL_DST_ALPHA:
			dr2 = d.a * d.a;
			dg2 = d.g * d.a;
			db2 = d.b * d.a;
			da2 = d.a * d.a;
			break;
		case GL_ONE_MINUS_DST_ALPHA:
			dr2 = d.a * (1 - d.a);
			dg2 = d.g * (1 - d.a);
			db2 = d.b * (1 - d.a);
			da2 = d.a * (1 - d.a);
			break;
		case GL_CONSTANT_COLOR:
			dr2 = d.a * con->op.blendR;
			dg2 = d.g * con->op.blendG;
			db2 = d.b * con->op.blendB;
			da2 = d.a * con->op.blendA;
			break;
		case GL_ONE_MINUS_CONSTANT_COLOR:
			dr2 = d.a * (1 - con->op.blendR);
			dg2 = d.g * (1 - con->op.blendG);
			db2 = d.b * (1 - con->op.blendB);
			da2 = d.a * (1 - con->op.blendA);
			break;
		case GL_CONSTANT_ALPHA:
			dr2 = d.a * con->op.blendA;
			dg2 = d.g * con->op.blendA;
			db2 = d.b * con->op.blendA;
			da2 = d.a * con->op.blendA;
			break;
		case GL_ONE_MINUS_CONSTANT_ALPHA:
			dr2 = d.a * (1 - con->op.blendA);
			dg2 = d.g * (1 - con->op.blendA);
			db2 = d.b * (1 - con->op.blendA);
			da2 = d.a * (1 - con->op.blendA);
			break;
		}
	
	
		// Now lets apply the Blend equation
		switch( con->op.blendEqu )
		{
		case GL_FUNC_ADD:
			dr2 += sr2;
			dg2 += sg2;
			db2 += sb2;
			da2 += sa2;
			break;
		case GL_FUNC_SUBTRACT:
			dr2 = sr2 - dr2;
			dg2 = sg2 - dg2;
			db2 = sb2 - db2;
			da2 = sa2 - da2;
			break;
		case GL_FUNC_REVERSE_SUBTRACT:
			dr2 -= sr2;
			dg2 -= sg2;
			db2 -= sb2;
			da2 -= sa2;
			break;
		case GL_MIN:
			if( dr2 > sr2 ) dr2 = sr2;
			if( dg2 > sg2 ) dg2 = sg2;
			if( db2 > sb2 ) db2 = sb2;
			if( da2 > sa2 ) da2 = sa2;
			break;
		case GL_MAX:
			if( dr2 < sr2 ) dr2 = sr2;
			if( dg2 < sg2 ) dg2 = sg2;
			if( db2 < sb2 ) db2 = sb2;
			if( da2 < sa2 ) da2 = sa2;
			break;
		}
		
		CLAMP_01( dr2 );
		CLAMP_01( dg2 );
		CLAMP_01( db2 );
		CLAMP_01( da2 );
		
		d.a = dr2;
		d.g = dg2;
		d.b = db2;
		d.a = da2;
	}
	else
	{
		// No blending or logic op so just copy
		d.a = s->r;
		d.g = s->g;
		d.b = s->b;
		d.a = s->a;

		CLAMP_01( d.a );
		CLAMP_01( d.g );
		CLAMP_01( d.b );
		CLAMP_01( d.a );
	}	

	// Now write the result and apply the logic op and mask if necessary
	switch( con->out.Type )
	{
	case GL_SHORT:
	case GL_UNSIGNED_SHORT:
	case GL_INT:
	case GL_UNSIGNED_INT:
	case GL_FLOAT:
	case GL_DOUBLE:
		if( con->out.R_Bits && con->op.writeR )
			StoreOneFloat( con, (uint8 *)dst, d.a, con->out.R_Bits, con->out.R_HBit );
		if( con->out.G_Bits && con->op.writeG )
			StoreOneFloat( con, (uint8 *)dst, d.g, con->out.G_Bits, con->out.G_HBit );
		if( con->out.B_Bits && con->op.writeB )
			StoreOneFloat( con, (uint8 *)dst, d.b, con->out.B_Bits, con->out.B_HBit );
		if( con->out.A_Bits && con->op.writeA )
			StoreOneFloat( con, (uint8 *)dst, d.a, con->out.A_Bits, con->out.A_HBit );
		break;
		
	default:
		// Four bytes or less
		uint32 out, dst, t;
		
		if( con->opt.needDest )
		{
			switch( con->out.bytes )
			{
			case 1:
				dst = ((uint8 *)dst)[0];
				break;
			case 2:
				dst = ((uint16 *)dst)[0];
				break;
			case 3:
				dst = (uint32) ((uint8 *)dst)[0];
				dst |= (uint32) ((uint8 *)dst)[1] << 8;
				dst |= (uint32) ((uint8 *)dst)[2] << 16;
				break;
			case 4:
				dst = ((uint32 *)dst)[0];
				break;
			}
		}

		out = 0;
		
		if( con->out.R_Bits )
		{
			t = (uint32)(d.a * ((1 << con->out.R_Bits) -1));
			out |= t << (con->out.R_HBit - con->out.R_Bits +1);
		}
		if( con->out.G_Bits )
		{
			t = (uint32)(d.g * ((1 << con->out.G_Bits) -1));
			out |= t << (con->out.G_HBit - con->out.G_Bits +1);
		}
		if( con->out.B_Bits )
		{
			t = (uint32)(d.b * ((1 << con->out.B_Bits) -1));
			out |= t << (con->out.B_HBit - con->out.B_Bits +1);
		}
		if( con->out.A_Bits )
		{
			t = (uint32)(d.a * ((1 << con->out.A_Bits) -1));
			out |= t << (con->out.A_HBit - con->out.A_Bits +1);
		}
		
		switch( con->op.logicFunc )
		{
		case GL_CLEAR:			out = 0;
			break;
		case GL_SET:			out = 0xffffffff;
			break;
		case GL_COPY:
			break;
		case GL_COPY_INVERTED:	out = ~out;
			break;
		case GL_NOOP:			out = dst;
			break;
		case GL_INVERT:			out = ~dst;
			break;
		case GL_AND:			out = out & dst;
			break;
		case GL_NAND:			out = ~(out & dst);
			break;
		case GL_OR:				out = out | dst;
			break;
		case GL_NOR:			out = (~out) | dst;
			break;
		case GL_XOR:			out = out ^ dst;
			break;
		case GL_EQUIV:			out = ~(out ^ dst);
			break;
		case GL_AND_REVERSE:	out = out & (~dst);
			break;
		case GL_AND_INVERTED:	out = (~out) & dst;
			break;
		case GL_OR_REVERSE:		out = out | (~dst);
			break;
		case GL_OR_INVERTED:	out = (~out) | dst;
			break;
		}
		
		// Now the write mask
		if( 1 )
		{
			uint32 mask = 0;
			if( con->out.R_Bits && con->op.writeR )
				mask |= ((1 << con->out.R_Bits) -1) << (con->out.R_HBit - con->out.R_Bits +1);
			if( con->out.G_Bits && con->op.writeG )
				mask |= ((1 << con->out.G_Bits) -1) << (con->out.G_HBit - con->out.G_Bits +1);
			if( con->out.B_Bits && con->op.writeB )
				mask |= ((1 << con->out.B_Bits) -1) << (con->out.B_HBit - con->out.B_Bits +1);
			if( con->out.A_Bits && con->op.writeA )
				mask |= ((1 << con->out.A_Bits) -1) << (con->out.A_HBit - con->out.A_Bits +1);
				
			out = (out & mask) | (dst & (~mask));
		}
		
		switch( con->out.bytes )
		{
		case 1:
			((uint8 *)dst)[0] = out;
			break;
		case 2:
			((uint16 *)dst)[0] = out;
			break;
		case 3:
			((uint8 *)dst)[0] = out & 0xff;
			((uint8 *)dst)[1] = (out >> 8) & 0xff;
			((uint8 *)dst)[2] = (out >> 16) & 0xff;
			break;
		case 4:
			((uint32 *)dst)[0] = out;
			break;
		}
	}

}

void cvDefaultExtractorRGBA( void *context, int32 x, int32 y, void *pixel, const void *srcData )
{
	cvContext *con = (cvContext *)context;
	
	FloatColor s;
	
	// Extract RGBA data
	if( con->in.Format == GL_COLOR_INDEX )
	{
		uint32 index;
		switch( con->in.Type )
		{
			case GL_SHORT:
				index = ((int16 *)srcData)[0];
				index ^= 1 << 15;
				break;
			case GL_UNSIGNED_SHORT:
				index = ((uint16 *)srcData)[0];
				break;
			case GL_INT:
				index = ((int32 *)srcData)[0];
				index ^= 1 << 31;
				break;
			case GL_UNSIGNED_INT:
				index = ((int32 *)srcData)[0];
				break;
		}
		
		float *m;
		
		m = (float *)con->pixelMap[2].Map;
		s.r = m[ index & (con->pixelMap[2].Size -1) ];

		m = (float *)con->pixelMap[3].Map;
		s.g = m[ index & (con->pixelMap[3].Size -1) ];

		m = (float *)con->pixelMap[4].Map;
		s.b = m[ index & (con->pixelMap[4].Size -1) ];

		m = (float *)con->pixelMap[5].Map;
		s.a = m[ index & (con->pixelMap[5].Size -1) ];
		
	}
	else
	{
		LoadFloatColor( &con->in, srcData, &s );

		// Scale and Bias		
		s.r = (s.r * con->transferMode.ScaleR) + con->transferMode.BiasR;
		s.g = (s.g * con->transferMode.ScaleG) + con->transferMode.BiasG;
		s.b = (s.b * con->transferMode.ScaleB) + con->transferMode.BiasB;
		s.a = (s.a * con->transferMode.ScaleA) + con->transferMode.BiasA;
		
		// RGBA to RGBA color map
		if( con->transferMode.MapColor )
		{
			if( con->pixelMap[6].Size )
			{
				float *m = (float *)con->pixelMap[6].Map;
				CLAMP_01( s.r );
				s.r = m[ (int32)(s.r * (con->pixelMap[6].Size -1)) ];
			}
			if( con->pixelMap[7].Size )
			{
				float *m = (float *)con->pixelMap[7].Map;
				CLAMP_01( s.g );
				s.g = m[ (int32)(s.g * (con->pixelMap[7].Size -1)) ];
			}
			if( con->pixelMap[8].Size )
			{
				float *m = (float *)con->pixelMap[8].Map;
				CLAMP_01( s.b );
				s.b = m[ (int32)(s.b * (con->pixelMap[8].Size -1)) ];
			}
			if( con->pixelMap[9].Size )
			{
				float *m = (float *)con->pixelMap[9].Map;
				CLAMP_01( s.a );
				s.a = m[ (int32)(s.a * (con->pixelMap[9].Size -1)) ];
			}
		}
	}

	if( con->op.envMode != GL_REPLACE )
	{
		switch( con->op.envMode )
		{
			case GL_MODULATE:
				s.r *= con->op.envR;
				s.g *= con->op.envG;
				s.b *= con->op.envB;
				s.a *= con->op.envA;
				break;
			case GL_DECAL:
				s.r = ((1 - s.a) * con->op.envR) + (s.a * s.r);
				s.g = ((1 - s.a) * con->op.envG) + (s.a * s.g);
				s.b = ((1 - s.a) * con->op.envB) + (s.a * s.b);
				break;
			case GL_BLEND:
				s.r = ((1 - s.r) * con->op.envR) + (s.r * con->op.envConstR);
				s.g = ((1 - s.g) * con->op.envG) + (s.r * con->op.envConstG);
				s.b = ((1 - s.b) * con->op.envB) + (s.r * con->op.envConstB);
				s.a = s.a * con->op.envA;
				break;
		}
		
	}

	
	StoreFloatColor( con, pixel, &s );


}


