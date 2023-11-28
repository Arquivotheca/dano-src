/*****************************************************************************/
/*
** "GenSynthInterp2Float.c"
**
**	Generalized Music Synthesis package. Part of SoundMusicSys.
**	Confidential-- Internal use only
**
**	\xA9 Copyright 1993-1999 Beatnik, Inc, All Rights Reserved.
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
**	3/12/99		Created from GenSynthInterp2Float.c
*/
/*****************************************************************************/

#include "GenSnd.h"
#include "GenPriv.h"

#if USE_FLOAT_LOOPS != FALSE

#undef KATMAI
#if (X_PLATFORM == X_WIN95) && USE_KAT == 1

	// KATMAI support
	#include "xmmintrin.h"
	#define KATMAI			1
	//#define USE_KATMAI_WRITE_CACHE
#endif

void PV_ServeFloatFullBuffer (GM_Voice *this_voice)
{
	register INT32 			*dest;
	register LOOPCOUNT		a, inner;
	register UBYTE 			*source, *calculated_source;
	register INT32			b, c;
	register UFLOAT			cur_wave;
	register INT32			cur_wave_whole;
	register UFLOAT 		wave_increment, wave_increment_x;
	register INT32			amplitude, amplitudeAdjust;

#if (USE_SMALL_MEMORY_REVERB == FALSE) && (USE_VARIABLE_REVERB == TRUE)
	if (this_voice->reverbLevel || this_voice->chorusLevel)
	{
		PV_ServeFloatFullBufferNewReverb (this_voice); 
		return;
	}
#endif
	amplitude = this_voice->lastAmplitudeL;
	amplitudeAdjust = (this_voice->NoteVolume * this_voice->NoteVolumeEnvelope) >> VOLUME_PRECISION_SCALAR;
	amplitudeAdjust = (amplitudeAdjust - amplitude) / MusicGlobals->Four_Loop;
	dest = &MusicGlobals->songBufferDry[0];
	source = this_voice->NotePtr;
	cur_wave = this_voice->samplePosition_f;

	wave_increment = PV_GetWavePitchFloat(this_voice->NotePitch);
	wave_increment_x = wave_increment * 3.0;
	#ifdef KATMAI
	if (MusicGlobals->useKatmaiCPU)
	{
		UFLOAT					cur_wave_next_frame;

		cur_wave_next_frame = wave_increment * (UFLOAT)MusicGlobals->Four_Loop;
		_mm_prefetch((char *)&source[(INT32)cur_wave], _MM_HINT_NTA);
		#ifdef USE_KATMAI_WRITE_CACHE
		_mm_prefetch((char *)dest, _MM_HINT_NTA);
		#endif

		if (this_voice->channels == 1)
		{
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[(INT32)(cur_wave + cur_wave_next_frame)], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)dest, _MM_HINT_NTA);
				#endif

				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				*dest += ((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80) * amplitude;
				cur_wave += wave_increment;

				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				dest[1] += ((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80) * amplitude;
				cur_wave += wave_increment;

				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				dest[2] += ((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80) * amplitude;
				cur_wave += wave_increment;

				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				dest[3] += ((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80) * amplitude;
				cur_wave += wave_increment;
				dest += 4;
				amplitude += amplitudeAdjust;
			}
		}
		else
		{	// stereo 8 bit instrument
			for (a = MusicGlobals->Sixteen_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[(INT32)(cur_wave + cur_wave_next_frame)], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)dest, _MM_HINT_NTA);
				#endif

				for (inner = 0; inner < 16; inner++)
				{
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + (cur_wave_whole * 2);
					b = calculated_source[0] + calculated_source[1];	// average left & right channels
					c = calculated_source[2] + calculated_source[3];
					*dest += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x100) * amplitude) >> 1;
					dest++;
					cur_wave += wave_increment;
				}
				amplitude += amplitudeAdjust;
			}
		}
	}
	else
	#endif
	{
		if (this_voice->channels == 1)
		{
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				*dest += ((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80) * amplitude;
				cur_wave += wave_increment;

				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				dest[1] += ((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80) * amplitude;
				cur_wave += wave_increment;

				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				dest[2] += ((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80) * amplitude;
				cur_wave += wave_increment;

				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				dest[3] += ((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80) * amplitude;
				cur_wave += wave_increment;
				dest += 4;
				amplitude += amplitudeAdjust;
			}
		}
		else
		{	// stereo 8 bit instrument
			for (a = MusicGlobals->Sixteen_Loop; a > 0; --a)
			{
				for (inner = 0; inner < 16; inner++)
				{
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + (cur_wave_whole * 2);
					b = calculated_source[0] + calculated_source[1];	// average left & right channels
					c = calculated_source[2] + calculated_source[3];
					*dest += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x100) * amplitude) >> 1;
					dest++;
					cur_wave += wave_increment;
				}
				amplitude += amplitudeAdjust;
			}
		}
	}
	this_voice->samplePosition_f = cur_wave;
	this_voice->lastAmplitudeL = amplitude;
}

