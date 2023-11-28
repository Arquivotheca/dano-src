}
		switch (byte) {	/* System Common/RealTime */
		case 0xf0:	/* SYS_EX_START */
			tocopy = max_size;
			break;
		case 0xf1:	/* MIDI_TIME_CODE */
			tocopy = 2;
			break;
		case 0xf2:	/* SONG_POSITION */
			tocopy = 3;
			break;
		case 0xf3:	/* SO