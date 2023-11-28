//
// AudioWrapperDataSource.cpp
//
//   See AudioWrapperDataSource.h
//
//   by Nathan Schrenk (nschrenk@be.com)
//

#include "AudioWrapperDataSource.h"
#include "SupportDefs.h"	// for min_c()

AudioWrapperDataSource::AudioWrapperDataSource(CDDataSource *realSource)
{
	Init(realSource);
}


AudioWrapperDataSource::AudioWrapperDataSource(BMessage *archive)
{
	// no error checking done here, unfortunately
	archive->FindInt64("startoffset", (int64 *)&fStartOffset);
	archive->FindInt64("endoffset", (int64 *)&fEndOffset);
	archive->FindInt32("fadein", (int32 *)&fFadeInLength);
	archive->FindInt32("fadeout", (int32 *)&fFadeOutLength);
	archive->FindFloat("gain", &fGain);
	BMessage msg;
	// Better hope this is B_OK....
	fSource = NULL;
	if(archive->FindMessage("subdatasource", &msg) == B_OK) {
		const char *pp = B_EMPTY_STRING;
		if(archive->FindString("project_path", &pp) == B_OK) {
			msg.ReplaceString("project_path", pp);
			fSource = (CDDataSource *)instantiate_object(&msg);
		}
	}
	fGainEnabled = true;
}

void AudioWrapperDataSource::Init(CDDataSource *source)
{
	if (source && source->IsAudio()) {
		fSource = source;
		fStartOffset = 0;
		fEndOffset = source->Length();
		fFadeInLength = 0;
		fFadeOutLength = 0;
		fGain = 1.0f;
		fGainEnabled = true;
	} else {
		fSource = NULL;
	}
}

AudioWrapperDataSource::~AudioWrapperDataSource()
{
	delete fSource;
}

status_t AudioWrapperDataSource::InitCheck()
{
	status_t ret = B_ERROR;
	if (fSource && fSource->InitCheck() == B_OK && fSource->IsAudio()) {
		ret = B_OK;
	}
	return ret;
}

status_t AudioWrapperDataSource::Read(void *data, size_t len, off_t posn)
{
	status_t ret = B_ERROR;
	off_t realPosn = posn + fStartOffset;
	len = min_c(len, Length() - posn); // disallow running off the end
	if (len > 0) {
		ret = fSource->Read(data, len, realPosn);
	}
	
	if (ret == B_OK) {
		size_t fadeLen;
		float fadeIn, fadeOut;
		// apply any needed fade in
		if (posn < fFadeInLength) { // fade in needed
			fadeLen = min_c(fFadeInLength - posn, len);
			fadeIn = ((float)posn / (float)fFadeInLength);
			fadeOut = ((float)(posn + fadeLen) / (float)fFadeInLength);
			ApplyGain((int16*)data, fadeLen, fadeIn, fadeOut);
		}

		uint32 end = Length() - 1;
		uint32 startFade = end - fFadeOutLength;	
		// apply any needed fade out
		if (posn + len > startFade) { // fade out needed
			fadeLen = min_c((posn + len) - startFade, len);
			fadeIn = ((float)(fFadeOutLength - ((posn + len - fadeLen) - startFade))) / (float)fFadeOutLength;
			fadeOut = ((float)(end - (posn + len))) / (float)fFadeOutLength;
			ApplyGain((int16*)(((char*)data) + (len - fadeLen)), fadeLen, fadeIn, fadeOut);
		}
		
		// apply any needed gain
		if (fGainEnabled && fGain != 1.0f) {
			ApplyGain((int16 *)data, len, fGain, fGain);
		}
	}
		
	return ret;
}

