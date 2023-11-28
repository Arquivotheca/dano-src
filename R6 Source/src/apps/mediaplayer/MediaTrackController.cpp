#include <AutoLock.h>
#include <Bitmap.h>
#include <Debug.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <SoundPlayer.h>
#include <Screen.h>
#include <stdlib.h>
#include <malloc.h>
#include <signal.h>
#include "MediaPlayerApp.h"
#include "HTTPStream.h"
#include "VideoView.h"
#include "TransportView.h"
#include "PlayerWindow.h"
#include "PlayListHandler.h"
#include "debug.h"
#include "MediaTrackController.h"

// This should be true if all audio decoders return correct timing information.
// They currently do not.
#define AUDIO_DECODER_TIMING_WORKS 0

const bigtime_t kScrubPollingInterval = 10000;
const bigtime_t kBufferLowWater = 1000000;
const bigtime_t kBufferHighWater = 5000000;
const bigtime_t kBufferPollingInterval = 10000;
const bigtime_t kMinBuffer = 150000;
const uint32 kRenegotiateAudio = 'rnga';
const bigtime_t kCloseTimeout = 3000000;

static const char *PrettyName(const char *four_cc_name);
static const char *GetFourCC(unsigned char *fmt_user_data);
static bigtime_t GetUsage();

MediaTrackController::MediaTrackController()
	:	fMediaFile(0),
		fRawDataStream(0),
		fStartTime(0),
		fPlayListHandler(0),
		fFormatBitRate(0),
		fVideoTrack(0),
		fVideoView(0),
		fNumBitmaps(0),
		fCurrentDecodeBitmap(0),
		fDecodedFramePresentationTime(0),
		fLogicalMediaTime(0),
		fUsingOverlay(false),
		fIsContinuousStream(false),
		fAudioTrack(0),
		fSoundPlayer(0),
		fAudioLatency(0),
		fAudioZeroValue(0),
		fAudioDumpingBuffer(0),
		fLastAudioMediaTime(0),
		fLastAudioSystemTime(0),
		fLastPlayUpdate(0),
		fCurrentFrame(0),
		fLastVideoDecodeSample(0),
		fLastAudioDecodeSample(0),
		fLooping(false),
		fLastScrubPosition(0)
{
	for (int i = 0; i < 3; i++)
		fBitmaps[i] = 0;

	memset(&fStats, 0, sizeof(fStats));
}

MediaController* MediaTrackController::Open(const URL &url, const char *cookies,
	const BMessenger &target, status_t *outError)
{
	MediaTrackController *result = new MediaTrackController;

	// Send the URL to the new controller, so it can open is asynchronously
	BMessage msg(kOpenURL);
	msg.AddString("be:url", url.GetURLString());
	msg.AddString("be:cookies", cookies);
	result->PostMessage(&msg);

	result->SetTarget(target);
	result->Run();
	if (outError)
		*outError = result->InitCheck();

	return result;
}

void MediaTrackController::Close()
{
	ASSERT(Thread() != find_thread(NULL));

	writelog("Unset raw data stream\n");
#if 0
	if (fRawDataStream)
		fRawDataStream->Unset();	// Return an error to blocked reader threads

	// If this is opening a file, return an error from the socket
	// operation
	writelog("Signal thread in case its blocked on a socket operation\n");
	send_signal(Thread(), SIGUSR1);
#endif

	writelog("Send quit request to controller looper\n");
	BMessage request(B_QUIT_REQUESTED);
	BMessage reply;
	if (BMessenger(this).SendMessage(&request, &reply, kCloseTimeout, kCloseTimeout) != B_OK) {
#if 0
		// Killing the thread can do really bad things, especially if the thread
		// is holding libc locks.  
		kill_thread(Thread());	// Sorry, but this was taking too long!
		writelog("Killed MediaTrackController thread.\n");
#endif
		// Just abandon this looper.  This isn't totally safe, but we don't have much
		// else we can do.
		writelog("Controller not responding, abandon it\n");
		ConnectVideoOutput(0);
	}
}

void MediaTrackController::Cleanup()
{
	writelog("MediaTrackController::Cleanup()\n");
	SetState(STATE_CLEANUP);
	ConnectVideoOutput(0);
	
	if (fSoundPlayer) {
		// Note: can't hold sound player lock here, otherwise it will
		// potentially deadlock if the sound player node thread is waiting
		// on the lock (it must exit for the sound player to be deleted).
		delete fSoundPlayer;
		fSoundPlayer = 0;
	}

	fVideoTrack = 0;
	fAudioTrack = 0;

	for (int i = 0; i < 3; i++) {
		delete fBitmaps[i];
		fBitmaps[i] = 0;
	}

	// BMediaFile doesn't take ownership of its BDataIO, so we delete it ourselves
	
	delete fMediaFile;
	fMediaFile = 0;

	delete fPlayListHandler;
	fPlayListHandler = 0;
	
	delete fRawDataStream;
	fRawDataStream = 0;

	free(fAudioDumpingBuffer);
	fAudioDumpingBuffer = 0;
}

status_t MediaTrackController::SetTo(const URL &url, const char *cookies,
	BString *failingFormatDescription)
{
	fDescriptionString = "";
	fRawDataStream = StreamHandler::InstantiateStreamHandler(url.GetScheme());
	if (fRawDataStream == 0) {
		*failingFormatDescription << "I do not speak protocol \"" << url.GetScheme() << "\"\n";
		return B_ERROR;
	}

	fRawDataStream->SetCookies(cookies);
	status_t error = fRawDataStream->SetTo(url, *failingFormatDescription);
	if (error < 0) {
		delete fRawDataStream;
		fRawDataStream = 0;
		return error;
	}

	// Check to see if this is a playlist.
	fPlayListHandler = PlayListHandler::InstantiateHandler(fRawDataStream);
	if (fPlayListHandler) {
		writelog("This is a playlist\n");
	
		// Right now, just grab the first item in the list.
		char urlString[256];
		if (fPlayListHandler->GetNextFile(urlString, 255) != B_OK)
			return B_ERROR;

		writelog("First item in playlist is \"%s\"\n", urlString);		
		URL playListURL(urlString);
		if (!playListURL.IsValid()) {
			writelog("URL is not valid\n");
			return B_ERROR;
		}

		fRawDataStream = StreamHandler::InstantiateStreamHandler(playListURL.GetScheme());
		if (fRawDataStream == 0) {
			*failingFormatDescription << "I do not speak protocol \"" << playListURL.GetScheme() << "\"\n";
			return B_ERROR;
		}

		writelog("Opening stream\n");	
		status_t error = fRawDataStream->SetTo(playListURL, *failingFormatDescription);
		if (error < 0) {
			writelog("Could not set open URL\n");
			*failingFormatDescription = strerror(error);
			delete fRawDataStream;
			fRawDataStream = 0;
			return error;
		}
	}

	fIsContinuousStream = fRawDataStream->IsContinuous();

	// If the stream handler already does its own buffering, don't bother
	// having the media file do read ahead.
	fMediaFile = new BMediaFile(fRawDataStream, fRawDataStream->IsBuffered()
		? B_MEDIA_FILE_UNBUFFERED : B_MEDIA_FILE_BIG_BUFFERS);

	error = fMediaFile->InitCheck();
	if (error == B_MEDIA_NO_HANDLER) {
		*failingFormatDescription = 
			"There is not a file reader installed that understands this format.  "
			"The file might be an unsupported format, it could be corrupted, "
			"or it might not even be a media file\n";
		return B_MEDIA_NO_HANDLER;
	} else if (error != B_OK) {
		*failingFormatDescription = "The file reader found problems with this file.  Either "
			"the file contains bad data, or it is of a slightly different version than the "
			"reader understands\n";
		return B_MEDIA_BAD_FORMAT;
	}
	
	fFileName = url.GetFileName();
	return SetupTracks(failingFormatDescription);	
}

