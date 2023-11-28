// cine_functions.cpp

#include "cine_functions.h"

#include <Debug.h>
#include <InterfaceDefs.h>
#include <ByteOrder.h>
#include <stdio.h>
#include <string.h>

const uint32 QT_CVID_MAX_STRIPS = 16;
void cpDrawIntraframe(uchar *baseAddr, long rowBytes, long width, long height,
	long realWidth, long realHeight, long *codebook, uchar *data);
void cpDrawInterframe(uchar *baseAddr, long rowBytes,long width, long height,
	long realWidth, long realHeight, long *codebook, uchar *data);

void cpDrawIntraframe8(uchar *baseAddr, long rowBytes, long width, long height,
	long realWidth, long realHeight, long *codebook, uchar *data);
void cpDrawInterframe8(uchar *baseAddr, long rowBytes, long width, long height,
	long realWidth, long realHeight, long *codebook, uchar *data);

inline int16 cpMAP(int16 p);

inline uchar clip255(int16 x);

struct CineStruct {

		char		*buffer;
		long		*codebook;

};

enum {
	kKeyType			= 0x00,
	kInterType			= 0x01,

	kFrameType			= 0x00,
	kKeyFrameType		= kKeyType + kFrameType,
	kInterFrameType		= kInterType + kFrameType,

	kTileType			= 0x10,
	kTileKeyType		= kKeyType + kTileType,
	kTileInterType		= kInterType + kTileType,

	kCodeBookType		= 0x20,
	kFullDBookType		= 0x00 + kCodeBookType,
	kPartialDBookType 	= 0x01 + kCodeBookType,
	kFullSBookType		= 0x02 + kCodeBookType,
	kPartialSBookType 	= 0x03 + kCodeBookType,

	kCodesType			= 0x30,
	kIntraCodesType		= 0x00 + kCodesType,
	kInterCodesType		= 0x01 + kCodesType,
	kAllSmoothCodesType = 0x02 + kCodesType
};

typedef struct {
	int32	frameSize;
	int16	frameWidth;
	int16	frameHeight;
	int16	frameTileCount;
} FrameHeader, *FrameHeaderPtr;

typedef struct {
	int32	tileSize;
	int16	tileRectTop;
	int16	tileRectLeft;
	int16	tileRectBottom;
	int16	tileRectRight;
} TileHeader, *TileHeaderPtr;

#define codecBadDataErr -1
#define kCodebookSize (256*4 + 256*16)

inline int16 cpMAP(int16 p) 
{
	p += 4;
	if (p < 0)
		return 0;
	if (p > 0xFF)
		return 0xF8;
	return p & 0xF8;
}

 void cpExpandCodeVector8(uchar *data, uchar *dst)
{
	const uint8 *map = system_colors()->index_map;
 	int16	u,v,uv,i;
	
	u = (int8)data[4] << 1;
	v = (int8)data[5] << 1;
	uv = ((v >> 1) + u) >> 1;
	
//	Convert the YUV to RGB pixels to indexes in map
	
	int16 dither[4] = { 0,8,12,4 };

	#if defined(__POWERPC__) || defined(__ARMEB__)	/* FIXME: This should probably use <endian.h> for the right define */
	int32 y;
	for (i = 0; i < 4; i++) {
		y = data[i] + (dither[i] - 6)*3;
		dst[i] = map[(cpMAP(y + v) << 7) | (cpMAP(y - uv) << 2) | cpMAP(y + u) >> 3];
	}
	#endif
	
	#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
	int32 y, j = 0;
	
	int16 scan[4] = { 2, 3, 0, 1 };
	
	for (i = 0; i < 4; i++) {
		y = data[i] + (dither[i] - 6)*3;
		dst[scan[j++]] = map[(cpMAP(y + v) << 7) | (cpMAP(y - uv) << 2) | cpMAP(y + u) >> 3];
	}	
#endif	
}

 void cpExpandCodeVectorSmooth8(uchar *data, uchar *dst)
{
	const uint8 *map = system_colors()->index_map;
	short	u,v,uv,i;	

	u = (signed char)data[4] << 1;
	v = (signed char)data[5] << 1;
	uv = ((v >> 1) + u) >> 1;
	
//	Convert the YUV to RGB pixels to indexes in map
	
	short j = 0;
	
	const short scan[16] = { 0,1,4,5,2,3,6,7, 8,9,12,13,10,11,14,15 };
	const short dither[16] = { 0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5 };
	
	long y;
	for (i = 0; i < 4; i++) {
		y = data[i] + (dither[scan[j]] - 8)*3;
		dst[scan[j++]] = map[(cpMAP(y + v) << 7) | (cpMAP(y - uv) << 2) | cpMAP(y + u) >> 3];
		y = data[i] + (dither[scan[j]] - 8)*3;
		dst[scan[j++]] = map[(cpMAP(y + v) << 7) | (cpMAP(y - uv) << 2) | cpMAP(y + u) >> 3];
		y = data[i] + (dither[scan[j]] - 8)*3;
		dst[scan[j++]] = map[(cpMAP(y + v) << 7) | (cpMAP(y - uv) << 2) | cpMAP(y + u) >> 3];
		y = data[i] + (dither[scan[j]] - 8)*3;
		dst[scan[j++]] = map[(cpMAP(y + v) << 7) | (cpMAP(y - uv) << 2) | cpMAP(y + u) >> 3];
	}
}


