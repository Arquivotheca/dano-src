
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "TrackReader.h"
#include <MediaFile.h>
#include <Entry.h>
#include <File.h>
#include <MediaTrack.h>
#include <rt_alloc.h>
#include <assert.h>

#include "FileGameSound.h"
#include "PrivGameSound.h"


//	BUFPAGE is frames
#define BUFPAGE 2048
//	PAGECOUNT must be at least 4
#define PAGECOUNT 16


static status_t
init_check(
	BMediaFile * file,
	BTrackReader ** outReader)
{
	*outReader = NULL;
	status_t err = file->InitCheck();
	if (err < B_OK) return err;
	media_format fmt;
	for (int ix=0; ix<file->CountTracks(); ix++) {
		fmt.type = B_MEDIA_RAW_AUDIO;
		fmt.u.raw_audio = media_raw_audio_format::wildcard;
		BMediaTrack * trk = file->TrackAt(ix);
		err = trk->DecodedFormat(&fmt);
		if ((err < B_OK) || (fmt.type != B_MEDIA_RAW_AUDIO)) {
			file->ReleaseTrack(trk);
			continue;
		}
		if (trk->CountFrames() < 1) {
			file->ReleaseTrack(trk);
			continue;
		}
		*outReader = new BTrackReader(trk, fmt.u.raw_audio);
		break;
	}
	if (*outReader != 0) return B_OK;
	return (err < B_OK) ? err : B_MEDIA_BAD_FORMAT;
}

static status_t
try_raw_file(
	const entry_ref * ref,
	BTrackReader ** reader)
{
	BFile * file = new BFile;
	status_t err = file->SetTo(ref, O_RDONLY);
	if (err < B_OK) {
		delete file;
		return err;
	}
	mode_t mode;
	if ((err = file->GetPermissions(&mode)) < B_OK) {
		delete file;
		return err;
	}
	if (!S_ISREG(mode)) {
		delete file;
		return ENOENT;
	}
	if (!(mode & 0111)) {
		delete file;
		return EPERM;
	}
	off_t that;
	if (1 > (that = file->Seek(0, 2))) {
		delete file;
		return B_MEDIA_BAD_FORMAT;
	}
	file->Seek(0, 0);
	media_raw_audio_format fmt;
	fmt.frame_rate = 44100.0;
	fmt.format = media_raw_audio_format::B_AUDIO_SHORT;
	fmt.channel_count = 2;
	fmt.byte_order = B_HOST_IS_LENDIAN ? B_MEDIA_LITTLE_ENDIAN : B_MEDIA_BIG_ENDIAN;
	fmt.buffer_size = 4096;
	*reader = new BTrackReader(file, fmt);
	return B_OK;
}

BFileGameSound::BFileGameSound(const entry_ref *file, bool looping, BGameSoundDevice * device) :
	BStreamingGameSound(device)
{
	_m_looping = looping;
	_m_soundFile = NULL;
	_m_streamThread = -1;
	_m_loopCount = 0;
	_m_buffer = 0;
	_m_bufferSize = 0;
	_m_soundFile = new BMediaFile(file);
	_m_trackReader = NULL;
	status_t err = init_check(_m_soundFile, &_m_trackReader);
	_m_streamPort = -1;
	_m_pauseState = B_NOT_PAUSED;
	if (err < B_OK) {
		delete _m_soundFile;
		_m_soundFile = NULL;
		err = try_raw_file(file, &_m_trackReader);
	}
	if (err < B_OK) {
		fprintf(stderr, "BFileGameSound: file %s damaged or missing: %lx\n", file->name, err);
		SetInitError(err);
		return;
	}
	if (_m_trackReader->FrameSize() < 1) {
		fprintf(stderr, "BFileGameSound: file %s not supported\n", file->name);
		SetInitError(B_ERROR);
		return;
	}
	_m_streamPort = create_port(5, "load cmd");
	if (_m_streamPort < B_OK) {
		fprintf(stderr, "BFileGameSound: cannot create port: %lx\n", _m_streamPort);
		SetInitError(_m_streamPort);
		return;
	}
	_m_streamThread = spawn_thread(stream_thread, "load thread", B_URGENT_PRIORITY, this);
	if (_m_streamThread < B_OK) {
		fprintf(stderr, "BFileGameSound: cannot spawn thread: %lx\n", _m_streamThread);
		SetInitError(_m_streamThread);
		return;
	}
	gs_audio_format fmt;
	fmt = *(const gs_audio_format *)&_m_trackReader->Format();
	_m_bufferSize = BUFPAGE*PAGECOUNT*(fmt.format&0xf)*fmt.channel_count;
	if (fmt.format == 0x11) {
		_m_zero = 0x80;
	}
	else {
		_m_zero = 0;
	}
	_m_buffer = 0;
	err = SetParameters(BUFPAGE, &fmt, PAGECOUNT);
	if (err < B_OK) {
		fprintf(stderr, "BFileGameSound: SetParameters() failed: %lx\n", err);
		SetInitError(err);
		return;
	}
	PrivGameSound * pgs = PrivGameSound::CurPlayer();
	if (pgs == NULL) {
		fprintf(stderr, "BFileGameSound: cannot get player\n");
		SetInitError(B_ERROR);
		return;
	}
	err = pgs->GetSoundInfo(ID(), &fmt, (void **)&_m_buffer, &_m_bufferSize);
	if (err < B_OK) {
		fprintf(stderr, "BFileGameSound: GetSoundInfo() failed: %lx\n", err);
		SetInitError(err);
		return;
	}
	resume_thread(_m_streamThread);
}