status_t MediaTrackController::SetupTracks(BString *failingFormatDescription)
{
	fFormatBitRate = 0;

	//	Get file format information
	media_file_format fileFormat;
	status_t error = fMediaFile->GetFileFormatInfo(&fileFormat);
	if (error != B_OK) {
		*failingFormatDescription = "The file reader found problems with this file.  Either "
			"the file contains bad data, or it is of a slightly different version than the "
			"codec understands\n";
		writelog("GetFileFormatInfo failed\n");
		return error;
	}

	writelog("Media file has %d tracks, set up decoders\n", fMediaFile->CountTracks());

	//	Step through the tracks and set each one up
	bool needCodec = false;
	int tracksFound = 0;
	for (int trackIndex = 0; trackIndex < fMediaFile->CountTracks(); trackIndex++) { 
		writelog("   Track %d: ", trackIndex);	
		BMediaTrack *track = fMediaFile->TrackAt(trackIndex);
		
		media_codec_info codecInfo;
		bool codecInfoValid = (track->GetCodecInfo(&codecInfo) == B_OK);
		bool trackUsed = false;

		media_format format;
		error = track->EncodedFormat(&format);
		if (error < 0)
			break;

		switch (format.type) {
			case B_MEDIA_ENCODED_VIDEO:
				writelog("Encoded video\n");			
				fDescriptionString << "Video: ";
				if(codecInfoValid) {
					fDescriptionString << codecInfo.pretty_name << "\n";
					error = SetVideoTrack(track, &format);
					if (error == B_OK) {
						trackUsed = true;
						fHasVideo = true;
					}
				}

				if (!trackUsed) {
					const char *prettyName = PrettyName(GetFourCC(format.user_data));
					if (prettyName) {
						*failingFormatDescription << "No codec installed that can handle the video encoding "
							"\"" << prettyName << "\" (" << GetFourCC(format.user_data) << ")\n";
						fDescriptionString << prettyName << " (could not handle)\n";
					} else {
						*failingFormatDescription << "There is not a codec installed that can handle the video encoding \""
							<< GetFourCC(format.user_data) << "\"\n";
						fDescriptionString << GetFourCC(format.user_data)
							<< " (could not handle)\n";
					}
					
					needCodec = true;
				}

				break;

			case B_MEDIA_RAW_VIDEO:
				writelog("Raw video");
				fDescriptionString << "Video: Raw video";
				error = SetVideoTrack(track, &format);
				if (error == B_OK) {
					trackUsed = true;
					fHasVideo = true;
				} else {
					*failingFormatDescription << "Unknown video format "
						<< GetFourCC(format.user_data);
					
					fDescriptionString << "Unknown: "
						<< GetFourCC(format.user_data)
						<< " (could not handle)\n";
					needCodec = true;
				}

				break;

			case B_MEDIA_RAW_AUDIO:
				writelog("Raw audio");
				fDescriptionString << "Sound: Raw audio\n";
				if (fAudioTrack == 0) {
					error = SetAudioTrack(fFileName.String(), track, &format);
					if (error == B_OK) {
						trackUsed = true;
						fHasAudio = true;
					}
				}

				if (!trackUsed && error == ENODEV)
					*failingFormatDescription = "Couldn't find audio hardware\n";
				else
					needCodec = true;

				break;
				
			case B_MEDIA_ENCODED_AUDIO:
				writelog("Encoded audio\n");
				if(codecInfoValid) {
					fDescriptionString << "Sound: " << codecInfo.pretty_name << "\n";
					if (fAudioTrack == 0) {
						error = track->DecodedFormat(&format);
						if (error == B_OK)
							error = SetAudioTrack(fFileName.String(), track, &format);
		
						if (error == B_OK) {
							trackUsed = true;
							fHasAudio = true;
							
							// This is mostly an .mp3 thing
							fFormatBitRate = (int32) format.u.encoded_audio.bit_rate;
						}
					}
				} else
					fDescriptionString << "Sound: Unknown codec\n";

				if (!trackUsed) {
					if (error == ENODEV) {
						*failingFormatDescription = "Couldn't find audio hardware\n";
					} else {
						needCodec = true;
						const char *prettyName = PrettyName(GetFourCC(format.user_data));
						if (prettyName) {
							*failingFormatDescription << "No codec installed that can handle the audio encoding "
								"\"" << prettyName << "\" (" << GetFourCC(format.user_data) << ")\n";
							fDescriptionString << prettyName << " (could not handle)\n";
						} else {
							*failingFormatDescription << "There is not a codec installed that can handle the audio encoding \""
								<< GetFourCC(format.user_data) << "\"\n";
							fDescriptionString << GetFourCC(format.user_data)
								<< " (could not handle)\n";
						}
					}
				}

				break;

			default:
				writelog("Unknown format\n");
		}
		
		if (trackUsed) {
			tracksFound++;
			writelog("\tInitialized\n");
		} else {
			fMediaFile->ReleaseTrack(track);
			writelog("\tCould not initialize\n");
		}
	}

	if (tracksFound == 0) {
		if (failingFormatDescription->Length() == 0)
			*failingFormatDescription = "Couldn't find any tracks in the file\n";
	
		if (needCodec)
			return B_MEDIA_NO_HANDLER;

		return B_MEDIA_BAD_FORMAT;
	}

	ASSERT(fVideoTrack || fAudioTrack);
	if (!fRawDataStream->IsContinuous()) {
		if (fVideoTrack)
			fLength = fVideoTrack->Duration();
		else
			fLength = fAudioTrack->Duration();

		if (fLength <= 0) {
			*failingFormatDescription = "The codec can't decode the data.  Either "
				"the file contains bad data, or it is of a slightly different version than the "
				"codec understands";
			return B_MEDIA_BAD_FORMAT;
		}

		uint32 lengthSeconds = (uint32)(fLength / 1000000);
		char lenstr[64];
		if (lengthSeconds > 60 * 60)
			sprintf(lenstr, "%01li:%02li:%02li", lengthSeconds / 60 / 60, (lengthSeconds / 60)
				% 60, lengthSeconds % 60); 
		else
			sprintf(lenstr, "%01li:%02li", (lengthSeconds / 60) % 60, lengthSeconds % 60);

		fDescriptionString << "Duration: " << lenstr << "\n";
	} else
		fLength = 0;

	ChangeInPoint(0, true);
	ChangeOutPoint(fLength, true);
	fReadyToPlay = true;
	fInitCheckVal = B_OK;

	if (fMediaFile->Copyright() && fMediaFile->Copyright()[0])
		fDescriptionString << "Copyright: " << fMediaFile->Copyright() << "\n";

	BString connectionDescription;
	fRawDataStream->DescribeConnection(connectionDescription);
	fDescriptionString << connectionDescription;

	fStats.has_video = (fVideoTrack != 0);
	fStats.num_buffers = fNumBitmaps;
	fStats.using_overlay = fUsingOverlay;

	return B_OK;
}