void PV_ServeFloatPartialBuffer (GM_Voice *this_voice, XBOOL looping)
{
	register INT32 			*dest;
	register LOOPCOUNT		a, inner;
	register UBYTE 			*source, *calculated_source;
	register INT32			b, c;
	register UFLOAT			cur_wave;
	register INT32			cur_wave_whole;
	register UFLOAT 		wave_increment, wave_increment_x;
	register UFLOAT 		end_wave, wave_adjust;
	register INT32			amplitude, amplitudeAdjust;

#if (USE_SMALL_MEMORY_REVERB == FALSE) && (USE_VARIABLE_REVERB == TRUE)
	if (this_voice->reverbLevel || this_voice->chorusLevel)
	{
		PV_ServeFloatPartialBufferNewReverb (this_voice, looping);
		return;
	}
#endif
	amplitude = this_voice->lastAmplitudeL;
	amplitudeAdjust = (this_voice->NoteVolume * this_voice->NoteVolumeEnvelope) >> VOLUME_PRECISION_SCALAR;
	amplitudeAdjust = (amplitudeAdjust - amplitude) / MusicGlobals->Four_Loop;
	dest = &MusicGlobals->songBufferDry[0];
	source = this_voice->NotePtr;
	cur_wave = this_voice->samplePosition_f;

	wave_increment = PV_GetWavePitchFloat(this_voice->NotePitch);
	wave_increment_x = wave_increment * 3.0;
	if (looping)
	{
		end_wave = (UFLOAT) (this_voice->NoteLoopEnd - this_voice->NotePtr);
		wave_adjust = (UFLOAT) (this_voice->NoteLoopEnd - this_voice->NoteLoopPtr);
	}
	else
	{
		end_wave = (UFLOAT) (this_voice->NotePtrEnd - this_voice->NotePtr - 1);
	}

	#ifdef KATMAI
	if (MusicGlobals->useKatmaiCPU)
	{
		UFLOAT					cur_wave_next_frame;

		cur_wave_next_frame = wave_increment * (UFLOAT)MusicGlobals->Four_Loop;
		_mm_prefetch((char *)&source[(INT32)cur_wave], _MM_HINT_NTA);
		#ifdef USE_KATMAI_WRITE_CACHE
		_mm_prefetch((char *)dest, _MM_HINT_NTA);
		#endif

		if (this_voice->channels == 1)
		{
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[(INT32)(cur_wave + cur_wave_next_frame)], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)dest, _MM_HINT_NTA);
				#endif

				for (inner = 0; inner < 4; inner++)
				{
					THE_CHECK_FLOAT(UBYTE *);
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					*dest += ((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80) * amplitude;
					dest++;
					cur_wave += wave_increment;
				}
				amplitude += amplitudeAdjust;
			}
		}
		else
		{	// stereo 8 bit instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[(INT32)(cur_wave + cur_wave_next_frame)], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)dest, _MM_HINT_NTA);
				#endif

				for (inner = 0; inner < 4; inner++)
				{
					THE_CHECK_FLOAT(UBYTE *);
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + (cur_wave_whole * 2);
					b = calculated_source[0] + calculated_source[1];
					c = calculated_source[2] + calculated_source[3];
					*dest += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x100) * amplitude) >> 1;
					dest++;
					cur_wave += wave_increment;
				}
				amplitude += amplitudeAdjust;
			}
		}
	}
	else
	#endif
	{
		if (this_voice->channels == 1)
		{
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				for (inner = 0; inner < 4; inner++)
				{
					THE_CHECK_FLOAT(UBYTE *);
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					*dest += ((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80) * amplitude;
					dest++;
					cur_wave += wave_increment;
				}
				amplitude += amplitudeAdjust;
			}
		}
		else
		{	// stereo 8 bit instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				for (inner = 0; inner < 4; inner++)
				{
					THE_CHECK_FLOAT(UBYTE *);
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + (cur_wave_whole * 2);
					b = calculated_source[0] + calculated_source[1];
					c = calculated_source[2] + calculated_source[3];
					*dest += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x100) * amplitude) >> 1;
					dest++;
					cur_wave += wave_increment;
				}
				amplitude += amplitudeAdjust;
			}
		}
	}

	this_voice->samplePosition_f = cur_wave;
	this_voice->lastAmplitudeL = amplitude;
FINISH:
	return;
}

