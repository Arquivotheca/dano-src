
// ToDo:
// use HookUpMixer for sound output


#include <AutoLock.h>
#include <Bitmap.h>
#include <Debug.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <SoundPlayer.h>
#include <Screen.h>

#include <malloc.h>

#include "MediaTrackController.h"
#include "VideoView.h"

// some internal message what fields
const uint32 kRescheduleWakeup = 'rsch';
const uint32 kPlay = 'play';
const uint32 kPause = 'paus';
const uint32 kStop = 'stop';
const uint32 kSetAutoLoop = 'auto';
const uint32 kSetPosition = 'spos';
const uint32 kSetOutPoint = 'soup';
const uint32 kSetInPoint = 'sinp';
const uint32 kScrubTo = 'scrb';
const uint32 kSetScrubbing = 'sscr';
const uint32 kBumpAndRewind = 'bprw';
const uint32 kBumpAndGoToEnd = 'bpge';
const uint32 kNudgeForward = 'ngfw';
const uint32 kNudgeBackward = 'ngbk';
const uint32 kScanForward = 'scfw';
const uint32 kScanBackward = 'scbk';
const uint32 kDoneScanning = 'dnsc';

#ifdef __USE_MEDIA_TRACK__


MediaTrackController::MediaTrackController(const entry_ref *ref, bool debug)
	:	MediaController(ref, debug),
		fMediaFile(0),
		fVideoTrack(0),
		fAudioTrack(0),
		fAudioOutput(0),
		fBitmap(0),
		fVideoView(0),
		fPlaying(false),
		fSnoozing(false),
		fAudioDumpingBuffer(0),
		fSeekToTimeNeeded(false),
		fSeekToFrameNeeded(false),
		fLooping(false)
{
}


MediaTrackController::~MediaTrackController()
{
	Reset();
}

bool 
MediaTrackController::CanHandle(const entry_ref *)
{
	// for now
	return true;
}

MediaController *
MediaTrackController::Open(const entry_ref *ref, BLooper *owner,
	status_t *error, bool debug)
{
	if (!CanHandle(ref)) {
		if (error)
			*error = B_MEDIA_NO_HANDLER;
		return 0;
	}

	MediaTrackController *result;
	result = new MediaTrackController(ref, debug);
	status_t tmpError = result->SetTo(ref);
	if (tmpError != B_OK) {
		delete result;
		if (error)
			*error = tmpError;
		return 0;
	}
	
	result->SetTarget(owner, owner);
	result->Run();

	if (error)
		*error = result->InitCheck();

	return result;
}

void 
MediaTrackController::Delete(BLooper *)
{
	Lock();
	Quit();
}

void
MediaTrackController::Reset()
{
	fVideoTrack = 0;
	fAudioTrack = 0;

	delete fAudioOutput;
	fAudioOutput = 0;

	delete fBitmap;
	fBitmap = 0;

	delete fMediaFile;
	fMediaFile = 0;

	free(fAudioDumpingBuffer);
	fAudioDumpingBuffer = 0;
}