status_t MediaTrackController::SetVideoTrack(BMediaTrack *track, const media_format *format)
{
	status_t err;
	if (fVideoTrack)
		return B_MEDIA_ALREADY_CONNECTED;

	// Determine if the decoder can support multiple buffers.
	// Cinepak, for example, cannot, because it applies change incrementally
	// to the same buffer.
	bool doubleBuffer = false;
	media_format myPreferredFormat;
	myPreferredFormat.type = B_MEDIA_RAW_VIDEO;
	myPreferredFormat.u.raw_video = media_raw_video_format::wildcard;
	myPreferredFormat.require_flags = B_MEDIA_MULTIPLE_BUFFERS;
	myPreferredFormat.deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS | B_MEDIA_RETAINED_DATA;
	media_format decodersPreferredFormat = myPreferredFormat;
	err = track->DecodedFormat(&decodersPreferredFormat);
	if (err < B_OK)
		return err;
	
	doubleBuffer = myPreferredFormat.Matches(&decodersPreferredFormat);

	// Get ready to make some bitmaps
	BRect bitmapBounds(0, 0, 
		decodersPreferredFormat.u.raw_video.display.line_width - 1,
		decodersPreferredFormat.u.raw_video.display.line_count - 1);

	fVideoWidth = format->u.raw_video.display.line_width;
	fVideoHeight = format->u.raw_video.display.line_count;
	fVideoFramePeriod = (bigtime_t)(1000000 / format->u.raw_video.field_rate);
	color_space bitmapDepth = BScreen().ColorSpace();

	// loop, asking the track for a format we can deal with
	bool tryOverlay = true;
	bool tryYCBCR = true;
	
	MediaPlayerApp *app = dynamic_cast<MediaPlayerApp*>(be_app);
	if (app && !app->EnableHardwareOverlays()) {
		tryOverlay = false;
		tryYCBCR = false;
	}

	media_format lastrequestedFormat;
	media_format decodedFormat = decodersPreferredFormat;
	for (;;) {
		status_t error;
		fUsingOverlay = false;
		
		int bpr = decodedFormat.u.raw_video.display.bytes_per_row;
		if (bpr == media_raw_video_format::wildcard.display.bytes_per_row)
			bpr = B_ANY_BYTES_PER_ROW;

		if (tryOverlay) {
			// Double buffering an overlay requires 3 bitmaps
			if (doubleBuffer)
				fNumBitmaps = 3;
			else
				fNumBitmaps = 1;
				
			if (tryYCBCR)
				fBitmaps[0] = new BBitmap(bitmapBounds, B_BITMAP_WILL_OVERLAY
					| B_BITMAP_RESERVE_OVERLAY_CHANNEL, B_YCbCr422, bpr);
			else
				fBitmaps[0] = new BBitmap(bitmapBounds, B_BITMAP_WILL_OVERLAY
					| B_BITMAP_RESERVE_OVERLAY_CHANNEL, bitmapDepth, bpr);
			
			fUsingOverlay = true;
		} else {
			if (doubleBuffer)
				fNumBitmaps = 2;
			else
				fNumBitmaps = 1;
			
			fBitmaps[0] = new BBitmap(bitmapBounds, 0, bitmapDepth, bpr);
		}
	
		error = fBitmaps[0]->InitCheck();
		if (error == B_OK && fNumBitmaps > 1) {
			for (int i = 1; i < fNumBitmaps; i++) {
				fBitmaps[i] = new BBitmap(fBitmaps[0]->Bounds(),
					fUsingOverlay ? B_BITMAP_WILL_OVERLAY : 0, fBitmaps[0]->ColorSpace(),fBitmaps[0]->BytesPerRow());
				error = fBitmaps[i]->InitCheck();
				if (error != B_OK)
					break;
			}
		}

		if (error != B_OK) {
			for (int i = 0; i < fNumBitmaps; i++) {
				delete fBitmaps[i];
				fBitmaps[i] = 0;
			}

			if (tryYCBCR) {
				tryYCBCR = false;
				continue;
			}
			
			if (tryOverlay) {
				tryOverlay = false;
				continue;
			}

			writelog("couldn't create bitmap.\n");
			return error;
		}

		if(decodedFormat.type != B_MEDIA_RAW_VIDEO) {
			decodedFormat.type = B_MEDIA_RAW_VIDEO;
			decodedFormat.u.raw_video = media_raw_video_format::wildcard;
		}

		decodedFormat.deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS
			| B_MEDIA_CONTIGUOUS_BUFFER;
		decodedFormat.require_flags = 0;
		if (fUsingOverlay && fNumBitmaps == 1)
			decodedFormat.require_flags |= B_MEDIA_LINEAR_UPDATES;
		
		if (fNumBitmaps > 1) {
			decodedFormat.require_flags |= B_MEDIA_MULTIPLE_BUFFERS;
			decodedFormat.deny_flags |= B_MEDIA_RETAINED_DATA;
		}

		decodedFormat.u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
		decodedFormat.u.raw_video.display.format = fBitmaps[0]->ColorSpace();
		decodedFormat.u.raw_video.display.line_width = (int32)bitmapBounds.Width()+1;
		decodedFormat.u.raw_video.display.line_count = (int32)bitmapBounds.Height()+1;
		decodedFormat.u.raw_video.display.bytes_per_row = fBitmaps[0]->BytesPerRow();

		media_format requestedFormat;
		requestedFormat = decodedFormat;

		if (!tryOverlay && lastrequestedFormat == requestedFormat) {
			err = B_MEDIA_BAD_FORMAT;
			break;
		}
		lastrequestedFormat = requestedFormat;
		
		char formatstring[256];
		string_for_format(decodedFormat, formatstring, sizeof(formatstring)-1);
		writelog("\trequest decoder format %s\n", formatstring);

		err = track->DecodedFormat(&decodedFormat);
		
		string_for_format(decodedFormat, formatstring, sizeof(formatstring)-1);
		writelog("\tdecoder requested format %s\n", formatstring);
		writelog("\trequestedFormat 0x%04x 0x%04x decodedFormat 0x%04x 0x%04x\n",
			requestedFormat.require_flags, requestedFormat.deny_flags,
			decodedFormat.require_flags, decodedFormat.deny_flags);
		if (requestedFormat.Matches(&decodedFormat)) 
			break;

		writelog("\twanted cspace 0x%x, but it was reset to 0x%x\n",
			bitmapDepth, decodedFormat.u.raw_video.display.format);
		
		bitmapDepth = decodedFormat.u.raw_video.display.format;
		delete fBitmaps[0];
		fBitmaps[0] = 0;

		// Decoder doesn't speak this colorspace
		if (tryYCBCR) {
			bitmapDepth = BScreen().ColorSpace();
			tryYCBCR = false;
			continue;
		}
		if (tryOverlay) {
			bitmapDepth = BScreen().ColorSpace();
			tryOverlay = false;
			continue;
		}
	}
	
	writelog("\tUsing %d video buffers\n", fNumBitmaps);
	
	if(err != B_NO_ERROR || fBitmaps[0] == 0)
		return B_MEDIA_BAD_FORMAT;

	fVideoTrack = track;

	fDescriptionString << (int32) fVideoWidth << " x " << (int32) fVideoHeight << ",  "
		<< (int32) format->u.encoded_video.output.field_rate << " fps\n";

	return B_OK;
}