void PV_ServeStereoFloatFullBuffer(GM_Voice *this_voice)
{
	register INT32 			*destL;
	register LOOPCOUNT		a, inner;
	register UBYTE 			*source, *calculated_source;
	register INT32			b, c;
	register UFLOAT			cur_wave;
	register INT32			cur_wave_whole;
	register INT32			sample;
	register UFLOAT 		wave_increment, wave_increment_x;
	INT32					ampValueL, ampValueR;
	register INT32			amplitudeL;
	register INT32			amplitudeR;
	register INT32			amplitudeLincrement;
	register INT32			amplitudeRincrement;

#if (USE_SMALL_MEMORY_REVERB == FALSE) && (USE_VARIABLE_REVERB == TRUE)
	if (this_voice->reverbLevel || this_voice->chorusLevel)
	{
		PV_ServeStereoFloatFullBufferNewReverb (this_voice);
		return;
	}
#endif
	PV_CalculateStereoVolume(this_voice, &ampValueL, &ampValueR);
	amplitudeL = this_voice->lastAmplitudeL;
	amplitudeR = this_voice->lastAmplitudeR;
	amplitudeLincrement = (ampValueL - amplitudeL) / (MusicGlobals->Four_Loop);
	amplitudeRincrement = (ampValueR - amplitudeR) / (MusicGlobals->Four_Loop);

	destL = &MusicGlobals->songBufferDry[0];
	source = this_voice->NotePtr;
	cur_wave = this_voice->samplePosition_f;

	wave_increment = PV_GetWavePitchFloat(this_voice->NotePitch);
	wave_increment_x = wave_increment * 3.0;
	#ifdef KATMAI
	if (MusicGlobals->useKatmaiCPU)
	{
		UFLOAT					cur_wave_next_frame;

		cur_wave_next_frame = wave_increment * (UFLOAT)MusicGlobals->Four_Loop;
		_mm_prefetch((char *)&source[(INT32)cur_wave], _MM_HINT_NTA);
		#ifdef USE_KATMAI_WRITE_CACHE
		_mm_prefetch((char *)destL, _MM_HINT_NTA);
		#endif

		if (this_voice->channels == 1)
		{	// mono instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[(INT32)(cur_wave + cur_wave_next_frame)], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)destL, _MM_HINT_NTA);
				#endif

				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80;
				destL[0] += sample * amplitudeL;
				destL[1] += sample * amplitudeR;
				cur_wave += wave_increment;

				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80;
				destL[2] += sample * amplitudeL;
				destL[3] += sample * amplitudeR;
				cur_wave += wave_increment;

				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80;
				destL[4] += sample * amplitudeL;
				destL[5] += sample * amplitudeR;
				cur_wave += wave_increment;

				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80;
				destL[6] += sample * amplitudeL;
				destL[7] += sample * amplitudeR;
				destL += 8;
				cur_wave += wave_increment;

				amplitudeL += amplitudeLincrement;
				amplitudeR += amplitudeRincrement;
			}
		}
		else
		{	// stereo 8 bit instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[(INT32)(cur_wave + cur_wave_next_frame)], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)destL, _MM_HINT_NTA);
				#endif

				for (inner = 0; inner < 4; inner++)
				{
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + (cur_wave_whole * 2);
					b = calculated_source[0];
					c = calculated_source[2];
					*destL += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80) * amplitudeL);
					b = calculated_source[1];
					c = calculated_source[3];
					destL[1] += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80) * amplitudeR);
					destL += 2;
			
					cur_wave += wave_increment;
				}
				amplitudeL += amplitudeLincrement;
				amplitudeR += amplitudeRincrement;
			}
		}
	}
	else
	#endif
	{
		if (this_voice->channels == 1)
		{	// mono instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80;
				destL[0] += sample * amplitudeL;
				destL[1] += sample * amplitudeR;
				cur_wave += wave_increment;

				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80;
				destL[2] += sample * amplitudeL;
				destL[3] += sample * amplitudeR;
				cur_wave += wave_increment;

				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80;
				destL[4] += sample * amplitudeL;
				destL[5] += sample * amplitudeR;
				cur_wave += wave_increment;

				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80;
				destL[6] += sample * amplitudeL;
				destL[7] += sample * amplitudeR;
				destL += 8;
				cur_wave += wave_increment;

				amplitudeL += amplitudeLincrement;
				amplitudeR += amplitudeRincrement;
			}
		}
		else
		{	// stereo 8 bit instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				for (inner = 0; inner < 4; inner++)
				{
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + (cur_wave_whole * 2);
					b = calculated_source[0];
					c = calculated_source[2];
					*destL += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80) * amplitudeL);
					b = calculated_source[1];
					c = calculated_source[3];
					destL[1] += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80) * amplitudeR);
					destL += 2;
			
					cur_wave += wave_increment;
				}
				amplitudeL += amplitudeLincrement;
				amplitudeR += amplitudeRincrement;
			}
		}
	}
	this_voice->lastAmplitudeL = amplitudeL;
	this_voice->lastAmplitudeR = amplitudeR;
	this_voice->samplePosition_f = cur_wave;
}