/*	Range to 0-255	*/

inline uchar clip255(int16 x)
{
	if (x < 0)
		return 0;
	if (x > 0xFF)
		return (0xFF);
	return x;
}

/*================================================================================================*/

//	Expand a code vector YUV to RGB

 void cpExpandCodeVector(uchar *data, uchar *dst)
{
	int16	u,v,uv,i;	
	u = (int8)data[4] << 1;
	v = (int8)data[5] << 1;
	uv = ((v >> 1) + u) >> 1;
	
//	Convert the YUV to RGB pixels
	
	for (i = 0; i < 4; i++)	{
		dst[0] = clip255(data[i] + u);	// BGRx
		dst[1] = clip255(data[i] - uv);
		dst[2] = clip255(data[i] + v);
		dst[3] = 255;
		dst += 4;
	}
}

 void cpExpandCodeVector15(uchar *data, uchar *dst)
{
	int16	u,v,uv,i;	
	u = (int8)data[4] << 1;
	v = (int8)data[5] << 1;
	uv = ((v >> 1) + u) >> 1;
	
//	Convert the YUV to RGB pixels
	
	int16 *dst16 = (int16 *)dst;
	
	for (i = 0; i < 4; i++)	{
		*dst16++ = (clip255(data[i] + u) >> 3) | (clip255(data[i] - uv) >> 3) << 5 | (clip255(data[i] + v) >> 3) << 10;
	}
}

inline  long	AtomSize(uchar *d) 
{
	return B_BENDIAN_TO_HOST_INT32(*((long *)d)) & 0x00FFFFFF;
}

 void cpExpandCodeBook(uchar *data, long *codeBook, long depth)
{
	long	codeBookType,size,codeSize,c;
	void	(*proc)(uchar *data, uchar *code);
	
	codeBookType = *data;
	size = AtomSize(data);
	data += 4;
	if (size == 4)
		return;
		
//	Determine how codebook will be expanded
	
	proc = cpExpandCodeVector;
	codeSize = 4;
	if (depth == 8) {
		proc = cpExpandCodeVectorSmooth8;
		if (codeBookType == kFullDBookType || codeBookType == kPartialDBookType) {
			proc = cpExpandCodeVector8;
			codeSize = 1;
		}
	} else if (depth == 15) {
		proc = cpExpandCodeVector15;
		codeSize = 2;
	}
	
	switch (codeBookType) {			// Full Codebook
		case kFullDBookType:
		case kFullSBookType:
		for (c = (size - 4)/6; c--;) {
			(proc)(data,(uchar *)codeBook);
			data += 6;

			codeBook += codeSize;
		}
		break;
		default:					// Partial Codebook
			long	bitsleft = 1;
			long	bits,n;
			for (c = 256; c--;)	{
				if (--bitsleft == 0) {
					for (bits = 0, n = 4; n--;)
						bits = (bits << 8) | *data++;
					bitsleft = 32;
				}
				if (bits < 0) {
					(proc)(data,(uchar *)codeBook);
					data += 6;
				}
				
				codeBook += codeSize;
				bits = (bits << 1);
			}
	}
}

short cpDecompress(uchar *data, uchar *baseAddr,
				   long rowuchars, long real_width, long real_height,
				   long *codebook, long depth)
{
	uchar	*frameEnd,*tileEnd,tileType,frameType;
	short	t,tileCount,width,height;
	
	long	codeBookSize;
	long*	smooth;
	
	if (depth == 32) {
		codeBookSize = 256*4 + 256*4;	// 32 bit codebook is 8k (codeBookSize is in longs)
		smooth = codebook + 256*4;
	} else if (depth == 15) {
		codeBookSize = 256*2 + 256*4;		// 8 bit codebook is 5k
		smooth = codebook + 256*2;		
	} else {
		codeBookSize = 256 + 256*4;		// 8 bit codebook is 5k
		smooth = codebook + 256;
	}
	
	//printf("uncle %x\n",data);
	
	frameType = *data;

	if (!(frameType == kKeyFrameType || frameType == kInterFrameType))
		return codecBadDataErr;
	frameEnd = data + AtomSize(data);
	tileCount = B_BENDIAN_TO_HOST_INT16(((FrameHeader *)data)->frameTileCount);
	data += 10; // Skip header
					
	if (tileCount > 4)
		tileCount = 4;	// Only support up to 4 tiles... (codebook propagation sucks down memory!)
	for (t = 0; t < tileCount; t++) {
		
		for (;;) {
			tileType = *data;
			if (tileType == kTileKeyType || tileType == kTileInterType)
				break;
			data += AtomSize(data);
			if (data >= frameEnd)
				return codecBadDataErr;
		}
		width = B_BENDIAN_TO_HOST_INT16(((TileHeader *)data)->tileRectRight) - B_BENDIAN_TO_HOST_INT16(((TileHeader *)data)->tileRectLeft);
		height = B_BENDIAN_TO_HOST_INT16(((TileHeader *)data)->tileRectBottom) - B_BENDIAN_TO_HOST_INT16(((TileHeader *)data)->tileRectTop);

		tileEnd = data + AtomSize(data);
		data += 12;
		
		if (t) {
			if (frameType == kKeyFrameType && tileType == kTileInterType)
				memcpy((uchar *)codebook + codeBookSize*4,codebook,codeBookSize*4);
			codebook += codeBookSize;
			smooth += codeBookSize;
		}
		
		while (data < tileEnd) {
			switch (*data) {
				case kFullDBookType:
				case kPartialDBookType:
					cpExpandCodeBook(data,codebook,depth);
					break;
				case kFullSBookType:
				case kPartialSBookType:
					cpExpandCodeBook(data,smooth,depth);
					break;
				case kIntraCodesType:
					if (depth == 32)
						cpDrawIntraframe(baseAddr, rowuchars, width, height,
							real_width, real_height, codebook, data + 4);
					else
						cpDrawIntraframe8(baseAddr, rowuchars, width, height,
							real_width, real_height, codebook, data + 4);
					break;
				case kInterCodesType:
					if (depth == 32)
						cpDrawInterframe(baseAddr, rowuchars, width, height,
							real_width, real_height, codebook, data + 4);
					else
						cpDrawInterframe8(baseAddr, rowuchars, width, height,
							real_width, real_height, codebook, data + 4);
					break;
			}
			data += AtomSize(data);
		}
		baseAddr += height * rowuchars;
	}
	return 0;
}

