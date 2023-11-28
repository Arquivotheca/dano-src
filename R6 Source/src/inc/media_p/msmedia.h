#ifndef _BASECODE_H_
#define _BASECODE_H_

#include <SupportDefs.h>

/****************************************************************************
 *
 *   basecode.h  - Registered Multimedia Information Public Header File
 *
 *   Copyright (c) 1991-1992 Microsoft Corporation.  All Rights Reserved.
 *
 * Multimedia Registration
 *
 * Place this system include file in your INCLUDE path with the Windows SDK
 * include files.
 *
 * Obtain the Multimedia Developer Registration Kit from:
 *
 *  Microsoft Corporation
 *  Multimedia Systems Group
 *  Product Marketing
 *  One Microsoft Way
 *  Redmond, WA 98052-6399
 *
 *
 * Last Update:  11/12/92
 *
 ***************************************************************************/

/* manufacturer IDs */
enum {
	MM_MICROSOFT		= 1,
	MM_CREATIVE			= 2,
	MM_MEDIAVISION		= 3,
	MM_FUJITSU			= 4,
	MM_ARTISOFT			= 20,
	MM_TURTLE_BEACH		= 21,
	MM_IBM				= 22,
	MM_VOCALTEC			= 23,
	MM_ROLAND			= 24,
	MM_DIGISPEECH		= 25,
	MM_NEC				= 26,
	MM_ATI				= 27,
	MM_WANGLABS			= 28,
	MM_TANDY			= 29,
	MM_VOYETRA			= 30,
	MM_ANTEX			= 31,
	MM_ICL_PS			= 32,
	MM_INTEL			= 33,
	MM_GRAVIS			= 34,
	MM_VAL				= 35,
	MM_INTERACTIVE		= 36,
	MM_YAMAHA			= 37,
	MM_EVEREX			= 38,
	MM_ECHO				= 39,
	MM_SIERRA			= 40
};

/* MM_MICROSOFT product IDs */
enum {
	MM_MIDI_MAPPER      			= 1,   /* MIDI Mapper */
	MM_WAVE_MAPPER      			= 2,   /* Wave Mapper */
	MM_SNDBLST_MIDIOUT  			= 3,   /* Sound Blaster MIDI output port */
	MM_SNDBLST_MIDIIN   			= 4,   /* Sound Blaster MIDI input port */
	MM_SNDBLST_SYNTH    			= 5,   /* Sound Blaster internal synthesizer */
	MM_SNDBLST_WAVEOUT  			= 6,   /* Sound Blaster waveform output */
	MM_SNDBLST_WAVEIN   			= 7,   /* Sound Blaster waveform input */
	MM_ADLIB            			= 9,   /* Ad Lib-compatible synthesizer */
	MM_MPU401_MIDIOUT   			= 10,  /* MPU401-compatible MIDI output port */
	MM_MPU401_MIDIIN    			= 11,  /* MPU401-compatible MIDI input port */
	MM_PC_JOYSTICK      			= 12,  /* Joystick adapter */
	MM_PCSPEAKER_WAVEOUT   			= 13,  /* PC Speaker waveform output */
	MM_MSFT_WSS_WAVEIN				= 14,  /* MS Audio Board waveform input */
	MM_MSFT_WSS_WAVEOUT				= 15,  /* MS Audio Board waveform output */
	MM_MSFT_WSS_FMSYNTH_STEREO		= 16,  /* MS Audio Board Stereo FM synthesizer */
	MM_MSFT_WSS_OEM_WAVEIN			= 18,  /* MS OEM Audio Board waveform input */
	MM_MSFT_WSS_OEM_WAVEOUT			= 19,  /* MS OEM Audio Board waveform Output */
	MM_MSFT_WSS_OEM_FMSYNTH_STEREO	= 20,  /* MS OEM Audio Board Stereo FM synthesizer */
	MM_MSFT_WSS_AUX					= 21,  /* MS Audio Board Auxiliary Port */
	MM_MSFT_WSS_OEM_AUX				= 22   /* MS OEM Audio Auxiliary Port */
};

/* MM_CREATIVE product IDs */
enum {
	MM_CREATIVE_SB15_WAVEIN         = 1,   /* SB (r) 1.5 waveform input */
	MM_CREATIVE_SB20_WAVEIN         = 2,   /* SB (r) 2.0 waveform input */
	MM_CREATIVE_SBPRO_WAVEIN        = 3,   /* SB Pro (r) waveform input */
	MM_CREATIVE_SBP16_WAVEIN        = 4,   /* SBP16 (r) waveform input */
	MM_CREATIVE_SB15_WAVEOUT        = 101, /* SB (r) 1.5 waveform output */
	MM_CREATIVE_SB20_WAVEOUT        = 102, /* SB (r) 2.0 waveform output */
	MM_CREATIVE_SBPRO_WAVEOUT       = 103, /* SB Pro (r) waveform output */
	MM_CREATIVE_SBP16_WAVEOUT       = 104, /* SBP16 (r) waveform output */
	MM_CREATIVE_MIDIOUT             = 201, /* SB (r) MIDI output port */
	MM_CREATIVE_MIDIIN              = 202, /* SB (r) MIDI input port */
	MM_CREATIVE_FMSYNTH_MONO        = 301, /* SB (r) FM synthesizer */
	MM_CREATIVE_FMSYNTH_STEREO      = 302, /* SB Pro (r) stereo FM synthesizer */
	MM_CREATIVE_AUX_CD              = 401, /* SB Pro (r) aux (CD) */
	MM_CREATIVE_AUX_LINE            = 402, /* SB Pro (r) aux (line in) */
	MM_CREATIVE_AUX_MIC             = 403  /* SB Pro (r) aux (mic) */
};


