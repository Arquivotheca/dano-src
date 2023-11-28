
#include "polyphase_int.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

#if 0
// tables moved to polyphase_int_asm.asm

// 0.15, scaled by 0.5
static const int16 syn_f_window_int[HAN_SIZE] =
{
	0x0000, 0x0001, 0x0007, 0x0019, 
	0x0035, 0xffdc, 0x0072, 0x0187, 
	0x01fd, 0x000b, 0x0508, 0x097f, 
	0x066b, 0x09bd, 0x249c, 0x3e84, 
	0x4947, 0xc17c, 0xdb64, 0xf643, 
	0x066b, 0xf681, 0xfaf8, 0xfff5, 
	0x01fd, 0xfe79, 0xff8e, 0x0024, 
	0x0035, 0xffe7, 0xfff9, 0xffff, 
	0x0000, 0x0006, 0x0007, 0x0034, 
	0x0036, 0x0064, 0x0081, 0x0203, 
	0x01f4, 0x04ad, 0x0563, 0x06f7, 
	0x05d1, 0x22ce, 0x266a, 0x493c, 
	0x493c, 0xd996, 0xdd32, 0x05d1, 
	0x06f7, 0xfa9d, 0xfb53, 0x01f4, 
	0x0203, 0xff7f, 0xff9c, 0x0036, 
	0x0034, 0xfff9, 0xfffa, 0x0000, 
	0x0000, 0x0005, 0x0008, 0x0032, 
	0x0037, 0x0056, 0x0091, 0x0208, 
	0x01e8, 0x0452, 0x05bd, 0x0776, 
	0x052a, 0x20ff, 0x2836, 0x491a, 
	0x491a, 0xd7ca, 0xdf01, 0x052a, 
	0x0776, 0xfa43, 0xfbae, 0x01e8, 
	0x0208, 0xff6f, 0xffaa, 0x0037, 
	0x0032, 0xfff8, 0xfffb, 0x0000, 
	0x0000, 0x0005, 0x0009, 0x0030, 
	0x0038, 0x0049, 0x00a1, 0x0209, 
	0x01d9, 0x03f7, 0x0617, 0x07e7, 
	0x0474, 0x1f32, 0x29ff, 0x48e1, 
	0x48e1, 0xd601, 0xe0ce, 0x0474, 
	0x07e7, 0xf9e9, 0xfc09, 0x01d9, 
	0x0209, 0xff5f, 0xffb7, 0x0038, 
	0x0030, 0xfff7, 0xfffb, 0x0000, 
	0x0000, 0x0004, 0x000a, 0x002f, 
	0x0038, 0x003c, 0x00b1, 0x0209, 
	0x01c7, 0x039e, 0x066f, 0x084b, 
	0x03b0, 0x1d68, 0x2bc5, 0x4892, 
	0x4892, 0xd43b, 0xe298, 0x03b0, 
	0x084b, 0xf991, 0xfc62, 0x01c7, 
	0x0209, 0xff4f, 0xffc4, 0x0038, 
	0x002f, 0xfff6, 0xfffc, 0x0000, 
	0x0000, 0x0004, 0x000b, 0x002d, 
	0x0038, 0x0031, 0x00c2, 0x0206, 
	0x01b2, 0x0345, 0x06c5, 0x08a2, 
	0x02dd, 0x1ba0, 0x2d86, 0x482d, 
	0x482d, 0xd27a, 0xe460, 0x02dd, 
	0x08a2, 0xf93b, 0xfcbb, 0x01b2, 
	0x0206, 0xff3e, 0xffcf, 0x0038, 
	0x002d, 0xfff5, 0xfffc, 0x0000, 
	0x0000, 0x0003, 0x000c, 0x002c, 
	0x0038, 0x0026, 0x00d3, 0x0202, 
	0x019b, 0x02ee, 0x0719, 0x08ec, 
	0x01fd, 0x19dc, 0x2f41, 0x47b1, 
	0x47b1, 0xd0bf, 0xe624, 0x01fd, 
	0x08ec, 0xf8e7, 0xfd12, 0x019b, 
	0x0202, 0xff2d, 0xffda, 0x0038, 
	0x002c, 0xfff4, 0xfffd, 0x0000, 
	0x0000, 0x0003, 0x000d, 0x002a, 
	0x0038, 0x001b, 0x00e5, 0x01fc, 
	0x017f, 0x0299, 0x076b, 0x092b, 
	0x010e, 0x181d, 0x30f6, 0x4720, 
	0x4720, 0xcf0a, 0xe7e3, 0x010e, 
	0x092b, 0xf895, 0xfd67, 0x017f, 
	0x01fc, 0xff1b, 0xffe5, 0x0038, 
	0x002a, 0xfff3, 0xfffd, 0x0000, 
	0x0000, 0x0003, 0x000e, 0x0028, 
	0x0038, 0x0011, 0x00f7, 0x01f4, 
	0x0161, 0x0246, 0x07b9, 0x095e, 
	0x0011, 0x1664, 0x32a3, 0x467a, 
	0x467a, 0xcd5d, 0xe99c, 0x0011, 
	0x095e, 0xf847, 0xfdba, 0x0161, 
	0x01f4, 0xff09, 0xffef, 0x0038, 
	0x0028, 0xfff2, 0xfffd, 0x0000, 
	0x0000, 0x0002, 0x000f, 0x0026, 
	0x0037, 0x0009, 0x0109, 0x01ea, 
	0x0140, 0x01f5, 0x0804, 0x0985, 
	0xff07, 0x14b1, 0x3447, 0x45bf, 
	0x45bf, 0xcbb9, 0xeb4f, 0xff07, 
	0x0985, 0xf7fc, 0xfe0b, 0x0140, 
	0x01ea, 0xfef7, 0xfff7, 0x0037, 
	0x0026, 0xfff1, 0xfffe, 0x0000, 
	0x0000, 0x0002, 0x0011, 0x0024, 
	0x0035, 0x0000, 0x011c, 0x01df, 
	0x011a, 0x01a6, 0x084a, 0x09a1, 
	0xfdee, 0x1305, 0x35e2, 0x44ef, 
	0x44ef, 0xca1e, 0xecfb, 0xfdee, 
	0x09a1, 0xf7b6, 0xfe5a, 0x011a, 
	0x01df, 0xfee4, 0x0000, 0x0035, 
	0x0024, 0xffef, 0xfffe, 0x0000, 
	0x0000, 0x0002, 0x0012, 0x0022, 
	0x0034, 0xfff9, 0x012e, 0x01d3, 
	0x00f2, 0x015b, 0x088c, 0x09b3, 
	0xfcc8, 0x1161, 0x3772, 0x440b, 
	0x440b, 0xc88e, 0xee9f, 0xfcc8, 
	0x09b3, 0xf774, 0xfea5, 0x00f2, 
	0x01d3, 0xfed2, 0x0007, 0x0034, 
	0x0022, 0xffee, 0xfffe, 0x0000, 
	0x0000, 0x0001, 0x0013, 0x0020, 
	0x0032, 0xfff2, 0x0140, 0x01c6, 
	0x00c6, 0x0111, 0x08c9, 0x09bb, 
	0xfb93, 0x0fc6, 0x38f7, 0x4315, 
	0x4315, 0xc709, 0xf03a, 0xfb93, 
	0x09bb, 0xf737, 0xfeef, 0x00c6, 
	0x01c6, 0xfec0, 0x000e, 0x0032, 
	0x0020, 0xffed, 0xffff, 0x0000, 
	0x0000, 0x0001, 0x0015, 0x001f, 
	0x002f, 0xffec, 0x0153, 0x01b7, 
	0x0097, 0x00cb, 0x0900, 0x09b9, 
	0xfa52, 0x0e35, 0x3a6f, 0x420b, 
	0x420b, 0xc591, 0xf1cb, 0xfa52, 
	0x09b9, 0xf700, 0xff35, 0x0097, 
	0x01b7, 0xfead, 0x0014, 0x002f, 
	0x001f, 0xffeb, 0xffff, 0x0000, 
	0x0000, 0x0001, 0x0016, 0x001d, 
	0x002c, 0xffe6, 0x0165, 0x01a8, 
	0x0064, 0x0088, 0x0932, 0x09af, 
	0xf904, 0x0cad, 0x3bda, 0x40ef, 
	0x40ef, 0xc426, 0xf353, 0xf904, 
	0x09af, 0xf6ce, 0xff78, 0x0064, 
	0x01a8, 0xfe9b, 0x001a, 0x002c, 
	0x001d, 0xffea, 0xffff, 0x0000, 
	0xffff, 0x0001, 0x0018, 0x001b, 
	0x0028, 0xffe1, 0x0176, 0x0198, 
	0x002e, 0x0047, 0x095c, 0x099b, 
	0xf7a9, 0x0b2f, 0x3d37, 0x3fc2, 
	0x3fc2, 0xc2c9, 0xf4d1, 0xf7a9, 
	0x099b, 0xf6a4, 0xffb9, 0x002e, 
	0x0198, 0xfe8a, 0x001f, 0x0028, 
	0x001b, 0xffe8, 0xffff, 0xffff, 
};

