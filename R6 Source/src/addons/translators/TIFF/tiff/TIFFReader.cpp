
#include "TIFFReader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define	CVT(x)		((x) / 256)

static int
checkcmap(int n, uint16* r, uint16* g, uint16* b)
{
	while (n-- > 0)
	    if (*r++ >= 256 || *g++ >= 256 || *b++ >= 256)
		return (16);
	return (8);
}

static bool
read_8bits_cmap_strip(TIFF *tif, rgb_color *data, int dw, int left, int top, int right, int bottom)
{
	bool 	ok = TRUE;
	int		i, j;
	uint16	bps, spp;

	uchar	*scanline = (uchar*)malloc(TIFFScanlineSize(tif)*sizeof(uchar));	

//fprintf(stderr,"read_8bits_cmap_strip %d : (%d,%d,%d,%d)\n",dw,left,top,right,bottom);
	/* get color map */	
	uint16 	*rmap, *gmap, *bmap;
	int		cmap_size;
	TIFFGetField(tif,TIFFTAG_BITSPERSAMPLE,&bps);				// should be 4 or 8
	TIFFGetField(tif,TIFFTAG_SAMPLESPERPIXEL,&spp);			// should be 1
	TIFFGetField(tif,TIFFTAG_COLORMAP,&rmap,&gmap,&bmap);
	cmap_size = checkcmap(1<<bps, rmap, gmap, bmap);
	if (cmap_size == 16) {										// convert to 8bit cmap
		for (i=0; i<(1<<bps); i++) {
			rmap[i] = CVT(rmap[i]);	
			gmap[i] = CVT(gmap[i]);
			bmap[i] = CVT(bmap[i]);
		}
	}

	/* decode */
	rgb_color	*clr;
	uchar		*sl;
	for (i=top;i<bottom;i++) {
		if (TIFFReadScanline(tif,scanline,i,0)<0)		{ok = FALSE; break;}
		clr = &data[(i-top)*dw+left];
		sl = &scanline[left];
		for (j=left;j<right;j++,clr++,sl++) {
			clr->red = rmap[*sl];
			clr->green = gmap[*sl];
			clr->blue = bmap[*sl];
		}
	}	

	free(scanline);
	return ok;	
}

static bool
read_8bits_rgb_planar_strip(TIFF *tif, rgb_color *data, int dw, int left, int top, int right, int bottom)
{
	bool 	ok = TRUE;
	int		i, j, k;
	uint16	bps, spp;

	TIFFGetField(tif,TIFFTAG_BITSPERSAMPLE,&bps);				// should be 8
	TIFFGetField(tif,TIFFTAG_SAMPLESPERPIXEL,&spp);				// should be 3

	tsize_t rowbytes = TIFFScanlineSize(tif);
	uchar	*scanline = (uchar*)malloc(rowbytes*spp);	

//fprintf(stderr,"read_8bits_rgb_planar_strip %d : (%d,%d,%d,%d)\n",dw,left,top,right,bottom);
	
	/* decode */
	rgb_color	*clr;
	uchar		*r, *g, *b;
	for (i=top;i<bottom;i++) {

		/* read */
		for (k=0;k<spp;k++) 
			if (TIFFReadScanline(tif,scanline+k*rowbytes,i,k)<0)	ok = FALSE;
		if (!ok)	break;
		
		/* span data */
		clr = &data[(i-top)*dw+left];
		r = &scanline[left];	g = &scanline[rowbytes+left];	b = &scanline[2*rowbytes+left];
		for (j=left;j<right;j++,clr++) {
			clr->red = *r++;
			clr->green = *g++;
			clr->blue = *b++;
		}
	}	

	free(scanline);
	return ok;	
}