void PV_ServeStereoFloatPartialBuffer (GM_Voice *this_voice, XBOOL looping)
{
	register INT32 			*destL;
	register LOOPCOUNT		a, inner;
	register UBYTE 			*source, *calculated_source;
	register INT32			b, c, sample;
	register UFLOAT			cur_wave;
	register INT32			cur_wave_whole;
	register UFLOAT 		wave_increment, wave_increment_x;
	register UFLOAT 		end_wave, wave_adjust;
	INT32					ampValueL, ampValueR;
	register INT32			amplitudeL;
	register INT32			amplitudeR;
	register INT32			amplitudeLincrement, amplitudeRincrement;

#if (USE_SMALL_MEMORY_REVERB == FALSE) && (USE_VARIABLE_REVERB == TRUE)
	if (this_voice->reverbLevel || this_voice->chorusLevel)
	{
		PV_ServeStereoFloatPartialBufferNewReverb (this_voice, looping); 
		return;
	}
#endif
	PV_CalculateStereoVolume(this_voice, &ampValueL, &ampValueR);
	amplitudeL = this_voice->lastAmplitudeL;
	amplitudeR = this_voice->lastAmplitudeR;
	amplitudeLincrement = (ampValueL - amplitudeL) / (MusicGlobals->Four_Loop);
	amplitudeRincrement = (ampValueR - amplitudeR) / (MusicGlobals->Four_Loop);

	destL = &MusicGlobals->songBufferDry[0];
	source = this_voice->NotePtr;
	cur_wave = this_voice->samplePosition_f;

	wave_increment = PV_GetWavePitchFloat(this_voice->NotePitch);
	wave_increment_x = wave_increment * 3.0;
	if (looping)
	{
		end_wave = (UFLOAT) (this_voice->NoteLoopEnd - this_voice->NotePtr);
		wave_adjust = (this_voice->NoteLoopEnd - this_voice->NoteLoopPtr);
	}
	else
	{
		end_wave = (UFLOAT) (this_voice->NotePtrEnd - this_voice->NotePtr - 1);
	}

	#ifdef KATMAI
	if (MusicGlobals->useKatmaiCPU)
	{
		UFLOAT					cur_wave_next_frame;

		cur_wave_next_frame = wave_increment * (UFLOAT)MusicGlobals->Four_Loop;
		_mm_prefetch((char *)&source[(INT32)cur_wave], _MM_HINT_NTA);
		#ifdef USE_KATMAI_WRITE_CACHE
		_mm_prefetch((char *)destL, _MM_HINT_NTA);
		#endif

		if (this_voice->channels == 1)
		{	// mono instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[(INT32)(cur_wave + cur_wave_next_frame)], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)destL, _MM_HINT_NTA);
				#endif

				if ((cur_wave + wave_increment_x) >= end_wave)
				{
					THE_CHECK_FLOAT(UBYTE *);
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80;
					destL[0] += sample * amplitudeL;
					destL[1] += sample * amplitudeR;
					cur_wave += wave_increment;
					THE_CHECK_FLOAT(UBYTE *);
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80;
					destL[2] += sample * amplitudeL;
					destL[3] += sample * amplitudeR;
					cur_wave += wave_increment;
					THE_CHECK_FLOAT(UBYTE *);
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80;
					destL[4] += sample * amplitudeL;
					destL[5] += sample * amplitudeR;
					cur_wave += wave_increment;
					THE_CHECK_FLOAT(UBYTE *);
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80;
					destL[6] += sample * amplitudeL;
					destL[7] += sample * amplitudeR;
					cur_wave += wave_increment;
				}
				else
				{
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80;
					destL[0] += sample * amplitudeL;
					destL[1] += sample * amplitudeR;
					cur_wave += wave_increment;

					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80;
					destL[2] += sample * amplitudeL;
					destL[3] += sample * amplitudeR;
					cur_wave += wave_increment;

					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80;
					destL[4] += sample * amplitudeL;
					destL[5] += sample * amplitudeR;
					cur_wave += wave_increment;

					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80;
					destL[6] += sample * amplitudeL;
					destL[7] += sample * amplitudeR;
					cur_wave += wave_increment;
				}
				destL += 8;
				amplitudeL += amplitudeLincrement;
				amplitudeR += amplitudeRincrement;
			}
		}
		else
		{	// Stereo 8 bit instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[(INT32)(cur_wave + cur_wave_next_frame)], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)destL, _MM_HINT_NTA);
				#endif

				for (inner = 0; inner < 4; inner++)
				{
					THE_CHECK_FLOAT(UBYTE *);
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + (cur_wave_whole * 2);
					b = calculated_source[0];
					c = calculated_source[2];
					*destL += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80) * amplitudeL);
					b = calculated_source[1];
					c = calculated_source[3];
					destL[1] += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80) * amplitudeR);
					destL += 2;
					cur_wave += wave_increment;
				}
				amplitudeL += amplitudeLincrement;
				amplitudeR += amplitudeRincrement;
			}
		}
	}
	else
	#endif
	{
		if (this_voice->channels == 1)
		{	// mono instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				if ((cur_wave + wave_increment_x) >= end_wave)
				{
					THE_CHECK_FLOAT(UBYTE *);

					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80;
					destL[0] += sample * amplitudeL;
					destL[1] += sample * amplitudeR;
					cur_wave += wave_increment;
					THE_CHECK_FLOAT(UBYTE *);

					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80;
					destL[2] += sample * amplitudeL;
					destL[3] += sample * amplitudeR;
					cur_wave += wave_increment;
					THE_CHECK_FLOAT(UBYTE *);

					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80;
					destL[4] += sample * amplitudeL;
					destL[5] += sample * amplitudeR;
					cur_wave += wave_increment;
					THE_CHECK_FLOAT(UBYTE *);

					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80;
					destL[6] += sample * amplitudeL;
					destL[7] += sample * amplitudeR;
					cur_wave += wave_increment;
				}
				else
				{
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80;
					destL[0] += sample * amplitudeL;
					destL[1] += sample * amplitudeR;
					cur_wave += wave_increment;

					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80;
					destL[2] += sample * amplitudeL;
					destL[3] += sample * amplitudeR;
					cur_wave += wave_increment;

					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80;
					destL[4] += sample * amplitudeL;
					destL[5] += sample * amplitudeR;
					cur_wave += wave_increment;

					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80;
					destL[6] += sample * amplitudeL;
					destL[7] += sample * amplitudeR;
					cur_wave += wave_increment;
				}
				destL += 8;
				amplitudeL += amplitudeLincrement;
				amplitudeR += amplitudeRincrement;
			}
		}
		else
		{	// Stereo 8 bit instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				for (inner = 0; inner < 4; inner++)
				{
					THE_CHECK_FLOAT(UBYTE *);
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + (cur_wave_whole * 2);
					b = calculated_source[0];
					c = calculated_source[2];
					*destL += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80) * amplitudeL);
					b = calculated_source[1];
					c = calculated_source[3];
					destL[1] += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b - 0x80) * amplitudeR);
					destL += 2;
					cur_wave += wave_increment;
				}
				amplitudeL += amplitudeLincrement;
				amplitudeR += amplitudeRincrement;
			}
		}
	}
	this_voice->samplePosition_f = cur_wave;
	this_voice->lastAmplitudeL = amplitudeL;
	this_voice->lastAmplitudeR = amplitudeR;
FINISH:
	return;
}

// \xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5
// \xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5
// \xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5
// \xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5
// \xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5