BFileGameSound::BFileGameSound(const char *path, bool looping, BGameSoundDevice * device) :
	BStreamingGameSound(device)
{
	_m_looping = looping;
	_m_soundFile = NULL;
	_m_streamThread = -1;
	_m_loopCount = 0;
	_m_buffer = 0;
	_m_bufferSize = 0;
	_m_streamPort = create_port(5, "load cmd");
	entry_ref file;
	status_t err = get_ref_for_path(path, &file);
	_m_streamPort = -1;
	_m_pauseState = B_NOT_PAUSED;
	_m_trackReader = NULL;
	if (err < B_OK) {
		fprintf(stderr, "BFileGameSound: path '%s' not found\n", path);
		SetInitError(err);
		_m_soundFile = 0;
		return;
	}
	_m_soundFile = new BMediaFile(&file);
	err = init_check(_m_soundFile, &_m_trackReader);
	if (err < B_OK) {
		delete _m_soundFile;
		_m_soundFile = NULL;
		err = try_raw_file(&file, &_m_trackReader);
	}
	if (err < B_OK) {
		fprintf(stderr, "BFileGameSound: file %s damaged or missing: %lx\n", file.name, err);
		SetInitError(err);
		return;
	}
	if (_m_trackReader->FrameSize() < 1) {
		fprintf(stderr, "BFileGameSound: file %s not supported\n", file.name);
		SetInitError(B_ERROR);
		return;
	}
	_m_streamPort = create_port(5, "load cmd");
	if (_m_streamPort < B_OK) {
		fprintf(stderr, "BFileGameSound: cannot create port: %lx\n", _m_streamPort);
		SetInitError(_m_streamPort);
		return;
	}
	_m_streamThread = spawn_thread(stream_thread, "load thread", B_URGENT_PRIORITY, this);
	if (_m_streamThread < B_OK) {
		fprintf(stderr, "BFileGameSound: cannot spawn thread: %lx\n", _m_streamThread);
		SetInitError(_m_streamThread);
		return;
	}
	gs_audio_format fmt;
	fmt = *(const gs_audio_format *)&_m_trackReader->Format();
	_m_bufferSize = BUFPAGE*PAGECOUNT*(fmt.format&0xf)*fmt.channel_count;
	if (fmt.format == 0x11) {
		_m_zero = 0x80;
	}
	else {
		_m_zero = 0;
	}
	_m_buffer = 0;
	err = SetParameters(BUFPAGE, &fmt, PAGECOUNT);
	if (err < B_OK) {
		fprintf(stderr, "BFileGameSound: SetParameters() failed: %lx\n", err);
		SetInitError(err);
		return;
	}
	PrivGameSound * pgs = PrivGameSound::CurPlayer();
	if (pgs == NULL) {
		fprintf(stderr, "BFileGameSound: cannot get player\n");
		SetInitError(B_ERROR);
		return;
	}
	err = pgs->GetSoundInfo(ID(), &fmt, (void **)&_m_buffer, &_m_bufferSize);
	if (err < B_OK) {
		fprintf(stderr, "BFileGameSound: GetSoundInfo() failed: %lx\n", err);
		SetInitError(err);
		return;
	}
	resume_thread(_m_streamThread);
}


