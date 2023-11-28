#include <OS.h>
#include <stdio.h>
#include <string.h>
#include <GraphicsDefs.h>
extern "C" {
#include "beos.h"
#include "iccv.h"
void DebugBreak(void);
void DrawKey32(void);
void DrawKey15(void);
extern WORD TQuality;
extern WORD SQuality;
};

long MulDiv(long x,long y,long z)
{
	return (x*y)/z;
}

// rotate val bits bits to the right...
unsigned long _lrotr(unsigned long val, int bits)
{
	return val;
}

void DebugBreak(void)
{
	debugger("test");
}

static sem_id mysem=-1;
long semcount=0;

void *cpDecompressInit(long real_width,long real_height,color_space cs)
{
	int depth;

	if (mysem==-1) mysem=create_sem(0,"cinepak sem");
//printf("initializing, %d,%d,%d\n",real_width,real_height,cs);
	switch (cs)
	{
		case B_RGB32:
		case B_RGB32_BIG:
			depth=32;
			break;
		case B_RGB16:
		case B_RGB16_BIG:
			depth=16;
			break;
		case B_RGB15:
		case B_RGB15_BIG:
			depth=15;
			break;
		case B_YCbCr422:
			depth=16;
			break;
		default:
			printf("Unsupported color_space=%08x\n",cs);
			break;
	}
	set_area_protection(area_for((void*)DrawKey32), B_READ_AREA | B_WRITE_AREA);
	set_area_protection(area_for((void*)DrawKey15), B_READ_AREA | B_WRITE_AREA);
	
	INSTINFO *pI=new INSTINFO;
	memset(pI,0,sizeof(INSTINFO));
	
	ICDECOMPRESSEX *px=&pI->px;

	BITMAPINFOHEADER *biSrc=px->lpbiSrc=new BITMAPINFOHEADER;
	memset(biSrc,0,sizeof(BITMAPINFOHEADER));
	biSrc->biCompression=BI_CV;
	biSrc->biBitCount=depth;
	biSrc->biWidth=real_width;
	biSrc->biHeight=real_height;

	BITMAPINFOHEADER *biDst=px->lpbiDst=new BITMAPINFOHEADER;
	memset(biDst,0,sizeof(BITMAPINFOHEADER));
	if (cs==B_YCbCr422)
		biDst->biCompression=BI_YUY2;
	else
		biDst->biCompression=BI_RGB;
	biDst->biPlanes=1;
	biDst->biBitCount=depth;
	biDst->biWidth=real_width;
	biDst->biHeight=-real_height;		// negative height, or image is upside down
	
	px->dxSrc=-1;
	px->dySrc=-1;
	px->dxDst=-1;
	px->dyDst=-1;
	
	return pI;
}

int
cpDecompress(void *ptr,unsigned char *data,unsigned char *baseAddr)
{
	if (atomic_add(&semcount,1)>0) acquire_sem(mysem);
	
	INSTINFO *pI=(INSTINFO*)ptr;
	ICDECOMPRESSEX *px=&pI->px;
	
	px->lpSrc=data;
	px->lpDst=baseAddr;

	pI->DStuff.Flags=0;
	
	if (Decompress(pI,0,
		px->lpbiSrc,px->lpSrc,
		px->xSrc,px->ySrc,px->dxSrc,px->dySrc,
		px->lpbiDst,px->lpDst,
		px->xDst,px->yDst,px->dxDst,px->dyDst)!=ICERR_OK)
		printf("decompress error\n");

	if (atomic_add(&semcount,-1)>1) release_sem(mysem);
	return 0;
}

void cpDecompressCleanup(void *ptr)
{
	delete_sem(mysem);
	if (INSTINFO *pI=(INSTINFO*)ptr)
		DecompressEnd(pI);
}

void *cpCompressInit(long real_width,long real_height,color_space cs)
{
	int depth;

	switch (cs)
	{
		case B_RGB32:
		case B_RGB32_BIG:
			depth=32;
			break;
		case B_RGB16:
		case B_RGB16_BIG:
			depth=16;
			break;
		case B_RGB15:
		case B_RGB15_BIG:
			depth=15;
			break;
		default:
			printf("Unsupported color_space=%08x\n",cs);
			break;
	}
	set_area_protection(area_for((void*)DrawKey32), B_READ_AREA | B_WRITE_AREA);
	set_area_protection(area_for((void*)DrawKey15), B_READ_AREA | B_WRITE_AREA);
	
	INSTINFO *pI=new INSTINFO;
	memset(pI,0,sizeof(INSTINFO));
	
	pI->last_keyframe = -1;

	ICCOMPRESS *icc=&pI->icc;
	icc->lFrameNum=-1;
	BITMAPINFOHEADER *biSrc=icc->lpbiInput=new BITMAPINFOHEADER;
	memset(biSrc,0,sizeof(BITMAPINFOHEADER));
	if (cs==B_YCbCr422)
		biSrc->biCompression=BI_YUY2;
	else
		biSrc->biCompression=BI_RGB;
	biSrc->biBitCount=depth;
	biSrc->biWidth=real_width;
	biSrc->biHeight=real_height;
	biSrc->biPlanes=1;

	BITMAPINFOHEADER *biDst=icc->lpbiOutput=new BITMAPINFOHEADER;
	memset(biDst,0,sizeof(BITMAPINFOHEADER));
	biDst->biCompression=BI_CV;
	biDst->biBitCount=depth;
	biDst->biWidth=real_width;
	biDst->biHeight=real_height;		// negative height, or image is upside down
	//CompressBegin(pI,biSrc,biDst);
	return pI;
}

void cpSetQuality(float quality)
{
	if (quality<0.0) quality=0.0;
	if (quality>1.0) quality=1.0;
	SQuality=(WORD)(quality*1023);
	TQuality=(WORD)(quality*1023);
}

int
cpCompress(void *ptr,unsigned char *data,unsigned char *baseAddr,int32 *keyframe)
{
	INSTINFO *pI=(INSTINFO*)ptr;
	ICCOMPRESS *icc=&pI->icc;
	int32 len;
	
	icc->lpInput=data;
	icc->lpOutput=baseAddr;

	pI->DStuff.Flags = 0;
	
	/*
	    if the user asked for a keyframe or more than 30 frames
		have gone by without a keyframe then ask for one.
	*/	
	if (*keyframe || ((pI->last_keyframe+29) < icc->lFrameNum))
		icc->dwFlags = ICCOMPRESS_KEYFRAME;
	else
		icc->dwFlags = 0;

	icc->lFrameNum++;
	len = Compress(pI,&pI->icc,0);
	if (len <= 0)
		printf("compress error\n");

	*keyframe = pI->CStuff.KeyFrame;
	if (*keyframe)
		pI->last_keyframe = icc->lFrameNum-1;

	return len;
}

void cpCompressCleanup(void *ptr)
{
//	delete_sem(mysem);
	if (INSTINFO *pI=(INSTINFO*)ptr)
		CompressEnd(pI);
}