// 16 bit cases
void PV_ServeFloatFullBuffer16 (GM_Voice *this_voice)
{
	register INT32 			*dest;
	register LOOPCOUNT		a, inner;
	register INT16 			*source, *calculated_source;
	register INT32			b, c, sample;
	register UFLOAT			cur_wave;
	register INT32			cur_wave_whole;
	register UFLOAT 		wave_increment, wave_increment_x;
	register INT32			amplitude, amplitudeAdjust;

#if (USE_SMALL_MEMORY_REVERB == FALSE) && (USE_VARIABLE_REVERB == TRUE)
	if (this_voice->reverbLevel || this_voice->chorusLevel)
	{
		PV_ServeFloatFullBuffer16NewReverb (this_voice); 
		return;
	}
#endif
	amplitude = this_voice->lastAmplitudeL;
	amplitudeAdjust = (this_voice->NoteVolume * this_voice->NoteVolumeEnvelope) >> VOLUME_PRECISION_SCALAR;
	amplitudeAdjust = (amplitudeAdjust - amplitude) / MusicGlobals->Four_Loop >> 4;
	amplitude = amplitude >> 4;

	dest = &MusicGlobals->songBufferDry[0];
	source = (short *) this_voice->NotePtr;
	cur_wave = this_voice->samplePosition_f;

	wave_increment = PV_GetWavePitchFloat(this_voice->NotePitch);
	wave_increment_x = wave_increment * 3.0;
	#ifdef KATMAI
	if (MusicGlobals->useKatmaiCPU)
	{
		UFLOAT					cur_wave_next_frame;

		cur_wave_next_frame = wave_increment * (UFLOAT)MusicGlobals->Four_Loop;
		_mm_prefetch((char *)&source[(INT32)cur_wave], _MM_HINT_NTA);
		#ifdef USE_KATMAI_WRITE_CACHE
		_mm_prefetch((char *)dest, _MM_HINT_NTA);
		#endif

		if (this_voice->channels == 1)
		{
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[(INT32)(cur_wave + cur_wave_next_frame)], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)dest, _MM_HINT_NTA);
				#endif

				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				*dest += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b) * amplitude) >> 4;

				cur_wave += wave_increment;
				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				dest[1] += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b) * amplitude) >> 4;

				cur_wave += wave_increment;
				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				dest[2] += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b) * amplitude) >> 4;

				cur_wave += wave_increment;
				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				dest[3] += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b) * amplitude) >> 4;
				dest += 4;
				cur_wave += wave_increment;
				amplitude += amplitudeAdjust;
			}
		}
		else
		{	// stereo 16 bit instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[(INT32)(cur_wave + cur_wave_next_frame)], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)dest, _MM_HINT_NTA);
				#endif

				for (inner = 0; inner < 4; inner++)
				{
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + (cur_wave_whole * 2);
					b = calculated_source[0] + calculated_source[1];
					c = calculated_source[2] + calculated_source[3];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
					*dest += (sample  * amplitude) >> 5;	// divide extra for summed stereo channels
					dest++;

					cur_wave += wave_increment;
				}
				amplitude += amplitudeAdjust;
			}
		}
	}
	else
	#endif
	{
		if (this_voice->channels == 1)
		{
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				*dest += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b) * amplitude) >> 4;

				cur_wave += wave_increment;
				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				dest[1] += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b) * amplitude) >> 4;

				cur_wave += wave_increment;
				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				dest[2] += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b) * amplitude) >> 4;

				cur_wave += wave_increment;
				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				dest[3] += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b) * amplitude) >> 4;
				dest += 4;
				cur_wave += wave_increment;
				amplitude += amplitudeAdjust;
			}
		}
		else
		{	// stereo 16 bit instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				for (inner = 0; inner < 4; inner++)
				{
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + (cur_wave_whole * 2);
					b = calculated_source[0] + calculated_source[1];
					c = calculated_source[2] + calculated_source[3];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
					*dest += (sample  * amplitude) >> 5;	// divide extra for summed stereo channels
					dest++;

					cur_wave += wave_increment;
				}
				amplitude += amplitudeAdjust;
			}
		}
	}
	this_voice->samplePosition_f = cur_wave;
	this_voice->lastAmplitudeL = amplitude << 4;
}