status_t MediaTrackController::SetAudioTrack(const char *name, BMediaTrack *track,
	const media_format *encoded_format)
{
	media_format format = *encoded_format;
	status_t err = track->DecodedFormat(&format);
	if(err != B_OK)
		return err;

	switch (format.u.raw_audio.format) {
	case media_raw_audio_format::B_AUDIO_UCHAR:
		fDescriptionString << "8 Bit Unsigned ";
		fAudioZeroValue = 0x80;
		fAudioFrameSize = 1;
		break;
	case media_raw_audio_format::B_AUDIO_CHAR:
		fDescriptionString << "8 Bit Signed ";
		fAudioZeroValue = 0;
		fAudioFrameSize = 1;
		break;
	case media_raw_audio_format::B_AUDIO_SHORT:
		fDescriptionString << "16 Bit ";
		fAudioZeroValue = 0;
		fAudioFrameSize = 2;
		break;
	case media_raw_audio_format::B_AUDIO_INT:
		fDescriptionString << "32 Bit (int) ";
		fAudioZeroValue = 0;
		fAudioFrameSize = 4;
		break;
	case media_raw_audio_format::B_AUDIO_FLOAT:
		fDescriptionString << "32 Bit (float) ";
		fAudioZeroValue = 0;
		fAudioFrameSize = 4;
		break;
	default:
		writelog("Bad raw audio format\n");
		return B_ERROR;
	}

	fAudioThreadInitialized = false;	
	fSoundPlayer = new BSoundPlayer(&format.u.raw_audio, name,
		MediaTrackController::SoundPlayerHook);

	err = fSoundPlayer->InitCheck();
	if (err != B_OK) {
		writelog("Couldn't create sound player\n");
		fDescriptionString << " (No audio hardware)\n";
		delete fSoundPlayer;
		fSoundPlayer = NULL;
		return ENODEV;
	} else {
		fAudioLatency = fSoundPlayer->Latency();
		fSoundPlayer->SetCookie(this);
		fSoundPlayer->SetHasData(true);
	}

	fAudioDumpingBuffer = malloc(format.u.raw_audio.buffer_size);

	fAudioFrameSize *= format.u.raw_audio.channel_count;
	switch(format.u.raw_audio.channel_count) {
		case 1:
			fDescriptionString << "Mono";
			break;
		case 2:
			fDescriptionString << "Stereo";
			break;
		default:
			fDescriptionString << format.u.raw_audio.channel_count
			                   << " Channels";
	}

	float samplerate = format.u.raw_audio.frame_rate / 1000.0;
	fDescriptionString << ", " << samplerate << " kHz\n";
	fAudioTrack = track;

	return B_OK;
}


status_t MediaTrackController::ConnectVideoOutput(VideoView *view)
{
	fVideoViewLock.Lock();
	fVideoView = view;
	fVideoViewLock.Unlock();
	return B_OK;
}

bigtime_t MediaTrackController::Position() const
{
	bigtime_t position = 0;
	if (CurrentState() == STATE_PLAYING) {
		if (fAudioTrack && AUDIO_DECODER_TIMING_WORKS) {
			fSoundPlayerLock.Lock();
			position = (system_time() - fLastAudioSystemTime) + fLastAudioMediaTime
				+ fAudioLatency;
			fSoundPlayerLock.Unlock();
		} else
			position = system_time() - fStartTime;
	} else
		position = fLogicalMediaTime;
	
	return position;
}

void MediaTrackController::SetInPoint(bigtime_t pos, bool)
{
	if (pos <= 0)
		pos = 0;
	else if (pos > fLength)
		pos = fLength;

	_SetInPoint(pos);
	
	// Drag main thumb along with left thumb
	if (Position() < pos && CurrentState() != STATE_SCRUBBING)
		SetPosition(pos);
}

void MediaTrackController::SetOutPoint(bigtime_t pos, bool)
{
	_SetOutPoint(pos);
}

void MediaTrackController::_SetInPoint(bigtime_t pos)
{
	if (pos <= 0)
		pos = 0;
	else if (pos > fLength)
		pos = fLength;

	ChangeInPoint(pos);	

	if (OutPoint() < pos)
		ChangeOutPoint(pos);
}

void MediaTrackController::_SetOutPoint(bigtime_t pos)
{
	if (pos <= 0)
		pos = 0;
	else if (pos > fLength)
		pos = fLength;

	ChangeOutPoint(pos);	
	if (InPoint() > pos)
		ChangeInPoint(pos);
}


void MediaTrackController::Play()
{
	PostMessage(kPlay);
}

void MediaTrackController::Stop()
{
	PostMessage(kStop);
}

void MediaTrackController::Pause()
{
	PostMessage(kPause);
}

void MediaTrackController::NudgeForward()
{
	PostMessage(kNudgeForward);
}

void MediaTrackController::NudgeBackward()
{
	PostMessage(kNudgeBackward);
}

void MediaTrackController::ScanForward()
{
	PostMessage(kScanForward);
}

void MediaTrackController::ScanBackward()
{
	PostMessage(kScanBackward);
}

void MediaTrackController::DoneSkipping()
{
	PostMessage(kDoneScanning);
}

void MediaTrackController::PauseAfterThumbNudge()
{
	Pause();
}

void MediaTrackController::BumpInPointAndRewind()
{
}

void MediaTrackController::BumpOutPointAndGoToEnd()
{
}

float MediaTrackController::Volume()
{
	// Volume and RawVolume are the same here, values 0 - 1
	float rawVolume;
	if (fMuted || fHalfVolume)
		rawVolume = fOriginalVolume;
	else
		rawVolume = RawVolume();

	float volume = rawVolume;
	
	if (volume > 1)
		volume = pow(volume, 1/3.0);
	else
		volume = sqrt(volume);
	
	volume = volume * 0.8;

	if (volume > 1)
		volume = 1;

	return volume;
}

void MediaTrackController::SetVolume(float volume)
{
	float rawVolume = volume;
	rawVolume = rawVolume/0.8;

	if (rawVolume > 1)
		rawVolume = rawVolume * rawVolume * rawVolume;
	else
		rawVolume = rawVolume * rawVolume;
	
	SetRawVolume(rawVolume);

	_inherited::SetVolume(volume);
}

float MediaTrackController::RawVolume() const
{
	BAutolock _lock(fSoundPlayerLock);
	if (fSoundPlayer == 0)	
		return 1.0;

	return fSoundPlayer->Volume();
}

void MediaTrackController::SetRawVolume(float volume)
{
	BAutolock _lock(fSoundPlayerLock);
	if (fSoundPlayer)
		fSoundPlayer->SetVolume(volume);
}

void MediaTrackController::SeekSoundTo(bigtime_t time)
{
	if (fAudioTrack == 0)
		return;

	BAutoLock _lock(fSoundPlayerLock);

	// Seek the extractor as close as possible
	bigtime_t audioStartTime = time;
	fAudioTrack->SeekToTime(&audioStartTime);

	// Read frames until we get less than 2ms ahead.
	bigtime_t lastTime = audioStartTime;
	while (audioStartTime < time) {
		media_header header;
		int64 numReadFrames = 1;
		if (fAudioTrack->ReadFrames((char *)fAudioDumpingBuffer,
			&numReadFrames, &header) != B_OK)
			break;
		audioStartTime = header.start_time;
		if (!numReadFrames || audioStartTime <= lastTime)
			break;
		lastTime = audioStartTime;
	}
}

status_t MediaTrackController::ReadyToLoop(loop_state* outState)
{
	if (!fLooping) {
		signal(SIGUSR1, InterruptSocketRead);
		fLooping = true;
		fLastScrubPosition = 0;
	}
	
	while (!ExecuteLoop(outState)) {
		outState->next_loop_time = B_INFINITE_TIMEOUT;
	}
	
	return B_OK;
}