static bool
read_8bits_rgb_meshed_strip(TIFF *tif, rgb_color *data, int dw, int left, int top, int right, int bottom)
{
	bool 	ok = TRUE;
	int		i, j;

	tsize_t rowbytes = TIFFScanlineSize(tif);
	uchar	*scanline = (uchar*)malloc(rowbytes);
		
//fprintf(stderr,"read_8bits_rgb_meshed_strip %d : (%d,%d,%d,%d)\n",dw,left,top,right,bottom);
	/* decode */
	rgb_color	*clr;
	uchar		*sl;
	for (i=top;i<bottom;i++) {

		/* read */
//fprintf(stderr,"read line nb %d\n",i);
		if (TIFFReadScanline(tif,scanline,i,0)<0)	ok = FALSE;
		if (!ok)	break;
		
		/* span data */
		clr = &(data[(i-top)*dw+left]);
		sl = &(scanline[left]);
//fprintf(stderr,"now copying line...0x%ld 0x%ld\n",(long)clr,(long)sl);
		for (j=left;j<right;j++,clr++) {
			clr->red = *sl++;
			clr->green = *sl++;
			clr->blue = *sl++;
		}
	}	

	free(scanline);
	return ok;	
}



TIFFReader::TIFFReader(const char *name)
{
	tiff = TIFFOpen(name,"r");
}

TIFFReader::~TIFFReader()
{
	if (tiff)	TIFFClose(tiff);
}

bool
TIFFReader::CanReadImage()
{
	if (!tiff)		return FALSE;

	uint16	spp, bps;
	uint16	colorspace, config, compression;
	
	TIFFGetField(tiff,TIFFTAG_PHOTOMETRIC,&colorspace);
	TIFFGetField(tiff,TIFFTAG_BITSPERSAMPLE,&bps);
	TIFFGetField(tiff,TIFFTAG_SAMPLESPERPIXEL,&spp);
	TIFFGetField(tiff,TIFFTAG_PLANARCONFIG,&config);
	TIFFGetField(tiff,TIFFTAG_COMPRESSION,&compression);
	if (TIFFIsTiled(tiff) || 
		(colorspace != PHOTOMETRIC_RGB && colorspace != PHOTOMETRIC_PALETTE) ||
		(bps != 8) ||
		(compression>COMPRESSION_LZW && compression!=COMPRESSION_PACKBITS)) {
		return FALSE;
	}
	else
		return TRUE;	
}

void
TIFFReader::GetBkColor(rgb_color *color)
{
	color->red = color->green = color->blue = 255;
}

int 
TIFFReader::GetImageSize(int *w, int *h)
{
	uint32	tw=0, th=0;
	if (tiff) {
		TIFFGetField(tiff,TIFFTAG_IMAGEWIDTH,&tw);
		TIFFGetField(tiff,TIFFTAG_IMAGELENGTH,&th);
	}
	*w = tw; *h = th;
	return 0;
}

int 
TIFFReader::GetImageResolution(float *x, float *y)
{
	float	xres=0., yres=0.;
	if (tiff) {
		TIFFGetField(tiff,TIFFTAG_XRESOLUTION,&xres);
		TIFFGetField(tiff,TIFFTAG_YRESOLUTION,&yres);
	}
	*x = xres; *y = yres;
	return 0;
}

bool
TIFFReader::GetImageData(rgb_color *data)
{
	uint32 tw, th;
	bool ok = FALSE;
	if (tiff) {
		TIFFGetField(tiff,TIFFTAG_IMAGEWIDTH,&tw);
		TIFFGetField(tiff,TIFFTAG_IMAGELENGTH,&th);
		ok = ReadBlock(data,tw,th,0,0,tw,th);
	}
	return ok;
}

bool
TIFFReader::ReadBlock(rgb_color *data, int dw, int dh, int l, int t, int r, int b)
{
	bool (*fn)(TIFF*,rgb_color*,int,int,int,int,int);

	uint16	bps, colorspace, config;
	bool ok = FALSE;
	
	if (tiff) {		
		TIFFGetField(tiff,TIFFTAG_BITSPERSAMPLE,&bps);
		TIFFGetField(tiff,TIFFTAG_PHOTOMETRIC,&colorspace);
		TIFFGetField(tiff,TIFFTAG_PLANARCONFIG,&config);

		switch(bps) {
		case 8 :
			switch(colorspace) {
			case PHOTOMETRIC_PALETTE:				fn = read_8bits_cmap_strip; break;
			case PHOTOMETRIC_RGB:
				if (config==PLANARCONFIG_SEPARATE)	fn = read_8bits_rgb_planar_strip;
				else								fn = read_8bits_rgb_meshed_strip;
				break;
			default :								fn = NULL; break;
			}	
			break;
		default :									fn = NULL; break;
		}
	
		if (fn)				ok = fn(tiff,data,dw,l,t,r,b);	
	}
	
	return ok;
}