status_t 
MediaTrackController::SetTo(const entry_ref *ref)
{
	try {
		fDescriptionString = "";
		
		fMediaFile = new BMediaFile(ref);
	
		fDescriptionString << "File: " << ref->name << "\n";
		
		media_file_format fileFormat;
		status_t error = fMediaFile->GetFileFormatInfo(&fileFormat);
		if (error != B_OK)
			throw error;

		fDescriptionString << "Format: " << fileFormat.pretty_name << "\n";

		bool foundTrack = false;
		int32 numTracks = fMediaFile->CountTracks();
	
		if (!numTracks)
			throw (status_t)B_MEDIA_BAD_FORMAT;

		status_t trackReadError = B_MEDIA_BAD_FORMAT;
		for (int32 index = 0; index < numTracks; index++) {
			BMediaTrack *track = fMediaFile->TrackAt(index);
			
			if (!track) 
				throw (status_t)B_MEDIA_BAD_FORMAT;

			media_codec_info codecInfo;
			track->GetCodecInfo(&codecInfo);

			bool trackUsed = false;
			media_format format;
			if (track->EncodedFormat(&format) == B_OK)
				switch (format.type) {
					case B_MEDIA_ENCODED_VIDEO:
						PRINT(("#################field rate %f\n",
							format.u.encoded_video.output.field_rate));

						fDescriptionString << "Video: " << codecInfo.pretty_name << "\n";
						trackReadError = SetVideoTrack(track, &format);
						if (trackReadError == B_OK) {
							trackUsed = true;
							fHasVideo = true;
						}
						break;
	
					case B_MEDIA_RAW_AUDIO:

						fDescriptionString << "Sound: Raw audio\n";
						trackReadError = SetAudioTrack(ref->name, track, &format);
						if (trackReadError == B_OK) {
							trackUsed = true;
							fHasAudio = true;
						}
						break;
						
					case B_MEDIA_ENCODED_AUDIO:

						fDescriptionString << "Sound: " << codecInfo.pretty_name << "\n";
						trackReadError = track->DecodedFormat(&format);
						if (trackReadError == B_OK)
							trackReadError = SetAudioTrack(ref->name, track, &format);
						if (trackReadError == B_OK) {
							trackUsed = true;
							fHasAudio = true;
						}
						break;
	
					default:
						break;
				}
	
			if (trackUsed)
				foundTrack = true;
			else 
				fMediaFile->ReleaseTrack(track);
		}

		fLength = 0;
		if (!foundTrack)
			throw trackReadError;
			
		ASSERT(fVideoTrack || fAudioTrack);

		if (fVideoTrack) {
			fLength = fVideoTrack->Duration();
			bigtime_t start = 0;
			ThrowOnError( fVideoTrack->SeekToTime(&start) );
		} else
			fLength = fAudioTrack->Duration();
		
		fCurTime = 0;
		fInPoint = 0;
		fOutPoint = fLength;

		if (fMediaFile->Copyright() && fMediaFile->Copyright()[0])
			fDescriptionString << "Copyright: " << fMediaFile->Copyright() << "\n";

		fReadyToPlay = true;
		fInitCheckVal = B_OK;

	} catch (status_t error) {
		Reset();
		fInitCheckVal = error;
		fReadyToPlay = false;
	}
	
	return fInitCheckVal;
}

status_t
MediaTrackController::SetVideoTrack(BMediaTrack *track, const media_format *format)
{
	if (fVideoTrack)
		// is it possible to have multiple video tracks?
		return B_MEDIA_ALREADY_CONNECTED;

	fVideoTrack = track;

	BRect bitmapBounds(0, 0, 
		format->u.encoded_video.output.display.line_width - 1,
		format->u.encoded_video.output.display.line_count - 1);

	fVideoWidth = format->u.raw_video.display.line_width;
	fVideoHeight = format->u.raw_video.display.line_count;
	fVideoFramePeriod = (bigtime_t)(1000000 / format->u.raw_video.field_rate);
	fBitmapDepth = BScreen().ColorSpace();

	// loop, asking the track for a format we can deal with
	for(;;) {

		fBitmap = new BBitmap(bitmapBounds, fBitmapDepth);

		media_format format;
		memset(&format, 0, sizeof(format));
	
		format.u.raw_video.last_active = (uint32)(bitmapBounds.Height() - 1);
		format.u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
		format.u.raw_video.pixel_width_aspect = 1;
		format.u.raw_video.pixel_height_aspect = 3;
		format.u.raw_video.display.format = fBitmap->ColorSpace();
		format.u.raw_video.display.line_width = (int32)bitmapBounds.Width();
		format.u.raw_video.display.line_count = (int32)bitmapBounds.Height();
		format.u.raw_video.display.bytes_per_row = fBitmap->BytesPerRow();

		media_format oldFormat;
		oldFormat = format;
		fVideoTrack->DecodedFormat(&format);
		if (oldFormat.u.raw_video.display.format == format.u.raw_video.display.format) 
			break;

		PRINT(("wanted cspace 0x%x, but it was reset to 0x%x\n",
			fBitmapDepth, format.u.raw_video.display.format));
		
		fBitmapDepth = format.u.raw_video.display.format;
		delete fBitmap;
	}

	fDescriptionString << (int32) fVideoWidth << " x " << (int32) fVideoHeight << ",  "
		<< (int32) format->u.encoded_video.output.field_rate << " fps\n";

	bigtime_t time = fCurTime;	
	fVideoTrack->SeekToTime(&time);

	int64 dummyNumFrames = 0;
	media_header header;
	fVideoTrack->ReadFrames((char *)fBitmap->Bits(), &dummyNumFrames, &header);

	time = fCurTime;
	fVideoTrack->SeekToTime(&time);	

	return B_OK;
}


