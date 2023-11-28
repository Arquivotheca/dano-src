f(("AC97: ExtendedAudioStatus = 0x%x\n",ac97hw->ExtendedAudioStatus));
				if((ac97hw->ExtendedAudioStatus & 0x1) == 0)
				{//but it does not support Variable Rate by the standard way
					dbprintf(("AC97: Initial DAC rate is %d, ADC rate is %d\n",ac97hw->Read(ac97->host, 0x7a),ac97hw->Read(ac97->host, 0x78)));

					if( (ac97hw->Read(ac97->host, 0x7a) == 0xBB80) &&
							(ac97hw->Read(ac97->host, 0x78) == 0xBB80) )
					{//And, yes, support using 0x7a and 0x78 regs 
						dbprintf(("AC97: ac97 Analog Devices old variable rate control.\n"));
						ac97hw->VendSpecific.AD.OldSampleRateControlRegs = TRUE;
						ac97hw->ExtendedAudioStatus = 0x1;
					}
				}
				break;
		 	}
		}
		break;
	default:
		dprintf("AC97: codec id is 0x%x\n",AC97_VENDOR_ID(ac97hw->VendorID.ch.F, ac97hw->VendorID.ch.S,ac97hw->VendorID.ch.T) );
		break;
	}

	if (flags & AC97_RESET_HARDWARE)
	{
	  /* Set default values (they will be re-set by MediaServer later, anyway)
	   */
	  AC97_cached_codec_write(ac97, AC97_MASTER, 0x0000, 0xffff);
	  AC97_cached_codec_write(ac97, AC97_PCM, 0x0808, 0xffff);
	  AC97_cached_codec_write(ac97, AC97_REC_GAIN, 0x0000, 0xffff);

	  AC97_cached_codec_write(ac97, 0x2c, 0xBB80, 0xffff);
	  AC97_cached_codec_write(ac97, 0x32, 0xBB80, 0xffff);

		configurable_reset(ac97);
	}



	// set up controls for this particular codec
	if (init_ac97_codec_controls( ac97hw ) != B_OK) {
		return ENOMEM;
	}


	return B_OK;
}

status_t AC97_SoundSetup(ac97_t_v2* ac97, sound_setup *ss)
{
	ac97hw_t* ac97hw = (ac97hw_t*)ac97->hw;
	bool mute;

	// Set master Out volume attenuation 
	// for now, hardcode to max., since there are no controls in the control panel 
	cached_codec_write_old(ac97, AC97_MASTER,  
			0,					   
			0xffff
		);
		
	// Set headphones Out volume attenuation
	cached_codec_write_old(ac97,AC97_HEADPHONE, 
			0,					   
			0xffff
		);
		
	// Set master mono Out volume attenuation
	cached_codec_write_old(ac97,AC97_MASTER_MONO, 
			0,
			0xffff
		);

	// Set Mic Volume
	// no separate controls for the left and right mute, so set both enable if one is enable
	mute = ss->left.mic_gain_enable || ss->right.mic_gain_enable;
	cached_codec_write_old(ac97, AC97_MIC,  
			(ss->loop_attn / 2)
			 | ((ss->loop_enable) ? 0 : 0x8000)
			 | (mu