bool MediaTrackController::ExecuteLoop(loop_state* outState)
{
	switch (CurrentState()) {
		case STATE_CLEANUP:
			break;		// Wait for the end...
		
		case STATE_STREAM_WAIT: {
			if (IsDataAvailable(Position(), kBufferHighWater)) {
				SetState(PopState());

				// Since the user may have seeked around while we were buffering,
				// re-seek to make sure we're in the right spot.
				SetState(STATE_PLAYING);
				if (fSoundPlayer)
					fSoundPlayer->Start();
				
				fDecodedFramePresentationTime = system_time() + fAudioLatency;
				fStartTime = fDecodedFramePresentationTime - fLogicalMediaTime;
				fLastAudioSystemTime = system_time();
				fLastAudioMediaTime = fLogicalMediaTime;
				SeekToTime(fLogicalMediaTime);
				outState->next_loop_time = system_time();
				break; 
			}
			
			outState->next_loop_time = system_time() + kBufferPollingInterval;
			break;
		}

		case STATE_SCRUBBING: {
			outState->next_loop_time = system_time() + kScrubPollingInterval;
			float scrubPosition = GetScrubTargetPosition(fScrubTarget, Length());
			if (scrubPosition == fLastScrubPosition)
				break;
				
			// drag the in/out points along with the main thumb
			if (InPoint() > Position())
				_SetInPoint(Position());
			else if (OutPoint() < Position())
				_SetOutPoint(Position());

			fLastScrubPosition = scrubPosition;
			SeekToTime((bigtime_t)((double) scrubPosition * (double) Length()));
			break;
		}
		
		case STATE_SCANNING_FORWARD:
		case STATE_SCANNING_BACKWARD:
			// Scan by skipping frames
			if (CurrentState() == STATE_SCANNING_FORWARD)
				SeekToTime(Position() + 75000);
			else if (CurrentState() == STATE_SCANNING_BACKWARD)
				SeekToTime(Position() - 150000);

			if (Position() >= OutPoint()) {
				outState->next_loop_time = 20000;
				break;
			}

			// falls through...

		case STATE_PLAYING: {
			fLastScrubPosition = -1;
			//
			//	Handle hitting the out point or end of file.
			//	Note that this doesn't happen if there is an audio track.
			// 	The audio play thread does it in this case.
			//
			if (!fIsContinuousStream && CurrentState() == STATE_PLAYING) {
				if (Position() >= OutPoint() || Position() >= fLength) {
					OutPointHit();
					// Note: set outState->next_loop_time here because OutPointHit
					// may post a Seek message to this looper .
					outState->next_loop_time = system_time() + 40000;
					break;
				} else
					outState->next_loop_time = OutPoint() - Position() + system_time();
			}

			//
			//	Handle running out of data while streaming
			//
			if (!IsDataAvailable(Position(), kBufferLowWater)) {
				writelog("No data available at %Ld, buffer more...\n", Position());
				SetBitmap(0);

				fLogicalMediaTime = Position();
				PushState(CurrentState());
				SetState(STATE_STREAM_WAIT);
				if (fSoundPlayer)
					fSoundPlayer->Stop();
				return false;
			}

			bigtime_t now = system_time();
			fStats.total_play_time += now - fLastPlayUpdate;
			fLastPlayUpdate = now;
			
			if (fVideoTrack) {
				if (DropFrames() && now > fDecodedFramePresentationTime + fVideoFramePeriod) {
						// Video is behind.  Attempt to skip data
						bigtime_t currentTrackTime = fVideoTrack->CurrentTime();
						
						bigtime_t closestKeyFrame = Position() + fVideoFramePeriod;
						status_t err = fVideoTrack->FindKeyFrameForTime(&closestKeyFrame,
							B_MEDIA_SEEK_CLOSEST_BACKWARD);
						if (err != B_OK)
							closestKeyFrame = 0LL;

						if (closestKeyFrame > currentTrackTime) {
							// Skip ahead
							bigtime_t newTime = closestKeyFrame;
							fVideoTrack->SeekToTime(&newTime, B_MEDIA_SEEK_CLOSEST_BACKWARD);
						}
				}

				if (now >= fDecodedFramePresentationTime) {
					fStats.frames_played++;
					
					// Draw the already decoded frame
					SetBitmap(fBitmaps[fCurrentDecodeBitmap], fUsingOverlay);

					fCurrentDecodeBitmap = (fCurrentDecodeBitmap + 1) % fNumBitmaps;

					// Decode a new frame
					media_header header;
					int64 numReadFrames = 1;
					Unlock();
					ASSERT(fBitmaps != 0);
					media_decode_info decodeParams;
					memset(&decodeParams, 0, sizeof(decodeParams));
					decodeParams.time_to_decode = fVideoFramePeriod;
					if (decodeParams.time_to_decode < 0)
						decodeParams.time_to_decode = 0;
						
					bigtime_t startUsage = GetUsage();
					fBitmaps[fCurrentDecodeBitmap]->LockBits();
					status_t result = fVideoTrack->ReadFrames((char*)
						fBitmaps[fCurrentDecodeBitmap]->Bits(), &numReadFrames,
						&header, &decodeParams);
					fBitmaps[fCurrentDecodeBitmap]->UnlockBits();
					fStats.video_decode_time =
						(fStats.video_decode_time / 2)
						+ ((double) (GetUsage() - startUsage) /
						(double) (now - fLastVideoDecodeSample)) / 2;
					fLastVideoDecodeSample = now;

					if (!Lock())
						return B_ERROR;

					// Compute statistics
					if ((int)header.u.raw_video.field_sequence - fCurrentFrame > 1) {
						fStats.frames_dropped += header.u.raw_video.field_sequence -
							fCurrentFrame - 1;
						fStats.frames_played += header.u.raw_video.field_sequence -
							fCurrentFrame - 1;
					}

					fCurrentFrame = header.u.raw_video.field_sequence;

					// Check for end of stream							
					if (result != B_OK || numReadFrames == 0) {
						if (result == B_LAST_BUFFER_ERROR) {
							OutPointHit();
						} else if (result == B_TIMED_OUT) {
							// I've seen decoders return B_TIMED_OUT if they can't decode
							// in the time passed in video_decode_time.  Ignore this
							// error.
						} else {
							// Something is legitimately fucked up.
							writelog("decoder returned error %s\n", strerror(result));
							Pause();
						}
						
						break;
					}

					// Determine the next frame time.
					if (fAudioTrack && AUDIO_DECODER_TIMING_WORKS) {
						// Sync to audio
						fDecodedFramePresentationTime = system_time() + header.start_time
							- Position();
					} else {
						// Sync to system time
						fDecodedFramePresentationTime = fStartTime + header.start_time;
					}
				}

				outState->next_loop_time = MIN(outState->next_loop_time,
												fDecodedFramePresentationTime);	
			} else
				outState->next_loop_time = MIN(outState->next_loop_time,
												now + 20000);

			break;
		}
		
		//	In the seek state, we are skipping over intermediate (non-key)
		//	frames to get to the exact time we want.
		case STATE_SEEKING: {
			if (!IsDataAvailable(Position(), kMinBuffer)) {
				SetBitmap(0);
				fLogicalMediaTime = Position();
				PushState(CurrentState());
				SetState(STATE_STREAM_WAIT);
				return false;
			}
				
			media_header header;
			int64 numReadFrames = 1;

			Unlock();

			fBitmaps[fCurrentDecodeBitmap]->LockBits();
			status_t result = fVideoTrack->ReadFrames(
				(char*) fBitmaps[fCurrentDecodeBitmap]->Bits(), &numReadFrames,
				&header);
			fBitmaps[fCurrentDecodeBitmap]->UnlockBits();
			fCurrentFrame = header.u.raw_video.field_sequence;
			if (!Lock())
				return B_ERROR;
				
			if (result != B_OK || numReadFrames == 0) {
				writelog("error reading video data\n");
				SetState(PopState());
				return false;
			}

			if (Position() >= fLogicalMediaTime - fVideoFramePeriod / 2) {
				SetBitmap(fBitmaps[fCurrentDecodeBitmap], fUsingOverlay);

				fCurrentDecodeBitmap = (fCurrentDecodeBitmap + 1) % fNumBitmaps;
				AbortPendingSeek();
			}


			// Note: don't check for messages when scanning because the
			// button will send spurious messages.
			return false;
		}

		default:
			break;
	}
	
	return true;
}

void MediaTrackController::StartPlaying()
{
	fLastPlayUpdate = system_time();
	if (CurrentState() == STATE_STREAM_WAIT) {
		// Can't start playing yet, but put it on the stack so it will be ready to.
		ControlState state = PopState();
		if (state == STATE_SEEKING)
			PopState();			
		
		PushState(STATE_PLAYING);
		return;
	}

	AbortPendingSeek();
	SetState(STATE_PLAYING);
	if (IsDataAvailable(Position(), kBufferLowWater) == false) {
		SetBitmap(0);

		if (fAudioTrack && CurrentState() == STATE_PLAYING)
			if (fSoundPlayer)
				fSoundPlayer->Stop();

		PushState(CurrentState());
		SetState(STATE_STREAM_WAIT);
		return;
	}

	ASSERT(CurrentState() != STATE_STREAM_WAIT);
	if (fSoundPlayer)
		fSoundPlayer->Start();

	fDecodedFramePresentationTime = system_time() + fAudioLatency;
	fStartTime = fDecodedFramePresentationTime - fLogicalMediaTime;
	fLastAudioSystemTime = system_time();
	fLastAudioMediaTime = fLogicalMediaTime;
}

