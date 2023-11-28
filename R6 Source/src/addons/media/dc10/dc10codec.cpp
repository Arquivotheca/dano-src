#include <assert.h>
#include <malloc.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <kernel/OS.h>

#include "dc10codec.h"
#include "definition.h"



#define PRINTF printf
#define FUNCTION	PRINTF
#define ERROR		PRINTF
#define PROGRESS	PRINTF
#define PULSE		PRINTF
#define LOOP		PRINTF

static unsigned char init_decoder[] = 
{
	0x4C, 0x3C, 0x0D, 0xEF, 0xBD, 0xF2, 0x03, 0x00,
  	0xF8, 0xF8, 0x60, 0x60, 0x00, 0x86, 0x18, 0x90,
  	0x00, 0x59, 0x40, 0x46, 0x42, 0x1A, 0xFF, 0xDA,
  	0xF2, 0x8B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  	0xD9, 0x16, 0x40, 0x41, 0x80, 0x41, 0x80, 0x4F,
  	0xFE, 0x01, 0xCF, 0x0F, 0x03, 0x01, 0x03, 0x0c,
  	0x44, 0x71, 0x02, 0x8C, 0x02
};

static  unsigned char init_encoder_pal_mir[] = {
   0x01,   // MR0, PAL enabled 
   0x80,   // MR1 
   0xcb,   // subc. freq. reg 0 
   0x8a,   // subc. freq. reg 1
   0x09,   // subc. freq. reg2
   0x2a,   // subc. freq. reg3
   0x00,   // subc. phase 
   
   0x0f,   // TR0, 16bit mode 1  
   0x00,   // Closed Captioning Ext Register 0 
   0x00,   // Closed Captioning Ext Register 1 
   0x00,   // Closed Captioning Register 0  
   0x00,   // Closed Captioning Register 1 
   0x00,   // TR1 
   0x09,   // MR2 
   0x00,   // Pedestral Control Register 0 
   0x00,   // Pedestral Control Register 1 
   0x00,   // Pedestral Control Register 2 
   0x00,   // Pedestral Control Register 3 
   0x00   // MR3   
};

#if 0
static  unsigned char init_encoder_ntsc_mir[] = {
   0x14,   // MR0, NTSC enabled 
   0x00,   // MR1 
   0x16,   // subc. freq. reg 0 
   0x7c,   // subc. freq. reg 1
   0xf0,   // subc. freq. reg2
   0x21,   // subc. freq. reg3
   0x00,   // subc. phase 
   
   0x4e,   // TR0, 16bit mode 1  
   0x21,   // Closed Captioning Ext Register 0 
   0x00,   // Closed Captioning Ext Register 1 
   0x00,   // Closed Captioning Register 0  
   0x00,   // Closed Captioning Register 1 
   0x80,   // TR1 
   0x49,   // MR2 
   0x00,   // Pedestral Control Register 0 
   0x00,   // Pedestral Control Register 1 
   0x00,   // Pedestral Control Register 2 
   0x00,   // Pedestral Control Register 3 
   0x00   // MR3   
 
 
   
};
#endif

static  unsigned char init_encoder_pal[] = {
   0x11,   // MR0, PAL enabled 
   0x00,   // MR1 
   0x0c,   // subc. freq. reg 0 
   0x8c,   // subc. freq. reg 1
   0x79,   // subc. freq. reg2
   0x26,   // subc. freq. reg3
   0x40,   // subc. phase 
   
   0x4e,   // TR0, 16bit mode 1  
   0x21,   // Closed Captioning Ext Register 0 
   0x00,   // Closed Captioning Ext Register 1 
   0x00,   // Closed Captioning Register 0  
   0x00,   // Closed Captioning Register 1 
   0x80,   // TR1 
   0x4f,   // MR2 
   0x00,   // Pedestral Control Register 0 
   0x00,   // Pedestral Control Register 1 
   0x00,   // Pedestral Control Register 2 
   0x00,   // Pedestral Control Register 3 
   0x00   // MR3   
};


static  unsigned char init_encoder_secam[] = {
   0x11,   // MR0, PAL enabled 
   0x00,   // MR1 
   0x0c,   // subc. freq. reg 0 
   0x8c,   // subc. freq. reg 1
   0x79,   // subc. freq. reg2
   0x26,   // subc. freq. reg3
   0x40,   // subc. phase 
   
   0x4e,   // TR0, 16bit mode 1  
   0x21,   // Closed Captioning Ext Register 0 
   0x00,   // Closed Captioning Ext Register 1 
   0x00,   // Closed Captioning Register 0  
   0x00,   // Closed Captioning Register 1 
   0x80,   // TR1 
   0x49,   // MR2 
   0x00,   // Pedestral Control Register 0 
   0x00,   // Pedestral Control Register 1 
   0x00,   // Pedestral Control Register 2 
   0x00,   // Pedestral Control Register 3 
   0x00   // MR3   
};

static  unsigned char init_encoder_ntsc[] = {
   0x00,   // MR0, NTSC enabled 
   0x00,   // MR1 
   0x55,   // subc. freq. reg 0 
   0x55,   // subc. freq. reg 1
   0x55,   // subc. freq. reg2
   0x25,   // subc. freq. reg3
   0x1a,   // subc. phase 
   
   0x4e,   // TR0, 16bit mode 1  
   0x21,   // Closed Captioning Ext Register 0 
   0x00,   // Closed Captioning Ext Register 1 
   0x00,   // Closed Captioning Register 0  
   0x00,   // Closed Captioning Register 1 
   0x80,   // TR1 
   0x49,   // MR2 
   0x00,   // Pedestral Control Register 0 
   0x00,   // Pedestral Control Register 1 
   0x00,   // Pedestral Control Register 2 
   0x00,   // Pedestral Control Register 3 
   0x00   // MR3   
};

dc10codec::dc10codec()
{	
	mBufferSize = 256 * 1024;
	mDevice.encoder_ad=0x00;	
	mDevice.decoder_ad=0x00;	
}


dc10codec::~dc10codec(void)
{
}


status_t dc10codec::post_office_wait(void)
{
	uint32 *reg;
	//Post Office Register
	
	reg = (uint32 *) fRegAddress;
	reg += 128;
   	while ((*reg  & (1 << 25)  ) == (1 << 25)) 
   	{
   	}
   	
   	if ( (*reg & (1 << 24)  ) == (1 << 24) )
   	{
    	ERROR("DC10codec :: postoffice time out \n");
      	return -1;
   	}
   	return 0;
}



