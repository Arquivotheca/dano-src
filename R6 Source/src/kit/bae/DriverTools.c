/*****************************************************/
/*
**	DriverTools.c
**
**	Instrument and Song resource tools for the SoundMusicSys driver.
**	All functions here at platform independent
**
**	\xA9 Copyright 1989-1999 Beatnik, Inc, All Rights Reserved.
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
**	1/31/93		Created
**	6/23/95		Integrated into SoundMusicSys Library
**	11/20/95	Removed pragma unused
**	12/11/95	Removed GetInstrumentEnvelopeData
**	12/15/95	Changed memory allocation to X_API
**				Pulled out most Mac specific code
**	1/1/96		Forced more defaults for NewSong
**	1/11/96		Forced more defaults for NewInstrument
**	1/18/96		Spruced up for C++ extra error checking
**	1/28/96		Added GetKeySplitFromPtr
**				Added NewSongPtr & DisposeSongPtr
**	2/3/96		Removed extra includes
**	2/12/96		Changed Boolean to SMSBoolean
**	2/17/96		Added more platform compile arounds
**	2/19/96		Changed NewSongPtr
**	4/21/96		Removed CPU edian issues by use XGetShort & XGetLong
**	4/29/96		Missed an edian issue in GetKeySplitFromPtr
**	5/15/96		Added CreateSongResource for use with BeginSongFromMemory
**	5/30/96		Added XBF_ignoreBadPatches to NewSong functions
**				Fixed bug with CreateSongResource. Bad data translation
**	6/30/96		Changed font and re tabbed
**				Removed Byte reference from GetKeySplitFromPtr
**	10/23/96	Removed all platform specific code
**				Changed GetKeySplitFromPtr to XGetKeySplitFromPtr
**	12/30/96	Changed copyright
**	1/3/97		Added XGetSongInformation
**	1/6/97		Added songType to XNewSongPtr
**	1/12/97		Added I_TITLE to XGetSongInformation for SMS type
**	1/13/97		Added XGetSongResourceInfo & XDisposeSongResourceInfo & 
**				XGetSongResourceObjectID & XGetSongPerformanceSettings &
**				XGetSongResourceObjectType
**	1/15/97		Fixed a bug with XNewSongPtr. Forgot to change song types
**	1/16/97		Added XGetSongInformationSize
**	1/17/97		Totally changed the SONG RMF resource type, much more resource based
**	1/18/97		Added XCheckAllInstruments & XCheckValidInstrument
**	1/29/97		Added XCollectSoundsFromInstrumentID
**				Added XGetMidiData
**				Added XSetSongLocked
**				Added XGetSongInstrumentList
**				Added XCompressAndEncrypt
**	1/30/97		Added XGetSoundResourceByName & XGetSoundResourceByID
**	2/13/97		Fixed platform bug when reading resourceCount out of the RMF structure
**				Added XSetSongVolumeGain & XGetSongVolumeGain
**	2/15/97		Changed volumeGain to songVolume, changed XSetSongVolumeGain & 
**				XGetSongVolumeGain to XSetSongVolume & XGetSongVolume
**	2/25/97		Changed more RMF feilds. Hopefully it'll be the last for awhile
**	3/12/97		Changed XNewSongPtr to only use a valid ReverbMode type
**	3/14/97		Added XIsSoundUsedInInstrument & XRenumberSampleInsideInstrument
**				Fixed a bug with XCollectSoundsFromInstrument that disposed of
**				the instrument passed into it, corrupting the heap
**	3/15/97		Found a bug in XSetSongLocked that would trash the heap if you
**				tried to lock a song that was already locked
**	3/17/97		Fixed bug with XAddKeySplit
**	3/29/97		Fixed bug with XRemoveKeySplit that trashed the heap
**	4/1/97		Fixed a bug with encrypted samples
**				Fixed a bug with XNewSongFromSongResourceInfo & XGetSongResourceInfo
**				that did not preserve the volume
**	5/3/97		Fixed a few potential problems that the MOT compiler found
**	6/28/97		Fixed a potential bug with XGetSoundResourceByName and did not copy
**				the data before decrypting
**	8/7/97		Changed XGetSongInformation to strip control characters before
**				return textual information via XDuplicateAndStripStr
**	8/12/97		Fixed XRenumberSampleInsideInstrument to save changes to renumbered
**				instruments and fixed platform dependancy
**	8/16/97		Added ifdefs (USE_CREATION_API) around functions that are used in
**				the process of creation of RMF files, XFILES, etc.
**	8/19/97		Removed XDuplicateAndStripStr from XGetSongInformation. Now relying
**				on editor not to put bad characters into data stream
**	9/30/97		Added BankStatus structure, added XCreateBankStatus & XGetBankStatus
**	10/2/97		Changed the search order in XGetMidiData, also fixed in XGetMidiData
**				a bug in which encrypted data was not copied first.
**	12/16/97	Moe: removed compiler warnings
**	1/22/98		Modifed XGetMidiData to return type of compression, if any
**	2/11/98		Put more code wrappers around code not needed for WebTV
**	2/16/98		Moved XNewInstrument & XDisposeInstrument to X_Instruments.c
**	2/18/98		Fixed XCreateBankStatus
**	4/27/98		Changed XCompressAndEncrypt to support new compression types
**	4/27/98		MOE: Changed calls to XDecompressPtr()
**	4/28/98		Changed XSetSongVolume. Removed old test for MAX_SONG_VOLUME
**	4/30/98		Added support for I_GENRE & I_SUB_GENRE
**				Removed code wrappers around XSetSongVolume to allow for API
**				use.
**	5/4/98		Removed from XNewSongPtr extra non-used bit types
**	5/11/98		Added XGetAllSoundID
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
**	7/1/98		Changed various API to use the new XResourceType and XLongResourceID
**	7/6/98		Fixed type problems with XGetMidiData
**				Fixed more type problems with XPutShort
**	7/7/98		Fixed -1 type warning
**				Fixed XGetSoundResourceByID to use new XLongResourceID
**	7/15/98		Added XGetMusicObjectFromSong
**	10/20/98	Changed SONG_TYPE_RMF_MOD to SONG_TYPE_RMF_LINEAR
**	11/5/98		Added changes to support new SONG_TYPE_RMF_LINEAR
**	12/17/98	Moved editor specific tools into X_EditorTools.c
**	1/29/99		Added XGetVersionNumber & XCreateVersion
**	2/21/99		Fixed a bug in XGetSoundResourceByID & XGetSoundResourceByName in which a 
**				memory failure would cause a decryption to happen on bad memory.
**	3/24/99		Added R_TEMPO & R_ORIGINAL_SOURCE rmf sub types
**	3/24/99		Added R_TEMPO & R_ORIGINAL_SOURCE rmf sub resource types
**				Added I_TEMPO & I_ORIGINAL_SOURCE rmf info types
**	5/15/99		Added XGetRawSoundResourceByID
**	8/11/99		Fixed bug with XNewSongFromSongResourceInfo inwhich a memory is deallocated
**				twice.
*/
/*****************************************************************************/
#include "X_API.h"
#include "X_Formats.h"
#include "GenPriv.h"
#include "GenSnd.h"



// Create a new song resource.
SongResource * XNewSongPtr(	SongType songType,
							XShortResourceID midiID,
							short int maxSongVoices, 
							short int mixLevel, 
							short int maxEffectVoices,
							ReverbMode reverbType)
{
	SongResource			*song;
	SongResource_SMS		*songSMS;
	SongResource_RMF		*songRMF;
	SongResource_RMF_Linear	*songRMF2;

	song = NULL;
	switch (songType)
	{
		case SONG_TYPE_SMS:
			songSMS = (SongResource_SMS *)XNewPtr((long)sizeof(SongResource_SMS));
			if (songSMS)
			{
				songSMS->songType = SONG_TYPE_SMS;
				XPutShort(&songSMS->midiResourceID, (unsigned short)midiID);
				songSMS->noteDecay = 5;
				songSMS->maxNotes = (char)maxSongVoices;
				XPutShort(&songSMS->mixLevel, (unsigned short)mixLevel);
				songSMS->maxEffects = (char)maxEffectVoices;
				songSMS->reverbType = reverbType;
				songSMS->flags1 = XBF_enableMIDIProgram;
				songSMS->flags2 = XBF_ignoreBadPatches;
				songSMS->defaultPercusionProgram = (XShortResourceID)-1;		// 126
//				XPutShort(&songSMS->remapCount, 0);
			}
			song = (SongResource *)songSMS;
			break;
		case SONG_TYPE_RMF:
			songRMF = (SongResource_RMF *)XNewPtr((long)sizeof(SongResource_RMF) - sizeof(short));
			if (songRMF)
			{
				songRMF->songType = SONG_TYPE_RMF;
				XPutShort(&songRMF->rmfResourceID, (unsigned short)midiID);
				XPutShort(&songRMF->maxNotes, (unsigned short)maxSongVoices);
				XPutShort(&songRMF->mixLevel, (unsigned short)mixLevel);
				XPutShort(&songRMF->maxEffects, (unsigned short)maxEffectVoices);

				songRMF->reverbType = reverbType;
//				XPutShort(&songRMF->resourceCount, 0);		// no resources
			}
			song = (SongResource *)songRMF;
			break;
		case SONG_TYPE_RMF_LINEAR:
			songRMF2 = (SongResource_RMF_Linear *)XNewPtr((long)sizeof(SongResource_RMF_Linear) - sizeof(short));
			if (songRMF2)
			{
				songRMF2->songType = SONG_TYPE_RMF_LINEAR;
				XPutShort(&songRMF2->audioResourceID, (unsigned short)midiID);
				XPutShort(&songRMF2->maxNotes, (unsigned short)maxSongVoices);
				XPutShort(&songRMF2->mixLevel, (unsigned short)mixLevel);
				XPutShort(&songRMF2->maxEffects, (unsigned short)maxEffectVoices);

				songRMF2->reverbType = reverbType;
//				XPutShort(&songRMF2->resourceCount, 0);		// no resources
			}
			song = (SongResource *)songRMF2;
			break;
	}
	return song;
}

