#ifndef	BITMAPPLAY
#define	BITMAPPLAY

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include <Application.h>
#include <SupportDefs.h>
#include <Bitmap.h>
#include <Rect.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <Screen.h>
#include <GraphicsDefs.h>

#include "MediaFile.h"
#include "MediaTrack.h"

//------------------------------------------------------------------------------------

class	BPushGameSound;

//------------------------------------------------------------------------------------

class	BitmapPlay {

public:
					BitmapPlay();
	void			SetDestination(ushort *base, long rowbyte, long x, long y);
	int				DoPlay(char *input);
	int				DoPlay0();
	char			IsPlaying();
	char			IsPaused();
	void			Pause(bool onoff);	
	void			Abort();
	void			SetHV(long h, long v);
private:
	
	void			BuildMediaFormat(int32 width, int32 height, color_space cspace,media_format *format);
	void			blit_me(ulong *bitmap, long width, long height);
	void			Decode(BMediaTrack *vidtrack, BMediaTrack *audtrack,
		  				   char *family_name, char *video_name, char *audio_name);
	void			dump_info(void);

	ushort			*dest;
	ulong			dest_rb;
	ulong			dest_x;
	ulong			dest_y;
	char			input[1024];
public:
	char			is_p;
	char			pause;
	char			do_exit;
	long			mh, mv;
	BPushGameSound	*snd;
};


#endif
