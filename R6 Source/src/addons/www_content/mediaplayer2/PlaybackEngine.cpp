#include <Autolock.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <ResourceCache.h>
#include <SoundPlayer.h>
#include <InterfaceDefs.h>
#include <signal.h>
#include "PasswordManager.h"
#include "IOAdapter.h"
#include "PlaybackEngine.h"
#include "MidiHandler.h"

enum WaitFlags {
	kPlaybackWaiting,
	kDecoderWaiting,
};

const int32 kEncodedDataBufferSize = 32*1024;
const bigtime_t kDecodeAheadTimeUsec = 200000;
const bigtime_t kUpdateInterval = 500000;
const char *kPlaylistMime[] = { "audio/x-mpegurl", "audio/x-scpls", NULL};
const char *kPlaylistExtensions[] = { "pls", "m3u", NULL};

PlaybackEngine::PlaybackEngine()
	:	fIOAdapter(0),
		fMediaFile(0),
		fMidiHandler(0),
		fDecoderThread(-1),
		fSoundPlayer(0),
		fAudioFrameSize(0),
		fAudioZeroValue(0),
		fAudioTrack(0),
		fProtocol(0),
		fStopFlag(false),
		fPaused(false),
		fNameChanged(false),
		fStopNotified(false),
		fStopNotify(0),
		fLastNotify(0),
		fUpdateHook(0),
		fUpdateData(0),
		fErrorHook(0),
		fErrorData(0),
		fPropertyHook(0),
		fPropertyData(0),
		fDecodedBufferAvailable(0),
		fFreeBufAvail(-1),
		fInitializeWait(-1),
		fRawAudioBuffer(0),
		fReadPos(0),
		fWritePos(0),
		fBufferCount(0),
		fNumFramesOutput(0),
		fEndOfStream(false),
		fOpening(false),
		fRebuffering(false)
{
}

PlaybackEngine::~PlaybackEngine()
{
	Stop();
}

status_t PlaybackEngine::Start(const URL &url, bool sync)
{
	fURLList.push_front(url);

	if(sync)
		fInitializeWait = create_sem(0,"wait");
	status_t ret=NextURL();
	acquire_sem(fInitializeWait);
	return ret;
}


long PlaybackEngine::DoNextURL(void *data)
{
	return ((PlaybackEngine*)data)->NextURL();
}

status_t PlaybackEngine::NextURL()
{
	if (!fStopFlag)
		Stop();

	fStopFlag = false;
	fFreeBufAvail = create_sem(0, "Decoded Audio Buffer Full");	// create here to prevent deadlock
	fDecoderThread = spawn_thread(AsyncOpenStart, "Decode Audio", 17, this);
	resume_thread(fDecoderThread);
	return B_OK;
}

bool PlaybackEngine::DidNameChange()
{
	bool tmp = fNameChanged;
	fNameChanged = false;
	return tmp;
}

void PlaybackEngine::Stop()
{
	// Order is very important in this function, as it is deadlock prone.
	// The most important thing is not to wait for the thread to exit until we
	// are sure that there's nothing it could be blocking on.
	fStopFlag = true;

	if(fMidiHandler)
	{
		delete fMidiHandler;
		fMidiHandler = NULL;
		return;
	}

	delete_sem(fFreeBufAvail);
	fFreeBufAvail = -1;

	// Wait for read ahead thread
	int32 retval;
	if (fDecoderThread > 0) {
		send_signal(fDecoderThread, SIGUSR1);
		wait_for_thread(fDecoderThread, &retval);
		fDecoderThread = -1;
	}


	if (fProtocol)
		fProtocol->Abort();

	if (fIOAdapter)
		fIOAdapter->Abort();

	// Free up resources
	delete fProtocol;
	fProtocol = 0;
	delete fSoundPlayer;
	fSoundPlayer = 0;
	delete fMediaFile;
	fMediaFile = 0;
	fAudioTrack = 0;
	fNumFramesOutput = 0;
	fIOAdapter = 0;	// Note: BMediaFile owns stream, so it will delete it automatically.
	delete [] fRawAudioBuffer;
	fRawAudioBuffer = 0;
}

void PlaybackEngine::TogglePause()
{
	if (fSoundPlayer) {
		if (fPaused)
			fSoundPlayer->Start();
		else
			fSoundPlayer->Stop();
	}
	else if (fMidiHandler) {
		if (fPaused)
			fMidiHandler->Resume();
		else
			fMidiHandler->Pause();
	}
	
	fPaused = !fPaused;
	NotifyProperty("Status");
}

const URL& PlaybackEngine::GetUrl() const
{
	return fURLList.front();
}

