
#include <stdio.h>
#include <stdlib.h>

#include <Entry.h>
#include <SoundFile.h>
#include <MediaFile.h>
#include <MediaTrack.h>

#include "TrackReader.h"

#include "SimpleGameSound.h"
#include "PrivGameSound.h"


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


BSimpleGameSound::BSimpleGameSound(const entry_ref *inFile, BGameSoundDevice *device) :
	BGameSound(device)
{
	if (!inFile) {
		fprintf(stderr, "BSimpleGameSound: inFile is NULL\n");
		SetInitError(B_BAD_VALUE);
		return;
	}
	BMediaFile file(inFile);
	BTrackReader * trk = NULL;
	status_t err = init_check(&file, &trk);
	if (err < B_OK) {
		fprintf(stderr, "BSimpleGameSound: %s damaged or not found\n", inFile->name);
		SetInitError(err);
		return;
	}
	if (trk->FrameSize() < 1) {
		fprintf(stderr, "BSimpleGameSound: %s not supported\n", inFile->name);
		SetInitError(err);
		return;
	}
	gs_audio_format fmt;
	fmt = *(gs_audio_format *)&trk->Format();
	PrivGameSound * pgs = PrivGameSound::MakePlayer(fmt);
	if (pgs == 0) {
		fprintf(stderr, "BSimpleGameSound: cannot create sound player\n");
		SetInitError(B_ERROR);
		return;
	}
	if ((err = pgs->InitCheck()) != B_OK) {
		fprintf(stderr, "BSimpleGameSound: error opening sound player %lx\n", err);
		SetInitError(err);
		return;
	}
	gs_id gsh;
	size_t size = trk->FrameSize()*trk->CountFrames();
	void * data = (void *)malloc(size);
	if (data == NULL) {
		fprintf(stderr, "BSimpleGameSound: cannot allocate %ld bytes for %s\n",
				size, inFile->name);
		SetInitError(B_NO_MEMORY);
		return;
	}
	(void)trk->ReadFrames((char*)data, size/trk->FrameSize());

	file.ReleaseTrack(trk->Track());
	delete trk;

	err = pgs->MakeSound(fmt, data, size, false, &gsh);
	free(data);
	if (err < B_OK) {
		fprintf(stderr, "BSimpleGameSound: cannot make sound handle\n");
		SetInitError(err);
		return;
	}
	(void) Init(gsh);
//	(void) pgs->OptimizeSound(gsh);
}


BSimpleGameSound::BSimpleGameSound(const char *inFile, BGameSoundDevice *device) :
	BGameSound(device)
{
	if (!inFile) {
		fprintf(stderr, "BSimpleGameSound: inFile is NULL\n");
		SetInitError(B_BAD_VALUE);
		return;
	}
	entry_ref ref;
	get_ref_for_path(inFile, &ref);
	BMediaFile file(&ref);
	BTrackReader * trk = NULL;
	status_t err = init_check(&file, &trk);
	if (err < B_OK) {
		fprintf(stderr, "BSimpleGameSound: %s damaged or not found\n", inFile);
		SetInitError(err);
		return;
	}
	if (trk->FrameSize() < 1) {
		fprintf(stderr, "BSimpleGameSound: %s not supported\n", inFile);
		SetInitError(err);
		return;
	}
	gs_audio_format fmt;
	fmt = *(gs_audio_format *)&trk->Format();
	PrivGameSound * pgs = PrivGameSound::MakePlayer(fmt);
	if (pgs == 0) {
		fprintf(stderr, "BSimpleGameSound: cannot create sound player\n");
		SetInitError(B_ERROR);
		return;
	}
	if ((err = pgs->InitCheck()) != B_OK) {
		fprintf(stderr, "BSimpleGameSound: error opening sound player %lx\n", err);
		SetInitError(err);
		return;
	}
	gs_id gsh;
	size_t size = trk->FrameSize()*trk->CountFrames();
	void * data = (void *)malloc(size);
	if (data == NULL) {
		fprintf(stderr, "BSimpleGameSound: cannot allocate %ld bytes for %s\n",
				size, inFile);
		SetInitError(B_NO_MEMORY);
		return;
	}
	(void)trk->ReadFrames((char*)data, size/trk->FrameSize());

	file.ReleaseTrack(trk->Track());
	delete trk;

	err = pgs->MakeSound(fmt, data, size, false, &gsh);
	free(data);
	if (err < B_OK) {
		fprintf(stderr, "BSimpleGameSound: cannot make sound handle\n");
		SetInitError(err);
		return;
	}
	(void) Init(gsh);
//	(void) pgs->OptimizeSound(gsh);
}


BSimpleGameSound::BSimpleGameSound(const void *inData, size_t inFrameCount, const gs_audio_format *format, BGameSoundDevice *device) :
	BGameSound(device)
{
	if (format == 0) {
		fprintf(stderr, "BSimpleGameSound: format is NULL\n");
		SetInitError(B_BAD_VALUE);
		return;
	}
	PrivGameSound * pgs = PrivGameSound::MakePlayer(*format);
	if (pgs == 0) {
		fprintf(stderr, "BSimpleGameSound: cannot create sound player\n");
		SetInitError(B_ERROR);
		return;
	}
	status_t err;
	if ((err = pgs->InitCheck()) != B_OK) {
		fprintf(stderr, "BSimpleGameSound: error opening sound player %lx\n", err);
		SetInitError(err);
		return;
	}
	gs_id gsh;
	size_t size = inFrameCount*frame_size_for(*format);
	err = pgs->MakeSound(*format, inData, size, false, &gsh);
	if (err < B_OK) {
		fprintf(stderr, "BSimpleGameSound: cannot make sound handle\n");
		SetInitError(err);
		return;
	}
	(void) Init(gsh);
//	(void) pgs->OptimizeSound(gsh);
}


