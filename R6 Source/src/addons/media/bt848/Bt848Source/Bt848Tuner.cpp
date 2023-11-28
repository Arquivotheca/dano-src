/*
	
	Bt848Tuner.cpp
	
	Copyright 1998 Be Incorporated, All Rights Reserved.
	
*/

#include  "Bt848Tuner.h"

#define DO_NOTHING(x...)

#if DEBUG
#define PRINTF printf
#else
#define PRINTF DO_NOTHING
#endif

#define TRACE PRINTF

//------------------------------------------------------------------

Bt848Tuner::Bt848Tuner(const char *name, uint32 bt848, bt848_config *config,
						hw_info	*hwinfo, BI2CBus * i2c, BAudioMux *mux):BTuner(name)
{
	fBt848 = bt848;
	fConfig = config;
	fHwInfo = hwinfo;	
	
	fI2C = i2c;
	fAudioMux = mux;
	fFrequency = 0;
	fIndex = 0;
	fBTSCAudioMode = 0;

	fTunerAddress = 0x61;
	if (fI2C->I2CDevicePresent(0x60))
		fTunerAddress = 0x60;
	else 
		if (fI2C->I2CDevicePresent(0x61))
			fTunerAddress = 0x61;
		else
			if (fI2C->I2CDevicePresent(0x62))
				fTunerAddress = 0x62;
			else
				if (fI2C->I2CDevicePresent(0x63))
					fTunerAddress = 0x63;
				else
					TRACE("No tuner detected\n");
					
	TRACE("Tuner I2C address is %02x\n", fTunerAddress);
}

//------------------------------------------------------------------

Bt848Tuner::~Bt848Tuner()
{

}

//------------------------------------------------------------------

