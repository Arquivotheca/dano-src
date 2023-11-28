e_mixer_control));
}

void
AC97_plug_get_mixer_control_values(
	ac97_t_v3* ac97,
	plug_api * i_info,
	const gaplug_mixer_control_values * io_request)
{
	int ix;
	dbprintf(("AC97: get_mixer_control_values \n" ));
	i_info = i_info;
	for (ix = 0; ix < io_request->i_count; ix++)
	{
		get_control_value( io_request->i_values[ix].i_cookie, io_request->i_values[ix].io_value, ac97 );
	}
}

void
AC97_plug_set_mixer_control_values(
	ac97_t_v3* ac97,
	plug_api * i_info,
	const gaplug_mixer_control_values * io_request)
{
	int ix;
	dbprintf(("AC97: set_mixer_control_values\n"));
	i_info = i_info;
	for (ix=0; ix < io_request->i_count; ix++)
	{
		set_control_value( io_request->i_values[ix].i_cookie, io_request->i_values[ix].io_value, ac97);
	}
}



//	get/set_codec_format works on all mode_select values
status_t
AC97_get_codec_info(ac97_t_v3* ac97, ac97_mode_select what, ac97_codec_info * o_info)
{
	ac97hw_t * ac97hw = (ac97hw_t*)ac97->hw;

	o_info->capabilities[0] = AC97_cached_codec_read(ac97, AC97_RESET);
	o_info->capabilities[1] = AC97_cached_codec_read(ac97, AC97_EXTENDED_ID);
	o_info->vendor_id[0] = ac97hw->info.vendor7C;
	o_info->vendor_id[1] = ac97hw->info.vendor7E;
	o_info->codec_info = 0;
	o_info->channel_counts = ac97hw->info.channel_counts;
	o_info->frame_rates = ac97hw->info.frame_rates;
	o_info->formats = ac97hw->info.formats;
	o_info->cvsr_min = ac97hw->info.cvsr_min;
	o_info->cvsr_max = ac97hw->info.cvsr_max;
	strncpy(o_info->name, ac97hw->info.name, sizeof(o_info->name));
	o_info->name[sizeof(o_info->name)-1] = 0;

	switch (what)
	{
	case AC97_ADC_INFO:
	case AC97_CURRENT_ADC_INFO:
		//	can't do double-rate on ADCs
		o_info->frame_rates &= ~(B_SR_96000 | B_SR_88200);
		break;
	default:
		break;
	}
	return B_OK;
}

//	get/set_codec_format works only on CURRENT mode_select values
status_t
AC97_get_codec_format(ac97_t_v3* ac97, ac97_mode_select what, game_codec_format * o_fmt)
{
	switch (what)
	{
	default:
		dprintf("AC97_get_codec_format: bad 'what' value %d\n", what);
		return B_BAD_VALUE;
	case AC97_CURRENT_ADC_INFO:
		o_fmt->cvsr = AC97_cached_codec_read(ac97, AC97_LR_ADC_RATE);
		break;
	case AC97_CURRENT_DAC_INFO:
		o_fmt->cvsr = AC97_cached_codec_read(ac97, AC97_FRONT_DAC_RATE);
		break;
	}
	
	o_fmt->cvsr = o_fmt->cvsr * 48000 / ((ac97hw_t *)(ac97->hw))->freq_adjust ;

	//fixme:	should support other formats, channel counts
	o_fmt->channels = 2;
	o_fmt->format = B_FMT_16BIT;
	if (o_fmt->cvsr < 1.f) o_fmt->cvsr = 48000.f;
	if (o_fmt->cvsr == 48000.f) o_fmt->frame_rate = B_SR_48000;
	else if (o_fmt->cvsr == 44100.f) o_fmt->frame_rate = B_SR_44100;
	else if (o_fmt->cvsr == 96000.f) o_fmt->frame_rate = B_SR_96000;
	else if (o_fmt->cvsr == 88200.f) o_fmt->frame_rate = B_SR_88200;
	else o_fmt->frame_rate = B_SR_CVSR;
	return B_OK;
}

