#include <stdio.h>
#include <string.h>
#include <InterfaceKit.h>
#include <AppKit.h>

extern "C" {
#include <windows.h>
#include "csedv.h"
void	InitDecParam( DEC_PARAM pd[5][6] );
};
#include "dvcodec.h"
#define WIDTH 720
#define HEIGHT 480 // 576 for PAL
#define DVSIZE 120000 // 144000 for PAL

BBitmap *bm1=NULL;
BBitmap *bm2=NULL;
char inputdata1[DVSIZE];
char inputdata2[DVSIZE];
sem_id rsem1;
sem_id rsem2;
sem_id wsem1;
sem_id wsem2;

int32 task1(void*)
{
	DEC_PARAM	DecParam[ 5 ][ 6 ];
	InitDecParam(DecParam);
	release_sem(wsem1);
	for (;;)
	{
		acquire_sem(rsem1);
		int32 result=DecodeDVNTSC(inputdata1,bm1->Bits(),B_RGB32,DecParam);
		release_sem(wsem1);
	}
}
int32 task2(void*)
{
	DEC_PARAM	DecParam[ 5 ][ 6 ];
	InitDecParam(DecParam);
	release_sem(wsem2);
	for (;;)
	{
		acquire_sem(rsem2);
		int32 result=DecodeDVNTSC(inputdata2,bm2->Bits(),B_RGB32,DecParam);
		release_sem(wsem2);
	}
}
int main(int argc,char *argv[])
{
	char *inputdata;
	FILE *FIn;
	FILE *FOut;
	BRect r(0,0,WIDTH-1,HEIGHT-1);
	
	if (argc!=2) return 1;

	new BApplication("application/x-vnd.Canopus-DVtest");
	BWindow *w=new BWindow(r,argv[1],
		B_TITLED_WINDOW_LOOK,B_NORMAL_WINDOW_FEEL,B_NOT_RESIZABLE);
	BView *v=new BView(r,"DVView",0,B_WILL_DRAW);
	BBitmap *bm=new BBitmap(r,B_RGB32);
	w->AddChild(v);
	w->MoveTo(100,100);
	w->Show();

	rsem1=create_sem(0,"read 1");
	rsem2=create_sem(0,"read 2");
	wsem1=create_sem(0,"write 1");
	wsem2=create_sem(0,"write 2");
	bm1=new BBitmap(r,B_RGB32);
	bm2=new BBitmap(r,B_RGB32);

	resume_thread(spawn_thread(task1,"Task1",B_NORMAL_PRIORITY,NULL));
	resume_thread(spawn_thread(task2,"Task2",B_NORMAL_PRIORITY,NULL));
	
	int32 cName;
	int32 cLen;
	FIn=fopen(argv[1],"rb");

	bigtime_t tt=0;
	int32 frames=0;
	int32 framenum=-2;
	while (!feof(FIn))
	{
		int32 scLen;
		int32 scName;
		if (fread(&scName,1,4,FIn)!=4) break;
		if (fread(&scLen,1,4,FIn)!=4) break;
		cLen-=8;
	//	printf("subchunk, name='%-4.4s', len=%d\n",&scName,scLen);
		if (scLen==DVSIZE)
		{
			bigtime_t t=system_time();
			status_t result;
			if (framenum&1)
			{
				acquire_sem(wsem1);
				if (fread(inputdata1,1,DVSIZE,FIn)!=DVSIZE) break;
			}
			else
			{
				acquire_sem(wsem2);
				if (fread(inputdata2,1,DVSIZE,FIn)!=DVSIZE) break;
			}
			if (framenum>=0)
			{
				if (w->Lock())
				{
					v->DrawBitmap((framenum&1)?bm1:bm2,BPoint(0,0));
					w->Unlock();
				}
			}
			if (framenum&1)
			{
				release_sem(rsem1);
			}
			else
			{
				release_sem(rsem2);
			}
			//printf("read\n");
		

//			if (HEIGHT==480)
//				result=DecodeDVNTSC(inputdata,bm->Bits(),B_RGB32);
//			else
//				result=DecodeDVPAL(inputdata,bm->Bits(),B_RGB32);

			t=system_time()-t;
			tt+=t;
			frames++;
			framenum++;
			if (frames==10)
			{
				printf("%g frames/second\n",frames/(tt/1000000.0));
				tt=0;
				frames=0;
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
