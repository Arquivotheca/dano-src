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
#include <SoundPlayer.h>

#include "MediaFile.h"
#include "MediaTrack.h"
#include <PushGameSound.h>

#ifndef _FILE_H
#include <File.h>
#include <Entry.h>
#endif

#define	BASE	0x16000000

//------------------------------------------------------------------------------------

#include	"bitmapplay.h"

//------------------------------------------------------------------------------------

void	BitmapPlay::SetDestination(ushort *base, long rowbyte, long x, long y)
{
	dest = base;
	dest_rb = rowbyte;
	dest_x = x;
	dest_y = y;
}

//------------------------------------------------------------------------------------

	BitmapPlay::BitmapPlay()
{
	is_p = 0;
	pause = 0;
	snd = 0;
}

//------------------------------------------------------------------------------------

void	BitmapPlay::BuildMediaFormat(int32 width, int32 height, color_space cspace,
				 media_format *format)
{
	media_raw_video_format *rvf = &format->u.raw_video;

	memset(format, 0, sizeof(*format));

	format->type = B_MEDIA_RAW_VIDEO;
	rvf->last_active = (uint32)(height - 1);
	rvf->orientation = B_VIDEO_TOP_LEFT_RIGHT;
	rvf->pixel_width_aspect = 1;
	rvf->pixel_height_aspect = 3;
	rvf->display.format = cspace;
	rvf->display.line_width = (int32)width;
	rvf->display.line_count = (int32)height;
	if (cspace == B_RGB32)
		rvf->display.bytes_per_row = 4 * width;
	else {
		printf("goodbye arve\n");
		exit(5);
	}
}

//------------------------------------------------------------------------------------

void	BitmapPlay::blit_me(ulong *bitmap, long width, long height)
{
	ushort	*dst;
	ulong	*src;
	int		x,y;
	ulong	v;
	uchar	r,g,b;
	long	pw;
	long	h0;

	h0 = width;

	if (width > mh)
		width = mh;

	if (height > mv)
		height = mv;

	for (y = 0; y < height; y++) {
		src = bitmap + (h0 * y);
		dst = dest + (dest_rb*(y + dest_y));
		dst += dest_x;
		for (x = 0; x < width; x++) {
			v = *src++;
			b = (v)&0xff;
			g = (v>>8)&0xff;
			r = (v>>16)&0xff;
		
			r >>= 3;
			g >>= 2;
			b >>= 3;

			v = (r<<11) | (g<<5) | b;
			*dst++ = v;
		}
	}
}

//------------------------------------------------------------------------------------

void
BitmapPlay::Decode(BMediaTrack *vidtrack, BMediaTrack *audtrack,
		  		   char *family_name, char *video_name, char *audio_name)
{
	char					*chunk;
	char					*bitmap = NULL;
	bool					found_family;
	int32					i, sz, cookie;
	int64					numFrames, j;
	int64					framesize;
	status_t				err;
	media_format			format, outfmt;
	media_codec_info    	mci;
	media_file_format		mfi;
	media_header 			mh;
	int 					width, height;



	// find the right file -format handler
	cookie = 0;
	while((err = get_next_file_format(&cookie, &mfi)) == B_OK) {
		if (strcmp(mfi.short_name, family_name) == 0)
			break;
	}

	if (err != B_OK) {
		printf("failed to find a file format handler !\n");
		return;
	}

	// Create the video track if any	
	if (vidtrack) {
		vidtrack->EncodedFormat(&format);
		
		width = format.u.encoded_video.output.display.line_width;
		height = format.u.encoded_video.output.display.line_count;
	
		memset(&format, 0, sizeof(format));
		BuildMediaFormat(width, height, B_RGB32, &format);
	
		vidtrack->DecodedFormat(&format);
		
		bitmap = (char *)malloc(width * height * 4);
		cookie = 0;
	}

	// Create the audio track if any
	if (audtrack) {
		audtrack->EncodedFormat(&format);
		audtrack->DecodedFormat(&format);
		framesize = (format.u.raw_audio.format&15)*format.u.raw_audio.channel_count;
		snd = new BPushGameSound(format.u.raw_audio.buffer_size/framesize, 
								 (gs_audio_format *)&format.u.raw_audio,
								 4);
		
		snd->StartPlaying();
	}

	// Process the video track, if any
	if (vidtrack) {

bigtime_t	t0;
int64 		framecount = 0;

		t0 = system_time();
		numFrames = vidtrack->CountFrames();
			
		for(j = 0; j < numFrames; j++) {
			int64 framecount = 1;
			err = vidtrack->ReadFrames(bitmap, &framecount, &mh);
			bigtime_t vStartTime = mh.start_time;
			bigtime_t snoozeTime = vStartTime - (system_time() - t0);

			if (pause && (do_exit == 0)) {
				if (snd)
					snd->StopPlaying();
				while (pause && (do_exit == 0)) {
					snooze(30000);
					t0 += 30000;
				}
				if (snd)
					snd->StartPlaying();
			}

			if (do_exit)
				goto out;
			if (snoozeTime > 0)
				snooze(snoozeTime);

			if (err) {
				break;
			}
			blit_me((ulong*)bitmap, width, height);
	
			if (snd) {
				void	*buf;
				size_t	size;

				while (snd->LockNextPage(&buf, &size) >= 0) {
					err = audtrack->ReadFrames((char *)buf, &framecount, &mh);
					snd->UnlockPage(buf);
				}
			}
		}
	}

out:;
	if (bitmap)
		free(bitmap);
	if (snd) {
		snd->StopPlaying();
		delete snd;
	}
}


