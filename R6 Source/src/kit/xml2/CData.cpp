c97->host, 0x2a); 
	        dprintf("ac97hw->ExtendedAudioStatus = 0x%x\n",ac97hw->ExtendedAudioStatus); 
	    } 
	} 
	
	switch( AC97_VENDOR_ID(ac97hw->VendorID.ch.F, ac97hw->VendorID.ch.S,ac97hw->VendorID.ch.T) ) 
	{ 
		case VENDOR_CYRRUS_LOGIC_CRYSTAL: 
	    { 
	        switch(ac97hw->VendorID.ch.Rev) 
	        { 
	        case 0x03: 
                dprintf("ac97 is Cyrrus Logic cs4297\n"); 
                break; 
	        case 0x33: 
                dprintf("ac97 is Cyrrus Logic cs4299\n"); 
                break; 
	        default: 
                dprintf("ac97 vendor is Cyrrus Logic.\n"); 
                break; 
	        } 
	    } 
	    break; 
		case VENDOR_SIGMATEL: 
	    { /*case*/
          if (reset_hw) 
              switch(ac97hw->VendorID.ch.Rev) 
              { 
              case 0x04:                                                                                      
	              dprintf("ac97 is Sigmatel 9701/03\n"); 
	              brea