void PV_ServeFloatPartialBuffer16 (GM_Voice *this_voice, XBOOL looping)
{
	register INT32 			*dest;
	register LOOPCOUNT		a, inner;
	register INT16 			*source, *calculated_source;
	register INT32			b, c, sample;
	register UFLOAT			cur_wave;
	register INT32			cur_wave_whole;
	register UFLOAT 		wave_increment, wave_increment_x;
	register UFLOAT 		end_wave, wave_adjust;
	register INT32			amplitude, amplitudeAdjust;

#if (USE_SMALL_MEMORY_REVERB == FALSE) && (USE_VARIABLE_REVERB == TRUE)
	if (this_voice->reverbLevel || this_voice->chorusLevel)
	{
		PV_ServeFloatPartialBuffer16NewReverb (this_voice, looping); 
		return;
	}
#endif
	amplitude = this_voice->lastAmplitudeL;
	amplitudeAdjust = (this_voice->NoteVolume * this_voice->NoteVolumeEnvelope) >> VOLUME_PRECISION_SCALAR;
	amplitudeAdjust = (amplitudeAdjust - amplitude) / MusicGlobals->Four_Loop >> 4;
	amplitude = amplitude >> 4;

	dest = &MusicGlobals->songBufferDry[0];
	cur_wave = this_voice->samplePosition_f;
	source = (short *) this_voice->NotePtr;

	wave_increment = PV_GetWavePitchFloat(this_voice->NotePitch);
	wave_increment_x = wave_increment * 3.0;
	if (looping)
	{
		end_wave = (UFLOAT) (this_voice->NoteLoopEnd - this_voice->NotePtr);
		wave_adjust = (UFLOAT) (this_voice->NoteLoopEnd - this_voice->NoteLoopPtr);
	}
	else
	{
		end_wave = (UFLOAT) (this_voice->NotePtrEnd - this_voice->NotePtr - 1);
	}

	#ifdef KATMAI
	if (MusicGlobals->useKatmaiCPU)
	{
		UFLOAT					cur_wave_next_frame;

		cur_wave_next_frame = wave_increment * (UFLOAT)MusicGlobals->Four_Loop;
		_mm_prefetch((char *)&source[(INT32)cur_wave], _MM_HINT_NTA);
		#ifdef USE_KATMAI_WRITE_CACHE
		_mm_prefetch((char *)dest, _MM_HINT_NTA);
		#endif

		if (this_voice->channels == 1)
		{
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[(INT32)(cur_wave + cur_wave_next_frame)], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)dest, _MM_HINT_NTA);
				#endif

				if ((cur_wave + wave_increment_x) >= end_wave)
				{
					THE_CHECK_FLOAT(INT16 *);
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					*dest += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b) * amplitude) >> 4;
					cur_wave += wave_increment;
					THE_CHECK_FLOAT(INT16 *);
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					dest[1] += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b) * amplitude) >> 4;
					cur_wave += wave_increment;
					THE_CHECK_FLOAT(INT16 *);
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					dest[2] += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b) * amplitude) >> 4;
					cur_wave += wave_increment;
					THE_CHECK_FLOAT(INT16 *);
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					dest[3] += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b) * amplitude) >> 4;
				}
				else
				{
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					*dest += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b) * amplitude) >> 4;
					cur_wave += wave_increment;

					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					dest[1] += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b) * amplitude) >> 4;
					cur_wave += wave_increment;

					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					dest[2] += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b) * amplitude) >> 4;
					cur_wave += wave_increment;

					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					dest[3] += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b) * amplitude) >> 4;
				}
				dest += 4;
				cur_wave += wave_increment;
				amplitude += amplitudeAdjust;
			}
		}
		else
		{
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[(INT32)(cur_wave + cur_wave_next_frame)], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)dest, _MM_HINT_NTA);
				#endif

				for (inner = 0; inner < 4; inner++)
				{
					THE_CHECK_FLOAT(INT16 *);
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + (cur_wave_whole * 2);
					b = calculated_source[0] + calculated_source[1];
					c = calculated_source[2] + calculated_source[3];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
					*dest += ((sample >> 1) * amplitude) >> 5;
					dest++;
					cur_wave += wave_increment;
				}
				amplitude += amplitudeAdjust;
			}
		}
	}
	else
	#endif
	{
		if (this_voice->channels == 1)
		{
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				if ((cur_wave + wave_increment_x) >= end_wave)
				{
					THE_CHECK_FLOAT(INT16 *);
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					*dest += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b) * amplitude) >> 4;
					cur_wave += wave_increment;
					THE_CHECK_FLOAT(INT16 *);
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					dest[1] += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b) * amplitude) >> 4;
					cur_wave += wave_increment;
					THE_CHECK_FLOAT(INT16 *);
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					dest[2] += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b) * amplitude) >> 4;
					cur_wave += wave_increment;
					THE_CHECK_FLOAT(INT16 *);
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					dest[3] += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b) * amplitude) >> 4;
				}
				else
				{
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					*dest += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b) * amplitude) >> 4;
					cur_wave += wave_increment;

					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					dest[1] += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b) * amplitude) >> 4;
					cur_wave += wave_increment;

					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					dest[2] += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b) * amplitude) >> 4;
					cur_wave += wave_increment;

					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					dest[3] += (((((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b) * amplitude) >> 4;
				}
				dest += 4;
				cur_wave += wave_increment;
				amplitude += amplitudeAdjust;
			}
		}
		else
		{
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				for (inner = 0; inner < 4; inner++)
				{
					THE_CHECK_FLOAT(INT16 *);
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + (cur_wave_whole * 2);
					b = calculated_source[0] + calculated_source[1];
					c = calculated_source[2] + calculated_source[3];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
					*dest += ((sample >> 1) * amplitude) >> 5;
					dest++;
					cur_wave += wave_increment;
				}
				amplitude += amplitudeAdjust;
			}
		}
	}
	this_voice->samplePosition_f = cur_wave;
	this_voice->lastAmplitudeL = amplitude << 4;
FINISH:
	return;
}

