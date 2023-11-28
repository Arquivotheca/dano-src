/*****************************************************************************/
/*
** "GenSynthDropSample.c"
**
**	Generalized Music Synthesis package. Part of SoundMusicSys.
**
**	\xA9 Copyright 1993-1998 Beatnik, Inc, All Rights Reserved.
**	Written by Jim Nitchals and Steve Hales
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
** Modification History:
**
**	1/18/96		Spruced up for C++ extra error checking
**				Changed the macro 'THE_CHECK' to accept a type for typecasting the source pointer
**	3/1/96		Removed extra PV_DoCallBack, and PV_GetWavePitch
**	5/2/96		Changed int to BOOL_FLAG
**	12/30/96	Changed copyright
**	6/20/97		Added new 16 bit drop sample cases
**	8/15/97		(sh) Added stereo sample cases to eliminate the use of PV_ServeInterp2 routines
**	2/3/98		Renamed songBufferLeftMono to songBufferDry
**	2/8/98		Changed BOOL_FLAG to XBOOL
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
**	11/10/98	Removed CLIP macro
*/
/*****************************************************************************/

#include "GenSnd.h"
#include "GenPriv.h"

#if USE_DROP_SAMPLE == TRUE

/*------------------------------------------------------------------------	*/
/*  Cases for Fully amplitude scaled synthesis 					                	*/
/*------------------------------------------------------------------------	*/

