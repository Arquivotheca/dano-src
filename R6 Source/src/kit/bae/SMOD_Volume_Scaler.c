/*****************************************************************************/
/*
** "SMOD_Volume_Scaler.c"
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
** Sound Modifier:  Amplifier/Volume Scaler.
**
** Parameter 1 is the multiplication factor (as a whole number.)  Set this
** number higher for more amplification.
**
** Parameter 2 is the division factor (also a whole number.)  Set this
** number higher for lower volume.
**
** The intermediate results are calculated to 32 bit precision.  Volume
** scaling of, for example, 100 / 99, will work as expected (a 1% rise in
** volume.)
**
**
** Written by James L. Nitchals.
**
** 'C' version by Steve Hales.
**
** Modification History:
**
**	10/5/95		Created
**	6/30/96		Changed font and retabbed
**	12/30/96	Changed copyright
**	2/11/98		Changed copyright, and did some house cleaning
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
*/
/*****************************************************************************/

#include "GenSnd.h"
#include "GenPriv.h"

#include "SMOD.h"

void VolumeAmpScaler(register unsigned char *pSample, long length, long param1, long param2)
{
	register long	count, scaleCount, scale;
	unsigned char	scaledLookup[256];

	if (pSample && length && param1 && param2 && (param1 != param2))
	{
		// build new scaling table
		scaleCount = param2 / 2;
		for (count = 0; count < 256; count++)
		{
			scale = (128 - count) * param1;
			if (scale < 0)
			{
				scale -= scaleCount;
			}
			else
			{
				scale += scaleCount;
			}
			scale = scale / param2;
			// Clip samples to max headroom
			if (scale > 127)
			{
				scale = 127;
			}
			if (scale < -128)
			{
				scale = -128;
			}
			scaledLookup[count] = scale + 128;
		}
		// Scale the samples via a the new lookup table
		for (count = 0; count < length; count++)
		{
			scale = pSample[count];
			pSample[count] = scaledLookup[scale];
		}
	}
}

// EOF of SMOD_Volume_Scaler.c