void XDisposeSongPtr(SongResource *theSong)
{
	XDisposePtr((XPTR)theSong);
}


// Get a keysplit entry. The result will be ordered for the native CPU
void XGetKeySplitFromPtr(InstrumentResource *theX, short int entry, KeySplit *keysplit)
{
	KeySplit	*pSplits;
	short int	count;

	count = (short)XGetShort(&theX->keySplitCount);
	if ( (count) && (entry < count) )
	{
		pSplits = (KeySplit *) ( ((unsigned char *)&theX->keySplitCount) + sizeof(short int));
		*keysplit = pSplits[entry];
		keysplit->sndResourceID = (XShortResourceID)XGetShort(&keysplit->sndResourceID);
		keysplit->miscParameter1 = (short)XGetShort(&keysplit->miscParameter1);
		keysplit->miscParameter2 = (short)XGetShort(&keysplit->miscParameter2);
	}
	else
	{
		XSetMemory((void *)keysplit, (long)sizeof(KeySplit), 0);
	}
}



// Find a resource type and return a pointer to its memory block. If type is R_LAST_RESOURCE, then return
// a pointer to the very last memory block
#if USE_FULL_RMF_SUPPORT == TRUE
static void * PV_FindSongResourceTypePointer(SongResource_RMF *songRMF, SongResourceType findType, long *pResourceLength)
{
	char				*pUnit;
	short int			count, subCount;
	SongResourceType	type;
	unsigned long		length;
	short int			resourceCount;
	void				*pEnd;

	pEnd = NULL;
	if (songRMF)
	{
		resourceCount = (short)XGetShort(&songRMF->resourceCount);
		pUnit = (char *)&songRMF->resourceData;
		for (count = 0; count < resourceCount; count++)
		{
			type = (SongResourceType)XGetLong(pUnit);
			pUnit += sizeof (SongResourceType);
			switch (type)
			{
				default:
					// found a bad type, just stop because we don't know how to proceed
					break;
				case R_PERFORMED_BY:
				case R_TITLE:
				case R_COMPOSER:
				case R_COPYRIGHT_DATE:
				case R_COPYRIGHT_LINE:
				case R_PUBLISHER_CONTACT:
				case R_LICENSED_TO_URL:
				case R_USE_OF_LICENSE:
				case R_LICENSE_TERM:
				case R_EXPIRATION_DATE:
				case R_COMPOSER_NOTES:
				case R_INDEX_NUMBER:
				case R_GENRE:
				case R_SUB_GENRE:
				case R_TEMPO:
				case R_ORIGINAL_SOURCE:
					// this is a zero terminated string, always
					if (songRMF->locked == FALSE)
					{
						length = (unsigned long)XStrLen(pUnit) + 1;
					}
					else
					{
						length = (unsigned long)XEncryptedStrLen(pUnit) + 1;
					}
					pUnit += length;
					if (pResourceLength)
					{
						*pResourceLength = (long)length;
					}
					break;
				case R_VELOCITY_CURVE:
					pUnit += 256;
					if (pResourceLength)
					{
						*pResourceLength = 256;
					}
					break;
				case R_INSTRUMENT_REMAP:
					subCount = (short)XGetShort(pUnit);
					pUnit += 2;
					// format is word count followed by x number of (word, word)
					length = sizeof(short) * subCount;
					pUnit += length;
					if (pResourceLength)
					{
						*pResourceLength = (long)length;
					}
					break;
			}
			if (type == findType)
			{
				break;
			}
		}
		if (findType == R_LAST_RESOURCE)
		{
			pEnd = pUnit;
		}
	}
	return pEnd;
}
#endif	//USE_FULL_RMF_SUPPORT == TRUE

#if USE_FULL_RMF_SUPPORT == TRUE
// Given a song resource type, and the pointer to the data block; this will put the resource
// into the given pointer if found.
//
// pFillData or pFillSize can be NULL. If not NULL, then data will be placed there
static void PV_FillSongResource(SongResourceType resourceType, short int resourceCount, XBOOL encrypted, 
								void *pResourceData, unsigned long resourceDataLength, 
								void *pFillData, unsigned long *pFillSize)
{
	char				*pUnit;
	short int			count, subCount;
	SongResourceType	type;
	XBOOL				fill;
	unsigned long		length;

	if (pResourceData && resourceCount && resourceDataLength)
	{
		pUnit = (char *)pResourceData;
		for (count = 0; count < resourceCount; count++)
		{
			type = (SongResourceType)XGetLong(pUnit);
			pUnit += sizeof (SongResourceType);

			fill = (XBOOL)((resourceType == type) ? TRUE : FALSE);
			switch (type)
			{
				default:
					// found a bad type, just stop because we don't know how to proceed
					break;
				case R_TITLE:
				case R_PERFORMED_BY:
				case R_COMPOSER:
				case R_COPYRIGHT_DATE:
				case R_COPYRIGHT_LINE:
				case R_PUBLISHER_CONTACT:
				case R_LICENSED_TO_URL:
				case R_USE_OF_LICENSE:
				case R_LICENSE_TERM:
				case R_EXPIRATION_DATE:
				case R_COMPOSER_NOTES:
				case R_INDEX_NUMBER:
				case R_GENRE:
				case R_SUB_GENRE:
				case R_TEMPO:
				case R_ORIGINAL_SOURCE:
					// this is a zero terminated string, always
					if (encrypted == FALSE)
					{
						length = (unsigned long)XStrLen(pUnit) + 1;
					}
					else
					{
						length = (unsigned long)XEncryptedStrLen(pUnit) + 1;
					}
					if (fill)
					{
						if (pFillData)
						{
							XBlockMove(pUnit, pFillData, (long)length);
							if (encrypted)
							{
								XDecryptData(pFillData, (unsigned long)length);
							}
						}
						if (pFillSize)
						{
							*pFillSize = length;
						}
					}
					pUnit += length;
					break;
				case R_VELOCITY_CURVE:
					if (fill)
					{
						XBlockMove(pUnit, pFillData, 256L);
					}
					pUnit += 256;
					break;
				case R_INSTRUMENT_REMAP:
					subCount = (short)XGetShort(pUnit);
					pUnit += 2;
					// format is word count followed by x number of (word, word)
					pUnit += sizeof(short) * subCount;
					break;
			}
			if (fill)
			{
				break;
			}
		}
	}
}
#endif	//USE_FULL_RMF_SUPPORT == TRUE

#if USE_CREATION_API == TRUE
static XBOOL PV_ValidResourceForSongType(SongResourceType resourceType, SongType type)
{
	XBOOL	valid;

	valid = FALSE;
	switch (type)
	{
		case SONG_TYPE_SMS:
			switch (resourceType)
			{
				case R_TITLE:
				case R_COMPOSER:
				case R_COPYRIGHT_DATE:
				case R_PUBLISHER_CONTACT:
				case R_INSTRUMENT_REMAP:
					valid = TRUE;
					break;
			}
			break;
		case SONG_TYPE_RMF:
			switch (resourceType)
			{
				case R_TITLE:
				case R_PERFORMED_BY:
				case R_COMPOSER:
				case R_COPYRIGHT_DATE:
				case R_COPYRIGHT_LINE:
				case R_PUBLISHER_CONTACT:
				case R_LICENSED_TO_URL:
				case R_USE_OF_LICENSE:
				case R_LICENSE_TERM:
				case R_EXPIRATION_DATE:
				case R_COMPOSER_NOTES:
				case R_INDEX_NUMBER:
				case R_GENRE:
				case R_SUB_GENRE:
				case R_TEMPO:
				case R_ORIGINAL_SOURCE:
				case R_VELOCITY_CURVE:
				case R_INSTRUMENT_REMAP:
					valid = TRUE;
					break;
			}
			break;
		case SONG_TYPE_RMF_LINEAR:
			switch (resourceType)
			{
				case R_TITLE:
				case R_PERFORMED_BY:
				case R_COMPOSER:
				case R_COPYRIGHT_DATE:
				case R_COPYRIGHT_LINE:
				case R_PUBLISHER_CONTACT:
				case R_LICENSED_TO_URL:
				case R_USE_OF_LICENSE:
				case R_LICENSE_TERM:
				case R_EXPIRATION_DATE:
				case R_COMPOSER_NOTES:
				case R_INDEX_NUMBER:
				case R_GENRE:
				case R_SUB_GENRE:
				case R_TEMPO:
				case R_ORIGINAL_SOURCE:
					valid = TRUE;
					break;
			}
			break;
	}
	return valid;
}
#endif	//USE_CREATION_API == TRUE

