f("ac97 is SigmaTel 9744\n");              
                   break; 
               default: 
                   dprintf("ac97 vendor is Sigmatel.\n"); 
                   break; 
               } 
		    } /*case*/
	        break; 
		case VENDOR_WOLFSON: 
	        dprintf("ac97 vendor is Wolfson.\n"); 
	        break; 
		case VENDOR_ANALOG_DEVICES: 
        { /*VAD*/
			switch(ac97hw->VendorID.ch.Rev) 
			{ 
				case 0x00: 
				case 0x03://I do not have specs for this release but it supports SampleRateControl as AD1819 
				{ 
					dprintf("ac97 is Analog Devices AD1819/AD1819A/...\n"); 
					// dprintf("AD1819SerialConfiguration = 0x%x\n",ac97->Read(ac97->host, 0x74)); 
					dprintf("AD1819SerialConfiguration = 0x%x\n",ac97->Read(ac97->host, 0x74)); 
					if (reset_hw) 
						(*ac97->Write) (ac97->host, 0x74, (0x1<<11), (0x1<<11)); 
					dprintf("AD1819SerialConfiguration = 0x%x\n",ac97->Read(ac97->host, 0x74)); 
					ac97hw->VendSpecific.AD.OldSampleRateControlRegs = TRUE; 
					ac97hw->ExtendedAudioStatus = 0x1; 
				} 
				break; 
                case 0x40: 
	                dprintf("ac97 is Analog Devices AD1881\n"); 
	                break; 
                default: 
					//I do not have the specs for this AD ac97 codec revision 
					dprintf("ac97 vendor is Analog Devices.\n"); 
					dprintf("ac97hw->ExtendedAudioStatus = 0x%x\n",ac97hw->ExtendedAudioStatus); 
					if((ac97hw->ExtendedAudioStatus & 0x1) == 0) 
					{//but it does not support Variable Rate by the standard way 
				        dprintf("Initial DAC rate is %d, ADC rate is %d\n",	ac97->Read(ac97->host, 0x7a),
				        													ac97->Read(ac97->host, 0x78)); 
				
				        if( (ac97->Read(ac97->host, 0x7a) == 0xBB80) &&
				        	(ac97->Read(ac97->host, 0x78) == 0xBB80) ) 
				        {//And, yes, support using 0x7a and 0x78 regs 
			                dprintf("ac97 Analog Devices old variable rate control.\n"); 
			                ac97hw->VendSpecific.AD.OldSampleRateControlRegs = TRUE; 
			                ac97hw->ExtendedAudioStatus = 0x1; 
				        } 
					} 
					break; 
                } 
	        } /* VAD */
	        break; 
		default: 
        	break; 
		} 
	
	if (reset_hw) 
	{ 
	  /* Set default values (they will be re-set by MediaServer later, anyway) 
	   */ 
	  cached_codec_write_old(ac97, AC97_MASTER, 0x0000, 0xffff); 
	  cached_codec_write_old(ac97, AC97_PCM, 0x0808, 0xffff); 
	  cached_codec_write_old(ac97, AC97_REC_GAIN, 0x0000, 0xffff); 
	  cached_codec_write_old(ac97, 0x2c, 0xac44, 0xffff); 
	  cached_codec_write_old(ac97, 0x32, 0xac44, 0xffff); 
	} 
	
	return B_OK; 
}

/****************************************/
/* Module stuff here.........  			*/
/****************************************/

static status_t
std_ops(int32 op, ...)
{
	dbprintf(("ac97_module: std_ops 0x%lx\n", op));
	
	switch(op) {
	case B_MODULE_INIT:
	case B_MODULE_UNINIT:
		return B_OK;
	default:
		return B_ERROR;
	}
}


static ac97_module_info ac97_module = 
{
	// the module_info struct
	{
		B_AC97_MODULE_NAME,
		// if your module can't be unloaded for some reason,
		// set this to be B_KEEP_LOADED instead of zero.
		0,
		std_ops
	},
	ac97init_old,
	ac97uninit_old,
	AC97_SoundSetup,
	AC97_GetSoundSetup,
	AC97_GameControl_Is_Gone,
	cached_codec_write_old,
	cached_codec_read_old
};

static ac97_module_info_v3 ac97_module_v3 = 
{
	// the module_info struct
	{
		B_AC97_MODULE_NAME_V3,
		// if your module can't be unloaded for some reason,
		// set this to be B_KEEP_LOADED instead of zero.
		0,
		std_ops
	},
	AC97_init,
	AC97_uninit,
	AC97_GameControl_Is_Gone,
	AC97_plug_get_mixer_description,
	AC97_plug_get_mixer_controls,
	AC97_plug_get_mixer_control_values,
	AC97_plug_set_mixer_control_values,
	AC97_cached_codec_write,
	AC97_cached_codec_read,
	AC97_get_codec_info,
	AC97_get_codec_format,
	AC97_set_codec_format,
	AC97_make_virtual_codec,
	{ 0, 0, 0}
};

_EXPORT void *modules[] =
{
	&ac97_module,
	&ac97_module_v3,
	NULL
};

                                                                                                                                          /************************************************************************/
/*                                                                      */
/*                              debug.h                              	*/
/*                                                                      */
/* 	Developed by Mikhael Khaymov						*/
/* 			alt.software inc.  www.altsoftware.com 						*/
/************************************************************************/

//#define DEBUG

#ifdef DEBUG
	#ifndef DEBUG_LEVEL
		#define DEBUG_LEVEL 1
	#endif
#endif

#ifdef DEBUG
	#define dbprintf(x) dprintf x
	#define DB_PRINTF(x) {dprintf(__FILE__); dprintf(" "); dprintf x;}
#else
	#define dbprintf(x) 
	#define DB_PRINTF(x)
#endif

#define DB_PRINTF1(x)
#define DB_PRINTF2(x)

                                                                                                                                                                                                                                                         #include <driver_settings.h>
#include <string.h>
#include "ac97_misc.h"
#include <game_audio.h