void cpDrawIntraframe(uchar *baseAddr,long rowBytes, long width, long height,
	long realWidth, long realHeight, long *codebook, uchar *data)
{
	long	*line0,*line1,*line2,*line3,*d;
	long	makeup,bits,bitsleft,ss;
	long	*smooth,*s;
	short	h,v,n;

//	printf("rowBytes = %ld rowBytes >> 2) %ld\n",rowBytes, rowBytes >> 2);
	
	smooth = codebook + 256*4;
	line0 = (long *)baseAddr;
	line1 = line0 + (rowBytes >> 2);
	line2 = line1 + (rowBytes >> 2);
	line3 = line2 + (rowBytes >> 2);
	makeup = rowBytes - width;
//	printf("makeup = %ld\n",makeup);
//	makeup = 0;
	
	long lineDelta = height - realHeight;
	long extraLines = 0;
	if (lineDelta > 0 && lineDelta < 4) {
		height = (realHeight >> 2) << 2;
		extraLines = realHeight - height;
	}
	
	long extraColumns = 0;
	if (width - realWidth != 0) {
		width = (realWidth >> 2) << 2;
		extraColumns = realWidth - width;
	}
	
	bitsleft = 0;
	for (v = (height >> 2); v--;) {
		for (h = (width >> 2); h--;) {
			if (!bitsleft) {
				for (bits = 0, n = 4; n--;)	{
					bits = (bits << 8) | *data++;
				}
				bitsleft = 32;
			}
			
			if (bits < 0) {	
				d = codebook + (*data++ << 2);
				*line0++ = *d++;
				*line0++ = *d++;
				*line1++ = *d++;
				*line1++ = *d;

				d = codebook + (*data++ << 2);
				*line0++ = *d++;
				*line0++ = *d++;
				*line1++ = *d++;
				*line1++ = *d;

				d = codebook + (*data++ << 2);
				*line2++ = *d++;
				*line2++ = *d++;
				*line3++ = *d++;
				*line3++ = *d;

				d = codebook + (*data++ << 2);
				*line2++ = *d++;
				*line2++ = *d++;
				*line3++ = *d++;
				*line3++ = *d;
			} else {
				s = smooth + (*data++ << 2);
				ss = *s++;
				*line0++ = ss;
				*line0++ = ss;
				*line1++ = ss;
				*line1++ = ss;

				ss = *s++;
				*line0++ = ss;
				*line0++ = ss;
				*line1++ = ss;
				*line1++ = ss;

				ss = *s++;
				*line2++ = ss;
				*line2++ = ss;
				*line3++ = ss;
				*line3++ = ss;

				ss = *s++;
				*line2++ = ss;
				*line2++ = ss;
				*line3++ = ss;
				*line3++ = ss;	
			}
			
			bits = (bits << 1);
			bitsleft--;
		}
		if (extraColumns) {
			if (!bitsleft) {
				for (bits = 0, n = 4; n--;)	{
					bits = (bits << 8) | *data++;
				}
				bitsleft = 32;
			}
			
			if (bits < 0) {	
				d = codebook + (*data++ << 2);
				if (extraColumns == 1) {
					// only one column left
					*line0++ = *d++;
					line0++;
					*line1++ = *++d;
					line1++;
				} else {
					// at least two colums left
					*line0++ = *d++;
					*line0++ = *d++;
					*line1++ = *d++;
					*line1++ = *d;
				}

				d = codebook + (*data++ << 2);
				if (extraColumns == 2) {
					// two columns left
					line0 += 2;
					line1 += 2;
				} else {
					// three columns left
					*line0++ = *d++;
					line0++;
					*line1++ = *++d;
					line1++;
				}
				
				d = codebook + (*data++ << 2);
				if (extraColumns == 1) {
					// only one column left
					*line2++ = *d++;
					line2++;
					*line3++ = *++d;
					line3++;
				} else {
					// at least two colums left
					*line2++ = *d++;
					*line2++ = *d++;
					*line3++ = *d++;
					*line3++ = *d;
				}

				d = codebook + (*data++ << 2);
				if (extraColumns == 2) {
					// two columns left
					line2 += 2;
					line3 += 2;
				} else {
					// three columns left
					*line2++ = *d++;
					line2++;
					*line3++ = *++d;
					line3++;
				}

			} else {
				s = smooth + (*data++ << 2);
				ss = *s++;
				if (extraColumns == 1) {
					*line0++ = ss;
					line0++;
					*line1++ = ss;
					line1++;
				} else {
					*line0++ = ss;
					*line0++ = ss;
					*line1++ = ss;
					*line1++ = ss;
				}

				ss = *s++;
				if (extraColumns == 2) {
					line0 += 2;
					line1 += 2;
				} else {
					*line0++ = ss;
					line0++;
					*line1++ = ss;
					line1++;
				}

				ss = *s++;
				if (extraColumns == 1) {
					*line2++ = ss;
					line2++;
					*line3++ = ss;
					line3++;
				} else {
					*line2++ = ss;
					*line2++ = ss;
					*line3++ = ss;
					*line3++ = ss;
				}
				
				ss = *s++;
				if (extraColumns == 2) {
					line2 += 2;
					line3 += 2;
				} else {
					*line2++ = ss;
					line2++;
					*line3++ = ss;
					line3++;	
				}
			}
			
			bits = (bits << 1);
			bitsleft--;
		}

		line0 += makeup;
		line1 += makeup;
		line2 += makeup;
		line3 += makeup;
	}
	if (extraLines) {
		for (h = (width >> 2); h--;) {
			if (!bitsleft) {
				for (bits = 0, n = 4; n--;)	{
					bits = (bits << 8) | *data++;
				}
				bitsleft = 32;
			}
			
			if (bits < 0) {	
				d = codebook + (*data++ << 2);
				*line0++ = *d++;
				*line0++ = *d++;
				if (extraLines > 1) {
					*line1++ = *d++;
					*line1++ = *d;
				} 

				d = codebook + (*data++ << 2);
				*line0++ = *d++;
				*line0++ = *d++;
				if (extraLines > 1) {
					*line1++ = *d++;
					*line1++ = *d;
				}
				
				d = codebook + (*data++ << 2);
				if (extraLines > 2) {
					*line2++ = *d++;
					*line2++ = *d;
				}

				d = codebook + (*data++ << 2);
				if (extraLines > 2) {
					*line2++ = *d++;
					*line2++ = *d;
				}

			} else {
				s = smooth + (*data++ << 2);
				ss = *s++;
				*line0++ = ss;
				*line0++ = ss;
				if (extraLines > 1) {
					*line1++ = ss;
					*line1++ = ss;
				}

				ss = *s++;
				*line0++ = ss;
				*line0++ = ss;
				if (extraLines > 1) {
					*line1++ = ss;
					*line1++ = ss;
				}

				ss = *s++;
				if (extraLines > 2) {
					*line2++ = ss;
					*line2++ = ss;
				}

				ss = *s++;
				if (extraLines > 2) {
					*line2++ = ss;
					*line2++ = ss;
				}
			}
			
			bits = (bits << 1);
			bitsleft--;
		}
		if (extraColumns) {
			if (!bitsleft) {
				for (bits = 0, n = 4; n--;)	{
					bits = (bits << 8) | *data++;
				}
				bitsleft = 32;
			}
			
			if (bits < 0) {	
				d = codebook + (*data++ << 2);
				if (extraColumns == 1) {
					// only one column left
					*line0++ = *d++;
					line0++;
					if (extraLines > 1) {
						*line1++ = *++d;
						line1++;
					}
				} else {
					// at least two colums left
					*line0++ = *d++;
					*line0++ = *d++;
					if (extraLines > 1) {
						*line1++ = *d++;
						*line1++ = *d;
					}
				}

				d = codebook + (*data++ << 2);
				if (extraColumns == 2) {
					// two columns left
					line0 += 2;
					line1 += 2;
				} else {
					// three columns left
					*line0++ = *d++;
					line0++;
					if (extraLines > 1) 
						*line1++ = *++d;
					line1++;
				}
				
				d = codebook + (*data++ << 2);
				if (extraColumns == 1) {
					// only one column left
					if (extraLines > 2) {
						*line2++ = *d++;
						line2++;
					}
				} else {
					// at least two colums left
					if (extraLines > 2) {
						*line2++ = *d++;
						*line2++ = *d;
					}
				}

				d = codebook + (*data++ << 2);
				if (extraColumns == 2) {
					// two columns left
					line2 += 2;
					line3 += 2;
				} else {
					// three columns left
					if (extraLines > 2) 
						*line2++ = *d;
					line2++;
				}

			} else {
				s = smooth + (*data++ << 2);
				ss = *s++;
				if (extraColumns == 1) {
					*line0++ = ss;
					line0++;
					if (extraLines > 1) 
						*line1++ = ss;
					line1++;
				} else {
					*line0++ = ss;
					*line0++ = ss;
					if (extraLines > 1) {
						*line1++ = ss;
						*line1++ = ss;
					}
				}

				ss = *s++;
				if (extraColumns == 2) {
					line0 += 2;
					line1 += 2;
				} else {
					*line0++ = ss;
					line0++;
					if (extraLines > 1)
						*line1++ = ss;
					line1++;
				}

				ss = *s++;
				if (extraColumns == 1) {
					if (extraLines > 2)
						*line2++ = ss;
					line2++;
				} else {
					if (extraLines > 2) {
						*line2++ = ss;
						*line2++ = ss;
					}
				}
				
				ss = *s++;
				if (extraColumns == 2) {
					line2 += 2;
				} else {
					if (extraLines > 2)
						*line2++ = ss;
					line2++;
				}
			}
			
			bits = (bits << 1);
			bitsleft--;
		}
	}
}