#if USE_CREATION_API == TRUE
static SongResource_RMF * PV_AddRMFSongResource(SongResource *theSong, SongResourceType resourceType, void *pResource, long resourceLength)
{
	SongResource_RMF	*newSong;
	long				size;
	char				*pBlock;

	newSong = NULL;
	if (theSong && pResource && resourceLength)
	{
		if (PV_ValidResourceForSongType(resourceType, SONG_TYPE_RMF))
		{
			size = XGetPtrSize(theSong);
			newSong = (SongResource_RMF *)XNewPtr((long)(size + sizeof(SongResourceType) + resourceLength));
			if (newSong)
			{
				XBlockMove(theSong, newSong, size);
				pBlock = (char *)PV_FindSongResourceTypePointer(newSong, R_LAST_RESOURCE, NULL);
				if (pBlock)
				{
					size = XGetShort(&newSong->resourceCount) + 1;
					XPutShort(&newSong->resourceCount, (unsigned short)size);
					XPutLong(pBlock, (unsigned long)resourceType);
					pBlock += 4;
					XBlockMove(pResource, pBlock, resourceLength);
				}
				else
				{
					XDisposePtr(newSong);
					newSong = NULL;
				}
			}

		}
	}
	return newSong;
}

static SongResource_RMF * PV_DeleteRMFSongResource(SongResource *theSong, SongResourceType resourceType)
{
	SongResource_RMF	*newSong;
	long				size, resourceLength, offsetStart, offsetEnd;
	char				*pBlock;

	newSong = NULL;
	if (theSong)
	{
		if (PV_ValidResourceForSongType(resourceType, SONG_TYPE_RMF))
		{
			pBlock = (char *)PV_FindSongResourceTypePointer((SongResource_RMF *)theSong, resourceType, &resourceLength);
			if (pBlock)
			{
				size = (long)(XGetPtrSize(theSong) - sizeof(SongResourceType) - resourceLength);
				if (size > 0)
				{
					newSong = (SongResource_RMF *)XNewPtr(size);
					if (newSong)
					{
						offsetStart = pBlock - (char *)theSong;
						offsetEnd = offsetStart + resourceLength + sizeof(SongResourceType);
						XBlockMove(theSong, newSong, offsetStart);
						XBlockMove(((char *)theSong) + offsetEnd, ((char *)newSong) + offsetStart, size - offsetEnd);

						size = (long)XGetShort(&newSong->resourceCount) - 1;
						XPutShort(&newSong->resourceCount, (unsigned short)size);
					}
				}
			}
		}
	}
	return newSong;
}


// Given a SongResource and a new type, data and length, this will create a new SongResource
// with the modified resource and return it.
// The newly returned SongResource is allocated, and the old one passed in is not deallocated.
SongResource * XChangeSongResource(SongResource *theSong, long songSize, SongResourceType resourceType, void *pResource, long resourceLength)
{
	SongResource_SMS	*songSMS;
	SongResource_RMF	*songRMF;
	SongResource		*newSong;
	char				name1[256];
	char				name2[256];
	char				name3[256];
	char				name4[256];
	SongType			songType;
	char				*pData;

	newSong = NULL;
	if (theSong && pResource && resourceLength)
	{
		songType = (SongType)((SongResource_SMS *)theSong)->songType;
		if (PV_ValidResourceForSongType(resourceType, songType))
		{
			switch (songType)
			{
				case SONG_TYPE_SMS:
					XGetSongInformation(theSong, songSize, I_COPYRIGHT, name1);
					XGetSongInformation(theSong, songSize, I_COMPOSER, name2);
					XGetSongInformation(theSong, songSize, I_TITLE, name3);
					XGetSongInformation(theSong, songSize, I_PUBLISHER_CONTACT, name4);
					switch (resourceType)
					{
						case R_COPYRIGHT_DATE:
							XBlockMove(pResource, name1, resourceLength);
							goto changeSMSresource;
						case R_COMPOSER:
							XBlockMove(pResource, name2, resourceLength);
							goto changeSMSresource;
						case R_TITLE:
							XBlockMove(pResource, name3, resourceLength);
							goto changeSMSresource;
						case R_PUBLISHER_CONTACT:
							XBlockMove(pResource, name4, resourceLength);
			changeSMSresource:
							newSong = (SongResource *)XNewPtr(songSize + resourceLength);
							songSMS = (SongResource_SMS *)newSong;
							if (newSong)
							{
								XBlockMove(theSong, newSong, songSize);
								pData = (char *)&songSMS->remaps;
								pData += sizeof(Remap) * songSMS->remapCount;
								// store copyright first
								XCtoPstr(name1);
								XBlockMove(name1, pData, name1[0] + 1);
								pData += name1[0] + 1;
								// store author second
								XCtoPstr(name2);
								XBlockMove(name2, pData, name2[0] + 1);
								pData += name2[0] + 1;
								// store title third
								XCtoPstr(name3);
								XBlockMove(name3, pData, name3[0] + 1);
								pData += name3[0] + 1;
								// store contact fourth
								XCtoPstr(name4);
								XBlockMove(name4, pData, name4[0] + 1);
								pData += name4[0] + 1;
							}
							break;
					}
					break;
				case SONG_TYPE_RMF:
					songRMF = (SongResource_RMF *)theSong;
					newSong = PV_DeleteRMFSongResource(theSong, resourceType);
					if (newSong)
					{
						XDisposeSongPtr(theSong);
						theSong = newSong;
					}
					newSong = PV_AddRMFSongResource(theSong, resourceType, pResource, resourceLength);
					// we don't throw away this because we let the caller do that
					break;
			}
		}
	}
	return newSong;
}
#endif	// USE_CREATION_API == TRUE

#if USE_FULL_RMF_SUPPORT == TRUE
static const SongInfo i_types[] =	{
					I_TITLE, I_PERFORMED_BY, I_COMPOSER, I_COPYRIGHT, I_PUBLISHER_CONTACT,
					I_USE_OF_LICENSE, I_LICENSE_TERM, I_LICENSED_TO_URL, I_EXPIRATION_DATE,
					I_COMPOSER_NOTES, I_INDEX_NUMBER, I_GENRE, I_SUB_GENRE, I_TEMPO, I_ORIGINAL_SOURCE
									};
static const SongResourceType r_types[] =	{
					R_TITLE, R_PERFORMED_BY, R_COMPOSER, R_COPYRIGHT_DATE, R_PUBLISHER_CONTACT,
					R_USE_OF_LICENSE, R_LICENSE_TERM, R_LICENSED_TO_URL, R_EXPIRATION_DATE,
					R_COMPOSER_NOTES, R_INDEX_NUMBER, R_GENRE, R_SUB_GENRE, R_TEMPO, R_ORIGINAL_SOURCE
											};

// Given a SongInfo type, return the equivalent SongResourceType
static SongResourceType PV_TranslateSongInfoIntoSongResourceType(SongInfo type)
{
	short int	count, max;

	max = sizeof(i_types) / sizeof(SongInfo);
	for (count = 0; count < max; count++)
	{
		if (i_types[count] == type)
		{
			return r_types[count];
		}
	}
	return R_LAST_RESOURCE;
}

// Given a SongResourceType type, return the equivalent SongInfo
static SongInfo PV_TranslateSongResourceTypeIntoSongInfo(SongResourceType type)
{
	short int	count, max;

	max = sizeof(r_types) / sizeof(SongResourceType);
	for (count = 0; count < max; count++)
	{
		if (r_types[count] == type)
		{
			return i_types[count];
		}
	}
	return I_INVALID;
}