status_t
Bt848Tuner::Tune(uint32 frequency)
{
	uint32	lo, midlo, midhi, hi, intermediate_frequency, band, band_info, divisor;

	#define	LOW	0
	#define MID	1
	#define HI	2
	#define	FM	3
	#define NONE 4 
		
	switch (TunerLocale()) 
	{
		case B_FM_RADIO:			
			lo = 64000000;
			hi = 108000000;
			
			if ( (frequency < lo) | (frequency > hi) )
				{
					TRACE("Tune FM: Frequency out of range\n");
					return B_ERROR;
				}
			band = FM;
			intermediate_frequency = 10700000;
			break;
		case B_US_NTSC_AIR:
		case B_US_NTSC_CABLE_IRC:
		case B_US_NTSC_CABLE_HRC:
			switch(fHwInfo->tuner_mfg)
			{
				case BT848HW_PANASONIC:
					lo 		=  54000000;
					midlo	= 164000000;
					midhi	= 314000000;
					hi		= 766000000;
					//intermediate_frequency = 45750000;
					intermediate_frequency = 58750000;
					break;
				case BT848HW_PHILIPS:
					lo 		=  54000000;
					midlo	= 160000000;
					midhi	= 454000000;
					hi		= 801250000;
					intermediate_frequency = 45750000;
					break;
				case BT848HW_TEMIC:
					lo		=  54000000;
					midlo	= 157250000;
					midhi	= 451250000;
					hi		= 801250000;
					intermediate_frequency = 45750000;
					break;
				case BT848HW_ALPS:
					lo		=  54000000;
					midlo	= 130000000;
					midhi	= 365000000;
					hi		= 800000000;
					intermediate_frequency = 45750000;
					break;
				default:
					TRACE("No tuner present");
					return B_ERROR;
			}
			
			if ( (frequency < lo) | (frequency > hi) )
				{
					TRACE("Tuner NTSC: Frequency out of range\n");
					return B_ERROR;
				}
				
			if ( (frequency >= lo) & (frequency < midlo) )
				{
					band = LOW;
				}
			else
				{
					if ( (frequency >= midlo) & (frequency < midhi) )
						{
							band = MID;
						}
					else
						{
							band = HI;
						}
				}
			break;
		case B_JAPAN_NTSC_AIR:
		case B_JAPAN_NTSC_CABLE:			
			switch(fHwInfo->tuner_mfg)
			{
				case BT848HW_PANASONIC:
					lo 		=  54000000;
					midlo	= 164000000;
					midhi	= 314000000;
					hi		= 766000000;
					intermediate_frequency = 58750000;
					break;
				case BT848HW_PHILIPS:
					lo 		=  54000000;
					midlo	= 160000000;
					midhi	= 454000000;
					hi		= 801250000;
					intermediate_frequency = 45750000;
					break;
				case BT848HW_TEMIC:
					lo		=  54000000;
					midlo	= 157250000;
					midhi	= 451250000;
					hi		= 801250000;
					intermediate_frequency = 45750000;
					break;
				case BT848HW_ALPS:
					lo		=  54000000;
					midlo	= 130000000;
					midhi	= 365000000;
					hi		= 800000000;
					intermediate_frequency = 45750000;
					break;
				default:
					TRACE("No tuner present");
					return B_ERROR;
			}
			
			if ( (frequency < lo) | (frequency > hi) )
				{
					TRACE("Tuner NTSC: Frequency out of range\n");
					return B_ERROR;
				}
				
			if ( (frequency >= lo) & (frequency < midlo) )
				{
					band = LOW;
				}
			else
				{
					if ( (frequency >= midlo) & (frequency < midhi) )
						{
							band = MID;
						}
					else
						{
							band = HI;
						}
				}
			break;
		case B_EUROPE_PAL_AIR:
		case B_EUROPE_PAL_CABLE:
		case B_GREAT_BRITAIN_PAL_AIR:
		case B_GREAT_BRITAIN_PAL_CABLE:	
		case B_CHINA_PAL_AIR:
		case B_BRAZIL_PAL_AIR:
		case B_AUSTRALIA_PAL_AIR:		
			switch(fHwInfo->tuner_mfg)
			{
				case BT848HW_PANASONIC:
					lo 		=  54000000;
					midlo	= 164000000;
					midhi	= 314000000;
					hi		= 766000000;
					intermediate_frequency = 38900000;
					break;
				case BT848HW_PHILIPS:
					lo		=  45750000;
					midlo	= 170000000;
					midhi	= 450000000;
					hi		= 855250000;
					intermediate_frequency = 38900000;
					break;
				case BT848HW_TEMIC:
					lo		=  45750000;
					midlo	= 145250000;
					midhi	= 451250000;
					hi		= 855250000;
					intermediate_frequency = 38900000;
					break;
				case BT848HW_ALPS:
					lo		=  54000000;
					midlo	= 130000000;
					midhi	= 365000000;
					hi		= 800000000;
					intermediate_frequency = 38900000;
					break;
				default:
					TRACE("No tuner present\n");
					return B_ERROR;
			}
			
			if ( (frequency < lo) | (frequency > hi) )
				{
					TRACE("Tune PAL: Frequency out of range\n");
					return B_ERROR;
				}
				
			if ( (frequency >= lo) & (frequency < midlo) )
				{
					band = LOW;
				}
			else
				{
					if ( (frequency >= midlo) & (frequency < midhi) )
						{
							band = MID;
						}
					else
						{
							band = HI;
						}
				}
			break;
		case B_FRANCE_SECAM_AIR:
		case B_FRANCE_SECAM_CABLE:
			switch(fHwInfo->tuner_mfg)
			{
				case BT848HW_PANASONIC:
					lo 		=  54000000;
					midlo	= 164000000;
					midhi	= 314000000;
					hi		= 766000000;
					intermediate_frequency = 38900000;
					break;
				case BT848HW_PHILIPS:
					lo		=  45750000;
					midlo	= 170000000;
					midhi	= 450000000;
					hi		= 855250000;
					intermediate_frequency = 38900000;
					break;
				case BT848HW_TEMIC:
					lo		=  45750000;
					midlo	= 145250000;
					midhi	= 451250000;
					hi		= 855250000;
					intermediate_frequency = 38900000;
					break;
				case BT848HW_ALPS:
					lo		=  54000000;
					midlo	= 130000000;
					midhi	= 365000000;
					hi		= 800000000;
					intermediate_frequency = 38900000;
					break;
				default:
					TRACE("No tuner present\n");
					return B_ERROR;
			}
			
			if ( (frequency < lo) | (frequency > hi) )
				{
					TRACE("Tune SECAM: Frequency out of range\n");
					return B_ERROR;
				}
				
			if ( (frequency >= lo) & (frequency < midlo) )
				{
					band = LOW;
				}
			else
				{
					if ( (frequency >= midlo) & (frequency < midhi) )
						{
							band = MID;
						}
					else
						{
							band = HI;
						}
				}
			break;
		default:
			band=NONE;
			intermediate_frequency=0;
			break;
	}
	
	uint32 mul = 1;
	switch (band) 
	{
		case FM:
			switch (fHwInfo->tuner_mfg)
			{
				case BT848HW_PHILIPS:
					band_info = 0x8ea5;
					break;
				case BT848HW_ALPS:
					band_info = 0x8204;
					break;
				default:
					TRACE("Tuner not FM capable\n");
					return B_ERROR;
			}
			break;
		case LOW:
			switch (fHwInfo->tuner_mfg)
			{
				case BT848HW_PANASONIC:
					band_info = 0xca04;
					mul = 2;
					break;
				case BT848HW_PHILIPS:
					if ((TunerLocale() == B_FRANCE_SECAM_AIR) ||
						(TunerLocale() == B_FRANCE_SECAM_CABLE))
						band_info = 0x8ea7;
					else
						band_info = 0x8ea0;					
					break;
				case BT848HW_TEMIC:
					band_info = 0x8e02;
					break;
				case BT848HW_ALPS:
					band_info = 0x8214;
					break;
				default:
					TRACE("No tuner present\n");
					return B_ERROR;
			}
			break;
		case MID:
			switch (fHwInfo->tuner_mfg)
			{
				case BT848HW_PANASONIC:
					band_info = 0xca02;
					mul = 2;
					break;
				case BT848HW_PHILIPS:	
					if ((TunerLocale() == B_FRANCE_SECAM_AIR) ||
						(TunerLocale() == B_FRANCE_SECAM_CABLE))
						band_info = 0x8e97;
					else
						band_info = 0x8e90;
					break;
				case BT848HW_TEMIC:
					band_info = 0x8e04;
					break;
				case BT848HW_ALPS:
					band_info = 0x8212;
					break;
				default:
					TRACE("No tuner present\n");
					return B_ERROR;
			}
			break;
		case HI:
			switch (fHwInfo->tuner_mfg)
			{
				case BT848HW_PANASONIC:
					band_info = 0xca01;
					mul = 2;
					break;
				case BT848HW_PHILIPS:	
					if ((TunerLocale() == B_FRANCE_SECAM_AIR) ||
						(TunerLocale() == B_FRANCE_SECAM_CABLE))
						band_info = 0x8e37;
					else
						band_info = 0x8e30;
					break;
				case BT848HW_TEMIC:
					band_info = 0x8e01;
					break;
				case BT848HW_ALPS:
					band_info = 0x8211;
					break;
				default:
					TRACE("No tuner present\n");
					return B_ERROR;
			}
			break;
		default:
			TRACE("Impossible Band setting\n");
			return B_ERROR;
			break;
	}
	
	divisor = mul * (16 * (frequency/1000 + intermediate_frequency/1000))/1000;

	TRACE("Tuning %d (%d + %d)Hz\n", frequency + intermediate_frequency, frequency, intermediate_frequency);

	TRACE("Tuner commands: c2 %02x %02x %02x %02x\n",
		(divisor>>8) & 0x7f, (divisor & 0xff),
		(band_info >> 8) | 0x40, (band_info & 0xff));
		
	if (frequency > fFrequency)
	{
		fI2C->I2CWrite2(fTunerAddress, (divisor>>8) & 0x7f, (divisor & 0xff));
		snooze(10000);
		fI2C->I2CWrite2(fTunerAddress, (band_info >> 8) | 0x40, (band_info & 0xff));
	}
	else
	{
		fI2C->I2CWrite2(fTunerAddress, (band_info >> 8) | 0x40, (band_info & 0xff));
		snooze(10000);
		fI2C->I2CWrite2(fTunerAddress, (divisor>>8) & 0x7f, (divisor & 0xff));	
	}
	fFrequency = frequency;

	return B_NO_ERROR;
}