status_t dc10codec::post_office_write(uint32 guest, uint32 reg, uint32 value)
{
	uint32 *preg;
	//Post Office Register
	preg = (uint32 *) fRegAddress;
	preg += 128;
	
	//wait for request	
	while((*preg & (1 << 25)) == (1 << 25))
	{
		snooze(1);
	}
	
	//clear time out
	if((*preg & (1 << 24)) == (1 << 24))
	{
		*preg = 1 << 24 ;
	}
	
	
   	*preg = (1 << 23)  | ((guest & 7) << 20) | ((reg & 7) << 16) | (value & 255);
   	snooze(10);
   	
   	//wait for request	
	while((*preg & (1 << 25)) == (1 << 25))
	{
		snooze(1);
	}
	
	//clear time out
	if((*preg & (1 << 24)) == (1 << 24))
	{
		*preg = 1 << 24 ;
		return B_ERROR;
	}
   	return B_OK;
}



int32 dc10codec::post_office_read( uint32 guest, uint32 reg)
{
	uint32 *preg;
	//Post Office Register
	preg = (uint32 *) fRegAddress;
	preg += 128;
	
	//wait for request	
	while((*preg & (1 << 25)) == (1 << 25))
	{
		snooze(1);
	}
	
	//clear time out
	if((*preg & (1 << 24)) == (1 << 24))
	{
		*preg = 1 << 24 ;
	}
	
	
   	*preg =  ((guest & 7) << 20) | ((reg & 7) << 16) ;
   	snooze(10);
   	
   	//wait for request	
	while((*preg & (1 << 25)) == (1 << 25))
	{
		snooze(1);
	}
	
	//clear time out
	if((*preg & (1 << 24)) == (1 << 24))
	{
		*preg = 1 << 24 ;
		return -1;
	}
   	return (*preg & 255);
}


status_t dc10codec::write(uint32 reg, uint32 val,uint32 size)
{
	uint32 i;
	status_t err = B_OK;
	i=0;
	
	while((i < size) && (err == B_OK))
	{
		err = post_office_write( 0, 1, (reg+i) >> 8);
		if(err != B_OK)
		{ return err;}
		err = post_office_write( 0, 2, (reg+i) );
		if(err != B_OK)
		{ return err;}
		err = post_office_write( 0, 3, (val >> ((size -1 - i)*8))  );
		if(err != B_OK)
		{ return err;}
		i++;
	}
	return B_OK;
}


int32	dc10codec::read_8(uint32 reg)
{
	status_t err = B_OK;
	err = post_office_write( 0, 1, reg >> 8);
	if (err != B_OK)
	{ return err;}
	err =  post_office_write( 0, 2, reg);
	if (err != B_OK)
	{ return err;}
 	
   	return (post_office_read( 0, 3) & 0xFF);
}
		
void dc10codec::sleep(bool state)
{
	uint32 * preg =(uint32 *)fRegAddress;
	uint32 mask = 1 << 27;
	preg += 11;
	
	if(state)
	{
		*preg = *preg  & (~mask);
		snooze(10);
	}
	else
	{
		*preg = *preg  | (mask);
		snooze(500);
	}
}

void dc10codec::reset(void)
{
	sleep(false);
	
	uint32 * preg =(uint32 *)fRegAddress;
	uint32 mask = 1 << 24 ;
	preg +=  11;	
	*preg = *preg  & (~mask);
	snooze(10);
	*preg = *preg  | (mask);
	snooze(10);
}


void dc10codec::frame(bool state)
{
	uint32 * preg =(uint32 *)fRegAddress;
	uint32 mask = 1 << 30 ;
	preg += 11;
	
	if(state)
	{
		*preg = *preg  | (mask);
	}
	else
	{
		*preg = *preg  & (~mask);
	}
	snooze(10);
}