unsigned long XGetSongInformationSize(SongResource *theSong, long songSize, SongInfo type)
{
	char				*pData, *pName;
	long				length, offset;
	unsigned long		temp;
	SongResourceType	r_temp; 
	SongResource_SMS	*songSMS;
	SongResource_RMF	*songRMF;
	unsigned long		size;

	size = 0;
	if (theSong && songSize)
	{
		switch (((SongResource_SMS *)theSong)->songType)
		{
			case SONG_TYPE_SMS:
				songSMS = (SongResource_SMS *)theSong;
				switch (type)
				{
					case I_COMPOSER:
						// name is a pascal string
						length = songSize;
		
						pData = (char *)&songSMS->remaps;
						pData += sizeof(Remap) * songSMS->remapCount;
						offset = pData - (char *)songSMS;

						// pointing at author text
						if (offset < length)
						{
							temp = (*pData) + 1;		// skip copyright text
							pData = (char *)songSMS;
							offset += temp;
							pName = (char *)pData + offset;
							size = pName[0];
						}
						break;
					case I_COPYRIGHT:
						// name is a pascal string
						length = songSize;
		
						pData = (char *)&songSMS->remaps;
						pData += sizeof(Remap) * songSMS->remapCount;
						offset = pData - (char *)songSMS;

						// pointing at author text
						if (offset < length)
						{
							pData = (char *)songSMS;
							pName = (char *)pData + offset;
							size = pName[0];
						}
						break;
					case I_TITLE:
						// name is a pascal string
						length = songSize;
		
						pData = (char *)&songSMS->remaps;
						pData += sizeof(Remap) * songSMS->remapCount;
						offset = pData - (char *)songSMS;

						// pointing at author text
						if (offset < length)
						{
							temp = (*pData) + 1;		// skip copyright text
							offset += temp;
							pData = (char *)songSMS + offset;

							temp = (*pData) + 1;		// skip author text
							offset += temp;
							pData = (char *)songSMS;

							if (offset < length)
							{
								pName = (char *)pData + offset;
								size = pName[0];
							}
						}
						break;
					case I_PUBLISHER_CONTACT:
						// name is a pascal string
						length = songSize;
		
						pData = (char *)&songSMS->remaps;
						pData += sizeof(Remap) * songSMS->remapCount;
						offset = pData - (char *)songSMS;

						// pointing at author text
						if (offset < length)
						{
							temp = (*pData) + 1;		// skip copyright text
							offset += temp;
							pData = (char *)songSMS + offset;

							temp = (*pData) + 1;		// skip author text
							offset += temp;
							pData = (char *)songSMS + offset;

							temp = (*pData) + 1;		// skip title text
							offset += temp;
							pData = (char *)songSMS;

							if (offset < length)
							{
								pName = (char *)pData + offset;
								size = pName[0];
							}
						}
						break;
				}
				break;
			
			case SONG_TYPE_RMF:
				songRMF = (SongResource_RMF *)theSong;
				r_temp = PV_TranslateSongInfoIntoSongResourceType(type);
				if (r_temp)
				{
					pData = (char *)&songRMF->resourceData;
					length = pData - (char *)songRMF;
					PV_FillSongResource(r_temp, XGetShort(&songRMF->resourceCount), songRMF->locked,
											pData, length, NULL, &size);
				}
				break;
			
			case SONG_TYPE_RMF_LINEAR:
				break;
		}
	}
	return size;
}
#endif	// USE_FULL_RMF_SUPPORT == TRUE

#if USE_FULL_RMF_SUPPORT == TRUE
void XGetSongInformation(SongResource *theSong, long songSize, SongInfo type, char *cName)
{
	char				*pData, *pName;
	long				length, offset;
	unsigned long		temp;
	SongResourceType	r_temp; 
	SongResource_SMS	*songSMS;
	SongResource_RMF	*songRMF;

	if (theSong && cName)
	{
		cName[0] = 0;
		switch (((SongResource_SMS *)theSong)->songType)
		{
			case SONG_TYPE_SMS:
				songSMS = (SongResource_SMS *)theSong;
				switch (type)
				{
					case I_COMPOSER:
						// name is a pascal string
						length = songSize;
		
						pData = (char *)&songSMS->remaps;
						pData += sizeof(Remap) * songSMS->remapCount;
						offset = pData - (char *)songSMS;

						// pointing at author text
						if (offset < length)
						{
							temp = (*pData) + 1;		// skip copyright text
							pData = (char *)songSMS;
							offset += temp;
							pName = (char *)pData + offset;
							XBlockMove(pName + 1, cName, (long)pName[0]);
							cName[pName[0]] = 0;
						}
						break;
					case I_COPYRIGHT:
						// name is a pascal string
						length = songSize;
		
						pData = (char *)&songSMS->remaps;
						pData += sizeof(Remap) * songSMS->remapCount;
						offset = pData - (char *)songSMS;

						// pointing at author text
						if (offset < length)
						{
							pData = (char *)songSMS;
							pName = (char *)pData + offset;
							XBlockMove(pName + 1, cName, (long)pName[0]);
							cName[pName[0]] = 0;
						}
						break;
					case I_TITLE:
						// name is a pascal string
						length = songSize;
		
						pData = (char *)&songSMS->remaps;
						pData += sizeof(Remap) * songSMS->remapCount;
						offset = pData - (char *)songSMS;

						// pointing at author text
						if (offset < length)
						{
							temp = (*pData) + 1;		// skip copyright text
							offset += temp;
							pData = (char *)songSMS + offset;

							temp = (*pData) + 1;		// skip author text
							offset += temp;
							pData = (char *)songSMS;

							if (offset < length)
							{
								pName = (char *)pData + offset;
								XBlockMove(pName + 1, cName, (long)pName[0]);
								cName[pName[0]] = 0;
							}
						}
						break;
					case I_PUBLISHER_CONTACT:
						// name is a pascal string
						length = songSize;
		
						pData = (char *)&songSMS->remaps;
						pData += sizeof(Remap) * songSMS->remapCount;
						offset = pData - (char *)songSMS;

						// pointing at author text
						if (offset < length)
						{
							temp = (*pData) + 1;		// skip copyright text
							offset += temp;
							pData = (char *)songSMS + offset;

							temp = (*pData) + 1;		// skip author text
							offset += temp;
							pData = (char *)songSMS + offset;

							temp = (*pData) + 1;		// skip title text
							offset += temp;
							pData = (char *)songSMS;

							if (offset < length)
							{
								pName = (char *)pData + offset;
								XBlockMove(pName + 1, cName, (long)pName[0]);
								cName[pName[0]] = 0;
							}
						}
						break;
				}
				break;
			
			case SONG_TYPE_RMF:
				songRMF = (SongResource_RMF *)theSong;
				r_temp = PV_TranslateSongInfoIntoSongResourceType(type);
				offset = XGetShort(&songRMF->resourceCount);
				if ( (r_temp) && (offset) )
				{
					pData = (char *)&songRMF->resourceData;
					length = pData - (char *)songRMF;
					PV_FillSongResource(r_temp, (short)offset, songRMF->locked,
											pData, length, cName, NULL);
				}
				break;
			
			case SONG_TYPE_RMF_LINEAR:
				break;
		}

// there should be no bad characters
/*
		// strip out undesirable characters, give them some walking money
		pName = XDuplicateAndStripStr(cName);
		if (pName)
		{
			XStrCpy(cName, pName);	// copy stripped string
			XDisposePtr(pName);
		}
*/
	}
}
#endif	//USE_FULL_RMF_SUPPORT == TRUE


XShortResourceID XGetSongResourceObjectID(SongResource *pSong)
{
	SongResource_SMS		*songSMS;
	SongResource_RMF		*songRMF;
	SongResource_RMF_Linear	*songRMF2;
	XShortResourceID		id;

	id = (XShortResourceID)-1;
	if (pSong)
	{
		switch (((SongResource_SMS *)pSong)->songType)
		{
			case SONG_TYPE_SMS:
				songSMS = (SongResource_SMS *)pSong;
				id = (XShortResourceID)XGetShort(&songSMS->midiResourceID);
				break;

			case SONG_TYPE_RMF:
				songRMF = (SongResource_RMF *)pSong;
				id = (XShortResourceID)XGetShort(&songRMF->rmfResourceID);
				break;
			case SONG_TYPE_RMF_LINEAR:
				songRMF2 = (SongResource_RMF_Linear *)pSong;
				id = (XShortResourceID)XGetShort(&songRMF2->audioResourceID);
				break;

		}
	}
	return id;
}