//------------------------------------------------------------------

status_t
Bt848Tuner::Tune(char *channel_name)
{
	int i;
	i= IndexForChannelName(channel_name);
	
	if (i >= 0)
	{
		fIndex = i;
		return TuneIndex(i);
	}
	else
	{
		TRACE("Tune Name:  Can't find channel name '%s' in table\n", channel_name);
		return B_ERROR;
	}
}

//------------------------------------------------------------------

status_t
Bt848Tuner::TuneIndex(uint32 channel_index)
{
	uint32	frequency;
	uint32 current_audio;
	
	if (channel_index < NumberChannels())
	{		
		current_audio = fAudioMux->Source();	
		fAudioMux->Mute();
	
		fIndex = channel_index;
	
		if ((frequency = FrequencyFor(channel_index)) != 0)
			Tune(frequency);
		
		snooze(75000);
				
		fAudioMux->SetSource(current_audio);
		return B_NO_ERROR;
	}
	else
	{
		TRACE("Index out of range\n");
		return B_ERROR;
	}
}

//------------------------------------------------------------------

uint32
Bt848Tuner::CurrentFrequency() const
{
	return(fFrequency);
}

//------------------------------------------------------------------

uint32	
Bt848Tuner::CurrentIndex() const
{
	return(fIndex);
}

