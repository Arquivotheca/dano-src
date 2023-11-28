
	}
		
	/* Tell device to change its config */
	status = send_request(dev_id, USB_REQTYPE_DEVICE_OUT, 
						  USB_REQUEST_SET_CONFIGURATION, 
						  conf ? conf->descr->configuration_value : 0,
						  0, 0, NULL, NULL);
	
	if(status) {
		dprintf(ID "error with send_request: %s\n", strerror(status));
		goto error;
	}
	
	/* Create the pipes if we're not unconfigured */
	if(conf){
		/* For each interface */
		for(i = 0; i < conf->interface_count; i++) {
			/* For each alternate to this interface */
			for(j = 0; j < conf->interface[i].alt_count; j++) {
				ddprintf("CONFIG: alt interface[%d]\n", j);
				ifc = &conf->interface[i].alt[j];
				
				/* Set the active interface to 0 */
				if (j == 0) {
					bm_usb_interface * interface;
					
					/* Activate the default alt in