void dc10codec::set_codec(void)
{
	uint32 adr,size,reg;
	uint32 i;
	
	unsigned char dqt[] = 
	{
      0xff, 0xdb,0x00, 0x84,  
      0x00, 0x10, 0x0b, 0x0c, 0x0e, 0x0c, 0x0a, 0x10, 0x0e, 0x0d, 0x0e, 0x12, 0x11, 0x10,
      0x13, 0x18, 0x28, 0x1a, 0x18, 0x16, 0x16, 0x18, 0x31, 0x23, 0x25, 0x1d, 0x28, 0x3a, 0x33,
      0x3d, 0x3c, 0x39, 0x33, 0x38, 0x37, 0x40, 0x48, 0x5c, 0x4e, 0x40, 0x44, 0x57, 0x45,
      0x37, 0x38, 0x50, 0x6d, 0x51, 0x57, 0x5f, 0x62, 0x67, 0x68, 0x67, 0x3e, 0x4d, 0x71, 0x79,
      0x70, 0x64, 0x78, 0x5c, 0x65, 0x67, 0x63, 0x01, 0x11, 0x12, 0x12, 0x18, 0x15, 0x18, 0x2f, 
      0x1a, 0x1a, 0x2f, 0x63, 0x42, 0x38, 0x42, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
      0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
      0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 
      0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63
   	};
	
	unsigned char dht[] = {
      0xff, 0xc4, 0x01, 0xa2,
      0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x01,      
      0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x10, 0x00, 
      0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7d,
      0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 
      0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08, 0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52,
      0xd1, 0xf0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25,
      0x26, 0x27, 0x28, 0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 
      0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64,
      0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x83, 
      0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 
      0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 
      0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 
      0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2, 0xe3, 
      0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 
      0xf8, 0xf9, 0xfa, 0x11, 0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04, 
      0x04, 0x00, 0x01, 0x02, 0x77, 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06,
      0x12, 0x41, 0x51, 0x07, 0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xa1,
      0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0, 0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24,
      0x34, 0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 
      0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55,
      0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74,
      0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 
      0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 
      0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 
      0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 
      0xd7, 0xd8, 0xd9, 0xda, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
      0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa
   	};

	
	//Code Interface Register
   	reg   = 1;     	  
	write( 0x002, reg,1);
	
	//Code Mode Register
	if(video_mode == MODE_DECOMPRESS)
	{
		reg   = 0x00;   
	} 
   	if(video_mode == MODE_COMPRESS)
	{
		reg   = 0xa4;   
	} 
   	write( 0x003, reg,1);

	//Reserved
	reg   = 0x00;      
   	write( 0x004, reg,1);

	//Maximum Block Code Volume Register
   	reg   = 0xfe;     
   	write( 0x005, reg, 1);

	//Markers Enable Register  
   	reg = 56; // DRI, DQT, DHT 
   	write( 0x006, reg,1);

	//Interrupt Mask Register	
   	reg   = 1;  
   	write( 0x007, reg,1);

   	//Target Net Code Volume Register
	size = video_height * video_width ;
	if (HScale == 0)
	{
		size = size / 2 ;
	}
	size = size  * mQuality  / 20; 
   	if(size < 16000) size = 16000;
   	if(size > mBufferSize*7)
      size = mBufferSize*7 ;
   	reg = size;
   	write(0x009, reg,4);

   	//Scale Factors Registers
   	reg   = 0x0100;    
   	write(0x011, reg,2);
	
	//Allocqtion Factor Registers
	reg = 0x00ffffff;   
   	write( 0x013, reg,3);

	//Test Control Register
   	reg   = 0x0000;  
   	write( 0x024, reg,2);
   	
   	//Video Control Register
   	reg   = (1 << 6) | (1 << 2);   
   	write(0x030, reg,1);

	//Video Polarity Register
	reg   = (1 << 3) | (1 << 2) | (1 << 1) | (1 << 0);      
   	write( 0x031, reg,1);

	//Scaling Register
  	reg = HScale | ( VScale << 2);
   	write( 0x032, reg,1);
	
	//Background Color Rgeisters
   	reg   = 0;       
   	write( 0x033, reg,3);
  	
   	//Sync Generator Registers 	
   	//VTotal
   	reg   = Ht - 1;           
   	write( 0x036, reg,2);

	//HTotal
   	reg   = Wt - 1;           
   	write(0x038, reg,2);

	//VsyncSize
   	reg   = 6 - 1;                 
   	write( 0x03a, reg,1);

   	//HsyncSize
   	reg = 57;
   	//reg   = (norm == 1 ? 57 : 68);
   	write( 0x03b, reg,1);

	//BVStart
	reg   = VStart - 1;       
   	write( 0x03c, reg,1);
	
	//BVend
   	reg   += Ha / 2;          
   	write( 0x03e, reg,2);

	//BHstart
   	reg   = HStart - 1 + 64;       
   	write( 0x03d, reg,1);

	//BHend
   	reg   += Wa;               
   	write( 0x040, reg,2);

   	//Active Area Registers
   	//VStart
   	reg   = 0 + VStart;  
   	write( 0x042, reg,2);

   	//Vend
   	reg  += (Ha / 2);           
   	write( 0x044, reg,2);

   	//Hstart
   	reg   = 4 + HStart + 64; 
   	write(0x046, reg,2);

   	//Hend
   	reg  += Wa;            
   	write( 0x048, reg,2);

   	//SUBIMG Window Registers
   	//SVstart
   	reg   = VStart - 4;  
   	write( 0x04a, reg,2);

   	//SVend
   	reg  += Ha / 2 + 8;          
   	write( 0x04c, reg,2);
	
	//SHstart
   	reg   = HStart - 4 + 64;
   	write( 0x04e, reg,2);

   	//SHend
   	reg  +=  Wa + 8; 
   	write( 0x050, reg,2);
   	
   	//JPEG MARKERS
   	
   	//Start of frame marker
   	reg   = 0xffc0;   
   	write( 0x060, reg,2);

	//Lenght of the segemt	
   	reg   = 0x11;     
   	write(0x062, reg,2);
	
	//Precision
   	reg   = 8; 
   	write( 0x064, reg,1);

	//Number of lines in the active area
   	reg   = Ha / 2;   
   	write( 0x065, reg,2);

	//Number of pixels in the active area
   	reg   = video_width ;  
   	write( 0x067, reg,2);

	//Number of color components
   	reg   = 3;      
   	write( 0x069, reg,1);

	//Y component
   	reg   = 0x002100;   

   	write( 0x06a, reg,3);

	//U component
   	reg   = 0x011101;  
   	write( 0x06d, reg,3);

	//V component
   	reg   = 0x021101;  
   	write( 0x070, reg,3);
   	
   	
   	//Start of scan marker
   	reg   = 0xffda;   
   	write(0x07a, reg,2);

	//lenght of this segment
   	reg   = 12;      
   	write( 0x07c, reg,2);

	//Number of component in this scan
   	reg   = 3;      
   	write( 0x07e, reg,1);

	//Y component
   	reg   = 0x0000;   
   	write( 0x07f, reg,2);

	//U component
   	reg   = 0x0111;  
   	write( 0x081, reg,2);

 	// V component
   	reg   = 0x0211;  
   	write( 0x083, reg,2);

	//Constant 3 byte data to indicate end of scan information
   	reg   = 0x003f00;   
   	write( 0x085, reg,3);
   	
   	
   	
   	
   	//Define restart interval
   	reg   = 0xffdd;   
   	write( 0x0c0, reg,2);

	
   	reg   = 0x04;    
   	write( 0x0c2, reg,2);

	//lenght of Restart Interval
   	reg   = 0x08;      
   	write( 0x0c4, reg,2);
   
   //DQT
   	adr = 0x0cc;
   	for (i = 0; i < sizeof(dqt); i++)
   	{
    	write( adr, dqt[i],1);
    	adr++;
   	}
  
   	//DHT 
   	adr = 0x1d4;
   	for (i = 0; i < sizeof(dht); i++) 
   	{
       	write( adr, dht[i],1);
  		adr++;
   	}
   	
   	
	// APPn marker 
   	reg   = 0xffe0 ;   
   	write(0x380,reg,2);
   	
 	// APPn len 
   	reg = 2;     
   	write(0x382,reg,2);

   	// write APPn data 
   	adr = 0x384;
   	for (i = 0; i < 60; i++) 
   	{
        write(adr++, 0,1 );
   		adr ++;	
   	}
   	  
	// COM marker 
   	reg   = 0xfffe;   
   	write(0x3c0,reg,2);
   	
	// COM len 
   	reg = 2;      
   	write(0x3c2,reg,2);

   	// write COM data 
   	adr = 0x3c4;
   	for (i = 0; i < 60; i++) 
   	{
    	write(adr, 0 ,1);
   		adr ++;
   	}
   	
}	
	
	
void dc10codec::set_quality(uint32 quality)
{
	mQuality = quality;
	if (mQuality > 100)
		mQuality = 100;
}

