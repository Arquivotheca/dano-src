/*****************************************************************************/
/*
** "BAEPrivate.h"
**
**	Private access to BAE internals.
**	This is a macro file used to translate from BAE.h to HAE.h
**
**	\xA9 Copyright 1996-1997 Beatnik, Inc, All Rights Reserved.
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
** Modification History:
**
**	6/15/99		Created
**	7/13/99		Renamed HAE to BAE. Renamed BAEAudioMixer to BAEAudioOutput. Renamed BAEMidiDirect
**				to BAEMidiSynth. Renamed BAEMidiFile to BAEMidiSong. Renamed BAERMFFile to BAERmfSong.
**				Renamed BAErr to BAEResult. Renamed BAEMidiExternal to BAEMidiInput. Renamed BAEMod
**				to BAEModSong. Renamed BAEGroup to BAENoiseGroup. Renamed BAEReverbMode to BAEReverbType.
**				Renamed BAEAudioNoise to BAENoise.
*/
/*****************************************************************************/

#ifndef HAE_AUDIO_PRIVATE
#define HAE_AUDIO_PRIVATE

#include "BAEPrivate.h"

#define HAE_TranslateOPErr			BAE_TranslateOPErr
#define HAE_TranslateHAErr			BAE_TranslateBAErr
#define HAE_TranslateFromHAEReverb	BAE_TranslateFromBAEReverb
#define	HAE_TranslateToHAEReverb	BAE_TranslateToBAEReverb
#define HAE_UseThisFile				BAE_UseThisFile

#endif	// HAE_AUDIO_PRIVATE



