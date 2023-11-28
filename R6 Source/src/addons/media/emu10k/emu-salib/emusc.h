
/******************************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1994. All rights Reserved.
*
*******************************************************************************
*/

/*****************************************************************************
*
* Filename: emusc.h
*
* Description: Header file for the Sound Coordinator API
*              For use with E-mu 8200 Sound Engine Software
*
* Revision: 2.0
*******************************************************************************
*/

#ifndef __EMUSC_H
#define __EMUSC_H

/***************
 * Include files
 **************/

#include "datatype.h" 
#include "emucllbk.h" 
#include "emuerrs.h"

/*******************************************************
 * System API Dependencies
 * Client should modify these to match their system
 * NOTE: APIPTR should match default EMU8200
 *       pointer types. 
 ******************************************************/
#define EXTERN EMUCTYPE
#define APITYPE      
#define APIPTR *
typedef void APIPTR EMUPARAMPTR;
typedef void APIPTR AVOIDPTR;
typedef CHAR APIPTR ACHARPTR;
typedef BYTE APIPTR ABYTEPTR;
typedef WORD APIPTR AWORDPTR;
typedef SHORT APIPTR ASHORTPTR;
typedef DWORD APIPTR ADWORDPTR;
typedef enum VoiceParamTag SEVOICEPARAM;
 
/***************
 * Defines
 **************/

/* Pass one of these into LocateSF byMemType parameter */
#define INRAM 0
#define INROM 1
#define MAX_ID_SIZE 8

#define RIGHT_STREAM 0
#define LEFT_STREAM 1

#define MAX_SF_NAME 100
#define MAX_PRESET_NAME 25
#define MAX_PRESETS 128

#define DA_PLAY 0
#define DA_RECORD 1

#define DA_MONO 0
#define DA_MONO2 2
#define DA_STEREO 1

#define DA_SAA_INPUT    0
#define DA_CPU_MEMORY   1
#define DA_CPU_MEMORY_8 4
#define DA_SYNTH_OUTPUT 2

#define NO_DAID -1
#define NO_MOD_CHAN 16

#define DEFAULT_DRUM_BANK 128
#define NO_MORE_SAMPLES 0xFFFFFFFFL

#define INVALID_SFID 0xffff

/****************************************
 * Enumeration tables and data structures
 ***************************************/

typedef WORD  SFID;
typedef AWORDPTR SFIDPTR;
typedef SHORT DAID;
typedef ASHORTPTR DAIDPTR;
typedef DWORD  SCID;
typedef ADWORDPTR SCIDPTR; 
typedef void (*unImplementedSysExCallBack)(void);
 
/*****************************************
 * Configuration data structure 
 *****************************************/

typedef struct stSoundCoordinatorEnvironTag 
{
   CHAR chCustomerID[MAX_ID_SIZE];
   BYTE byProcessSFAtLoad;
   BYTE bySFInSoundROM;
   BYTE byUseDriver;
   WORD uiSAALowRightVoice, uiSAAHighRightVoice;
   WORD uiSAALowLeftVoice,  uiSAAHighLeftVoice;
   DWORD singleDABufferSize;
   DWORD soundROMOffset;
   DWORD soundROMSize;
   unImplementedSysExCallBack sysExCallBack;
   DWORD dwSoundRAMOffset;     // Overrides
   DWORD dwSoundRAMSize;       //    ""
   DWORD driverEnvironSize;
   AVOIDPTR driverEnviron;
} stSoundCoordinatorEnviron;

/*************************************************************************  
*  This series of data structures is used in the Sound Bank loading
*  and unloading functions. 
*************************************************************************/

typedef struct stPacketSampleLoadTag
{
  DWORD sampleLocation;
  DWORD sampleSize;
} stPacketSampleLoad;
typedef stPacketSampleLoad APIPTR STPACKETSAMPLELOADPTR;

typedef struct stPacketChunkDataTag
{
  ACHARPTR chunkData;
  DWORD    chunkDataLocation;
  DWORD    chunkDataSize;
} stPacketChunkData;
typedef stPacketChunkData APIPTR STPACKETCHUNKDATAPTR;

