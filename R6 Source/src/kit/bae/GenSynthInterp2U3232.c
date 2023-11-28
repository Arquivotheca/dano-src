/*****************************************************************************/
/*
** "GenSynthInterp2U3232.c"
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
**	3/11/99	MOE: created file from GenSynthInterp2Float.c
*/
/*****************************************************************************/

#include "GenSnd.h"
#include "GenPriv.h"

#if USE_U3232_LOOPS != FALSE

#undef KATMAI
#if (X_PLATFORM == X_WIN95) && USE_KAT == 1

	// KATMAI support
	#include "xmmintrin.h"
	#define KATMAI			1
	//#define USE_KATMAI_WRITE_CACHE
#endif

void PV_ServeU3232FullBuffer (GM_Voice *this_voice)
{
	register INT32 			*dest;
	register LOOPCOUNT		a, inner;
	register UBYTE 			*source, *calculated_source;
	register INT32			b, c;
	register U32	 		cur_wave_i, cur_wave_f;
	U3232			 		wave_increment;
	register INT32			amplitude, amplitudeAdjust;

#if (USE_SMALL_MEMORY_REVERB == FALSE) && (USE_VARIABLE_REVERB == TRUE)
	if (this_voice->reverbLevel || this_voice->chorusLevel)
	{
		PV_ServeU3232FullBufferNewReverb (this_voice); 
		return;
	}
#endif
	amplitude = this_voice->lastAmplitudeL;
	amplitudeAdjust = (this_voice->NoteVolume * this_voice->NoteVolumeEnvelope) >> VOLUME_PRECISION_SCALAR;
	amplitudeAdjust = (amplitudeAdjust - amplitude) / MusicGlobals->Four_Loop;
	dest = &MusicGlobals->songBufferDry[0];
	source = this_voice->NotePtr;
	cur_wave_i = this_voice->samplePosition.i;
	cur_wave_f = this_voice->samplePosition.f;

	wave_increment = PV_GetWavePitchU3232(this_voice->NotePitch);

	#ifdef KATMAI
	if (MusicGlobals->useKatmaiCPU)
	{
	U32					cur_wave_next_frame;

		cur_wave_next_frame = wave_increment.i * MusicGlobals->Four_Loop;
		_mm_prefetch((char *)&source[cur_wave_i], _MM_HINT_NTA);
		#ifdef USE_KATMAI_WRITE_CACHE
		_mm_prefetch((char *)dest, _MM_HINT_NTA);
		#endif

		if (this_voice->channels == 1)
		{
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[cur_wave_i + cur_wave_next_frame], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)dest, _MM_HINT_NTA);
				#endif

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				dest[0] += ((((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80) * amplitude;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				dest[1] += ((((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80) * amplitude;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				dest[2] += ((((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80) * amplitude;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				dest[3] += ((((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80) * amplitude;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
				dest += 4;
				amplitude += amplitudeAdjust;
			}
		}
		else
		{	// stereo 8 bit instrument
			for (a = MusicGlobals->Sixteen_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[cur_wave_i + cur_wave_next_frame], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)dest, _MM_HINT_NTA);
				#endif

				for (inner = 0; inner < 16; inner++)
				{
					calculated_source = source + cur_wave_i * 2;
					b = calculated_source[0] + calculated_source[1];	// average left & right channels
					c = calculated_source[2] + calculated_source[3];
					*dest += (((((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x100) * amplitude) >> 1;
					dest++;
					ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
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
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				dest[0] += ((((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80) * amplitude;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				dest[1] += ((((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80) * amplitude;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				dest[2] += ((((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80) * amplitude;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				dest[3] += ((((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80) * amplitude;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

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
					calculated_source = source + cur_wave_i * 2;
					b = calculated_source[0] + calculated_source[1];	// average left & right channels
					c = calculated_source[2] + calculated_source[3];
					*dest += (((((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x100) * amplitude) >> 1;
					dest++;
					ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
				}
				amplitude += amplitudeAdjust;
			}
		}
	}
	this_voice->samplePosition.i = cur_wave_i;
	this_voice->samplePosition.f = cur_wave_f;
	this_voice->lastAmplitudeL = amplitude;
}

void PV_ServeU3232PartialBuffer (GM_Voice *this_voice, XBOOL looping)
{
	register INT32 			*dest;
	register LOOPCOUNT		a, inner;
	register UBYTE 			*source, *calculated_source;
	register INT32			b, c;
	register U32	 		cur_wave_i, cur_wave_f;
	register U32	 		end_wave, wave_adjust;
	U3232	 				wave_increment;
	register INT32			amplitude, amplitudeAdjust;

#if (USE_SMALL_MEMORY_REVERB == FALSE) && (USE_VARIABLE_REVERB == TRUE)
	if (this_voice->reverbLevel || this_voice->chorusLevel)
	{
		PV_ServeU3232PartialBufferNewReverb (this_voice, looping);
		return;
	}
#endif
	amplitude = this_voice->lastAmplitudeL;
	amplitudeAdjust = (this_voice->NoteVolume * this_voice->NoteVolumeEnvelope) >> VOLUME_PRECISION_SCALAR;
	amplitudeAdjust = (amplitudeAdjust - amplitude) / MusicGlobals->Four_Loop;
	dest = &MusicGlobals->songBufferDry[0];
	source = this_voice->NotePtr;
	cur_wave_i = this_voice->samplePosition.i;
	cur_wave_f = this_voice->samplePosition.f;

	wave_increment = PV_GetWavePitchU3232(this_voice->NotePitch);

	if (looping)
	{
		wave_adjust = this_voice->NoteLoopEnd - this_voice->NoteLoopPtr;
		end_wave = this_voice->NoteLoopEnd - this_voice->NotePtr;
	}
	else
	{
		end_wave = this_voice->NotePtrEnd - this_voice->NotePtr - 1;
	}

	#ifdef KATMAI
	if (MusicGlobals->useKatmaiCPU)
	{
	U32					cur_wave_next_frame;

		cur_wave_next_frame = wave_increment.i * MusicGlobals->Four_Loop;
		_mm_prefetch((char *)&source[cur_wave_i], _MM_HINT_NTA);
		#ifdef USE_KATMAI_WRITE_CACHE
		_mm_prefetch((char *)dest, _MM_HINT_NTA);
		#endif

		if (this_voice->channels == 1)
		{
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[cur_wave_i + cur_wave_next_frame], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)dest, _MM_HINT_NTA);
				#endif

				for (inner = 0; inner < 4; inner++)
				{
					THE_CHECK_U3232(UBYTE *);
					b = source[cur_wave_i];
					c = source[cur_wave_i+1];
					*dest += ((((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80) * amplitude;
					dest++;
					ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
				}
				amplitude += amplitudeAdjust;
			}
		}
		else
		{	// stereo 8 bit instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[cur_wave_i + cur_wave_next_frame], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)dest, _MM_HINT_NTA);
				#endif

				for (inner = 0; inner < 4; inner++)
				{
					THE_CHECK_U3232(UBYTE *);
					calculated_source = source + cur_wave_i * 2;
					b = calculated_source[0] + calculated_source[1];
					c = calculated_source[2] + calculated_source[3];
					*dest += (((((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x100) * amplitude) >> 1;
					dest++;
					ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
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
					THE_CHECK_U3232(UBYTE *);
					b = source[cur_wave_i];
					c = source[cur_wave_i+1];
					*dest += ((((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80) * amplitude;
					dest++;
					ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
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
					THE_CHECK_U3232(UBYTE *);
					calculated_source = source + cur_wave_i * 2;
					b = calculated_source[0] + calculated_source[1];
					c = calculated_source[2] + calculated_source[3];
					*dest += (((((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x100) * amplitude) >> 1;
					dest++;
					ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
				}
				amplitude += amplitudeAdjust;
			}
		}
	}

	this_voice->samplePosition.i = cur_wave_i;
	this_voice->samplePosition.f = cur_wave_f;
	this_voice->lastAmplitudeL = amplitude;
FINISH:
	return;
}

void PV_ServeU3232StereoFullBuffer(GM_Voice *this_voice)
{
	register INT32 			*destL;
	register LOOPCOUNT		a, inner;
	register UBYTE 			*source, *calculated_source;
	register INT32			b, c;
	register U32	 		cur_wave_i, cur_wave_f;
	U3232			 		wave_increment;
	register INT32			sample;
	INT32					ampValueL, ampValueR;
	register INT32			amplitudeL;
	register INT32			amplitudeR;
	register INT32			amplitudeLincrement;
	register INT32			amplitudeRincrement;

#if (USE_SMALL_MEMORY_REVERB == FALSE) && (USE_VARIABLE_REVERB == TRUE)
	if (this_voice->reverbLevel || this_voice->chorusLevel)
	{
		PV_ServeU3232StereoFullBufferNewReverb (this_voice);
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
	cur_wave_i = this_voice->samplePosition.i;
	cur_wave_f = this_voice->samplePosition.f;

	wave_increment = PV_GetWavePitchU3232(this_voice->NotePitch);

	#ifdef KATMAI
	if (MusicGlobals->useKatmaiCPU)
	{
	U32					cur_wave_next_frame;

		cur_wave_next_frame = wave_increment.i * MusicGlobals->Four_Loop;
		_mm_prefetch((char *)&source[cur_wave_i], _MM_HINT_NTA);
		#ifdef USE_KATMAI_WRITE_CACHE
		_mm_prefetch((char *)destL, _MM_HINT_NTA);
		#endif

		if (this_voice->channels == 1)
		{	// mono instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[cur_wave_i + cur_wave_next_frame], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)destL, _MM_HINT_NTA);
				#endif

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80;
				destL[0] += sample * amplitudeL;
				destL[1] += sample * amplitudeR;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80;
				destL[2] += sample * amplitudeL;
				destL[3] += sample * amplitudeR;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80;
				destL[4] += sample * amplitudeL;
				destL[5] += sample * amplitudeR;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80;
				destL[6] += sample * amplitudeL;
				destL[7] += sample * amplitudeR;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				destL += 8;
				amplitudeL += amplitudeLincrement;
				amplitudeR += amplitudeRincrement;
			}
		}
		else
		{	// stereo 8 bit instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[cur_wave_i + cur_wave_next_frame], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)destL, _MM_HINT_NTA);
				#endif

				for (inner = 0; inner < 4; inner++)
				{
					calculated_source = source + cur_wave_i * 2;
					b = calculated_source[0];
					c = calculated_source[2];
					destL[0] += ((((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80) * amplitudeL;
					b = calculated_source[1];
					c = calculated_source[3];
					destL[1] += ((((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80) * amplitudeR;
					destL += 2;
			
					ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
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
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80;
				destL[0] += sample * amplitudeL;
				destL[1] += sample * amplitudeR;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80;
				destL[2] += sample * amplitudeL;
				destL[3] += sample * amplitudeR;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80;
				destL[4] += sample * amplitudeL;
				destL[5] += sample * amplitudeR;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80;
				destL[6] += sample * amplitudeL;
				destL[7] += sample * amplitudeR;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				destL += 8;
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
					calculated_source = source + cur_wave_i * 2;
					b = calculated_source[0];
					c = calculated_source[2];
					destL[0] += ((((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80) * amplitudeL;
					b = calculated_source[1];
					c = calculated_source[3];
					destL[1] += ((((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80) * amplitudeR;
					destL += 2;
			
					ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
				}
				amplitudeL += amplitudeLincrement;
				amplitudeR += amplitudeRincrement;
			}
		}
	}
	this_voice->lastAmplitudeL = amplitudeL;
	this_voice->lastAmplitudeR = amplitudeR;
	this_voice->samplePosition.i = cur_wave_i;
	this_voice->samplePosition.f = cur_wave_f;
}

void PV_ServeU3232StereoPartialBuffer (GM_Voice *this_voice, XBOOL looping)
{
	register INT32 			*destL;
	register LOOPCOUNT		a, inner;
	register UBYTE 			*source, *calculated_source;
	register INT32			b, c, sample;
	register U32	 		cur_wave_i, cur_wave_f;
	register U32	 		end_wave, wave_adjust;
	U3232	 				wave_increment;
	INT32					ampValueL, ampValueR;
	register INT32			amplitudeL;
	register INT32			amplitudeR;
	register INT32			amplitudeLincrement, amplitudeRincrement;

#if (USE_SMALL_MEMORY_REVERB == FALSE) && (USE_VARIABLE_REVERB == TRUE)
	if (this_voice->reverbLevel || this_voice->chorusLevel)
	{
		PV_ServeU3232StereoPartialBufferNewReverb (this_voice, looping); 
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
	cur_wave_i = this_voice->samplePosition.i;
	cur_wave_f = this_voice->samplePosition.f;

	wave_increment = PV_GetWavePitchU3232(this_voice->NotePitch);

	if (looping)
	{
		wave_adjust = this_voice->NoteLoopEnd - this_voice->NoteLoopPtr;
		end_wave = this_voice->NoteLoopEnd - this_voice->NotePtr;
	}
	else
	{
		end_wave = this_voice->NotePtrEnd - this_voice->NotePtr - 1;
	}

	#ifdef KATMAI
	if (MusicGlobals->useKatmaiCPU)
	{
	U32					cur_wave_next_frame;

		cur_wave_next_frame = wave_increment.i * MusicGlobals->Four_Loop;
		_mm_prefetch((char *)&source[cur_wave_i], _MM_HINT_NTA);
		#ifdef USE_KATMAI_WRITE_CACHE
		_mm_prefetch((char *)destL, _MM_HINT_NTA);
		#endif

		if (this_voice->channels == 1)
		{	// mono instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[cur_wave_i + cur_wave_next_frame], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)destL, _MM_HINT_NTA);
				#endif

#if 1	//MOE'S OBSESSIVE FOLLY
				for (inner = 0; inner < 4; inner++)
				{
					THE_CHECK_U3232(UBYTE *);
					b = source[cur_wave_i];
					c = source[cur_wave_i+1];
					sample = (((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80;
					destL[0] += sample * amplitudeL;
					destL[1] += sample * amplitudeR;
					ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
					destL += 2;
				}
#else
				THE_CHECK_U3232(UBYTE *);
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80;
				destL[0] += sample * amplitudeL;
				destL[1] += sample * amplitudeR;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				THE_CHECK_U3232(UBYTE *);
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80;
				destL[2] += sample * amplitudeL;
				destL[3] += sample * amplitudeR;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				THE_CHECK_U3232(UBYTE *);
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80;
				destL[4] += sample * amplitudeL;
				destL[5] += sample * amplitudeR;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				THE_CHECK_U3232(UBYTE *);
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80;
				destL[6] += sample * amplitudeL;
				destL[7] += sample * amplitudeR;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				destL += 8;
#endif
				amplitudeL += amplitudeLincrement;
				amplitudeR += amplitudeRincrement;
			}
		}
		else
		{	// Stereo 8 bit instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[cur_wave_i + cur_wave_next_frame], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)destL, _MM_HINT_NTA);
				#endif

				for (inner = 0; inner < 4; inner++)
				{
					THE_CHECK_U3232(UBYTE *);
					calculated_source = source + cur_wave_i * 2;
					b = calculated_source[0];
					c = calculated_source[2];
					destL[0] += ((((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80) * amplitudeL;
					b = calculated_source[1];
					c = calculated_source[3];
					destL[1] += ((((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80) * amplitudeR;
					destL += 2;
					ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
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
#if 1	//MOE'S OBSESSIVE FOLLY
				for (inner = 0; inner < 4; inner++)
				{
					THE_CHECK_U3232(UBYTE *);
					b = source[cur_wave_i];
					c = source[cur_wave_i+1];
					sample = (((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80;
					destL[0] += sample * amplitudeL;
					destL[1] += sample * amplitudeR;
					ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
					destL += 2;
				}
#else
				THE_CHECK_U3232(UBYTE *);
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80;
				destL[0] += sample * amplitudeL;
				destL[1] += sample * amplitudeR;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
				
				THE_CHECK_U3232(UBYTE *);
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80;
				destL[2] += sample * amplitudeL;
				destL[3] += sample * amplitudeR;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				THE_CHECK_U3232(UBYTE *);
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80;
				destL[4] += sample * amplitudeL;
				destL[5] += sample * amplitudeR;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				THE_CHECK_U3232(UBYTE *);
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80;
				destL[6] += sample * amplitudeL;
				destL[7] += sample * amplitudeR;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				destL += 8;
#endif
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
					THE_CHECK_U3232(UBYTE *);
					calculated_source = source + cur_wave_i * 2;
					b = calculated_source[0];
					c = calculated_source[2];
					destL[0] += ((((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80) * amplitudeL;
					b = calculated_source[1];
					c = calculated_source[3];
					destL[1] += ((((INT32)(cur_wave_f >> 16) * (INT32)(c-b)) >> 16) + b - 0x80) * amplitudeR;
					destL += 2;
					ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
				}
				amplitudeL += amplitudeLincrement;
				amplitudeR += amplitudeRincrement;
			}
		}
	}
	this_voice->samplePosition.i = cur_wave_i;
	this_voice->samplePosition.f = cur_wave_f;
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
void PV_ServeU3232FullBuffer16 (GM_Voice *this_voice)
{
	register INT32 			*dest;
	register LOOPCOUNT		a, inner;
	register INT16 			*source, *calculated_source;
	register INT32			b, c, sample;
	register U32	 		cur_wave_i, cur_wave_f;
	U3232			 		wave_increment;
	register INT32			amplitude, amplitudeAdjust;

#if (USE_SMALL_MEMORY_REVERB == FALSE) && (USE_VARIABLE_REVERB == TRUE)
	if (this_voice->reverbLevel || this_voice->chorusLevel)
	{
		PV_ServeU3232FullBuffer16NewReverb (this_voice); 
		return;
	}
#endif
	amplitude = this_voice->lastAmplitudeL;
	amplitudeAdjust = (this_voice->NoteVolume * this_voice->NoteVolumeEnvelope) >> VOLUME_PRECISION_SCALAR;
	amplitudeAdjust = (amplitudeAdjust - amplitude) / MusicGlobals->Four_Loop >> 4;
	amplitude = amplitude >> 4;

	dest = &MusicGlobals->songBufferDry[0];
	source = (short *) this_voice->NotePtr;
	cur_wave_i = this_voice->samplePosition.i;
	cur_wave_f = this_voice->samplePosition.f;

	wave_increment = PV_GetWavePitchU3232(this_voice->NotePitch);

	#ifdef KATMAI
	if (MusicGlobals->useKatmaiCPU)
	{
	U32					cur_wave_next_frame;

		cur_wave_next_frame = wave_increment.i * MusicGlobals->Four_Loop;
		_mm_prefetch((char *)&source[cur_wave_i], _MM_HINT_NTA);
		#ifdef USE_KATMAI_WRITE_CACHE
		_mm_prefetch((char *)dest, _MM_HINT_NTA);
		#endif

		if (this_voice->channels == 1)
		{
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[cur_wave_i + cur_wave_next_frame], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)dest, _MM_HINT_NTA);
				#endif

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				dest[0] += (((((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b) * amplitude) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				dest[1] += (((((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b) * amplitude) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				dest[2] += (((((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b) * amplitude) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				dest[3] += (((((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b) * amplitude) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				dest += 4;
				amplitude += amplitudeAdjust;
			}
		}
		else
		{	// stereo 16 bit instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[cur_wave_i + cur_wave_next_frame], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)dest, _MM_HINT_NTA);
				#endif

				for (inner = 0; inner < 4; inner++)
				{
					calculated_source = source + cur_wave_i * 2;
					b = calculated_source[0] + calculated_source[1];
					c = calculated_source[2] + calculated_source[3];
					sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
					*dest += (sample  * amplitude) >> 5;	// divide extra for summed stereo channels
					dest++;

					ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
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
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				dest[0] += (((((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b) * amplitude) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				dest[1] += (((((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b) * amplitude) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				dest[2] += (((((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b) * amplitude) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				dest[3] += (((((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b) * amplitude) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				dest += 4;
				amplitude += amplitudeAdjust;
			}
		}
		else
		{	// stereo 16 bit instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				for (inner = 0; inner < 4; inner++)
				{
					calculated_source = source + cur_wave_i * 2;
					b = calculated_source[0] + calculated_source[1];
					c = calculated_source[2] + calculated_source[3];
					sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
					*dest += (sample  * amplitude) >> 5;	// divide extra for summed stereo channels
					dest++;

					ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
				}
				amplitude += amplitudeAdjust;
			}
		}
	}
	this_voice->samplePosition.i = cur_wave_i;
	this_voice->samplePosition.f = cur_wave_f;
	this_voice->lastAmplitudeL = amplitude << 4;
}

void PV_ServeU3232PartialBuffer16 (GM_Voice *this_voice, XBOOL looping)
{
	register INT32 			*dest;
	register LOOPCOUNT		a, inner;
	register INT16 			*source, *calculated_source;
	register INT32			b, c, sample;
	register U32	 		cur_wave_i, cur_wave_f;
	register U32	 		end_wave, wave_adjust;
	U3232	 				wave_increment;
	register INT32			amplitude, amplitudeAdjust;

#if (USE_SMALL_MEMORY_REVERB == FALSE) && (USE_VARIABLE_REVERB == TRUE)
	if (this_voice->reverbLevel || this_voice->chorusLevel)
	{
		PV_ServeU3232PartialBuffer16NewReverb (this_voice, looping); 
		return;
	}
#endif
	amplitude = this_voice->lastAmplitudeL;
	amplitudeAdjust = (this_voice->NoteVolume * this_voice->NoteVolumeEnvelope) >> VOLUME_PRECISION_SCALAR;
	amplitudeAdjust = (amplitudeAdjust - amplitude) / MusicGlobals->Four_Loop >> 4;
	amplitude = amplitude >> 4;

	dest = &MusicGlobals->songBufferDry[0];
	cur_wave_i = this_voice->samplePosition.i;
	cur_wave_f = this_voice->samplePosition.f;
	source = (short *) this_voice->NotePtr;

	wave_increment = PV_GetWavePitchU3232(this_voice->NotePitch);

	if (looping)
	{
		wave_adjust = this_voice->NoteLoopEnd - this_voice->NoteLoopPtr;
		end_wave = this_voice->NoteLoopEnd - this_voice->NotePtr;
	}
	else
	{
		end_wave = this_voice->NotePtrEnd - this_voice->NotePtr - 1;
	}

	#ifdef KATMAI
	if (MusicGlobals->useKatmaiCPU)
	{
	U32					cur_wave_next_frame;

		cur_wave_next_frame = wave_increment.i * MusicGlobals->Four_Loop;
		_mm_prefetch((char *)&source[cur_wave_i], _MM_HINT_NTA);
		#ifdef USE_KATMAI_WRITE_CACHE
		_mm_prefetch((char *)dest, _MM_HINT_NTA);
		#endif

		if (this_voice->channels == 1)
		{
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[cur_wave_i + cur_wave_next_frame], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)dest, _MM_HINT_NTA);
				#endif

#if 1	//MOE'S OBSESSIVE FOLLY
				for (inner = 0; inner < 4; inner++)
				{
					THE_CHECK_U3232(INT16 *);
					b = source[cur_wave_i];
					c = source[cur_wave_i+1];
					dest[0] += (((((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b) * amplitude) >> 4;
					ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
					dest++;
				}
#else
				THE_CHECK_U3232(INT16 *);
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				dest[0] += (((((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b) * amplitude) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				THE_CHECK_U3232(INT16 *);
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				dest[1] += (((((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b) * amplitude) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				THE_CHECK_U3232(INT16 *);
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				dest[2] += (((((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b) * amplitude) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				THE_CHECK_U3232(INT16 *);
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				dest[3] += (((((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b) * amplitude) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				dest += 4;
#endif
				amplitude += amplitudeAdjust;
			}
		}
		else
		{
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[cur_wave_i + cur_wave_next_frame], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)dest, _MM_HINT_NTA);
				#endif

				for (inner = 0; inner < 4; inner++)
				{
					THE_CHECK_U3232(INT16 *);
					calculated_source = source + cur_wave_i * 2;
					b = calculated_source[0] + calculated_source[1];
					c = calculated_source[2] + calculated_source[3];
					sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
					*dest += ((sample >> 1) * amplitude) >> 5;
					ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
					dest++;
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
#if 1	//MOE'S OBSESSIVE FOLLY
				for (inner = 0; inner < 4; inner++)
				{
					THE_CHECK_U3232(INT16 *);
					b = source[cur_wave_i];
					c = source[cur_wave_i+1];
					dest[0] += (((((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b) * amplitude) >> 4;
					ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
					dest++;
				}
#else
				THE_CHECK_U3232(INT16 *);
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				dest[0] += (((((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b) * amplitude) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				THE_CHECK_U3232(INT16 *);
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				dest[1] += (((((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b) * amplitude) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				THE_CHECK_U3232(INT16 *);
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				dest[2] += (((((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b) * amplitude) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				THE_CHECK_U3232(INT16 *);
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				dest[3] += (((((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b) * amplitude) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				dest += 4;
#endif
				amplitude += amplitudeAdjust;
			}
		}
		else
		{
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				for (inner = 0; inner < 4; inner++)
				{
					THE_CHECK_U3232(INT16 *);
					calculated_source = source + cur_wave_i * 2;
					b = calculated_source[0] + calculated_source[1];
					c = calculated_source[2] + calculated_source[3];
					sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
					*dest += ((sample >> 1) * amplitude) >> 5;
					dest++;
					ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
				}
				amplitude += amplitudeAdjust;
			}
		}
	}
	this_voice->samplePosition.i = cur_wave_i;
	this_voice->samplePosition.f = cur_wave_f;
	this_voice->lastAmplitudeL = amplitude << 4;
FINISH:
	return;
}

void PV_ServeU3232StereoFullBuffer16 (GM_Voice *this_voice)
{
	register INT32 			*destL;
	register LOOPCOUNT		a, inner;
	register INT16 			*source, *calculated_source;
	register INT32			b, c;
	register U32	 		cur_wave_i, cur_wave_f;
	U3232			 		wave_increment;
	register INT32			sample;
	INT32					ampValueL, ampValueR;
	register INT32			amplitudeL;
	register INT32			amplitudeR;
	register INT32			amplitudeLincrement;
	register INT32			amplitudeRincrement;

#if (USE_SMALL_MEMORY_REVERB == FALSE) && (USE_VARIABLE_REVERB == TRUE)
	if (this_voice->reverbLevel || this_voice->chorusLevel)
	{ 
		PV_ServeU3232StereoFullBuffer16NewReverb (this_voice);
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
	cur_wave_i = this_voice->samplePosition.i;
	cur_wave_f = this_voice->samplePosition.f;

	source = (short *) this_voice->NotePtr;

	wave_increment = PV_GetWavePitchU3232(this_voice->NotePitch);
	#ifdef KATMAI
	if (MusicGlobals->useKatmaiCPU)
	{
	U32					cur_wave_next_frame;

		cur_wave_next_frame = wave_increment.i * MusicGlobals->Four_Loop;
		_mm_prefetch((char *)&source[cur_wave_i], _MM_HINT_NTA);
		#ifdef USE_KATMAI_WRITE_CACHE
		_mm_prefetch((char *)destL, _MM_HINT_NTA);
		#endif

		if (this_voice->channels == 1)
		{	// mono instrument

			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[cur_wave_i + cur_wave_next_frame], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)destL, _MM_HINT_NTA);
				#endif

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
				destL[0] += (sample * amplitudeL) >> 4;
				destL[1] += (sample * amplitudeR) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
				destL[2] += (sample * amplitudeL) >> 4;
				destL[3] += (sample * amplitudeR) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
				destL[4] += (sample * amplitudeL) >> 4;
				destL[5] += (sample * amplitudeR) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
				destL[6] += (sample * amplitudeL) >> 4;
				destL[7] += (sample * amplitudeR) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				destL += 8;
				amplitudeL += amplitudeLincrement;
				amplitudeR += amplitudeRincrement;
			}
		}
		else
		{	// stereo 16 bit instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[cur_wave_i + cur_wave_next_frame], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)destL, _MM_HINT_NTA);
				#endif

				for (inner = 0; inner < 4; inner++)
				{
					calculated_source = source + cur_wave_i * 2;
					b = calculated_source[0];
					c = calculated_source[2];
					sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
					destL[0] += (sample * amplitudeL) >> 4;
					b = calculated_source[1];
					c = calculated_source[3];
					sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
					destL[1] += (sample * amplitudeR) >> 4;
					destL += 2;
			
					ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
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
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
				destL[0] += (sample * amplitudeL) >> 4;
				destL[1] += (sample * amplitudeR) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
				destL[2] += (sample * amplitudeL) >> 4;
				destL[3] += (sample * amplitudeR) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
				destL[4] += (sample * amplitudeL) >> 4;
				destL[5] += (sample * amplitudeR) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
				destL[6] += (sample * amplitudeL) >> 4;
				destL[7] += (sample * amplitudeR) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				destL += 8;
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
					calculated_source = source + cur_wave_i * 2;
					b = calculated_source[0];
					c = calculated_source[2];
					sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
					destL[0] += (sample * amplitudeL) >> 4;
					b = calculated_source[1];
					c = calculated_source[3];
					sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
					destL[1] += (sample * amplitudeR) >> 4;
					destL += 2;
			
					ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
				}
				amplitudeL += amplitudeLincrement;
				amplitudeR += amplitudeRincrement;
			}
		}
	}
	this_voice->lastAmplitudeL = amplitudeL << 4;
	this_voice->lastAmplitudeR = amplitudeR << 4;
	this_voice->samplePosition.i = cur_wave_i;
	this_voice->samplePosition.f = cur_wave_f;
}

void PV_ServeU3232StereoPartialBuffer16 (GM_Voice *this_voice, XBOOL looping)
{
	register INT32 			*destL;
	register LOOPCOUNT		a, inner;
	register INT16 			*source, *calculated_source;
	register INT32			b, c, sample;
	register U32	 		cur_wave_i, cur_wave_f;
	register U32	 		end_wave, wave_adjust;
	U3232	 				wave_increment;
	INT32					ampValueL, ampValueR;
	register INT32			amplitudeL;
	register INT32			amplitudeR;
	register INT32			amplitudeLincrement, amplitudeRincrement;

#if (USE_SMALL_MEMORY_REVERB == FALSE) && (USE_VARIABLE_REVERB == TRUE)
	if (this_voice->reverbLevel || this_voice->chorusLevel)
	{
		PV_ServeU3232StereoPartialBuffer16NewReverb (this_voice, looping);
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
	cur_wave_i = this_voice->samplePosition.i;
	cur_wave_f = this_voice->samplePosition.f;
	source = (short *) this_voice->NotePtr;

	wave_increment = PV_GetWavePitchU3232(this_voice->NotePitch);

	if (looping)
	{
		wave_adjust = this_voice->NoteLoopEnd - this_voice->NoteLoopPtr;
		end_wave = this_voice->NoteLoopEnd - this_voice->NotePtr;
	}
	else
	{
		end_wave = this_voice->NotePtrEnd - this_voice->NotePtr - 1;
	}

	#ifdef KATMAI
	if (MusicGlobals->useKatmaiCPU)
	{
	U32					cur_wave_next_frame;

		cur_wave_next_frame = wave_increment.i * MusicGlobals->Four_Loop;
		_mm_prefetch((char *)&source[cur_wave_i], _MM_HINT_NTA);
		#ifdef USE_KATMAI_WRITE_CACHE
		_mm_prefetch((char *)destL, _MM_HINT_NTA);
		#endif

		if (this_voice->channels == 1)
		{	// mono instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[cur_wave_i + cur_wave_next_frame], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)destL, _MM_HINT_NTA);
				#endif
#if 1	//MOE'S OBSESSIVE FOLLY
				for (inner = 0; inner < 4; inner++)
				{
					THE_CHECK_U3232(INT16 *);
					b = source[cur_wave_i];
					c = source[cur_wave_i+1];
					sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
					destL[0] += (sample * amplitudeL) >> 4;
					destL[1] += (sample * amplitudeR) >> 4;
					ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
					destL += 2;
				}
#else
				THE_CHECK_U3232(INT16 *);
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
				destL[0] += (sample * amplitudeL) >> 4;
				destL[1] += (sample * amplitudeR) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				THE_CHECK_U3232(INT16 *);
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
				destL[2] += (sample * amplitudeL) >> 4;
				destL[3] += (sample * amplitudeR) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				THE_CHECK_U3232(INT16 *);
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
				destL[4] += (sample * amplitudeL) >> 4;
				destL[5] += (sample * amplitudeR) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				THE_CHECK_U3232(INT16 *);
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
				destL[6] += (sample * amplitudeL) >> 4;
				destL[7] += (sample * amplitudeR) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				destL += 8;
#endif
				amplitudeL += amplitudeLincrement;
				amplitudeR += amplitudeRincrement;
			}
		}
		else
		{	// Stereo 16 bit instrument
			for (a = MusicGlobals->Four_Loop; a > 0; --a)
			{
				_mm_prefetch((char *)&source[cur_wave_i + cur_wave_next_frame], _MM_HINT_NTA);
				#ifdef USE_KATMAI_WRITE_CACHE
				_mm_prefetch((char *)destL, _MM_HINT_NTA);
				#endif

				for (inner = 0; inner < 4; inner++)
				{
					THE_CHECK_U3232(INT16 *);
					calculated_source = source + cur_wave_i * 2;
					b = calculated_source[0];
					c = calculated_source[2];
					sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
					destL[0] += (sample * amplitudeL) >> 4;
					b = calculated_source[1];
					c = calculated_source[3];
					sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
					destL[1] += (sample * amplitudeR) >> 4;
					destL += 2;
					ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
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
#if 1	//MOE'S OBSESSIVE FOLLY
				for (inner = 0; inner < 4; inner++)
				{
					THE_CHECK_U3232(INT16 *);
					b = source[cur_wave_i];
					c = source[cur_wave_i+1];
					sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
					destL[0] += (sample * amplitudeL) >> 4;
					destL[1] += (sample * amplitudeR) >> 4;
					ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
					destL += 2;
				}
#else
				THE_CHECK_U3232(INT16 *);
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
				destL[0] += (sample * amplitudeL) >> 4;
				destL[1] += (sample * amplitudeR) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				THE_CHECK_U3232(INT16 *);
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
				destL[2] += (sample * amplitudeL) >> 4;
				destL[3] += (sample * amplitudeR) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				THE_CHECK_U3232(INT16 *);
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
				destL[4] += (sample * amplitudeL) >> 4;
				destL[5] += (sample * amplitudeR) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				THE_CHECK_U3232(INT16 *);
				b = source[cur_wave_i];
				c = source[cur_wave_i+1];
				sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
				destL[6] += (sample * amplitudeL) >> 4;
				destL[7] += (sample * amplitudeR) >> 4;
				ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);

				destL += 8;
#endif
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
					THE_CHECK_U3232(INT16 *);
					calculated_source = source + cur_wave_i * 2;
					b = calculated_source[0];
					c = calculated_source[2];
					sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
					destL[0] += (sample * amplitudeL) >> 4;
					b = calculated_source[1];
					c = calculated_source[3];
					sample = (((INT32)(cur_wave_f >> 17) * (INT32)(c-b)) >> 15) + b;
					destL[1] += (sample * amplitudeR) >> 4;
					destL += 2;
					ADD_U3232(cur_wave_i, cur_wave_f, wave_increment);
				}
				amplitudeL += amplitudeLincrement;
				amplitudeR += amplitudeRincrement;
			}
		}
	}
	this_voice->samplePosition.i = cur_wave_i;
	this_voice->samplePosition.f = cur_wave_f;
	this_voice->lastAmplitudeL = amplitudeL << 4;
	this_voice->lastAmplitudeR = amplitudeR << 4;
FINISH:
	return;
}

#endif	// #if USE_U3232_LOOPS != FALSE

