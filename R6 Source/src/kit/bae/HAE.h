/*****************************************************************************/
/*
** "HAE.h"
**
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
**
*/
/*****************************************************************************/

#include "BAE.h"
#include "BAEMidiInput.h"

#ifndef HAE_AUDIO
#define HAE_AUDIO

#define	HAE_DROP_SAMPLE					BAE_DROP_SAMPLE
#define	HAE_2_POINT_INTERPOLATION		BAE_2_POINT_INTERPOLATION
#define	HAE_LINEAR_INTERPOLATION		BAE_LINEAR_INTERPOLATION
#define HAETerpMode						BAETerpMode

#define HAE_8K							BAE_8K
#define	HAE_11K							BAE_11K
#define HAE_11K_TERP_22K				BAE_11K_TERP_22K
#define	HAE_22K							BAE_22K
#define HAE_22K_TERP_44K				BAE_22K_TERP_44K
#define	HAE_24K							BAE_24K
#define	HAE_44K							BAE_44K
#define	HAE_48K							BAE_48K
#define	HAEQuality						BAEQuality

#define HAE_NONE						BAE_NONE
#define HAE_USE_16						BAE_USE_16
#define HAE_USE_STEREO					BAE_USE_STEREO
#define HAE_DISABLE_REVERB				BAE_DISABLE_REVERB
#define HAE_STEREO_FILTER				BAE_STEREO_FILTER
#define	HAEAudioModifiers				BAEAudioModifiers

#define	HAE_REVERB_NO_CHANGE			BAE_REVERB_NO_CHANGE
#define	HAE_REVERB_NONE					BAE_REVERB_NONE
#define	HAE_REVERB_TYPE_1				BAE_REVERB_TYPE_1
#define	HAE_REVERB_TYPE_2 				BAE_REVERB_TYPE_2
#define	HAE_REVERB_TYPE_3 				BAE_REVERB_TYPE_3
#define	HAE_REVERB_TYPE_4 				BAE_REVERB_TYPE_4
#define	HAE_REVERB_TYPE_5 				BAE_REVERB_TYPE_5
#define	HAE_REVERB_TYPE_6 				BAE_REVERB_TYPE_6
#define	HAE_REVERB_TYPE_7 				BAE_REVERB_TYPE_7
#define	HAE_REVERB_TYPE_8 				BAE_REVERB_TYPE_8
#define	HAE_REVERB_TYPE_9 				BAE_REVERB_TYPE_9
#define	HAE_REVERB_TYPE_10 				BAE_REVERB_TYPE_10
#define	HAE_REVERB_TYPE_11				BAE_REVERB_TYPE_11
#define HAE_REVERB_TYPE_COUNT			BAE_REVERB_TYPE_COUNT
#define	HAEReverbMode					BAEReverbType

#define HAE_FIXED						BAE_FIXED
#define HAE_UNSIGNED_FIXED				BAE_UNSIGNED_FIXED

#define	HAEPathName						BAEPathName