/*-------------------------------------------------------------------------*/

// 0.15, scaled by 0.5
static const int16 syn_f_window_short[HAN_SIZE-128] =
{
	0x0000, 0xffdc, 0x0072, 0x0187, 
	0x01fd, 0x000b, 0x0508, 0x097f, 
	0x066b, 0x09bd, 0x249c, 0x3e84, 
	0x4947, 0xc17c, 0xdb64, 0xf643, 
	0x066b, 0xf681, 0xfaf8, 0xfff5, 
	0x01fd, 0xfe79, 0xff8e, 0x0024, 
	0x0036, 0x0064, 0x0081, 0x0203, 
	0x01f3, 0x04ad, 0x0563, 0x06f7, 
	0x05d1, 0x22cd, 0x266a, 0x493c, 
	0x493c, 0xd996, 0xdd33, 0x05d1, 
	0x06f7, 0xfa9d, 0xfb53, 0x01f3, 
	0x0203, 0xff7f, 0xff9c, 0x0036, 
	0x0037, 0x0056, 0x0091, 0x0207, 
	0x01e7, 0x0452, 0x05bd, 0x0776, 
	0x052a, 0x20ff, 0x2836, 0x491a, 
	0x491a, 0xd7ca, 0xdf01, 0x052a, 
	0x0776, 0xfa43, 0xfbae, 0x01e7, 
	0x0207, 0xff6f, 0xffaa, 0x0037, 
	0x0038, 0x0049, 0x00a1, 0x0209, 
	0x01d9, 0x03f7, 0x0617, 0x07e7, 
	0x0474, 0x1f32, 0x29ff, 0x48e1, 
	0x48e1, 0xd601, 0xe0ce, 0x0474, 
	0x07e7, 0xf9e9, 0xfc09, 0x01d9, 
	0x0209, 0xff5f, 0xffb7, 0x0038, 
	0x0038, 0x003d, 0x00b1, 0x0209, 
	0x01c7, 0x039e, 0x066f, 0x084a, 
	0x03b0, 0x1d68, 0x2bc5, 0x4892, 
	0x4892, 0xd43b, 0xe298, 0x03b0, 
	0x084a, 0xf991, 0xfc62, 0x01c7, 
	0x0209, 0xff4f, 0xffc3, 0x0038, 
	0x0038, 0x0031, 0x00c2, 0x0206, 
	0x01b2, 0x0345, 0x06c5, 0x08a2, 
	0x02dd, 0x1ba0, 0x2d86, 0x482c, 
	0x482c, 0xd27a, 0xe460, 0x02dd, 
	0x08a2, 0xf93b, 0xfcbb, 0x01b2, 
	0x0206, 0xff3e, 0xffcf, 0x0038, 
	0x0038, 0x0026, 0x00d4, 0x0202, 
	0x019a, 0x02ee, 0x0719, 0x08ec, 
	0x01fd, 0x19dc, 0x2f41, 0x47b1, 
	0x47b1, 0xd0bf, 0xe624, 0x01fd, 
	0x08ec, 0xf8e7, 0xfd12, 0x019a, 
	0x0202, 0xff2c, 0xffda, 0x0038, 
	0x0038, 0x001b, 0x00e5, 0x01fc, 
	0x017f, 0x0299, 0x076b, 0x092b, 
	0x010e, 0x181d, 0x30f6, 0x4720, 
	0x4720, 0xcf0a, 0xe7e3, 0x010e, 
	0x092b, 0xf895, 0xfd67, 0x017f, 
	0x01fc, 0xff1b, 0xffe5, 0x0038, 
	0x0038, 0x0012, 0x00f7, 0x01f4, 
	0x0161, 0x0246, 0x07b9, 0x095e, 
	0x0011, 0x1664, 0x32a3, 0x467a, 
	0x467a, 0xcd5d, 0xe99c, 0x0011, 
	0x095e, 0xf847, 0xfdba, 0x0161, 
	0x01f4, 0xff09, 0xffee, 0x0038, 
	0x0037, 0x0008, 0x0109, 0x01ea, 
	0x013f, 0x01f5, 0x0804, 0x0985, 
	0xff07, 0x14b1, 0x3447, 0x45bf, 
	0x45bf, 0xcbb9, 0xeb4f, 0xff07, 
	0x0985, 0xf7fc, 0xfe0b, 0x013f, 
	0x01ea, 0xfef7, 0xfff8, 0x0037, 
	0x0035, 0x0000, 0x011c, 0x01df, 
	0x011a, 0x01a6, 0x084a, 0x09a1, 
	0xfdee, 0x1305, 0x35e2, 0x44ef, 
	0x44ef, 0xca1e, 0xecfb, 0xfdee, 
	0x09a1, 0xf7b6, 0xfe5a, 0x011a, 
	0x01df, 0xfee4, 0x0000, 0x0035, 
	0x0034, 0xfff9, 0x012e, 0x01d3, 
	0x00f2, 0x015a, 0x088c, 0x09b3, 
	0xfcc7, 0x1161, 0x3772, 0x440b, 
	0x440b, 0xc88e, 0xee9f, 0xfcc7, 
	0x09b3, 0xf774, 0xfea6, 0x00f2, 
	0x01d3, 0xfed2, 0x0007, 0x0034, 
	0x0031, 0xfff2, 0x0140, 0x01c6, 
	0x00c6, 0x0111, 0x08c9, 0x09bb, 
	0xfb93, 0x0fc6, 0x38f7, 0x4314, 
	0x4314, 0xc709, 0xf03a, 0xfb93, 
	0x09bb, 0xf737, 0xfeef, 0x00c6, 
	0x01c6, 0xfec0, 0x000e, 0x0031, 
	0x002f, 0xffec, 0x0153, 0x01b7, 
	0x0097, 0x00cb, 0x0900, 0x09b9, 
	0xfa52, 0x0e34, 0x3a6f, 0x420b, 
	0x420b, 0xc591, 0xf1cc, 0xfa52, 
	0x09b9, 0xf700, 0xff35, 0x0097, 
	0x01b7, 0xfead, 0x0014, 0x002f, 
	0x002c, 0xffe6, 0x0164, 0x01a8, 
	0x0064, 0x0088, 0x0931, 0x09ae, 
	0xf904, 0x0cad, 0x3bda, 0x40ef, 
	0x40ef, 0xc426, 0xf353, 0xf904, 
	0x09ae, 0xf6cf, 0xff78, 0x0064, 
	0x01a8, 0xfe9c, 0x001a, 0x002c, 
	0x0028, 0xffe1, 0x0176, 0x0198, 
	0x002e, 0x0048, 0x095c, 0x099b, 
	0xf7a9, 0x0b2f, 0x3d37, 0x3fc2, 
	0x3fc2, 0xc2c9, 0xf4d1, 0xf7a9, 
	0x099b, 0xf6a4, 0xffb8, 0x002e, 
	0x0198, 0xfe8a, 0x001f, 0x0028, 
};