void MediaTrackController::DispatchMessage(BMessage *message, BHandler *target)
{
	if ((target == this) && (message->what == B_QUIT_REQUESTED))
	{
		Cleanup();
		message->SendReply('bye ');
	}
	MediaController::DispatchMessage(message, target);
}

void MediaTrackController::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kPlay:
			StartPlaying();
			break;
		
		case kPause:
			if (CurrentState() == STATE_STREAM_WAIT) {
				PopState();
				PushState(STATE_PAUSED);
				break;
			}
			
			fLogicalMediaTime = Position();
			AbortPendingSeek();
			SetState(STATE_PAUSED);
			if (fSoundPlayer)
				fSoundPlayer->Stop();

			break;
		
		case kStop:
			if (CurrentState() == STATE_STREAM_WAIT) {
				PopState();
				break;
			}

			AbortPendingSeek();
			SetState(STATE_STOPPED);
			fLogicalMediaTime = Position();
			if (fSoundPlayer)
				fSoundPlayer->Stop();

			SeekToTime(InPoint());
			break;

		case kSetPosition: {
			AbortPendingSeek();

			bigtime_t pos;
			bigtime_t when;

			message->FindInt64("be:pos", (int64 *)&pos);
			message->FindInt64("be:when", (int64 *)&when);

			if (pos > OutPoint())
				pos = OutPoint();
			if (pos < InPoint())
				pos = InPoint();
		
			SeekToTime(pos);
			break;
		}

		case kSetScrubbing: {
			bool scrub = false;
			status_t result = message->FindBool("be:scrub", &scrub);
			ASSERT(result == B_OK);
			if (scrub) {
				//
				// Start scrubbing
				//
				AbortPendingSeek();
				if (CurrentState() == STATE_SCRUBBING)
					break;
					
				if (CurrentState() != STATE_STREAM_WAIT)
					PushState(CurrentState());
				// otherwise the state is already pushed

				SetState(STATE_SCRUBBING);
				if (fSoundPlayer)
					fSoundPlayer->Stop();

				// Record the messenger of the control driving us
				result = message->FindMessenger("be:scrubDriver", &fScrubTarget);
				ASSERT(result == B_OK);
			} else {
				//
				//	Stop scrubbing
				//
				AbortPendingSeek();
				if (CurrentState() == STATE_SCRUBBING) {
					// The transport control tends to be a little loose about
					// sending these messages spuriously, just ignore it if it
					// doesn't seem to apply.
					SetState(PopState());
					if (CurrentState() == STATE_PLAYING)
						StartPlaying();
				}
			}
		}
		break;
			
		case kNudgeForward:
		case kNudgeBackward: {
			if (CurrentState() == STATE_PAUSED || CurrentState() == STATE_STOPPED) {
				// Nudge
				AbortPendingSeek();
				bigtime_t timeDelta = 0;
				if (fVideoTrack)
					timeDelta = fVideoFramePeriod;
				else
					timeDelta = 60000;

				if (message->what == kNudgeBackward)
					SeekToTime(Position() - timeDelta);
				else
					SeekToTime(Position() + timeDelta);
			} else if (CurrentState() == STATE_STREAM_WAIT) {
				// Data is not present
				if (message->what == kNudgeBackward) {
					if (fLogicalMediaTime > 100000)
						fLogicalMediaTime -= 100000;
					else
						fLogicalMediaTime = 0;
				} else {
					if (fLogicalMediaTime < Length() - 100000)
						fLogicalMediaTime += 100000;
					else
						fLogicalMediaTime = Length();
				}
			}
				
			break;
		}


		case kScanBackward:
			switch (CurrentState()) {
				case STATE_STREAM_WAIT:
						PopState();
						// falls through
		
				case STATE_PLAYING:
				case STATE_STOPPED:
				case STATE_PAUSED:
					if (CurrentState() == STATE_STREAM_WAIT)
						PushState(STATE_STOPPED);
					else
						PushState(CurrentState());
					
					SetState(STATE_SCANNING_BACKWARD);
					break;
					
				default:
					break;
			}
			break;
			
		case kScanForward:
			switch (CurrentState()) {
				case STATE_STREAM_WAIT:
						PopState();
						// falls through
		
				case STATE_PLAYING:
				case STATE_STOPPED:
				case STATE_PAUSED:
					if (CurrentState() == STATE_STREAM_WAIT)
						PushState(STATE_STOPPED);
					else
						PushState(CurrentState());

					SetState(STATE_SCANNING_FORWARD);
					break;
					
				default:
					break;
			}
			break;

		case kDoneScanning:
			// Make sure we're actually scanning, as we get this
			// notification when nudging.
			if (CurrentState() == STATE_SEEKING)
				AbortPendingSeek();

			if (CurrentState() != STATE_SCANNING_FORWARD
				&& CurrentState() != STATE_SCANNING_BACKWARD)
				break;

			SetState(PopState());
			if (CurrentState() == STATE_STOPPED || CurrentState() == STATE_PAUSED)
				if (fSoundPlayer)
					fSoundPlayer->Stop();
				
			break;
			
		case kOpenURL: {
			const char *urlString;
			if (message->FindString("be:url", &urlString) == B_OK) {
				const char *cookies;
				if (message->FindString("be:cookies", &cookies) != B_OK)
					cookies = "";

				BString errorDescription;
				SetState(STATE_LOADING);
				URL url(urlString);
				fInitCheckVal = SetTo(url, cookies, &errorDescription);
				if (CurrentState() == STATE_CLEANUP) {
					PostMessage(B_QUIT_REQUESTED);
					break;
				}
					
				SetState(STATE_STOPPED);
				BMessage msg(kFileReady);
				msg.AddInt32("be:error_code", fInitCheckVal);
				msg.AddString("be:error_description", errorDescription.String());
				Messenger().SendMessage(&msg);
				break;
			}
			
			case kRenegotiateAudio: {
				// The audio format of the data has changed.  Since the sample rate
				// has probably changed as well, destroy the old sound player and
				// create a new one.
				writelog("Renegotiate audio format\n");
				delete fSoundPlayer;
				fSoundPlayer = 0;
			
				// Note: delete sound player before grabbing lock, otherwise
				// it will deadlock (node thread has to exit)
				BAutolock _lock(&fSoundPlayerLock);		
				media_format decodedFormat;
				SetAudioTrack(fFileName.String(), fAudioTrack, &decodedFormat);
				if (fSoundPlayer)
					fSoundPlayer->Start();
					
				break;
			}
		}			
	}
}

