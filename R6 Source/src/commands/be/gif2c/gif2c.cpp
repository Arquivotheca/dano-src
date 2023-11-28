#include <stdio.h>
#include <Application.h>
#include <Window.h>
#include <View.h>

#include "gfxcodec.h"
#include "IDgif.h"

static void
writeHeader(GfxImage *img, const char *tagval)
{
	int width, height;
	int counter = 0;
	unsigned char *dataPtr = img->data;
	
	printf("#define %swidth %d\n", tagval, img->width);
	printf("#define %sheight %d\n", tagval, img->height);

	switch(img->type)
	{
		case B_COLOR_8_BIT:
			printf("#define %scspace B_COLOR_8_BIT\n",tagval);
			printf("#define %sbytesperpixel 1\n",tagval);
		break;
		
		case B_RGB_32_BIT:
			printf("#define %scspace B_RGB_32_BIT\n",tagval);
			printf("#define %sbytesperpixel 3\n",tagval);
		break;
		
		case B_BIG_RGB_32_BIT:
			printf("#define %scspace B_BIG_RGB_32_BIT\n",tagval);
			printf("#define %sbytesperpixel 3\n",tagval);
		break;
	}
	printf("\nunsigned char %sbits[] = {\n", tagval);
	
	fflush(stdout);
	
	for (height =0; height < img->height; height++)
	{
		for (width = 0; width < img->width; width++)
		{
			switch (img->type)
			{
				// When using SetBits with 8-bit images, the data
				// is simply the 8-bit values of the individual
				// pixels.  They should already be matched to the
				// system palette.
				case B_COLOR_8_BIT:
				{
					printf("0x%x,",dataPtr[width]);			
					counter++;
				}
				break;
			
				// When using SetBits with 32-bit images, the data
				// must actually represent a 24-bit format.  The order
				// of the bytes must be red, green, blue.  Thus you can
				// not simply take the Bits() of a BBitmap and call
				// SetBits using the same, you must convert it to 
				// a 24-bit format first.
				case B_RGB_32_BIT:
				{
					printf("0x%x,",dataPtr[width*4+2]);	// Red
					printf("0x%x,",dataPtr[width*4+1]);	// Green
					printf("0x%x,",dataPtr[width*4+0]);	// Blue			
					counter+=3;
				}
				break;

				case B_BIG_RGB_32_BIT:
					printf("0x%x,",dataPtr[width*4]);			
					printf("0x%x,",dataPtr[width*4+1]);			
					printf("0x%x,",dataPtr[width*4+2]);			
					counter+=3;
				break;
			}
			
			if ((counter%15)==0)
			{
				printf("\n");
				fflush(stdout);
			}
		}
		dataPtr+= img->bytes_per_row;
	}
	
	printf("};\n");
	
}

class myapp : public BApplication
{
public:
			myapp();
			
	virtual void	ArgvReceived(int32 argc, char **argv);
	virtual void	ReadyToRun();
	
protected:
private:
};

myapp::myapp()
	: BApplication("application/x-vnd.Be-gif2c")
{
	BWindow		*aWindow;
	BRect			aRect;

	// set up a rectangle and instantiate a new window
	//aRect.Set(100, 80, 260, 120);
	//aWindow = new BWindow(aRect,"mkimghdr",B_TITLED_WINDOW,0);
	
	// set up a rectangle and instantiate a new view
	// view rect should be same size as window rect but with left top at (0, 0)
	//aRect.OffsetTo(B_ORIGIN);
	//BView *aView = new BView(aRect, "HelloView",0,0);
	
	// add view to window	//aWindow->AddChild(aView);
	
	// make window visible
	//aWindow->Show();
}

void
myapp::ReadyToRun()
{
	PostMessage(B_QUIT_REQUESTED);
}


void
myapp::ArgvReceived(int32 argc, char **argv)
{
	GfxImage *img=0;
	
	if (argc < 2)
	{
		printf("USAGE: mkimghdr imagefile\n");
		return ;
	}
	
	img = CreateImage(argv[1]);
	matchPalette(img);
	
	if (!img)
	{
		printf("Image not created.\n");
		return ;
	}
	
	if (argc > 2)
		writeHeader(img,argv[2]);
	else
		writeHeader(img,"img_");
}

int 
main(int argc, char **argv)
{
	myapp anApp;
	anApp.Run();
}