void PV_ServeStereoFloatFullBuffer16 (GM_Voice *this_voice)
{
	register INT32 			*destL;
	register LOOPCOUNT		a, inner;
	register INT16 			*source, *calculated_source;
	register INT32			b, c;
	register UFLOAT			cur_wave;
	register INT32			cur_wave_whole;
	register INT32			sample;
	register UFLOAT 		wave_increment;
	INT32					ampValueL, ampValueR;
	register INT32			amplitudeL;
	register INT32			amplitudeR;
	register INT32			amplitudeLincrement;
	register INT32			amplitudeRincrement;

#if (USE_SMALL_MEMORY_REVERB == FALSE) && (USE_VARIABLE_REVERB == TRUE)
	if (this_voice->reverbLevel || this_voice->chorusLevel)
	{ 
		PV_ServeStereoFloatFullBuffer16NewReverb (this_voice);
		return;
	}
#endif
	PV_CalculateStereoVolume(this_voice, &ampValueL, &ampValueR);
	amplitudeL = this_voice->lastAmplitudeL;
	amplitudeR = this_voice->lastAmplitudeR;
	amplitudeLincrement = (ampValueL - amplitudeL) / (MusicGlobals->Four_Loop);
	amplitudeRincrement = (ampValueR - amplitudeR) / (MusicGlobals->Four_Loop);

	amplitudeL = amplitudeL >> 4;
	amplitudeR = amplitudeR >> 4;
	amplitudeLincrement = amplitudeLincrement >> 4;
	amplitudeRincrement = amplitudeRincrement >> 4;

	destL = &MusicGlobals->songBufferDry[0];
	cur_wave = this_voice->samplePosition_f;

	source = (short *) this_voice->NotePtr;

	wave_increment = PV_GetWavePitchFloat(this_voice->NotePitch);
	#ifdef KATMAI
	if (MusicGlobals->useKatmaiCPU)
	{
		UFLOAT					cur_wave_next_frame;

		cur_wave_next_frame = wave_increment * (UFLOAT)MusicGlobals->Four_Loop;
		_mm_prefetch((char *)&source[(INT32)cur_wave], _MM_HINT_NTA);
		#ifdef USE_KATMAI_WRITE_CACHE
		_mm_prefetch((char *)destL, _MM_HINT_NTA);
		#endif

		if (this_voice->channels == 1)
		{	// mono instrument

			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[(INT32)(cur_wave + cur_wave_next_frame)], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)destL, _MM_HINT_NTA);
				#endif

				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
				destL[0] += (sample * amplitudeL) >> 4;
				destL[1] += (sample * amplitudeR) >> 4;
				cur_wave += wave_increment;

				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
				destL[2] += (sample * amplitudeL) >> 4;
				destL[3] += (sample * amplitudeR) >> 4;
				cur_wave += wave_increment;

				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
				destL[4] += (sample * amplitudeL) >> 4;
				destL[5] += (sample * amplitudeR) >> 4;
				cur_wave += wave_increment;

				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
				destL[6] += (sample * amplitudeL) >> 4;
				destL[7] += (sample * amplitudeR) >> 4;
				destL += 8;
				cur_wave += wave_increment;
				amplitudeL += amplitudeLincrement;
				amplitudeR += amplitudeRincrement;
			}
		}
		else
		{	// stereo 16 bit instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[(INT32)(cur_wave + cur_wave_next_frame)], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)destL, _MM_HINT_NTA);
				#endif

				for (inner = 0; inner < 4; inner++)
				{
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + (cur_wave_whole * 2);
					b = calculated_source[0];
					c = calculated_source[2];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
					*destL += (sample * amplitudeL) >> 4;
					b = calculated_source[1];
					c = calculated_source[3];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
					destL[1] += (sample * amplitudeR) >> 4;
					destL += 2;
			
					cur_wave += wave_increment;
				}
				amplitudeL += amplitudeLincrement;
				amplitudeR += amplitudeRincrement;
			}
		}
	}
	else
	#endif
	{
		if (this_voice->channels == 1)
		{	// mono instrument

			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
				destL[0] += (sample * amplitudeL) >> 4;
				destL[1] += (sample * amplitudeR) >> 4;
				cur_wave += wave_increment;

				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
				destL[2] += (sample * amplitudeL) >> 4;
				destL[3] += (sample * amplitudeR) >> 4;
				cur_wave += wave_increment;

				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
				destL[4] += (sample * amplitudeL) >> 4;
				destL[5] += (sample * amplitudeR) >> 4;
				cur_wave += wave_increment;

				cur_wave_whole = (INT32)cur_wave;
				calculated_source = source + cur_wave_whole;
				b = calculated_source[0];
				c = calculated_source[1];
				sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
				destL[6] += (sample * amplitudeL) >> 4;
				destL[7] += (sample * amplitudeR) >> 4;
				destL += 8;
				cur_wave += wave_increment;
				amplitudeL += amplitudeLincrement;
				amplitudeR += amplitudeRincrement;
			}
		}
		else
		{	// stereo 16 bit instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				for (inner = 0; inner < 4; inner++)
				{
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + (cur_wave_whole * 2);
					b = calculated_source[0];
					c = calculated_source[2];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
					*destL += (sample * amplitudeL) >> 4;
					b = calculated_source[1];
					c = calculated_source[3];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
					destL[1] += (sample * amplitudeR) >> 4;
					destL += 2;
			
					cur_wave += wave_increment;
				}
				amplitudeL += amplitudeLincrement;
				amplitudeR += amplitudeRincrement;
			}
		}
	}
	this_voice->lastAmplitudeL = amplitudeL << 4;
	this_voice->lastAmplitudeR = amplitudeR << 4;
	this_voice->samplePosition_f = cur_wave;
}

/*
#if 0
INLINE static UFLOAT PV_GetWhole(UFLOAT value)
{
	return value;
}
INLINE static INT32 PV_GetFractionalStep(UFLOAT value, INT32 step1, INT32 step2)
{
	return (((INT32) (value & STEP_FULL_RANGE) * (step2-step1))) + step1;
}
#else
INLINE static INT32 PV_GetWhole(UFLOAT value)
{
		return (INT32)value;
}
INLINE static INT32 PV_GetFractionalStep(UFLOAT value, INT32 step1, INT32 step2)
{
	UFLOAT	nvalue;

	nvalue = fmod(value, 1.0);
	//nvalue = value - ((long)value);
	return (INT32)(nvalue * (c-b)) + b;
}
#endif
*/

