/*
	
	AudioMux.h
	
	Copyright 1997-8 Be Incorporated, All Rights Reserved.
	
*/

#ifndef _AUDIO_MUX_H
#define _AUDIO_MUX_H

#include "VideoDefs.h"

#include <SupportDefs.h>

class BAudioMux
{
public:
							BAudioMux(const char *name);
virtual						~BAudioMux();
							
virtual char *				Name() const;
virtual uint32				NumberInputs() const;
virtual	status_t			SetSource(const uint32 source);
virtual	uint32				Source() const;
virtual void				SetMute(const uint32 source);
virtual void				Mute();
virtual void				Unmute();
virtual	void				SetShift(const uint32 shift);

private:

virtual	void				_ReservedAudioMux1();
virtual	void				_ReservedAudioMux2();
virtual	void				_ReservedAudioMux3();

							BAudioMux(const BAudioMux &);
		BAudioMux			&operator=(const BAudioMux &);

		uint32				fSource;
		uint32				fMute;
		uint32				fLastSource;
		char 				fName[32];
		uint32				_reserved[3];

};

#endif