#if USE_CREATION_API == TRUE
void XSetSongResourceObjectID(SongResource *pSong, XShortResourceID id)
{
	SongResource_SMS		*songSMS;
	SongResource_RMF		*songRMF;
	SongResource_RMF_Linear	*songRMF2;

	if (pSong)
	{
		switch (((SongResource_SMS *)pSong)->songType)
		{
			case SONG_TYPE_SMS:
				songSMS = (SongResource_SMS *)pSong;
				XPutShort(&songSMS->midiResourceID, (unsigned short)id);
				break;

			case SONG_TYPE_RMF:
				songRMF = (SongResource_RMF *)pSong;
				XPutShort(&songRMF->rmfResourceID, (unsigned short)id);
				break;
			case SONG_TYPE_RMF_LINEAR:
				songRMF2 = (SongResource_RMF_Linear *)pSong;
				XPutShort(&songRMF2->audioResourceID, (unsigned short)id);
				break;
		}
	}
}
#endif	// USE_CREATION_API == TRUE

#if X_PLATFORM != X_WEBTV
XBOOL XIsSongCompressed(SongResource *pSong)
{
	XShortResourceID	id;
	XPTR				data;
	long				type;
	XBOOL				compressed;

	compressed = FALSE;
	if (pSong)
	{
		switch (((SongResource_SMS *)pSong)->songType)
		{
			case SONG_TYPE_SMS:
			case SONG_TYPE_RMF:
				id = XGetSongResourceObjectID(pSong);
				data = XGetMidiData(id, NULL, &type);
				XDisposePtr(data);
				switch (type)
				{
					case ID_ECMI:	// compressed and encrypted
					case ID_CMID:	// compressed only
						compressed = TRUE;
						break;
					case ID_EMID:	// encrypted only
					case ID_MIDI:	// NOT compressed or encrypted
						compressed = FALSE;
						break;
				}
				break;

			case SONG_TYPE_RMF_LINEAR:
				break;
		}
	}
	return compressed;
}

XBOOL XIsSongLocked(SongResource *pSong)
{
	XBOOL					locked;
	SongResource_SMS		*songSMS;
	SongResource_RMF		*songRMF;
	SongResource_RMF_Linear	*songRMF2;

	locked = FALSE;
	if (pSong)
	{
		switch (((SongResource_SMS *)pSong)->songType)
		{
			case SONG_TYPE_SMS:
				songSMS = (SongResource_SMS *)pSong;
				locked = TEST_FLAG_VALUE(songSMS->flags1, XBF_locked);
				break;

			case SONG_TYPE_RMF:
				songRMF = (SongResource_RMF *)pSong;
				locked = songRMF->locked;
				break;
			case SONG_TYPE_RMF_LINEAR:
				songRMF2 = (SongResource_RMF_Linear *)pSong;
				locked = songRMF2->locked;
				break;
		}
	}
	return locked;
}
#endif	// X_PLATFORM != X_WEBTV

#if USE_CREATION_API == TRUE
void XSetSongLocked(SongResource *pSong, XBOOL locked)
{
	SongResource_SMS		*songSMS;
	SongResource_RMF		*songRMF;
	SongResource_RMF_Linear	*songRMF2;
	char					*pData;
	unsigned long			length;
	char					*pUnit;
	short int				count, resourceCount;
	SongResourceType		type;


	if (pSong)
	{
		switch (((SongResource_SMS *)pSong)->songType)
		{
			case SONG_TYPE_SMS:
				songSMS = (SongResource_SMS *)pSong;
				songSMS->flags1 = SET_FLAG_VALUE(songSMS->flags1, XBF_locked, locked);
				break;

			case SONG_TYPE_RMF_LINEAR:
				songRMF2 = (SongResource_RMF_Linear *)pSong;
				resourceCount = (short)XGetShort(&songRMF2->resourceCount);
				if (resourceCount)
				{
				}
				break;

			case SONG_TYPE_RMF:
				songRMF = (SongResource_RMF *)pSong;
				
				resourceCount = (short)XGetShort(&songRMF->resourceCount);
				if (resourceCount)
				{
					pData = (char *)&songRMF->resourceData;
					length = (unsigned long)(pData - (char *)songRMF);
					pUnit = pData;
					for (count = 0; count < resourceCount; count++)
					{
						type = (SongResourceType)XGetLong(pUnit);
						pUnit += sizeof (SongResourceType);

						switch (type)
						{
							default:
								// found a bad type, just stop because we don't know how to proceed
								break;
							case R_TITLE:
							case R_PERFORMED_BY:
							case R_COMPOSER:
							case R_COPYRIGHT_DATE:
							case R_COPYRIGHT_LINE:
							case R_PUBLISHER_CONTACT:
							case R_LICENSED_TO_URL:
							case R_USE_OF_LICENSE:
							case R_LICENSE_TERM:
							case R_EXPIRATION_DATE:
							case R_COMPOSER_NOTES:
							case R_INDEX_NUMBER:
							case R_GENRE:
							case R_SUB_GENRE:
							case R_TEMPO:
							case R_ORIGINAL_SOURCE:
								// this is a zero terminated string, always
								if ( (songRMF->locked == FALSE) && locked)
								{
									length = (unsigned long)XStrLen(pUnit) + 1;
									XEncryptData(pUnit, length);
								}
								if (songRMF->locked && (locked == FALSE))
								{
									length = (unsigned long)XEncryptedStrLen(pUnit) + 1;
									XDecryptData(pUnit, length);
								}
								if (songRMF->locked && (locked))
								{
									length = (unsigned long)XEncryptedStrLen(pUnit) + 1;
								}
								if ((songRMF->locked == FALSE) && (locked == FALSE))
								{
									length = (unsigned long)XStrLen(pUnit) + 1;
								}
								pUnit += length;
								break;
							case R_VELOCITY_CURVE:
								pUnit += 256;
								break;
							case R_INSTRUMENT_REMAP:
								length = XGetShort(pUnit);
								pUnit += 2;
								// format is word count followed by x number of (word, word)
								pUnit += sizeof(short) * length;
								break;
						}
					}
				}
				songRMF->locked = locked;
				break;
		}
	}
}
#endif	// USE_CREATION_API == TRUE

//#if USE_CREATION_API == TRUE
XBOOL XGetSongEmbeddedStatus(SongResource *pSong)
{
//	SongResource_SMS		*songSMS;
	SongResource_RMF		*songRMF;
	SongResource_RMF_Linear	*songRMF2;
	XBOOL					embedded;

	embedded = FALSE;
	if (pSong)
	{
		switch (((SongResource_SMS *)pSong)->songType)
		{
//			case SONG_TYPE_SMS:
//				songSMS = (SongResource_SMS *)pSong;
//				break;

			case SONG_TYPE_RMF:
				songRMF = (SongResource_RMF *)pSong;
				embedded = songRMF->embeddedSong;
				break;

			case SONG_TYPE_RMF_LINEAR:
				songRMF2 = (SongResource_RMF_Linear *)pSong;
				embedded = TEST_FLAG_VALUE(songRMF2->flags, XBFL_embedded);
				break;
		}
	}
	return embedded;
}
//#endif	// USE_CREATION_API == TRUE

#if USE_CREATION_API == TRUE
void XSetSongEmbeddedStatus(SongResource *pSong, XBOOL embedded)
{
//	SongResource_SMS		*songSMS;
	SongResource_RMF		*songRMF;
	SongResource_RMF_Linear	*songRMF2;

	if (pSong)
	{
		switch (((SongResource_SMS *)pSong)->songType)
		{
//			case SONG_TYPE_SMS:
//				songSMS = (SongResource_SMS *)pSong;
//				break;

			case SONG_TYPE_RMF:
				songRMF = (SongResource_RMF *)pSong;
				songRMF->embeddedSong = embedded;
				break;

			case SONG_TYPE_RMF_LINEAR:
				songRMF2 = (SongResource_RMF_Linear *)pSong;
				songRMF2->flags = SET_FLAG_VALUE(songRMF2->flags, XBFL_embedded, embedded);
				break;
		}
	}
}
#endif	// USE_CREATION_API == TRUE


#if USE_CREATION_API == TRUE
/*
SongResource * XConvertToSongType(SongResource *pSong, SongType newSongType)
{
	SongResource_SMS	*songSMS;
	SongResource_RMF	*songRMF;
	short int			id;
	SongResource		*newSong;

	newSong = NULL;
	if (pSong)
	{
		switch (((SongResource_SMS *)pSong)->songType)
		{
			case SONG_TYPE_SMS:
				songSMS = (SongResource_SMS *)pSong;
				id = XGetShort(&songSMS->midiResourceID);
				
				switch (newSongType)
				{
					case SONG_TYPE_SMS:
						newSong = XNewSongPtr(
						break;
					case SONG_TYPE_RMF:
						break;
				}
				break;

			case SONG_TYPE_RMF:
				songRMF = (SongResource_RMF *)pSong;
				id = XGetShort(&songRMF->rmfResourceID);
				break;
		}
	}
	return newSong;
}
*/
#endif	// USE_CREATION_API == TRUE



