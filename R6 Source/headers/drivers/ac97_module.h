/* ac97_module.h */

/* Interface description for drivers interfacing with AC97 compliant codecs */
/* Copyright © 1998-2001 Be Incorporated. All rights reserved. */

#ifndef __AC97_MODULE_H
#define __AC97_MODULE_H

#include <module.h>
#if _SOUND_COMPATIBILITY
#include "sound.h"
#endif
#include <KernelExport.h>
#include "gaplug.h"


#if _SOUND_COMPATIBILITY
#define B_AC97_MODULE_NAME "media/ac97_module/v2"
#endif

#define B_AC97_MODULE_NAME_V3 "media/ac97_module/v3"
#define B_AC97_MAX_REGS 64
#define AC97_RESET_HARDWARE 0x1

#define CACHE_BIT(OFFSET) (1LL<<(OFFSET))

typedef status_t (*ac97_write_ft)(void* host, uchar offset,
								  uint16 value, uint16 mask);
typedef uint16 (*ac97_read_ft)(void* host, uchar offset);

//	Do not use the members of this struct; treat it as opaque. Actual use in
//	the AC97 module may vary from these names.
typedef struct ac97_t_v3 {
  void* host;
  void* hw;
} ac97_t_v3;

#if _SOUND_COMPATIBILITY

	typedef struct ac97_t_v2 {
	  void* host;
	  void* hw;
	  ac97_write_ft Write;
	  ac97_read_ft Read;	
	  int32 lock;				/* spinlock for Read/Write */
	  sem_id cache_ben_sem;
	  int32 cache_ben_atom;
	  uint64 cache_valid;
	  uint16 cache[B_AC97_MAX_REGS];
	} ac97_t_v2;

	typedef struct ac97_module_info {
	  module_info module;
	
	  status_t (*ac97init)(ac97_t_v2* ac97, void* host,
						   ac97_write_ft codec_write_proc,
						   ac97_read_ft codec_read_proc,
						   bool init_hw);
	  void (*ac97uninit)(ac97_t_v2* ac97);
	
	  status_t (*AC97_SoundSetup)(ac97_t_v2* ac97, sound_setup* ss);
	  status_t (*AC97_GetSoundSetup)(ac97_t_v2* ac97, sound_setup* ss);
	  status_t (*AC97_GameControl)(void* ac97, uint32 iop,
								   void* data, size_t length);
	  status_t (*AC97_CachedCodecWrite)(ac97_t_v2* ac97, uchar offset,
										uint16 value, uint16 mask);
	  uint16 (*AC97_CachedCodecRead)(ac97_t_v2* ac97, uchar offset);
	} ac97_module_info;

	typedef ac97_t_v2 ac97_t;
#else
	typedef ac97_t_v3 ac97_t;
#endif	

typedef struct ac97_codec_info_v3 ac97_codec_info;
struct ac97_codec_info_v3
{
	uint16		capabilities[2];	//	reset reg (00h), extended caps (28h)
	uint16		vendor_id[2];		//	vendor ID (7Ch and 7Eh)
	uint32		codec_info;			//	codec specific (rev ID etc)

	uint32		channel_counts;
	uint32		frame_rates;
	uint32		formats;
	float		cvsr_min;
	float		cvsr_max;

	char		name[16];			//	"cs4297" or similar short chip name

	uint32		_reserved_[8];
};

enum ac97_mode_select;
typedef enum ac97_mode_select ac97_mode_select;
enum ac97_mode_select
{
	AC97_ALL_INFO,
	AC97_ADC_INFO,
	AC97_DAC_INFO,
	AC97_CURRENT_ADC_INFO,
	AC97_CURRENT_DAC_INFO
};

typedef struct virtual_control_data virtual_control_data;
struct virtual_control_data {
	ac97_t_v3 *	codec;
	int32		control_id;
	uint32		type;
	uint32		_reserved[5];
};