// applies a linear gain to some 16 bit samples in a buffer.  This can be used
// for applying fade in, fade out, and gain.
void AudioWrapperDataSource::ApplyGain(	int16 *data,
										size_t len,
										float beginGain,
										float endGain)
{
	float gainDiff = endGain - beginGain;
	len /= sizeof(int16); // change len to number of samples
	float sample;
	for (uint32 i = 0; i < len; i++) {
#if B_HOST_IS_BENDIAN
		B_SWAP_INT16(data[i]);
#endif
		sample = (float)data[i];
		sample *= (beginGain + (((float)i / (float)len) * gainDiff));
		if (sample > 32767.0f) sample = 32767.0f;
		if (sample < -32767.0f) sample = -32767.0f;
		data[i] = (int16)sample;
#if B_HOST_IS_BENDIAN
		B_SWAP_INT16(data[i]);
#endif
	}
}

size_t AudioWrapperDataSource::Length(void)
{
	return fEndOffset - fStartOffset;
}

bool AudioWrapperDataSource::IsAudio()
{
	return true;
}

char *AudioWrapperDataSource::Description()
{
	return fSource->Description();
}

BArchivable *AudioWrapperDataSource::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "AudioWrapperDataSource")) {
		return new AudioWrapperDataSource(archive);
	} else {
		return NULL;
	}
}

status_t AudioWrapperDataSource::Archive(BMessage *archive, bool deep) const
{
	archive->AddString("class", "AudioWrapperDataSource");
	archive->AddInt64("startoffset", fStartOffset);
	archive->AddInt64("endoffset", fEndOffset);
	archive->AddInt32("fadein", fFadeInLength);
	archive->AddInt32("fadeout", fFadeOutLength);
	archive->AddFloat("gain", fGain);
	if (deep) {
		BMessage subArchive;
		// Add project path... slight hack, should be cleaned up...
		// We got here from BurnerProject::SaveToMessage()
		subArchive.AddString("project_path", archive->FindString("project_path"));
		
		fSource->Archive(&subArchive, true);
		archive->AddMessage("subdatasource", &subArchive);
	}
	return B_OK;
}

void AudioWrapperDataSource::SetGain(float gain)
{
	// sanity check the gain value... allow only values between 0 and 2
	if (gain < 0.0f) {
		gain = 0.0f;
	} else if (gain > 2.0f) {
		gain = 2.0f;
	}
	fGain = gain;
}

float AudioWrapperDataSource::Gain()
{
	return fGain;
}

void AudioWrapperDataSource::SetGainEnabled(bool enabled)
{
	fGainEnabled = enabled;
}

bool AudioWrapperDataSource::GainEnabled()
{
	return fGainEnabled;
}

void AudioWrapperDataSource::SetStart(uint64 offset)
{
	// note: doesn't enforce start < end
	offset -= offset % 4;	// offset must be on frame boundary
	if (offset >= fSource->Length()) {
		offset = fSource->Length() - 1;
	}
	fStartOffset = offset;
}

void AudioWrapperDataSource::SetEnd(uint64 offset)
{
	// note: doesn't enforce start < end
	offset -= offset % 4; // offset must be on frame boundary
	if (offset > fSource->Length()) {
		offset = fSource->Length();
	} 
	fEndOffset = offset;
}

void AudioWrapperDataSource::SetFadeIn(uint32 length)
{
	// note: does not prevent fade in/fade out overlap
	length -= length % 4; // must be on frame boundary
	if (length > Length()) {
		length = Length();
	}
	fFadeInLength = length;
}

void AudioWrapperDataSource::SetFadeOut(uint32 length)
{
	// note: does not prevent fade in/fade out overlap
	length -= length % 4; // must be on frame boundary
	if (length > Length()) {
		length = Length();
	}
	fFadeOutLength = length;
}

uint64 AudioWrapperDataSource::GetStart()
{
	return fStartOffset;
}

uint64 AudioWrapperDataSource::GetEnd()
{
	return fEndOffset;
}

uint32 AudioWrapperDataSource::GetFadeIn()
{
	return fFadeInLength;
}

uint32 AudioWrapperDataSource::GetFadeOut()
{
	return fFadeOutLength;
}

CDDataSource *AudioWrapperDataSource::Source()
{
	return fSource;
}