void dc10codec::set_format(uint32 format,uint32 width,uint32 height)
{
	video_format = format;
	video_width = width;
	video_height = height;
	
	//Set configuration of video format 
	if((video_format==FORMAT_PAL)
		||(video_format==FORMAT_SECAM))
	{
		Wt=944;
		Wa=768;
		HStart=83;
		HSyncStart=880;
		Ht=625;
		Ha=576;
		VStart=16;	
		if((width == 768) && (height == 576))
		{
			HScale = 0;
			VScale = 0;	
		}
		if((width == 384) && (height == 288))
		{
			HScale = 1;
			VScale = 0;	
		}
	}
	if(video_format==FORMAT_NTSC)
	{
		Wt=780;
		Wa=640;
		HStart=51;
		HSyncStart=716;
		Ht=525;
		Ha=480;
		VStart=12;	
		if((width == 640) && (height == 480))
		{
			HScale = 0;
			VScale = 0;
		}
		if((width == 320) && (height == 240))
		{
			HScale = 1;
			VScale = 0;
		}
	}	
}


		

void dc10codec::set_codec(uint32 /*mod*/)
{	
	int32 i;
 	uint32 reg;
    
  	//FUNCTION("dc10codec :: set_codec\n");
    
   	//FUNCTION("dc10codec :: set_codec unreset\n");
   	reset();
   	snooze(10);
	
	//FUNCTION("dc10codec :: set_codec write_8\n");
   	reg   = (0 << 7)    // Load=0 
         | (1 << 0);   // SynRst=1 
   	write(0x000, reg,1);

	set_codec();
	
   	reg   = (1 << 7) ;  
 
   	write( 0x000, reg,1);

   	// wait for codec to unbusy 
   	for (i = 0; i < 100000; ++i) 
   	{
      	reg = read_8( 0x001);
      if ((reg & (1 << 7)) == 0) {
         return;
      }
     
   }
   //FUNCTION("Codec buzy\n");
}

void dc10codec::set_interface(void)
{
	//FUNCTION("dc10codec::set_interface\n");
	uint32 * preg ;
   	uint32 reg;
   	
   	
   	preg = (uint32 *)fRegAddress;
   	//reset jpeg chipset
   	preg += 65;
   	*preg = 0;
	snooze(10);

	reg = 0;
	//Set mode and control 
	if(video_mode == MODE_COMPRESS)
	{
		reg =  3 << 29;
	}
	if(video_mode == MODE_DECOMPRESS)
	{
		reg =  2 << 29;
		reg |= 1 << 4;
	}
	//set JPEG mode
	reg   |= 1 << 31;
	//Set fld_per_buffer   
   	//if(HScale == 1)
   	 reg |= 1 << 3;
   	 
   	 
   	 
   	preg = (uint32 *)fRegAddress;
   	preg += 64;
   	*preg = reg;

   	// vertical 
   	preg = (uint32 *)fRegAddress;
   	preg += 1;
   	*preg |= 1 << 30;
   	
   	reg = (6 << 16) | (Ht );
   	preg = (uint32 *)fRegAddress;
   	preg += 66;
   	*preg = reg;
   	
   	reg = ( (img_y + VStart) << 16)
       | ( (Ha/2) );
	preg = (uint32 *)fRegAddress;
   	preg += 69;
   	*preg = reg;
   	
   	// horizontal 
   	preg = (uint32 *)fRegAddress;
   	preg += 0;
   	*preg |= 1 << 30;
   
   	reg = ( (HSyncStart) << 16) | (Wt );
   	preg = (uint32 *)fRegAddress;
   	preg += 67;
   	*preg = reg;
   	
   	reg = ( (4 + HStart + 4) << 16)
       | ( Wa );
    preg = (uint32 *)fRegAddress;
   	preg += 68;
   	*preg = reg;
   	

   	// field process parameters 
    preg = (uint32 *)fRegAddress;
   	preg += 70;
   	*preg = 1;
   


   	// code base address 
   	preg = (uint32 *)fRegAddress;
   	preg += 71;
   	*preg = (uint32)fBufferPhysicalAddress;
   
   	// FIFO threshold 
   	switch (video_mode) 
   	{  
   		case MODE_COMPRESS:
      		reg = 140;
      		break;
      	case MODE_DECOMPRESS:
      		reg = 20;
      		break;
	  	default:
      		reg = 80;
      		break;
   	}
   	
   	preg = (uint32 *)fRegAddress;
   	preg += 72;
   	*preg = reg;
   	
   	
   	preg = (uint32 *)fRegAddress;
   	preg += 2;
   	reg = *preg;
   	if((video_format==FORMAT_PAL)
		||(video_format==FORMAT_SECAM))
	{
		*preg = 1<<26;
	}
	else
	{
		*preg = 0;
	}
	
   	
   	
   	
   	

   	

   	// deassert P_Reset 
   	preg = (uint32 *)fRegAddress;
   	preg += 65;
   	*preg = 1 << 7;
   	snooze(10);
   	
   
}

void dc10codec::init(uint32 format,uint32 width,uint32 height,uint32 mod,uint32 rate)
{
	//FUNCTION("dc10codec :: init\n");
	
	int i;
	for(i = 0; i<4 ;i++)
	{
		mDevice.buffer_reset_index = i;
		if(ioctl(mDevice.fd, DC10_SET_BUFFER,&mDevice, sizeof(dc10_config)) < 0)
		{
			//error in ioctl
			ERROR("dc10codec::init ioctl error DC10_SET_BUFFER \n");
		}
	}
	
	
	
	video_mode =  mod;
	frame(true);
	//JPEG process control P_reset
	uint32 *preg = (uint32 *)fRegAddress;
	preg += 65;
	*preg =  0;
	snooze(10);
	set_quality(rate);
    set_format(format,width,height);
	set_codec(mod);
	set_interface();
}