typedef struct stPacketInfoArtDataTag
{
  ACHARPTR fileName;
  STPACKETCHUNKDATAPTR fileDataChunkArray;
  DWORD numFileDataChunks;
  stPacketSampleLoad firstSample;
  DWORD totalSampleSize;
} stPacketInfoArtData;
typedef stPacketInfoArtData APIPTR STPACKETINFOARTDATAPTR;

/* 
   This union specifies a destination MIDI bank.
   The data space for midiBank is subdivided into a space for
   MIDI Bank LSB (CC32) and MIDI Bank MSB (CC0). 
   NOTE: midiBank is NOT (MIDIBankMSB << 7) + MIDIBankLSB.
   NOTE: Emu8200 ignores data in midiBankCC32 or midiBankLSB field!
*/

typedef struct stMIDIBankValueTag
{
  BYTE midiBankCC0, midiBankCC32;
} stMIDIBankValue;

typedef struct stMIDIBankResolutionTag
{
  BYTE midiBankMSB, midiBankLSB;
} stMIDIBankResolution;

typedef union unMIDIBankTag
{
  stMIDIBankValue      midiBankByValue;
  stMIDIBankResolution midiBankByRes;
  WORD                 data;
} unMIDIBank;

/* 
   This data structure specifies a destination MIDI location.
   A MIDI location is defined as a MIDI bank/preset combination.
*/
typedef struct midiDestTag
{
  unMIDIBank midiBankIndex;
  WORD       midiProgramIndex;
} midiDest;

/* 
   This data structure holds Preset information of an UNLOADED 
   sound bank. Inside is a Preset name and a default destination
   MIDI location to which that preset would lie if the bank were
   loaded using LoadSoundBank().
*/
typedef struct stSoundBankSinglePresetStruct
{
  CHAR chPresetName[MAX_PRESET_NAME];
  midiDest defaultDestination;
} stSoundBankSinglePreset;  
typedef stSoundBankSinglePreset APIPTR STSOUNDBANKSINGLEPRESETPTR;

/* 
   This data structure is designed to hold information for ALL Presets 
   in an UNLOADED sound bank. 
*/
typedef struct stSoundBankPresetStruct
{
  CHAR chSoundBankName[MAX_SF_NAME];
  STSOUNDBANKSINGLEPRESETPTR singlePresetArray;
} stSoundBankPresetInfo;
typedef stSoundBankPresetInfo APIPTR STSOUNDBANKPRESETINFOPTR;
  
typedef struct stSoundBankSingleItemType
{
  CHAR itemName[MAX_PRESET_NAME];
} stSoundBankSingleItem;
typedef stSoundBankSingleItem APIPTR STSOUNDBANKSINGLEITEMPTR;

/* 
   This data structure holds Sound Object information of an UNLOADED 
   sound bank. A Sound Object is a single entity within a sound
   bank which may be loaded and played back using EMU8200. 
   This may include a SoundFont Preset/Instrument/Sample, a WAV file,
   etc.
*/
typedef struct stSoundBankSoundInfoStruct
{
  CHAR chSoundBankName[MAX_SF_NAME];
  STSOUNDBANKSINGLEITEMPTR singleItemArray;
} stSoundBankSoundInfo;
typedef stSoundBankSoundInfo APIPTR STSOUNDBANKSOUNDINFOPTR;

/*  
   This enumeration list is used in the LoadSound functions below.
   It specifies the nature of the SoundFont object which is to be
   loaded.
*/
typedef enum enSoundObjectType
{
  sotBank,                 /* All sounds in a SoundFont bank      */
  sotPreset,               /* A preset in a SoundFont bank        */
  sotInstrument,           /* An instrument in a SoundFont bank   */
  sotSample,               /* A sampled waveform in a SoundFont bank */

  sotEndSoundObjects
} enSoundObject;

