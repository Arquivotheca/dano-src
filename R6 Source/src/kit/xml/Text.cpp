p) {
	case B_MIDI_GET_READ_TIMEOUT:
	case B_MIDI_GET_READ_TIMEOUT_OLD:
		memcpy(data, &config.timeout, sizeof(long long));
		return B_OK;
	case B_MIDI_SET_READ_TIMEOUT:
	case B_MIDI_SET_READ_TIMEOUT_OLD:
		memcpy(&config.timeout, data, sizeof(long long));
		err = B_OK;
		break;
	case B_MIDI_TIMED_READ:
		return midi_timed_read(port, (midi_timed_data *)data);
	case B_MIDI_TIMED_WRITE:
		return midi_timed_write(port, (midi_timed_data *)data);
	case B_MIDI_WRITE_SYNC:
		return midi_write_sync(port);
	case B_MIDI_WRITE_CLEAR:
		return midi_write_clear(port);
	case 0x10203040:
		dprintf("SPECIAL: %s\n", data);
		break;
	default:
		err = B_BAD_VALUE;
		break;
	}
	if (err == B_OK) {
		cpu_status cp;
		KTRACE();
		cp = disable_interrupts();
		acquire_spinlock(&port->port_lock);
		err = configure_midi(port, &config, false);
		release_spinlock(&port->port_lock);
		restore_interrupts(cp);
	}
	return err;
}


static status_t
midi_read(
	void * cookie,
	off_t