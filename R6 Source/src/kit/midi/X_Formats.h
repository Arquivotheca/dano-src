/*****************************************************************************/
/*
**	X_Formats.h
**
**		This is platform independent file and data formats for SoundMusicSys
**
**	© Copyright 1989-1996 Headspace, Inc, All Rights Reserved.
**	Written by Steve Hales
**
**	Headspace products contain certain trade secrets and confidential and
**	proprietary information of Headspace.  Use, reproduction, disclosure
**	and distribution by any means are prohibited, except pursuant to
**	a written license from Headspace. Use of copyright notice is
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
**	6/30/96		Created
**	7/3/96		Added packing pragmas
**	7/14/96		Removed PRAGMA_ALIGN_SUPPORTED
**	8/19/96		Added compressionType to SampleDataInfo
**	10/23/96	Changed GetKeySplitFromPtr to XGetKeySplitFromPtr
**	12/5/96		Added locked flags for songs and instruments
**	12/10/96	Added ID_RMF type
**	12/19/96	Added Sparc pragmas
**	12/30/96	Changed copyrights
**	1/2/97		Added ID_MOD type
**	1/3/97		Added XGetSongInformation
**	1/6/97		Added songType to XNewSongPtr
**	1/7/97		Changed structures typedef forms
**	1/12/97		Broke SongResource into two types: SongResource_SMS and SongResource_RMF
**	1/13/97		Added XGetSongResourceInfo & XDisposeSongResourceInfo & 
**				XGetSongResourceObjectID & XGetSongPerformanceSettings &
**				XGetSongResourceObjectType
**	1/18/97		Added XCheckAllInstruments & XCheckValidInstrument
**	1/24/97		Added SongResource_MOD
**	1/29/97		Added XGetSongInstrumentList
**				Added XGetMidiData
**				Added ID_ESND and ID_EMID types
**	1/30/97		Added XGetSoundResourceByName & XGetSoundResourceByID
**				Added XGetSongVoices & XSetSongVoices
*/
/*****************************************************************************/

#ifndef X_FORMATS
#define X_FORMATS

#include <BeBuild.h>

#ifndef __X_API__
	#include "X_API.h"
#endif

#ifndef G_SOUND
	#include "GenSnd.h"
#endif

