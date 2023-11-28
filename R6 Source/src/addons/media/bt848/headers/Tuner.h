/*
	
	Tuner.h
	
	Copyright 1997-8 Be Incorporated, All Rights Reserved.
	
*/

#ifndef _TUNER_H
#define _TUNER_H

#include <string.h>
#include <SupportDefs.h>

#include "VideoDefs.h"

class BTuner
{
public:
							BTuner(const char *name);
virtual						~BTuner();
							
virtual	char *				Name();

virtual	status_t			SetTunerLocale(tuner_locale locale);
virtual	tuner_locale		TunerLocale() const;
		
virtual	uint32				NumberChannels() const;
virtual	int32				IndexForChannelName(char *channel_name) const;
virtual	char * 				ChannelNameForIndex(uint32 channel_index) const;
virtual	uint32				FrequencyFor(char *channel_name) const;
virtual	uint32				FrequencyFor(uint32 channel) const;
virtual	bool				ValidFrequency(uint32 frequency) const;
		
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

virtual	bool				TVCapable() const;
virtual	bool				FMRadioCapable() const;
virtual	bool				BTSCStereoCapable() const;
virtual	bool				BTSCSAPCapable() const;
virtual	bool				BTSCStereoPresent();
virtual	bool				BTSCSAPPresent();
virtual	void				SetBTSCAudioMode(uchar mode);
virtual	uchar				BTSCAudioMode();

private:

virtual	void				_ReservedTuner1();
virtual	void				_ReservedTuner2();
virtual	void				_ReservedTuner3();

							BTuner(const BTuner &);
		BTuner				&operator=(const BTuner &);
		char				fName[32];
		uint32				fFrequency;
		uint32				fChannelIndex;
		tuner_locale		fTunerLocale;
		uint32				fBTSCAudioMode;
		uint32				_reserved[3];

};

#endif


