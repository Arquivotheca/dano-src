#ifndef DC10CODEC_H
#define DC10CODEC_H

#include <stdio.h>
#include <stdlib.h>
#include <SupportDefs.h>

#include <dc10_driver.h>

#define MODE_COMPRESS 1
#define MODE_DECOMPRESS 2

class dc10codec
{
public:
						dc10codec();
						~dc10codec();
		status_t 		post_office_wait(void);
		status_t		post_office_write(uint32 guest, uint32 reg, uint32 value);
		int32			post_office_read( uint32 guest, uint32 reg);
		status_t 		write(uint32 reg, uint32 val,uint32 size);
		
		int32			read_8(uint32 reg);
		
		void 			sleep(bool state);
		void 			reset(void);
		void 			frame(bool state);
		
		
		void 			set_quality(uint32 quality);
		void 			set_format(uint32 format,uint32 width,uint32 height);
		
		void 			set_codec(void);
		void 			set_codec(uint32 mode);
		void			set_interface(void);
		
		void			decoder_init(unsigned char mode);

		void			decoder_commande(unsigned char command);
		void			encoder_commande(unsigned char command);
		
		void 			setup(int32 fd,uint32 mode);
		void 			init(uint32 format,uint32 width,uint32 height,uint32 mode,uint32 quality);
		void			enable(void);
		void			disable(void);
		
		void 			brightness(float value);
		void 			saturation(float value);
		void 			contrast(float value);
		void 			hue(float value);			 
		
		dc10_config		mDevice;
		
private:			
			
		void 			i2c_set_data(void * memadd,
						 			 uint32 bit);
		uint32 			i2c_get_data(void * memadd);
		void 			i2c_set_clock(void * memadd,
						  			  uint32 bit);
		void 			i2c_start(void * memadd);
		void 			i2c_stop(void * memadd);
		uint32 			i2c_write_byte(void *  memadd,
				      		 		   uint32 data);
		uint32 			i2c_read_byte(void *  memadd);				      		 
		uint32 			i2c_probe(void * memadd,
				 				  unsigned char add);
		uint32 			i2c_write(void * memadd,
				 				  unsigned char add,
				 				  unsigned char subadd,
				 				  unsigned char data);
		uint32 			i2c_read(void * memadd,
				 				 unsigned char add);
		uint32 			i2c_write_block(void * memadd,
				 	   		  			unsigned char add,
				 	   		  			unsigned char subadd,
				 	   	 	  			unsigned char * pdata,
				 	   		  			int length);

		void 			decoder_mode(void * memadd,
				 		 			 unsigned char add,
				 		 			 unsigned char mode);
		uint32 			decoder_initialise(void * memadd,
				 		 				   unsigned char add,
				 		 				   unsigned char encoder_add,
				 		 				   unsigned char mode);
		uint32 			decoder_command(void * memadd,
				 		 	 			unsigned char add,
				 		 	 			uint32 command);
		void 			decoder_brightness(void * memadd,
				 		 	   			   unsigned char add,
				 		 	   			   float value);
		void 			decoder_contrast(void * memadd,
				 		 	 			 unsigned char add,
				 		 	 			 float value);
		void 			decoder_saturation(void * memadd,
				 		 	   			   unsigned char add,
				 		 	   			   float value);
		void 			decoder_hue(void * memadd,
				 					unsigned char add,
				 					float value);
		
		void 			encoder_command(void * memadd,
				  	 					unsigned char add,
				  	 					uint32 command);
				  	 				 		
				 		
		void 			*fRegAddress;
		void 			*fBufferPhysicalAddress;
		uint32 			mQuality;
		//uint32			mInterlaced;
		uint32 			mBufferSize;		
		uint32 			Wa,Wt,HStart,HSyncStart;
		uint32 			Ha,Ht,VStart;
		uint32 			VScale,HScale;
		uint32			img_x,img_y;
		uint32 			video_mode;
		uint32 			video_width,video_height,video_format;
		uint32 			field_per_buff;

			
};


#endif
