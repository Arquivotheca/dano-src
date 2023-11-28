/*
 * SNIFFER.c
 * Font Fusion Copyright (c) 1989-1999 all rights reserved by Bitstream Inc.
 * http://www.bitstream.com/
 * http://www.typesolutions.com/
 * Author: Robert Eggers
 *
 * This software is the property of Bitstream Inc. and it is furnished
 * under a license and may be used and copied only in accordance with the
 * terms of such license and with the inclusion of the above copyright notice.
 * This software or any other copies thereof may not be provided or otherwise
 * made available to any other person or entity except as allowed under license.
 * No title to and no ownership of the software or intellectual property
 * contained herein is hereby transferred. This information in this software
 * is subject to change without notice
 */

#include "syshead.h"
#include "t2k.h"
#ifdef ENABLE_SPD
#include "spdread.h"
#endif

static char IsTT(InputStream *in);


#ifdef ENABLE_PFR
static char IsPFR(InputStream *in);
#endif

#ifdef ENABLE_T1
static char IsT1(InputStream *in);
#endif

#ifdef ENABLE_CFF
static char IsCFF(InputStream *in);
#endif

#ifdef ENABLE_PCL
static char IsPCLEO(InputStream *in);
#endif

#ifdef ENABLE_PCLETTO
static char IsPCLETTO(InputStream *in);
#endif

#ifdef ENABLE_SPD
static char IsSPD(InputStream *in);
#endif

/*
 *	Internal Font type from Stream Function
 */
short ff_FontTypeFromStream(InputStream *in)
{
	if (IsTT(in))
		return FONT_TYPE_TT_OR_T2K;
#ifdef ENABLE_PCL
	if (IsPCLEO(in))
		return FONT_TYPE_PCL;
#endif
#ifdef ENABLE_PCLETTO
	if (IsPCLETTO(in))
		return FONT_TYPE_PCLETTO;
#endif
#ifdef ENABLE_PFR
	if (IsPFR(in))
		return FONT_TYPE_PFR;
#endif
#ifdef ENABLE_CFF
	if (IsCFF(in))
		return FONT_TYPE_2;
#endif
#ifdef ENABLE_T1
	if (IsT1(in))
		return FONT_TYPE_1;
#endif
#ifdef ENABLE_SPD
	if (IsSPD(in))
		return FONT_TYPE_SPD;
#endif
	return -1;
}

/*
 *	Public Font type from Stream Function
 */
short FF_FontTypeFromStream(InputStream *in, int *errCode)
{
	if ( (*errCode = setjmp(in->mem->env)) == 0 )
	{ /* try */
		return ff_FontTypeFromStream(in);
	}
	else
	{ /* catch */
		tsi_EmergencyShutDown(in->mem);
		return (short)*errCode;
	}
}

#ifdef ENABLE_PCL
static char IsPCLEO(InputStream *in)
{
char isMe;
uint8 aBuffer[10];
int16 temp16;
	Seek_InputStream( in, 0 );
	ReadSegment( in, (uint8 *)&aBuffer[0], 3 );
	isMe = (char)(((aBuffer[0] == 0x1b) && (aBuffer[1] == 0x29) && (aBuffer[2] == 0x73) ));
	if (isMe)
	{/* ensure font is *not* pcletto font: */
		do
		{
			aBuffer[0] = ReadUnsignedByteMacro(in);
		} while (aBuffer[0] != 'W');
		temp16 = ReadInt16(in);
		isMe = (char)(temp16 != 72);	/* if (temp16 == 72) then it is pcletto */
	}
	Seek_InputStream( in, 0 );
	return isMe;
}
#endif

#ifdef ENABLE_PCLETTO
static char IsPCLETTO(InputStream *in)
{
char isMe;
uint8 aBuffer[10];
int16 temp16;
	Seek_InputStream( in, 0 );
	ReadSegment( in, (uint8 *)&aBuffer[0], 3 );
	isMe = (char)(((aBuffer[0] == 0x1b) && (aBuffer[1] == 0x29) && (aBuffer[2] == 0x73) ));
	if (isMe)
	{/* ensure font is a pcletto font: */
		do
		{
			aBuffer[0] = ReadUnsignedByteMacro(in);
		} while (aBuffer[0] != 'W');
		temp16 = ReadInt16(in);
		isMe = (char)(temp16 == 72);	/* if (temp16 == 72) then it is pcletto */
	}
	Seek_InputStream( in, 0 );
	return isMe;
}
#endif

#ifdef ENABLE_PFR
static char IsPFR(InputStream *in)
{
char isMe;
uint8 aBuffer[10];
	Seek_InputStream( in, 0 );
	ReadSegment( in, (uint8 *)&aBuffer[0], 10 );
	isMe = (char)(((aBuffer[0] == (uint8)'P') && (aBuffer[1] == (uint8)'F') && (aBuffer[2] == (uint8)'R') ));
	Seek_InputStream( in, 0 );
	return isMe;
}
#endif

