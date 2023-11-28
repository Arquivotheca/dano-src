                                                                   
	              dprintf("ac97 is SigmaTel 9704\n"); 
	              // enable the extra attenuation 
	              (*ac97->Write) (ac97->host, 0x5A, 0xABAB, 0xffff ); 
	              (*ac97->Write) (ac97->host, 0x5C, 0x0008, 0xffff ); 
	              break; 
              case 0x08:                                                              
	              dprintf("ac97 is SigmaTel 9708\n"); 
	              { 
	               uint16 wRegVal, wRegVal2;               
	               (*ac97->Write)(ac97->host, AC97_CTRL_STAT, 0x8000, 0x8000 ); 
	               // identify revision to make configuration adjustments 
	               wRegVal = (*ac97->Read)(ac97->host, 0x72 ); 
	               wRegVal2 = (*ac97->Read)(ac97->host, 0x6c ); 
	               if (wRegVal == 0x0000 && wRegVal2 == 0x0000)    // LA2 
	               { 
	                   dprintf("This is a Sigmatel STAC9708 LA2 codec\n"); 
	                   // CIC filter 
	                   (*ac97->Write) (ac97->host, 0x76, 0xABBA, 0xff); 
	                   (*ac97->Write) (ac97->host, 0x78, 0x1000, 0xff); 
	                   // increase the analog currents 
	                   (*ac97->Write) (ac97->host, 0x70, 0xABBA, 0xff ); 
	                   (*ac97->Write) (ac97->host, 0x72, 0x0007, 0xff ); 
	               } 
	               else if (wRegVal == 0x8000 && wRegVal2 == 0x0000)    // LA4 
	             