status_t
MediaTrackController::SetAudioTrack(const char *name, BMediaTrack *track,
	const media_format *format)
{
	if (fAudioTrack)
		// is it possible to have multiple tracks?
		return B_MEDIA_ALREADY_CONNECTED;

	fAudioTrack = track;

	fAudioOutput = new AudioOutput(fAudioTrack, name);
	status_t err = fAudioOutput->InitCheck();
	if (err != B_OK) {
		delete fAudioOutput;
		fAudioOutput = 0;
		fAudioTrack = 0;

		return err;
	}

	fAudioDumpingBuffer = malloc(format->u.raw_audio.buffer_size);

	switch (format->u.raw_audio.format) {
		case media_raw_audio_format::B_AUDIO_UCHAR:
			fDescriptionString << "8 Bit ";
			break;
		
		case media_raw_audio_format::B_AUDIO_SHORT:
			fDescriptionString << "16 Bit ";
			break;
		case media_raw_audio_format::B_AUDIO_FLOAT:
			fDescriptionString << "32 Bit (float) ";
			break;
		case media_raw_audio_format::B_AUDIO_INT:
			fDescriptionString << "32 Bit (int) ";
			break;
	
		default:
			fDescriptionString << "Unknown bit rate ";
	}

	if (format->u.raw_audio.channel_count == 0)
		fDescriptionString << "Mono\n";
	else 
		fDescriptionString << "Stereo\n";

	return B_OK;
}


status_t 
MediaTrackController::ConnectVideoOutput(VideoView *view)
{
	fVideoView = view;
	return B_OK;
}

bigtime_t 
MediaTrackController::Position() const
{
 	return fCurTime;
}

void 
MediaTrackController::SetInPoint(bigtime_t pos, bool tracking)
{
	_SetInPoint(pos);
	if (Position() < pos) {
		if (!fScrubbing) {
			SetPosition(pos, 0);
			if (IsPlaying() && tracking)
				PauseAfterThumbNudge();
		}
	}
}

void 
MediaTrackController::SetOutPoint(bigtime_t pos, bool tracking)
{
	_SetOutPoint(pos);

	if (Position() > pos) {
		if (!fScrubbing) {
			SetPosition(pos, 0);
			if (IsPlaying() && tracking)
				PauseAfterThumbNudge();
		}
	}
}

void 
MediaTrackController::_SetInPoint(bigtime_t pos)
{
	if (pos <= 0)
		pos = 0;
	else if (pos > fLength)
		pos = fLength;

	fInPoint = pos;	

	if (fOutPoint < pos)
		fOutPoint = pos;
}

void 
MediaTrackController::_SetOutPoint(bigtime_t pos)
{
	if (pos <= 0)
		pos = 0;
	else if (pos > fLength)
		pos = fLength;

	fOutPoint = pos;	

	if (fInPoint > pos)
		fInPoint = pos;
}


void 
MediaTrackController::Play()
{
	PostMessage(kPlay);
}

void 
MediaTrackController::Stop()
{
	PostMessage(kStop);
}

void 
MediaTrackController::Pause()
{
	PostMessage(kPause);
}

void 
MediaTrackController::NudgeForward()
{
	PostMessage(kNudgeForward);
}

void 
MediaTrackController::NudgeBackward()
{
	PostMessage(kNudgeBackward);
}

void 
MediaTrackController::ScanForward()
{
	PostMessage(kScanForward);
}

void 
MediaTrackController::ScanBackward()
{
	PostMessage(kScanBackward);
}

void 
MediaTrackController::DoneSkipping()
{
	PostMessage(kDoneScanning);
}

void 
MediaTrackController::PauseAfterThumbNudge()
{
	Pause();
}

void 
MediaTrackController::BumpInPointAndRewind()
{
}

void 
MediaTrackController::BumpOutPointAndGoToEnd()
{
}

float 
MediaTrackController::Volume()
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

	// PRINT(("Volume volume %f, raw %f\n", volume, rawVolume));
	return volume;
}

void 
MediaTrackController::SetVolume(float volume)
{
	float rawVolume = volume;
	rawVolume = rawVolume/0.8;

	if (rawVolume > 1)
		rawVolume = rawVolume * rawVolume * rawVolume;
	else
		rawVolume = rawVolume * rawVolume;
	
	// PRINT(("SetVolume volume %f, raw %f\n", volume, rawVolume));
	SetRawVolume(rawVolume);
}

