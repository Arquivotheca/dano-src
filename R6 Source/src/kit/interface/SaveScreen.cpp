#include <Debug.h>

#if _SUPPORTS_FEATURE_SCREEN_DUMP

#include <stdio.h>
#include <string.h>
#include <Bitmap.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <NodeInfo.h>
//+#include <Volume.h>
//+#include <VolumeRoster.h>
#include <Screen.h>
#include <Path.h>
#include <StopWatch.h>
#include <Beep.h>

#include "SaveScreen.h"


status_t	_save_bitmap_to_targa_(BBitmap *, FILE *);
int32		_save_screen_thread_main_(void *);


static int32 sSaveScreenCount = 0;


status_t
_save_bitmap_to_targa_(
	BBitmap	*bm, 
	FILE	*fp)
{
	int i, j;
	uint16 width, height;
	rgb_color acolor;
	
	BScreen screen( B_MAIN_SCREEN_ID );
	const color_map *map = screen.ColorMap();
	if( map == NULL ) {
		return B_ERROR;
	}

	color_space ptype = bm->ColorSpace();
	unsigned char *xpic = (unsigned char *)bm->Bits();
	width = bm->Bounds().IntegerWidth()+1;
	height = bm->Bounds().IntegerHeight()+1;
	  
	// write the header
	for (i=0; i<12; i++) 
		if (putc( (i==2) ? 2 : 0, fp)==EOF) goto cannot_save;
  
	if (
		(putc(width & 0xff, fp)==EOF)||
		(putc((width>>8) & 0xff, fp)==EOF)||
		(putc(height & 0xff, fp)==EOF)||
		(putc((height >> 8) & 0xff, fp)==EOF)||
		(putc(24,fp)==EOF)||
		(putc(0x20,fp)==EOF)
	) goto cannot_save;
	{
	uchar *lineBuffer = (uchar *)malloc(sizeof(uchar) * 3 * width);

	for (i=0; i<height; i++) {
		int32 offset = 0;

		for (j=0; j<width; j++) {
			if (ptype == B_COLOR_8_BIT) {			
				acolor = map->color_list[*xpic];
				lineBuffer[offset++] = acolor.blue;
				lineBuffer[offset++] = acolor.green;
				lineBuffer[offset++] = acolor.red;
				xpic++;
			} 
			else { 
				if (ptype == B_RGB_32_BIT) { 
					lineBuffer[offset++] = xpic[0];
					lineBuffer[offset++] = xpic[1];
					lineBuffer[offset++] = xpic[2];
					xpic += 4;
				}
			}
		}

		if (fwrite(lineBuffer, sizeof(uchar), 3 * width, fp) != (size_t)(3 * width))
			goto unalloc_cannot_save;
	}
	
	free(lineBuffer);

	return (B_NO_ERROR);
unalloc_cannot_save:
	free(lineBuffer);}
cannot_save:
	beep();
	return B_ERROR;

}


int32
_save_screen_thread_main_(
	void	*)
{
	BStopWatch	timer("", true);
	BScreen screen;
	BRect screen_frame = screen.Frame();

	// WAA - monkey business with frames prevents us from using
	// what get_screen_info returns directly, so we construct
	// a temporary rectangle
	BRect tempRect = screen_frame;

	BBitmap *screen_bits = new BBitmap(screen_frame, B_BITMAP_IS_AREA, B_RGB32);	
	_get_screen_bitmap_(screen_bits, tempRect, true);

	// create file with unique name
	BPath path;
	if (find_directory(B_USER_DIRECTORY, &path) == B_NO_ERROR) {
		int32		i = 1;
		BDirectory	root(path.Path());
		char		name[B_FILE_NAME_LENGTH + 1];
		strcpy(name, "screen1.tga");
		while (root.Contains(name))
			sprintf(name, "screen%ld.tga", ++i);
		path.Append(name);

		// Now actually create the file	
		FILE *fp = fopen(path.Path(), "w+");
		if (fp != NULL) {
			// Write out as targa as default method
			if (_save_bitmap_to_targa_(screen_bits, fp)==B_OK) {
				fclose(fp);
				BNode		node(path.Path());
				BNodeInfo	ninfo(&node);
				ninfo.SetType("image/x-targa");
			} else {
				fclose(fp);
				remove(path.Path());
			}
		}
	}

	delete screen_bits;		

	/*
	 Don't allow more than 1 screen shot every half second.
	 To prevent a million from getting created if the user holds down
	 the print screen button for too long.
	*/
	timer.Suspend();
	bigtime_t	et = timer.ElapsedTime();
	if (et < 500000)
		snooze(500000 - et);

	atomic_add(&sSaveScreenCount, -1);

	return (B_NO_ERROR);
}


void
_save_screen_to_file_()
{
	if (atomic_add(&sSaveScreenCount, 1) > 0) {
		atomic_add(&sSaveScreenCount, -1);
		return;
	}

	thread_id tid = spawn_thread(_save_screen_thread_main_, "Print Screen", 
								 B_NORMAL_PRIORITY, NULL);
	resume_thread(tid);
}

#endif
