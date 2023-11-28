/******************************************************************************
/
/	File:			UTF8.h
/
/	Description:	UTF-8 conversion functions.
/
/	Copyright 1993-98, Be Incorporated
/
******************************************************************************/


#ifndef _TEXTENCODING2_H
#define _TEXTENCODING2_H

#include <be_prim.h>

// This should probably go somewhere else, but we need it for now.
// Use to be in <InterfaceDefs.h>
#define	B_SUBSTITUTE	(0x1a)

// More stuff from <InterfaceDefs.h>.  This might be a reasonable
// place for it, or maybe in a separate "UTF8" file in the support kit.
#define B_UTF8_ELLIPSIS		"\xE2\x80\xA6"
#define B_UTF8_OPEN_QUOTE	"\xE2\x80\x9C"
#define B_UTF8_CLOSE_QUOTE	"\xE2\x80\x9D"
#define B_UTF8_COPYRIGHT	"\xC2\xA9"
#define B_UTF8_REGISTERED	"\xC2\xAE"
#define B_UTF8_TRADEMARK	"\xE2\x84\xA2"
#define B_UTF8_SMILING_FACE	"\xE2\x98\xBB"
#define B_UTF8_HIROSHI		"\xE5\xBC\x98"

/*------------------------------------------------------------*/
/*------- Conversion Flavors ---------------------------------*/
enum {
	B_ISO1_CONVERSION				=	0,				/* ISO 8859-1 */
	B_ISO2_CONVERSION				= 	1,				/* ISO 8859-2 */
	B_ISO3_CONVERSION				= 	2,				/* ISO 8859-3 */
	B_ISO4_CONVERSION				= 	3,				/* ISO 8859-4 */
	B_ISO5_CONVERSION				= 	4,				/* ISO 8859-5 */
	B_ISO6_CONVERSION				= 	5,				/* ISO 8859-6 */
	B_ISO7_CONVERSION				= 	6,				/* ISO 8859-7 */
	B_ISO8_CONVERSION				=	7,				/* ISO 8859-8 */
	B_ISO9_CONVERSION				=	8,				/* ISO 8859-9 */
	B_ISO10_CONVERSION				=	9,				/* ISO 8859-10 */
	B_ISO13_CONVERSION				=	21,				/* ISO 8859-13 */
	B_ISO14_CONVERSION				=	22,				/* ISO 8859-14 */
	B_ISO15_CONVERSION				=	23,				/* ISO 8859-15 */

	B_UNICODE_CONVERSION			=	15,				/* Unicode 2.0 */

	B_SJIS_CONVERSION				=	11,				/* Shift-JIS */
	B_EUC_CONVERSION				=	12,				/* EUC Packed Japanese */
	B_JIS_CONVERSION				=	13,				/* JIS X 0208-1990 */
	B_KOI8R_CONVERSION				=	16,				/* KOI8-R */
	B_EUC_KR_CONVERSION				=	20,				/* EUC Korean */
	B_BIG5_CONVERSION				=	24,				/* Big5 */
	B_GBK_CONVERSION				=	25,				/* GBK */
	
	B_MS_WINDOWS_CONVERSION			=	14,				/* MS-Windows Codepage 1252 */
	B_MS_WINDOWS_1250_CONVERSION	=	26,				/* MS-Windows Codepage 1250 */
	B_MS_WINDOWS_1251_CONVERSION	=	17,				/* MS-Windows Codepage 1251 */
	B_MS_WINDOWS_1252_CONVERSION	=	B_MS_WINDOWS_CONVERSION,
	B_MS_WINDOWS_1253_CONVERSION	=	27,				/* MS-Windows Codepage 1253 */
	B_MS_WINDOWS_1254_CONVERSION	=	28,				/* MS-Windows Codepage 1254 */
	B_MS_WINDOWS_1255_CONVERSION	=	29,				/* MS-Windows Codepage 1255 */
	B_MS_WINDOWS_1256_CONVERSION	=	30,				/* MS-Windows Codepage 1256 */
	B_MS_WINDOWS_1257_CONVERSION	=	31,				/* MS-Windows Codepage 1257 */
	B_MS_WINDOWS_1258_CONVERSION	=	32,				/* MS-Windows Codepage 1258 */

	B_MS_DOS_CONVERSION				=	19,				/* MS-DOS Codepage 437 */
	B_MS_DOS_437_CONVERSION		 	=	B_MS_DOS_CONVERSION,
	B_MS_DOS_737_CONVERSION			=	33,				/* MS-DOS Codepage 737 (Greek) */
	B_MS_DOS_775_CONVERSION			=	34,				/* MS-DOS Codepage 775 (Baltic Rim) */
	B_MS_DOS_850_CONVERSION			=	35,				/* MS-DOS Codepage 850 (Latin 1) */
	B_MS_DOS_852_CONVERSION			=	36,				/* MS-DOS Codepage 852 (Latin 2) */
	B_MS_DOS_855_CONVERSION			=	37,				/* MS-DOS Codepage 855 (Cyrillic) */
	B_MS_DOS_857_CONVERSION			=	38,				/* MS-DOS Codepage 857 (Turkish) */
	B_MS_DOS_860_CONVERSION			=	39,				/* MS-DOS Codepage 860 (Portugese) */
	B_MS_DOS_861_CONVERSION			=	40,				/* MS-DOS Codepage 861 (Icelandic) */
	B_MS_DOS_862_CONVERSION			=	41,				/* MS-DOS Codepage 862 (Hebrew) */
	B_MS_DOS_863_CONVERSION			=	42,				/* MS-DOS Codepage 863 (French Canada) */
	B_MS_DOS_864_CONVERSION			=	43,				/* MS-DOS Codepage 864 (Arabic) */
	B_MS_DOS_865_CONVERSION			=	44,				/* MS-DOS Codepage 865 (Nordic) */
	B_MS_DOS_866_CONVERSION			=	18,				/* MS-DOS Codepage 866 */
	B_MS_DOS_869_CONVERSION			=	45,				/* MS-DOS Codepage 869 (Greek2) */
	B_MS_DOS_874_CONVERSION			=	46,				/* MS-DOS Codepage 874 */

	B_MAC_ROMAN_CONVERSION			=	10,				/* Macintosh Roman */
	B_MAC_CENTEURO_CONVERSION		=	47,				/* Macintosh Central Europe */
	B_MAC_CROATIAN_CONVERSION		=	48,				/* Macintosh Croatian */
	B_MAC_CYRILLIC_CONVERSION		=	49,				/* Macintosh Cyrillic */
	B_MAC_GREEK_CONVERSION			=	50,				/* Macintosh Greek */
	B_MAC_HEBREW_CONVERSION			=	51,				/* Macintosh Hebrew */
	B_MAC_ICELAND_CONVERSION		=	52,				/* Macintosh Iceland */
	B_MAC_TURKISH_CONVERSION		=	53,				/* Macintosh Turkish */
};

/*-------------------------------------------------------------*/
/*------- Conversion Functions --------------------------------*/

status_t convert_to_utf8(uint32		srcEncoding, 
						 					  const char	*src, 
						 					  int32			*srcLen, 
											  char			*dst, 
						 					  int32			*dstLen,
											  int32			*state,
											  char			substitute = B_SUBSTITUTE);

status_t convert_from_utf8(uint32		dstEncoding,
												const char	*src, 
												int32		*srcLen, 
						  						char		*dst, 
						   						int32		*dstLen,
												int32		*state,
												char		substitute = B_SUBSTITUTE);

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif	/* _TEXTENCODING2_H */