float 
MediaTrackController::RawVolume() const
{
	if (!fAudioOutput)
		return 0;
		
	return fAudioOutput->Volume();
}

void 
MediaTrackController::SetRawVolume(float volume)
{
	if (fAudioOutput)
		fAudioOutput->SetVolume(volume);
}

void 
MediaTrackController::SeekSoundTo(bigtime_t time)
{
	ASSERT(fAudioTrack);
	if (time < 0)
		time = 0;
	else if (time > fLength)
		time = fLength;

	// Seek the extractor as close as possible
	bigtime_t audioStartTime = time;
	fAudioOutput->SeekToTime(&audioStartTime);
	
	// Read frames until we get less than 50ms ahead.
	bigtime_t lastTime = audioStartTime;
	while (time - audioStartTime > 50000) {
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

status_t
MediaTrackController::ReadyToLoop(loop_state* outState)
{
	if (!fLooping) {
		// Initialize.
		fLooping = true;
		fLocalScrubbing = false;
		fLocalLength = Length();
		
		fCounterTrack = (fVideoTrack != NULL) ? fVideoTrack : fAudioTrack;
	
		fVideoStartTime = 0;
		fStartTime = system_time() - fCounterTrack->CurrentTime();		
	
		fCurScrubbing = system_time();
		fLastScrubbing = fCurScrubbing;
	
		fNumFramesToSkip = 0;
		fNumSkippedFrames = 0;
		fNumSkippedInARow = 0;
			// count how many frames we skipped in a sequence
			// every now and then show a frame even if it's behind
		fLastLocalScrubTargetPosition = 0;
			// used to detect if there was a change in the scrub location
	
		fCurTime = 0;
		fScrubTime = 0;
		fSeekTime = 0;
		fSeekFrame = 0;
		fSeekToTimeNeeded = false;
		fSeekToFrameNeeded = false;
		fScanningForward = false;
		fScanningBackward = false;
	}
	
	while (!ExecuteLoop(outState)) ;
	
	return B_OK;
}

bool
MediaTrackController::ExecuteLoop(loop_state* outState)
{
	if (!fSeekToTimeNeeded && !fSeekToFrameNeeded && !fScrubbing
		&& !fScanningForward && !fScanningBackward && fPlayState != fPlaying)
		// nothing to do
		return true;
	else
		// attention needed, don't hang around waiting for messages
		outState->next_loop_time = 0;

	float localScrubTargetPosition = 0;
	if (fLocalScrubbing) 
		// we are scrubbing, read the scrub slider value for later
		localScrubTargetPosition = GetScrubTargetPosition(fLocalScrubTarget,
			fLocalLength);

	if (fScrubbing) {
		// transform a scrub into a seek
		fSeekToTimeNeeded = true;
		if (fAudioOutput) {
			// need to start and stop audio here so that it get's played
			// properly during the scrub

			if (fLastLocalScrubTargetPosition == localScrubTargetPosition)
				// we are pausing in the scrub, stop sound so that
				// we don't glitch on the spot
				fAudioOutput->Stop();
			else 
				fAudioOutput->Play();
		}
		fSeekTime = (bigtime_t)((double)localScrubTargetPosition * (double)Length());
		
		// drag the in/out points along with the main thumb
		if (fInPoint > fSeekTime)
			_SetInPoint(fSeekTime);
		if (fOutPoint < fSeekTime)
			_SetOutPoint(fSeekTime);
		
		fLastLocalScrubTargetPosition = localScrubTargetPosition;
	}

	// Handle seeking
	if (fSeekToTimeNeeded) {

		// pin the seek time within the in and out point
		if (fSeekTime < fInPoint)
			fSeekTime = fInPoint;
		else if (fSeekTime > fOutPoint)
			fSeekTime = fOutPoint;

		if (fVideoTrack) {
			// Seek the fSeekTime as close as possible
			fVideoStartTime = fSeekTime;
			
			fVideoTrack->SeekToTime(&fVideoStartTime);
			
			// Read frames until we get less than 50ms ahead.
			bigtime_t lastTime = fVideoStartTime;
			do {
				media_header header;
				int64 numReadFrames = 1;
				if (fVideoTrack->ReadFrames((char *)fBitmap->Bits(), &numReadFrames,
					&header) != B_OK)
					break;
				fVideoStartTime = header.start_time;
				if (!numReadFrames || fVideoStartTime <= lastTime)
					break;
				lastTime = fVideoStartTime;
			} while (fSeekTime - fVideoStartTime > 50000);
		}
		
		if (fAudioTrack) 
			SeekSoundTo(fSeekTime);
		else
			fStartTime = system_time() - fVideoStartTime;
		
		// Set the current time
		fCurTime = fSeekTime;	
		fSeekToTimeNeeded = false;
		
	} else if (fSeekToFrameNeeded) {
			
		if (fVideoTrack) {

			// pin the seek frame within the in and out point
			if (fSeekFrame < (fInPoint / fVideoFramePeriod))
				fSeekFrame = fInPoint / fVideoFramePeriod;
			else if (fSeekFrame > (fOutPoint / fVideoFramePeriod))
				fSeekFrame = fOutPoint / fVideoFramePeriod;

			int64 frame = fSeekFrame - 1;
				// start at one frame before the one we actually want
				// this is a very simple and not entirely efficient way
				// of dealing with seeking back a frame and not getting stuck
				// on a keyframe
			
			if (fSeekFrame < 0)
				fSeekFrame = 0;

			// seek to the nearest keyframe
			fVideoTrack->SeekToFrame(&frame);
			
			do {
				// read frames to get to the one we are interrested in
				media_header header;
				int64 numReadFrames = 1;
				if (fVideoTrack->ReadFrames((char *)fBitmap->Bits(), &numReadFrames,
					&header) != B_OK)
					break;
				fVideoStartTime = header.start_time;
			} while (fVideoTrack->CurrentFrame() < fSeekFrame);

			fSeekTime = fCounterTrack->CurrentTime();
		}

		if (fAudioTrack) 
			SeekSoundTo(fSeekTime);

		fCurTime = fSeekTime;
		fStartTime = system_time() - fVideoStartTime;
		fSeekToFrameNeeded = false;
	} else {
	
		// Handle normal playing mode

		// Get the next video frame, if any
		if (fVideoTrack) {
			media_header header;
			int64 numReadFrames = 1;

//PRINT(("frame %Ld, total frames %Ld, time %Ld, total time %Ld\n",
//	fVideoTrack->CurrentFrame(), fVideoTrack->CountFrames(),
//	fVideoTrack->CurrentTime(), fVideoTrack->Duration()));

			status_t result = fVideoTrack->ReadFrames((char *)fBitmap->Bits(),
				&numReadFrames, &header);
			if (result != B_OK || numReadFrames == 0) {
				if (result == B_LAST_BUFFER_ERROR) 
					OutPointHit();
				else
					Pause();
				return true;
			}
			fVideoStartTime = header.start_time;
		}

		if (fScanningForward) {
			// when scanning forward, we are playing video as fast as we can
			// - sync everyting else up to it
			if (fVideoTrack) {
				fCurTime = fVideoStartTime;
				if (fAudioTrack) 
					SeekSoundTo(fCurTime);					
			}				
		}

		// Estimate snoozeTime
		if (fAudioTrack)
			fStartTime = fAudioOutput->TrackTimebase();

		bigtime_t snoozeTime;
		if (fVideoTrack)
			snoozeTime = fVideoStartTime - (system_time() - fStartTime);
		else
			snoozeTime = 25000;

		if (snoozeTime > 3000000)
			// something is wrong, don't snooze this long
			snoozeTime = 3000000;
			
		// Handle timing issues
		if (fScanningForward || snoozeTime > 5000) {
			fSnoozing = true;
			Unlock();
			if (fScanningForward)
				snooze(1000);
			else
				snooze(snoozeTime - 1000);
			Lock();
			fSnoozing = false;
		} else if (snoozeTime < -5000) {
			PRINT(("frame behind by %Ld, dropping\n", snoozeTime));
			fNumSkippedFrames++;
			fNumFramesToSkip++;
		}
		
		// Set the current time
		if (!fScrubbing) {
			fCurTime = system_time() - fStartTime;
			if (fCurTime < fSeekTime)
				fCurTime = fSeekTime;
		}				
	}

	// Handle the drawing : no drawing if we need to skip a frame...
	if (fNumSkippedFrames > 0 && fNumSkippedInARow < 10) {
		fNumSkippedFrames--;
		fNumSkippedInARow++;
	// If we can't lock the window after 50ms, better to give up for
	// that frame...
	} 
	else if (fVideoView ) //&& fVideoView->SetBitmap(fBitmap)) 
	{
		fNumSkippedInARow = 0;
		// In scrub mode, don't scrub more than 10 times a second
		if (fScrubbing) {
			bigtime_t snoozeTime = (100000 + fLastScrubbing) - system_time();
			if (snoozeTime > 4000) {
				fSnoozing = true;
				Unlock();
				snooze(snoozeTime - 1000);
				Lock();
				fSnoozing = false;
			}
			fLastScrubbing = fCurScrubbing;
		}
	}

	if (fCurTime >= fOutPoint && fPlayState == kPlaying && !fScrubbing && !fScanningForward)
		OutPointHit();
	return true;
}


void MediaTrackController::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_QUIT_REQUESTED:
			return;

		case kPlay:
			fScanningForward = false;
			fScanningBackward = false;
			fPlayState = kPlaying;
			if (fAudioTrack)
				fAudioOutput->Play();
			fStartTime = system_time() - fCounterTrack->CurrentTime();
			break;
		
		case kPause:
			fScanningForward = false;
			fScanningBackward = false;
			fPlayState = kPaused;
			if (fAudioTrack)
				fAudioOutput->Stop();
			break;
		
		case kStop:
			fScanningForward = false;
			fScanningBackward = false;
			fPlayState = kStopped;
			fSeekToTimeNeeded = true;
			fSeekTime = fInPoint;
			if (fAudioTrack)
				fAudioOutput->Stop();
			break;

		case kSetPosition:
			{
				bigtime_t pos;
				bigtime_t when;

				message->FindInt64("be:pos", (int64 *)&pos);
				message->FindInt64("be:when", (int64 *)&when);

				if (pos > fOutPoint)
					pos = fOutPoint;
				if (pos < fInPoint)
					pos = fInPoint;
			
				fSeekToTimeNeeded = true;
				fSeekTime = pos;
				break;
			}

		case kSetScrubbing:
			{
				fScanningForward = false;
				fScanningBackward = false;

				bool scrub = false;
				status_t result = message->FindBool("be:scrub", &scrub);
				ASSERT(result == B_OK);
				if (scrub == fScrubbing)
					break;
				
PRINT(("scrubbing %s\n", scrub ? "on" : "off"));

				fLocalScrubbing = scrub;
				fScrubbing = scrub;

				if (scrub) {
					// if scrub on, record the messenger of the control driving us
					result = message->FindMessenger("be:scrubDriver",
						&fLocalScrubTarget);
					ASSERT(result == B_OK);
					fCurScrubbing = system_time();
					// go back to read scurubber value
					return;
				} else if (fAudioTrack) {
					// done scrubbing, we were turning audio
					// on and off, make sure it is in the right state
					if (fPlayState == kPlaying)
						fAudioOutput->Play();
					else
						fAudioOutput->Stop();
				}
			}
			break;
			
		case kNudgeForward:
		case kNudgeBackward:
			{
				fScanningForward = false;
				fScanningBackward = false;
				int64 current = fCounterTrack->CurrentFrame();
				int64 delta = 0;
				bigtime_t timeDelta = 0; // for sound
				if (!IsPlaying() && fCounterTrack == fVideoTrack)
					delta = 1;
				else if (message->what == kNudgeForward) {
					delta = 20;
					timeDelta = 1000000;
				} else {
					delta = 40;
					timeDelta = 2000000;
				}

				if (message->what != kNudgeForward) {
					delta = -delta;
					timeDelta = -timeDelta;
				}
				
				fSeekFrame = current + delta;
				fSeekTime = fCounterTrack->CurrentTime() + timeDelta;
				fSeekToFrameNeeded = true;
				break;
			}
			
		case kScanBackward:
			// for now scan back by doing a seek
			fSeekToTimeNeeded = true;
			fSeekTime = fCurTime - 2000000;
			if (fAudioTrack)
				fAudioOutput->Play();
				// seek the audio too
			break;
			
		case kScanForward:
			if (fVideoTrack) {
				fScanningForward = true;
			} else {
				fSeekToTimeNeeded = true;
				fSeekTime = fCurTime + 2000000;
			}
			if (fAudioTrack)
				fAudioOutput->Play();
				// seek the audio too
			break;

		case kDoneScanning:
			fScanningForward = false;
			fScanningBackward = false;
			if (fAudioTrack && fPlayState != kPlaying)
				fAudioOutput->Stop();
			break;
	}
}