/*  
   This enumeration list is used in the sfBankOpen data structure below.
   It specifies the physical nature of the SoundFont bank which is to
   be loaded, not the format of the bank (WAV, SF2, etc).
*/
typedef enum enSourceBankType
{
  btFileFullPath,          /* A full path to a file               */
  btInternalROM,           /* Internal (Sound) ROM (Silicon SF)   */
  btFileSpecifierPointer,  /* A system specific pointer to a file */
  btDataSystemMemory,      /* System RAM                          */
  btDataPacketMemory,      /* Packet-ized system memory           */

  btEndSourceBankTypes
} enSourceBank;

/* 
   This union is provided for convenience to allow you to use the API
   without recasting. The idea is that all parameters are in the same
   memory location. Based on which kind of Sound Data you are loading,
   let the "path" in the appropriate "user locations". Then pass in
   the "general location" to the API.
*/
typedef union unPathTypesType
{
  /* General location */
  EMUPARAMPTR path;                  /* This parameter is used in sfBankOpen  */

  /* User locations */
  ACHARPTR    fullPath;              /* This is where full path is set        */
  DWORD       siliconSoundFontIndex; /* This is where Silicon SF Index is set */
  AVOIDPTR    memoryBufferPointer;   /* this is where RAM location is set     */
  STPACKETINFOARTDATAPTR
	      packetBufferPointer;   /* this is where Packet info location    */
  /*FSSpecPtr macFSSpec;             /* This is where MAC FSSpec is set       */

} unPathTypes;

/*
   This data structure is used in all of the SoundFont loading/information
   commands. It specifies the physical location of the SoundFont data
   as well as a "path" to get to a particular element.
*/
typedef struct stBankOpenType
{
  enSourceBank type;       // What (physical) kind of bank you have
  unPathTypes  path;       // What the "path" to the bank is
} stBankOpen;
typedef stBankOpen APIPTR STBANKOPENPTR;

typedef struct stInfoTag
{
  WORD blah;
} stInfo;
typedef stInfo APIPTR STINFOPTR;

typedef DWORD midiRegion;
typedef WORD  SoundIndex;
#define NO_INDEX 0xFFFF

/*
   This data structure is used to retrieve information about a LOADED
   SoundFont bank. Since the bank is now LOADED, the information is
   based upon MIDI space, so total necessary space may be allocated
   by the client without asking information.
*/
typedef struct stBrowseBankStruct 
{
     WORD wParentBankNum;
     CHAR chFileName[MAX_SF_NAME];
     CHAR chSoundFontName[MAX_SF_NAME];
     CHAR chPresetNames[MAX_PRESETS][MAX_PRESET_NAME];
     CHAR chPresetFileNames[MAX_PRESETS][MAX_SF_NAME];
} stBrowseBank;
typedef stBrowseBank APIPTR STBROWSEBANKPTR;

/* Synthesizer modes: Use with SetSynthMode() */
typedef enum enSynthModeType
{
  synSingleBankGM   = 0, /* All instruments in bank index 0
			    other bank indices ignored
			    MIDI channel 10 plays instruments in percussive
			    section (bank index 128)
			    GM emulation */
  synMultiBankGM    = 1, /* Instruments in banks 0-127
			    MIDI channel 10 plays instruments in percussive
			    section (bank index 128)
			    GM emulation */
  synSingleBankMT32 = 2, /* Instruments in bank index 127, then 0
			    other bank indices ignored
			    MIDI channel 10 plays instruments in percussive
			    section (bank index 128 preset 127 then 0)
			    MT-32 emulation */
  synFullCustom     = 3  /* Instruments in banks 0 to 127
			    MIDI channel 10 is treated no different than others
			    Percussive section of SF bank inaccessible
			    (however not necessary outside of GM emulation) */
} enSynthMode;

typedef struct daInitStruct
{ 
   WORD  operationMode;       // DA_PLAY or DA_RECORD
   WORD  audioType;           // DA_MONO or DA_STEREO
   WORD  audioSource;         // DA_SAA_INPUT or DA_CPU_MEMORY or DA_SYNTH_OUTPUT
   BYTE  modulationChannel;   // 0 to 16, 16 means no MIDI Chan modulation
   DWORD sampleRate;          // 11025, 22050, 44100
   BYTE  panValue;            // same as VoiceChannel->byPanEffectsSend
   BYTE  volumeValue;         // same as VoiceChannel->byInitialVolume
} daInit;