/*	Draw an interframe	*/

void cpDrawInterframe(uchar *baseAddr, long rowBytes, long width, long height,
	long realWidth, long realHeight,  long *codebook, uchar *data)
{
	long	*line0,*line1,*line2,*line3,*d;
	long	*smooth,*s;
	long	makeup,bits,bitsleft,ss;
	short	h,v,n;
	
	smooth = codebook + 1024;
	line0 = (long *)baseAddr;	
	line1 = line0 + (rowBytes >> 2);
	line2 = line1 + (rowBytes >> 2);
	line3 = line2 + (rowBytes >> 2);
	makeup = rowBytes - width;
		
	long lineDelta = height - realHeight;
	long extraLines = 0;
	if (lineDelta > 0 && lineDelta < 4) {
		height = (realHeight >> 2) << 2;
		extraLines = realHeight - height;
	}
	
	long extraColumns = 0;
	if (width - realWidth != 0) {
		width = (realWidth >> 2) << 2;
		extraColumns = realWidth - width;
	}

	bitsleft = 0;
	for (v = 0; v < (height >> 2); v++)	{
		for (h = 0; h < (width >> 2); h++)	{
			if (!bitsleft) {
				for (bits = n = 0; n < 4; n++)	{
					bits = (bits << 8) | *data++;
				}
				bitsleft = 32;
			}
			
			if (bits < 0) {	
			
				bits = (bits << 1);
				bitsleft--;
				if (!bitsleft) {
					for (bits = n = 0; n < 4; n++)	{
						bits = (bits << 8) | *data++;
					}
					bitsleft = 32;
				}

				if (bits < 0) {	
					d = codebook + (*data++ << 2);
					*line0++ = *d++;
					*line0++ = *d++;
					*line1++ = *d++;
					*line1++ = *d;
	
					d = codebook + (*data++ << 2);
					*line0++ = *d++;
					*line0++ = *d++;
					*line1++ = *d++;
					*line1++ = *d;
	
					d = codebook + (*data++ << 2);
					*line2++ = *d++;
					*line2++ = *d++;
					*line3++ = *d++;
					*line3++ = *d;
	
					d = codebook + (*data++ << 2);
					*line2++ = *d++;
					*line2++ = *d++;
					*line3++ = *d++;
					*line3++ = *d;
				} else {
					s = smooth + (*data++ << 2);
					ss = *s++;
					*line0++ = ss;
					*line0++ = ss;
					*line1++ = ss;
					*line1++ = ss;
	
					ss = *s++;
					*line0++ = ss;
					*line0++ = ss;
					*line1++ = ss;
					*line1++ = ss;
	
					ss = *s++;
					*line2++ = ss;
					*line2++ = ss;
					*line3++ = ss;
					*line3++ = ss;
	
					ss = *s++;
					*line2++ = ss;
					*line2++ = ss;
					*line3++ = ss;
					*line3++ = ss;	
				}
				bits = (bits << 1);
				bitsleft--;
				
			}	else	{
			
				line0 += 4;
				line1 += 4;
				line2 += 4;
				line3 += 4;
				bits = (bits << 1);
				bitsleft--;
				
			}
		}


		if (extraColumns) {
			if (!bitsleft) {
				for (bits = n = 0; n < 4; n++)	{
					bits = (bits << 8) | *data++;
				}
				bitsleft = 32;
			}
			
			if (bits < 0) {	

				bits = (bits << 1);
				bitsleft--;
				if (!bitsleft) {
					for (bits = n = 0; n < 4; n++)	{
						bits = (bits << 8) | *data++;
					}
					bitsleft = 32;
				}

				if (bits < 0) {	
					d = codebook + (*data++ << 2);
					if (extraColumns == 1) {
						// only one column left
						*line0++ = *d++;
						line0++;
						*line1++ = *++d;
						line1++;
					} else {
						// at least two colums left
						*line0++ = *d++;
						*line0++ = *d++;
						*line1++ = *d++;
						*line1++ = *d;
					}
	
					d = codebook + (*data++ << 2);
					if (extraColumns == 2) {
						// two columns left
						line0 += 2;
						line1 += 2;
					} else {
						// three columns left
						*line0++ = *d++;
						line0++;
						*line1++ = *++d;
						line1++;
					}
					
					d = codebook + (*data++ << 2);
					if (extraColumns == 1) {
						// only one column left
						*line2++ = *d++;
						line2++;
						*line3++ = *++d;
						line3++;
					} else {
						// at least two colums left
						*line2++ = *d++;
						*line2++ = *d++;
						*line3++ = *d++;
						*line3++ = *d;
					}
	
					d = codebook + (*data++ << 2);
					if (extraColumns == 2) {
						// two columns left
						line2 += 2;
						line3 += 2;
					} else {
						// three columns left
						*line2++ = *d++;
						line2++;
						*line3++ = *++d;
						line3++;
					}
	
				} else {
					s = smooth + (*data++ << 2);
					ss = *s++;
					if (extraColumns == 1) {
						*line0++ = ss;
						line0++;
						*line1++ = ss;
						line1++;
					} else {
						*line0++ = ss;
						*line0++ = ss;
						*line1++ = ss;
						*line1++ = ss;
					}
	
					ss = *s++;
					if (extraColumns == 2) {
						line0 += 2;
						line1 += 2;
					} else {
						*line0++ = ss;
						line0++;
						*line1++ = ss;
						line1++;
					}
	
					ss = *s++;
					if (extraColumns == 1) {
						*line2++ = ss;
						line2++;
						*line3++ = ss;
						line3++;
					} else {
						*line2++ = ss;
						*line2++ = ss;
						*line3++ = ss;
						*line3++ = ss;
					}
					
					ss = *s++;
					if (extraColumns == 2) {
						line2 += 2;
						line3 += 2;
					} else {
						*line2++ = ss;
						line2++;
						*line3++ = ss;
						line3++;	
					}
				}
			
				bits = (bits << 1);
				bitsleft--;
			} else {
			
				line0 += 4;
				line1 += 4;
				line2 += 4;
				line3 += 4;
				bits = (bits << 1);
				bitsleft--;
			}
		}

		line0 += makeup;
		line1 += makeup;
		line2 += makeup;
		line3 += makeup;
	}
	
	if (extraLines) {
		for (h = (width >> 2); h--;) {
			if (!bitsleft) {
				for (bits = 0, n = 4; n--;)	{
					bits = (bits << 8) | *data++;
				}
				bitsleft = 32;
			}
			
			if (bits < 0) {	
			
				bits = (bits << 1);
				bitsleft--;
				if (!bitsleft) {
					for (bits = n = 0; n < 4; n++)	{
						bits = (bits << 8) | *data++;
					}
					bitsleft = 32;
				}

				if (bits < 0) {	
					d = codebook + (*data++ << 2);
					*line0++ = *d++;
					*line0++ = *d++;
					if (extraLines > 1) {
						*line1++ = *d++;
						*line1++ = *d;
					} 
	
					d = codebook + (*data++ << 2);
					*line0++ = *d++;
					*line0++ = *d++;
					if (extraLines > 1) {
						*line1++ = *d++;
						*line1++ = *d;
					}
					
					d = codebook + (*data++ << 2);
					if (extraLines > 2) {
						*line2++ = *d++;
						*line2++ = *d;
					}
	
					d = codebook + (*data++ << 2);
					if (extraLines > 2) {
						*line2++ = *d++;
						*line2++ = *d;
					}
	
				} else {
					s = smooth + (*data++ << 2);
					ss = *s++;
					*line0++ = ss;
					*line0++ = ss;
					if (extraLines > 1) {
						*line1++ = ss;
						*line1++ = ss;
					}
	
					ss = *s++;
					*line0++ = ss;
					*line0++ = ss;
					if (extraLines > 1) {
						*line1++ = ss;
						*line1++ = ss;
					}
	
					ss = *s++;
					if (extraLines > 2) {
						*line2++ = ss;
						*line2++ = ss;
					}
	
					ss = *s++;
					if (extraLines > 2) {
						*line2++ = ss;
						*line2++ = ss;
					}
				}
			
				bits = (bits << 1);
				bitsleft--;

			} else {
			
				line0 += 4;
				line1 += 4;
				line2 += 4;
				bits = (bits << 1);
				bitsleft--;
			}
		}

		if (extraColumns) {
			if (!bitsleft) {
				for (bits = 0, n = 4; n--;)	{
					bits = (bits << 8) | *data++;
				}
				bitsleft = 32;
			}
			
			if (bits < 0) {	

				bits = (bits << 1);
				bitsleft--;
				if (!bitsleft) {
					for (bits = n = 0; n < 4; n++)	{
						bits = (bits << 8) | *data++;
					}
					bitsleft = 32;
				}

				if (bits < 0) {	
					d = codebook + (*data++ << 2);
					if (extraColumns == 1) {
						// only one column left
						*line0++ = *d++;
						line0++;
						if (extraLines > 1) {
							*line1++ = *++d;
							line1++;
						}
					} else {
						// at least two colums left
						*line0++ = *d++;
						*line0++ = *d++;
						if (extraLines > 1) {
							*line1++ = *d++;
							*line1++ = *d;
						}
					}
	
					d = codebook + (*data++ << 2);
					if (extraColumns == 2) {
						// two columns left
						line0 += 2;
						line1 += 2;
					} else {
						// three columns left
						*line0++ = *d++;
						line0++;
						if (extraLines > 1) 
							*line1++ = *++d;
						line1++;
					}
					
					d = codebook + (*data++ << 2);
					if (extraColumns == 1) {
						// only one column left
						if (extraLines > 2) {
							*line2++ = *d++;
							line2++;
						}
					} else {
						// at least two colums left
						if (extraLines > 2) {
							*line2++ = *d++;
							*line2++ = *d;
						}
					}
	
					d = codebook + (*data++ << 2);
					if (extraColumns == 2) {
						// two columns left
						line2 += 2;
						line3 += 2;
					} else {
						// three columns left
						if (extraLines > 2) 
							*line2++ = *d++;
						line2++;
					}
	
				} else {
					s = smooth + (*data++ << 2);
					ss = *s++;
					if (extraColumns == 1) {
						*line0++ = ss;
						line0++;
						if (extraLines > 1) 
							*line1++ = ss;
						line1++;
					} else {
						*line0++ = ss;
						*line0++ = ss;
						if (extraLines > 1) {
							*line1++ = ss;
							*line1++ = ss;
						}
					}
	
					ss = *s++;
					if (extraColumns == 2) {
						line0 += 2;
						line1 += 2;
					} else {
						*line0++ = ss;
						line0++;
						if (extraLines > 1)
							*line1++ = ss;
						line1++;
					}
	
					ss = *s++;
					if (extraColumns == 1) {
						if (extraLines > 2)
							*line2++ = ss;
						line2++;
					} else {
						if (extraLines > 2) {
							*line2++ = ss;
							*line2++ = ss;
						}
					}
					
					ss = *s++;
					if (extraColumns == 2) {
						line2 += 2;
					} else {
						if (extraLines > 2)
							*line2++ = ss;
						line2++;
					}
				}
				bits = (bits << 1);
				bitsleft--;

			} else {
			
				line0 += 4;
				line1 += 4;
				line2 += 4;
				bits = (bits << 1);
				bitsleft--;
			}
		}
	}
}