void dc10codec::enable(void)
{
	//FUNCTION("dc10codec :: enable\n");
	
	uint32 * preg;
	
	//JPEG process control P_reset
	preg = (uint32 *)fRegAddress;
	preg += 65;
	*preg =  (1 << 7) ;
	snooze(10);
	
	
	//CFlush MPEG code transfer control register
	preg = (uint32 *)fRegAddress;
	preg += 13;
	*preg =  *preg & (~(1 << 28)) ;
	snooze(10);
	
	//JPEG process control CodTrnsEn
	preg = (uint32 *)fRegAddress;
	preg += 65;
	*preg |=  (1 << 5) ;
	snooze(10);
	
	//interrupt
	
	
	//enable
	preg = (uint32 *)fRegAddress;
	preg += 16;
	*preg = (1<<27) | (1 << 29)|(1 << 30)| (1<<24);	
	snooze(10);
	
	//reset all
	preg = (uint32 *)fRegAddress;
	preg += 15;
	*preg = (1<< 27) | (1 << 29) |(1 << 30);
	snooze(10);
	
	frame(false);
	
	//JPEG codec guest ID 
	preg = (uint32 *)fRegAddress;
	preg += 73;
	*preg = 1 << 4;
	
	//JPEG process control Active
	preg = (uint32 *)fRegAddress;
	preg += 65; 
	*preg = *preg | (1);
	
	//JPEG mode and control GO_enable
	preg = (uint32 *)fRegAddress;
	preg += 64; 
	*preg = *preg | (1 << 5);
	//snooze(10);
	
	
	
	snooze(30);
	
	frame(true);

}

void dc10codec::disable(void)
{
	//FUNCTION("dc10codec :: disable\n");
	
	frame(false);
	uint32 *preg = (uint32 *)fRegAddress;
	preg += 16;
	*preg = 0;
	snooze(10);
	
	preg = (uint32 *)fRegAddress;
	uint32 mask = 1 << 5;
	
	preg += 64;
	*preg = *preg & (~mask);
	snooze(10);
	
	preg ++;
	*preg =0;
	snooze(10);
		
	sleep(true);
	snooze(10);
	
	
}



void dc10codec::setup(int32 fd,uint32 mode)
{
	mDevice.fd = fd; 
	if(ioctl(mDevice.fd, DC10_GET_INFO, &mDevice, sizeof(dc10_config)) < 0)
	{
		ERROR("dc10codec::setup erreur DC10_GET_INFO ioctl\n");
		return;
	}
	
	//get the address of the register of ZR36067
	fRegAddress = mDevice.reg_address;
	//get the address of the buffer for JPEG data
	fBufferPhysicalAddress = mDevice.buffer_physical_address;
	 
	uint32 * preg=(uint32*)fRegAddress;
	   
	//reset ZR36067
	preg+=10;
	*preg=0;
	snooze(2);
	*preg=1<<24;
	snooze(2);
	*preg=1<<24;
	snooze(2);

	
	//set duration time & recovery time for guest 0..3
	preg++;
	*preg=(*preg|(8<<12));
	*preg=(*preg&~(7<<12));
	*preg=(*preg|(8<<8));
	*preg=(*preg&~(7<<8));
	*preg=(*preg|(8<<4));
	*preg=(*preg&~(7<<4));
	*preg=(*preg|(8));
	*preg=(*preg&~(7));
	
	preg=(uint32*)fRegAddress;
	preg += 75;
	*preg=(*preg|(8<<12));
	*preg=(*preg&~(7<<12));
	*preg=(*preg|(8<<8));
	*preg=(*preg&~(7<<8));
	*preg=(*preg|(8<<4));
	*preg=(*preg&~(7<<4));
	*preg=(*preg|(8));
	*preg=(*preg&~(7));
	
	
	preg=(uint32*)fRegAddress;
	preg += 11;
	
	//reset ADV7176

	*preg=*preg&(~(1<<31));
	snooze(10);
	*preg=*preg|(1<<31);
	snooze(10);
	
	//set video bus direction
	if(mode == MODE_COMPRESS)
	{
		*preg=*preg&(~(1<<25));
	}
	else
	{
		*preg=*preg|(1<<25);
	}
	
	//set video bus enable
	*preg=*preg&(~(1<<26));
	
	
	//set Zoran frequency
	*preg=*preg&(~(1<<29));
	
	
	//set ADV7176 frequency
	*preg=*preg&(~(1<<28));
	
	
	//set ZR36060 SLEEP
	*preg=*preg&(~(1<<27));
	
	preg=(uint32*)fRegAddress;
	uint32 ack=0;
	mDevice.encoder_ad=0x00;	
	mDevice.decoder_ad=0x00;	
	
	//find SAA7110 address
	ack=i2c_probe(fRegAddress,0x9c);
	if(ack==0)
	{
		mDevice.decoder_ad=0x9c;		
	}
	ack=i2c_probe(fRegAddress,0x9e);
	if(ack==0)
	{
		mDevice.decoder_ad=0x9e;		
	}
	
	//find ADV 7175 address
	ack=i2c_probe(fRegAddress,0xd4);
	if(ack==0)
	{
		mDevice.encoder_ad=0xd4;		
	}
	ack=i2c_probe(fRegAddress,0xd6);
	if(ack==0)
	{
		mDevice.encoder_ad=0xd6;		
	}
	
	//find ADV 7176 address
	ack=i2c_probe(fRegAddress,0x54);
	if(ack==0)
	{
		mDevice.encoder_ad=0x54;		
	}
	ack=i2c_probe(fRegAddress,0x56);
	if(ack==0)
	{
		mDevice.encoder_ad=0x56;		
	}
	
		
	//FUNCTION("dc10codec::setup encoder et decoder %ld %ld \n",mDevice.encoder_ad,mDevice.decoder_ad);	
}


void dc10codec::decoder_init(unsigned char mode)
{
	uint32 format;
	format =  decoder_initialise(mDevice.reg_address,
				 		   		mDevice.decoder_ad,
				 		   		mDevice.encoder_ad,
				 		   		mode);	

	encoder_command(mDevice.reg_address,
				 	mDevice.encoder_ad,
				 	format);
	
	mDevice.video_format = format;	 
}