status_t
AC97_set_codec_format(ac97_t_v3* ac97, ac97_mode_select what, const game_codec_format * i_fmt)
{
	uint16 rate = 0;
	bool dra = false;

	if (i_fmt->cvsr > 48000.f)
	{
		rate = (uint16)i_fmt->cvsr/2.f;
		dra = true;
	}
	else
	{
		rate = (uint16)i_fmt->cvsr;
	}

	rate = rate * ((ac97hw_t *)(ac97->hw))->freq_adjust / 48000;

	switch (what)
	{
	default:
		dprintf("AC97_set_codec_format: bad 'what' value %d\n", what);
		return B_BAD_VALUE;
	case AC97_CURRENT_ADC_INFO:
		if (i_fmt->flags & GAME_CODEC_SET_FRAME_RATE)
		{
			if (dra)
			{
				dprintf("AC97_set_codec_format: can't do double rate on input\n");
				return B_BAD_VALUE;
			}
			AC97_cached_codec_write(ac97, AC97_LR_ADC_RATE, rate, 0xffff);
		}
		break;
	case AC97_CURRENT_DAC_INFO:
		if (i_fmt->flags & GAME_CODEC_SET_FRAME_RATE)
		{
			AC97_cached_codec_write(ac97, AC97_EXTENDED_CTRL_STAT, dra ? 0x2 : 0, 0x2);
			AC97_cached_codec_write(ac97, AC97_FRONT_DAC_RATE, rate, 0xffff);
		}
		break;
	}
	return B_OK;
}

status_t
write_error(
			void* host,
			uchar offset,
			uint16 value,
			uint16 mask )
{
	dprintf("AC97: Write called from virtual codec.\n");
	host = host;
	offset = offset;
	value = value;
	mask = mask;
	return B_ERROR;
}			

uint16
read_error(
			void* host,
			uchar offset)
{
	dprintf("AC97: Read called from virtual codec.\n");
	host = host;
	offset = offset;
	return 0xFFFF;
}
			
status_t
virtual_info( virtual_control_data * pvcd,
		int32 num_controls,
		int32 * io_levels,
		int32 * io_dacs
		)
{		
	game_mixer_control * pgmc = NULL;
	int32 i = 0;
	*io_levels = *io_dacs = 0;

	for (i = 0; i< num_controls; i++) {
		if (pvcd[i].codec) {
			pgmc = control_id_to_control(pvcd[i].codec, pvcd[i].control_id);
		}
		else {
			dbprintf(("\033[31mAC97: virtual_info() codec is NULL.\033[0m\n"));
			return B_BAD_VALUE;	
		}
		if ( pgmc ) {
			if ( pgmc->kind == GAME_MIXER_CONTROL_IS_LEVEL )
				(*io_levels)++;
			if ( ((GAME_MIXER_ORDINAL(pgmc->mixer_id)) & 0x1) == 1  )
				(*io_dacs)++;
		}
	}
//	dprintf("\033[31mAC97: virtual_info is levels %ld adcs %ld dacs %ld.\033[0m\n", *io_levels, num_controls - *io_dacs, *io_dacs );	
	return B_OK;
}

status_t
AC97_make_virtual_codec(
						ac97_t_v3 * virtual_codec,
						virtual_control_data * pvcd,
						int32 num_controls
						)
{
	ac97hw_t * ac97hw = NULL;
	game_mixer_control * pgmc;
	int32 i,j;
	int32 levels, dacs;
		
	//allocate a virtual codec...
	virtual_codec->hw = malloc(sizeof(ac97hw_t));
	if(virtual_codec->hw == NULL) return ENOMEM;
	memset(virtual_codec->hw,0,sizeof(ac97hw_t));
	ac97hw = (ac97hw_t*)virtual_codec->hw;

	virtual_codec->host = NULL;
	ac97hw->Write = write_error;
	ac97hw->Read = read_error;
	ac97hw->lock = 0;

	/* Benaphor for cache */
	ac97hw->cache_valid = 0;
	ac97hw->cache_ben_atom = 0;
	ac97hw->cache_ben_sem = 0;

	// set up controls for this particular codec

	if (B_OK != virtual_info(pvcd, num_controls, &levels, &dacs)) {
		dprintf("\033[31mAC97: bind failed in virtual_info.\033[0m\n" );	
		return B_BAD_VALUE;
	}
	ac97hw->mixer_ctls = malloc(sizeof(game_mixer_control) * num_controls);
	if(ac97hw->mixer_ctls == NULL) return ENOMEM;
	memset(ac97hw->mixer_ctls,0,sizeof(game_mixer_control) * num_controls);

	ac97hw->level_info = malloc(sizeof(game_get_mixer_level_info) * (levels));
	if(ac97hw->level_info == NULL) return ENOMEM;
	memset(ac97hw->level_info,0,sizeof(game_get_mixer_level_info) * (levels));

	ac97hw->ctl_cookies = malloc(sizeof(void *) * num_controls);
	if(ac97hw->ctl_cookies == NULL) return ENOMEM;
	memset(ac97hw->ctl_cookies,0,sizeof(void *) * num_controls);

	j = 0;
	for (i = 0; i < num_controls; i++) {
		pgmc = control_id_to_control(pvcd[i].codec, pvcd[i].control_id);
		memcpy(&ac97hw->mixer_ctls[i], pgmc, sizeof(game_mixer_control));
		ac97hw->ctl_cookies[i] = pvcd[i].codec;
		if (pgmc->kind == GAME_MIXER_CONTROL_IS_LEVEL && j <= levels) {
			if (ac97hw->level_info[j].type != pvcd[i].type) {
				void * ptr = NULL;
				ptr = info_from_type( pvcd[i].codec, pvcd[i].type);
				if (ptr == NULL) {
					dprintf("AC97: bind_to_virtual_codec info_from_type returned NULL! type mismatch? (level != mux != enable)\n");
					ptr = info_from_control( pvcd[i].codec, pvcd[i].control_id, pgmc->kind);
				}
				memcpy(&ac97hw->level_info[j], ptr, sizeof(game_get_mixer_level_info));			
				// repopulate it correctly....
				ac97hw->level_info[j].control_id = pvcd[i].control_id;
				//change the name here.............
				ptr = info_from_control( pvcd[i].codec, pvcd[i].control_id, pgmc->kind);
				if (ptr) {
					strncpy(ac97hw->level_info[j].label, ((game_get_mixer_level_info *)(ptr))->label, 32);
				}
			}
			else {
				memcpy(&ac97hw->level_info[j], info_from_control( pvcd[i].codec, pvcd[i].control_id, pgmc->kind), sizeof(game_get_mixer_level_info) );
			}
			j++;
		}
	}
	
	ac97hw->adc_ctls	= num_controls - dacs;
	ac97hw->dac_ctls	= dacs;
	ac97hw->levels		= levels;
	
	return B_OK;
}

