#include <stdio.h>
#include <string.h>
#include <InterfaceKit.h>
#include <AppKit.h>

#include "dvcodec.h"


#define WIDTH 720
#define HEIGHT 480 // 576 for PAL
#define DVSIZE 120000 // 144000 for PAL

int main(int argc,char *argv[])
{
	char inputdata[DVSIZE];
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
			if (scName=='lrdh')
			{
				fread(&scName,4,1,FIn);
				int32 sscLen;
				fread(&sscLen,4,1,FIn);
				cLen-=8;
				struct {
				int32 tdelay,avidrate,res,flags,nframes,frames,streams,pbsize,wid,hei,tscale,drate,sttime,size;
				} avihdr;
				fread(&avihdr,sizeof(avihdr),1,FIn);
				cLen-=sizeof(avihdr);
				printf("frames=%d,streams=%d,pbsize=%d,wid=%d,hei=%d,tscale=%d,drate=%d,sttime=%d,size=%d\n",
					avihdr.nframes,avihdr.streams,avihdr.pbsize,avihdr.wid,
					avihdr.hei,avihdr.tscale,avihdr.drate,avihdr.sttime,avihdr.size);
			}
			else
			if (scName=='1rts')
			{
				fread(&scName,4,1,FIn);
				int32 sscLen;
				fread(&sscLen,4,1,FIn);
				cLen-=8;
				fread(&scName,4,1,FIn);
				cLen-=8;
				printf("scName='%-4.4s'\n",&scName);
			}
			else
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
		
			bigtime_t t=system_time();
			status_t result;

			if (HEIGHT==480)
				result=DecodeDVNTSC(inputdata,bm->Bits(),B_RGB32);
			else
				result=DecodeDVPAL(inputdata,bm->Bits(),B_RGB32);

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