SongType XGetSongResourceObjectType(SongResource *pSong)
{
	SongResource_SMS		*songSMS;
	SongResource_RMF		*songRMF;
	SongResource_RMF_Linear	*songRMF2;
	SongType				id;

	id = SONG_TYPE_BAD;
	if (pSong)
	{
		switch (((SongResource_SMS *)pSong)->songType)
		{
			case SONG_TYPE_SMS:
				songSMS = (SongResource_SMS *)pSong;
				id = (SongType)songSMS->songType;
				break;

			case SONG_TYPE_RMF:
				songRMF = (SongResource_RMF *)pSong;
				id = (SongType)songRMF->songType;
				break;
			case SONG_TYPE_RMF_LINEAR:
				songRMF2 = (SongResource_RMF_Linear *)pSong;
				id = (SongType)songRMF2->songType;
				break;
		}
	}
	return id;
}

#if USE_CREATION_API == TRUE
// given a valid SongResource_info structure and a type, this will return the string pointer
// from that structure
static char * PV_GetStringItemFromResource_info(SongResource_Info *pSongInfo, SongResourceType type)
{
	char * resourceName;

	resourceName = NULL;
	switch (type)
	{
		case R_TITLE:
			resourceName = pSongInfo->title;
			break;
		case R_COMPOSER:
			resourceName = pSongInfo->composer;
			break;
		case R_COPYRIGHT_DATE:
			resourceName = pSongInfo->copyright;
			break;
		case R_COPYRIGHT_LINE:
			break;
		case R_PUBLISHER_CONTACT:
			resourceName = pSongInfo->publisher_contact_info;
			break;
		case R_USE_OF_LICENSE:
			resourceName = pSongInfo->use_license;
			break;
		case R_LICENSED_TO_URL:
			resourceName = pSongInfo->licensed_to_URL;
			break;
		case R_LICENSE_TERM:
			resourceName = pSongInfo->license_term;
			break;
		case R_EXPIRATION_DATE:
			resourceName = pSongInfo->expire_date;
			break;
		case R_COMPOSER_NOTES:
			resourceName = pSongInfo->compser_notes;
			break;
		case R_INDEX_NUMBER:
			resourceName = pSongInfo->index_number;
			break;
		case R_PERFORMED_BY:
			resourceName = pSongInfo->performed;
			break;
		case R_GENRE:
			resourceName = pSongInfo->genre;
			break;
		case R_SUB_GENRE:
			resourceName = pSongInfo->sub_genre;
			break;
		case R_TEMPO:
			resourceName = pSongInfo->tempo_description;
			break;
		case R_ORIGINAL_SOURCE:
			resourceName = pSongInfo->original_source;
			break;
	}
	return resourceName;
}
#endif

#if USE_FULL_RMF_SUPPORT == TRUE
// given a valid SongResource_info structure and a type and a new string, this will set the string pointer
// in that structure
static void PV_SetStringItemFromResource_info(SongResource_Info *pSongInfo, SongResourceType type, char *resourceName)
{
	switch (type)
	{
		case R_TITLE:
			pSongInfo->title = resourceName;
			break;
		case R_COMPOSER:
			pSongInfo->composer = resourceName;
			break;
		case R_COPYRIGHT_DATE:
			pSongInfo->copyright = resourceName;
			break;
		case R_COPYRIGHT_LINE:
			break;
		case R_PUBLISHER_CONTACT:
			pSongInfo->publisher_contact_info = resourceName;
			break;
		case R_USE_OF_LICENSE:
			pSongInfo->use_license = resourceName;
			break;
		case R_LICENSED_TO_URL:
			pSongInfo->licensed_to_URL = resourceName;
			break;
		case R_LICENSE_TERM:
			pSongInfo->license_term = resourceName;
			break;
		case R_EXPIRATION_DATE:
			pSongInfo->expire_date = resourceName;
			break;
		case R_COMPOSER_NOTES:
			pSongInfo->compser_notes = resourceName;
			break;
		case R_INDEX_NUMBER:
			pSongInfo->index_number = resourceName;
			break;
		case R_PERFORMED_BY:
			pSongInfo->performed = resourceName;
			break;
		case R_GENRE:
			pSongInfo->genre = resourceName;
			break;
		case R_SUB_GENRE:
			pSongInfo->sub_genre = resourceName;
			break;
		case R_TEMPO:
			pSongInfo->tempo_description = resourceName;
			break;
		case R_ORIGINAL_SOURCE:
			pSongInfo->original_source = resourceName;
			break;
	}
}
#endif

#if USE_FULL_RMF_SUPPORT == TRUE
#if X_PLATFORM != X_WEBTV

// types used for the two forms of text types. Used in the XGetSongResourceInfo and
// XNewSongFromSongResourceInfo functions.
static const SongResourceType	sms_processTypes[] = 
			{R_TITLE, R_COMPOSER, R_COPYRIGHT_DATE, R_PUBLISHER_CONTACT};
static const SongResourceType	rmf_processTypes[] = 
			{R_TITLE, R_COMPOSER, R_COPYRIGHT_DATE, R_COPYRIGHT_LINE, 
			R_PUBLISHER_CONTACT, R_USE_OF_LICENSE, R_LICENSED_TO_URL, 
			R_LICENSE_TERM, R_EXPIRATION_DATE, R_COMPOSER_NOTES, 
			R_INDEX_NUMBER, R_PERFORMED_BY, R_GENRE, R_SUB_GENRE,
			R_TEMPO, R_ORIGINAL_SOURCE};



SongResource_Info * XGetSongResourceInfo(SongResource *pSong, long songSize)
{
	SongResource_Info		*pInfo;
	char					*text;
	SongResource_SMS		*songSMS;
	SongResource_RMF		*songRMF;
	SongResource_RMF_Linear	*songRMF2;
	short int				count, max;
	SongInfo				type;

	pInfo = NULL;
	text = (char *)XNewPtr(8192L);
	if (pSong && songSize && text)
	{
		pInfo = (SongResource_Info *)XNewPtr((long)sizeof(SongResource_Info));
		if (pInfo)
		{
			// fields common to all types
			pInfo->songType = (SongType)((SongResource_SMS *)pSong)->songType;
			pInfo->songVolume = XGetSongVolume(pSong);
			pInfo->songEmbedded = XGetSongEmbeddedStatus(pSong);
			pInfo->songLocked = XIsSongLocked(pSong);

			switch (((SongResource_SMS *)pSong)->songType)
			{
				case SONG_TYPE_SMS:
					songSMS = (SongResource_SMS *)pSong;
					pInfo->maxMidiNotes = songSMS->maxNotes;
					pInfo->maxEffects = songSMS->maxEffects;
					pInfo->mixLevel = (short)XGetShort(&songSMS->mixLevel);
					pInfo->reverbType = songSMS->reverbType;
					pInfo->objectResourceID = (XShortResourceID)XGetShort(&songSMS->midiResourceID);
					pInfo->songTempo = (short)XGetShort(&songSMS->songTempo);
					pInfo->songPitchShift = songSMS->songPitchShift;
					break;

				case SONG_TYPE_RMF:
					songRMF = (SongResource_RMF *)pSong;
					pInfo->maxMidiNotes = (short)XGetShort(&songRMF->maxNotes);
					pInfo->maxEffects = (short)XGetShort(&songRMF->maxEffects);
					pInfo->mixLevel = (short)XGetShort(&songRMF->mixLevel);
					pInfo->reverbType = songRMF->reverbType;
					pInfo->objectResourceID = (XShortResourceID)XGetShort(&songRMF->rmfResourceID);
					pInfo->songTempo = (short)XGetShort(&songRMF->songTempo);
					pInfo->songPitchShift = (short)XGetShort(&songRMF->songPitchShift);
					break;

				case SONG_TYPE_RMF_LINEAR:
					songRMF2 = (SongResource_RMF_Linear *)pSong;
					pInfo->maxMidiNotes = (short)XGetShort(&songRMF2->maxNotes);
					pInfo->maxEffects = (short)XGetShort(&songRMF2->maxEffects);
					pInfo->mixLevel = (short)XGetShort(&songRMF2->mixLevel);
					pInfo->reverbType = songRMF2->reverbType;
					pInfo->objectResourceID = (XShortResourceID)XGetShort(&songRMF2->audioResourceID);
					pInfo->songTempo = (short)XGetShort(&songRMF2->songTempo);
					break;
			}
			
			// now copy in all the strings
			max = sizeof(rmf_processTypes) / sizeof(SongResourceType);
			for (count = 0; count < max; count++)
			{
				type = PV_TranslateSongResourceTypeIntoSongInfo(rmf_processTypes[count]);
				if (type != I_INVALID)
				{
					XGetSongInformation(pSong, songSize, type, text);
					PV_SetStringItemFromResource_info(pInfo, rmf_processTypes[count], XDuplicateStr(text));
				}
			}
		}
		XDisposePtr(text);
	}
	return pInfo;
}
#endif	// X_PLATFORM != X_WEBTV
#endif	// USE_FULL_RMF_SUPPORT