void PlaybackEngine::SetStopNotify(NotifyFunc hook, void *data)
{
	fStopNotify = hook;
	fStopData = data;
}

void PlaybackEngine::SetUpdateNotify(NotifyFunc hook, void *data)
{
	fUpdateHook = hook;
	fUpdateData = data;
}

void PlaybackEngine::SetErrorNotify(ErrorFunc hook, void *data)
{
	fErrorHook = hook;
	fErrorData = data;
}

void PlaybackEngine::SetPropertyNotify(PropertyFunc hook, void *data)
{
	fPropertyHook = hook;
	fPropertyData = data;
}

void PlaybackEngine::ReportError(const char *error)
{
	if(fErrorHook)
		fErrorHook(fErrorData,error);
}

void PlaybackEngine::NotifyProperty(const char *prop)
{
	if(fPropertyHook)
		fPropertyHook(fPropertyData,prop);
}

float PlaybackEngine::GetCompletion()
{
	if(fMidiHandler)
		return float(fMidiHandler->CurrentTime())/float(fMidiHandler->Duration());
	return fIOAdapter ? (float) fIOAdapter->AmountRead() / (float) fContentLength : 0.0;
}

int32 PlaybackEngine::GetElapsedTime()
{
	if(fMidiHandler)
		return fMidiHandler->CurrentTime()/1000000;

	return fIOAdapter ? fNumFramesOutput/44100 : 0;
}

int32 PlaybackEngine::GetTotalTime()
{
	if(fMidiHandler)
		return fMidiHandler->Duration()/1000000;

	// servers don't specify a uniform length for "infinite" content,
	// so we simply take everything that's "very long" to be "infinite".
	if(fContentLength >= 20000000)
		return -1;

	return fAudioTrack ? fAudioTrack->Duration()/1000000 : -1;
}

const char* PlaybackEngine::GetStreamName() const
{
	return fStreamName.String();
}

const char* PlaybackEngine::GetStreamGenre() const
{
	return fStreamGenre.String();
}

const char* PlaybackEngine::GetHomePage() const
{
	return fStreamUrl.String();
}

int32 PlaybackEngine::AsyncOpenStart(void *castToEngine)
{
	PlaybackEngine *engine = reinterpret_cast<PlaybackEngine*>(castToEngine);
	engine->fOpening=true;
	engine->NotifyProperty("Status");
	status_t ret = engine->InitializeStream();
	engine->fOpening=false;
	engine->NotifyProperty("Status");
	delete_sem(engine->fInitializeWait);
	engine->fInitializeWait = -1;

	if ( ret < 0 && !engine->fStopFlag)
		engine->Close();	// An error occured
	else if (!engine->fMidiHandler)
		engine->DecodeAndBufferAudio();
	
	return 0;
}

inline void PlaybackEngine::ReadBufferedAudio(void *buffer, size_t bufferSize)
{
//printf("available: %d/%d (rebuffering: %d\n",fDecodedBufferAvailable,fBufferCount,fRebuffering);
	if (fDecodedBufferAvailable == 0  || fRebuffering) {
		memset(buffer, fAudioZeroValue, bufferSize);
		if (fEndOfStream)
		{
			fSoundPlayer->SetCallbacks(NULL,NULL,NULL);
			fURLList.pop_front();
			if (!fURLList.empty())
			{
				// Call NextURL() (cause it to be called in another thread)
				fNameChanged = true;
				resume_thread(spawn_thread(DoNextURL,"DoNextURL",B_NORMAL_PRIORITY,this));
			}
			else
			{
				Close();
			}
		}
		else
		{
			if(!fRebuffering)
			{
				// perhaps it's not buffering yet (if you pull the plug, the buffer will
				// drain and the thread will block on ReadFrames() before it realizes it
				// needs to rebuffer), but it will be soon, and we should indicate that now.
				fRebuffering = true;
				NotifyProperty("Status");
			}
		}
		return;
	}

	memcpy(buffer, fRawAudioBuffer + fAudioBufferSize * fReadPos, bufferSize);
	fNumFramesOutput += bufferSize/fAudioFrameSize;
	atomic_add(&fDecodedBufferAvailable, -1);
	fReadPos = (fReadPos + 1) % fBufferCount;
	release_sem_etc(fFreeBufAvail,1,B_DO_NOT_RESCHEDULE);
}

//
//	Sound player hook.  Don't block.  Do as little processing as possible
//
void PlaybackEngine::SoundPlayerHook(void *controller, void *buffer, size_t bufferSize,
	const media_raw_audio_format&)
{
	reinterpret_cast<PlaybackEngine*>(controller)->ReadBufferedAudio(buffer, bufferSize);
}