void
AudioOutput::AudioPlay(void *castToThis, void *buffer, size_t bufferSize,
	const media_raw_audio_format &format)
{
	uint32 filled;

	AudioOutput *audioOutput = (AudioOutput *)castToThis;
	audioOutput->Lock();

	bool update_trackTime = true;

	if (audioOutput->isPlaying) {
		media_header header;
		int64 frameCount = 1;
		status_t err = audioOutput->track->ReadFrames((char*)buffer, &frameCount, &header);
		if (err != B_OK || frameCount < 0) {
			memset((char*)buffer, audioOutput->default_data, audioOutput->buffer_size);
			update_trackTime = false;
		} else {
			filled = audioOutput->frame_size * frameCount;
			if (filled < audioOutput->buffer_size)
				memset((char *)buffer+filled, audioOutput->default_data,
					audioOutput->buffer_size-filled);
			if (err != B_OK)
				update_trackTime = false;
		}
	} else
		memset((char *)buffer, audioOutput->default_data, audioOutput->buffer_size);

	audioOutput->perfTime = audioOutput->player->PerformanceTime();
	if (update_trackTime)
		audioOutput->trackTime = audioOutput->track->CurrentTime();
	else
		audioOutput->trackTime += (bigtime_t)(1e6*(float)bufferSize /
			((float)audioOutput->frame_size * audioOutput->frame_rate));

	audioOutput->Unlock();
}