typedef enum {
   Emu3DaudPause,
   Emu3DaudStop,
   Emu3DaudStart,
   Emu3DaudSuspend,
   Emu3DaudResume
} Emu3DSoundState;

/****************************************
 * The Sound Coordinator API functions
 ***************************************/

/* Revision control */
EXTERN void APITYPE   GetCustomerVersion(SCID scID, ACHARPTR); /* Returns chCustomerID */
EXTERN void APITYPE   GetSoundCoorVersion(SCID scID, ACHARPTR); /*No more than 8 bytes long*/

/* Global */
EXTERN EMUSTAT APITYPE ResetDriver(stSoundCoordinatorEnviron APIPTR,
                                   SCIDPTR pScID);
EXTERN DWORD APITYPE GetDriverID(SCID scID);
EXTERN EMUSTAT APITYPE DestructDriver(SCID scID);
EXTERN ACHARPTR APITYPE GetErrorDetails(SCID scID, AWORDPTR severity,
                                        AWORDPTR detailCode);

/* SoundFont loading and unloading */

EXTERN BYTE    APITYPE GetNumSiliconFonts(SCID scID);

/* Using this function removes the need to use the 3 following
   functions */
EXTERN SoundIndex APITYPE GetNumberOfObjects(SCID scID, STBANKOPENPTR bank, 
                                             enSoundObject obj);

/* These functions provided for convenience */
EXTERN SoundIndex APITYPE GetNumberOfPresets(SCID scID, STBANKOPENPTR bank);
EXTERN SoundIndex APITYPE GetNumberOfInstruments(SCID scID, STBANKOPENPTR bank);
EXTERN SoundIndex APITYPE GetNumberOfSamples(SCID scID, STBANKOPENPTR bank);

/* 
   Get information about a SoundFont bank. Information may be general, 
   and / or sound object names.  

   Client MUST allocate ALL data space for use in this function!

   The number of elements in the array of 'presets' should be the value 
   returned by GetNumberOfObjects(bo, sotPreset).

   The number of elements in the array of 'instruments' or 'sounds' 
   should be the value returned by GetNumberOfObjects(bo, sotInstrument) 
   or GetNumberOfObjects(sotSound), respectively.

   Set any of the PTRs to NULL if you do NOT want that information.

   IE GetSoundBankInfo(&bankOpen, &info, NULL, NULL, NULL) will only
   retreive general information about the bank (and will return faster)
*/
EXTERN EMUSTAT APITYPE GetSoundBankInfo(SCID scID, STBANKOPENPTR bank,
					STINFOPTR     info,
					STSOUNDBANKPRESETINFOPTR presets,
					STSOUNDBANKSOUNDINFOPTR  instruments,
					STSOUNDBANKSOUNDINFOPTR  samples);

/* Load ONLY the specified OBJECT in a sound bank to a given MIDI
   destination. Specify the object by 'object', sound by index, 
   as indexed in the array of 'sounds' passed back from 
   GetSoundBankInfo(). Client MUST specify the destination MIDI 
   location with 'destination'. For loading Banks, only the 
   midiDest->midiBankIndex is considered. For loading Presets
   or Instruments, the midiDest->midiProgramIndex is also 
   considered. For loading Sounds, client may also specify a 
   destination key/velocity range within a MIDI location for the 
   sound using 'midiDestRegion' 

   Using this function removes the need to use the 3 following
   functions */

EXTERN EMUSTAT APITYPE LoadSoundObject (SCID scID, STBANKOPENPTR bank,
					   enSoundObject object,
					   SoundIndex bankSoundIndex,
					   midiDest destination,
					   midiRegion midiDestRegion,
					   SFIDPTR  sfID);
//EXTERN EMUSTAT APITYPE LoadSoundObjectByMIDILoc (STBANKOPENPTR bank,
//					   enSoundObject object,
//					   midiDest source,
//					   midiDest destination,
//					   midiRegion midiDestRegion,
//					   SFIDPTR  sfID);

/* These functions provided for convenience */
EXTERN EMUSTAT APITYPE LoadSoundBank(SCID scID, STBANKOPENPTR bank,
				     midiDest destination,
				     SFIDPTR sfID);
