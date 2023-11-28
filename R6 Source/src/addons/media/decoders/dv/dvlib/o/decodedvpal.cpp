#include <stdio.h>
#include <string.h>
#include <InterfaceKit.h>
#include <AppKit.h>
extern "C" {
#include <windows.h>
#include "csedv.h"
#include "putimage.h"
extern  void    InitDecParam( void );
extern  void    InitEncParam( void );
};

PBYTE PutImageBuffer;
int PutImageStride;

#define WIDTH 720
#define HEIGHT 576 //480 // 576 for PAL
#define DVSIZE 144000 //120000 // 144000 for PAL

int main(int argc,char *argv[])
{
	BYTE inputdata[DVSIZE];
	BYTE output[WIDTH*HEIGHT*4];
	FILE *FIn;
	FILE *FOut;
	char fname[100];
	int frame=0;
	
	if (argc!=2) return 1;

	new BApplication("application/x-vnd.Canopus-DVtest");
	BWindow *w=new BWindow(BRect(100,100,WIDTH+100-1,HEIGHT+100-1),argv[1],
		B_TITLED_WINDOW_LOOK,B_NORMAL_WINDOW_FEEL,B_NOT_RESIZABLE);
	BView *v=new BView(BRect(0,0,WIDTH-1,HEIGHT-1),"DVView",0,B_WILL_DRAW);
	BBitmap *bm=new BBitmap(BRect(0,0,WIDTH-1,HEIGHT-1),B_RGB32);
	w->AddChild(v);
	w->Show();
	InitDecParam();
	InitEncParam();

	int32 cName;
	int32 cLen;
	FIn=fopen(argv[1],"rb");
	fread(&cName,4,1,FIn);
	if (cName!='FFIR')
	{
		printf("Not a RIFF file.\n");
		fclose(FIn);
		return 0;
	}
	fread(&cLen,4,1,FIn);
	int32 tLen=cLen;
	printf("len=%d\n",cLen);
	fread(&cName,4,1,FIn);
	//fread(&cLen,4,1,FIn);
	if (cName!=' IVA')
	{
		printf("Not an AVI file.\n");
		fclose(FIn);
		return 0;
	}
	while (tLen>0)
	{
		fread(&cName,4,1,FIn);
		fread(&cLen,4,1,FIn);
		printf("CHUNK: %-4.4s, len=%d, %d left\n",&cName,cLen,tLen);
		tLen-=8;
		tLen-=cLen;
		
		// list could have 'movi' data...
		if (cName=='TSIL')
		{
			int32 scName;
			fread(&scName,4,1,FIn);
			printf("SUBCHUNK='%-4.4s'\n",&scName);
			cLen-=4;
			if (scName=='ivom')
				break;
		}
		fseek(FIn,cLen,SEEK_CUR);
	}
	bigtime_t tt=0;
	int32 frames=0;
	while (cLen>0)
	{
		int32 scLen;
		int32 scName;
		fread(&scName,4,1,FIn);
		fread(&scLen,4,1,FIn);
		cLen-=8;
	//	printf("subchunk, name='%-4.4s', len=%d\n",&scName,scLen);
		if (scLen==DVSIZE)
		{
			fread(inputdata,1,DVSIZE,FIn);
			//printf("read\n");
		
			PutImageBuffer=(BYTE*)bm->Bits();
			PutImageStride=720*4;
			bigtime_t t=system_time();
			if (HEIGHT==480)
				SoftEngineDecodeDV(DV_525_60_SYSTEM,inputdata,bm->Bits(),PutImage525_RGBQ);
			else
				SoftEngineDecodeDV(DV_625_50_SYSTEM,inputdata,bm->Bits(),PutImage625_RGBQ);
				
			t=system_time()-t;
			tt+=t;
			frames++;
			if (frames==10)
			{
				printf("%g frames/second\n",frames/(tt/1000000.0));
				tt=0;
				frames=0;
			}
			if (w->Lock())
			{
				v->DrawBitmap(bm,BPoint(0,0));
				w->Unlock();
			}
		}
		else
			fseek(FIn,scLen,SEEK_CUR);
		cLen-=scLen;
	//	printf("%d bytes left\n",cLen);
	}
	delete bm;
	if (w->Lock())
		w->Quit();
	fclose(FIn);
}