/* MM_ARTISOFT product IDs */
enum {
	MM_ARTISOFT_SBWAVEIN	= 1,  /* Artisoft Sounding Board waveform input */
	MM_ARTISOFT_SBWAVEOUT	= 2   /* Artisoft Sounding Board waveform output */
};

/* MM_IBM Product IDs */
enum {
	MM_MMOTION_WAVEAUX	= 1,	/* IBM M-Motion Auxiliary Device */
	MM_MMOTION_WAVEOUT	= 2,	/* IBM M-Motion Waveform Output */
	MM_MMOTION_WAVEIN	= 3		/* IBM M-Motion Waveform Input */
};

/* MM_MEDIAVISION Product IDs */
enum {
	MM_PROAUD_MIDIOUT		= 21,	/* MediaVision MIDI output port */
	MM_PROAUD_MIDIIN		= 22,	/* MediaVision MIDI input port */
	MM_PROAUD_SYNTH			= 23,	/* MediaVision synthesizer */
	MM_PROAUD_WAVEOUT		= 24,	/* MediaVision Waveform output */
	MM_PROAUD_WAVEIN		= 25,	/* MediaVision Waveform input */
	MM_PROAUD_MIXER			= 26,	/* MediaVision Mixer */
	MM_PROAUD_AUX			= 27,	/* MediaVision aux */
	MM_MEDIAVISION_THUNDER	= 32	/* Thunderboard Sound Card */
};

/* MM_VOCALTEC Product IDs */
enum {
	MM_VOCALTEC_WAVEOUT	= 1,	/* Vocaltec Waveform output port */
	MM_VOCALTEC_WAVEIN	= 2		/* Vocaltec Waveform input port */
};
			

/* MM_DIGISPEECH Product IDs */
enum {
	MM_DIGISP_WAVEOUT	= 1,	/* Digispeech Waveform output port */
	MM_DIGISP_WAVEIN	= 2		/* Digispeech Waveform input port */
};
			
/* MM_WANGLABS Product IDs */
enum {
	MM_WANGLABS_WAVEIN1		= 1,
	/* Input audio wave device present on the CPU board of the following Wang models: Exec 4010, 4030 and 3450; PC 251/25C, PC 461/25S and PC 461/33C */
	MM_WANGLABS_WAVEOUT1	= 2
	/* Output audio wave device present on the CPU board of the Wang models listed above. */
};

/* MM_INTEL Product IDs */
enum {
	MM_INTELOPD_WAVEIN	= 1,	// HID2 WaveAudio Input driver
	MM_INTELOPD_WAVEOUT	= 101,	// HID2 WaveAudio Output driver
	MM_INTELOPD_AUX		= 401	// HID2 Auxiliary driver (required for mixing functions)
};

/* MM_INTERACTIVE Product IDs */
enum {
	MM_INTERACTIVE_WAVEIN	= 0x45,	// no comment provided by Manufacturer
	MM_INTERACTIVE_WAVEOUT	= 0x45	// no comment provided by Manufacturer
};

/* MM_YAMAHA Product IDs */
enum {
	MM_YAMAHA_GSS_SYNTH		= 0x01,	// Yamaha Gold Sound Standard FM sythesis driver
	MM_YAMAHA_GSS_WAVEOUT	= 0x02,	// Yamaha Gold Sound Standard wave output driver
	MM_YAMAHA_GSS_WAVEIN	= 0x03,	// Yamaha Gold Sound Standard wave input driver
	MM_YAMAHA_GSS_MIDIOUT	= 0x04,	// Yamaha Gold Sound Standard midi output driver
	MM_YAMAHA_GSS_MIDIIN	= 0x05,	// Yamaha Gold Sound Standard midi input driver
	MM_YAMAHA_GSS_AUX		= 0x06	// Yamaha Gold Sound Standard auxillary driver for mixer functions
};

/* MM_EVEREX Product IDs */
enum {
	MM_EVEREX_CARRIER		= 0x01	// Everex Carrier SL/25 Notebook
};