EXTERN EMUSTAT APITYPE LoadSoundBankPreset(SCID scID, STBANKOPENPTR bank,
					   SoundIndex sourceIndex,
					   midiDest destination,
					   SFIDPTR sfID);
EXTERN EMUSTAT APITYPE LoadSoundBankInstrument(SCID scID, STBANKOPENPTR bank,
						SoundIndex bankInstIndex,
						midiDest destination,
						SFIDPTR sfID);
EXTERN EMUSTAT APITYPE LoadSoundBankSample(SCID scID, STBANKOPENPTR bank,
					   SoundIndex bankSoundIndex,
					   midiDest destination,
					   midiRegion midiDestRegion,
					   SFIDPTR  sfID);

EXTERN EMUSTAT APITYPE LoadSamplePacket(SCID scID, AWORDPTR data,
                                        DWORD dataSize,
					STPACKETSAMPLELOADPTR nextPacket);

EXTERN void    APITYPE UnloadAllSounds(SCID scID);
EXTERN void    APITYPE UnloadSoundInstance(SCID scID, SFID uiFontID);
EXTERN void    APITYPE UnloadSoundsInMIDISpace(SCID scID, enSoundObject object,
                                               midiDest dest); 

/* The following are the OLD SoundFont Bank Loading API functions. */
/* We recommend customers stop using them and start using the above
   API functions instead. */

EXTERN EMUSTAT APITYPE LoadSF(SCID scID, ACHARPTR name, BYTE userBank,
                              SFIDPTR pSFID);
#ifdef __SYS_MACINTOSH
EXTERN EMUSTAT APITYPE LoadSFMac(SCID scID, FSSpec *fspec, BYTE userBank,
                                 SFIDPTR pSFID);
#endif
EXTERN EMUSTAT APITYPE LoadSiliconSF(SCID scID, BYTE index, BYTE userBank,
                                     SFIDPTR pSFID);
EXTERN void    APITYPE UnloadSF(SCID scID, SFID sfID);
EXTERN void    APITYPE UnloadAllSF(SCID scID);

/* Loaded SoundFont and Sample Memory information and organization */
EXTERN SFID    APITYPE GetSoundInstanceFromMIDILocation(SCID scID, midiDest dest);
EXTERN BOOL    APITYPE IsPresetSelfLoaded(SCID scID, midiDest dest);

EXTERN SHORT   APITYPE BrowseBanks(SCID scID, WORD uiBankNum);
EXTERN EMUSTAT APITYPE BrowseBankPresets(SCID scID, WORD uiBankNum,
                                         STBROWSEBANKPTR pBrowseBank);
EXTERN EMUSTAT APITYPE SetSoundRAMLimits(SCID scID, DWORD linearOffsetInBytes,
                                         DWORD maxSizeInBytes);
EXTERN EMUSTAT APITYPE GetSoundRAMStatus(SCID scID, ADWORDPTR availableRAM,
                                         ADWORDPTR totalRAM);
EXTERN EMUSTAT APITYPE SwitchSFBankIndex(SCID scID, SFID fontID,
                                         WORD newUserBank);
EXTERN EMUSTAT APITYPE SwitchMIDIBankIndex(SCID scID, WORD oldBankIndex,
                                           WORD newBankIndex);
EXTERN EMUSTAT APITYPE DisablePreset(SCID scID, WORD uiBank, WORD uiPreset);
EXTERN EMUSTAT     APITYPE SetSynthMode(SCID scID, enSynthMode theMode);
EXTERN enSynthMode APITYPE GetSynthMode(SCID scID);
EXTERN EMUSTAT APITYPE SetDrumBankFullCustom(SCID scID, WORD bank);
EXTERN WORD    APITYPE GetDrumBankFullCustom(SCID scID);

/* MIDI Event API (for SoundFont sounds) */
EXTERN BYTE    APITYPE GetNumberOfMIDIPorts(SCID scID);
EXTERN EMUSTAT APITYPE SendMIDIByte(SCID scID, BYTE byMIDIPort,
                                    BYTE byMidiByte);
