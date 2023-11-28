#include 	"Cinepak.h"

extern long LONG(void *l);
extern long SHORT(void *l);

/*	Interpret and expand a codebook		*/

long	AtomSize(Byte *d) {
	return (d[1]<<16) | (d[2]<<8) | d[3];
}

/*================================================================================================*/

/*	Range to 0-255	*/

inline Byte clip255(short x)
{
	if (x < 0)
		return 0;
	if (x > 0xFF)
		return (0xFF);
	return x;
}

/*================================================================================================*/

//	Expand a code vector YUV to RGB

void cpExpandCodeVector(Byte *data, Byte *dst)
{
	short	u,v,uv,i;	
	u = (signed char)data[4] << 1;
	v = (signed char)data[5] << 1;
	uv = ((v >> 1) + u) >> 1;
	
//	Convert the YUV to RGB pixels
	
	for (i = 0; i < 4; i++)	{
#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
		dst[3] = clip255(data[i] + u);	// xRGB
		dst[2] = clip255(data[i] - uv);
		dst[1] = clip255(data[i] + v);
		dst[0] = 0;
#else	
		dst[0] = clip255(data[i] + u);	// BGRx
		dst[1] = clip255(data[i] - uv);
		dst[2] = clip255(data[i] + v);
		dst[3] = 0;
#endif
		dst += 4;
	}
}

inline short cpMAP(short p) {
	p += 4;
	if (p < 0)
		return 0;
	if (p > 0xFF)
		return 0xF8;
	return p & 0xF8;
}

//	Dithering tables could be greatly simplified...
//	They show full calculations for tweaking.

void cpExpandCodeVector8(Byte *data, Byte *dst)
{
	uchar *map = (Byte *)&system_colors()->index_map;

	short	u,v,uv,i;	
	u = (signed char)data[4] << 1;
	v = (signed char)data[5] << 1;
	uv = ((v >> 1) + u) >> 1;
	
//	Convert the YUV to RGB pixels to indexes in map
	
	short dither[4] = { 0,8,12,4 };
	long y;
	for (i = 0; i < 4; i++) {
		y = data[i] + (dither[i] - 6)*3;
		short a = map[(cpMAP(y + v) << 7) | (cpMAP(y - uv) << 2) | cpMAP(y + u) >> 3];
		dst[i] = a;
	}
}

void cpExpandCodeVectorSmooth8(Byte *data, Byte *dst)
{
	uchar *map = (Byte *)&system_colors()->index_map;

	short	u,v,uv,i;	
	u = (signed char)data[4] << 1;
	v = (signed char)data[5] << 1;
	uv = ((v >> 1) + u) >> 1;
	
//	Convert the YUV to RGB pixels to indexes in map
	
	short j = 0;
	short scan[16] = { 0,1,4,5,2,3,6,7, 8,9,12,13,10,11,14,15 };
	short dither[16] = { 0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5 };
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

/*================================================================================================*/

void cpExpandCodeBook(Byte *data, long *codeBook, long depth)
{
	long	codeBookType,size,codeSize,c;
	void	(*proc)(Byte *data, Byte *code);
	
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
	}
	
	switch (codeBookType) {			// Full Codebook
		case kFullDBookType:
		case kFullSBookType:
		for (c = (size - 4)/6; c--;) {
			(proc)(data,(Byte *)codeBook);
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
					(proc)(data,(Byte *)codeBook);
					data += 6;
				}
				codeBook += codeSize;
				bits = (bits << 1);
			}
	}
}

/*================================================================================================*/

short	cpDecompress(Byte *data, Byte *baseAddr,long rowBytes, long *codebook, long depth)
{
	Byte	*frameEnd,*tileEnd,tileType,frameType;
	short	t,tileCount,width,height;
	
	long	codeBookSize;
	long*	smooth;
	
	if (depth == 32) {
		codeBookSize = 256*4 + 256*4;	// 32 bit codebook is 8k (codeBookSize is in longs)
		smooth = codebook + 256*4;
	} else {
		codeBookSize = 256 + 256*4;		// 8 bit codebook is 5k
		smooth = codebook + 256;
	}
	
	frameType = *data;
	if (!(frameType == kKeyFrameType || frameType == kInterFrameType))
		return codecBadDataErr;
	frameEnd = data + AtomSize(data);
	tileCount = SHORT((void*)&((FrameHeader *)data)->frameTileCount);
	data += 10; // Skip header
		
/*	Loop through all tiles and find codebooks				*/
		
	if (tileCount > 4)
		tileCount = 4;	// Only support up to 4 tiles... (codebook propagation sucks down memory!)
	for (t = 0; t < tileCount; t++) {
		
/*		Parse atoms within frame looking for tiles			*/

		for (;;) {
			tileType = *data;
			if (tileType == kTileKeyType || tileType == kTileInterType)
				break;
			data += AtomSize(data);
			if (data >= frameEnd)
				return codecBadDataErr;
		}
		width = SHORT((void*)&((TileHeader *)data)->tileRectRight)
			- SHORT((void*)&((TileHeader *)data)->tileRectLeft);
		height = SHORT((void*)&((TileHeader *)data)->tileRectBottom)
			- SHORT((void*)&((TileHeader *)data)->tileRectTop);
		tileEnd = data + AtomSize(data);
		data += 12;
		
/*		if this is a keyframe with more than one tile then propagate codebook	*/

		if (t) {
			if (frameType == kKeyFrameType && tileType == kTileInterType)
				memcpy((Byte *)codebook + codeBookSize*4,codebook,codeBookSize*4);
			codebook += codeBookSize;
			smooth += codeBookSize;
		}
			
/*		Parse atoms within tile looking for codes			*/
			
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
						cpDrawIntraframe(baseAddr,rowBytes,width,height,codebook,data + 4);
					else
						cpDrawIntraframe8(baseAddr,rowBytes,width,height,codebook,data + 4);
					break;
				case kInterCodesType:
					if (depth == 32)
						cpDrawInterframe(baseAddr,rowBytes,width,height,codebook,data + 4);
					else
						cpDrawInterframe8(baseAddr,rowBytes,width,height,codebook,data + 4);
					break;
			}
			data += AtomSize(data);
		}
		baseAddr += height * rowBytes;
	}
	return 0;
}