void dc10codec::encoder_commande(unsigned char commande)
{
	encoder_command(mDevice.reg_address,
				 	mDevice.encoder_ad,
				 	commande);
	
}
void dc10codec::decoder_commande(unsigned char commande)
{
	decoder_command(mDevice.reg_address,
				 	mDevice.decoder_ad,
				 	commande);
	
}

void dc10codec::brightness(float value)
{
	decoder_brightness(mDevice.reg_address,
				 	   mDevice.decoder_ad,
				 	   value);
}
void dc10codec::saturation(float value)
{
	decoder_saturation(mDevice.reg_address,
				 	   mDevice.decoder_ad,
				 	   value);
}
void dc10codec::contrast(float value)
{
	decoder_contrast(mDevice.reg_address,
				 	 mDevice.decoder_ad,
				 	 value);
}

void dc10codec::hue(float value)
{
	decoder_hue(mDevice.reg_address,
				mDevice.decoder_ad,
				value);
}	


void dc10codec::i2c_set_data(void * memadd,uint32 bit)
{
	uint32 * pdata;
	pdata=(uint32*)memadd;
	pdata+=0x011;
	if(bit==0)
	{
		*pdata=*pdata&1;
	}
	else
	{
		*pdata=*pdata|2;
	}
	snooze(10);
}

uint32 dc10codec::i2c_get_data(void * memadd)
{
	uint32 * pdata,bit;
	snooze(10);
	pdata=(uint32*)memadd;
	pdata+=0x011;
	bit=(*pdata&2)>1;
	return bit;
}

void dc10codec::i2c_set_clock(void * memadd,uint32 bit)
{
	uint32 * pdata;
	pdata=(uint32*)memadd;
	pdata+=0x011;
	if(bit==0)
	{
		*pdata=*pdata&2;
	}
	else
	{
		*pdata=*pdata|1;
	}
	snooze(10);
}

void dc10codec::i2c_start(void * memadd)
{
	i2c_set_data(memadd,1);
	i2c_set_clock(memadd,1);
	i2c_set_data(memadd,0);
	i2c_set_clock(memadd,0);	
}

void dc10codec::i2c_stop(void * memadd)
{
	i2c_set_clock(memadd,0);
	i2c_set_data(memadd,0);
	i2c_set_clock(memadd,1);	
	i2c_set_data(memadd,1);
}

uint32 dc10codec::i2c_write_byte(void *  memadd,
				      			 uint32 data)
{
	int i=0,j;
	uint32 d=128;
	for(i=7;i>=0;i--)
	{
		j=0;
		if((data&d)==d)
		{
			j=1;
		}
		d=d/2;
		i2c_set_clock(memadd,0);
		i2c_set_data(memadd,j);
		i2c_set_clock(memadd,1);		
	}	
	
	i2c_set_clock(memadd,0);
    i2c_set_data(memadd,1);
    i2c_set_clock(memadd,1);
    uint32 ack =i2c_get_data(memadd); 
    i2c_set_clock(memadd,0);
    return ack;
}

uint32 dc10codec::i2c_read_byte(void *  memadd)
{
	int i=0,j;
	uint32 d=128,data=0;
	for(i=7;i>=0;i--)
	{
		i2c_set_clock(memadd,0);
		i2c_set_data(memadd,1);
		i2c_set_clock(memadd,1);
		j=i2c_get_data(memadd);
		
		if(j==1)
		{
			data=data+d;
		}
		d=d/2;
				
	}	
	
	i2c_set_clock(memadd,0);
    i2c_set_data(memadd,0);
    i2c_set_clock(memadd,1);
    i2c_set_clock(memadd,0);
    return data;
}

uint32 dc10codec::i2c_probe(void * memadd,
				 			unsigned char add)
{
	uint32 ack=0;	
	i2c_start(memadd);
	ack=i2c_write_byte(memadd,add);
	i2c_stop(memadd);
	return ack;
}

uint32 dc10codec::i2c_write(void * memadd,
				 			unsigned char add,
				 			unsigned char subadd,
				 			unsigned char data)
{
	uint32 ack=0;
	i2c_start(memadd);
	ack=i2c_write_byte(memadd,add);
	i2c_write_byte(memadd,subadd);
	i2c_write_byte(memadd,data);
	i2c_stop(memadd);
	return ack;
}

uint32 dc10codec::i2c_read(void * memadd,
						   unsigned char add)
{
	uint32 ack=0;
	i2c_start(memadd);
	ack=i2c_write_byte(memadd,add|1);;
	ack=i2c_read_byte(memadd);
	i2c_stop(memadd);
	return ack;
}

uint32 dc10codec::i2c_write_block(void * memadd,
				 	   			  unsigned char add,
				 	   			  unsigned char subadd,
				 	   			  unsigned char * pdata,
				 	   			  int length)
{
	uint32 ack=0;
	i2c_start(memadd);
	ack=i2c_write_byte(memadd,add);
	i2c_write_byte(memadd,subadd);
	for(int i=0;i<length;i++)
	{
		i2c_write_byte(memadd,pdata[i]);
	}
	i2c_stop(memadd);
	return ack;
}


