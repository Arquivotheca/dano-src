
#ifndef _InterfaceCmd_H
#define _InterfaceCmd_H


/*------------------------------------------------------------------------------
   Reader list of commands:
------------------------------------------------------------------------------*/
      #define  DEFAULT_TIME      5000
#define  LOW_TIME           500
#define  DEFAULT_VPP          0
#define  DEFAULT_PRESENCE     3
#define  BUFFER_SIZE        261
#define  OS_STRING_SIZE      IFD_LEN_VERSION+1

/*------------------------------------------------------------------------------
   Reader list of commands:
------------------------------------------------------------------------------*/
#define  IFD_CMD_MODE_SET    		"\x01\x00"
#define  IFD_CMD_SIO_SET	    		0x0A
#define  IFD_CMD_DIR		    		"\x17\x00"
#define  IFD_CMD_ICC_DEFINE_TYPE	0x17
#define  IFD_CMD_ICC_POWER_DOWN 	0x11
#define  IFD_CMD_ICC_POWER_UP 		0x12
#define  IFD_CMD_ICC_ISO_OUT    	0x13
#define  IFD_CMD_ICC_ISO_IN     	0x14
#define  IFD_CMD_ICC_APDU    		0x15
#define  IFD_CMD_ICC_SYNCHRONE 	   0x16
#define  IFD_CMD_ICC_DISPATCHER 	"\x17\x01"
#define  IFD_CMD_ICC_STATUS    	   0x17
#define  IFD_CMD_MOD_DEFINE_TYPE	0x1F
#define  IFD_CMD_MOD_POWER_DOWN 	0x19
#define  IFD_CMD_MOD_POWER_UP 		0x1A
#define  IFD_CMD_MOD_ISO_OUT    	0x1B
#define  IFD_CMD_MOD_ISO_IN     	0x1C
#define  IFD_CMD_MOD_APDU    		0x1D
#define  IFD_CMD_MOD_SYNCHRONE 	   0x1E
#define  IFD_CMD_MOD_DISPATCHER 	"\x1F\x01"
#define  IFD_CMD_MOD_STATUS    	   0x1F
#define  IFD_CMD_MEM_RD	    		0x22
#define  IFD_CMD_MEM_WR	    		0x23
#define  IFD_CMD_CPU_RD	    		0x24
#define  IFD_CMD_CPU_WR	    		0x25
#define  IFD_CMD_MEM_ERASE    		0x26
#define  IFD_CMD_MEM_SELECT    	   0x27
#define  IFD_CMD_IO_RD		   	   0x42
#define  IFD_CMD_IO_WR		   	   0x43
#define  IFD_CMD_IO_BIT_WR	   	0x44
#define  IFD_CMD_LCD_ON	    		0x2A
#define  IFD_CMD_LCD_OFF	    		0x29
#define  IFD_CMD_LCD_CHAR    		0x2C
#define  IFD_CMD_LCD_STRING    	   0x2B
#define  IFD_CMD_LCD_CMD	    		0x2D
#define  IFD_CMD_KEY_TIMEOUT    	0x32
#define  IFD_CMD_SOUND_BUZZER   	0x33
#define  IFD_CMD_RTC_RD	    		0x3A
#define  IFD_CMD_RTC_WR	    		0x3B


#endif