void PV_ServeDropSampleFullBuffer (GM_Voice *this_voice)
{
	register INT32 			*dest;
	register LOOPCOUNT		count, inner;
	register UBYTE 			*source;
	register XFIXED			cur_wave, wave_increment;
	register INT32			amplitude, amplitudeAdjust;
	register XFIXED			cur_wave_shift;

	dest = &MusicGlobals->songBufferDry[0];
	amplitude = this_voice->lastAmplitudeL;
	amplitudeAdjust = (this_voice->NoteVolume * this_voice->NoteVolumeEnvelope) >> VOLUME_PRECISION_SCALAR;
	amplitudeAdjust = (amplitudeAdjust - amplitude) / MusicGlobals->Sixteen_Loop;
	source = this_voice->NotePtr;
	cur_wave = this_voice->NoteWave;

	wave_increment = PV_GetWavePitch(this_voice->NotePitch);

	if (this_voice->channels == 1)
	{	// mono instrument
		for (count = MusicGlobals->Sixteen_Loop; count > 0; --count)
		{
			for (inner = 0; inner < 4; inner++)
			{
				dest[0] += (source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitude;
				cur_wave += wave_increment;
				dest[1] += (source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitude;
				cur_wave += wave_increment;
				dest[2] += (source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitude;
				cur_wave += wave_increment;
				dest[3] += (source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitude;
				cur_wave += wave_increment;
				dest += 4;
			}
			amplitude += amplitudeAdjust;
		}
	}
	else
	{	// stereo instrument
		for (count = MusicGlobals->Sixteen_Loop; count > 0; --count)
		{
			for (inner = 0; inner < 4; inner++)
			{
				cur_wave_shift = (cur_wave>>STEP_BIT_RANGE) * 2;
				dest[0] += ((source[cur_wave_shift+0]+source[cur_wave_shift+1]/2) - 0x80) * amplitude;
				cur_wave += wave_increment;

				cur_wave_shift = (cur_wave>>STEP_BIT_RANGE) * 2;
				dest[1] += ((source[cur_wave_shift+0]+source[cur_wave_shift+1]/2) - 0x80) * amplitude;
				cur_wave += wave_increment;

				cur_wave_shift = (cur_wave>>STEP_BIT_RANGE) * 2;
				dest[2] += ((source[cur_wave_shift+0]+source[cur_wave_shift+1]/2) - 0x80) * amplitude;
				cur_wave += wave_increment;

				cur_wave_shift = (cur_wave>>STEP_BIT_RANGE) * 2;
				dest[3] += ((source[cur_wave_shift+0]+source[cur_wave_shift+1]/2) - 0x80) * amplitude;
				cur_wave += wave_increment;

				dest += 4;
			}
			amplitude += amplitudeAdjust;
		}
	}
	this_voice->NoteWave = cur_wave;
	this_voice->lastAmplitudeL = amplitude;
}

void PV_ServeDropSamplePartialBuffer (GM_Voice *this_voice, XBOOL looping)
{
	register INT32 			*dest;
	register LOOPCOUNT		count, inner;
	register UBYTE 			*source;
	register XFIXED			cur_wave, wave_increment, end_wave, wave_adjust;
	register INT32			amplitude, amplitudeAdjust;
	register XFIXED			cur_wave_shift;

	dest = &MusicGlobals->songBufferDry[0];
	amplitude = this_voice->lastAmplitudeL;
	amplitudeAdjust = (this_voice->NoteVolume * this_voice->NoteVolumeEnvelope) >> VOLUME_PRECISION_SCALAR;
	amplitudeAdjust = (amplitudeAdjust - amplitude) / MusicGlobals->Sixteen_Loop;
	source = this_voice->NotePtr;
	cur_wave = this_voice->NoteWave;

	wave_increment = PV_GetWavePitch(this_voice->NotePitch);

	if (looping)
	{
		end_wave = (XFIXED) (this_voice->NoteLoopEnd - this_voice->NotePtr) << STEP_BIT_RANGE;
		wave_adjust = (this_voice->NoteLoopEnd - this_voice->NoteLoopPtr) << STEP_BIT_RANGE;
	}
	else
	{	// Interpolated cases have to stop sooner, hence the - 1 everywhere else.
		end_wave = (XFIXED) (this_voice->NotePtrEnd - this_voice->NotePtr) << STEP_BIT_RANGE;
	}

	if (this_voice->channels == 1)
	{	// mono instrument
		for (count = MusicGlobals->Sixteen_Loop; count > 0; --count)
		{
			for (inner = 0; inner < 16; inner++)
			{
				THE_CHECK(UBYTE *);
				*dest += (source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitude;
				dest++;
				cur_wave += wave_increment;
			}
			amplitude += amplitudeAdjust;
		}
	}
	else
	{	// stereo instrument
		for (count = MusicGlobals->Sixteen_Loop; count > 0; --count)
		{
			for (inner = 0; inner < 16; inner++)
			{
				THE_CHECK(UBYTE *);
				cur_wave_shift = (cur_wave>>STEP_BIT_RANGE) * 2;
				*dest += ((source[cur_wave_shift+0]+source[cur_wave_shift+1]/2) - 0x80) * amplitude;
				dest++;
				cur_wave += wave_increment;
			}
			amplitude += amplitudeAdjust;
		}
	}
	this_voice->NoteWave = cur_wave;
	this_voice->lastAmplitudeL = amplitude;
FINISH:
	return;
}


void PV_ServeDropSampleFullBuffer16 (GM_Voice *this_voice)
{
	register INT32 			*dest;
	register LOOPCOUNT		count, inner;
	register INT16 			*source;
	register XFIXED			cur_wave, wave_increment;
	register INT32			amplitude, amplitudeAdjust;
	register XFIXED			cur_wave_shift;

	dest = &MusicGlobals->songBufferDry[0];
	amplitude = this_voice->lastAmplitudeL;
	amplitudeAdjust = (this_voice->NoteVolume * this_voice->NoteVolumeEnvelope) >> VOLUME_PRECISION_SCALAR;
	amplitudeAdjust = (amplitudeAdjust - amplitude) / MusicGlobals->Sixteen_Loop;
	source = (short *) this_voice->NotePtr;
	cur_wave = this_voice->NoteWave;

	amplitude = amplitude >> 4;
	amplitudeAdjust = amplitudeAdjust >> 4;
	wave_increment = PV_GetWavePitch(this_voice->NotePitch);

	if (this_voice->channels == 1)
	{	// mono instrument
		for (count = MusicGlobals->Sixteen_Loop; count > 0; --count)
		{
			for (inner = 0; inner < 4; inner++)
			{
				*dest++ += (source[cur_wave>>STEP_BIT_RANGE] * amplitude) >> 4;
				cur_wave += wave_increment;
				*dest++ += (source[cur_wave>>STEP_BIT_RANGE] * amplitude) >> 4;
				cur_wave += wave_increment;
				*dest++ += (source[cur_wave>>STEP_BIT_RANGE] * amplitude) >> 4;
				cur_wave += wave_increment;
				*dest++ += (source[cur_wave>>STEP_BIT_RANGE] * amplitude) >> 4;
				cur_wave += wave_increment;
			}
			amplitude += amplitudeAdjust;
		}
	}
	else
	{	// stereo instrument
		for (count = MusicGlobals->Sixteen_Loop; count > 0; --count)
		{
			for (inner = 0; inner < 4; inner++)
			{
				cur_wave_shift = (cur_wave>>STEP_BIT_RANGE) * 2;
				*dest++ += ((source[cur_wave_shift+0]+source[cur_wave_shift+1]/2) * amplitude) >> 4;
				cur_wave += wave_increment;

				cur_wave_shift = (cur_wave>>STEP_BIT_RANGE) * 2;
				*dest++ += ((source[cur_wave_shift+0]+source[cur_wave_shift+1]/2) * amplitude) >> 4;
				cur_wave += wave_increment;

				cur_wave_shift = (cur_wave>>STEP_BIT_RANGE) * 2;
				*dest++ += ((source[cur_wave_shift+0]+source[cur_wave_shift+1]/2) * amplitude) >> 4;
				cur_wave += wave_increment;

				cur_wave_shift = (cur_wave>>STEP_BIT_RANGE) * 2;
				*dest++ += ((source[cur_wave_shift+0]+source[cur_wave_shift+1]/2) * amplitude) >> 4;
				cur_wave += wave_increment;
			}
			amplitude += amplitudeAdjust;
		}
	}
	this_voice->NoteWave = cur_wave;
	this_voice->lastAmplitudeL = amplitude << 4;
}

void PV_ServeDropSamplePartialBuffer16 (GM_Voice *this_voice, XBOOL looping)
{
	register INT32 			*dest;
	register LOOPCOUNT		count, inner;
	register INT16 			*source;
	register XFIXED			cur_wave, wave_increment, end_wave, wave_adjust;
	register INT32			amplitude, amplitudeAdjust;
	register XFIXED			cur_wave_shift;

	dest = &MusicGlobals->songBufferDry[0];
	amplitude = this_voice->lastAmplitudeL;
	amplitudeAdjust = (this_voice->NoteVolume * this_voice->NoteVolumeEnvelope) >> VOLUME_PRECISION_SCALAR;
	amplitudeAdjust = (amplitudeAdjust - amplitude) / MusicGlobals->Sixteen_Loop;
	source = (short *) this_voice->NotePtr;
	cur_wave = this_voice->NoteWave;
	amplitude = amplitude >> 4;
	amplitudeAdjust = amplitudeAdjust >> 4;

	wave_increment = PV_GetWavePitch(this_voice->NotePitch);

	if (looping)
	{
		end_wave = (XFIXED) (this_voice->NoteLoopEnd - this_voice->NotePtr) << STEP_BIT_RANGE;
		wave_adjust = (this_voice->NoteLoopEnd - this_voice->NoteLoopPtr) << STEP_BIT_RANGE;
	}
	else
	{
		// Interpolated cases have to stop sooner, hence the - 1 everywhere else.
		end_wave = (XFIXED) (this_voice->NotePtrEnd - this_voice->NotePtr) << STEP_BIT_RANGE;
	}

	if (this_voice->channels == 1)
	{	// mono instrument
		for (count = MusicGlobals->Sixteen_Loop; count > 0; --count)
		{
			for (inner = 0; inner < 4; inner++)
			{
				THE_CHECK(INT16 *);
				*dest++ += (source[cur_wave>>STEP_BIT_RANGE] * amplitude) >> 4;
				cur_wave += wave_increment;
				THE_CHECK(INT16 *);
				*dest++ += (source[cur_wave>>STEP_BIT_RANGE] * amplitude) >> 4;
				cur_wave += wave_increment;
				THE_CHECK(INT16 *);
				*dest++ += (source[cur_wave>>STEP_BIT_RANGE] * amplitude) >> 4;
				cur_wave += wave_increment;
				THE_CHECK(INT16 *);
				*dest++ += (source[cur_wave>>STEP_BIT_RANGE] * amplitude) >> 4;
				cur_wave += wave_increment;
			}
			amplitude += amplitudeAdjust;
		}
	}
	else
	{	// stereo instrument
		for (count = MusicGlobals->Sixteen_Loop; count > 0; --count)
		{
			for (inner = 0; inner < 4; inner++)
			{
				THE_CHECK(INT16 *);
				cur_wave_shift = (cur_wave>>STEP_BIT_RANGE) * 2;
				*dest++ += ((source[cur_wave_shift+0]+source[cur_wave_shift+1]/2) * amplitude) >> 4;
				cur_wave += wave_increment;

				THE_CHECK(INT16 *);
				cur_wave_shift = (cur_wave>>STEP_BIT_RANGE) * 2;
				*dest++ += ((source[cur_wave_shift+0]+source[cur_wave_shift+1]/2) * amplitude) >> 4;
				cur_wave += wave_increment;

				THE_CHECK(INT16 *);
				cur_wave_shift = (cur_wave>>STEP_BIT_RANGE) * 2;
				*dest++ += ((source[cur_wave_shift+0]+source[cur_wave_shift+1]/2) * amplitude) >> 4;
				cur_wave += wave_increment;

				THE_CHECK(INT16 *);
				cur_wave_shift = (cur_wave>>STEP_BIT_RANGE) * 2;
				*dest++ += ((source[cur_wave_shift+0]+source[cur_wave_shift+1]/2) * amplitude) >> 4;
				cur_wave += wave_increment;
			}
			amplitude += amplitudeAdjust;
		}
	}
	this_voice->NoteWave = cur_wave;
	this_voice->lastAmplitudeL = amplitude << 4;
FINISH:
	return;
}

// STEREO OUTPUT CODE

/*------------------------------------------------------------------------	*/
/*  Cases for Fully amplitude scaled synthesis 					                	*/
/*------------------------------------------------------------------------	*/

void PV_ServeStereoAmpFullBuffer (GM_Voice *this_voice)
{
	register INT32 			*destL;
	register LOOPCOUNT		count, inner;
	register UBYTE 			*source;
	register XFIXED			cur_wave_shift;
	register XFIXED			cur_wave, wave_increment;
	INT32					ampValueL, ampValueR;
	register INT32			amplitudeL;
	register INT32			amplitudeR;
	register INT32			amplitudeLincrement, amplitudeRincrement;

	PV_CalculateStereoVolume(this_voice, &ampValueL, &ampValueR);
	amplitudeL = this_voice->lastAmplitudeL;
	amplitudeR = this_voice->lastAmplitudeR;
	amplitudeLincrement = (ampValueL - amplitudeL) / (MusicGlobals->Sixteen_Loop);
	amplitudeRincrement = (ampValueR - amplitudeR) / (MusicGlobals->Sixteen_Loop);

	destL = &MusicGlobals->songBufferDry[0];
	source = this_voice->NotePtr;
	cur_wave = this_voice->NoteWave;

	wave_increment = PV_GetWavePitch(this_voice->NotePitch);

	if (this_voice->channels == 1)
	{	// mono instrument
		for (count = MusicGlobals->Sixteen_Loop; count > 0; --count)
		{
			for (inner = 0; inner < 4; inner++)
			{
				destL[0] += ((source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitudeL);
				destL[1] += ((source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitudeR);
				cur_wave += wave_increment;
				destL[2] += ((source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitudeL);
				destL[3] += ((source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitudeR);
				cur_wave += wave_increment;
				destL[4] += ((source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitudeL);
				destL[5] += ((source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitudeR);
				cur_wave += wave_increment;
				destL[6] += ((source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitudeL);
				destL[7] += ((source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitudeR);
				destL += 8;
				cur_wave += wave_increment;
			}
			amplitudeL += amplitudeLincrement;
			amplitudeR += amplitudeRincrement;
		}
	}
	else
	{	// stereo 8 bit instrument
		for (count = MusicGlobals->Sixteen_Loop; count > 0; --count)
		{
			for (inner = 0; inner < 4; inner++)
			{
				cur_wave_shift = (cur_wave>>STEP_BIT_RANGE) * 2;
				*destL++ += ((source[cur_wave_shift+0] - 0x80) * amplitudeL);
				*destL++ += ((source[cur_wave_shift+1] - 0x80) * amplitudeR);
				cur_wave += wave_increment;
				cur_wave_shift = (cur_wave>>STEP_BIT_RANGE) * 2;
				*destL++ += ((source[cur_wave_shift+0] - 0x80) * amplitudeL);
				*destL++ += ((source[cur_wave_shift+1] - 0x80) * amplitudeR);
				cur_wave += wave_increment;
				cur_wave_shift = (cur_wave>>STEP_BIT_RANGE) * 2;
				*destL++ += ((source[cur_wave_shift+0] - 0x80) * amplitudeL);
				*destL++ += ((source[cur_wave_shift+1] - 0x80) * amplitudeR);
				cur_wave += wave_increment;
				cur_wave_shift = (cur_wave>>STEP_BIT_RANGE) * 2;
				*destL++ += ((source[cur_wave_shift+0] - 0x80) * amplitudeL);
				*destL++ += ((source[cur_wave_shift+1] - 0x80) * amplitudeR);
				cur_wave += wave_increment;
			}
			amplitudeL += amplitudeLincrement;
			amplitudeR += amplitudeRincrement;
		}
	}
	this_voice->NoteWave = cur_wave;
	this_voice->lastAmplitudeL = ampValueL;
	this_voice->lastAmplitudeR = ampValueR;
}

void PV_ServeStereoAmpPartialBuffer (GM_Voice *this_voice, XBOOL looping)
{
	register INT32 			*destL;
	register LOOPCOUNT		count, inner;
	register UBYTE 			*source;
	register XFIXED			cur_wave_shift;
	register XFIXED			cur_wave, wave_increment, end_wave, wave_adjust;
	INT32					ampValueL, ampValueR;
	register INT32			amplitudeL;
	register INT32			amplitudeR;
	register INT32			amplitudeLincrement, amplitudeRincrement;

	PV_CalculateStereoVolume(this_voice, &ampValueL, &ampValueR);
	amplitudeL = this_voice->lastAmplitudeL;
	amplitudeR = this_voice->lastAmplitudeR;
	amplitudeLincrement = (ampValueL - amplitudeL) / (MusicGlobals->Sixteen_Loop);
	amplitudeRincrement = (ampValueR - amplitudeR) / (MusicGlobals->Sixteen_Loop);

	destL = &MusicGlobals->songBufferDry[0];
	source = this_voice->NotePtr;
	cur_wave = this_voice->NoteWave;

	wave_increment = PV_GetWavePitch(this_voice->NotePitch);
	if (looping)
	{
		end_wave = (XFIXED) (this_voice->NoteLoopEnd - this_voice->NotePtr) << STEP_BIT_RANGE;
		wave_adjust = (this_voice->NoteLoopEnd - this_voice->NoteLoopPtr) << STEP_BIT_RANGE;
	}
	else
	{
		end_wave = (XFIXED) (this_voice->NotePtrEnd - this_voice->NotePtr) << STEP_BIT_RANGE;
	}
	if (this_voice->channels == 1)
	{	// mono instrument
		for (count = MusicGlobals->Sixteen_Loop; count > 0; --count)
		{
			for (inner = 0; inner < 16; inner++)
			{
				THE_CHECK(UBYTE *);
				*destL += ((source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitudeL);
				destL[1] += ((source[cur_wave>>STEP_BIT_RANGE] - 0x80) * amplitudeR);
				destL += 2;
				cur_wave += wave_increment;
			}
			amplitudeL += amplitudeLincrement;
			amplitudeR += amplitudeRincrement;
		}
	}
	else
	{	// stereo 8 bit instrument
		for (count = MusicGlobals->Sixteen_Loop; count > 0; --count)
		{
			for (inner = 0; inner < 16; inner++)
			{
				THE_CHECK(UBYTE *);
				cur_wave_shift = (cur_wave>>STEP_BIT_RANGE) * 2;
				*destL += ((source[cur_wave_shift+0] - 0x80) * amplitudeL) >> 1;
				destL[1] += ((source[cur_wave_shift+1] - 0x80) * amplitudeR) >> 1;
				destL += 2;
				cur_wave += wave_increment;
			}
			amplitudeL += amplitudeLincrement;
			amplitudeR += amplitudeRincrement;
		}
	}
	this_voice->NoteWave = cur_wave;
	this_voice->lastAmplitudeL = ampValueL;
	this_voice->lastAmplitudeR = ampValueR;
FINISH:
	return;
}


#endif	// 	USE_DROP_SAMPLE

// EOF of GenSynthDropSample.c