void dc10codec::decoder_mode(void * memadd,
				  			 unsigned char add,
				  			 unsigned char mode)
{
	if(mode==0)
	{
		i2c_write(memadd,add,0x06,0x03);
		i2c_write(memadd,add,0x20,0xd9);
		i2c_write(memadd,add,0x21,0x17);
		i2c_write(memadd,add,0x22,0x40);
		i2c_write(memadd,add,0x2c,0x03);
		i2c_write(memadd,add,0x30,0x44);
		i2c_write(memadd,add,0x31,0x75);
		i2c_write(memadd,add,0x21,0x16);
	}
	if(mode==1)
	{
		i2c_write(memadd,add,0x06,0x00);
		i2c_write(memadd,add,0x20,0xd8);
		i2c_write(memadd,add,0x21,0x17);
		i2c_write(memadd,add,0x22,0x40);
		i2c_write(memadd,add,0x2c,0x03);
		i2c_write(memadd,add,0x30,0x44);
		i2c_write(memadd,add,0x31,0x75);
		i2c_write(memadd,add,0x21,0x16);
	}
	if(mode==2)
	{
		i2c_write(memadd,add,0x06,0x00);
		i2c_write(memadd,add,0x20,0xba);
		i2c_write(memadd,add,0x21,0x07);
		i2c_write(memadd,add,0x22,0x91);
		i2c_write(memadd,add,0x2c,0x03);
		i2c_write(memadd,add,0x30,0x60);
		i2c_write(memadd,add,0x31,0xb5);
		i2c_write(memadd,add,0x21,0x05);
	}
	if(mode==3)
	{
		i2c_write(memadd,add,0x06,0x00);
		i2c_write(memadd,add,0x20,0xb8);
		i2c_write(memadd,add,0x21,0x07);
		i2c_write(memadd,add,0x22,0x91);
		i2c_write(memadd,add,0x2c,0x03);
		i2c_write(memadd,add,0x30,0x60);
		i2c_write(memadd,add,0x31,0xb5);
		i2c_write(memadd,add,0x21,0x05);
	}
	if(mode==4)
	{
		i2c_write(memadd,add,0x06,0x00);
		i2c_write(memadd,add,0x20,0x7c);
		i2c_write(memadd,add,0x21,0x07);
		i2c_write(memadd,add,0x22,0xd2);
		i2c_write(memadd,add,0x2c,0x83);
		i2c_write(memadd,add,0x30,0x60);
		i2c_write(memadd,add,0x31,0xb5);
		i2c_write(memadd,add,0x21,0x03);
	}
	if(mode==5)
	{
		i2c_write(memadd,add,0x06,0x00);
		i2c_write(memadd,add,0x20,0x78);
		i2c_write(memadd,add,0x21,0x07);
		i2c_write(memadd,add,0x22,0xd2);
		i2c_write(memadd,add,0x2c,0x83);
		i2c_write(memadd,add,0x30,0x60);
		i2c_write(memadd,add,0x31,0xb5);
		i2c_write(memadd,add,0x21,0x03);
	}
	if(mode==6)
	{
		i2c_write(memadd,add,0x06,0x80);
		i2c_write(memadd,add,0x20,0x59);
		i2c_write(memadd,add,0x21,0x17);
		i2c_write(memadd,add,0x22,0x42);
		i2c_write(memadd,add,0x2c,0xa3);
		i2c_write(memadd,add,0x30,0x44);
		i2c_write(memadd,add,0x31,0x75);
		i2c_write(memadd,add,0x21,0x12);
	}
	if(mode==7)
	{
		i2c_write(memadd,add,0x06,0x83);
		i2c_write(memadd,add,0x20,0x9a);
		i2c_write(memadd,add,0x21,0x17);
		i2c_write(memadd,add,0x22,0xb1);
		i2c_write(memadd,add,0x2c,0x13);
		i2c_write(memadd,add,0x30,0x60);
		i2c_write(memadd,add,0x31,0xb5);
		i2c_write(memadd,add,0x21,0x14);
	}
	if(mode==8)
	{
		i2c_write(memadd,add,0x06,0x80);
		i2c_write(memadd,add,0x20,0x3c);
		i2c_write(memadd,add,0x21,0x27);
		i2c_write(memadd,add,0x22,0xc1);
		i2c_write(memadd,add,0x2c,0x23);
		i2c_write(memadd,add,0x30,0x44);
		i2c_write(memadd,add,0x31,0x75);
		i2c_write(memadd,add,0x21,0x21);
	}
}

uint32 dc10codec::decoder_initialise(void * memadd,
				  					 unsigned char add,
				  					 unsigned char /*encoder_add*/,
				  					 unsigned char mode)
{
	i2c_write_block(memadd,add,0x00,init_decoder,53);
	if(mode == 1)
	{
		printf("dc10codec::decoder_initialise mode 7\n");
		decoder_mode(memadd,add,7);
	}
	else
	{
		printf("dc10codec::decoder_initialise mode 0\n");
		decoder_mode(memadd,add,0);
	}
	 
	uint32 format_video=decoder_command(memadd,add,FIND_FORMAT);
	return format_video;
}

uint32 dc10codec::decoder_command(void * memadd,
				 	 			  unsigned char add,
				 	 			  uint32 command)
{
	if((command)==FIND_FORMAT)
	{
		//i2c_write_block(memadd,add,0x00,init,53);
	
		//i2c_write(memadd,add,0x21,0x16);
		snooze(150000);
		uint32 data=i2c_read(memadd,add);
		
		if((data&0x40)==0x40)
		{
			//NO FORMAT
			printf("dc10codec::decoder_command Format IN NO\n");
			return NO_FORMAT;
		}
		if((data&0x23)==0)
		{
			//BW50HZ
			printf("dc10codec::decoder_command Format IN BW50HZ\n");
			i2c_write(memadd,add,0x06,0x80);
			i2c_write(memadd,add,0x2e,0x9a);
			return FORMAT_BW50;
		}
		if((data&0x23)==0x20)
		{
			//BW60HZ
			printf("dc10codec::decoder_command Format IN BW60HZ\n");
			i2c_write(memadd,add,0x06,0x80);
			i2c_write(memadd,add,0x2e,0x81);
			return FORMAT_BW60;
		}
		
		i2c_write(memadd,add,0x06,0x03);
		
		if((data&0x20)==0x20)
		{
			//NTSC
			printf("dc10codec::decoder_command Format IN NTSC\n");
			i2c_write(memadd,add,0x0d,0x06);
			i2c_write(memadd,add,0x11,0x2c);
			i2c_write(memadd,add,0x0F,0x50);
			//i2c_write(memadd,add,0x2e,0x81);
			return FORMAT_NTSC; 
		}
		
		if((data&0x20)==0x00)
		{
			//PAL
			printf("dc10codec::decoder_command Format IN PAL\n");
			i2c_write(memadd,add,0x0d,0x86);
			i2c_write(memadd,add,0x0f,0x10);
			i2c_write(memadd,add,0x11,0x59);
			//i2c_write(memadd,add,0x2e,0x9a);
			snooze(150000);
			data=i2c_read(memadd,add);
			if((data&0x03)==0x01)
			{
				//SECAM
				printf("dc10codec::decoder_command Format IN SECAM\n");
				i2c_write(memadd,add,0x0d,0x87);
				return FORMAT_SECAM;
			}
			else
			{
				return FORMAT_PAL;
			}
		}
		return NO_FORMAT;
	}
	
	if((command)==INPUT_ZR36060)
	{
		printf("dc10codec::decoder_command Decoder INPUT_ZR36060\n");
		i2c_write(memadd,add,0x0e,0x80);
	}
	if((command)==FORMAT_NTSC)
	{
		printf("dc10codec::decoder_command Decoder NTSC FORMAT\n");
		i2c_write_block(memadd,add,0x00,init_decoder,53);
		i2c_write(memadd,add,0x21,0x10);
		i2c_write(memadd,add,0x0e,0x18);
		i2c_write(memadd,add,0x0d,0x04);
		i2c_read(memadd,add);
		i2c_write(memadd,add,0x0d,0x06);
		i2c_read(memadd,add);
		i2c_write(memadd,add,0x0d,0x86);
		i2c_write(memadd,add,0x0f,0x10);
		i2c_write(memadd,add,0x11,0x59);
		
		i2c_write(memadd,add,0x0d,0x86);
		i2c_write(memadd,add,0x0f,0x50);
		i2c_write(memadd,add,0x11,0x2c);
		
	}
	return NO_FORMAT;

}