//------------------------------------------------------------------

uint32
Bt848Tuner::NextChannel()
{
	if (CurrentIndex() < (NumberChannels()-1))
		TuneIndex(CurrentIndex() + 1);
	else
		TuneIndex(0);

	return (CurrentIndex());
}

//------------------------------------------------------------------

uint32
Bt848Tuner::PreviousChannel()
{
	if (CurrentIndex() > 0)
		TuneIndex(CurrentIndex() - 1);
	else
		TuneIndex(NumberChannels() - 1);

	return (CurrentIndex());
}

//------------------------------------------------------------------

uint32
Bt848Tuner::ScanUp()
{
	uint32	count, current_channel;
	uint32 current_audio;

	count = 0;
	current_channel = CurrentIndex();
	current_audio = fAudioMux->Source();	
	fAudioMux->Mute();

	while (count++ < NumberChannels())
	{
		if (CurrentIndex() < (NumberChannels()-1))
			TuneIndex(CurrentIndex() + 1);
		else
			TuneIndex(0);
			
		if (TunerLocked())
		{
			switch(TunerLocale())
			{
				case B_FM_RADIO:
					fAudioMux->SetSource(current_audio);
					return (CurrentIndex());
				default:
					snooze(75000);		
					if (VideoPresent())
						{
							fAudioMux->SetSource(current_audio);
							return (CurrentIndex());
						}
					break;
			}
		}
	}

	TuneIndex(current_channel);	
	fAudioMux->SetSource(current_audio);
	return (CurrentIndex());
}

//------------------------------------------------------------------

uint32
Bt848Tuner::ScanDown()
{
	uint32	count, current_channel;
	uint32 current_audio;

	count = 0;
	current_channel = CurrentIndex();
	current_audio = fAudioMux->Source();	
	fAudioMux->Mute();
	
	while (count++ < NumberChannels())
	{	
		if (CurrentIndex() > 0)
			TuneIndex(CurrentIndex() - 1);
		else
			TuneIndex(NumberChannels() - 1);
			
		snooze(100000);

		if (TunerLocked())
		{
			switch(TunerLocale())
			{
				case B_FM_RADIO:
					fAudioMux->SetSource(current_audio);
					return (CurrentIndex());
				default:
					if (VideoPresent())
						{
							fAudioMux->SetSource(current_audio);
							return (CurrentIndex());
						}
					break;
			}
		}
	}

	TuneIndex(current_channel);	
	fAudioMux->SetSource(current_audio);
	return (CurrentIndex());
}

//------------------------------------------------------------------

uint32
Bt848Tuner::FineTuneUp()
{
	Tune(CurrentFrequency() + 62500);
	return(CurrentFrequency());
}

//------------------------------------------------------------------

uint32
Bt848Tuner::FineTuneDown()
{
	Tune(CurrentFrequency() - 62500);
	return(CurrentFrequency());
}

//------------------------------------------------------------------

uchar
Bt848Tuner::TunerStatus()
{
	return(fI2C->I2CRead(fTunerAddress)& 0xc7);
}

//------------------------------------------------------------------

bool
Bt848Tuner::TunerLocked()
{
	#define TUNER_LOCK 0x40

	if((TunerStatus() & TUNER_LOCK) == TUNER_LOCK)
		return true;
	else
		return false;
}

//------------------------------------------------------------------

bool
Bt848Tuner::VideoPresent()
{
	ioctl(fBt848,BT848_STATUS,fConfig);
	if((fConfig->status & (BT848_VIDEO_PRESENT | BT848_HLOCK)) == (BT848_VIDEO_PRESENT | BT848_HLOCK))
		return true;
	else
		return false;
}