#define	HAE_NO_ERROR					BAE_NO_ERROR
#define	HAE_PARAM_ERR					BAE_PARAM_ERR
#define	HAE_MEMORY_ERR					BAE_MEMORY_ERR
#define	HAE_BAD_INSTRUMENT				BAE_BAD_INSTRUMENT
#define	HAE_BAD_MIDI_DATA				BAE_BAD_MIDI_DATA
#define	HAE_ALREADY_PAUSED				BAE_ALREADY_PAUSED
#define	HAE_ALREADY_RESUMED				BAE_ALREADY_RESUMED
#define	HAE_DEVICE_UNAVAILABLE			BAE_DEVICE_UNAVAILABLE
#define	HAE_NO_SONG_PLAYING				BAE_NO_SONG_PLAYING
#define	HAE_STILL_PLAYING				BAE_STILL_PLAYING
#define	HAE_TOO_MANY_SONGS_PLAYING		BAE_TOO_MANY_SONGS_PLAYING
#define	HAE_NO_VOLUME					BAE_NO_VOLUME
#define	HAE_GENERAL_ERR					BAE_GENERAL_ERR
#define	HAE_NOT_SETUP					BAE_NOT_SETUP
#define	HAE_NO_FREE_VOICES				BAE_NO_FREE_VOICES
#define	HAE_STREAM_STOP_PLAY			BAE_STREAM_STOP_PLAY
#define	HAE_BAD_FILE_TYPE				BAE_BAD_FILE_TYPE
#define	HAE_GENERAL_BAD					BAE_GENERAL_BAD
#define	HAE_BAD_FILE					BAE_BAD_FILE
#define	HAE_NOT_REENTERANT				BAE_NOT_REENTERANT
#define	HAE_BAD_SAMPLE					BAE_BAD_SAMPLE
#define	HAE_BUFFER_TO_SMALL				BAE_BUFFER_TO_SMALL
#define HAE_MIDI_MANAGER_FAILED			BAE_MIDI_MANAGER_FAILED
#define HAE_OMS_FAILED					BAE_OMS_FAILED
#define HAE_MIDI_MANGER_NOT_THERE		BAE_MIDI_MANGER_NOT_THERE
#define HAE_BAD_BANK					BAE_BAD_BANK
#define HAE_BAD_SAMPLE_RATE				BAE_BAD_SAMPLE_RATE
#define HAE_TOO_MANY_SAMPLES			BAE_TOO_MANY_SAMPLES
#define HAE_UNSUPPORTED_FORMAT			BAE_UNSUPPORTED_FORMAT
#define HAE_FILE_IO_ERROR				BAE_FILE_IO_ERROR
#define HAE_SAMPLE_TO_LARGE				BAE_SAMPLE_TO_LARGE
#define HAE_UNSUPPORTED_HARDWARE		BAE_UNSUPPORTED_HARDWARE
#define HAE_ABORTED						BAE_ABORTED
#define HAE_FILE_NOT_FOUND				BAE_FILE_NOT_FOUND
#define HAE_ERROR_COUNT					BAE_ERROR_COUNT

#define HAEInfoType						BAEInfoType
#define HAEInfoTypes					BAEInfoType
#define HAEMetaType						BAEMetaType
#define HAEMetaTypes					BAEMetaType

#define HAE_AIFF_TYPE					BAE_AIFF_TYPE
#define HAE_WAVE_TYPE					BAE_WAVE_TYPE
#define HAE_AU_TYPE						BAE_AU_TYPE
#define HAE_INVALID_TYPE				BAE_INVALID_TYPE
#define HAE_MPEG_TYPE					BAE_MPEG_TYPE
#define HAE_GROOVOID					BAE_GROOVOID
#define HAE_RMF							BAE_RMF
#define HAEFileType						BAEFileType

#define	HAE_BOOL						BAE_BOOL
#define	HAE_INSTRUMENT					BAE_INSTRUMENT
#define	HAE_FIXED						BAE_FIXED

#define HAEControlerCallbackPtr 		BAEControlerCallbackPtr
#define HAETaskCallbackPtr				BAETaskCallbackPtr
#define	HAETimeCallbackPtr				BAETimeCallbackPtr
#define	HAEMetaEventCallbackPtr			BAEMetaEventCallbackPtr
#define HAEDoneCallbackPtr				BAEDoneCallbackPtr
#define	HAELoopDoneCallbackPtr			BAELoopDoneCallbackPtr
#define	HAEOutputCallbackPtr			BAEOutputCallbackPtr
#define HAESampleFrameCallbackPtr		BAESampleFrameCallbackPtr

#define HAE_MAX_VOICES					BAE_MAX_VOICES
#define HAE_MAX_MIDI_VOLUME				BAE_MAX_MIDI_VOLUME

#define HAE_MIN_STREAM_BUFFER_SIZE		BAE_MIN_STREAM_BUFFER_SIZE

#define HAE_FULL_LEFT_PAN				BAE_FULL_LEFT_PAN
#define HAE_CENTER_PAN					BAE_CENTER_PAN
#define HAE_FULL_RIGHT_PAN				BAE_FULL_RIGHT_PAN

#define	HAEAudioInfo					BAEAudioInfo
#define	HAESampleInfo					BAESampleInfo

#define HAEStreamMessage				BAEStreamMessage
#define HAE_STREAM_NULL					BAE_STREAM_NULL
#define HAE_STREAM_CREATE				BAE_STREAM_CREATE
#define HAE_STREAM_DESTROY				BAE_STREAM_DESTROY
#define HAE_STREAM_GET_DATA				BAE_STREAM_GET_DATA
#define HAE_STREAM_GET_SPECIFIC_DATA	BAE_STREAM_GET_SPECIFIC_DATA