AudioOutput::AudioOutput(BMediaTrack *new_track, const char *name)
{
	media_format format;

	lock_count = 0;
	lock_sem = create_sem(0, "audio_output sem");
	track = new_track;
	isPlaying = false;
	perfTime = -1;
	perfTime = -1;
	trackTime = 0;
	
	fInitCheckValue = B_MEDIA_BAD_FORMAT;
	
	track->DecodedFormat(&format);
	switch (format.u.raw_audio.format) {
	case media_raw_audio_format::B_AUDIO_UCHAR :
		default_data = 0x80;
		frame_size = 1;
		break;
	case media_raw_audio_format::B_AUDIO_SHORT :
		default_data = 0;
		frame_size = 2;
		break;
	case media_raw_audio_format::B_AUDIO_INT :
		default_data = 0;
		frame_size = 4;
		break;
	case media_raw_audio_format::B_AUDIO_FLOAT :
		default_data = 0;
		frame_size = 4;
		break;
	default :
		player = NULL;
		return;
	}
	channelCount = format.u.raw_audio.channel_count;
	frame_size *= channelCount;
	buffer_size = format.u.raw_audio.buffer_size;
	frame_rate = format.u.raw_audio.frame_rate;

	player = new BSoundPlayer(&format.u.raw_audio, name, AudioOutput::AudioPlay);
	fInitCheckValue = player->InitCheck();
	if (fInitCheckValue != B_OK) {
		delete player;
		player = NULL;
	} else {
		player->SetCookie(this);
		player->Start();
		player->SetHasData(true);
	}
}