void MediaTrackController::SeekToTime(bigtime_t time)
{
	// Clip to in/out point
	if (time < InPoint())
		time = InPoint();
	else if (time > OutPoint())
		time = OutPoint();

	fLogicalMediaTime = time;
	if (!IsDataAvailable(time, kMinBuffer)) {
		// The user has attempted to seek to part of the file that isn't loaded.
		// When this part of the file becomes available, this will transition from
		// STATE_STREAM_WAIT to whatever state it was in before, in which case it
		// will re-seek.
		if (CurrentState() != STATE_STREAM_WAIT && CurrentState() != STATE_SCRUBBING) {
			PushState(CurrentState());
			SetState(STATE_STREAM_WAIT);
		}

		SetBitmap(0);
		return;
	}

	// Now seek some video
	bigtime_t position;
	if (fVideoTrack) {
		bigtime_t temp = fLogicalMediaTime;
		fVideoTrack->SeekToTime(&temp);
		position = fVideoTrack->CurrentTime();
	} else
		position = fLogicalMediaTime;


	if (fVideoTrack && position < fLogicalMediaTime - fVideoFramePeriod / 2) {
		// The video didn't seek to the time we wanted because it rounded
		// to a key frame when playing a movie. Silently play through the
		// video data until we get where we want to be.
		PushState(CurrentState());
		SetState(STATE_SEEKING);
	} else {
		// We seeked right where we wanted to
		if (fVideoTrack) {
			ASSERT(fBitmaps != 0);

			// Draw this frame
			int64 numReadFrames;
			media_header header;
			fBitmaps[fCurrentDecodeBitmap]->LockBits();
			fVideoTrack->ReadFrames((char*) fBitmaps[fCurrentDecodeBitmap]->Bits(),
				&numReadFrames, &header);
			fBitmaps[fCurrentDecodeBitmap]->UnlockBits();
			fCurrentFrame = header.u.raw_video.field_sequence;
			SetBitmap(fBitmaps[fCurrentDecodeBitmap], fUsingOverlay);
			fCurrentDecodeBitmap = (fCurrentDecodeBitmap + 1) % fNumBitmaps;
		}

		// Now seek sound.  This is pretty important, as timing is based on audio
		// when it is present
		SeekSoundTo(position);

		fStartTime = system_time() - position;
		fDecodedFramePresentationTime = fStartTime + position;
	}
}

void MediaTrackController::AbortPendingSeek()
{
	if (CurrentState() == STATE_SEEKING) {
		// Note: do this before popping time, because timing will change!
		fStartTime = system_time() - Position();
		fDecodedFramePresentationTime = fStartTime + Position();

		SetState(PopState());
		if (fAudioTrack && IsDataAvailable(fLogicalMediaTime, kMinBuffer)) {
			SeekSoundTo(fLogicalMediaTime);
			if (CurrentState() != STATE_SCRUBBING
				&& CurrentState() != STATE_PAUSED
				&& CurrentState() != STATE_STOPPED) {
				// We currently scrub with audio off
				if (fSoundPlayer)
					fSoundPlayer->Start();
			}
		}

		SetBitmap(fBitmaps[(fCurrentDecodeBitmap + fNumBitmaps - 1) % fNumBitmaps],
			fUsingOverlay);
	}
}

bool MediaTrackController::IsDataAvailable(bigtime_t atTime, bigtime_t timeNeeded)
{
	if (fRawDataStream == 0)
		return true;

	size_t size = 0;
	off_t startOffset = 0;
	if (fIsContinuousStream || Length() == 0) {
		if (fFormatBitRate == 0) 
			return true;
			
		size = timeNeeded * (fFormatBitRate / 8) / 1000000;
		startOffset = 0;
	} else {
		startOffset = (off_t) (fRawDataStream->GetLength() * atTime / Length());
		size = (size_t)(fRawDataStream->GetLength() * timeNeeded / Length());
		if (startOffset + size > fRawDataStream->GetLength())
			size = fRawDataStream->GetLength() - startOffset;
	}

	return fRawDataStream->IsDataAvailable(startOffset, size);
}

void MediaTrackController::AddAudioDecodeTime(bigtime_t time)
{
	bigtime_t now = system_time();
	fStats.audio_decode_time =
		(fStats.audio_decode_time / 2)
		+ ((double) time / (double) (now - fLastAudioDecodeSample)) / 2;

	fLastAudioDecodeSample = now;
}

void MediaTrackController::SoundPlayerHook(void *controller, void *buffer, size_t bufferSize,
	const media_raw_audio_format&)
{

	reinterpret_cast<MediaTrackController*>(controller)->DecodeAudio(buffer, bufferSize);
}

void MediaTrackController::DecodeAudio(void *buffer, size_t bufferSize)
{
	BAutolock _lock(&fSoundPlayerLock);		

	media_header header;
	int64 frameCount = 1;
	bigtime_t startDecode = GetUsage();			
	status_t err = fAudioTrack->ReadFrames((char*)buffer, &frameCount, &header);
	AddAudioDecodeTime(GetUsage() - startDecode);
	if (err != B_OK || frameCount < 0) {
		if (err == B_MEDIA_BAD_FORMAT)
			PostMessage(kRenegotiateAudio);
				// The audio format has changed.  Post a message to the looper
				// requesting it to delete us and create a new sound player.
	
		memset((char*) buffer, fAudioZeroValue, bufferSize);
	} else {
		fLastAudioMediaTime = header.start_time;
		fLastAudioSystemTime = system_time() + fAudioLatency;
		size_t sizeUsed = fAudioFrameSize * (int32) frameCount;
		if (sizeUsed < bufferSize) {
			// We've reached the end of the file, but there isn't enough
			// audio data to fill the entire buffer.  Zero out the rest
			// so there won't be noise.
			memset((char*) buffer + sizeUsed, fAudioZeroValue,
				bufferSize - sizeUsed);
		}
	}
}

float MediaTrackController::EnabledPortion() const
{
	if (fRawDataStream)
		return (float) fRawDataStream->GetDownloadedSize()
			/ (float) fRawDataStream->GetLength();

	return 1.0;	
}

void MediaTrackController::SetBitmap(BBitmap *bitmap, bool useOverlay)
{
	fVideoViewLock.Lock();
	if (fVideoView) {
		// Unroll my own lock to prevent deadlock.
		int32 unroll = 0;
		while (IsLocked()) {
			Unlock();
			unroll++;
		}

		// In this case, we're more concerned with how long it took
		bigtime_t start = system_time();		
		fVideoView->SetBitmap(bitmap, useOverlay);
		fStats.video_display_time += system_time() - start;

		while (unroll--)
			Lock();
	}
	fVideoViewLock.Unlock();
}

const char* MediaTrackController::GetConnectionName()
{
	if (fRawDataStream)
		return fRawDataStream->GetConnectionName();
	
	return "";
}

bool MediaTrackController::IsContinuous() const
{
	if (fRawDataStream)
		return fRawDataStream->IsContinuous();
		
	return false;
}

float MediaTrackController::BufferUtilization() const
{
	if (fRawDataStream)
		return fRawDataStream->BufferUtilization();

	return 1.0;
}

bool MediaTrackController::IsBuffering() const
{
	return CurrentState() == STATE_STREAM_WAIT;
}

void MediaTrackController::GetStats(player_stats *stats) const
{
	*stats = fStats;
	if (fRawDataStream)
		fRawDataStream->GetStats(stats);
}

void MediaTrackController::GetOutputSizeLimits(float *outMinWidth, float *outMaxWidth,
	float *outMinHeight, float *outMaxHeight)
{
	if (fUsingOverlay) {
		overlay_restrictions rest;
		fBitmaps[0]->GetOverlayRestrictions(&rest);
		*outMinWidth = MAX(fBitmaps[0]->Bounds().Width() * rest.min_width_scale,
			rest.destination.min_width);
		*outMinHeight = MAX(fBitmaps[0]->Bounds().Height() * rest.min_width_scale,
			rest.destination.min_height);
		*outMaxWidth = MIN(fBitmaps[0]->Bounds().Width() * rest.max_width_scale,
			rest.destination.max_width);
		*outMaxHeight = MIN(fBitmaps[0]->Bounds().Height() * rest.max_height_scale,
			rest.destination.max_height);
	} else {
		*outMinWidth = 0.0;
		*outMaxWidth = 1000000.0;
		*outMinHeight = 0.0;	
		*outMaxHeight = 1000000.0;	
	}
}

void MediaTrackController::InterruptSocketRead(int)
{
	// Yes, this is necessary
}

