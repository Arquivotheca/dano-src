/*****************************************************************************/
/*
**	X_Instruments.h
**
**	Tools for creating instruments. The structure enclosed here are used
**	for create expanded editable structures.
**
**	\xA9 Copyright 1996-1999 Beatnik, Inc, All Rights Reserved.
**	Written by Steve Hales
**
**	Beatnik products contain certain trade secrets and confidential and
**	proprietary information of Beatnik.  Use, reproduction, disclosure
**	and distribution by any means are prohibited, except pursuant to
**	a written license from Beatnik. Use of copyright notice is
**	precautionary and does not imply publication or disclosure.
**
**	Restricted Rights Legend:
**	Use, duplication, or disclosure by the Government is subject to
**	restrictions as set forth in subparagraph (c)(1)(ii) of The
**	Rights in Technical Data and Computer Software clause in DFARS
**	252.227-7013 or subparagraphs (c)(1) and (2) of the Commercial
**	Computer Software--Restricted Rights at 48 CFR 52.227-19, as
**	applicable.
**
**	Confidential-- Internal use only
**
**	History	-
**	2/16/98		Created. Pulled from MacOS specific editor codebase
**
**	6/5/98		Jim Nitchals RIP	1/15/62 - 6/5/98
**				I'm going to miss your irreverent humor. Your coding style and desire
**				to make things as fast as possible. Your collaboration behind this entire
**				codebase. Your absolute belief in creating the best possible relationships 
**				from honesty and integrity. Your ability to enjoy conversation. Your business 
**				savvy in understanding the big picture. Your gentleness. Your willingness 
**				to understand someone else's way of thinking. Your debates on the latest 
**				political issues. Your generosity. Your great mimicking of cartoon voices. 
**				Your friendship. - Steve Hales
**
**	11/10/98	Added XNewInstrumentWithBasicEnvelopeResource
**	3/11/99		Renamed CurveRecord to GM_TieTo.
*/
/*****************************************************************************/
#ifndef X_INSTRUMENTS
#define X_INSTRUMENTS

#ifndef __X_API__
	#include "X_API.h"
#endif

#ifndef X_FORMATS
	#include "X_Formats.h"
#endif

#ifndef G_SOUND
	#include "GenSnd.h"
#endif


#ifdef __cplusplus
	extern "C" {
#endif

#define FULL_RANGE				(VOLUME_RANGE * 2)
#define SUSTAIN_DEFAULT_TIME	45000


// Basic envelope structure used for LFO's and ADSR's
typedef struct
{
// Used for display
	long		startScaleH;
	long		endScaleH;

	long		startScaleV;
	long		endScaleV;

	long		startRangeV;
	long		endRangeV;

	long		stageCount;
	long		level[ADSR_STAGES+1];
	long		time[ADSR_STAGES+1];
	long		flags[ADSR_STAGES+1];
} XEnvelopeData;

typedef GM_TieTo XTieToData;

typedef struct
{
	XEnvelopeData	envelopeLFO;

	long			period;
	long			waveShape;
	long			DC_feed;			// amount to use as ADSR
	long			depth;				// amount to use as LFO
} XLFOData;

typedef struct
{
	long			LPF_frequency;
	long			LPF_resonance;
	long			LPF_lowpassAmount;
} XLowPassFilterData;

// Use one of these INST_XXXX for unitType in the structure XUnitData below. When there
// are multiple LFO types use a different unitID for tracking.
/*
	INST_ADSR_ENVELOPE
	INST_EXPONENTIAL_CURVE
	INST_LOW_PASS_FILTER
	INST_DEFAULT_MOD
	INST_PITCH_LFO
	INST_VOLUME_LFO
	INST_STEREO_PAN_LFO
	INST_STEREO_PAN_NAME2
	INST_LOW_PASS_AMOUNT
	INST_LPF_DEPTH
	INST_LPF_FREQUENCY
*/
typedef unsigned long	XUnitType;

typedef struct
{
	XUnitType		unitType;
	unsigned long	unitID;

	union	
	{
		XEnvelopeData		envelopeADSR;
		XLFOData			lfo;
		XLowPassFilterData	lpf;
		XTieToData			curve;
		XBOOL				useDefaultModwheelAction;
	} u;
} XUnitData;

// The XInstrumentData structure is a deconstruction of the InstrumentResource structure
typedef struct
{
// how many units in instrument, only used for reconstruction
	long			unitCount;
// seperate unit information:
	XUnitData		units[256];
}  XInstrumentData;

typedef enum XEnvelopeType
{
	NONE_E			=	0,
	FOUR_POINT_E,
	TWO_POINT_E,
	FLAT_FULL_E
} XEnvelopeType;

InstrumentResource*	XNewInstrumentResource(XShortResourceID leadSndID);
InstrumentResource* XNewInstrumentWithBasicEnvelopeResource(XShortResourceID leadSndID, XEnvelopeType type);
void				XDisposeInstrumentResource(InstrumentResource* theX);

XInstrumentData*	XCreateXInstrument(InstrumentResource* theX,
										unsigned long theXSize);

// pass in original instrument resource (theX) its size (theXSize) and the structure XInstrumentData (pXInstrument)
// to build a new instrument resource. The variable theX is not touched, and you can deallocate it after this
// function is used.
InstrumentResource*	XReconstructInstrument(InstrumentResource* theX,
											unsigned long theXSize,
											XInstrumentData* pXInstrument);

long				XGetTotalEnvelopeTime(XEnvelopeData* pXEnvelope);

void				XEnvelopeAdjustSustainTime(XEnvelopeData* pXEnvelope);

long				XFindType(XInstrumentData* pXInstrument, XUnitType type);
long				XAddType(XInstrumentData* pXInstrument, XUnitType type);
void				XRemoveType(XInstrumentData* pXInstrument,
								XUnitType unitType,
								unsigned long unitID);

XBOOL				XDeleteEnvelopePoint(XEnvelopeData* pEnvelope, short int whichPoint);

// Given an envelope, rebuild the ADSR type to one of XEnvelopeType type
void				XFillDefaultADSREnvelope(XEnvelopeData* pEnvelope, XEnvelopeType type);

// Given an envelope, shift volume plus or minus 
void				XShiftEnvelopeVolume(XEnvelopeData* pEnvelope, long shift);

// add extra 0 time point for editing
void				XAddZeroEnvelope(XEnvelopeData* pEnvelope);

// remove extra 0 time point from editing
void				XRemoveZeroEnvelope(XEnvelopeData* pEnvelope);

void				XAddDefaultADSREnvelope(XInstrumentData* pXInstrument, XEnvelopeType type);


#ifdef __cplusplus
	}
#endif


#endif	// X_INSTRUMENTS
// EOF of X_Instruments.h


