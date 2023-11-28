#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <OS.h>
#include <GraphicsDefs.h>

extern "C" {
};
char *buffer;
char *obuffer;

short cpDecompress(void *ptr,unsigned char *data,unsigned char *baseAddr);
void *cpDecompressInit(long real_width,long real_height,color_space cs);
void cpDecompressCleanup(void *ptr);
void YUY2toRGB32(uint8 *src,uint32 *dest,int32 width,int32 height,int32 rowbytes);

#define WIDTH 304
#define HEIGHT 228
//#define WIDTH 160
//#define HEIGHT 120

void
main(int argc,char *argv[])
{
	FILE *FIn;
	int len;
	int z;
	long        *codebook;
	int depth=15;
	color_space cs=B_RGB32;//B_YCbCr422;//B_YCbCr422;
	int bpp=(depth+7)>>3;
	
	void *ptr=cpDecompressInit(WIDTH,HEIGHT,cs);
	char *yuybuffer=NULL;
	
	buffer=(char*)malloc(WIDTH*HEIGHT*4);
	obuffer=(char*)malloc(WIDTH*HEIGHT*4);
	if (cs==B_YCbCr422) yuybuffer=(char*)malloc(WIDTH*HEIGHT*4);
	
	for (z=1;z<11;z++)
	{
		char fname[120];

		sprintf(fname,"cframes/frame%03d.cin",z);
//		printf("Open '%s'\n",fname);
		FIn=fopen(fname,"rb");
		fseek(FIn,0,SEEK_END);
		len=ftell(FIn);
		fseek(FIn,0,SEEK_SET);
//		memset(buffer,0,160*120*4);
		fread(buffer,1,len,FIn);
		fclose(FIn);
		
#if 0
bigtime_t t=system_time();
		for (int32 t=0;t<10000;t++)
#endif
		cpDecompress(ptr,(unsigned char *)buffer,
					 (unsigned char *)obuffer);
#if 0
	t=system_time()-t;
	printf("time=%g\n",(t/1000000.0));
#endif
#if 1
		sprintf(fname,"/tmp/output%03d.ppm",z);
		FIn=fopen(fname,"wb");
		{
			int x,y;
			fprintf(FIn,"P6\n%d %d\n255\n",WIDTH,HEIGHT);
			if (cs==B_RGB32)
			{
				for (x=0;x<(WIDTH*HEIGHT);x++)
				{
#if 0
					printf("%02x %02x %02x\n",
						obuffer[(x*4)+2],
						obuffer[(x*4)+1],
						obuffer[(x*4)+0]
						);
#endif
					fprintf(FIn,"%c%c%c",
						obuffer[(x*4)+2],
						obuffer[(x*4)+1],
						obuffer[(x*4)+0]
						);
				}
			}
			else
			if (cs==B_RGB16)
			{
				unsigned short *p=(unsigned short*)obuffer;
				for (x=0;x<(WIDTH*HEIGHT);x++)
				{
#if 0
					printf("%02x %02x %02x %04x\n",
						(p[x]>>11)<<3,
						((p[x]>>5)&0x3f)<<2,
						((p[x]&0x1f)<<3),p[x]
						);
#endif
					fprintf(FIn,"%c%c%c",
						(p[x]>>11)<<3,
						((p[x]>>5)&0x3f)<<2,
						((p[x]&0x1f)<<3)
						);
				}
			}
			else
			if (cs==B_RGB15)
			{
				unsigned short *p=(unsigned short*)obuffer;
				for (x=0;x<(WIDTH*HEIGHT);x++)
				{
#if 0
					printf("%02x %02x %02x %04x\n",
						(p[x]>>10)<<3,
						((p[x]>>5)&0x1f)<<3,
						((p[x]&0x1f)<<3),p[x]
						);
#endif
					fprintf(FIn,"%c%c%c",
						(p[x]>>10)<<3,
						((p[x]>>5)&0x1f)<<3,
						((p[x]&0x1f)<<3)
						);
				}
			}
			else
			if (cs==B_YCbCr422)
			{
	//			memset(obuffer,0,WIDTH*HEIGHT);memset(obuffer+WIDTH*HEIGHT,255,WIDTH*HEIGHT);
#if 0
				YUY2toRGB32((uchar*)obuffer,(uint32*)yuybuffer,WIDTH,HEIGHT,WIDTH*2);
				for (x=0;x<(WIDTH*HEIGHT);x++)
				{
					fprintf(FIn,"%c%c%c",
						yuybuffer[(x*4)+2],
						yuybuffer[(x*4)+1],
						yuybuffer[(x*4)+0]
						);
				}
#else
				for (x=0;x<(WIDTH*HEIGHT);x++)
				{
					fprintf(FIn,"%c%c%c",
						obuffer[(x*2)+0],
						obuffer[(x*2)+1],0
						);
				}
#endif
			}
		}
		fclose(FIn);
#endif
	}
	cpDecompressCleanup(ptr);
}