struct FourCCName  {
	const char *four_cc_name;
	const char *pretty_name;
} kFourCCNames[] = {
	{"MP42", "MPEG-4"},
	{"MP43", "MPEG-4"},
	{"MPG4", "MPEG-4"},
	{"ANIM", "Intel - RDX"},
	{"AUR2", "AuraVision - Aura 2 Codec - YUV 422"},
	{"AURA", "AuraVision - Aura 1 Codec - YUV 411"},
	{"BT20", "Brooktree - MediaStream codec"},
	{"BTCV", "Brooktree - Composite Video codec"},
	{"CC12", "Intel - YUV12 codec"},
	{"CDVC", "Canopus - DV codec"},
	{"CHAM", "Winnov, Inc. - MM_WINNOV_CAVIARA_CHAMPAGNE"},
	{"CPLA", "Weitek - 4:2:0 YUV Planar"},
	{"CVID", "Supermac - Cinepak"},
	{"CWLT", "reserved"},
	{"DUCK", "Duck Corp. - TrueMotion 1.0"},
	{"DVE2", "InSoft - DVE-2 Videoconferencing codec"},
	{"DXT1", "reserved"},
	{"DXT2", "reserved"},
	{"DXT3", "reserved"},
	{"DXT4", "reserved"},
	{"DXT5", "reserved"},
	{"DXTC", "DirectX Texture Compression"},
	{"FLJP", "D-Vision - Field Encoded Motion JPEG With LSI Bitstream Format"},
	{"GWLT", "reserved"},
	{"H260", "Intel - Conferencing codec"},
	{"H261", "Intel - Conferencing codec"},
	{"H262", "Intel - Conferencing codec"},
	{"H263", "Intel - Conferencing codec"},
	{"H264", "Intel - Conferencing codec"},
	{"H265", "Intel - Conferencing codec"},
	{"H266", "Intel - Conferencing codec"},
	{"H267", "Intel - Conferencing codec"},
	{"H268", "Intel - Conferencing codec"},
	{"H269", "Intel - Conferencing codec"},
	{"I263", "Intel - I263"},
	{"I420", "Intel - Indeo 4 codec"},
	{"IAN ", "Intel - RDX"},
	{"ICLB", "InSoft - CellB Videoconferencing codec"},
	{"ILVC", "Intel - Layered Video"},
	{"ILVR", "ITU-T - H.263+ compression standard"},
	{"IRAW", "Intel - YUV uncompressed"},
	{"IV30", "Intel - Indeo Video 3 codec"},
	{"IV31", "Intel - Indeo Video 3.1 codec"},
	{"IV32", "Intel - Indeo Video 3 codec"},
	{"IV33", "Intel - Indeo Video 3 codec"},
	{"IV34", "Intel - Indeo Video 3 codec"},
	{"IV35", "Intel - Indeo Video 3 codec"},
	{"IV36", "Intel - Indeo Video 3 codec"},
	{"IV37", "Intel - Indeo Video 3 codec"},
	{"IV38", "Intel - Indeo Video 3 codec"},
	{"IV39", "Intel - Indeo Video 3 codec"},
	{"IV40", "Intel - Indeo Video 4 codec"},
	{"IV41", "Intel - Indeo Video 4 codec"},
	{"IV42", "Intel - Indeo Video 4 codec"},
	{"IV43", "Intel - Indeo Video 4 codec"},
	{"IV44", "Intel - Indeo Video 4 codec"},
	{"IV45", "Intel - Indeo Video 4 codec"},
	{"IV46", "Intel - Indeo Video 4 codec"},
	{"IV47", "Intel - Indeo Video 4 codec"},
	{"IV48", "Intel - Indeo Video 4 codec"},
	{"IV49", "Intel - Indeo Video 4 codec"},
	{"IV50", "Intel - Indeo 5.0"},
	{"MP42", "Microsoft - MPEG-4 Video Codec V2"},
	{"MPEG", "Chromatic - MPEG 1 Video I Frame"},
	{"MRCA", "FAST Multimedia - Mrcodec"},
	{"MRLE", "Microsoft - Run Length Encoding"},
	{"MSVC", "Microsoft - Video 1"},
	{"NTN1", "Nogatech - Video Compression 1"},
	{"qpeq", "Q-Team - QPEG 1.1 Format video codec"},
	{"QDMC", "QDesign Music Codec"},
	{"QDM2", "QDesign Music Codec v2"},
	{"RGBT", "Computer Concepts - 32 bit support"},
	{"RT21", "Intel - Indeo 2.1 codec"},
	{"RVX ", "Intel - RDX"},
	{"SDCC", "Sun Communications - Digital Camera Codec"},
	{"SFMC", "Crystal Net - SFM Codec"},
	{"SMSC", "Radius - proprietary"},
	{"SMSD", "Radius - proprietary"},
	{"SPLC", "Splash Studios - ACM audio codec"},
	{"SQZ2", "Microsoft - VXtreme Video Codec V2"},
	{"SV10", "Sorenson - Video R1"},
	{"SVQ1", "Sorenson - Video"},
	{"rle ", "Sorenson - Video"},	// ???
	{"TLMS", "TeraLogic - Motion Intraframe Codec"},
	{"TLST", "TeraLogic - Motion Intraframe Codec"},
	{"TM20", "Duck Corp. - TrueMotion 2.0"},
	{"TMIC", "TeraLogic - Motion Intraframe Codec"},
	{"TMOT", "Horizons Technology - TrueMotion Video Compression Algorithm"},
	{"TR20", "Duck Corp. - TrueMotion RT 2.0"},
	{"V422", "Vitec Multimedia - 24 bit YUV 4:2:2 format (CCIR 601)."},
	{"V655", "Vitec Multimedia - 16 bit YUV 4:2:2 format."},
	{"VCR1", "ATI - VCR 1.0"},
	{"VIVO", "Vivo - H.263 Video Codec"},
	{"VIXL", "Miro Computer Products AG - for use with the Miro line of capture cards."},
	{"VLV1", "Videologic - VLCAP.DRV"},
	{"WBVC", "Winbond Electronics - W9960"},
	{"XLV0", "NetXL, Inc. - XL Video Decoder"},
	{"YC12", "Intel - YUV12 codec"},
	{"YUV8", "Winnov, Inc. - MM_WINNOV_CAVIAR_YUV8"},
	{"YUV9", "Intel - YUV9"},
	{"YUYV", "Canopus - YUYV compressor"},
	{"ZPEG", "Metheus - Video Zipper"},
	{"CYUV", "Creative Labs, Inc - Creative Labs YUV"},
	{"FVF1", "Iterated Systems, Inc. - Fractal Video Frame"},
	{"IF09", "Intel - Intel Intermediate YUV9"},
	{"JPEG", "Microsoft - Still Image JPEG DIB"},
	{"MJPG", "Microsoft - Motion JPEG DIB Format"},
	{"PHMO", "IBM - Photomotion"},
	{"ULTI", "IBM - Ultimotion"},
	{"VDCT", "Vitec Multimedia - Video Maker Pro DIB"},
	{"VIDS", "Vitec Multimedia - YUV 4:2:2 CCIR 601 for V422"},
	{"VSV1", "VisioWave Wavelet Compression"},
	{"YU92", "Intel - YUV"},
	{0, 0}
};

static const char* PrettyName(const char *four_cc_name)
{
	for (FourCCName *name = kFourCCNames; name->four_cc_name; name++)
		if (memcmp(name->four_cc_name, four_cc_name, 4) == 0)
			return name->pretty_name;

	return 0;
}

static const char *GetFourCC(unsigned char *fmt_user_data)
{
	return (const char*) fmt_user_data + strlen((char*) fmt_user_data) - 4;
}

static bigtime_t GetUsage()
{
	thread_info info;
	get_thread_info(find_thread(NULL), &info);
	return info.user_time + info.kernel_time;
}
