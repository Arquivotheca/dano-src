
#ifndef _SOUNDBEOS_H_
#define _SOUNDBEOS_H_

//#include <Sound.h>
//#include <SoundPlayer.h>

#include "Sound.hxx"
#include "TIASoundBeOS.h"


class SoundBeOS : public Sound, public TIASoundBeOS //, public BSoundPlayer, 
{
public:
			SoundBeOS();
	virtual ~SoundBeOS();
	virtual void set(Sound::Register reg, uInt8 value);
	virtual void mute(bool state);
	virtual void reset();
//private:
//	virtual	void PlayBuffer(void *buffer, size_t size, const media_raw_audio_format & format);
//	const media_raw_audio_format *format(void);
//	media_raw_audio_format	fFormat;
};



#endif

