#include <stdio.h>
#include <string.h>
#include <InterfaceKit.h>
#include <AppKit.h>
#include <TranslationUtils.h>

#define DVASM
extern "C" {
#include <windows.h>
#include "csedv.h"
};
#include "dvcodec.h"
#define WIDTH 720
#define HEIGHT 480 // 576 for PAL
#define DVSIZE 120000 // 144000 for PAL


int main(int argc,char *argv[])
{
	FILE *FIn;
	FILE *FOut;
	BRect r(0,0,WIDTH-1,HEIGHT-1);
	char outputdata[144000];
	
	SetupCodec();

	if (argc!=3) return 1;

	new BApplication("application/x-vnd.test");

	//InitEncParam(EncParam);
	
	BBitmap *bm=BTranslationUtils::GetBitmap(argv[1]);
	if (bm->InitCheck()!=B_OK)
	{
		printf("Error loading image.\n");
		return 0;
	}
	int32 height=bm->Bounds().IntegerHeight()+1;
	if (bm->Bounds().IntegerWidth()!=719 ||
		(height!=480 && height!=576))
	{
		printf("Bounds were not 720x480 (NTSC) or 720x576 (PAL).\n");
		delete bm;
		return 0;
	}
	int32 dvsize=(height==480)?120000:144000;

bigtime_t t=system_time();
#define REPS 30
for (int32 z=0;z<REPS;z++)
	if (dvsize==120000)
		EncodeDVNTSC(bm->Bits(),outputdata,B_RGB32);
	else
		EncodeDVPAL(bm->Bits(),outputdata,B_RGB32);
t=system_time()-t;
printf("fps=%g\n",REPS/(t/1000000.0));
	FOut=fopen(argv[2],"wb");
	fwrite(outputdata,1,dvsize,FOut);
	fclose(FOut);
	delete bm;
}