static char IsTT(InputStream *in)
{
char isMe;
uint8 aBuffer[10];
	Seek_InputStream( in, 0 );
	ReadSegment( in, (uint8 *)&aBuffer[0], 10 );
	isMe = (char)(((aBuffer[0] == (uint8)0x00) && ((aBuffer[1] == (uint8)0x01) || (aBuffer[1] == (uint8)0x02)) &&
						(aBuffer[2] == (uint8)0x00) && (aBuffer[3] == (uint8)0x00) ));
	if (!isMe)	/* not regular TTF, see if TTC ('ttcf') */
		isMe = (char)(((aBuffer[0] == (uint8)'t') && (aBuffer[1] == (uint8)'t') &&
						(aBuffer[2] == (uint8)'c') && (aBuffer[3] == (uint8)'f') ));

	if (!isMe)	/* not regular TTF or TTC, see if OTF ('OTTO') */
		isMe = (char)(((aBuffer[0] == (uint8)'O') && (aBuffer[1] == (uint8)'T') &&
						(aBuffer[2] == (uint8)'T') && (aBuffer[3] == (uint8)'O') ));
	if (!isMe)	/* not regular TTF, see if Font Bureau ('true') */
		isMe = (char)(((aBuffer[0] == (uint8)'t') && (aBuffer[1] == (uint8)'r') &&
						(aBuffer[2] == (uint8)'u') && (aBuffer[3] == (uint8)'e') ));

	Seek_InputStream( in, 0 );
	return isMe;
}

#ifdef ENABLE_T1
static char IsT1(InputStream *in)
{
char isMe;
int8 aBuffer[16], idx;
	Seek_InputStream( in, 0 );
	ReadSegment( in, (uint8 *)&aBuffer[0], 16 );
	idx = 0;
	while ( (aBuffer[idx] != '%') && (idx < 13))
		idx++;
	isMe = (char)(((aBuffer[idx] == '%') && (aBuffer[idx+1] == '!')));
	Seek_InputStream( in, 0 );
	return isMe;
}
#endif

#ifdef ENABLE_CFF
static char IsCFF(InputStream *in)
{
char isMe;
uint8 aBuffer[10];
	Seek_InputStream( in, 0 );
	ReadSegment( in, (uint8 *)&aBuffer[0], 10 );
	isMe = (char)(((aBuffer[0] == (uint8)0x01)));
	Seek_InputStream( in, 0 );
	return isMe;
}
#endif

#ifdef ENABLE_SPD
static char IsSPD(InputStream *in)
{
char isMe = false;
uint32 tmpuint32;
	Seek_InputStream( in, FH_FMVER + 4 );
	tmpuint32 = (uint32)ReadInt32(in);
	if (tmpuint32 == 0x0d0a0000)
		isMe = true;
	Seek_InputStream( in, 0 );
	return isMe;
}
#endif

/*********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                            *
 *     $Header: R:/src/FontFusion/Source/Core/rcs/sniffer.c 1.19 2001/05/03 16:04:19 reggers Exp $
 *                                                                            *
 *     $Log: sniffer.c $
 *     Revision 1.19  2001/05/03 16:04:19  reggers
 *     Warning repair.
 *     Revision 1.18  2000/10/18 20:59:20  reggers
 *     Broke out internal ff_FontTypeFromStream() that does not call setjmp(),
 *     so can be called from Font Manager, also from public FF_FontTypeFromStream().
 *     Revision 1.17  2000/10/18 18:06:59  reggers
 *     Added a return after the emergency shutdown.
 *     Revision 1.16  2000/10/18 17:42:24  reggers
 *     Added error handling to FF_FontTypeFromStream()
 *     Revision 1.15  2000/07/11 17:32:10  reggers
 *     Borland STRICK warning fixes
 *     Revision 1.14  2000/06/16 15:26:39  reggers
 *     Fixed warnings for Borland STRICT
 *     Revision 1.13  2000/03/13 15:15:27  reggers
 *     Add support for Font Bureau fonts in IsTT(): check for 'true'
 *     Revision 1.12  2000/02/25 17:45:56  reggers
 *     STRICT warning cleanup.
 *     Revision 1.11  2000/02/18 18:56:11  reggers
 *     Added Speedo processor capability.
 *     Revision 1.10  2000/01/14 18:20:09  reggers
 *     Added PCLETTO sniffing capability
 Revision 1.9  1999/12/23 22:02:58  reggers
 New ENABLE_PCL branches. Rename any 'code' and 'data' symbols.
 *     Revision 1.8  1999/12/08 18:11:01  reggers
 *     Allow Opentype fonts in isTT(), checking for 'OTTO' at head.
 *     Revision 1.7  1999/10/18 16:57:24  jfatal
 *     Changed all include file names to lower case.
 *     Revision 1.6  1999/10/14 15:51:58  jfatal
 *     Added Blank line at end of file to satisfy Unix Compiler
 *     Revision 1.5  1999/09/30 15:10:32  jfatal
 *     Added correct Copyright notice.
 *                                                                            *
 ******************************************************************************/

