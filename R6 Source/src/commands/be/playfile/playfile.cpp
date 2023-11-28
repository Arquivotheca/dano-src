
#include <MediaFile.h>
#include <MediaTrack.h>
#include <SoundPlayer.h>
#include <File.h>
#include <Bitmap.h>
#include <Window.h>
#include <Screen.h>
#include <Application.h>
#include <OS.h>
#include <string.h>
#include <stdio.h>
#include <View.h>


static sem_id g_sem;
static uint32 frame_count;


static void
get_data(
	void * cookie,
	void * buffer,
	size_t size,
	const media_raw_audio_format & fmt)
{
	int64 fc = size / ((fmt.format & 0xf) * fmt.channel_count);
	frame_count += fc;
	bool done = false;
	if ((cookie != 0) && ((BMediaTrack *)cookie)->ReadFrames(buffer, &fc) < 0) {
		done = true;
	}
	int played = fc * (fmt.format & 0xf) * fmt.channel_count;
	if (played < size) {
		memset(((char *)buffer)+played, 0, size-played);
		done = (cookie != 0);
	}
	if (done) {
		delete_sem(g_sem);
	}
	else {
		release_sem_etc(g_sem, 1, B_DO_NOT_RESCHEDULE);
	}
}


static void
playtracks(
	const char * path,
	BMediaTrack * audio,
	BMediaTrack * video,
	const media_raw_audio_format & afmt,
	const media_raw_video_format & vfmt)
{
	frame_count = 0;
	const char * name = strrchr(path, '/');
	name = name ? (name+1) : path;
	BSoundPlayer bsp(&afmt, name, get_data, 0, audio);
	status_t err;
	if ((err = bsp.InitCheck()) < B_OK) {
		fprintf(stderr, "%s: %s\n", path, strerror(err));
		return;
	}

	g_sem = create_sem(0, "Play Completion");
	bool do_read = true;
	media_header mh;
	mh.start_time = 0;
	BBitmap * bitmap = 0;
	BWindow * window = 0;
	if (video != 0) {
		/* to handle interlace, we should double bytes_per_row and adjust when reading below... */
		const media_video_display_info & vdi(vfmt.display);
		BRect r(0,0, vdi.line_width-1, vdi.line_count-1);
		bitmap = new BBitmap(r, 0, vdi.format, vdi.bytes_per_row);
		/* shrink display to correct aspect ratio */
		if (vfmt.pixel_width_aspect > vfmt.pixel_height_aspect) {
			r.bottom = (r.bottom+1)*vfmt.pixel_height_aspect/vfmt.pixel_width_aspect-1;
		}
		else if (vfmt.pixel_height_aspect > vfmt.pixel_width_aspect) {
			r.right = (r.right+1)*vfmt.pixel_width_aspect/vfmt.pixel_height_aspect-1;
		}
		BRect f(BScreen().Frame());
		r.OffsetBy(floor((f.Width()-r.Width())/2), floor((f.Height()-r.Height())/3));
		window = new BWindow(r, name, B_TITLED_WINDOW, B_NOT_CLOSABLE);
		window->AddChild(new BView(window->Bounds(), "video", B_FOLLOW_ALL, B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE));
		window->ChildAt(0)->SetViewColor(B_TRANSPARENT_32_BIT);
		window->Show();
	}
	bigtime_t latency = bsp.Latency() + bigtime_t(afmt.buffer_size * 1000000LL /
		(afmt.frame_rate * (afmt.format & 0xf) * afmt.channel_count));
	bsp.Start();
	bsp.SetHasData(true);
	while (acquire_sem(g_sem) != B_BAD_SEM_ID) {
		if (video != 0) {
			off_t cnt = 1;
			if (do_read) {
				video->ReadFrames(bitmap->Bits(), &cnt, &mh);
				do_read = false;
			}
			uint32 fc = frame_count;
			bigtime_t play_time = bigtime_t(fc * 1000000LL / afmt.frame_rate)-latency;
			if ((play_time >= mh.start_time) && window->Lock()) {
				window->ChildAt(0)->DrawBitmap(bitmap, bitmap->Bounds(), window->ChildAt(0)->Bounds());
				window->Sync();
				window->Unlock();
				do_read = true;
			}
		}
	}
	fprintf(stderr, "%s: Sound Player Done\n", path);
	snooze(50000);	/*	cheezy way to wait for all sound to drain	*/
	if (window->Lock()) {
		window->Quit();
	}
	delete bitmap;
}


static void
playfile(
	const char * path)
{
	BFile file(path, O_RDONLY);
	BMediaFile mf(&file);
	status_t err;
	if ((err = mf.InitCheck()) < 0) {
		goto error;
	}
	else {
		BMediaTrack * audio = 0, *video = 0, *mt;
		media_format afmt, vfmt;
		int ix = 0;
		while ((mt = mf.TrackAt(ix)) != 0) {
			ix++;
			media_format mft;
			mft.type = B_MEDIA_RAW_AUDIO;
			if (!mt->DecodedFormat(&mft) && mft.type == B_MEDIA_RAW_AUDIO) {
				audio = mt;
				afmt = mft;
fprintf(stderr, "FOUND AUDIO\n");
			}
			else {
				mft.type = B_MEDIA_RAW_VIDEO;
				mft.u.raw_video = media_raw_video_format::wildcard;
				if (!mt->DecodedFormat(&mft) && mft.type == B_MEDIA_RAW_VIDEO) {
					video = mt;
					vfmt = mft;
fprintf(stderr, "FOUND VIDEO\n");
				}
				else {
					mf.ReleaseTrack(mt);
fprintf(stderr, "UNHANDLED TRACK\n");
				}
			}
		}
		if (!audio) {
			afmt.u.raw_audio.frame_rate = 44100.0;
			afmt.u.raw_audio.format = 0x2;
			afmt.u.raw_audio.channel_count = 1;
			afmt.u.raw_audio.buffer_size = 1024;	/* 512 frames */
		}
		playtracks(path, audio, video, afmt.u.raw_audio, vfmt.u.raw_video);
	}
	return;
error:
	fprintf(stderr, "%s: %s\n", path, strerror(err));
	return;
}


int
main(
	int argc,
	char * argv[])
{
	BApplication app("application/x-vnd.be.playfile");
	while (1) {
		argv++;
		if (!*argv) break;
		playfile(*argv);
	}
	app.PostMessage(B_QUIT_REQUESTED);
	app.Run();
	return 0;
}