BFileGameSound::~BFileGameSound()
{
	if (_m_streamPort > -1) close_port(_m_streamPort);
	status_t err;
	if (_m_streamThread > -1) wait_for_thread(_m_streamThread, &err);
	if (_m_streamPort > -1) delete_port(_m_streamPort);
	if ((_m_trackReader != 0) && (_m_soundFile != 0)) _m_soundFile->ReleaseTrack(_m_trackReader->Track());
	delete _m_trackReader;
	delete _m_soundFile;
//	rtm_free(_m_buffer);
}

BGameSound *
BFileGameSound::Clone() const
{
	fprintf(stderr, "BFileGameSound cannot be cloned\n");
	return NULL;
}

status_t 
BFileGameSound::StartPlaying()
{
	status_t err = write_port_etc(_m_streamPort, cmdStartPlaying, NULL, 0, B_TIMEOUT, 5000000LL);
	if (err < 0) return err;
	return B_OK;
}

status_t 
BFileGameSound::StopPlaying()
{
	status_t err = write_port_etc(_m_streamPort, cmdStopPlaying, NULL, 0, B_TIMEOUT, 5000000LL);
	if (err < 0) return err;
	return B_OK;
}

status_t
BFileGameSound::Preload()
{
	sem_id sem = create_sem(0, "FileGameSound::Preload");
	status_t err = write_port_etc(_m_streamPort, cmdPreload, &sem, sizeof(sem), B_TIMEOUT, 5000000LL);
	if (err < 0) return err;
	err = acquire_sem_etc(sem, 1, B_TIMEOUT, 5000000LL);
	delete_sem(sem);
	return err;
}

void 
BFileGameSound::FillBuffer(void *inBuffer, size_t inByteCount)
{
	size_t cnt = inByteCount;
	size_t offs = (_m_loopCount % _m_bufferSize);
	assert(offs+cnt <= _m_bufferSize);
	if (offs+cnt > _m_bufferSize) {
		cnt = _m_bufferSize-offs;
	}
//	fprintf(stderr, "FillBuffer( 0x%x - 0x%x )\n", inBuffer, (char *)inBuffer+inByteCount-1);
//	we are loading into the play buffer -- no memcpy needed
//	memcpy(inBuffer, _m_buffer+offs, cnt);
//	if (cnt < inByteCount) {
//		memcpy(((char *)inBuffer)+cnt, _m_buffer, inByteCount-cnt);
//	}
	_m_loopCount += inByteCount;
	write_port_etc(_m_streamPort, cmdUsedBytes, &inByteCount, sizeof(inByteCount), B_TIMEOUT, 10000LL);
	switch (_m_pauseState) {
	case B_PAUSE_IN_PROGRESS: {
			gs_attribute attr[2];
			attr[0].attribute = B_GS_GAIN;
			attr[1].attribute = B_GS_PAN;
			GetAttributes(attr, 2);
			if ((attr[0].duration == 0) && (attr[1].duration == 0)) {
				_m_pauseState = B_PAUSED;
			}
		}
		break;
	case B_PAUSED:
		BStreamingGameSound::StopPlaying();
		break;
	}
}


status_t
BFileGameSound::stream_thread(
	void * obj)
{
	reinterpret_cast<BFileGameSound *>(obj)->LoadFunc();
	return B_OK;
}


