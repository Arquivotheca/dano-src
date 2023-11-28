/*
	
	vidview.cpp
	
	Copyright 1997 Be Incorporated, All Rights Reserved.
	
*/

#include <OS.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <Bitmap.h>
#include "vidopt.h"
#include "vidsource.h"

extern "C" {
	int _kopen_(char*, int);
	int _kclose_(int);
	int _kread_(int,void*,int);
	int _kioctl_(int, int, void*);
}

void playvideo(int, int, void *, void *);


static volatile int 	quit;		
static thread_id       worker_id,drawer_id;

static sem_id          sem_ready, sem_used;
static int 			fw;
static double			numworker,numdrawer,numtimeout;
static double			starttime,endtime;
static double			memstart,memend;
static double			drawstart,drawend;

#define bmsgDraw	'vidf'
static long 			__worker(void *arg);
static long 			__drawer(void *arg);

#define B_CONTIGUOUS 3

BTSVideoSource::BTSVideoSource(char *name)
{
	fRunning = 0;
	Init();
}

BTSVideoSource::~BTSVideoSource()
{
	Stop();
	
	/* clean up */
	delete bitmap1;
	delete bitmap2;

	delete_sem(sem_ready);
	delete_sem(sem_used);

	delete_area(isoc_area1);
	delete_area(isoc_area2);

	/* close video driver */
	close(fw);
	
	printf("BTSVideoSource::~BTSVideoSource() - END\n");
}

void	
BTSVideoSource::Init()
{
	int dummy;
	unsigned long i,step;
	void	*even[512];
	void	*odd[512];
	
	quit  = 0;
	numworker=numdrawer=numtimeout=0;


	if ((fw = _kopen_("/dev/Bt848",0)) == B_ERROR) {
		printf("Can't open Bt848 driver\n");
		//exit(0);
	}
	printf("opened Bt848: %x\n",fw);

	isoc_area1 = create_area("isochronous buffer #1", &isocbuffer1, B_ANY_ADDRESS, BUFFER_SIZE8K, B_CONTIGUOUS, B_READ_AREA + B_WRITE_AREA);
	if (isoc_area1 == B_ERROR | isoc_area1 == B_BAD_VALUE | isoc_area1 == B_NO_MEMORY) 
	{
		printf("Can't allocate isochronous buffer #1 space\n");
		quit = 1;
	}
	
	isoc_area2 = create_area("isochronous buffer #2", &isocbuffer2, B_ANY_ADDRESS, BUFFER_SIZE8K, B_CONTIGUOUS, B_READ_AREA + B_WRITE_AREA);
	if (isoc_area2 == B_ERROR |
		isoc_area2 == B_BAD_VALUE |
		isoc_area2 == B_NO_MEMORY) {
		printf("Can't allocate isochronous buffer #2 space\n");
		quit = 1;;
	}

	ib					= isocbuffer1;
	isocbuffer			= isocbuffer1;	
/* if FIELDS is defined, we're going to try to capture all 60 fields per second */
/* we'll capture odd fields in isocbuffer1, even in isocbuffer2 */
/* we'll alternately display both buffers if FIELDS is set, otherwise we'll just display buffer1 */ 	
#ifdef FIELDS
	nextisocbuffer		= isocbuffer2;
#else
	nextisocbuffer		= isocbuffer1;
#endif

	bitmap1 = new BBitmap(BRect(0,0,HORIZONTAL_SIZE-1,VERTICAL_SIZE-1),B_RGB_32_BIT,TRUE);
	bitmap2 = new BBitmap(BRect(0,0,HORIZONTAL_SIZE-1,VERTICAL_SIZE-1),B_RGB_32_BIT,TRUE);

	sem_ready = create_sem(0, "buffer ready sem");
	sem_used = create_sem(2, "buffer used sem");

	bm			= bitmap1;
	bitmap		= bitmap1;
	nextbitmap	= bitmap2;

	
	fConfig.source = COMPOSITE;
	fConfig.format = NTSC_M;
	fConfig.x_size = HORIZONTAL_SIZE;
	fConfig.y_size = VERTICAL_SIZE;
	fConfig.color_format = RGB32;
	fConfig.bright = 0;
	fConfig.contrast = 0;
	fConfig.hue = 0;
	fConfig.saturation = 0;
	fConfig.capture_mode = CONTINUOUS;
	//fConfig.capture_mode = CONTINUOUS;

/* if FIELDS is defined try to capture all 60 fields */
/* otherwise, just capture the odd fields            */
#ifdef FIELDS
	fConfig.e_line = even;
#else
	fConfig.e_line = 0;
#endif

	fConfig.e_address_type = LOGICAL;
	fConfig.e_clip = 0;
	fConfig.o_line = odd;
	fConfig.o_address_type = LOGICAL;
	fConfig.o_clip = 0;
	fConfig.decimate = 0;
	fConfig.status = 0;
		
	if (fConfig.y_size <= 240)  	/* overlay single fields if CIF or lower resolution */
		step = 1;
	else
		step = 2;				/* otherwise interlace */
		
	for (i=0; i < fConfig.y_size/step; i++)
	{
		odd[i] = (void *)((unsigned long)isocbuffer1 + ((i*step)*fConfig.x_size*BYTES_PER_PIXEL));
	}

	for (i=0; i < fConfig.y_size/step; i++)
	{
		even[i] = (void *)((unsigned long)isocbuffer2 + (((i*step)+(step-1))*fConfig.x_size*BYTES_PER_PIXEL));
	}

}

void	
BTSVideoSource::Start()
{
	_kioctl_(fw,BT848_INIT,&fConfig);
	_kioctl_(fw,BT848_START,&fConfig);
	fRunning = 1;

	//printf("BTSVideoSource::Start() - END\n");
}

void	
BTSVideoSource::Stop()
{
	int dummy;
	
	if (fRunning)
	{
		fRunning = 0;
		/* turn off video stream */
		if (_kioctl_(fw,BT848_STOP,&dummy) == B_ERROR)
			printf("BTSVideoSource::Stop() - ioctl failed\n");
	}
	
	//printf("BTSVideoSource::Stop() - END\n");
}

void	
BTSVideoSource::Restart()
{
	Start();
}
	
void	
BTSVideoSource::SetBrightness(int bright)
{
	Stop();
	
	// Get current configuration
	// Set new brightness configuration
	fConfig.bright = bright;
	
	Start();
}

void	
BTSVideoSource::SetContrast(int contrast)
{
	Stop();
	
	// Get current configuration
	// Set new brightness configuration
	fConfig.contrast = contrast;
	
	Start();
}

void	
BTSVideoSource::SetHue(int hue)
{
	Stop();
	
	// Get current configuration
	// Set new brightness configuration
	fConfig.hue = hue;
	
	Start();
}

void	
BTSVideoSource::SetSaturation(int saturation)
{
	Stop();
	
	// Get current configuration
	// Set new brightness configuration
	fConfig.saturation = saturation;
	
	Start();
}

BBitmap *
BTSVideoSource::GetNextFrame()
{
	int dummy;
	
	// blocking read to synchronize with end of frame 
	_kread_(fw,&dummy,1);	
		
	// otherwise just capture into same bitmap as last time			
	memcpy((char *)(bm->Bits()),ib,HORIZONTAL_SIZE*VERTICAL_SIZE*BYTES_PER_PIXEL);
			
	return bm;
}

