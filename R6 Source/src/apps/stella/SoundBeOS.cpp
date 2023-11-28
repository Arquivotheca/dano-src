#include "SoundBeOS.h"

#include <stdio.h>
#include <ByteOrder.h>


SoundBeOS::SoundBeOS()
	: 	Sound(),
//		BSoundPlayer(format(), "StellaSoundPlayer"),
		TIASoundBeOS(44100.f/31400.0f)
{
}

SoundBeOS::~SoundBeOS()
{
//	Stop();
}

//const media_raw_audio_format *SoundBeOS::format(void)
//{
//	fFormat.frame_rate = 44100.0f;
//	fFormat.channel_count = 1;
//	fFormat.format = media_raw_audio_format::B_AUDIO_SHORT;
//	fFormat.byte_order = ((B_HOST_IS_LENDIAN) ? (B_MEDIA_LITTLE_ENDIAN) : (B_MEDIA_BIG_ENDIAN));
//	fFormat.buffer_size = 1024;
//	return &fFormat;
//}
//
//void SoundBeOS::PlayBuffer(void *inBuffer, size_t byteCount, const media_raw_audio_format& format)
//{
//	TIASoundBeOS::TIAProcess((int16 *)inBuffer, byteCount/2);
//}

void SoundBeOS::set(Sound::Register reg, uInt8 value)
{
	switch(reg) 
	{
		case Sound::AUDC0:	TIASoundBeOS::UpdateTIASound(0x15, value);	break;
		case Sound::AUDC1:	TIASoundBeOS::UpdateTIASound(0x16, value);	break;
		case Sound::AUDF0:	TIASoundBeOS::UpdateTIASound(0x17, value);	break;
		case Sound::AUDF1:	TIASoundBeOS::UpdateTIASound(0x18, value);	break;
		case Sound::AUDV0:	TIASoundBeOS::UpdateTIASound(0x19, value);	break;
		case Sound::AUDV1:	TIASoundBeOS::UpdateTIASound(0x1A, value);	break;
	}
}

void SoundBeOS::mute(bool state)
{
	if (state == true)
	{
//		Stop(true, false);
	}
	else if (state == false)
	{
//		Start();
//		SetHasData(true);
	}
}

void SoundBeOS::reset(void)
{
	set(Sound::AUDC0, 0);
	set(Sound::AUDC1, 0);
	set(Sound::AUDF0, 0);
	set(Sound::AUDF1, 0);
	set(Sound::AUDV0, 0);
	set(Sound::AUDV1, 0);
}

