/*
	
	Bt848Tuner.h
	
	Copyright 1997-8 Be Incorporated, All Rights Reserved.
	
*/

#ifndef BT848_TUNER_H
#define BT848_TUNER_H

#include <stdio.h>
#include <unistd.h>

#include "Tuner.h"
#include "AudioMux.h"
#include "Bt848Source.h"

class Bt848Tuner : public BTuner
{
public:
							Bt848Tuner(const char *name,
										uint32 bt848,
										bt848_config *config,
										hw_info	*hwinfo,
										BI2CBus *i2c,
										BAudioMux *mux);
virtual						~Bt848Tuner();
							
virtual	status_t			Tune(uint32 frequency);
virtual	status_t			Tune(char *channel_name);
virtual	status_t			TuneIndex(uint32 channel_index);
virtual	uint32				CurrentFrequency() const;
virtual	uint32				CurrentIndex() const;
		
virtual	uint32				NextChannel();
virtual	uint32				PreviousChannel();
virtual	uint32				ScanUp();
virtual	uint32				ScanDown();
virtual	uint32				FineTuneUp();
virtual	uint32				FineTuneDown();
		
virtual	uchar				TunerStatus();
virtual	bool				TunerLocked();

private:

							Bt848Tuner(const Bt848Tuner &);
		Bt848Tuner			&operator=(const Bt848Tuner &);
		bool				VideoPresent();

		bt848_config		*fConfig;
		hw_info				*fHwInfo;	
		uint32				fTunerAddress;
		BAudioMux			*fAudioMux;
		BI2CBus				*fI2C;
		uint32				fBt848;
		uint32				fFrequency;
		uint32				fIndex;
		uint32				fBTSCAudioMode;
};

#endif