status_t
init_ac97_basic_controls( ac97hw_t * ac97hw )
{
	ac97hw->adc_ctls	= X_NUM_ADC_CONTROLS;
	ac97hw->dac_ctls	= 5;//X_NUM_DAC_CONTROLS;
	ac97hw->levels		= 6;//X_NUM_LEVELS;

	//these are hard coded for now, at some point control_id's might be better
	//and maybe a loop...
	memcpy( &ac97hw->mixer_ctls[0] , &xxx_controls[0], sizeof(game_mixer_control) );
	memcpy( &ac97hw->mixer_ctls[1] , &xxx_controls[1], sizeof(game_mixer_control) );
	memcpy( &ac97hw->mixer_ctls[2] , &xxx_controls[2], sizeof(game_mixer_control) );
	memcpy( &ac97hw->mixer_ctls[3] , &xxx_controls[3], sizeof(game_mixer_control) );
	memcpy( &ac97hw->mixer_ctls[4] , &xxx_controls[4], sizeof(game_mixer_control) );
	memcpy( &ac97hw->mixer_ctls[5] , &xxx_controls[8], sizeof(game_mixer_control) );
	memcpy( &ac97hw->mixer_ctls[6] , &xxx_controls[10],sizeof(game_mixer_control) );
	memcpy( &ac97hw->mixer_ctls[7] , &xxx_controls[11],sizeof(game_mixer_control) );
	memcpy( &ac97hw->mixer_ctls[8] , &xxx_controls[12],sizeof(game_mixer_control) );

	memcpy(	&ac97hw->level_info[0] , &xxx_level_info[0], sizeof(game_get_mixer_level_info) );
	memcpy(	&ac97hw->level_info[1] , &xxx_level_info[1], sizeof(game_get_mixer_level_info) );
	memcpy(	&ac97hw->level_info[2] , &xxx_level_info[5], sizeof(game_get_mixer_level_info) );
	memcpy(	&ac97hw->level_info[3] , &xxx_level_info[7], sizeof(game_get_mixer_level_info) );
	memcpy(	&ac97hw->level_info[4] , &xxx_level_info[8], sizeof(game_get_mixer_level_info) );
	memcpy(	&ac97hw->level_info[5] , &xxx_level_info[9], sizeof(game_get_mixer_level_info) );

	memcpy( &ac97hw->ctl_cookies[0], &xxx_ctl_cookies[0], sizeof(void *) * (ac97hw->adc_ctls + ac97hw->dac_ctls));

	return B_OK;
}

status_t
init_ac97_codec_controls( ac97hw_t * ac97hw )
{
	int32 optionals = 0;
	int32 mask = 1;
	int32 count = 0;