void PlaybackEngine::ProcessMeta(void *castToEngine, const char *name, const char *value)
{
	PlaybackEngine *engine = reinterpret_cast<PlaybackEngine*>(castToEngine);
	if (strcasecmp(name, "icy-name") == 0 || strcasecmp(name, "x-audiocast-name") == 0)
		engine->fStreamName = value;
	else if (strcasecmp(name, "icy-genre") == 0 || strcasecmp(name, "x-audiocast-genre") == 0)
		engine->fStreamGenre = value;
	else if (strcasecmp(name, "icy-url") == 0 || strcasecmp(name, "x-audiocast-url") == 0)
		engine->fStreamUrl = value;
}

void PlaybackEngine::Close()
{
	if (!fStopNotified) {
		fStopNotify(fStopData);
		fStopNotified = true;
	}
}

status_t PlaybackEngine::InitializeStream()
{
	status_t error = B_OK;
	fStreamName = "";
	fStreamGenre = "";
	fStreamUrl = "";
	bool authenticationretry = false;

	for (;;) {
		URL currentURL = fURLList.front();
		// Open a connection
		fProtocol = Protocol::InstantiateProtocol(currentURL.GetScheme());
		if (fProtocol == 0)
			return B_ERROR;
		
		fProtocol->SetMetaCallback(this, ProcessMeta);

		BMessage msg;
		char contentType[B_MIME_TYPE_LENGTH];
		
		// If this stream is getting played from a merlin protocol
		// (ie: it's an email attachment) then we have to extract
		// the name of it from the URL query string...
		if (strcmp(currentURL.GetScheme(), "merlin") == 0) {
			if (currentURL.GetQueryParameter("name", &fStreamName) != B_OK)
				fStreamName = "Email attachment";
		}
		//
		// HACK
		//
		// Try to connect twice, once not sending the user agent string and once sending it.
		// Shoutcast will not send mpeg streams if you have the user agent
		// It won't send you the playlist unless you do (it even returns 404 in this case).
		// Try first without it just in case this is an audio stream
		// NOTE: The second rep of this loop only gets executed if this is an invalid URL or for new shoutcast
		// servers.
		//
		// HACK II
		//
		// Some non-shoutcast servers will not send a 404 when receiving the terse header, but will
		// instead show a page that mentions that the document has moved.
		// To make the midi files at for example http://members.nbci.com/bestpopmidis/ work
		// (and possibly others too), we check to see if the file has an extension, and if so
		// use the full header. This should not interfere with shoutcast URLs.
		//

		bool hasextension = strlen(currentURL.GetExtension())>0;
		for (int retry = hasextension?1:0; retry < 2; retry++) {
			error = fProtocol->Open(currentURL, currentURL, &msg, retry == 0 ? TERSE_HEADER : 0);
			if (error >= B_OK)
				break;
		}
		
		if (error == B_AUTHENTICATION_ERROR && !authenticationretry) {
			// Handle authentication.  This is kind of an inefficient way
			// to do it, as we always fail and retry.
			const char *challenge = "";
			msg.FindString(S_CHALLENGE_STRING, &challenge);
			
			// Look up realm...
			BString user, password;
			if (passwordManager.GetPassword(currentURL.GetHostName(), challenge, &user, &password)) {
				URL augmentedURL(currentURL.GetScheme(), currentURL.GetHostName(), currentURL.GetPath(),
					currentURL.GetPort(), currentURL.GetFragment(), user.String(), password.String(),
					currentURL.GetQuery(), currentURL.GetQueryMethod());
	
				delete fProtocol;
				fProtocol = 0;
				fURLList.pop_front();
				fURLList.push_front(augmentedURL);
				msg.MakeEmpty();
				authenticationretry = true;
				continue; // try again with authentication
			}
		}

		if (error < 0)
			return error;
		authenticationretry = false;
		
		bigtime_t delay;
		if (fProtocol->GetRedirectURL(currentURL, &delay)) {
			delete fProtocol;
			fProtocol = 0;
			// replace the old URL with the redirected version
			fURLList.pop_front();
			fURLList.push_front(currentURL);
			continue;		// This is a redirect, loop and continue
		}

		fProtocol->GetContentType(contentType, B_MIME_TYPE_LENGTH);
		
		// Determine if this is some type of playlist
		bool isPlaylist = false;
		if (strcasecmp(contentType, "text/plain") == 0
			|| strcasecmp(contentType, "application/octet-stream") == 0) {
			// Sucker mime types, use extension
			for (const char **extension = kPlaylistExtensions; *extension; extension++) {
				if (strcasecmp(contentType, *extension) == 0) {
					isPlaylist = true;
					break;
				}
			}
		} else {
			for (const char **mime = kPlaylistMime; *mime; mime++)
				if (strcasecmp(contentType, *mime) == 0) {
					isPlaylist = true;
					break;
				}
		}
			
		if (isPlaylist) {
			// It is a playlist.  Load a bit of it and grep for a URL
			const int kBufSize = 0x2000;
			char buf[kBufSize];
			int totRead = 0;
			while (totRead < kBufSize) {
				int rd = fProtocol->Read(buf + totRead, kBufSize - totRead);
				if (rd <= 0)
					break;
					
				totRead += rd;
			}

			buf[totRead] = '\0';
		
			// Now look for a URL.
			BString firstURL;
			const char *urlStart = strstr(buf, "http://");
			if (urlStart) {
				const char *urlEnd = strchr(urlStart, '\n');
				if (urlEnd)
					firstURL.SetTo(urlStart, urlEnd - urlStart);
				else
					firstURL.SetTo(urlStart);
			}

			if (firstURL.Length() == 0)
				return -1;			// Give up
	
			// replace current URL (which turned out to be a playlist) with
			// all the URLs that it represents
			fURLList.pop_front();
			url_list::iterator insertpoint = fURLList.begin();
			do {
				URL newURL;
				newURL.SetTo(firstURL.String());
				fURLList.insert(insertpoint, newURL);
				// iter still points to the same item, which is where we keep
				// inserting the replacement-URLs

				urlStart += firstURL.Length();
				firstURL.Truncate(0);
				urlStart = strstr(urlStart, "http://");
				if (urlStart) {
					const char *urlEnd = strchr(urlStart, '\n');
					if (urlEnd)
						firstURL.SetTo(urlStart, urlEnd - urlStart);
					else
						firstURL.SetTo(urlStart);
				}
			} while (firstURL.Length());

			delete fProtocol;
			fProtocol = 0;
			continue;	// Redirected
		}
			
		break;
	}

	fContentLength = fProtocol->GetContentLength();

	// Start read ahead thread
	fIOAdapter = new IOAdapter(fProtocol, kEncodedDataBufferSize);
	if (fContentLength > 0)
		fIOAdapter->SetStreamLength(fProtocol->GetContentLength());

	char contentType[B_MIME_TYPE_LENGTH];
	fProtocol->GetContentType(contentType, B_MIME_TYPE_LENGTH);

	if(	(strcmp(contentType,"audio/x-midi") == 0) ||
		(strcmp(contentType,"audio/midi") == 0) ||
		(strcmp(contentType,"audio/mid") == 0) ||
		(strcmp(contentType,"audio/x-rmf") == 0) ||
		(strcmp(contentType,"audio/rmf") == 0) ||
		(strcasecmp(GetUrl().GetExtension(),"mid") == 0) ||
		(strcasecmp(GetUrl().GetExtension(),"rmf") == 0))
	{
		fMidiHandler = new MidiHandler(this);
		fMidiHandler->SetTo(fIOAdapter);
		if(fMidiHandler->InitCheck() == B_OK)
		{
			fStopFlag = false;
			fPaused = false;
			delete fIOAdapter;
			fIOAdapter = 0;
			delete fProtocol;
			fProtocol = 0;
			return B_OK;
		}
		delete fMidiHandler;
		fMidiHandler = NULL;
		// let it fall through, maybe the media kit can handle this after all
	}
	
	// Initialize media file
	fMediaFile = new BMediaFile(fIOAdapter, B_MEDIA_FILE_UNBUFFERED);
	if (fMediaFile->InitCheck() < 0)
		return fMediaFile->InitCheck();
	
	media_file_format fileFormat;
	error = fMediaFile->GetFileFormatInfo(&fileFormat);
	if (error != B_OK)
		return error;
	
	//	Step through the tracks and set each one up
	media_format audioFormat;
	for (int trackIndex = 0; trackIndex < fMediaFile->CountTracks(); trackIndex++) { 
		BMediaTrack *track = fMediaFile->TrackAt(trackIndex);
		media_codec_info codecInfo;
		bool codecInfoValid = (track->GetCodecInfo(&codecInfo) == B_OK);
		error = track->EncodedFormat(&audioFormat);
		if (error < 0)
			break;

		switch (audioFormat.type) {
			case B_MEDIA_RAW_AUDIO:
				fAudioTrack = track;
				break;
				
			case B_MEDIA_ENCODED_AUDIO:
				if (codecInfoValid) {
					error = track->DecodedFormat(&audioFormat);
					fAudioTrack = track;
				}
				
				break;

			default:
				fMediaFile->ReleaseTrack(track);
				;
		}
	}
	
	if (fAudioTrack == 0)
		return B_MEDIA_BAD_FORMAT;	// No audio
	
	// Set up the sound player
	error = fAudioTrack->DecodedFormat(&audioFormat);
	if(error != B_OK)
		return error;

	switch (audioFormat.u.raw_audio.format) {
		case media_raw_audio_format::B_AUDIO_UCHAR:
			fAudioZeroValue = 0x80;
			fAudioFrameSize = 1;
			break;
			
		case media_raw_audio_format::B_AUDIO_CHAR:
			fAudioZeroValue = 0;
			fAudioFrameSize = 1;
			break;

		case media_raw_audio_format::B_AUDIO_SHORT:
			fAudioZeroValue = 0;
			fAudioFrameSize = 2;
			break;

		case media_raw_audio_format::B_AUDIO_INT:
			fAudioZeroValue = 0;
			fAudioFrameSize = 4;
			break;

		case media_raw_audio_format::B_AUDIO_FLOAT:
			fAudioZeroValue = 0;
			fAudioFrameSize = 4;
			break;

		default:
			return B_ERROR;
	}

	fAudioFrameSize *= audioFormat.u.raw_audio.channel_count;
	fAudioBufferSize = audioFormat.u.raw_audio.buffer_size;
	fDecodedBufferAvailable = 0;
	fReadPos = 0;
	fWritePos = 0;
	fBufferCount = kDecodeAheadTimeUsec * ((static_cast<int32>(audioFormat.u.raw_audio.frame_rate)
		* fAudioFrameSize) / fAudioBufferSize) / 1000000;

	fRawAudioBuffer = new char[fBufferCount * fAudioBufferSize];
	if (fRawAudioBuffer == 0)
		return ENOMEM;

	fSoundPlayer = new BSoundPlayer(&audioFormat.u.raw_audio, "mediaplayer",
		PlaybackEngine::SoundPlayerHook);
	error = fSoundPlayer->InitCheck();
	if (error != B_OK) {
		delete fSoundPlayer;
		fSoundPlayer = NULL;
		return ENODEV;
	} else {
		fSoundPlayer->SetCookie(this);
		fSoundPlayer->SetHasData(true);
	}
	
	fStopFlag = false;
	fEndOfStream = false;
	fSoundPlayer->Start();
	fPaused = false;

	release_sem_etc(fFreeBufAvail,fBufferCount,B_DO_NOT_RESCHEDULE); // we now have this much 'empty' buffers
	return B_OK;
}

