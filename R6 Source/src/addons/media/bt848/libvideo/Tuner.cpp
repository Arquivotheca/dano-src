/*
	
	Tuner.cpp
	
	Copyright 1997-8 Be Incorporated, All Rights Reserved.
	
*/

#include <stdio.h>

#include "Tuner.h"
#include "channels.h"

//-----------------------------------------------------------------

BTuner::BTuner(const char *name)
{
	strncpy(fName, name, 32);
	fTunerLocale = B_UNDEFINED_LOCALE;

}

//-----------------------------------------------------------------

BTuner::~BTuner()
{

}

//-----------------------------------------------------------------

char *
BTuner::Name()
{
	return fName;
}

//-----------------------------------------------------------------

status_t	
BTuner::SetTunerLocale(tuner_locale locale)
{
	switch (locale)
	{
		case B_US_NTSC_CABLE_IRC:
		case B_US_NTSC_CABLE_HRC:	
		case B_US_NTSC_AIR:
		case B_JAPAN_NTSC_CABLE:
		case B_JAPAN_NTSC_AIR:
		case B_EUROPE_PAL_CABLE:
		case B_EUROPE_PAL_AIR:
		case B_GREAT_BRITAIN_PAL_CABLE:
		case B_GREAT_BRITAIN_PAL_AIR:
		case B_FRANCE_SECAM_CABLE:
		case B_FRANCE_SECAM_AIR:
		case B_CHINA_PAL_AIR:
		case B_BRAZIL_PAL_AIR:
		case B_AUSTRALIA_PAL_AIR:
		case B_FM_RADIO:
			fTunerLocale = locale;
			return B_NO_ERROR;
		default:
			return B_ERROR;
	}
}

//-----------------------------------------------------------------

tuner_locale	
BTuner::TunerLocale() const
{
	switch (fTunerLocale)
	{
		case B_US_NTSC_CABLE_IRC:
		case B_US_NTSC_CABLE_HRC:	
		case B_US_NTSC_AIR:
		case B_JAPAN_NTSC_CABLE:
		case B_JAPAN_NTSC_AIR:
		case B_EUROPE_PAL_CABLE:
		case B_EUROPE_PAL_AIR:
		case B_GREAT_BRITAIN_PAL_CABLE:
		case B_GREAT_BRITAIN_PAL_AIR:
		case B_FRANCE_SECAM_CABLE:
		case B_FRANCE_SECAM_AIR:
		case B_CHINA_PAL_AIR:
		case B_BRAZIL_PAL_AIR:
		case B_AUSTRALIA_PAL_AIR:
		case B_FM_RADIO:
			return(fTunerLocale);
		default:
			return(B_UNDEFINED_LOCALE);
	}
}

//-----------------------------------------------------------------

uint32
BTuner::NumberChannels() const
{
	switch (fTunerLocale) 
	{
		case B_US_NTSC_AIR:
			return sizeof(USNtscAir)/sizeof(ChannelInfo);
		case B_US_NTSC_CABLE_IRC:
			return sizeof(USNtscCableIRC)/sizeof(ChannelInfo);
		case B_US_NTSC_CABLE_HRC:
			return  sizeof(USNtscCableHRC)/sizeof(ChannelInfo);
		case B_JAPAN_NTSC_AIR:
			return sizeof(JapanNtscAir)/sizeof(ChannelInfo);
		case B_JAPAN_NTSC_CABLE:
			return sizeof(JapanNtscCable)/sizeof(ChannelInfo);
		case B_EUROPE_PAL_AIR:
			return sizeof(EuropePalAir)/sizeof(ChannelInfo);
		case B_EUROPE_PAL_CABLE:
			return sizeof(EuropePalCable)/sizeof(ChannelInfo);
		case B_GREAT_BRITAIN_PAL_AIR:
			return sizeof(GreatBritainPalAir)/sizeof(ChannelInfo);
		case B_GREAT_BRITAIN_PAL_CABLE:
			return sizeof(GreatBritainPalCable)/sizeof(ChannelInfo);
		case B_FRANCE_SECAM_AIR:
			return sizeof(FranceSecamAir)/sizeof(ChannelInfo);
		case B_FRANCE_SECAM_CABLE:
			return sizeof(FranceSecamCable)/sizeof(ChannelInfo);
		case B_CHINA_PAL_AIR:
			return sizeof(ChinaPalAir)/sizeof(ChannelInfo);
		case B_BRAZIL_PAL_AIR:
			return sizeof(BrazilPalAir)/sizeof(ChannelInfo);
		case B_AUSTRALIA_PAL_AIR:
			return sizeof(AustraliaPalAir)/sizeof(ChannelInfo);
		case B_FM_RADIO:
			return sizeof(FmRadio)/sizeof(ChannelInfo);
		default:
			return(0);
	}
}

//-----------------------------------------------------------------

