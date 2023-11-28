#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <OS.h>
#include <GraphicsDefs.h>

extern "C" {
};
char *buffer;
char *obuffer;

short cpCompress(void *ptr,unsigned char *data,unsigned char *baseAddr,int32 *keyframe);
void *cpCompressInit(long real_width,long real_height,color_space cs);
void cpCompressCleanup(void *ptr);
void cpSetQuality(float quality);
void YUY2toRGB32(uint8 *src,uint32 *dest,int32 width,int32 height,int32 rowbytes);

#define WIDTH 304
#define HEIGHT 228

main(int argc,char *argv[])
{
	FILE *FIn;
	int len;
	int z;
	long        *codebook;
	int depth=15;
	color_space cs=B_RGB32;//B_YCbCr422;//B_YCbCr422;
	int bpp=(depth+7)>>3;
	
	void *ptr=cpCompressInit(WIDTH,HEIGHT,cs);
	char *yuybuffer=NULL;
	int32 keyframe=0;
	
	cpSetQuality(0.7);
	buffer=(char*)malloc(WIDTH*HEIGHT*4);
	obuffer=(char*)malloc(WIDTH*HEIGHT*4);
	if (cs==B_YCbCr422) yuybuffer=(char*)malloc(WIDTH*HEIGHT*4);
	
	for (z=1;z<11;z++)
	{
		char fname[120];

		sprintf(fname,"frames/output%03d.ppm",z);
		printf("Open '%s'\n",fname);
		FIn=fopen(fname,"rb");
		fgets(buffer,WIDTH*HEIGHT,FIn);
		if (buffer[0]!='P' || buffer[1]!='6')
		{
			printf("Not a ppm file!\n");
			exit(0);
		}
		fgets(buffer,WIDTH*HEIGHT,FIn);
		int32 w,h;
		if (sscanf(buffer,"%d %d",&w,&h)!=2)
		{
			printf("2Not a ppm file!\n");
			exit(0);
		}
		printf("read %d %d\n",w,h);
		if (w!=WIDTH || h!=HEIGHT)
		{
			printf("Got %dx%d!\n",w,h);
			exit(0);
		}
		fgets(buffer,WIDTH*HEIGHT,FIn);
		if (cs==B_RGB32)
		{
			for (int32 x=0;x<(WIDTH*HEIGHT);x++)
			{
				buffer[(x*4)+2]=fgetc(FIn);
				buffer[(x*4)+1]=fgetc(FIn);
				buffer[(x*4)+0]=fgetc(FIn);
				buffer[(x*4)+3]=0;
			}
		}
		if (cs==B_RGB16)
		{
			unsigned short *p=(unsigned short*)buffer;
			for (int32 x=0;x<(WIDTH*HEIGHT);x++)
			{
				short val;
				
				val =((fgetc(FIn)>>3)<<11);
				val|=(((fgetc(FIn)>>2)&0x3f)<<5);
				val|=((fgetc(FIn)>>3)&0x1f);
				p[x]=val;
			}
		}
		else
		if (cs==B_RGB15)
		{
			unsigned short *p=(unsigned short*)buffer;
			for (int32 x=0;x<(WIDTH*HEIGHT);x++)
			{
				short val;
				
				val =((fgetc(FIn)>>3)<<10);
				val|=(((fgetc(FIn)>>3)&0x1f)<<5);
				val|=((fgetc(FIn)>>3)&0x1f);
				p[x]=val;
			}
		}
		else
		if (cs==B_YCbCr422)
		{
			for (int32 x=0;x<(WIDTH*HEIGHT);x++)
			{
				buffer[(x*2)+0]=fgetc(FIn);
				buffer[(x*2)+1]=fgetc(FIn);
				fgetc(FIn);
			}
		}
		fclose(FIn);
		
#if 0
bigtime_t t=system_time();
		for (int32 t=0;t<10000;t++)
#endif
printf("Compress\n");
		int32 len=cpCompress(ptr,(unsigned char *)buffer,
					 (unsigned char *)obuffer,&keyframe);
printf("done compress, keyframe=%d\n",keyframe);
#if 0
	t=system_time()-t;
	printf("time=%g\n",(t/1000000.0));
#endif
#if 1
		sprintf(fname,"/tmp/frame%03d.cin",z);
		FIn=fopen(fname,"wb");
		fwrite(obuffer,len,1,FIn);
		fclose(FIn);
#endif
	}
	cpCompressCleanup(ptr);
}