#ifdef __cplusplus
	extern "C" {
#endif

#if __GNUC__
	#define _PACKED __attribute__((packed))
#elif __MWERKS__
	#define _PACKED
	#if (CPU_TYPE == kRISC) || (CODE_TYPE == BEBOX)
		#pragma options align=mac68k
	#elif ((CPU_TYPE == k80X86) || (CPU_TYPE == kSPARC))
		#pragma pack (1)
	#endif
#else
	#error - Unsupported compiler! -
#endif

/* Instrument and Song structures
*/
typedef struct 
{
	short int		instrumentNumber;
	short int		ResourceINSTID;
} _PACKED Remap;

#define ID_SONG							'SONG'		// song
#define ID_INST							'INST'		// instrument format
#define ID_MIDI							'Midi'		// standard midi file
#define ID_MIDI_OLD						'MIDI'		// standard midi file
#define ID_CMID							'cmid'		// compressed midi file
#define ID_CMIDI						'cmid'		// compressed midi file
#define ID_EMID							'emid'		// encrypted midi file
#define ID_ECMI							'ecmi'		// encrypted and compressed midi file
#define ID_SND							'snd '		// sample
#define ID_CSND							'csnd'		// compressed sample
#define ID_ESND							'esnd'		// encrypted sample
#define ID_RMF							'RMF!'		// rmf object
#define ID_MOD							'MOD!'		// mod object

// bits for SongResource_SMS flags1
#define XBF_locked						0x80
#define XBF_terminateDecay				0x40
#define XBF_interpolateSong				0x20
#define XBF_interpolateLead				0x10
#define XBF_fileTrackFlag				0x08
#define XBF_enableMIDIProgram			0x04
#define XBF_disableClickRemoval			0x02
#define XBF_useLeadForAllVoices			0x01

// bits for SongResource_SMS flags2
#define XBF_ignoreBadPatches			0x80
#define XBF_reserved4					0x40
#define XBF_reserved5					0x20
#define XBF_masterEnablePitchRandomness	0x10
#define XBF_ampScaleLead				0x08
#define XBF_forceAmpScale				0x04
#define XBF_masterEnableAmpScale		0x02
#define XBF_reserved6					0x01

typedef enum
{
	SONG_TYPE_BAD = -1,
	SONG_TYPE_SMS = 0,
	SONG_TYPE_RMF = 1,
	SONG_TYPE_MOD = 2
} SongType;

// Song resource (SMS type)
typedef struct
{
	short int		midiResourceID;
	char			reserved_0;
	char			reverbType;
	unsigned short	songTempo;
	char			songType;						//	0 - SMS, 1 - RMF, 2 - MOD
	char			songPitchShift;
	char			maxEffects;
	char			maxNotes;
	short int		mixLevel;
	unsigned char	flags1;							// see XBF for flags1
	char			noteDecay;
	char			defaultPercusionProgram;		// yes, I wanted signed!
	unsigned char	flags2;							// see XBF for flags2
	short int		remapCount;
	char 			remaps;							// Remap variable
//	unsigned char	copyright;						// variable pascal string
	unsigned char	author;							// variable pascal string
	unsigned char	title;							// variable pascal string
	unsigned char	licensor_contact;
} _PACKED SongResource_SMS;

typedef enum
{
	R_LAST_RESOURCE		= 0,				// empty. Not used
	R_TITLE				= 'TITL',			// Byte zero terminated string
	R_PERFORMED_BY		= 'PERF',			// Byte zero terminated string
	R_COMPOSER			= 'COMP',			// Byte zero terminated string
	R_COPYRIGHT_DATE	= 'COPD',			// Byte zero terminated string
	R_COPYRIGHT_LINE	= 'COPL',			// Byte zero terminated string
	R_PUBLISHER_CONTACT	= 'LICC',			// Byte zero terminated string
	R_USE_OF_LICENSE	= 'LUSE',			// Byte zero terminated string
	R_LICENSED_TO_URL	= 'LDOM',			// Byte zero terminated string
	R_LICENSE_TERM		= 'LTRM',			// Byte zero terminated string
	R_EXPIRATION_DATE	= 'EXPD',			// Byte zero terminated string
	R_COMPOSER_NOTES	= 'NOTE',			// Byte zero terminated string
	R_INDEX_NUMBER		= 'INDX',			// Byte zero terminated string

	R_INSTRUMENT_REMAP	= 'RMAP',			// variable amount
	R_VELOCITY_CURVE	= 'VELC'			// 128 bytes
} SongResourceType;


// Song resource (RMF type)
typedef struct
{
	short int		rmfResourceID;
	char			reserved_0;
	char			reverbType;
	unsigned short	songTempo;
	char			songType;						//	0 - SMS, 1 - RMF, 2 - MOD

	char			locked;
	short int		songPitchShift;
	short int		maxEffects;
	short int		maxNotes;
	short int		mixLevel;
	long			unused[8];
	
	short int		resourceCount;
	short int		resourceData;					// subtract this when calculating empty structure
	//
	// from this point on, the data is based upon types and data blocks
//	char			title[1];						// variable C string
//	char			composer[1];					// variable C string
//	char			copyright_date[1];				// variable C string
//	char			copyright_line[1];				// variable C string
//	char			contact_info[1];				// variable C string
//	char			use_license[1];					// variable C string
//	char			license_term[1];				// variable C string
//	char			territory[1];					// variable C string
//	char			expire_date[1];					// variable C string
//	char			foreign_rights[1];				// variable C string
//	char			compser_notes[1];				// variable C string
//	char			index_number[1];				// variable C string
//	unsigned char	velocityCurve[128];
} _PACKED SongResource_RMF;

// Song resource (MOD type)
typedef struct
{
	short int		modResourceID;
	char			reserved_0;
	char			reverbType;
	unsigned short	songTempo;
	char			songType;						//	0 - SMS, 1 - RMF, 2 - MOD

	char			locked;
	short int		maxEffects;
	short int		maxNotes;
	short int		mixLevel;
	long			unused[8];
} _PACKED SongResource_MOD;

typedef void SongResource;

// SongResource structure expanded. These values are always in native word order
// Use XGetSongResourceInfo, and XDisposeSongResourceInfo
typedef struct
{
	short int		maxMidiNotes;
	short int		maxEffects;
	short int		mixLevel;
	short int		reverbType;
	short int		objectResourceID;
	SongType		songType;
	long			songTempo;
	short int		songPitchShift;

	char			*title;							// 0
	char			*performed;						// 1
	char			*composer;						// 2
	char			*copyright_date;				// 3
	char			*copyright_line;				// 4
	char			*publisher_contact_info;		// 5
	char			*use_license;					// 6
	char			*licensed_to_URL;				// 7
	char			*license_term;					// 8
	char			*expire_date;					// 9
	char			*compser_notes;					// 10
	char			*index_number;					// 11
	
} _PACKED SongResource_Info;


typedef struct
{
	char			lowMidi;
	char			highMidi;
	short int		sndResourceID;
	short int		smodParameter1;
	short int		smodParameter2;
} _PACKED KeySplit;

// bits for Instrument flags1
#define ZBF_enableInterpolate			0x80
#define ZBF_enableAmpScale				0x40
#define ZBF_disableSndLooping			0x20
#define ZBF_reserved_1					0x10
#define ZBF_useSampleRate				0x08
#define ZBF_sampleAndHold				0x04
#define ZBF_extendedFormat				0x02
#define ZBF_avoidReverb					0x01
// bits for Instrument flags2
#define ZBF_neverInterpolate			0x80
#define ZBF_playAtSampledFreq			0x40
#define ZBF_fitKeySplits				0x20
#define ZBF_enableSoundModifier			0x10
#define ZBF_useINSTforSplits			0x08
#define ZBF_notPolyphonic				0x04
#define ZBF_enablePitchRandomness		0x02
#define ZBF_playFromSplit				0x01

#define SET_FLAG_VALUE(oldflag, newflag, value)		(value) ? ((oldflag) | (newflag)) : ((oldflag) & ~(newflag))
#define TEST_FLAG_VALUE(flags, flagbit)				((flags) & (flagbit)) ? TRUE : FALSE

// Special Instrument resource. This can only be used when there is no tremolo data, or key splits
typedef struct
{
	short int		sndResourceID;
	short int		midiRootKey;
	char			panPlacement;
	unsigned char	flags1;				// see ZBF bits for values
	unsigned char	flags2;				// see ZBF bits for values
	char			smodResourceID;
	short int		smodParameter1;
	short int		smodParameter2;
	short int		keySplitCount;		// if this is non-zero, then KeySplit structure is inserted
	// to go beyond this point, if keySplitCount is non-zero, you must use function calls.
	short int		tremoloCount;		// if this is non-zero, then a Word is inserted.
	short int		tremoloEnd;			// Always 0x8000
	short int		reserved_3;
	short int		descriptorName;		// Always 0
	short int		descriptorFlags;	// Always 0
} _PACKED InstrumentResource;


// These are included here, because we want to be independent of MacOS, but use this standard format
#ifndef __SOUND__
enum 
{
	notCompressed			= 0,			/*compression ID's*/
	fixedCompression		= -1,			/*compression ID for fixed-sized compression*/
	variableCompression		= -2,			/*compression ID for variable-sized compression*/
	twoToOne				= 1,
	eightToThree			= 2,
	threeToOne				= 3,
	sixToOne				= 4,

	stdSH					= 0x00,			/*Standard sound header encode value*/
	extSH					= 0xFF,			/*Extended sound header encode value*/
	cmpSH					= 0xFE,			/*Compressed sound header encode value*/

	rate44khz				= 0xAC440000L,	/*44100.00000 in fixed-point*/
	rate22050hz				= 0x56220000L,	/*22050.00000 in fixed-point*/
	rate22khz				= 0x56EE8BA3L,	/*22254.54545 in fixed-point*/
	rate11khz				= 0x2B7745D1L,	/*11127.27273 in fixed-point*/
	rate11025hz				= 0x2B110000,	/*11025.00000 in fixed-point*/

	kMiddleC				= 60,			/*MIDI note value for middle C*/

	soundCmd				= 80,
	bufferCmd				= 81,
	firstSoundFormat		= 0x0001,		/*general sound format*/
	secondSoundFormat		= 0x0002		/*special sampled sound format (HyperCard)*/
};
#endif

typedef struct
{
	char					*samplePtr;		/*if NIL then samples are in sampleArea*/
	unsigned long			length;			/*length of sound in bytes*/
	unsigned long			sampleRate;		/*sample rate for this sound*/
	unsigned long			loopStart;		/*start of looping portion*/
	unsigned long			loopEnd;		/*end of looping portion*/
	unsigned char			encode;			/*header encoding*/
	unsigned char			baseFrequency;	/*baseFrequency value*/
	unsigned char			sampleArea[1];	/*space for when samples follow directly*/
} _PACKED XSoundHeader;
typedef XSoundHeader *XSoundHeaderPtr;

typedef struct
{
	char					*samplePtr;		/*if nil then samples are in sample area*/
	unsigned long			numChannels;	/*number of channels i.e. mono = 1*/
	unsigned long			sampleRate;		/*sample rate in Apples Fixed point representation*/
	unsigned long			loopStart;		/*loopStart of sound before compression*/
	unsigned long			loopEnd;		/*loopEnd of sound before compression*/
	unsigned char			encode;			/*data structure used , stdSH, extSH, or cmpSH*/
	unsigned char			baseFrequency;	/*same meaning as regular SoundHeader*/
	unsigned long			numFrames;		/*length in frames ( packetFrames or sampleFrames )*/
	char					AIFFSampleRate[10];	/*IEEE sample rate*/
	char					*markerChunk;	/*sync track*/
	long					format;			/*data format type, was futureUse1*/
	char					futureUse2_0;	/*reserved by Apple, Igor will use as IMA encoder to 8 or 16 bit output. Set to 0x80 */
											// to encode as 8 bit output
	char					futureUse2_1;	/*reserved by Apple*/
	char					futureUse2_2;	/*reserved by Apple*/
	char					futureUse2_3;	/*reserved by Apple*/
	void					*stateVars;		/*pointer to State Block*/
	void					*leftOverSamples;	/*used to save truncated samples between compression calls*/
	short					compressionID;	/*0 means no compression, non zero means compressionID*/
	unsigned short			packetSize;		/*number of bits in compressed sample packet*/
	unsigned short			snthID;			/*resource ID of Sound Manager snth that contains NRT C/E*/
	unsigned short			sampleSize;		/*number of bits in non-compressed sample*/
	unsigned char			sampleArea[1];	/*space for when samples follow directly*/
} _PACKED XCmpSoundHeader;
typedef XCmpSoundHeader * XCmpSoundHeaderPtr;

typedef struct
{
	char					*samplePtr;		/*if nil then samples are in sample area*/
	unsigned long			numChannels;	/*number of channels,  ie mono = 1*/
	unsigned long			sampleRate;		/*sample rate in Apples Fixed point representation*/
	unsigned long			loopStart;		/*same meaning as regular SoundHeader*/
	unsigned long			loopEnd;		/*same meaning as regular SoundHeader*/
	unsigned char			encode;			/*data structure used , stdSH, extSH, or cmpSH*/
	unsigned char			baseFrequency;	/*same meaning as regular SoundHeader*/
	unsigned long			numFrames;		/*length in total number of frames*/
	char					AIFFSampleRate[10];	/*IEEE sample rate*/
	char					*markerChunk;	/*sync track*/
	char					*instrumentChunks;	/*AIFF instrument chunks*/
	char					*AESRecording;
	unsigned short			sampleSize;		/*number of bits in sample*/
	unsigned short			futureUse1;		/*reserved by Apple*/
	unsigned long			futureUse2;		/*reserved by Apple*/
	unsigned long			futureUse3;		/*reserved by Apple*/
	unsigned long			futureUse4;		/*reserved by Apple*/
	unsigned char			sampleArea[1];	/*space for when samples follow directly*/
} _PACKED XExtSoundHeader;
typedef XExtSoundHeader *XExtSoundHeaderPtr;

typedef struct
{
	short int		type;
	short int		numModifiers;
	unsigned short	modNumber;
	long			modInit;
	short int		numCommands;
// first command
	unsigned short	cmd;
	short int		param1;
	long			param2;
} _PACKED XSoundFormat1;

typedef struct
{
	XSoundFormat1	sndHeader;
	XSoundHeader	sndBuffer;
} _PACKED XSndHeader1;


typedef struct
{
	short int		type;
	short int		refCount;
	short int		numCmds;
// first command
	unsigned short	cmd;
	short int		param1;
	long			param2;
} _PACKED XSoundFormat2;

typedef struct
{
	XSoundFormat2	sndHeader;
	XSoundHeader	sndBuffer;
} _PACKED XSndHeader2;

#if __MWERKS__
	#if (CPU_TYPE == kRISC) || (CODE_TYPE == BEBOX)
		#pragma options align=reset
	#elif ((CPU_TYPE == k80X86) || (CPU_TYPE == kSPARC))
		#pragma pack ()
	#endif
#endif // __MWERKS__

#define C_NONE	'none'
#define C_IMA4	'ima4'	// CCITT G.721 ADPCM compression (IMA 4 to 1)
#define C_MACE3	'mac3'	// Apple MACE type 3 to 1
#define C_MACE6	'mac6'	// Apple MACE type 6 to 1
#define C_ULAW	'ulaw'	// u law; 2 to 1

typedef struct
{
	unsigned long	rate;				// sample rate
	unsigned long	frames;				// number of audio frames
	unsigned long	size;				// size in bytes
	unsigned long	loopStart;			// loop start frame
	unsigned long	loopEnd;			// loop end frame
	short int		bitSize;			// sample bit size; 8 or 16
	short int		channels;			// mono or stereo; 1 or 2
	short int		baseKey;			// base sample key
	short int		theID;				// sample ID if required
	long			compressionType;	// compression type
	void			*pMasterPtr;		// master pointer if required
} SampleDataInfo;


typedef enum
{
	I_INVALID = 0,				// invalid type
	I_TITLE,					// Title
	I_PERFORMED_BY,				// Performed by
	I_COMPOSER,					// Composer(s)
	I_COPYRIGHT_DATE,			// Copyright Date
	I_COPYRIGHT_LINE,			// Copyright Line
	I_PUBLISHER_CONTACT,		// Publisher Contact Info
	I_USE_OF_LICENSE,			// Use of License
	I_LICENSED_TO_URL,			// License to URL
	I_LICENSE_TERM,				// License term
	I_EXPIRATION_DATE,			// Expiration Date
	I_COMPOSER_NOTES,			// Composer Notes
	I_INDEX_NUMBER				// Index Number
} SongInfo;

void XGetSongInformation(SongResource *theSong, long songSize, SongInfo type, char *cName);

unsigned long XGetSongInformationSize(SongResource *theSong, long songSize, SongInfo type);

// Utillities for instruments
short int XCollectSoundsFromInstrument(InstrumentResource *theX, short *sndArray, short maxArraySize);
short int XCollectSoundsFromInstrumentID(short int theID, short *sndArray, short maxArraySize);
XBOOL XCheckAllInstruments(short *badInstrument, short *badSnd);
short int XCheckValidInstrument(short int theID);
short int XGetInstrumentArray(short int *instArray, short maxArraySize);

short int XGatherAllSoundsFromAllInstruments(short int *pSndArray, short int maxArraySize);

// Given a song ID and two arrays, this will return the INST resources ID and the 'snd ' resource ID
// that are needed to load the song terminated with a -1.
// Will return 0 for success or 1 for failure
OPErr XGetSongInstrumentList(short int theSongID, short int *pInstArray, short int maxInstArraySize, 
										short int *pSndArray, short int maxSndArraySize);

short int XGetSamplesFromInstruments(short int *pInstArray, short int maxInstArraySize, 
										short int *pSndArray, short int maxSndArraySize);

void XGetKeySplitFromPtr(InstrumentResource *theX, short int entry, KeySplit *keysplit);
void XSetKeySplitFromPtr(InstrumentResource *theX, short int entry, KeySplit *keysplit);

InstrumentResource * XAddKeySplit(InstrumentResource *theX, short int howMany);
InstrumentResource * XRemoveKeySplit(InstrumentResource *theX, short int howMany);

short int XGetTotalKeysplits(short int *instArray, short int totalInstruments, 
								short int *sndArray, short int totalSnds);


// Create a new song resource.
SongResource * XNewSongPtr(	SongType songType, 
							short int midiID,
							short int maxSongVoices, 
							short int mixLevel, 
							short int maxEffectVoices,
							short int reverbType);

void XDisposeSongPtr(SongResource *theSong);

XPTR XGetSoundResourceByID(long theID, long *pSize);
XPTR XGetSoundResourceByName(void *cName, long *pSize);

XPTR XGetMidiData(long theID, long *pSize);
XPTR XCompressAndEncrypt(XPTR pData, long size, long *pNewSize);

XPTR XGetSamplePtrFromSnd(XPTR pRes, SampleDataInfo *pInfo);

void XSetSoundLoopPoints(XPTR pRes, long loopStart, long loopEnd);
void XSetSoundSampleRate(XPTR pRes, unsigned long sampleRate);
void XSetSoundBaseKey(XPTR pRes, short int baseKey);

InstrumentResource * XNewInstrument(short int leadSndID);
void XDisposeInstrument(InstrumentResource *theX);

SongResource_Info * XGetSongResourceInfo(SongResource *pSong, long songSize);
void XDisposeSongResourceInfo(SongResource_Info *pSongInfo);

SongResource * XNewSongFromSongResourceInfo(SongResource_Info *pSongInfo);

short int XGetSongResourceObjectID(SongResource *pSong);
void XSetSongResourceObjectID(SongResource *pSong, short int id);
void XSetSongLocked(SongResource *pSong, XBOOL locked);
void XGetSongPerformanceSettings(SongResource * theSong, short int *maxMidiVoices, 
									short int *maxEffectsVoices, short int *mixLevel);
void XSetSongPerformanceSettings(SongResource *pSong, short int maxMidiVoices, short int maxEffectsVoices,
										short int mixLevel);
void XSetSongReverbType(SongResource *pSong, short int reverbType);
short int XGetSongReverbType(SongResource *pSong);

long XGetSongTempoFactor(SongResource *pSong);
void XSetSongTempoFactor(SongResource *pSong, long newTempo);

SongType XGetSongResourceObjectType(SongResource *pSong);

SongResource * XChangeSongResource(SongResource *theSong, long songSize, 
					SongResourceType resourceType, void *pResource, long resourceLength);

#ifdef __cplusplus
	}
#endif

#endif	// X_FORMATS