AudioOutput::~AudioOutput()
{
	if (player)
		player->Stop();
	delete player;
	delete_sem(lock_sem);
}

void
AudioOutput::Lock()
{
	if (atomic_add(&lock_count, 1) > 0)
		acquire_sem(lock_sem);
}

void
AudioOutput::Unlock()
{
	if (atomic_add(&lock_count, -1) > 1)
		release_sem(lock_sem);
}

status_t
AudioOutput::SeekToTime(bigtime_t *inout_time)
{
	Lock();
	status_t err = track->SeekToTime(inout_time);
	trackTime = *inout_time;
	Unlock();
	return err;
}

status_t
AudioOutput::Play()
{
	Lock();
	isPlaying = true;
	Unlock();
	return B_OK;
}

status_t
AudioOutput::Stop()
{
	isPlaying = false;
	return B_OK;
}

bigtime_t
AudioOutput::TrackTimebase() const
{
	return perfTime - trackTime;
}

status_t 
AudioOutput::InitCheck() const
{
	return fInitCheckValue;
}

BMediaTrack *
AudioOutput::Track() const
{
	return track;
}

bool 
AudioOutput::IsPlaying() const
{
	return isPlaying;
}

float 
AudioOutput::Volume() const
{
	if (!player)
		return 0;
	
	return player->Volume();
}

void 
AudioOutput::SetVolume(float volume)
{
	if (player)
		player->SetVolume(volume);
}


#endif

