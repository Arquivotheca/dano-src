c97hw_t * hw = (ac97hw_t *) ac97->hw;
	status_t ret = B_OK;
	
	dbprintf(("AC97: get_mixer_controls\n"));
	i_info = i_info;
	if ((GAME_MIXER_OR