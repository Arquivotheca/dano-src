/*****************************************************************************/
/*
**	X_EditorTools.h
**
**	Tools for editors create and manipulating RMF data
**
**	\xA9 Copyright 1998-1999 Beatnik, Inc, All Rights Reserved.
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
**	12/16/98	Created. Pulled from MacOS specific editor codebase
**	2/5/98		Added XCopySongMidiResources & XCopyInstrumentResources & XCopySndResources
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
**	3/16/99		MOE:  Changed parameters of XCompressAndEncrypt()
**	3/25/99		MOE:  Added procData parameter to functions using XCompressStatusProc
**	5/15/99		Added XRemoveThisKeySplit
*/
/*****************************************************************************/
#ifndef X_EDITOR_TOOLS
#define X_EDITOR_TOOLS

#ifndef __X_API__
	#include "X_API.h"
#endif

#ifndef X_FORMATS
	#include "X_Formats.h"
#endif

#ifndef G_SOUND
	#include "GenSnd.h"
	#include "GenPriv.h"
#endif

#ifdef __cplusplus
	extern "C" {
#endif


// Utillities for instruments
XBOOL XIsSoundUsedInInstrument(InstrumentResource *theX, XShortResourceID sampleSoundID);
void XRenumberSampleInsideInstrument(InstrumentResource *theX, XShortResourceID originalSampleID, 
																XShortResourceID newSampleID);
short int XCollectSoundsFromInstrument(InstrumentResource *theX, XShortResourceID *sndArray, short maxArraySize);
short int XCollectSoundsFromInstrumentID(XShortResourceID theID, XShortResourceID *sndArray, short maxArraySize);
// Verifiy each keysplit sample by accessing it through the currently open resource file chain, then
// remove sample references that are bad. This will return a new instrument.
InstrumentResource * XRemoveUnusedSampleKeysplitFromInstrument(InstrumentResource *theX, long instrumentSize);
XBOOL XCheckAllInstruments(XShortResourceID *badInstrument, XShortResourceID *badSnd);
XShortResourceID XCheckValidInstrument(XShortResourceID theID);
short int XGetInstrumentArray(XShortResourceID *instArray, short maxArraySize);

short int XGatherAllSoundsFromAllInstruments(XShortResourceID *pSndArray, short int maxArraySize);

XBOOL XIsSampleUsedInAllInstruments(XShortResourceID soundSampleID, XShortResourceID *pWhichInstrument);

short int XGetTotalKeysplits(XShortResourceID *instArray, short int totalInstruments, 
								XShortResourceID *sndArray, short int totalSnds);

// Given a song ID and two arrays, this will return the INST resources ID and the 'snd ' resource ID
// that are needed to load the song terminated with a -1.
// Will return 0 for success or 1 for failure
OPErr XGetSongInstrumentList(XShortResourceID theSongID, XShortResourceID *pInstArray, short int maxInstArraySize, 
										XShortResourceID *pSndArray, short int maxSndArraySize);

short int XGetSamplesFromInstruments(XShortResourceID *pInstArray, short int maxInstArraySize, 
										XShortResourceID *pSndArray, short int maxSndArraySize);

void XSetKeySplitFromPtr(InstrumentResource *theX, short int entry, KeySplit *keysplit);

InstrumentResource * XAddKeySplit(InstrumentResource *theX, short int howMany);
InstrumentResource * XRemoveKeySplit(InstrumentResource *theX, short int howMany);
// Remove specific key split.
InstrumentResource * XRemoveThisKeySplit(InstrumentResource *theX, short int entry);

// returns >0 if successful, 0 if aborted, -1 if failed
long XCompressAndEncrypt(XPTR* newData, XPTR pData, unsigned long size,
							XCompressStatusProc proc, void* procData);

long XGetSongTempoFactor(SongResource *pSong);
void XSetSongTempoFactor(SongResource *pSong, long newTempo);

// allocate and return an list of ID's collected from ID_SND, ID_CSND, ID_ESND. pCount will
// be the number of ID's, and the long array will be the list. use XDisposePtr on the return
// pointer
XLongResourceID * XGetAllSoundID(long *pCount);

// This will return a MIDI/CMID/EMID/ECMI object from an open resource file
//
// INPUT:
//	theXSong		is the SongResource structure
//
// OUTPUT:
//	pMusicName		is a pascal string
//	pMusicType		is the resource type
//	pMusicID		is the resource ID
//	pReturnedSize			is the resource size
XPTR XGetMusicObjectFromSong(SongResource *theXSong, char *pMusicName, 
								XResourceType *pMusicType, XLongResourceID *pMusicID, long *pReturnedSize);

XERR XCopySongMidiResources(XLongResourceID theSongID, XFILE readFileRef, 
								XFILE writeFileRef, XBOOL protect, XBOOL copyNames);
XERR XCopyInstrumentResources(XShortResourceID *pInstCopy, short int instCount, 
										XFILE readFileRef, XFILE writeFileRef, XBOOL copyNames);
XERR XCopySndResources(XShortResourceID *pSndCopy, short int sndCount, XFILE readFileRef, 
									XFILE writeFileRef, XBOOL protect, XBOOL copyNames);


// Test API's
void XTestCompression(XPTR compressedAndEncryptedData, long size, XPTR originalData, long originalSize);

#ifdef __cplusplus
	}
#endif


#endif	// X_EDITOR_TOOLS
// EOF of X_EditorTools.h


