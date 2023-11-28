/****************************************************************************
 *	Copyright (C) 1997 E-mu Systems Inc.
 ****************************************************************************
 * @doc INTERNAL 
 * @module sacommon.h | 
 *	This file contains the class definition for the Streaming Audio
 *  Input manager.   
 *
 *  NOTE: This file is included by both C and C++ source files, so please
 *  don't introduce any C++-specific idioms into it.
 *
 * @iex
 *  Revision History
 *  Version	Author	Date		Description
 *  -------	------	----		--------------------------
 *	0.001	JK		Nov 11, 97	Created.
 *
 ****************************************************************************
 * @doc EXTERNAL 
 * @contents E-MU Streaming Audio Input Manager Programmer's Manual |
 *  
 */

#ifndef __SACOMMON_H
#define __SACOMMON_H

#include "datatype.h"
#include "mmdevcmn.h"

/* #define MAXDEVNAMELEN	22 */

/* @enum enSASampleRate |
 *  Enumerates all of the sample rates supported by the streaming audio 
 *  devices.  The sample rates are actually stored as distinct bits so that a user
 *  can query the streaming audio managers to determine which sample rates 
 *  are supported by a particular device.
 */
typedef enum
{
	saRate96K         = 0x0800,  /* @emem 96 KHz sample rate */
	saRate48K         = 0x0400,  /* @emem 48 KHz sample rate */
	saRate44_1K       = 0x0200,  /* @emem 44.1 KHz sample rate */
	saRate32K         = 0x0100,  /* @emem 32 KHz sample rate */
	saRate24K         = 0x0080,  /* @emem 24 KHz sample rate */
	saRate22_05K      = 0x0040,  /* @emem 22.050 KHz sample rate */
	saRate22K         = 0x0020,  /* @emem 22 KHz sample rate */
	saRate16K         = 0x0010,  /* @emem 16 KHz sample rate */
	saRate11_025K     = 0x0008,  /* @emem 11.025 KHz sample rate */
	saRate11K         = 0x0004,  /* @emem 11 KHz sample rate */
	saRate8K          = 0x0002,  /* @emem 8 KHz sample rate */
	saRateRange       = 0x0001   /* @emem The device supports a continuous
								  *  range of sample rates.  When this bit is set,
								  *  the lowest and highest supported sample rates
								  *  are stored in the SADevCaps structure.  */
} enSASampleRate;


/* @enum enSASampleFormat |
 *  Enumerates all of the sample format encodings supported by a particular
 *  device.
 */
typedef enum 
{
	saFormatSigned4ADPCM    = 0x0001, /* @emem Signed two's complement 4-bit ADPCM format */
	saFormatSigned8ADPCM    = 0x0002, /* @emem Signed two's complement 8-bit ADPCM format */
	saFormatSigned8PCM      = 0x0004, /* @emem Signed two's complement 8-bit linear PCM format */
	saFormatUnsigned8PCM    = 0x0008, /* @emem Unsigned 8-bit linear PCM format */
	saFormatSigned16PCM     = 0x0010, /* @emem Signed two's complement 16-bit linear PCM format */
	saFormatUnsigned16PCM   = 0x0020, /* @emem Unsigned 16-bit linear PCM format */
	saFormatSigned24PCM     = 0x0040, /* @emem Signed two's complement 24-bit linear PCM format */
	saFormatUnsigned24PCM   = 0x0080, /* @emem Unsigned 24-bit linear PCM format */
	saFormatSigned32PCM     = 0x0100, /* @emem Signed two's complement linear PCM format */
	saFormatUnsigned32PCM   = 0x0200, /* @emem Unsigned two's complement linear PCM format */
	saFormatFloat32PCM      = 0x0400, /* @emem 32-bit single precision floating point linear PCM */
} enSASampleFormat;


/* @struct stSADevCaps | Describes the capabilities of a
 *  particular audio input device.  
 */
typedef struct
{
	DWORD  sampleRates;      /* @field A bit field of all the sample
							  *  rates supported by the device.  
							  *  If the sarateRange bit is set, the 
							  *  device supports any arbitrary sample
							  *  rate between <f dwLowSampleRate> and
							  *  <f dwHighSampleRate> inclusive.  */
	DWORD  sampleFormats;    /* @field A bit field containing all of
							  *  the sample formats supported by a 
							  *  particular device.  */
	DWORD  dwChannels;       /* @field The number of channels supported
							  *  by the device.  */
	DWORD  dwFlags;			 /* @field Misc. flags. */
	DWORD  dwLowSampleRate;	 /* @field The lowest supported sample rate.
							  *  This field is used mostly when the
							  *  sarateRange flag is set in <f sampleRates> */
	DWORD  dwHighSampleRate; /* @field The highest supported sample rate.
							  *  This field is used mostly when the
							  *  sarateRange flag is set in <f sampleRate>. */
	CHAR   szDevName[MAXMMDEVNAMELEN];  /* @field  The name of the device */
} stSADevCaps;
                                           

/* @struct stSAInputBuffer | Describes the input data buffer and contains
 *  any associated data.
 */
typedef struct stSAInputBufferTag {
	void *        virtAddr;     /* @field The virtual address of the base
								 *  of the allocated buffer to fill.  */
    DWORD         dwSize;       /* @field The size of the buffer in bytes. Note that
                                 *  the underlying buffer routines may actually reduce
                                 *  this value to something which is a multiple of the
                                 *  framesize in order to assure that a sample frame
                                 *  doesn't span a buffer boundary. */
	DWORD		  dwNumFramesRecorded;/* @field The Number of valid sample frames in the
								 *  buffer. This field does _not_ need to
								 *  be set by the client; it will be 
								 *  initialized by the device after the buffer
								 *  has been enqueued.	 */
	DWORD         dwNumFramesDropped; /* @field The number of sample frames dropped.
								 *  Samples will only be dropped if the client
								 *  doesn't add buffers fast enough to 
								 *  satisfy the needs of the device.  The field
								 *  is set by the input device.  */
	DWORD         dwFrameNumber;  /* @field The sample index of the first
								 *  sample in the buffer.  */
    struct stSAInputBufferTag * pNextBuffer;  /* @field The next buffer in the data
								 *  buffer chain.  It should be set to NULL
								 *  if only a single buffer is being passed. */
	DWORD         dwUser1;      /* @field This field is available to the
								 *  client for any purpose.  Its contents
								 *  will remain constant from the time this
								 *  buffer is passed to the HRB until the
								 *  buffer is returned via a callback.  */
	DWORD         dwUser2;      /* @field Available to the client.  */
	DWORD         dwUser3;      /* @field Available to the client.  */
} stSAInputBuffer;


typedef struct stSAOutputBufferTag stSAOutputBuffer;
typedef void (*SAOutputCallback)(stSAOutputBuffer*);

#endif /* __SACOMMON_H */