void
BFileGameSound::LoadFunc()
{
	status_t err = 0;
	int32 count = 0;
	int32 cmd = 0;
	int32 deficit = 0;
	size_t offs = 0;
	size_t deferred_deficit = 0;
	bool first = true;
	bool playing = false;

	_m_stopCount = 0;
	int64 seekFrame = 0;
	_m_trackReader->SeekToFrame(&seekFrame);
	Load(offs, BUFPAGE*PAGECOUNT);

	while (true) {
		err = read_port(_m_streamPort, &cmd, &count, sizeof(count));
		if (err == B_INTERRUPTED) continue;
		if (err < B_OK) break;
		switch (cmd) {
		case cmdStartPlaying:
			BStreamingGameSound::StopPlaying();
			if (_m_pauseState > 0) {
				gs_attribute attr[2];
				attr[0].attribute = B_GS_GAIN;
				attr[1].attribute = B_GS_PAN;
				attr[0].duration = attr[1].duration = 0;
				attr[0].value = _m_pauseGain;
				attr[1].value = _m_pausePan;
				attr[0].flags = attr[1].flags = 0;
				SetAttributes(attr, 2);
			}
			_m_pauseState = 0;
			_m_stopping = false;
			_m_stopCount = 0;
			if (!first) {	//	make the first one trigger really quickly
				//	this code could also move to stop, but that seems wasteful...
				seekFrame = 0;
				offs = 0;
				_m_trackReader->SeekToFrame(&seekFrame);
				Load(offs, BUFPAGE*PAGECOUNT);
			}
			first = false;
			deficit = 0;
			deferred_deficit = 0;
			playing = true;
			BStreamingGameSound::StartPlaying();
			break;
		case cmdPreload:
			if (!first && !playing) {
				int64 seekFrame = 0;
				_m_trackReader->SeekToFrame(&seekFrame);
				offs = 0;
				Load(offs, BUFPAGE*PAGECOUNT);
				first = true;
			}
			release_sem(count);
			break;
		case cmdStopPlaying:
			_m_stopCount = 0;
			BStreamingGameSound::StopPlaying();
			playing = false;
			break;
		case cmdUsedBytes:
			deficit += deferred_deficit;
			deferred_deficit = count;
			if (deficit >= BUFPAGE) {
				deficit -= Load(offs, deficit);
			}
			if (_m_stopping) {
				_m_stopCount -= count;
				if (_m_stopCount <= 0) {
					BStreamingGameSound::StopPlaying();
					memset(_m_buffer, _m_zero, _m_bufferSize);
					_m_stopCount = 0;
				}
			}
			break;
		default:
			fprintf(stderr, "BFileGameSound: unknown command on load port: %ld\n", cmd);
			break;
		}
	}
}


//	The logic is complicated by two things:
//	1) The buffer logically wraps, but memory does not. We may have to read
//	   one logical chunk in two memory blocks.
//	2) The file logically wraps if _m_looping is true, so we may have to seek
//	   the file back to the beginning and re-read to fill one block.
size_t
BFileGameSound::Load(
	size_t & offset,
	size_t size)
{
	size_t loaded = 0;
again:
	size_t toload = size;
	if (offset + toload > _m_bufferSize) {
		toload = _m_bufferSize - offset;
	}
	read_data(_m_buffer+offset, toload);
	offset += toload;
	size -= toload;
	loaded += toload;
	if (offset > _m_bufferSize-_m_trackReader->FrameSize()) offset = 0;
	if (size > 0) goto again;
	return loaded;
}


void
BFileGameSound::read_data(
	char * dest,
	size_t bytes)
{
	if (_m_stopping) {
		memset(dest, _m_zero, bytes);
		return;
	}
	int32 frameSize = _m_trackReader->FrameSize();
again:
	int32 count = bytes/frameSize;
	int32 loaded = _m_trackReader->ReadFrames(dest, count);
//	fprintf(stderr, "read_data( 0x%x - 0x%x ) loaded 0x%x frames\n", dest, dest+bytes-1, loaded);
	if (loaded < count) {	//	end of file
		if (loaded < 0) {
			loaded = 0;
			goto stopping;
		}
		if (_m_looping) {
			int64 seekFrame = 0;
			_m_trackReader->SeekToFrame(&seekFrame);
			dest += loaded*frameSize;
			bytes -= loaded*frameSize;
			goto again;
		}
		else {
	stopping:
			_m_stopping = true;
			memset(dest+loaded*frameSize, _m_zero, bytes-loaded*frameSize);
			_m_stopCount = BUFPAGE*PAGECOUNT-(bytes/frameSize-loaded);
			return;
		}
	}
}

status_t BFileGameSound::Perform(int32 selector, void * data)
{
	return BStreamingGameSound::Perform(selector, data);
}