#define HAEStreamData					BAEStreamData
#define HAEStreamObjectProc				BAEStreamObjectProc

// exporter types
#define HAECompressionType				BAECompressionType
#define HAEEncryptionType				BAEEncryptionType

#define HAE_ENCRYPTION_NONE				BAE_ENCRYPTION_NONE
#define HAE_ENCRYPTION_NORMAL			BAE_ENCRYPTION_NORMAL
#define HAE_ENCRYPTION_TYPE_COUNT		BAE_ENCRYPTION_TYPE_COUNT
#define HAE_COMPRESSION_NONE			BAE_COMPRESSION_NONE
#define HAE_COMPRESSION_LOSSLESS		BAE_COMPRESSION_LOSSLESS
#define HAE_COMPRESSION_IMA				BAE_COMPRESSION_IMA
#define HAE_COMPRESSION_MPEG_32			BAE_COMPRESSION_MPEG_32
#define HAE_COMPRESSION_MPEG_40			BAE_COMPRESSION_MPEG_40
#define HAE_COMPRESSION_MPEG_48			BAE_COMPRESSION_MPEG_48
#define HAE_COMPRESSION_MPEG_56			BAE_COMPRESSION_MPEG_56
#define HAE_COMPRESSION_MPEG_64			BAE_COMPRESSION_MPEG_64
#define HAE_COMPRESSION_MPEG_80			BAE_COMPRESSION_MPEG_80
#define HAE_COMPRESSION_MPEG_96			BAE_COMPRESSION_MPEG_96
#define HAE_COMPRESSION_MPEG_112		BAE_COMPRESSION_MPEG_112
#define HAE_COMPRESSION_MPEG_128		BAE_COMPRESSION_MPEG_128
#define HAE_COMPRESSION_MPEG_160		BAE_COMPRESSION_MPEG_160
#define HAE_COMPRESSION_MPEG_192		BAE_COMPRESSION_MPEG_192
#define HAE_COMPRESSION_MPEG_224		BAE_COMPRESSION_MPEG_224
#define HAE_COMPRESSION_MPEG_256		BAE_COMPRESSION_MPEG_256
#define HAE_COMPRESSION_MPEG_320		BAE_COMPRESSION_MPEG_320
#define HAE_COMPRESSION_TYPE_COUNT		BAE_COMPRESSION_TYPE_COUNT

#define HAE_MAX_SONGS					BAE_MAX_SONGS

#define HAE_MIDI_MANAGER_DEVICE			BAE_MIDI_MANAGER_DEVICE
#define HAE_OMS_DEVICE					BAE_OMS_DEVICE

#define HAECPUType						BAECPUType
#define HAE_CPU_UNKNOWN					BAE_CPU_UNKNOWN
#define HAE_CPU_POWERPC					BAE_CPU_POWERPC
#define HAE_CPU_SPARC					BAE_CPU_SPARC
#define HAE_CPU_JAVA					BAE_CPU_JAVA
#define HAE_CPU_MIPS					BAE_CPU_MIPS
#define HAE_CPU_INTEL_PENTIUM			BAE_CPU_INTEL_PENTIUM
#define HAE_CPU_INTEL_PENTIUM3			BAE_CPU_INTEL_PENTIUM3
#define HAE_CPU_CRAY_XMP3				BAE_CPU_CRAY_XMP3
#define HAE_CPU_COUNT					BAE_CPU_COUNT

#define HAE_ASSERT						BAE_ASSERT
#define HAE_VERIFY						BAE_VERIFY

// compatibility types
#define	HAEAudioMixer					BAEOutputMixer
#define HAEMixer						BAEOutputMixer
#define HAECapture						BAEInputMixer
#define	HAEAudioNoise					BAENoise
#define	HAEMidiDirect					BAEMidiSynth
#define HAEMidiExternal					BAEMidiInput
#define HAEMidiInputDevice				BAEMidiInput
#define	HAEMidiFile						BAEMidiSong
#define	HAERMFFile	  					BAERmfSong
#define	HAEMod							BAEModSong
#define HAEGroup						BAENoiseGroup
#define HAEErr							BAEResult
#define HAEReverbMode					BAEReverbType
#define HAESound						BAESound
#define HAESoundStream					BAESoundStream

#endif	// HAE_AUDIO