/* MM_SIERRA Product IDs */
enum {
	MM_SIERRA_ARIA_MIDIOUT	= 0x14,	// Sierra Aria MIDI output
	MM_SIERRA_ARIA_MIDIIN	= 0x15,	// Sierra Aria MIDI input
	MM_SIERRA_ARIA_SYNTH	= 0x16,	// Sierra Aria Synthesizer
	MM_SIERRA_ARIA_WAVEOUT	= 0x17,	// Sierra Aria Waveform output
	MM_SIERRA_ARIA_WAVEIN	= 0x18,	// Sierra Aria Waveform input
	MM_SIERRA_ARIA_AUX		= 0x19	// Siarra Aria Auxiliary device
};

/*-----------------------------------------------------------------------------*/

/* WAVE form wFormatTag IDs */
enum {
	WAVE_FORMAT_UNKNOWN			= 0x0000,
	WAVE_FORMAT_ADPCM			= 0x0002,
	WAVE_FORMAT_IBM_CVSD		= 0x0005,
	WAVE_FORMAT_ALAW			= 0x0006,
	WAVE_FORMAT_MULAW			= 0x0007,
	WAVE_FORMAT_OKI_ADPCM		= 0x0010,
	WAVE_FORMAT_DVI_ADPCM		= 0x0011,
	WAVE_FORMAT_DIGISTD			= 0x0015,
	WAVE_FORMAT_DIGIFIX			= 0x0016,
	WAVE_FORMAT_YAMAHA_ADPCM	= 0x0020,
	WAVE_FORMAT_CREATIVE_ADPCM	= 0x0200
};

/* general waveform format structure (information common to all formats) */
struct WaveFormat {
    uint16			wFormatTag;        /* format type */
    uint16			nChannels;         /* number of channels (i.e. mono, stereo...) */
    uint32			nSamplesPerSec;    /* sample rate */
    uint32			nAvgBytesPerSec;   /* for buffer estimation */
    uint16			nBlockAlign;       /* block size of data */
};

/* flags for wFormatTag field of WAVEFORMAT */
#define	WAVE_FORMAT_PCM     1

/* specific waveform format structure for PCM data */
struct PCMWaveFormat {
    WaveFormat		wf;
    uint16			wBitsPerSample;
};


/* 
   General extended waveform format structure 
   Use this for all NON PCM formats 
   (information common to all formats)
*/

struct WaveFormatEx {
    uint16			wFormatTag;        /* format type */
    uint16			nChannels;         /* number of channels (i.e. mono, stereo...) */
    uint32			nSamplesPerSec;    /* sample rate */
    uint32			nAvgBytesPerSec;   /* for buffer estimation */
    uint16			nBlockAlign;       /* block size of data */
    uint16			wBitsPerSample;    /* Number of bits per sample of mono data */
    uint16			cbSize;	       	   /* The count in bytes of the size of extra
    									  information (after cbSize) */
};

// Define data for MS ADPCM 
struct ADPCMCoefSet {
	int16			iCoef1;
	int16			iCoef2;
};

struct ADPCMWaveFormat {
	WaveFormatEx	wfx;
	uint16			wSamplesPerBlock;
	uint16			wNumCoef;
	ADPCMCoefSet	aCoef[1];
};

//  Intel's DVI ADPCM structure definitions
struct DVIADPCMWaveFormat{
	WaveFormatEx	wfx;
	uint16			wSamplesPerBlock;
};

//  Creative's ADPCM structure definitions
struct CreativeADPCMWaveFormat {
	WaveFormatEx	wfx;
	uint16			wRevision;
};

// DIB Compression Defines
enum {
	BI_BITFIELDS		= 3
};

enum {
	QUERYDIBSUPPORT 	= 3073,
	QDI_SETDIBITS		= 0x0001,
	QDI_GETDIBITS		= 0x0002,
	QDI_DIBTOSCREEN		= 0x0004,
	QDI_STRETCHDIB		= 0x0008
};

// Defined IC types
enum {
	ICTYPE_VIDEO	= 'vidc',
	ICTYPE_AUDIO	= 'audc'
};

//  constants used by the Microsoft 4 Bit ADPCM algorithm
//
//  CAUTION: the code contained in this file assumes that the number of
//  channels will be no greater than 2! this is for minor optimization
//  purposes and would be very easy to change if >2 channels is required.
//  it also assumes that the PCM data will either be 8 or 16 bit.
//
//  the code to look out for looks 'similar' to this:
//
//      PCM.BytesPerSample = (PCM.BitsPerSample >> 3) << (Channels >> 1);
enum {
	MSADPCM_MAX_COEF		= 14,
	
	MSADPCM_NUM_COEF		= 7,
	MSADPCM_MAX_CHANNELS	= 4,

	MSADPCM_CSCALE			= 8,
	MSADPCM_PSCALE			= 8,
	MSADPCM_CSCALE_NUM		= (1 << MSADPCM_CSCALE),
	MSADPCM_PSCALE_NUM		= (1 << MSADPCM_PSCALE),

	MSADPCM_DELTA4_MIN 		= 16,

	MSADPCM_OUTPUT4_MAX		= 7,
	MSADPCM_OUTPUT4_MIN		= -8
};

#endif
