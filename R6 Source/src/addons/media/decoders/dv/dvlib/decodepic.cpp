#include <stdio.h>
#include <string.h>
#include <InterfaceKit.h>
#include <AppKit.h>
#define DVASM

extern "C" {
#include <windows.h>
#include "csedv.h"
};
#include "dvcodec.h"


#define CS B_RGB32
int main(int argc,char *argv[])
{
	FILE *FIn;
	FILE *FOut;
	char inputdata[144000];
	char outputdata[720*576*4];
	int32 dvsize=120000;
	SetupCodec();
	
	memset(outputdata,0,720*576*4);
	if (argc!=3) return 1;
	FIn=fopen(argv[1],"rb");
	fseek(FIn,0,SEEK_END);
	dvsize=ftell(FIn);
	if (dvsize!=144000 && dvsize!=120000)
	{
		printf("File Size should be 144000 (PAL) or 120000 (NTSC).\n");
		fclose(FIn);
		return 0;
	}

	fseek(FIn,0,SEEK_SET);
	fread(inputdata,1,dvsize,FIn);
	fclose(FIn);

bigtime_t t=system_time();
#define REPS 1
for (int32 z=0;z<REPS;z++)
	if (dvsize==120000)
		DecodeDVNTSC(inputdata,outputdata,CS);
	else
		DecodeDVPAL(inputdata,outputdata,CS);
t=system_time()-t;
printf("fps=%g\n",REPS/(t/1000000.0));

printf("decoding, dvsize=%d\n",dvsize);

	FOut=fopen(argv[2],"wb");
	int32 pixels=720*(dvsize==120000?480:576);
#if 0
	fwrite(outputdata,1,pixels*2,FOut);
#else
	fprintf(FOut,"P6\n720 %d\n255\n",dvsize==120000?480:576);
	for (int32 z=0;z<pixels;z++)
	{
#if 1
		fputc(outputdata[(z*4)+2],FOut);	//2 red -> 2
		fputc(outputdata[(z*4)+1],FOut);	//1 green->0
		fputc(outputdata[(z*4)+0],FOut);	//0 blue->?
#endif
#if 0	// RGB16
		unsigned short d=*((unsigned short *)&outputdata[z*2]);
		fputc(((d>>11)&0x1f)<<3,FOut);	//2 red -> 2
		fputc(((d>>4)&0x3f)<<2,FOut);	//1 green->0
		fputc(((d>>0)&0x1f)<<3,FOut);	//0 blue->?
#endif
	}
#endif
	fclose(FOut);
}