void cpDrawIntraframe8(uchar *baseAddr, long rowBytes, long width, long height,
	long realWidth, long realHeight, long *codebook, uchar *data)
{
	long	*line0,*line1,*line2,*line3,makeup;
	unsigned long	c0,c1;
	long	*smooth,*s;
	long	bits,bitsleft;
	short	h,v,n;
	
	makeup = rowBytes;
	rowBytes >>= 2;				// Convert to longs
	width >>= 2;
		
	smooth = codebook + 256;	// Detail codebook is 256*4 bytes, smooth is 256*16
	line0 = (long *)baseAddr;
	line1 = line0 + rowBytes;
	line2 = line1 + rowBytes;
	line3 = line2 + rowBytes;
	
	long lineDelta = height - realHeight;
	long extraLines = 0;
	if (lineDelta > 0 && lineDelta < 4) {
		height = (realHeight >> 2) << 2;
		extraLines = realHeight - height;
	}

	bitsleft = 1;
	for (v = (height >> 2); v--;) {
		for (h = 0; h < width; h++) {
			if (--bitsleft == 0) {
				for (bits = 0, n = 4; n--;)
					bits = (bits << 8) | *data++;
				bitsleft = 32;
			}
			
			if (bits < 0) {	
				
				c0 = codebook[data[0]];		// Detail position 0,1
				c1 = codebook[data[1]];
				
				#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
				line0[h] = (c1 & 0xFFFF0000) | (c0 >> 16);
				line1[h] = (c1 << 16) | (c0 & 0x0FFFF);
				#else
				line0[h] = (c0 & 0xFFFF0000) | (c1 >> 16);
				line1[h] = (c0 << 16) | (c1 & 0x0FFFF);
				#endif				
				
				c0 = codebook[data[2]];		// Detail position 2,3
				c1 = codebook[data[3]];

				#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
				line2[h] = (c1 & 0xFFFF0000) | (c0 >> 16);
				line3[h] = (c1 << 16) | (c0 & 0x0FFFF);
				#else
				line2[h] = (c0 & 0xFFFF0000) | (c1 >> 16);
				line3[h] = (c0 << 16) | (c1 & 0x0FFFF);
				#endif				

				data += 4;
								
			} else {
						
				s = smooth + (*data++ << 2);	// Smooth
				line0[h] = s[0];
				line1[h] = s[1];
				line2[h] = s[2];
				line3[h] = s[3];
				
			}
			
			bits = (bits << 1);
		}
		line0 += makeup;
		line1 += makeup;
		line2 += makeup;
		line3 += makeup;
	}
	if (extraLines) {
		for (h = 0; h < width; h++) {
			if (--bitsleft == 0) {
				for (bits = 0, n = 4; n--;)
					bits = (bits << 8) | *data++;
				bitsleft = 32;
			}
			
			if (bits < 0) {	
				
				c0 = codebook[data[0]];		// Detail position 0,1
				c1 = codebook[data[1]];
				
				#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
				line0[h] = (c1 & 0xFFFF0000) | (c0 >> 16);
				if (extraLines > 1)
					line1[h] = (c1 << 16) | (c0 & 0x0FFFF);
				#else
				line0[h] = (c0 & 0xFFFF0000) | (c1 >> 16);
				if (extraLines > 1)
					line1[h] = (c0 << 16) | (c1 & 0x0FFFF);
				#endif				
				
				c0 = codebook[data[2]];		// Detail position 2,3
				c1 = codebook[data[3]];

				#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
				if (extraLines > 2)
					line2[h] = (c1 & 0xFFFF0000) | (c0 >> 16);
				#else
				if (extraLines > 2)
					line2[h] = (c0 & 0xFFFF0000) | (c1 >> 16);
				#endif				

				data += 4;
								
			} else {
						
				s = smooth + (*data++ << 2);	// Smooth
				line0[h] = s[0];
				if (extraLines > 1)
					line1[h] = s[1];
				if (extraLines > 2)
					line2[h] = s[2];
				
			}
			
			bits = (bits << 1);
		}
	}
}