int32
BTuner::IndexForChannelName(char *channel_name) const
{
	uint32 i;
	ChannelInfo	*c;
	
	switch (fTunerLocale) 
	{
		case B_US_NTSC_AIR:
			c = USNtscAir;
			break;
		case B_US_NTSC_CABLE_IRC:
			c = USNtscCableIRC;
			break;
		case B_US_NTSC_CABLE_HRC:
			c = USNtscCableHRC;
			break;
		case B_JAPAN_NTSC_AIR:
			c = JapanNtscAir;
			break;
		case B_JAPAN_NTSC_CABLE:
			c = JapanNtscCable;
			break;
		case B_EUROPE_PAL_AIR:
			c = EuropePalAir;
			break;
		case B_EUROPE_PAL_CABLE:
			c = EuropePalCable;
			break;
		case B_GREAT_BRITAIN_PAL_AIR:
			c = GreatBritainPalAir;
			break;
		case B_GREAT_BRITAIN_PAL_CABLE:
			c = GreatBritainPalCable;
			break;
		case B_FRANCE_SECAM_AIR:
			c = FranceSecamAir;
			break;
		case B_FRANCE_SECAM_CABLE:
			c = FranceSecamCable;
			break;
		case B_CHINA_PAL_AIR:
			c = ChinaPalAir;
			break;
		case B_BRAZIL_PAL_AIR:
			c = BrazilPalAir;
			break;
		case B_AUSTRALIA_PAL_AIR:
			c = EuropePalAir;
			break;
		case B_FM_RADIO:
			c = FmRadio;
			break;
		default:
			return(-1);
	}
	
	for (i = 0; i < NumberChannels(); i++)
		{
			if (strcmp(channel_name, c[i].name) == 0)
				{
					return i;
				}
		}
	return -1;
}

//-----------------------------------------------------------------

char *
BTuner::ChannelNameForIndex(uint32 channel_index) const
{
	if (channel_index < NumberChannels())
	{
		switch (fTunerLocale) 
		{
			case B_US_NTSC_AIR:
				return USNtscAir[channel_index].name;
			case B_US_NTSC_CABLE_IRC:
				return USNtscCableIRC[channel_index].name;
			case B_US_NTSC_CABLE_HRC:
				return  USNtscCableHRC[channel_index].name;
			case B_JAPAN_NTSC_AIR:
				return JapanNtscAir[channel_index].name;
			case B_JAPAN_NTSC_CABLE:
				return JapanNtscCable[channel_index].name;
			case B_EUROPE_PAL_AIR:
				return EuropePalAir[channel_index].name;
			case B_EUROPE_PAL_CABLE:
				return EuropePalCable[channel_index].name;
			case B_GREAT_BRITAIN_PAL_AIR:
				return GreatBritainPalAir[channel_index].name;
			case B_GREAT_BRITAIN_PAL_CABLE:
				return GreatBritainPalCable[channel_index].name;
			case B_FRANCE_SECAM_AIR:
				return FranceSecamAir[channel_index].name;
			case B_FRANCE_SECAM_CABLE:
				return FranceSecamCable[channel_index].name;
			case B_CHINA_PAL_AIR:
				return ChinaPalAir[channel_index].name;
			case B_BRAZIL_PAL_AIR:
				return BrazilPalAir[channel_index].name;
			case B_AUSTRALIA_PAL_AIR:
				return AustraliaPalAir[channel_index].name;
			case B_FM_RADIO:
				return FmRadio[channel_index].name;
			default:
				return("ERR");
		}
	}
	else return ("ERR");
}

//-----------------------------------------------------------------

uint32
BTuner::FrequencyFor(char *channel_name) const
{
	uint32 i;
	
	i = IndexForChannelName(channel_name);
	return FrequencyFor(i);
}

//-----------------------------------------------------------------

uint32
BTuner::FrequencyFor(uint32 channel_index) const
{
	if (channel_index < NumberChannels())
	{
		switch (fTunerLocale) 
		{
			case B_US_NTSC_AIR:
				return(USNtscAir[channel_index].frequency);
			case B_US_NTSC_CABLE_IRC:
				return(USNtscCableIRC[channel_index].frequency);
			case B_US_NTSC_CABLE_HRC:
				return(USNtscCableHRC[channel_index].frequency);
			case B_JAPAN_NTSC_AIR:
				return(JapanNtscAir[channel_index].frequency);
			case B_JAPAN_NTSC_CABLE:
				return(JapanNtscCable[channel_index].frequency);
			case B_EUROPE_PAL_AIR:
				return(EuropePalAir[channel_index].frequency);
			case B_EUROPE_PAL_CABLE:
				return(EuropePalCable[channel_index].frequency);
			case B_GREAT_BRITAIN_PAL_AIR:
				return(GreatBritainPalAir[channel_index].frequency);
			case B_GREAT_BRITAIN_PAL_CABLE:
				return(GreatBritainPalCable[channel_index].frequency);
			case B_FRANCE_SECAM_AIR:
				return(FranceSecamAir[channel_index].frequency);
			case B_FRANCE_SECAM_CABLE:
				return(FranceSecamCable[channel_index].frequency);
			case B_CHINA_PAL_AIR:
				return(ChinaPalAir[channel_index].frequency);
			case B_BRAZIL_PAL_AIR:
				return(BrazilPalAir[channel_index].frequency);
			case B_AUSTRALIA_PAL_AIR:
				return(AustraliaPalAir[channel_index].frequency);
			case B_FM_RADIO:
				return(FmRadio[channel_index].frequency);
			default:
				return(0);
		}
	}
	else return (0);
}