EXTERN EMUSTAT APITYPE SendMIDIMessage(SCID scID, BYTE byMIDIPort,
                                       ABYTEPTR midiBytes, DWORD numMIDIBytes);
EXTERN EMUSTAT APITYPE ReceiveMIDIByte(SCID scID, ABYTEPTR byMIDIByte);
EXTERN WORD    APITYPE CanReceiveMIDIByte(SCID scID);
/* For setting SysEx ID:
   0xf0 0x18 0x40 {deviceID} ...
   Value between 0 and 0x7e (126). 0x7f (127) is broadcast (ALL devices)
   Default is 0.
*/
EXTERN EMUSTAT APITYPE SetMIDISysExDeviceID(SCID scID, WORD deviceID);
EXTERN WORD    APITYPE GetMIDISysExDeviceID(SCID scID);
/* For setting MIDI Base Channel.
   Omni/Poly controller (CC 124-127) messages are only recognized if
   they come from this channel.
   Value between 1 and 16.
   Default is 1.
*/
EXTERN EMUSTAT APITYPE SetMIDIBaseChannel(SCID scID, WORD midiChannel);
EXTERN WORD    APITYPE GetMIDIBaseChannel(SCID scID);

/* For setting SE Output to FX Input per MIDI channel */
EXTERN EMUSTAT APITYPE SetEffectsBusRouting(SCID scID, BYTE byPort, WORD channel, WORD seOutputIndex, WORD fxInputIndex );
EXTERN EMUSTAT APITYPE GetEffectsBusRouting(SCID scID, BYTE byPort, WORD channel, WORD seOutputIndex, WORD *pfxInputIndex);
EXTERN EMUSTAT APITYPE SetEffectsBusSendMasterAmount(SCID scID, BYTE byPort, WORD channel, WORD seOutputIndex, LONG amount);
EXTERN EMUSTAT APITYPE GetEffectsBusSendMasterAmount(SCID scID, BYTE byPort, WORD channel, WORD seOutputIndex, LONG *pAmount);

/* Sound Event API (for SoundFonts) */
EXTERN EMUSTAT APITYPE  NoteOn(SCID scID, WORD uiChannel, WORD uiKeyNumber,
                               WORD uiVelocity);
EXTERN EMUSTAT APITYPE  NoteOff(SCID scID, WORD uiChannel, WORD uiKeyNumber,
                                WORD uiVelocity);
EXTERN EMUSTAT APITYPE  ChannelProgramChange(SCID scID, WORD uiChannel,
                                             WORD uiProgram);
EXTERN EMUSTAT APITYPE  ChannelBankChange(SCID scID, WORD uiChannel,
                                          WORD uiBank);
EXTERN EMUSTAT APITYPE  ChannelController(SCID scID, WORD uiChannel,
                                          WORD uiControllerNdx, WORD ui7BitValue);
EXTERN EMUSTAT APITYPE  ChannelKnob(SCID scID, WORD uiChannel, WORD uiKnobType,
                                    WORD uiKnobNdx, WORD ui14BitValue);
EXTERN EMUSTAT APITYPE  ChannelPressure(SCID scID, WORD uiChannel,
                                        WORD uiPressureAmount);
EXTERN EMUSTAT APITYPE  ChannelPitchBend(SCID scID, WORD uiChannel,
                                         WORD uiPitchBendAmount);
EXTERN EMUSTAT APITYPE  KeyPressure(SCID scID, WORD uiChannel, WORD uiKeyNumber,
                                    WORD uiPressureAmount);

/* Realtime audio API */
EXTERN EMUSTAT APITYPE EnableDA(SCID scID, DWORD singleBufferSize);
EXTERN EMUSTAT APITYPE DisableDA(SCID scID);
EXTERN EMUSTAT APITYPE InitDA(SCID scID, daInit APIPTR, DAIDPTR theID);
EXTERN EMUSTAT APITYPE StartDA(SCID scID, DAID theID);
EXTERN EMUSTAT APITYPE StopDA(SCID scID, DAID theID);
EXTERN BOOL    APITYPE PollDA(SCID scID, DAID theID);
EXTERN DWORD   APITYPE GetDACount(SCID scID, DAID theID);
EXTERN EMUSTAT APITYPE SendDA(SCID scID, DAID theID, AWORDPTR samples,
                              LONG count);
