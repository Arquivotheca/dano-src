/*****************************************************************************/
/*
** "BAEPrivate.h"
**
**	Private access to BAE internals.
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
** Modification History:
**
**	5/7/96		Created
**				Moved BAE_TranslateOPErr from BAE.cpp
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
**	7/17/98		Added BAE_UseThisFile
**	11/19/98	Added new parameter to BAE_UseThisFile
**	3/2/99		Added BAE_TranslateBAErr
**	7/13/99		Renamed HAE to BAE. Renamed BAErr to BAEResult
*/
/*****************************************************************************/

#ifndef BAE_AUDIO_PRIVATE
#define BAE_AUDIO_PRIVATE

#ifndef BAE_AUDIO
	#include "BAE.h"
#endif

#ifndef G_PRIVATE
	#include "GenPriv.h"
#endif

#if USE_HIGHLEVEL_FILE_API == TRUE
// translate file types
AudioFileType BAE_TranslateBAEFileType(BAEFileType fileType);
#endif

// Translate from OPErr to BAEResult
BAEResult BAE_TranslateOPErr(OPErr theErr);

// Translate from BAEResult to OPErr
OPErr BAE_TranslateBAErr(BAEResult theErr);

// translate reverb types from BAEReverbType to ReverbMode
ReverbMode BAE_TranslateFromBAEReverb(BAEReverbType igorVerb);
// translate reverb types to BAEReverbType from ReverbMode
BAEReverbType BAE_TranslateToBAEReverb(ReverbMode r);

// Change audio file to use the passed in XFILE
BAEResult BAE_UseThisFile(XFILE audioFile, XBOOL closeOldFile);

#endif	// BAE_AUDIO_PRIVATE