// This will create a new song resource from the more accessable SongResource_Info
#if USE_CREATION_API == TRUE
SongResource * XNewSongFromSongResourceInfo(SongResource_Info *pSongInfo)
{
	SongResource 					*newSong, *otherNew;
	SongResource_SMS				*songSMS;
	SongResource_RMF				*songRMF;
	short int						count, max;
	char							*resourceName;
	SongResourceType				type;
	SongResourceType const			*pProcess;
	long							resourceLength;

	newSong = NULL;
	max = 0;
	if (pSongInfo)
	{
		newSong = XNewSongPtr(pSongInfo->songType,
							pSongInfo->objectResourceID,
							pSongInfo->maxMidiNotes, 
							pSongInfo->mixLevel, 
							pSongInfo->maxEffects,
							(ReverbMode)pSongInfo->reverbType);
		if (newSong)
		{
			pProcess = NULL;
			switch (pSongInfo->songType)
			{
				case SONG_TYPE_SMS:
					pProcess = sms_processTypes;
					max = sizeof(sms_processTypes) / sizeof(SongResourceType);
					songSMS = (SongResource_SMS *)newSong;
					XPutShort(&songSMS->songTempo, (unsigned short)pSongInfo->songTempo);
					songSMS->songPitchShift = (char)pSongInfo->songPitchShift;
					break;
				case SONG_TYPE_RMF:
					songRMF = (SongResource_RMF *)newSong;
					XPutShort(&songRMF->songTempo, (unsigned short)pSongInfo->songTempo);
					XPutShort(&songRMF->songPitchShift, (unsigned short)pSongInfo->songPitchShift);
					pProcess = rmf_processTypes;
					max = sizeof(rmf_processTypes) / sizeof(SongResourceType);
					XSetSongVolume(newSong, pSongInfo->songVolume);
					break;
			}

			if (pProcess)
			{
				for (count = 0; count < max; count++)
				{
					type = pProcess[count];
					resourceName = PV_GetStringItemFromResource_info(pSongInfo, type);
					resourceLength = XStrLen(resourceName);
					if (resourceLength)
					{
						otherNew = XChangeSongResource(newSong, XGetPtrSize(newSong), 
										type, resourceName, resourceLength + 1);
						if (otherNew)
						{
							// throw away the old one, and starting merging with the new ones
							XDisposeSongPtr(newSong);
							newSong = otherNew;
						}
					}
				}
			}
		}
	}
	return newSong;
}

void XDisposeSongResourceInfo(SongResource_Info *pSongInfo)
{
	if (pSongInfo)
	{
		XDisposePtr(pSongInfo->remaps);
		XDisposePtr(pSongInfo->velocityCurve);
		XDisposePtr(pSongInfo->title);
		XDisposePtr(pSongInfo->performed);
		XDisposePtr(pSongInfo->composer);
		XDisposePtr(pSongInfo->copyright);
		XDisposePtr(pSongInfo->licensed_to_URL);
		XDisposePtr(pSongInfo->use_license);
		XDisposePtr(pSongInfo->license_term);
		XDisposePtr(pSongInfo->expire_date);
		XDisposePtr(pSongInfo->compser_notes);
		XDisposePtr(pSongInfo->index_number);
		XDisposePtr(pSongInfo->publisher_contact_info);
		XDisposePtr(pSongInfo->genre);
		XDisposePtr(pSongInfo->sub_genre);
		XDisposePtr(pSongInfo->original_source);
		XDisposePtr(pSongInfo->tempo_description);
		XDisposePtr(pSongInfo);
	}
}
#endif	// USE_CREATION_API == TRUE


// Global function to get a Midi resource via its various forms:
// ID_ECMI		- compressed and encrypted midi file
// ID_EMID		- encrypted midi file
// ID_CMID		- compressed midi file
// ID_MIDI		- standard midi file
// ID_MIDI_OLD	- standard midi file
//
// if pType is not NULL, then store the type
// if pReturnedSize is not NULL, then store size
XPTR XGetMidiData(XLongResourceID theID, long *pReturnedSize, XResourceType *pType)
{
	XPTR			theData, pData;
	long			midiSize;
	XResourceType	type;

	type = ID_NULL;
// try encrypted and compressed midi files
	theData = XGetAndDetachResource(ID_ECMI, theID, &midiSize);
	if (theData)
	{
		// since this is encrypted, make a new copy and decrypt
		pData = XNewPtr(midiSize);
		if (pData)
		{
			XBlockMove(theData, pData, midiSize);
			XDecryptData(pData, (unsigned long)midiSize);				// decrypt first
		}
		XDisposePtr(theData);
		theData = pData;
		if (theData)
		{
			pData = XDecompressPtr(theData, (unsigned long)midiSize, TRUE);		// uncompress second
			if (pData)
			{
				midiSize = XGetPtrSize(pData);	// get new size
				XDisposePtr(theData);
				theData = pData;
				type = ID_ECMI;
			}
			else
			{	// something failed
				XDisposePtr(theData);
				theData = NULL;
			}
		}
	}
	else
	{
		theData = XGetAndDetachResource(ID_EMID, theID, &midiSize);
		if (theData)
		{
			// since this is encrypted, make a new copy and decrypt
			pData = XNewPtr(midiSize);
			if (pData)
			{
				XBlockMove(theData, pData, midiSize);
				XDecryptData(pData, (unsigned long)midiSize);				// decrypt first
				type = ID_EMID;
			}
			XDisposePtr(theData);
			theData = pData;
		}
	}

// nothing there, so try compressed midi files
	if (theData == NULL)
	{
		theData = XGetAndDetachResource(ID_CMID, theID, &midiSize);
		if (theData)
		{
			pData = XDecompressPtr(theData, (unsigned long)midiSize, TRUE);
			if (pData)
			{
				XDisposePtr(theData);
				theData = pData;
				type = ID_CMID;
			}
		}
	}

// still nothing try standard midi files
	if (theData == NULL)
	{
		theData = XGetAndDetachResource(ID_MIDI, theID, &midiSize);
		if (theData == NULL)
		{
			theData = XGetAndDetachResource(ID_MIDI_OLD, theID, &midiSize);
		}
		if (theData)
		{
			type = ID_MIDI;
		}
	}
	if (theData && pReturnedSize)
	{
		*pReturnedSize = midiSize;
	}
	if (pType)
	{
		*pType = type;
	}
	return theData;
}


// Get sound resource and detach from resource manager but don't decompress.
XPTR XGetRawSoundResourceByID(XLongResourceID theID, XResourceType *pReturnedType, long *pReturnedSize)
{
	XPTR			theData;
	long			size;
	XResourceType	type;

	size = 0;
	type = ID_CSND;
	theData = XGetAndDetachResource(ID_CSND, theID, &size);		// look for compressed version first
	if (theData == NULL)
	{
		theData = XGetAndDetachResource(ID_ESND, theID, &size);
		type = ID_ESND;
	}
	if (theData == NULL)
	{
		theData = XGetAndDetachResource(ID_SND, theID, &size);
		type = ID_SND;
	}
	if (theData == NULL)
	{
		type = ID_NULL;
	}
	if (pReturnedSize)
	{
		*pReturnedSize = size;
	}
	if (pReturnedType)
	{
		*pReturnedType = type;
	}
	return theData;
}

// Get sound resource and detach from resource manager or decompress
// This function can be replaced for a custom sound retriver
XPTR XGetSoundResourceByID(XLongResourceID theID, long *pReturnedSize)
{
	XPTR	thePreSound, theData;

	// look for compressed version first
	theData = XGetAndDetachResource(ID_CSND, theID, pReturnedSize);
	if (theData == NULL)
	{
		// look for encrypted version
		theData = XGetAndDetachResource(ID_ESND, theID, pReturnedSize);
		if (theData)
		{
			// since this is encrypted, make a new copy and decrypt
			thePreSound = theData;
			theData = XNewPtr(*pReturnedSize);
			if (theData)
			{
				XBlockMove(thePreSound, theData, *pReturnedSize);
				XDecryptData(theData, (unsigned long)*pReturnedSize);
			}
			XDisposePtr(thePreSound);
		}
		// look for standard version
		if (theData == NULL)
		{
			theData = XGetAndDetachResource(ID_SND, theID, pReturnedSize);
		}
	}
	else
	{
		thePreSound = theData;
		theData = XDecompressPtr(thePreSound, (unsigned long)*pReturnedSize, FALSE);
		XDisposePtr(thePreSound);
		*pReturnedSize = XGetPtrSize(theData);
	}
	return theData;
}