//
//	Read ahead thread.  Download, decode, and buffer up as much
//	data as possible.
//	
void PlaybackEngine::DecodeAndBufferAudio()
{
	while (!fStopFlag) {
		if (acquire_sem(fFreeBufAvail) < 0)
			return;

		char *data = fRawAudioBuffer + fAudioBufferSize * fWritePos;
		media_header header;
		int64 frameCount = 1;
		status_t error = fAudioTrack->ReadFrames(data, &frameCount, &header);
		if (error != B_OK || frameCount < 0) {
			fEndOfStream = true;
			memset(data, fAudioZeroValue, fAudioBufferSize);
			break;
		} else {
			size_t sizeUsed = fAudioFrameSize * static_cast<int32>(frameCount);
			if (sizeUsed < fAudioBufferSize) {
				// We've reached the end of the file, but there isn't enough
				// audio data to fill the entire buffer.  Zero out the rest
				// so there won't be noise.
				memset(data + sizeUsed, fAudioZeroValue, fAudioBufferSize - sizeUsed);
				fEndOfStream = true;
				break;
			}
		}

		fWritePos = (fWritePos + 1) % fBufferCount;
		DoUpdate();
		if(fDecodedBufferAvailable == 0)
		{
			fRebuffering = true; // start rebuffering if this was the first buffer
			NotifyProperty("Status");
		}
		if(atomic_add(&fDecodedBufferAvailable, 1)==fBufferCount-1)
		{
			if(fRebuffering)
			{
				fRebuffering = false; // stop rebuffering if this was the last buffer
				NotifyProperty("Status");
			}
		}
	}
}

void PlaybackEngine::DoUpdate(bool force)
{
	if (force || (fUpdateHook && system_time() - fLastNotify > kUpdateInterval)) {
		(*fUpdateHook)(fUpdateData);
		fLastNotify = system_time();
	}
}