/*	Draw an interframe	*/

void cpDrawInterframe8(uchar *baseAddr, long rowBytes, long width, long height,
	long realWidth, long realHeight, long *codebook, uchar *data)
{
	long	*line0,*line1,*line2,*line3,makeup;
	unsigned long	c0,c1;
	long	*smooth,*s;
	long	bits,bitsleft;
	short	h,v,n;
	
	makeup = rowBytes;
	rowBytes >>= 2;				// Convert to longs
	width >>= 2;
	
	long lineDelta = height - realHeight;
	long extraLines = 0;
	if (lineDelta > 0 && lineDelta < 4) {
		height = (realHeight >> 2) << 2;
		extraLines = realHeight - height;
	}

	smooth = codebook + 256;	// Detail codebook is 256*4 bytes, smooth is 256*16
	line0 = (long *)baseAddr;
	line1 = line0 + rowBytes;
	line2 = line1 + rowBytes;
	line3 = line2 + rowBytes;
	
	bitsleft = 1;
	for (v = (height >> 2); v--;) {
		for (h = 0; h < width; h++) {
			if (--bitsleft == 0) {
				for (bits = 0, n = 4; n--;)
					bits = (bits << 8) | *data++;
				bitsleft = 32;
			}
			
			if (bits < 0) {	
			
				bits = (bits << 1);
				if (--bitsleft == 0) {
					for (bits = 0, n = 4; n--;)
						bits = (bits << 8) | *data++;
					bitsleft = 32;
				}

				if (bits < 0) {	

					c0 = codebook[data[0]];		// Detail position 0,1
					c1 = codebook[data[1]];

					#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
					line0[h] = (c1 & 0xFFFF0000) | (c0 >> 16);
					line1[h] = (c1 << 16) | (c0 & 0x0FFFF);
					#else
					line0[h] = (c0 & 0xFFFF0000) | (c1 >> 16);
					line1[h] = (c0 << 16) | (c1 & 0x0FFFF);
					#endif				
					
					c0 = codebook[data[2]];		// Detail position 2,3
					c1 = codebook[data[3]];
	
					#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
					line2[h] = (c1 & 0xFFFF0000) | (c0 >> 16);
					line3[h] = (c1 << 16) | (c0 & 0x0FFFF);
					#else
					line2[h] = (c0 & 0xFFFF0000) | (c1 >> 16);
					line3[h] = (c0 << 16) | (c1 & 0x0FFFF);
					#endif				

					data += 4;
										
				} else {
				
					s = smooth + (*data++ << 2);	// Smooth
					line0[h] = s[0];
					line1[h] = s[1];
					line2[h] = s[2];
					line3[h] = s[3];
					
				}
			}
			
			bits = (bits << 1);
		}
		line0 += makeup;
		line1 += makeup;
		line2 += makeup;
		line3 += makeup;
	}
	if (extraLines) {
		for (h = 0; h < width; h++) {
			if (--bitsleft == 0) {
				for (bits = 0, n = 4; n--;)
					bits = (bits << 8) | *data++;
				bitsleft = 32;
			}
			
			if (bits < 0) {	
			
				bits = (bits << 1);
				if (--bitsleft == 0) {
					for (bits = 0, n = 4; n--;)
						bits = (bits << 8) | *data++;
					bitsleft = 32;
				}

				if (bits < 0) {	

					c0 = codebook[data[0]];		// Detail position 0,1
					c1 = codebook[data[1]];

					#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
					line0[h] = (c1 & 0xFFFF0000) | (c0 >> 16);
					if (extraLines > 1)
						line1[h] = (c1 << 16) | (c0 & 0x0FFFF);
					#else
					line0[h] = (c0 & 0xFFFF0000) | (c1 >> 16);
					if (extraLines > 1)
						line1[h] = (c0 << 16) | (c1 & 0x0FFFF);
					#endif				
					
					c0 = codebook[data[2]];		// Detail position 2,3
					c1 = codebook[data[3]];
	
					#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
					if (extraLines > 2)
						line2[h] = (c1 & 0xFFFF0000) | (c0 >> 16);
					#else
					if (extraLines > 2)
						line2[h] = (c0 & 0xFFFF0000) | (c1 >> 16);
					#endif				

					data += 4;
										
				} else {
				
					s = smooth + (*data++ << 2);	// Smooth
					line0[h] = s[0];
					if (extraLines > 1)
						line1[h] = s[1];
					if (extraLines > 2)
						line2[h] = s[2];
				}
			}
			
			bits = (bits << 1);
		}
	}
}