/*-------------------------------------------------------------------------*/

// 0.15
static const int16 cost32_c0_int[] = 
{ 
//  0-10 unscaled
	0x4013, 0x40b3, 0x41fa, 0x43f9, 
	0x46cc, 0x4a9d, 0x4fae, 0x5660, 
	0x5f4c, 0x6b6f, 0x7c7d, 
	
//  11, 12 scaled by 1/2
	0x4ad8, 0x5efc, 
	
//  13, 14 scaled by 1/4
	0x41d9, 0x6d0b, 
	
//  15 scaled by 1/16
	0x5185,
};

/* ------------------------------------------------------------------------*/

// 0.15
static const int16 cost32_c1_int[] =
{
//  0-4 unscaled
	0x404f, 0x42e1, 0x4891, 0x52cb, 
	0x64e2, 
	
//  5, 6 scaled by 1/2	
	0x43e2, 0x6e3c, 

//  7 scaled by 1/8	
	0x519e, 
};

/* ------------------------------------------------------------------------*/

// 0.15
static const int16 cost32_c2_int[] =
{
//  0-2 unscaled
	0x4140, 0x4cf8, 0x7332, 
//  3 scaled by 1/4	
	0x5203, 
};

/* ------------------------------------------------------------------------*/

// 0.15 
static const int16 cost32_c3_int[] =
{
//  0 unscaled
	0x4545,
//  1 scaled by 1/2
	0x539e, 
};

/* ------------------------------------------------------------------------*/

// 0.15
static const int16 cost32_c4[] =
{
	0x5a82, 
};

#endif

/* ------------------------------------------------------------------------*/

void cost32(const int32 *vec,int16 *f_vec);

/*-------------------------------------------------------------------------*/

//-------------------------------------------------------------------------*
//
//                   C P o l y p h a s e
//
//-------------------------------------------------------------------------*

//-------------------------------------------------------------------------*
//   constructor
//-------------------------------------------------------------------------*

CPolyphaseInt::CPolyphaseInt (const MPEG_INFO &_info,
                        int              _qual,
                        int              _resl,
                        int              _downMix) : 
  info (_info),
  qual (_qual),
  resl (_resl),
  downMix (_downMix)
{
  Init() ;
}

CPolyphaseInt::~CPolyphaseInt()
{
}

//-------------------------------------------------------------------------*
//   Init
//-------------------------------------------------------------------------*

void CPolyphaseInt::Init(void)
{
  int i,j;

  memset(syn_buf, 0, sizeof(syn_buf));
  bufOffset = 64;
}

//-------------------------------------------------------------------------*
//   Apply (short)
//
// sample is ordered like so: [SSLIMIT][SBLIMIT][2]
//-------------------------------------------------------------------------*

extern "C" void CPolyphaseInt__cost32_s_asm(const int32 *in, int16 *out);
extern "C" void CPolyphaseInt__window_band_s_asm16( int bufOffset, int16 *syn_buf, int32 qual, int16 *out_samples );

short *CPolyphaseInt::Apply(INT_SPECTRUM &sample, short *pPcm, int frames)
{
	int nChannels    = (downMix ? 1:info.stereo);
	int nIncrement   = (16<<nChannels)>>(qual+resl);
	int fShortWindow = (downMix && (info.stereo==2)) ? 1 : 0;
	
	int j,k;
	
	for ( k=0; k<frames; k++ )
	{
		bufOffset = (bufOffset-64)&((HAN_SIZE-1)*2);
		
		CPolyphaseInt__cost32_s_asm(&sample[k*SBLIMIT*2], &syn_buf[bufOffset]);
//		cost32(&sample[k*SBLIMIT*2],   &(syn_buf[bufOffset]));
//		cost32(&sample[k*SBLIMIT*2+1], &(syn_buf[bufOffset+1]));

		CPolyphaseInt__window_band_s_asm16(bufOffset, syn_buf, qual, pPcm);
		if (nChannels == 1)
		{
			// pack to mono
			int32 a, b;
			for (a = 1, b = 2; a < nIncrement; a++, b += 2)
				pPcm[a] = pPcm[b];
		}	
		pPcm += nIncrement;
	}

	return pPcm;
}

//
// functions for sample clipping
// 

static inline short CLIP16(int32 fSample)
{
//  const int32 min = int32_to_fixed32(-fixed32_guard_domain);
//  const int32 max = int32_to_fixed32( fixed32_guard_domain-1);
//  if(fSample < min) fSample = min;
//  else if(fSample > max) fSample = max;
//  return short(fixed32_to_int32_round(fSample << fixed32_guard_bits));
	return fixed16_saturate(fSample >> 12);
}