EXTERN EMUSTAT APITYPE ReceiveDA(SCID scID, DAID theID, AWORDPTR puiSampleData);
EXTERN EMUSTAT APITYPE TogglePauseDA(SCID scID, DAID theID);

/* Mixer API */
EXTERN EMUSTAT APITYPE SetMIDIMasterVolume(SCID scID, WORD dataValue);
EXTERN WORD    APITYPE GetMIDIMasterVolume(SCID scID);

EXTERN EMUSTAT APITYPE SetStreamOutMasterVolume(SCID scID, WORD volumeAmount);
EXTERN WORD    APITYPE GetStreamOutMasterVolume(SCID scID);
EXTERN EMUSTAT APITYPE SetStreamOutMasterPan(SCID scID, SHORT panAmount);
EXTERN SHORT   APITYPE GetStreamOutMasterPan(SCID scID);
EXTERN EMUSTAT APITYPE SetStreamOutMasterVolumeLeft(SCID scID,
                                                    WORD volumeAmount);
EXTERN WORD    APITYPE GetStreamOutMasterVolumeLeft(SCID scID);
EXTERN EMUSTAT APITYPE SetStreamOutMasterVolumeRight(SCID scID,
                                                     WORD volumeAmount);
EXTERN WORD    APITYPE GetStreamOutMasterVolumeRight(SCID scID);
EXTERN EMUSTAT APITYPE SetStreamOutMasterChorus(SCID scID, WORD chorusAmount);
EXTERN WORD    APITYPE GetStreamOutMasterChorus(SCID scID);
EXTERN EMUSTAT APITYPE SetStreamOutMasterReverb(SCID scID, WORD reverbAmount);
EXTERN WORD    APITYPE GetStreamOutMasterReverb(SCID scID);
EXTERN BOOL    APITYPE ToggleStreamOutMasterMute(SCID scID);
EXTERN BOOL    APITYPE IsStreamOutMasterMuted(SCID scID);

EXTERN EMUSTAT APITYPE SetMonitorInMasterVolume(SCID scID, WORD volumeAmount);
EXTERN WORD    APITYPE GetMonitorInMasterVolume(SCID scID);
EXTERN EMUSTAT APITYPE SetMonitorInMasterPan(SCID scID, SHORT panAmount);
EXTERN SHORT   APITYPE GetMonitorInMasterPan(SCID scID);
EXTERN EMUSTAT APITYPE SetMonitorInMasterVolumeLeft(SCID scID,
                                                    WORD volumeAmount);
EXTERN WORD    APITYPE GetMonitorInMasterVolumeLeft(SCID scID);
EXTERN EMUSTAT APITYPE SetMonitorInMasterVolumeRight(SCID scID,
                                                     WORD volumeAmount);
EXTERN WORD    APITYPE GetMonitorInMasterVolumeRight(SCID scID);
EXTERN EMUSTAT APITYPE SetMonitorInMasterChorus(SCID scID, WORD chorusAmount);
EXTERN WORD    APITYPE GetMonitorInMasterChorus(SCID scID);
EXTERN EMUSTAT APITYPE SetMonitorInMasterReverb(SCID scID, WORD reverbAmount);
EXTERN WORD    APITYPE GetMonitorInMasterReverb(SCID scID);
EXTERN BOOL    APITYPE ToggleMonitorInMasterMute(SCID scID);
EXTERN BOOL    APITYPE IsMonitorInMasterMuted(SCID scID);

/* 3D Audio API */
EXTERN EMUSTAT APITYPE Set3DAudioMaxDistance(SCID scID, SHORT shDist);
EXTERN EMUSTAT APITYPE Set3DAudioDopplerEffect(SCID scID, WORD wAmount);
EXTERN DWORD   APITYPE Create3DAudioEmitter(SCID scID, SHORT x, SHORT y,
                                            SHORT z);