void PV_ServeStereoFloatPartialBuffer16 (GM_Voice *this_voice, XBOOL looping)
{
	register INT32 			*destL;
	register LOOPCOUNT		a, inner;
	register INT16 			*source, *calculated_source;
	register INT32			b, c, sample;
	register UFLOAT			cur_wave;
	register INT32			cur_wave_whole;
	register UFLOAT 		wave_increment, wave_increment_x;
	register UFLOAT 		end_wave, wave_adjust;
	INT32					ampValueL, ampValueR;
	register INT32			amplitudeL;
	register INT32			amplitudeR;
	register INT32			amplitudeLincrement, amplitudeRincrement;

#if (USE_SMALL_MEMORY_REVERB == FALSE) && (USE_VARIABLE_REVERB == TRUE)
	if (this_voice->reverbLevel || this_voice->chorusLevel)
	{
		PV_ServeStereoFloatPartialBuffer16NewReverb (this_voice, looping);
		return;
	}
#endif
	PV_CalculateStereoVolume(this_voice, &ampValueL, &ampValueR);
	amplitudeL = this_voice->lastAmplitudeL;
	amplitudeR = this_voice->lastAmplitudeR;
	amplitudeLincrement = (ampValueL - amplitudeL) / (MusicGlobals->Four_Loop);
	amplitudeRincrement = (ampValueR - amplitudeR) / (MusicGlobals->Four_Loop);

	amplitudeL = amplitudeL >> 4;
	amplitudeR = amplitudeR >> 4;
	amplitudeLincrement = amplitudeLincrement >> 4;
	amplitudeRincrement = amplitudeRincrement >> 4;

	destL = &MusicGlobals->songBufferDry[0];
	cur_wave = this_voice->samplePosition_f;
	source = (short *) this_voice->NotePtr;

	wave_increment = PV_GetWavePitchFloat(this_voice->NotePitch);
	wave_increment_x = wave_increment * 3.0;
	if (looping)
	{
		end_wave = (UFLOAT) (this_voice->NoteLoopEnd - this_voice->NotePtr);
		wave_adjust = (this_voice->NoteLoopEnd - this_voice->NoteLoopPtr);
	}
	else
	{
		end_wave = (UFLOAT) (this_voice->NotePtrEnd - this_voice->NotePtr - 1);
	}

	#ifdef KATMAI
	if (MusicGlobals->useKatmaiCPU)
	{
		UFLOAT					cur_wave_next_frame;

		cur_wave_next_frame = wave_increment * (UFLOAT)MusicGlobals->Four_Loop;
		_mm_prefetch((char *)&source[(INT32)cur_wave], _MM_HINT_NTA);
		#ifdef USE_KATMAI_WRITE_CACHE
		_mm_prefetch((char *)destL, _MM_HINT_NTA);
		#endif

		if (this_voice->channels == 1)
		{	// mono instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[(INT32)(cur_wave + cur_wave_next_frame)], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)destL, _MM_HINT_NTA);
				#endif

				if ((cur_wave + wave_increment_x) >= end_wave)
				{
					for (inner = 0; inner < 4; inner++)
					{
						THE_CHECK_FLOAT(INT16 *);
						cur_wave_whole = (INT32)cur_wave;
						calculated_source = source + cur_wave_whole;
						b = calculated_source[0];
						c = calculated_source[1];
						sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
						*destL += (sample * amplitudeL) >> 4;
						destL[1] += (sample * amplitudeR) >> 4;
						destL += 2;
						cur_wave += wave_increment;
					}
				}
				else
				{
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
					*destL += (sample * amplitudeL) >> 4;
					destL[1] += (sample * amplitudeR) >> 4;
					cur_wave += wave_increment;

					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
					destL[2] += (sample * amplitudeL) >> 4;
					destL[3] += (sample * amplitudeR) >> 4;
					cur_wave += wave_increment;

					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
					destL[4] += (sample * amplitudeL) >> 4;
					destL[5] += (sample * amplitudeR) >> 4;
					cur_wave += wave_increment;

					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
					destL[6] += (sample * amplitudeL) >> 4;
					destL[7] += (sample * amplitudeR) >> 4;
					destL += 8;
					cur_wave += wave_increment;
				}
				amplitudeL += amplitudeLincrement;
				amplitudeR += amplitudeRincrement;
			}
		}
		else
		{	// Stereo 16 bit instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[(INT32)(cur_wave + cur_wave_next_frame)], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)destL, _MM_HINT_NTA);
				#endif

				for (inner = 0; inner < 4; inner++)
				{
					THE_CHECK_FLOAT(INT16 *);
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + (cur_wave_whole * 2);
					b = calculated_source[0];
					c = calculated_source[2];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
					*destL += (sample * amplitudeL) >> 4;
					b = calculated_source[1];
					c = calculated_source[3];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
					destL[1] += (sample * amplitudeR) >> 4;
					destL += 2;
					cur_wave += wave_increment;
				}
				amplitudeL += amplitudeLincrement;
				amplitudeR += amplitudeRincrement;
			}
		}
	}
	else
	#endif
	{
		if (this_voice->channels == 1)
		{	// mono instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				if ((cur_wave + wave_increment_x) >= end_wave)
				{
					for (inner = 0; inner < 4; inner++)
					{
						THE_CHECK_FLOAT(INT16 *);
						cur_wave_whole = (INT32)cur_wave;
						calculated_source = source + cur_wave_whole;
						b = calculated_source[0];
						c = calculated_source[1];
						sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
						*destL += (sample * amplitudeL) >> 4;
						destL[1] += (sample * amplitudeR) >> 4;
						destL += 2;
						cur_wave += wave_increment;
					}
				}
				else
				{
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
					*destL += (sample * amplitudeL) >> 4;
					destL[1] += (sample * amplitudeR) >> 4;
					cur_wave += wave_increment;

					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
					destL[2] += (sample * amplitudeL) >> 4;
					destL[3] += (sample * amplitudeR) >> 4;
					cur_wave += wave_increment;

					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
					destL[4] += (sample * amplitudeL) >> 4;
					destL[5] += (sample * amplitudeR) >> 4;
					cur_wave += wave_increment;

					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + cur_wave_whole;
					b = calculated_source[0];
					c = calculated_source[1];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
					destL[6] += (sample * amplitudeL) >> 4;
					destL[7] += (sample * amplitudeR) >> 4;
					destL += 8;
					cur_wave += wave_increment;
				}
				amplitudeL += amplitudeLincrement;
				amplitudeR += amplitudeRincrement;
			}
		}
		else
		{	// Stereo 16 bit instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				for (inner = 0; inner < 4; inner++)
				{
					THE_CHECK_FLOAT(INT16 *);
					cur_wave_whole = (INT32)cur_wave;
					calculated_source = source + (cur_wave_whole * 2);
					b = calculated_source[0];
					c = calculated_source[2];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
					*destL += (sample * amplitudeL) >> 4;
					b = calculated_source[1];
					c = calculated_source[3];
					sample = (((INT32)(FRAC(cur_wave, cur_wave_whole) * (c-b)))) + b;
					destL[1] += (sample * amplitudeR) >> 4;
					destL += 2;
					cur_wave += wave_increment;
				}
				amplitudeL += amplitudeLincrement;
				amplitudeR += amplitudeRincrement;
			}
		}
	}
	this_voice->samplePosition_f = cur_wave;
	this_voice->lastAmplitudeL = amplitudeL << 4;
	this_voice->lastAmplitudeR = amplitudeR << 4;
FINISH:
	return;
}

#endif	// #if USE_FLOAT_LOOPS != FALSE

// EOF of GenSynthFloat.c

