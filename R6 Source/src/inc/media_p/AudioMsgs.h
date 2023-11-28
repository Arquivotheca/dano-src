/* ++++++++++

   FILE:  AudioMsgs.h
   REVS:  $Revision: 1.10 $
   NAME:  r
   DATE:  Mon Jun 05 18:53:05 PDT 1995

   Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _AUDIO_MSGS_H
#define _AUDIO_MSGS_H

#ifndef _BUFFER_MSGS_H
#include <BufferMsgs.h>
#endif

/* Signature of the audio server.  Use this to find or launch the server */
//+#define AUDIO_SERVER_ID	'AUSV'
#define AUDIO_SERVER_ID	"application/x-vnd.Be-AUSV"

/* define additional messages beyond those defined in BufferMsgs.h */
enum {
  GET_SAMPLING_RATE = ERROR_RETURN+1,
  SET_SAMPLING_RATE,

  GET_DAC_SAMPLE_INFO,
  SET_DAC_SAMPLE_INFO,

  GET_ADC_SAMPLE_INFO,
  SET_ADC_SAMPLE_INFO,

  SET_ADC_INPUT,
  GET_ADC_INPUT,

  SET_MIC_BOOST,
  GET_MIC_BOOST,

  SET_SOUND_HARDWARE_INFO,
  GET_SOUND_HARDWARE_INFO,
  SAVE_SOUND_HARDWARE_INFO,
  REVERT_SOUND_HARDWARE_INFO,
  DEFAULT_SOUND_HARDWARE_INFO,

  BEEP
  };

/* Message type */
#define B_STEREO_FACTS_TYPE 'STRO'
#define B_SAMPLE_FACTS_TYPE 'SMPL'
#define B_STEREO_CONSIDER_TYPE 'STEN'

/*  Hardware and sample format structures. */
typedef struct
{
	float left;
	float right;
	bool mute;
} stereo_facts;

typedef struct
{
	bool left;
	bool right;
	bool mute;
} stereo_consider;

typedef struct
{
	int32 encode;
	int32 size;
	int32 channel_count;
	int32 byte_order;
	int32 format;
} sample_facts;


#endif // #ifndef _AUDIO_MSGS_H