EXTERN EMUSTAT APITYPE Destroy3DAudioEmitter(SCID scID, DWORD dwEmitterId);
EXTERN EMUSTAT APITYPE DestroyAll3DAudioEmitters(SCID scID);
EXTERN EMUSTAT APITYPE Set3DAudioEmitterMIDISource(SCID scID, DWORD dwEmitterId,
                                                   WORD wBankNum,
                                                   WORD wPresetNum,
                                                   BYTE byNote);
EXTERN EMUSTAT APITYPE Set3DAudioEmitterPosition(SCID scID, DWORD dwEmitterId,
                                                 SHORT x, SHORT y, SHORT z);
EXTERN EMUSTAT APITYPE Set3DAudioEmitterOrientation(SCID scID,
                                                    DWORD dwEmitterId, SHORT x,
                                                    SHORT y, SHORT z);
EXTERN EMUSTAT APITYPE Set3DAudioEmitterSoundState(SCID scID, DWORD dwEmitterId,
                                                   Emu3DSoundState soundState);
EXTERN EMUSTAT APITYPE Set3DAudioEmitterGain(SCID scID, DWORD dwEmitterId,
                                             WORD wGain);
EXTERN EMUSTAT APITYPE Set3DAudioEmitterPitchInc(SCID scID, DWORD dwEmitterId,
                                                 SHORT shIncrement);
EXTERN EMUSTAT APITYPE Set3DAudioEmitterDelay(SCID scID, DWORD dwEmitterId,
                                              WORD wNumSamples);
EXTERN DWORD   APITYPE Create3DAudioReceiver(SCID scID, SHORT x, SHORT y,
                                             SHORT z);
EXTERN EMUSTAT APITYPE Destroy3DAudioReceiver(SCID scID, DWORD dwReceiverId);
EXTERN EMUSTAT APITYPE DestroyAll3DAudioReceivers(SCID scID);
EXTERN DWORD   APITYPE Get3DAudioActiveReceiver(SCID scID);
EXTERN EMUSTAT APITYPE Set3DAudioActiveReceiver(SCID scID, DWORD dwReceiverId);
EXTERN EMUSTAT APITYPE Set3DAudioReceiverPosition(SCID scID, DWORD dwReceiverId,
                                                  SHORT x, SHORT y, SHORT z);

/* Vienna API */
EXTERN EMUSTAT APITYPE ViennaStart(SCID scID);
EXTERN EMUSTAT APITYPE ViennaEnd(SCID scID);
EXTERN EMUSTAT APITYPE ViennaLoadSample(SCID scID, AVOIDPTR smplObj);
EXTERN EMUSTAT APITYPE ViennaFreeSample(SCID scID, DWORD dwSampleHandle);
EXTERN EMUSTAT APITYPE ViennaPlaySample(SCID scID, AVOIDPTR playObj);
EXTERN EMUSTAT APITYPE ViennaLoadPreset(SCID scID, AVOIDPTR prst);
EXTERN EMUSTAT APITYPE ViennaFreePreset(SCID scID, AVOIDPTR prst,
                                        ADWORDPTR availRAM);
EXTERN EMUSTAT APITYPE ViennaNoteOn(SCID scID, BYTE byKey, BYTE byVel);
EXTERN EMUSTAT APITYPE ViennaNoteOff(SCID scID, BYTE byKey, BYTE byVel);
EXTERN EMUSTAT APITYPE ViennaController(SCID scID, BYTE byContNum,
                                        BYTE byContVal);
EXTERN EMUSTAT APITYPE ViennaPitchBend(SCID scID, BYTE byPbendLSB,
                                       BYTE byPbendMSB);
EXTERN EMUSTAT APITYPE ViennaChannelPressure(SCID scID, BYTE byChPres);
EXTERN EMUSTAT APITYPE ViennaSysEx(SCID scID, ABYTEPTR sysex);
EXTERN EMUSTAT APITYPE ViennaGetDRAMSize(SCID scID, ADWORDPTR totalRAM,
                                         ADWORDPTR availRAM);

/* Feedback API */
EXTERN EMUSTAT APITYPE SetupCallBack(SCID scID, downLoadCallBack theCallBack,
                                     BYTE thePercent);
#endif