BSimpleGameSound::BSimpleGameSound(const BSimpleGameSound &other) :
	BGameSound(other)
{
	gs_id clone = 0;
	PrivGameSound * pgs = PrivGameSound::CurPlayer();
	if (pgs == NULL) {
		fprintf(stderr, "BSimpleGameSound: no current player\n");
		SetInitError(B_ERROR);
		return;
	}
	status_t err = pgs->CloneSound(other.ID(), &clone);
	if (err < B_OK) {
		fprintf(stderr, "BSimpleGameSound: cannot clone from handle %ld: %lx\n",
				other.ID(), err);
		SetInitError(err);
		return;
	}
	(void)Init(clone);
	//	don't need to optimize, because what we're cloning already is
}


BSimpleGameSound::~BSimpleGameSound()
{
	//	do nothing
}

BGameSound *
BSimpleGameSound::Clone() const
{
	return new BSimpleGameSound(*this);
}

status_t BSimpleGameSound::Perform(int32 selector, void * data)
{
	return BGameSound::Perform(selector, data);
}

status_t
BSimpleGameSound::SetIsLooping(bool looping)
{
	gs_attribute attr;
	attr.attribute = B_GS_LOOPING;
	attr.value = (looping ? 1.0 : 0.0);
	attr.flags = 0;
	attr.duration = 0;
	return SetAttributes(&attr, 1);
}

bool
BSimpleGameSound::IsLooping() const
{
	gs_attribute attr;
	attr.attribute = B_GS_LOOPING;
	status_t err = const_cast<BSimpleGameSound *>(this)->GetAttributes(&attr, 1);
	return err ? false : (attr.value > 0.0);
}


status_t BSimpleGameSound::_Reserved_BSimpleGameSound_0(int32 arg, ...) { return B_ERROR; }
status_t BSimpleGameSound::_Reserved_BSimpleGameSound_1(int32 arg, ...) { return B_ERROR; }
status_t BSimpleGameSound::_Reserved_BSimpleGameSound_2(int32 arg, ...) { return B_ERROR; }
status_t BSimpleGameSound::_Reserved_BSimpleGameSound_3(int32 arg, ...) { return B_ERROR; }
status_t BSimpleGameSound::_Reserved_BSimpleGameSound_4(int32 arg, ...) { return B_ERROR; }
status_t BSimpleGameSound::_Reserved_BSimpleGameSound_5(int32 arg, ...) { return B_ERROR; }
status_t BSimpleGameSound::_Reserved_BSimpleGameSound_6(int32 arg, ...) { return B_ERROR; }
status_t BSimpleGameSound::_Reserved_BSimpleGameSound_7(int32 arg, ...) { return B_ERROR; }
status_t BSimpleGameSound::_Reserved_BSimpleGameSound_8(int32 arg, ...) { return B_ERROR; }
status_t BSimpleGameSound::_Reserved_BSimpleGameSound_9(int32 arg, ...) { return B_ERROR; }
status_t BSimpleGameSound::_Reserved_BSimpleGameSound_10(int32 arg, ...) { return B_ERROR; }
status_t BSimpleGameSound::_Reserved_BSimpleGameSound_11(int32 arg, ...) { return B_ERROR; }
status_t BSimpleGameSound::_Reserved_BSimpleGameSound_12(int32 arg, ...) { return B_ERROR; }
status_t BSimpleGameSound::_Reserved_BSimpleGameSound_13(int32 arg, ...) { return B_ERROR; }
status_t BSimpleGameSound::_Reserved_BSimpleGameSound_14(int32 arg, ...) { return B_ERROR; }
status_t BSimpleGameSound::_Reserved_BSimpleGameSound_15(int32 arg, ...) { return B_ERROR; }
status_t BSimpleGameSound::_Reserved_BSimpleGameSound_16(int32 arg, ...) { return B_ERROR; }
status_t BSimpleGameSound::_Reserved_BSimpleGameSound_17(int32 arg, ...) { return B_ERROR; }
status_t BSimpleGameSound::_Reserved_BSimpleGameSound_18(int32 arg, ...) { return B_ERROR; }
status_t BSimpleGameSound::_Reserved_BSimpleGameSound_19(int32 arg, ...) { return B_ERROR; }
status_t BSimpleGameSound::_Reserved_BSimpleGameSound_20(int32 arg, ...) { return B_ERROR; }
status_t BSimpleGameSound::_Reserved_BSimpleGameSound_21(int32 arg, ...) { return B_ERROR; }
status_t BSimpleGameSound::_Reserved_BSimpleGameSound_22(int32 arg, ...) { return B_ERROR; }
status_t BSimpleGameSound::_Reserved_BSimpleGameSound_23(int32 arg, ...) { return B_ERROR; }

/* that completes our FBC padding for BSimpleGameSound (BGameSound, vcount=24, dcount=24) */