//------------------------------------------------------------------------------------


void	BitmapPlay::dump_info(void)
{
	int32                  cookie = 0, cookie2;
	media_format 		   format, outfmt;
	media_file_format      mfi;
	media_codec_info     mci;


	while(get_next_file_format(&cookie, &mfi) == B_OK) {
		cookie2 = 0;

		memset(&format, 0, sizeof(format));
		format.type = B_MEDIA_RAW_VIDEO;

		format.type = B_MEDIA_RAW_VIDEO;
		format.u.raw_video.last_active = (uint32)(320 - 1);
		format.u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
		format.u.raw_video.pixel_width_aspect = 1;
		format.u.raw_video.pixel_height_aspect = 3;
		format.u.raw_video.display.format = B_RGB32;
		format.u.raw_video.display.line_width = (int32)320;
		format.u.raw_video.display.line_count = (int32)240;
		format.u.raw_video.display.bytes_per_row = 4 * 320;

		printf("    Video Encoders:\n");
		while (get_next_encoder(&cookie2, &mfi, &format, &outfmt, &mci) == B_OK) {
			printf("        %s / %s (%d)\n", mci.pretty_name, mci.short_name, mci.id);
		}

		cookie2 = 0;
		format.type = B_MEDIA_RAW_AUDIO;
		format.u.raw_audio.format = media_raw_audio_format::B_AUDIO_UCHAR;
		format.u.raw_audio.channel_count = 1;
		printf("    Audio Encoders:\n");
		while (get_next_encoder(&cookie2, &mfi, &format, &outfmt, &mci) == B_OK) {
			printf("        %s / %s (%d)\n", mci.pretty_name, mci.short_name, mci.id);
		}
	}
}


//------------------------------------------------------------------------------------

int	BitmapPlay::DoPlay0()
{
	status_t		err;
	entry_ref       ref;
	media_format	format;
	BMediaFile		*mediaFile;
	BMediaTrack		*track = NULL, *vidtrack = NULL, *audtrack = NULL;
	int32			i, numTracks;
	char			*output = NULL;
	char			*video_encoder_name = NULL, *audio_encoder_name = NULL;
	char            *family_name = NULL;

	do_exit = 0;
	strcpy(input, input);

	// get the ref from the path
	err = get_ref_for_path(input, &ref);
	
	if (err) {
		return -1;
	}

	// instanciate a BMediaFile object, and make sure there was no error.
	mediaFile = new BMediaFile(&ref);
	err = mediaFile->InitCheck();
	if (err) {
		return -1;
	}

	numTracks = mediaFile->CountTracks();
	
	for(i=0; i < numTracks; i++) {
		track = mediaFile->TrackAt(i);
		if (!track) {
			return -1;
		}

		// get the encoded format
		err = track->EncodedFormat(&format);
		if (err) {
			return -1;
		}

		if (format.type == B_MEDIA_RAW_VIDEO ||
			format.type == B_MEDIA_ENCODED_VIDEO) {
			vidtrack = track;
		} else if (format.type == B_MEDIA_RAW_AUDIO ||
				   format.type == B_MEDIA_ENCODED_AUDIO) {

			audtrack = track;
		} else {
			mediaFile->ReleaseTrack(track);
			track = NULL;
		}
	}

	if (family_name == NULL && vidtrack == NULL)
		family_name = "wav";
	else if (family_name == NULL)
		family_name = "quicktime";

	Decode(vidtrack, audtrack, family_name,
				  video_encoder_name, audio_encoder_name);

	delete mediaFile;
	return 0;
}


//------------------------------------------------------------------------------------


static	long	init_anim(void *p)
{
	BitmapPlay	*p_obj;

	p_obj = (BitmapPlay*)p;

	p_obj->DoPlay0();

	p_obj->is_p = 0;
	return 0;
}

//------------------------------------------------------------------------------------

void	BitmapPlay::SetHV(long h, long v)
{
	mh = h;
	mv = v;
}

//------------------------------------------------------------------------------------

char	BitmapPlay::IsPlaying()
{
	return is_p;
}

//------------------------------------------------------------------------------------

char	BitmapPlay::IsPaused()
{
	return pause;
}

//------------------------------------------------------------------------------------

int	BitmapPlay::DoPlay(char *vinput)
{
	is_p = 1;
	strcpy(input, vinput);
	resume_thread(spawn_thread(init_anim,"init_anim",B_DISPLAY_PRIORITY,this));

	return 0;
}

//------------------------------------------------------------------------------------

void	BitmapPlay::Pause(bool yesno)
{
	pause = yesno;
}

//------------------------------------------------------------------------------------

void	BitmapPlay::Abort()
{
	do_exit = 1;
	do {
		if (IsPlaying() == 0)
			return;
		snooze(30000);
	} while(1);
}


//------------------------------------------------------------------------------------
/*
void	main()
{
	BitmapPlay	*p;

	p = new BitmapPlay();
	p->SetDestination((ushort*)BASE, 1024, 100, 100);

	p->DoPlay("movie.mov");

	while(p->IsPlaying()) {
		snooze(30000);
	}

	delete p;
}
*/
