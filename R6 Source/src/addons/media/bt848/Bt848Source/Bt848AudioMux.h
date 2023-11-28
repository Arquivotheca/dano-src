/*
	
	Bt848AudioMux.h
	
	Copyright 1998 Be Incorporated, All Rights Reserved.
	
*/

#ifndef BT848_AUDIO_MUX_H
#define BT848_AUDIO_MUX_H

#include <unistd.h>
#include <bt848_driver.h>

#include "AudioMux.h"
#include "Bt848Source.h"

class Bt848AudioMux : public BAudioMux
{
public:

							Bt848AudioMux(	const char *name,
											uint32 bt848,
											bt848_config *config);
							~Bt848AudioMux();
							
		uint32				NumberInputs() const;
		status_t			SetSource(const uint32 source);
		void				SetMute(const uint32);
		void				Mute();
		void				Unmute();
		void				SetShift(const uint32);

private:

							Bt848AudioMux(const Bt848AudioMux &);
		Bt848AudioMux		&operator=(const Bt848AudioMux &);

		uint32 				fBt848;
		uint32				fMute;
		uint32				fLastSource;
		bt848_config 		*fConfig;
		uint32				fMask;
		uint32				fShift;	
};

#endif