typedef struct ac97_module_info_v3 {
  module_info module;

  status_t	(*AC97_init)(ac97_t_v3* ac97, void* host, ac97_write_ft codec_write_proc, ac97_read_ft codec_read_proc, uint32 flags);
  void		(*AC97_uninit)(ac97_t_v3* ac97);
  status_t	(*AC97_game_control)(void* ac97, uint32 iop, void* data, size_t length);
  void		(*AC97_plug_get_mixer_description)(ac97_t_v3* ac97, plug_api * i_info, int index, game_mixer_info * o_mixers);
  void		(*AC97_plug_get_mixer_controls)(ac97_t_v3* ac97, plug_api * i_info, const gaplug_get_mixer_controls * io_controls);
  void		(*AC97_plug_get_mixer_control_values)(ac97_t_v3* ac97, plug_api * i_info, const gaplug_mixer_control_values * io_request);
  void		(*AC97_plug_set_mixer_control_values)(ac97_t_v3* ac97, plug_api * i_info, const gaplug_mixer_control_values * io_request);
  status_t  (*AC97_cached_codec_write)(ac97_t_v3* ac97, uchar offset, uint16 value, uint16 mask);
  uint16    (*AC97_cached_codec_read)(ac97_t_v3* ac97, uchar offset);
  //	get_codec_info works on all mode_select values
  status_t	(*AC97_get_codec_info)(ac97_t_v3* ac97, ac97_mode_select what, ac97_codec_info * o_info);
  //	get/set_codec_format works only on CURRENT mode_select values
  status_t	(*AC97_get_codec_format)(ac97_t_v3* ac97, ac97_mode_select what, game_codec_format * o_fmt);
  status_t	(*AC97_set_codec_format)(ac97_t_v3* ac97, ac97_mode_select what, const game_codec_format * i_fmt);
  //  binding
  status_t	(*AC97_make_virtual_codec)(ac97_t_v3 * virtual_codec, virtual_control_data * virtual_control_data, int32 i_count );
  uint32	_reserved_[3];
} ac97_module_info_v3;
 
/*
 *  Audio Codec '97 registers
 *  For more details see AC'97 Component Specification, revision 2.1,
 *  by Intel Corporation (http://developer.intel.com).
 */

#define AC97_RESET			0x00	/* Reset */
#define AC97_MASTER			0x02	/* Master Volume */
#define AC97_HEADPHONE		0x04	/* Headphone Volume (optional) */
#define AC97_MASTER_MONO	0x06	/* Master Volume Mono (optional) */
#define AC97_MASTER_TONE	0x08	/* Master Tone (Bass & Treble) (optional) */
#define AC97_PC_BEEP		0x0a	/* PC Beep Volume (optinal) */
#define AC97_PHONE			0x0c	/* Phone Volume (optional) */
#define AC97_MIC			0x0e	/* MIC Volume */
#define AC97_LINE			0x10	/* Line In Volume */
#define AC97_CD				0x12	/* CD Volume */
#define AC97_VIDEO			0x14	/* Video Volume (optional) */
#define AC97_AUX			0x16	/* AUX Volume (optional) */
#define AC97_PCM			0x18	/* PCM Volume */
#define AC97_REC_SEL		0x1a	/* Record Select */
#define AC97_REC_GAIN		0x1c	/* Record Gain */
#define AC97_REC_GAIN_MIC	0x1e	/* Record Gain MIC (optional) */
#define AC97_GENERAL_PURPOSE	0x20	/* General Purpose (optional) */
#define AC97_3D_CONTROL		0x22	/* 3D Control (optional) */
#define AC97_RESERVED		0x24	/* Reserved */
#define AC97_POWERDOWN		0x26	/* Powerdown control / status */
#define AC97_CTRL_STAT	0x26

/* range 0x28-0x3a - AUDIO */
#define AC97_EXTENDED_ID	0x28
#define AC97_EXTENDED_CTRL_STAT	0x2A
#define AC97_FRONT_DAC_RATE	0x2C
#define AC97_SURR_DAC_RATE	0x2E
#define AC97_LFE_DAC_RATE	0x30
#define AC97_LR_ADC_RATE	0x32
#define AC97_MIC_ADC_RATE	0x34
#define AC97_2_C_LFE		0x36	/* 6-channel volume control (optional) */
#define AC97_2_L_R_SURR		0x38	/* 4-channel volume control (optional) */
/* range 0x3c-0x58 - MODEM */
#define AC97_2_LINE1_DAC_ADC	0x46	/* modem dac/adc level conrol */
#define AC97_2_LINE2_DAC_ADC	0x48	
#define AC97_2_HANDSET_DAC_ADC	0x48	
/* range 0x5a-0x7b - Vendor Specific */
#define AC97_VENDOR_ID1		0x7c	/* Vendor ID1 */
#define AC97_VENDOR_ID2		0x7e	/* Vendor ID2 (revision) */

/* AC'97 Quad codec type */
#define AC97_QUAD_CODEC_NONE			(420)							// non-quad codec
#define AC97_QUAD_CODEC_ASYMMETRIC		(AC97_QUAD_CODEC_NONE + 1)	// asymmetric quad codec
#define AC97_QUAD_CODEC_SYMMETRIC		(AC97_QUAD_CODEC_NONE + 2)	// symmetric quad codec

/* AC'97 CODEC 3D Effect Type */
#define AC97_CODEC_3D_NONE              (410)     
#define AC97_CODEC_3D_FIXED             (AC97_CODEC_3D_NONE + 1)
#define AC97_CODEC_3D_VARIABLE          (AC97_CODEC_3D_NONE + 2)
#define AC97_CODEC_3D_VAR_CENTER_ONLY	(AC97_CODEC_3D_NONE + 3)
#define AC97_CODEC_3D_VAR_DEPTH_ONLY	(AC97_CODEC_3D_NONE + 4)

#endif