#if X_PLATFORM != X_WEBTV
// Get sound resource and detach from resource manager or decompress
// This function can be replaced for a custom sound retriver
XPTR XGetSoundResourceByName(void *cName, long *pReturnedSize)
{
	XPTR	thePreSound, theData;

	// look for compressed version first
	theData = XGetNamedResource(ID_CSND, cName, pReturnedSize);
	if (theData == NULL)
	{
		// look for standard version
		theData = XGetNamedResource(ID_SND, cName, pReturnedSize);

		if (theData == NULL)
		{
			// look for encrypted version
			theData = XGetNamedResource(ID_ESND, cName, pReturnedSize);
			if (theData)
			{
				// since this is encrypted, make a new copy and decrypt
				thePreSound = theData;
				theData = XNewPtr(*pReturnedSize);
				if (theData)
				{
					XBlockMove(thePreSound, theData, *pReturnedSize);
					XDecryptData(theData, (unsigned long)*pReturnedSize);
				}
				XDisposePtr(thePreSound);
			}
		}
	}
	else
	{
		thePreSound = theData;
		theData = XDecompressPtr(thePreSound, (unsigned long)*pReturnedSize, FALSE);
		XDisposePtr(thePreSound);
		*pReturnedSize = XGetPtrSize(theData);
	}
	return theData;
}
#endif


#if USE_CREATION_API == TRUE
void XSetSongPerformanceSettings(SongResource *pSong, short int maxMidiVoices, short int maxEffectsVoices,
										short int mixLevel)
{
	SongResource_SMS	*songSMS;
	SongResource_RMF	*songRMF;

	if (pSong)
	{
		switch (((SongResource_SMS *)pSong)->songType)
		{
			case SONG_TYPE_SMS:
				songSMS = (SongResource_SMS *)pSong;
				songSMS->maxNotes = (char)maxMidiVoices;
				XPutShort(&songSMS->mixLevel, (unsigned short)mixLevel);
				songSMS->maxEffects = (char)maxEffectsVoices;
				break;
			case SONG_TYPE_RMF:
				songRMF = (SongResource_RMF *)pSong;
				XPutShort(&songRMF->maxNotes, (unsigned short)maxMidiVoices);
				XPutShort(&songRMF->mixLevel, (unsigned short)mixLevel);
				XPutShort(&songRMF->maxEffects, (unsigned short)maxEffectsVoices);
				break;
		}
	}
}
#endif	// USE_CREATION_API == TRUE


#if USE_CREATION_API == TRUE
void XGetSongPerformanceSettings(SongResource * pSong, short int *maxMidiVoices, short int *maxEffectsVoices, short int *mixLevel)
{
	SongResource_SMS	*songSMS;
	SongResource_RMF	*songRMF;

	if (pSong && maxMidiVoices && maxEffectsVoices && mixLevel)
	{
		switch (((SongResource_SMS *)pSong)->songType)
		{
			case SONG_TYPE_SMS:
				songSMS = (SongResource_SMS *)pSong;
				*maxMidiVoices = songSMS->maxNotes;
				*mixLevel = (short)XGetShort(&songSMS->mixLevel);
				*maxEffectsVoices = songSMS->maxEffects;
				break;
			case SONG_TYPE_RMF:
				songRMF = (SongResource_RMF *)pSong;
				*maxMidiVoices = (short)XGetShort(&songRMF->maxNotes);
				*mixLevel = (short)XGetShort(&songRMF->mixLevel);
				*maxEffectsVoices = (short)XGetShort(&songRMF->maxEffects);
				break;
		}
	}
}
#endif	// USE_CREATION_API == TRUE



short int XGetSongVolume(SongResource *pSong)
{
	short int	volume;

	volume = MAX_SONG_VOLUME;
	if (pSong)
	{
		switch (((SongResource_SMS *)pSong)->songType)
		{
			case SONG_TYPE_RMF:
				volume = (short)XGetShort(&((SongResource_RMF *)pSong)->songVolume);
				if (volume == 0)
				{
					volume = MAX_SONG_VOLUME;
				}
				break;
		}
	}
	return volume;
}

void XSetSongVolume(SongResource *pSong, short int volume)
{
	if (pSong)
	{
		switch (((SongResource_SMS *)pSong)->songType)
		{
			case SONG_TYPE_RMF:
				XPutShort(&((SongResource_RMF *)pSong)->songVolume, (unsigned short)volume);
				break;
		}
	}
}


#if X_PLATFORM != X_WEBTV
short int XGetSongReverbType(SongResource *pSong)
{
	short int		reverbType;

	reverbType = 0;
	if (pSong)
	{
		switch (((SongResource_SMS *)pSong)->songType)
		{
			case SONG_TYPE_SMS:
				reverbType = ((SongResource_SMS *)pSong)->reverbType;
				break;
			case SONG_TYPE_RMF:
				reverbType = ((SongResource_RMF *)pSong)->reverbType;
				break;
		}
	}
	return reverbType;
}
#endif	//	X_PLATFORM != X_WEBTV


#if USE_CREATION_API == TRUE
void XSetSongReverbType(SongResource *pSong, short int reverbType)
{
	if (pSong)
	{
		switch (((SongResource_SMS *)pSong)->songType)
		{
			case SONG_TYPE_SMS:
				((SongResource_SMS *)pSong)->reverbType = (char)reverbType;
				break;
			case SONG_TYPE_RMF:
				((SongResource_RMF *)pSong)->reverbType = (char)reverbType;
				break;
		}
	}
}
#endif	// USE_CREATION_API == TRUE

#if USE_FULL_RMF_SUPPORT == TRUE
#if USE_CREATION_API == TRUE
XPTR XCreateBankStatus(BankStatus *pStatus)
{
	char			*pBank, *pData;
	long			size;

	pBank = NULL;
	if (pStatus)
	{
		size = XStrLen(pStatus->bankURL) + 1;
		size += XStrLen(pStatus->bankName) + 1;
		size += sizeof(long);
		pBank = (char *)XNewPtr(size);
		if (pBank)
		{
			pData = pBank;

			XPutLong(pData, pStatus->version);
			pData += sizeof(long);
			size = XStrLen(pStatus->bankURL) + 1;
			if (size > 1)
			{
				XBlockMove(pStatus->bankURL, pData, size);
			}
			pData += size;
			size = XStrLen(pStatus->bankName);
			if (size > 1)
			{
				XBlockMove(pStatus->bankName, pData, size);
			}
		}
	}
	return pBank;
}

// Create version resource that is ready to be stored
XPTR XCreateVersion(short int major, short int minor, short int subMinor)
{
	XVersion	*pVers;

	pVers = (XVersion *)XNewPtr((long)sizeof(XVersion));
	if (pVers)
	{
		XPutShort(&pVers->versionMajor, major);
		XPutShort(&pVers->versionMinor, minor);
		XPutShort(&pVers->versionSubMinor, subMinor);
	}
	return (XPTR)pVers;
}
#endif	// USE_CREATION_API == TRUE

void XGetBankStatus(BankStatus *pStatus)
{
	char			*pBank;
	long			size;

	if (pStatus)
	{
		XSetMemory((XPTR)pStatus, (long)sizeof(BankStatus), 0);
		pBank = (char *)XGetAndDetachResource(ID_BANK, DEFAULT_RESOURCE_BANK_ID, &size);
		if (pBank)
		{
			pStatus->version = XGetLong(pBank);		// get version

			pBank += sizeof(long);					// get bank URL
			size = XStrLen(pBank) + 1;
			if (size > BANK_NAME_MAX_SIZE-1)
			{
				size = BANK_NAME_MAX_SIZE-1;
			}
			XBlockMove(pBank, pStatus->bankURL, size);

			pBank += size;							// get bank name
			size = XStrLen(pBank) + 1;
			if (size > BANK_NAME_MAX_SIZE-1)
			{
				size = BANK_NAME_MAX_SIZE-1;
			}
			XBlockMove(pBank, pStatus->bankName, size);
		}
		XDisposePtr((XPTR)pBank);
	}
}

// Will return a XVersion number in platform order from the currently open resource file
void XGetVersionNumber(XVersion *pVersionNumber)
{
	XVersion	*pData;
	long		size;

	if (pVersionNumber)
	{
		pData = (XVersion *)XGetAndDetachResource(ID_VERS, DEFAULT_RESOURCE_VERS_ID, &size);
		if (pData)
		{
			pVersionNumber->versionMajor = (short)XGetShort(&pData->versionMajor);
			pVersionNumber->versionMinor = (short)XGetShort(&pData->versionMinor);
			pVersionNumber->versionSubMinor = (short)XGetShort(&pData->versionSubMinor);
			XDisposePtr(pData);
		}
		else
		{
			pVersionNumber->versionMajor = 1;
			pVersionNumber->versionMinor = 0;
			pVersionNumber->versionSubMinor = 0;
		}
	}
}

#endif	// USE_FULL_RMF_SUPPORT == TRUE


// EOF of DriverTools.c