void dc10codec::decoder_brightness(void * memadd,
				 		  		   unsigned char add,
				 		  		   float value)
{
	unsigned char mValue = 0;
	float fValue = 0.0;
	fValue =(value + 100.0) / 200.0 *255.0;
	mValue = (unsigned char) fValue;
	i2c_write(memadd,add, 0x19, mValue);
}


void dc10codec::decoder_contrast(void * memadd,
				 	  			 unsigned char add,
				 	  			 float value)
{
	unsigned char mValue = 0;
	float fValue = 0.0;
	fValue =(value + 100.0)  * 127.0 / 200.0 ;
	mValue = (unsigned char) fValue;
	i2c_write(memadd,add, 0x13, mValue);
}

void dc10codec::decoder_saturation(void * memadd,
				 				   unsigned char add,
				 				   float value)
{
	unsigned char mValue = 0;
	float fValue = 0.0;
	fValue =(value + 100.0)  * 127.0 / 200.0  ;
	mValue = (unsigned char) fValue;
	i2c_write(memadd,add, 0x12, mValue);
}
				 		  
void dc10codec::decoder_hue(void * memadd,
				 			unsigned char add,
				 			float value)
{
	unsigned char mValue = 0;
	float fValue = 0.0;
	fValue =(value )  * 127.0 / 100.0;
	mValue = (unsigned char) fValue;
	i2c_write(memadd,add, 0x07, mValue);
}
		
		
void dc10codec::encoder_command(void * memadd,
				  	 			unsigned char add,
				  	 			uint32 command)
{
	uint32 * preg=(uint32*)memadd;
	preg+=0x0b;
	
	if((command)==NO_FORMAT)
	{
		//set ADV7176 frequency
		*preg=*preg|(1<<28);
		i2c_write_block(memadd,add,0x00,init_encoder_pal_mir,19);
		i2c_write(memadd,add,0x07,0x4e|0x80);
		i2c_write(memadd,add,0x07,0x4e);
		printf("dc10codec::encoder_command encoder :: to pal mir\n");
	}
	if((command)==FORMAT_NTSC)
	{
		//set ADV7176 frequency
		*preg=*preg&(~(1<<28));
		i2c_write_block(memadd,add,0x00,init_encoder_ntsc,19);
		i2c_write(memadd,add,0x07,0x4e|0x80);
		i2c_write(memadd,add,0x07,0x4e);
		printf("dc10codec::encoder_command encoder :: to ntsc\n");
	}
	if((command)==FORMAT_PAL)
	{
		//set ADV7176 frequency
		*preg=*preg&(~(1<<28));
		printf("dc10codec::encoder_command encoder :: to pal\n");
		i2c_write_block(memadd,add,0x00,init_encoder_pal,19);
		i2c_write(memadd,add,0x07,0x4e|0x80);
		i2c_write(memadd,add,0x07,0x4e);		
	}
	if((command)==FORMAT_SECAM)
	{
		//set ADV7176 frequency
		*preg=*preg&(~(1<<28));
		printf("dc10codec::encoder_command encoder :: secam to pal\n");
		i2c_write_block(memadd,add,0x00,init_encoder_secam,19);
		i2c_write(memadd,add,0x07,0x4e|0x80);
		i2c_write(memadd,add,0x07,0x4e);		
	}
	if((command)==FORMAT_PALM)
	{
		printf("dc10codec::encoder_command encoder :: to pal m\n");
	}
	if((command)==FORMAT_PALN)
	{
		printf("dc10codec::encoder_command encoder :: to pal N\n");	
	}
	if((command)==INPUT_SAA7110)
	{
		printf("dc10codec::encoder_command encoder :: input saa7110\n");
		i2c_write(memadd,add,0x01,0x00);
		//i2c_write(memadd,add,0x06,0x00);
		i2c_write(memadd,add,0x0c,0x80);
		i2c_write(memadd,add,0x0d,0x4f);
		i2c_write(memadd,add,0x07,0xce);
		i2c_write(memadd,add,0x07,0x4e);
		
	}
	if((command)==INPUT_ZR36060)
	{
		printf("dc10codec::encoder_command encoder :: input zr36060\n");	
		i2c_write(memadd,add,0x01,0x00);
		//i2c_write(memadd,add,0x06,0x00);
		i2c_write(memadd,add,0x0c,0x00);
		i2c_write(memadd,add,0x0d,0x4f);
		i2c_write(memadd,add,0x07,0xce);
		i2c_write(memadd,add,0x07,0x4e);
	}
	if((command)==INPUT_MASTER)
	{	
		printf("dc10codec::encoder_commandencoder :: master\n");
		//set ADV7176 frequency
		*preg=*preg|((1<<28));
		
		i2c_write(memadd,add,0x01,0x80);
		i2c_write(memadd,add,0x06,0x00);
		i2c_write(memadd,add,0x0c,0x00);
		i2c_write(memadd,add,0x0d,0x00);
		i2c_write(memadd,add,0x07,0xc9);
		i2c_write(memadd,add,0x07,0x49);
	}
	if((command)==COLOR_MIR)
	{	
		printf("dc10codec::encoder_command encoder :: color mir\n");
		i2c_write(memadd,add, 0x01, 0x80);
        i2c_write(memadd,add, 0x0d, 0x49);
     	i2c_write(memadd,add, 0x07, 0xce);
     	i2c_write(memadd,add, 0x07, 0x4e);
	}
}	 