static inline unsigned char CLIP8(int32 fSample)
{
  return (unsigned char) ( (CLIP16(fSample) >> 8) + 0x80 );
}


/*-------------------------------------------------------------------------*/

// C++ implementation stuff

#if 0

void CPolyphaseInt::window_band_m(int bufOffset,short *out_samples, int /* short_window */)
{
  const int16 *winPtr = syn_f_window_int;
  int32        sum1,sum2;
  int          i,j;

  /* sum 0 and sum 16, 8, 4 (full, half, quarter spectrum) */
  sum1 = sum2 = 0;

  for ( i=0; i<512; i+=64 )
    {
    sum1 = fixed32_add(
    	sum1,
    	fixed32_mul16(syn_buf[(bufOffset+i*2+32) & ((HAN_SIZE-1)*2)], winPtr[0]));
    sum2 = fixed32_add(
    	sum2,
    	fixed32_mul16(syn_buf[(bufOffset+i*2+64) & ((HAN_SIZE-1)*2)], winPtr[3]));
    sum1 = fixed32_add(
    	sum1,
    	fixed32_mul16(syn_buf[(bufOffset+i*2+96) & ((HAN_SIZE-1)*2)], winPtr[2]));
    winPtr += 4;
    }

  if ( 0 == resl )
    {
    // 16bit PCM
    out_samples[0]          = CLIP16(sum1);
    out_samples[16 >> qual] = CLIP16(sum2);
    }
  else
    {
    // 8bit PCM
    ((unsigned char*)out_samples)[0]          = CLIP8(sum1);
    ((unsigned char*)out_samples)[16 >> qual] = CLIP8(sum2);
    }

  /* sum 1-15, 1-7, 1-3 and 17-31, 9-15, 5-7 (full, half, quarter spectrum) */

  if ( 0 == resl )
    {
    // 16bit PCM
    for ( j=1; j<(16>>qual); j++ )
      {
      sum1 = sum2 = 0;
      
      winPtr += (1<<qual)*32 - 32;
      
      for ( i=0;i<512;i+=64 )
        {
        sum1 = fixed32_add(
        	sum1,
        	fixed32_mul16(syn_buf[(bufOffset+i*2+j*(2<<qual)+32) & ((HAN_SIZE-1)*2)], winPtr[0]));
        sum2 = fixed32_add(
        	sum2,
        	fixed32_mul16(syn_buf[(bufOffset+i*2+j*(2<<qual)+32) & ((HAN_SIZE-1)*2)], winPtr[1]));
        sum1 = fixed32_add(
        	sum1,
        	fixed32_mul16(syn_buf[(bufOffset+i*2+j*(2<<qual)+64) & ((HAN_SIZE-1)*2)], winPtr[2]));
        sum2 = fixed32_add(
        	sum2,
        	fixed32_mul16(syn_buf[(bufOffset+i*2+j*(2<<qual)+64) & ((HAN_SIZE-1)*2)], winPtr[3]));
        winPtr += 4;
        }
      
      out_samples[j]            = CLIP16(sum1);
      out_samples[(32>>qual)-j] = CLIP16(sum2);
      }
    }
  else
    {
    // 8 bit PCM
    for ( j=1; j<(16>>qual); j++ )
      {
      sum1 = sum2 = 0;
      
      winPtr += (1<<qual)*32 - 32;
      
      for ( i=0;i<512;i+=64 )
        {
        sum1 = fixed32_add(
        	sum1,
        	fixed32_mul16(syn_buf[(bufOffset+i*2+j*(2<<qual)+32) & ((HAN_SIZE-1)*2)], winPtr[0]));
        sum2 = fixed32_add(
        	sum2,
        	fixed32_mul16(syn_buf[(bufOffset+i*2+j*(2<<qual)+32) & ((HAN_SIZE-1)*2)], winPtr[1]));
        sum1 = fixed32_add(
        	sum1,
        	fixed32_mul16(syn_buf[(bufOffset+i*2+j*(2<<qual)+64) & ((HAN_SIZE-1)*2)], winPtr[2]));
        sum2 = fixed32_add(
        	sum2,
        	fixed32_mul16(syn_buf[(bufOffset+i*2+j*(2<<qual)+64) & ((HAN_SIZE-1)*2)], winPtr[3]));
        winPtr += 4;
        }
      
      ((unsigned char*)out_samples)[j]            = CLIP8(sum1);
      ((unsigned char*)out_samples)[(32>>qual)-j] = CLIP8(sum2);
      }
    }
}


// C++ implementation:

void CPolyphaseInt::window_band_s(int bufOffset,short *out_samples, int /* short_window */)
{
	const int16 *winPtr = syn_f_window_int;
	int16        sum1l,sum2l,sum1r,sum2r;
	int          i,j,bufPtr;

	const int upshift = 4;
	
	/* sum 0 and sum 16, 8, 4 (full, half, quarter spectrum) */
	sum1l = sum2l = sum1r = sum2r = 0;
	
	bufPtr = bufOffset;

	for ( i=0; i<512; i+=64 )
	{
		int16 t1, t2;
		t1 = fixed16_mul(syn_buf[bufPtr+32], winPtr[0]);
		sum1l = fixed16_add(sum1l, t1);
		t2 = fixed16_mul(syn_buf[bufPtr+33], winPtr[0]);
		sum1r = fixed16_add(sum1r, t2);
		
		bufPtr = (bufPtr+64)&((HAN_SIZE-1)*2);
		
		t1 = fixed16_mul(syn_buf[bufPtr+32], winPtr[2]);
		sum1l = fixed16_add(sum1l, t1);
		t2 = fixed16_mul(syn_buf[bufPtr+33], winPtr[2]);
		sum1r = fixed16_add(sum1r, t2);
		sum2l = fixed16_add(
			sum2l,
			fixed16_mul(syn_buf[bufPtr   ], winPtr[3]));
		sum2r = fixed16_add(
			sum2r,
			fixed16_mul(syn_buf[bufPtr+1 ], winPtr[3]));
		
		bufPtr = (bufPtr+64)&((HAN_SIZE-1)*2);
		
		winPtr+=4;
	}

	if ( 0 == resl )
	{
		// 16bit PCM
		out_samples[0]            = sum1l << upshift; //CLIP16(sum1l);
		out_samples[32>>qual]     = sum2l << upshift; //CLIP16(sum2l);
		out_samples[1]            = sum1r << upshift; //CLIP16(sum1r);
		out_samples[(32>>qual)+1] = sum2r << upshift; //CLIP16(sum2r);

		/* sum 1-15, 1-7, 1-3 and 17-31, 9-15, 5-7 (full, half, quarter spectrum) */
		// 16 bit PCM
		for ( j=1; j<(16>>qual); j++ )
		{
			sum1l = sum2l = sum1r = sum2r = 0;
			
			bufPtr  = bufOffset+j*(2<<qual);
			winPtr += (1<<qual)*32 - 32;

			for ( i=0; i<512; i+=64 )
			{
				sum1l = fixed16_add(
					sum1l,
					fixed16_mul(syn_buf[bufPtr+32], winPtr[0]));
				sum1r = fixed16_add(
					sum1r,
					fixed16_mul(syn_buf[bufPtr+33], winPtr[0]));
				sum2l = fixed16_add(
					sum2l,
					fixed16_mul(syn_buf[bufPtr+32], winPtr[1]));
				sum2r = fixed16_add(
					sum2r,
					fixed16_mul(syn_buf[bufPtr+33], winPtr[1]));
				
				bufPtr = (bufPtr+64)&((HAN_SIZE-1)*2);
				
				sum1l = fixed16_add(
					sum1l,
					fixed16_mul(syn_buf[bufPtr   ], winPtr[2]));
				sum1r = fixed16_add(
					sum1r,
					fixed16_mul(syn_buf[bufPtr+1 ], winPtr[2]));
				sum2l = fixed16_add(
					sum2l,
					fixed16_mul(syn_buf[bufPtr   ], winPtr[3]));
				sum2r = fixed16_add(
					sum2r,
					fixed16_mul(syn_buf[bufPtr+1 ], winPtr[3]));
				
				bufPtr = (bufPtr+64)&((HAN_SIZE-1)*2);
				
				winPtr += 4;
			}
			
			out_samples[j*2]                = sum1l << upshift; //CLIP16(sum1l);
			out_samples[((32>>qual)-j)*2]   = sum2l << upshift; //CLIP16(sum2l);
			out_samples[j*2+1]              = sum1r << upshift; //CLIP16(sum1r);
			out_samples[((32>>qual)-j)*2+1] = sum2r << upshift; //CLIP16(sum2r);
		}
	}
	else
	{
		// 8bit PCM
		((unsigned char*)out_samples)[0]            = CLIP8(sum1l);
		((unsigned char*)out_samples)[32>>qual]     = CLIP8(sum2l);
		((unsigned char*)out_samples)[1]            = CLIP8(sum1r);
		((unsigned char*)out_samples)[(32>>qual)+1] = CLIP8(sum2r);
		
		// 8bit PCM
		for ( j=1; j<(16>>qual); j++ )
		{
			sum1l = sum2l = sum1r = sum2r = 0;
			
			bufPtr  = bufOffset+j*(2<<qual);
			winPtr += (1<<qual)*32 - 32;
			
			for ( i=0; i<512; i+=64 )
			{
				sum1l = fixed16_add(
					sum1l,
					fixed16_mul(syn_buf[bufPtr+32], winPtr[0]));
				sum1r = fixed16_add(
					sum1r,
					fixed16_mul(syn_buf[bufPtr+33], winPtr[0]));
				sum2l = fixed16_add(
					sum2l,
					fixed16_mul(syn_buf[bufPtr+32], winPtr[1]));
				sum2r = fixed16_add(
					sum2r,
					fixed16_mul(syn_buf[bufPtr+33], winPtr[1]));
				
				bufPtr = (bufPtr+64)&((HAN_SIZE-1)*2);
				
				sum1l = fixed16_add(
					sum1l,
					fixed16_mul(syn_buf[bufPtr   ], winPtr[2]));
				sum1r = fixed16_add(
					sum1r,
					fixed16_mul(syn_buf[bufPtr+1 ], winPtr[2]));
				sum2l = fixed16_add(
					sum2l,
					fixed16_mul(syn_buf[bufPtr   ], winPtr[3]));
				sum2r = fixed16_add(
					sum2r,
					fixed16_mul(syn_buf[bufPtr+1 ], winPtr[3]));
				
				bufPtr = (bufPtr+64)&((HAN_SIZE-1)*2);
				
				winPtr += 4;
			}
			
			((unsigned char*)out_samples)[j*2]                = CLIP8(sum1l);
			((unsigned char*)out_samples)[((32>>qual)-j)*2]   = CLIP8(sum2l);
			((unsigned char*)out_samples)[j*2+1]              = CLIP8(sum1r);
			((unsigned char*)out_samples)[((32>>qual)-j)*2+1] = CLIP8(sum2r);
		}
	}
}

/*-------------------------------------------------------------------------*/

// C++ implemenation of 16-bit cost32 transform