status_t
BFileGameSound::SetPaused(bool paused, bigtime_t ramp)
{
	if (paused) {
		if (IsPaused()) return EALREADY;
		_m_pauseState = B_PAUSE_IN_PROGRESS;
		gs_attribute attr[2];
		attr[0].attribute = B_GS_GAIN;
		attr[1].attribute = B_GS_PAN;
		GetAttributes(attr, 2);
		_m_pauseGain = attr[0].value;
		_m_pausePan = attr[1].value;
		SetGain(0, ramp);
	}
	else {
		if (!IsPaused()) return EALREADY;
		_m_pauseState = B_NOT_PAUSED;
		SetGain(_m_pauseGain, ramp);
		SetPan(_m_pausePan, ramp);
		//	ResumeSound() is not part of public BGameSound API.
		PrivGameSound * pgs = PrivGameSound::CurPlayer();
		if (!pgs) return B_ERROR;
		return pgs->ResumeSound(ID());
	}
	return B_OK;
}

int32
BFileGameSound::IsPaused()
{
	return _m_pauseState;
}

status_t BFileGameSound::_Reserved_BFileGameSound_0(int32 arg, ...)	/* SetPaused() */
{
	va_list l;
	va_start(l, arg);
	bigtime_t ramp = va_arg(l, bigtime_t);
	va_end(l);
	return SetPaused(arg != 0, ramp);
}

status_t BFileGameSound::_Reserved_BFileGameSound_1(int32 arg, ...) { return B_ERROR; }
status_t BFileGameSound::_Reserved_BFileGameSound_2(int32 arg, ...) { return B_ERROR; }
status_t BFileGameSound::_Reserved_BFileGameSound_3(int32 arg, ...) { return B_ERROR; }
status_t BFileGameSound::_Reserved_BFileGameSound_4(int32 arg, ...) { return B_ERROR; }
status_t BFileGameSound::_Reserved_BFileGameSound_5(int32 arg, ...) { return B_ERROR; }
status_t BFileGameSound::_Reserved_BFileGameSound_6(int32 arg, ...) { return B_ERROR; }
status_t BFileGameSound::_Reserved_BFileGameSound_7(int32 arg, ...) { return B_ERROR; }
status_t BFileGameSound::_Reserved_BFileGameSound_8(int32 arg, ...) { return B_ERROR; }
status_t BFileGameSound::_Reserved_BFileGameSound_9(int32 arg, ...) { return B_ERROR; }
status_t BFileGameSound::_Reserved_BFileGameSound_10(int32 arg, ...) { return B_ERROR; }
status_t BFileGameSound::_Reserved_BFileGameSound_11(int32 arg, ...) { return B_ERROR; }
status_t BFileGameSound::_Reserved_BFileGameSound_12(int32 arg, ...) { return B_ERROR; }
status_t BFileGameSound::_Reserved_BFileGameSound_13(int32 arg, ...) { return B_ERROR; }
status_t BFileGameSound::_Reserved_BFileGameSound_14(int32 arg, ...) { return B_ERROR; }
status_t BFileGameSound::_Reserved_BFileGameSound_15(int32 arg, ...) { return B_ERROR; }
status_t BFileGameSound::_Reserved_BFileGameSound_16(int32 arg, ...) { return B_ERROR; }
status_t BFileGameSound::_Reserved_BFileGameSound_17(int32 arg, ...) { return B_ERROR; }
status_t BFileGameSound::_Reserved_BFileGameSound_18(int32 arg, ...) { return B_ERROR; }
status_t BFileGameSound::_Reserved_BFileGameSound_19(int32 arg, ...) { return B_ERROR; }
status_t BFileGameSound::_Reserved_BFileGameSound_20(int32 arg, ...) { return B_ERROR; }
status_t BFileGameSound::_Reserved_BFileGameSound_21(int32 arg, ...) { return B_ERROR; }
status_t BFileGameSound::_Reserved_BFileGameSound_22(int32 arg, ...) { return B_ERROR; }
status_t BFileGameSound::_Reserved_BFileGameSound_23(int32 arg, ...) { return B_ERROR; }

/* that completes our FBC padding for BFileGameSound (BStreamingGameSound, vcount=24, dcount=24) */