//-----------------------------------------------------------------

bool
BTuner::ValidFrequency(uint32 frequency) const
{
	switch(fTunerLocale)
	{
		case B_FM_RADIO:
			if ( (frequency < 76000000) | (frequency > 108000000) )
					return false;
			else return true;		
		case B_US_NTSC_AIR:
		case B_US_NTSC_CABLE_IRC:
		case B_US_NTSC_CABLE_HRC:
		case B_JAPAN_NTSC_AIR:
		case B_JAPAN_NTSC_CABLE:			
			if ( (frequency < 54000000) | (frequency > 801250000) )
					return false;
			else return true;		
		case B_EUROPE_PAL_AIR:
		case B_EUROPE_PAL_CABLE:			
		case B_GREAT_BRITAIN_PAL_AIR:
		case B_GREAT_BRITAIN_PAL_CABLE:			
		case B_FRANCE_SECAM_AIR:
		case B_FRANCE_SECAM_CABLE:
		case B_CHINA_PAL_AIR:
		case B_BRAZIL_PAL_AIR:
		case B_AUSTRALIA_PAL_AIR:
			if ( (frequency < 45750000) | (frequency > 855250000) )
					return false;
			else return true;
		default:
			return false;
	}
}		

//-----------------------------------------------------------------

status_t
BTuner::Tune(uint32 frequency)
{
	fFrequency = frequency;
	return B_NO_ERROR;
}

//-----------------------------------------------------------------

status_t
BTuner::Tune(char *channel_name)
{
	return Tune(FrequencyFor(channel_name));
}

//-----------------------------------------------------------------

status_t
BTuner::TuneIndex(uint32 channel_index)
{
	if( channel_index < NumberChannels())
		fChannelIndex = channel_index;
	else 
		return B_ERROR;
	return B_NO_ERROR;
}

//-----------------------------------------------------------------

uint32
BTuner::CurrentFrequency() const
{
	return fFrequency;
}

//-----------------------------------------------------------------

uint32
BTuner::CurrentIndex() const
{
	return fChannelIndex;
}

//-----------------------------------------------------------------

uint32
BTuner::NextChannel()
{
	if (CurrentIndex() < (NumberChannels()-1))
		TuneIndex(CurrentIndex() + 1);
	else
		TuneIndex(0);

	return (CurrentIndex());
}

//-----------------------------------------------------------------

uint32
BTuner::PreviousChannel()
{
	if (CurrentIndex() > 0)
		TuneIndex(CurrentIndex() - 1);
	else
		TuneIndex(NumberChannels() - 1);

	return (CurrentIndex());
}

//-----------------------------------------------------------------

uint32
BTuner::ScanUp()
{
	return NextChannel();
}

//-----------------------------------------------------------------

uint32
BTuner::ScanDown()
{
	return PreviousChannel();
}

//-----------------------------------------------------------------

uint32
BTuner::FineTuneUp()
{
	Tune(CurrentFrequency() + 62500);
	return(CurrentFrequency());
}

//-----------------------------------------------------------------

uint32
BTuner::FineTuneDown()
{
	Tune(CurrentFrequency() - 62500);
	return(CurrentFrequency());
}

//-----------------------------------------------------------------

uchar
BTuner::TunerStatus()
{
	return 0;
}

//-----------------------------------------------------------------

bool
BTuner::TunerLocked()
{
	return false;
}

//-----------------------------------------------------------------

bool
BTuner::TVCapable() const
{
	return false;
}

//-----------------------------------------------------------------

bool
BTuner::FMRadioCapable() const
{
	return false;
}

//-----------------------------------------------------------------

bool
BTuner::BTSCStereoCapable() const
{
	return false;
}

//-----------------------------------------------------------------

bool
BTuner::BTSCSAPCapable() const
{
	return false;
}

//-----------------------------------------------------------------

bool
BTuner::BTSCStereoPresent()
{
	return false;
}

//-----------------------------------------------------------------

bool
BTuner::BTSCSAPPresent()
{
	return false;
}

//-----------------------------------------------------------------

void
BTuner::SetBTSCAudioMode(uchar mode)
{
	fBTSCAudioMode = mode;	
}

//-----------------------------------------------------------------

uchar
BTuner::BTSCAudioMode()
{
	return fBTSCAudioMode;
}

//-----------------------------------------------------------------

void
BTuner::_ReservedTuner1()
{

}

//-----------------------------------------------------------------

void
BTuner::_ReservedTuner2()
{

}

//-----------------------------------------------------------------

void
BTuner::_ReservedTuner3()
{

}