void cost32(const int32 *vec,int16 *f_vec)
{
  int16 tmp0_0,tmp0_1,tmp0_2,tmp0_3,tmp0_4,tmp0_5,tmp0_6,tmp0_7;
  int16 tmp0_8,tmp0_9,tmp0_10,tmp0_11,tmp0_12,tmp0_13,tmp0_14,tmp0_15;
  int16 res0_0,res0_1,res0_2,res0_3,res0_4,res0_5,res0_6,res0_7;
  int16 res0_8,res0_9,res0_10,res0_11,res0_12,res0_13,res0_14,res0_15;

  int16 tmp1_0,tmp1_1,tmp1_2,tmp1_3,tmp1_4,tmp1_5,tmp1_6,tmp1_7;
  int16 res1_0,res1_1,res1_2,res1_3,res1_4,res1_5,res1_6,res1_7;

  int16 tmp2_0,tmp2_1,tmp2_2,tmp2_3;
  int16 res2_0,res2_1,res2_2,res2_3;

  int16 tmp3_0,tmp3_1;
  int16 res3_0,res3_1;
  
  const int downshift = 16;

  tmp0_0 =  (fixed32_add(vec[0] , vec[62]) >> downshift);
  tmp0_1 =  (fixed32_add(vec[2] , vec[60]) >> downshift);
  tmp0_2 =  (fixed32_add(vec[4] , vec[58]) >> downshift);
  tmp0_3 =  (fixed32_add(vec[6] , vec[56]) >> downshift);
  tmp0_4 =  (fixed32_add(vec[8] , vec[54]) >> downshift);
  tmp0_5 =  (fixed32_add(vec[10], vec[52]) >> downshift);
  tmp0_6 =  (fixed32_add(vec[12], vec[50]) >> downshift);
  tmp0_7 =  (fixed32_add(vec[14], vec[48]) >> downshift);
  tmp0_8 =  (fixed32_add(vec[16], vec[46]) >> downshift);
  tmp0_9 =  (fixed32_add(vec[18], vec[44]) >> downshift);
  tmp0_10 = (fixed32_add(vec[20], vec[42]) >> downshift);
  tmp0_11 = (fixed32_add(vec[22], vec[40]) >> downshift);
  tmp0_12 = (fixed32_add(vec[24], vec[38]) >> downshift);
  tmp0_13 = (fixed32_add(vec[26], vec[36]) >> downshift);
  tmp0_14 = (fixed32_add(vec[28], vec[34]) >> downshift);
  tmp0_15 = (fixed32_add(vec[30], vec[32]) >> downshift);

  tmp1_0 = fixed16_add(tmp0_0, tmp0_15);
  tmp1_1 = fixed16_add(tmp0_1, tmp0_14);
  tmp1_2 = fixed16_add(tmp0_2, tmp0_13);
  tmp1_3 = fixed16_add(tmp0_3, tmp0_12);
  tmp1_4 = fixed16_add(tmp0_4, tmp0_11);
  tmp1_5 = fixed16_add(tmp0_5, tmp0_10);
  tmp1_6 = fixed16_add(tmp0_6, tmp0_9);
  tmp1_7 = fixed16_add(tmp0_7, tmp0_8);

  tmp2_0 = fixed16_add(tmp1_0, tmp1_7);
  tmp2_1 = fixed16_add(tmp1_1, tmp1_6);
  tmp2_2 = fixed16_add(tmp1_2, tmp1_5);
  tmp2_3 = fixed16_add(tmp1_3, tmp1_4);

  tmp3_0 = fixed16_add(tmp2_0, tmp2_3);
  tmp3_1 = fixed16_add(tmp2_1, tmp2_2);

  f_vec[0]  = fixed16_add(tmp3_0, tmp3_1);
  f_vec[32] = fixed16_mul(
  	fixed16_sub(tmp3_0, tmp3_1),
  	cost32_c4[0]) << 1;

  tmp3_0 = fixed16_mul(fixed16_sub(tmp2_0, tmp2_3), cost32_c3_int[0]);
  tmp3_1 = fixed16_mul(fixed16_sub(tmp2_1, tmp2_2), cost32_c3_int[1]) << 1; // +++++ MAY CLIP +++++

  res3_0 = fixed16_add(tmp3_0, tmp3_1);
  res3_1 = fixed16_mul(fixed16_sub(tmp3_0, tmp3_1), cost32_c4[0]);

  f_vec[16] = fixed16_add(res3_0, res3_1);
  f_vec[48] = res3_1;

  tmp2_0 = fixed16_mul(fixed16_sub(tmp1_0, tmp1_7), cost32_c2_int[0]);
  tmp2_1 = fixed16_mul(fixed16_sub(tmp1_1, tmp1_6), cost32_c2_int[1]);
  tmp2_2 = fixed16_mul(fixed16_sub(tmp1_2, tmp1_5), cost32_c2_int[2]);
  tmp2_3 = fixed16_mul(fixed16_sub(tmp1_3, tmp1_4), cost32_c2_int[3]) << 2; // +++++ MAY CLIP +++++
  tmp3_0 = fixed16_add(tmp2_0, tmp2_3);
  tmp3_1 = fixed16_add(tmp2_1, tmp2_2);

  res2_0 = fixed16_add(tmp3_0, tmp3_1);
  res2_2 = fixed16_mul(fixed16_sub(tmp3_0, tmp3_1), cost32_c4[0]);

  tmp3_0 = fixed16_mul(fixed16_sub(tmp2_0, tmp2_3), cost32_c3_int[0]);
  tmp3_1 = fixed16_mul(fixed16_sub(tmp2_1, tmp2_2), cost32_c3_int[1]) << 1; // +++++ MAY CLIP +++++
  res3_0 = fixed16_add(tmp3_0, tmp3_1);
  res3_1 = fixed16_mul(fixed16_sub(tmp3_0, tmp3_1), cost32_c4[0]);

  res2_1 = fixed16_add(res3_0, res3_1);

  f_vec[24] = fixed16_add(res2_0, res2_1);
  f_vec[8]  = fixed16_add(res2_1, res2_2);
  f_vec[40] = fixed16_add(res2_2, res3_1);
  f_vec[56] = res3_1;

  tmp1_0 = fixed16_mul(fixed16_sub(tmp0_0, tmp0_15), cost32_c1_int[0]);
  tmp1_1 = fixed16_mul(fixed16_sub(tmp0_1, tmp0_14), cost32_c1_int[1]);
  tmp1_2 = fixed16_mul(fixed16_sub(tmp0_2, tmp0_13), cost32_c1_int[2]);
  tmp1_3 = fixed16_mul(fixed16_sub(tmp0_3, tmp0_12), cost32_c1_int[3]);
  tmp1_4 = fixed16_mul(fixed16_sub(tmp0_4, tmp0_11), cost32_c1_int[4]);
  tmp1_5 = fixed16_mul(fixed16_sub(tmp0_5, tmp0_10), cost32_c1_int[5]) << 1; // +++++ MAY CLIP +++++
  tmp1_6 = fixed16_mul(fixed16_sub(tmp0_6, tmp0_9),  cost32_c1_int[6]) << 1; // +++++ MAY CLIP +++++
  tmp1_7 = fixed16_mul(fixed16_sub(tmp0_7, tmp0_8),  cost32_c1_int[7]) << 3; // +++++ MAY CLIP +++++
  tmp2_0 = fixed16_add(tmp1_0, tmp1_7);
  tmp2_1 = fixed16_add(tmp1_1, tmp1_6);
  tmp2_2 = fixed16_add(tmp1_2, tmp1_5);
  tmp2_3 = fixed16_add(tmp1_3, tmp1_4);
  tmp3_0 = fixed16_add(tmp2_0, tmp2_3);
  tmp3_1 = fixed16_add(tmp2_1, tmp2_2);

  res1_0 = fixed16_add(tmp3_0, tmp3_1);
  res1_4 = fixed16_mul(fixed16_sub(tmp3_0, tmp3_1), cost32_c4[0]);

  tmp3_0 = fixed16_mul(fixed16_sub(tmp2_0, tmp2_3), cost32_c3_int[0]);
  tmp3_1 = fixed16_mul(fixed16_sub(tmp2_1, tmp2_2), cost32_c3_int[1]) << 1; // +++++ MAY CLIP +++++

  res3_0 = fixed16_add(tmp3_0, tmp3_1);
  res3_1 = fixed16_mul(fixed16_sub(tmp3_0, tmp3_1), cost32_c4[0]);

  res1_2 = fixed16_add(res3_0, res3_1);
  res1_6 = res3_1;

  tmp2_0 = fixed16_mul(fixed16_sub(tmp1_0, tmp1_7), cost32_c2_int[0]);
  tmp2_1 = fixed16_mul(fixed16_sub(tmp1_1, tmp1_6), cost32_c2_int[1]);
  tmp2_2 = fixed16_mul(fixed16_sub(tmp1_2, tmp1_5), cost32_c2_int[2]);
  tmp2_3 = fixed16_mul(fixed16_sub(tmp1_3, tmp1_4), cost32_c2_int[3]) << 2; // +++++ MAY CLIP +++++
  tmp3_0 = fixed16_add(tmp2_0, tmp2_3);
  tmp3_1 = fixed16_add(tmp2_1, tmp2_2);

  res2_0 = fixed16_add(tmp3_0, tmp3_1);
  res2_2 = fixed16_mul(fixed16_sub(tmp3_0, tmp3_1), cost32_c4[0]);

  tmp3_0 = fixed16_mul(fixed16_sub(tmp2_0, tmp2_3), cost32_c3_int[0]);
  tmp3_1 = fixed16_mul(fixed16_sub(tmp2_1, tmp2_2), cost32_c3_int[1]) << 1; // +++++ MAY CLIP +++++

  res3_0 = fixed16_add(tmp3_0, tmp3_1);
  res3_1 = fixed16_mul(fixed16_sub(tmp3_0, tmp3_1), cost32_c4[0]);

  res2_1 = fixed16_add(res3_0, res3_1);
  res2_3 = res3_1;
  
  res1_1 = fixed16_add(res2_0, res2_1);
  res1_3 = fixed16_add(res2_1, res2_2);
  res1_5 = fixed16_add(res2_2, res2_3);
  res1_7 = res2_3;

  f_vec[28] = fixed16_add(res1_0, res1_1);
  f_vec[20] = fixed16_add(res1_1, res1_2);
  f_vec[12] = fixed16_add(res1_2, res1_3);
  f_vec[4] = fixed16_add(res1_3, res1_4);
  f_vec[36] = fixed16_add(res1_4, res1_5);
  f_vec[44] = fixed16_add(res1_5, res1_6);
  f_vec[52] = fixed16_add(res1_6, res1_7);
  f_vec[60] = res1_7;

  /*  Odd Terms */
  tmp0_0 =  fixed16_mul(fixed32_sub(vec[0], vec[62]) >> downshift,  cost32_c0_int[0]);
  tmp0_1 =  fixed16_mul(fixed32_sub(vec[2], vec[60]) >> downshift,  cost32_c0_int[1]);
  tmp0_2 =  fixed16_mul(fixed32_sub(vec[4], vec[58]) >> downshift,  cost32_c0_int[2]);
  tmp0_3 =  fixed16_mul(fixed32_sub(vec[6], vec[56]) >> downshift,  cost32_c0_int[3]);
  tmp0_4 =  fixed16_mul(fixed32_sub(vec[8], vec[54]) >> downshift,  cost32_c0_int[4]);
  tmp0_5 =  fixed16_mul(fixed32_sub(vec[10], vec[52]) >> downshift, cost32_c0_int[5]);
  tmp0_6 =  fixed16_mul(fixed32_sub(vec[12], vec[50]) >> downshift, cost32_c0_int[6]);
  tmp0_7 =  fixed16_mul(fixed32_sub(vec[14], vec[48]) >> downshift, cost32_c0_int[7]);
  tmp0_8 =  fixed16_mul(fixed32_sub(vec[16], vec[46]) >> downshift, cost32_c0_int[8]);
  tmp0_9 =  fixed16_mul(fixed32_sub(vec[18], vec[44]) >> downshift, cost32_c0_int[9]);
  tmp0_10 = fixed16_mul(fixed32_sub(vec[20], vec[42]) >> downshift, cost32_c0_int[10]);
  tmp0_11 = fixed16_mul(fixed32_sub(vec[22], vec[40]) >> downshift, cost32_c0_int[11]) << 1; // +++++ MAY CLIP +++++
  tmp0_12 = fixed16_mul(fixed32_sub(vec[24], vec[38]) >> downshift, cost32_c0_int[12]) << 1; // +++++ MAY CLIP +++++
  tmp0_13 = fixed16_mul(fixed32_sub(vec[26], vec[36]) >> downshift, cost32_c0_int[13]) << 2; // +++++ MAY CLIP +++++
  tmp0_14 = fixed16_mul(fixed32_sub(vec[28], vec[34]) >> downshift, cost32_c0_int[14]) << 2; // +++++ MAY CLIP +++++
  tmp0_15 = fixed16_mul(fixed32_sub(vec[30], vec[32]) >> downshift, cost32_c0_int[15]) << 4; // +++++ MAY CLIP +++++

  tmp1_0 = fixed16_add(tmp0_0, tmp0_15);
  tmp1_1 = fixed16_add(tmp0_1, tmp0_14);
  tmp1_2 = fixed16_add(tmp0_2, tmp0_13);
  tmp1_3 = fixed16_add(tmp0_3, tmp0_12);
  tmp1_4 = fixed16_add(tmp0_4, tmp0_11);
  tmp1_5 = fixed16_add(tmp0_5, tmp0_10);
  tmp1_6 = fixed16_add(tmp0_6, tmp0_9);
  tmp1_7 = fixed16_add(tmp0_7, tmp0_8);
  
  tmp2_0 = fixed16_add(tmp1_0, tmp1_7);
  tmp2_1 = fixed16_add(tmp1_1, tmp1_6);
  tmp2_2 = fixed16_add(tmp1_2, tmp1_5);
  tmp2_3 = fixed16_add(tmp1_3, tmp1_4);
  
  tmp3_0 = fixed16_add(tmp2_0, tmp2_3);
  tmp3_1 = fixed16_add(tmp2_1, tmp2_2);


	res0_0 = fixed16_add(tmp3_0, tmp3_1);
	res0_8 = fixed16_mul(fixed16_sub(tmp3_0, tmp3_1), cost32_c4[0]);
//printf( "r0_0 %f   r0_8 %f \n", res0_0, res0_8 );	
	   
	tmp3_0 = fixed16_mul(fixed16_sub(tmp2_0, tmp2_3), cost32_c3_int[0]);
	tmp3_1 = fixed16_mul(fixed16_sub(tmp2_1, tmp2_2), cost32_c3_int[1]) << 1; // +++++ MAY CLIP +++++
	res3_0 = fixed16_add(tmp3_0, tmp3_1);
	res3_1 = fixed16_mul(fixed16_sub(tmp3_0, tmp3_1), cost32_c4[0]);
	res0_4 = fixed16_add(res3_0, res3_1);
	res0_12 = res3_1;
//printf( "r0_4 %f   r0_12 %f \n", res0_4, res0_12 );	
	
	
	tmp2_0 = fixed16_mul(fixed16_sub(tmp1_0, tmp1_7), cost32_c2_int[0]);
	tmp2_1 = fixed16_mul(fixed16_sub(tmp1_1, tmp1_6), cost32_c2_int[1]);
	tmp2_2 = fixed16_mul(fixed16_sub(tmp1_2, tmp1_5), cost32_c2_int[2]);
	tmp2_3 = fixed16_mul(fixed16_sub(tmp1_3, tmp1_4), cost32_c2_int[3]) << 2; // +++++ MAY CLIP +++++
	tmp3_0 = fixed16_add(tmp2_0, tmp2_3);
	tmp3_1 = fixed16_add(tmp2_1, tmp2_2);
	res2_0 = fixed16_add(tmp3_0, tmp3_1);
	res2_2 = fixed16_mul(fixed16_sub(tmp3_0, tmp3_1), cost32_c4[0]);
	
	tmp3_0 = fixed16_mul(fixed16_sub(tmp2_0, tmp2_3), cost32_c3_int[0]);
	tmp3_1 = fixed16_mul(fixed16_sub(tmp2_1, tmp2_2), cost32_c3_int[1]) << 1; // +++++ MAY CLIP +++++
	res3_0 = fixed16_add(tmp3_0, tmp3_1);
	res3_1 = fixed16_mul(fixed16_sub(tmp3_0, tmp3_1), cost32_c4[0]);
	res2_1 = fixed16_add(res3_0, res3_1);
	res2_3 = res3_1;
	res0_2 = fixed16_add(res2_0, res2_1);
	res0_6 = fixed16_add(res2_1, res2_2);
	res0_10 = fixed16_add(res2_2, res2_3);
	res0_14 = res2_3;
//printf( "r0_2  %f   r0_6  %f \n", res0_2, res0_6 );	
//printf( "r0_10 %f   r0_14 %f \n", res0_10, res0_14 );	
	
	tmp1_0 = fixed16_mul(fixed16_sub(tmp0_0, tmp0_15), cost32_c1_int[0]);
	tmp1_1 = fixed16_mul(fixed16_sub(tmp0_1, tmp0_14), cost32_c1_int[1]);
	tmp1_2 = fixed16_mul(fixed16_sub(tmp0_2, tmp0_13), cost32_c1_int[2]);
	tmp1_3 = fixed16_mul(fixed16_sub(tmp0_3, tmp0_12), cost32_c1_int[3]);
	tmp1_4 = fixed16_mul(fixed16_sub(tmp0_4, tmp0_11), cost32_c1_int[4]);
	tmp1_5 = fixed16_mul(fixed16_sub(tmp0_5, tmp0_10), cost32_c1_int[5]) << 1; // +++++ MAY CLIP +++++
	tmp1_6 = fixed16_mul(fixed16_sub(tmp0_6, tmp0_9),  cost32_c1_int[6]) << 1; // +++++ MAY CLIP +++++
	tmp1_7 = fixed16_mul(fixed16_sub(tmp0_7, tmp0_8),  cost32_c1_int[7]) << 3; // +++++ MAY CLIP +++++
	tmp2_0 = fixed16_add(tmp1_0, tmp1_7);
	tmp2_1 = fixed16_add(tmp1_1, tmp1_6);
	tmp2_2 = fixed16_add(tmp1_2, tmp1_5);
	tmp2_3 = fixed16_add(tmp1_3, tmp1_4);
	tmp3_0 = fixed16_add(tmp2_0, tmp2_3);
	tmp3_1 = fixed16_add(tmp2_1, tmp2_2);

	res1_0 = fixed16_add(tmp3_0, tmp3_1);
	res1_4 = fixed16_mul(fixed16_sub(tmp3_0, tmp3_1), cost32_c4[0]);
//printf( "r1_0  %f     r1_4  %f \n", res1_0, res1_4 );	
	
	tmp3_0 = fixed16_mul(fixed16_sub(tmp2_0, tmp2_3), cost32_c3_int[0]);
	tmp3_1 = fixed16_mul(fixed16_sub(tmp2_1, tmp2_2), cost32_c3_int[1]) << 1; // +++++ MAY CLIP +++++
	res3_0 = fixed16_add(tmp3_0, tmp3_1);
	res3_1 = fixed16_mul(fixed16_sub(tmp3_0, tmp3_1), cost32_c4[0]);
	res1_2 = fixed16_add(res3_0, res3_1);
	res1_6 = res3_1;
//printf( "r1_2  %f     r1_6  %f \n", res1_2, res1_6 );	
	
	tmp2_0 = fixed16_mul(fixed16_sub(tmp1_0, tmp1_7), cost32_c2_int[0]);
	tmp2_1 = fixed16_mul(fixed16_sub(tmp1_1, tmp1_6), cost32_c2_int[1]);
	tmp2_2 = fixed16_mul(fixed16_sub(tmp1_2, tmp1_5), cost32_c2_int[2]);
	tmp2_3 = fixed16_mul(fixed16_sub(tmp1_3, tmp1_4), cost32_c2_int[3]) << 2; // +++++ MAY CLIP +++++
	tmp3_0 = fixed16_add(tmp2_0, tmp2_3);
	tmp3_1 = fixed16_add(tmp2_1, tmp2_2);
	res2_0 = fixed16_add(tmp3_0, tmp3_1);
	res2_2 = fixed16_mul(fixed16_sub(tmp3_0, tmp3_1), cost32_c4[0]);
	
	tmp3_0 = fixed16_mul(fixed16_sub(tmp2_0, tmp2_3), cost32_c3_int[0]);
	tmp3_1 = fixed16_mul(fixed16_sub(tmp2_1, tmp2_2), cost32_c3_int[1]) << 1; // +++++ MAY CLIP +++++
	res3_0 = fixed16_add(tmp3_0, tmp3_1);
	res3_1 = fixed16_mul(fixed16_sub(tmp3_0, tmp3_1), cost32_c4[0]);
	
	res2_1 = fixed16_add(res3_0, res3_1);
	res2_3 = res3_1;
	res1_1 = fixed16_add(res2_0, res2_1);
	res1_3 = fixed16_add(res2_1, res2_2);
	res1_5 = fixed16_add(res2_2, res2_3);
	res1_7 = res2_3;
	
	res0_1 = fixed16_add(res1_0,  res1_1);
	res0_3 = fixed16_add(res1_1,  res1_2);
	res0_5 = fixed16_add(res1_2,  res1_3);
	res0_7 = fixed16_add(res1_3,  res1_4);
	res0_9 = fixed16_add(res1_4,  res1_5);
	res0_11 = fixed16_add(res1_5, res1_6);
	res0_13 = fixed16_add(res1_6, res1_7);
	res0_15 = res1_7;
	
	f_vec[30] = fixed16_add(res0_0,  res0_1);
	f_vec[26] = fixed16_add(res0_1,  res0_2);
	f_vec[22] = fixed16_add(res0_2,  res0_3);
	f_vec[18] = fixed16_add(res0_3,  res0_4);
	f_vec[14] = fixed16_add(res0_4,  res0_5);
	f_vec[10] = fixed16_add(res0_5,  res0_6);
	f_vec[6]  = fixed16_add(res0_6,  res0_7);
	f_vec[2]  = fixed16_add(res0_7,  res0_8);
	f_vec[34] = fixed16_add(res0_8,  res0_9);
	f_vec[38] = fixed16_add(res0_9,  res0_10);
	f_vec[42] = fixed16_add(res0_10, res0_11);
	f_vec[46] = fixed16_add(res0_11, res0_12);
	f_vec[50] = fixed16_add(res0_12, res0_13);
	f_vec[54] = fixed16_add(res0_13, res0_14);
	f_vec[58] = fixed16_add(res0_14, res0_15);
	f_vec[62] = res0_15;
}

#endif
