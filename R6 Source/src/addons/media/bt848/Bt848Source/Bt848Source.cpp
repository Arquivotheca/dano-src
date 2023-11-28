/*
	
	Bt848Source.cpp
	
	Copyright 1997-8 Be Incorporated, All Rights Reserved.
	
*/

#include <errno.h>

#include "Bt848Source.h"
#include "Bt848I2C.h"
#include "Bt848Tuner.h"
#include "Bt848Hwinfo.h"
#include "Bt848AudioMux.h"
#include "Bt848VideoMux.h"
#include "Bt848VideoControls.h"

#define PLL(x...)
#define TRACE(x...)
#define PROBE(x...)

//------------------------------------------------------------------------------

Bt848Source::Bt848Source(const char *name):BVideoSource(name)
{
	if( (fBt848 = open(name,O_CLOEXEC)) > 0)
	{
		
		fConfig.gpio_out_en = 0xf0003f;
		ioctl(fBt848,BT848_WRITE_GPIO_OUT_EN,&fConfig);

		/* some safe defaults */
		fConfig.video_format = BT848_NTSC_M;
		fConfig.o_color_format = BT848_RGB32;
		fConfig.e_color_format = BT848_RGB32;
		fConfig.o_x_size = 320;
		fConfig.e_x_size = 320;
		fConfig.o_y_size = 240;
		fConfig.e_y_size = 240;
		fConfig.capture_mode = BT848_CONTINUOUS;	
		fConfig.e_address_type = BT848_LOGICAL;
		fConfig.o_address_type = BT848_LOGICAL;
		fConfig.e_endian = BT848_LITTLE;
		fConfig.o_endian = BT848_LITTLE;
		fConfig.e_clip = NULL;
		fConfig.o_clip = NULL;
		fConfig.e_line = NULL;
		fConfig.o_line = NULL;
		fConfig.e_vbi = NULL;
		fConfig.o_vbi = NULL;
		fConfig.status = 0;
		fConfig.video_source = 0;
		fConfig.decimate = 0;

		fConfig.number_buffers = 0;	/* number of buffers in ring */
		fConfig.current_buffer = 0;	/* index to current buffer in ring */
		fConfig.last_buffer = 0;
		fConfig.timeout = 40000;

		fConfig.frame_number = 0;
		fConfig.time_stamp = 0;
		fConfig.capture_status = 0;
		
		fConfig.i2c_register = 0;
		fConfig.i2c_address = 0;
		fConfig.i2c_data1 = 0;
		fConfig.i2c_data2 = 0;
		fConfig.i2c_status = 0;

		fConfig.gpio_out_en = 0;
		fConfig.gpio_in_en = 0;
		fConfig.gpio_data = 0;

		fConfig.brightness = 0;
		fConfig.contrast = 0;
		fConfig.hue = 0;
		fConfig.saturation = 0;

		fConfig.gamma = true;
		fConfig.error_diffusion = true;
		fConfig.luma_coring = true;
		fConfig.luma_comb = false;
		fConfig.chroma_comb = true;
		fConfig.color_bars = false;
			
		fConfig.pll = 0;
		fConfig.version = 0;
		fConfig.device_id = 0;

		fConfig.h_delay_adj = 0;
		fConfig.v_delay_adj = 0;

		fVideoFormat = B_NTSC_M;
		fVideoImageArrayF1 = NULL;
		fVideoImageArrayF2 = NULL;
		fOldVideoImageArrayF1 = NULL;
		fOldVideoImageArrayF2 = NULL;
		fOldVideoImage = true;
		fOlderVideoImage = true;
		fPrimaryRiscProgram = false;
		
		fI2CBus = new Bt848I2C(name, fBt848, &fConfig);		

		/* autodetect card brand */
		Probe();
		
		/* input muxes */
		fVideoMux = new Bt848VideoMux(name, fBt848, &fConfig);
		fAudioMux = new Bt848AudioMux(name, fBt848, &fConfig);
		
		fTuner = new Bt848Tuner(name, fBt848, &fConfig, &fHwInfo, fI2CBus, fAudioMux);	
		
		fVideoControls = new Bt848VideoControls(name, fBt848, &fConfig);
		
	}
}

//------------------------------------------------------------------------------

Bt848Source::~Bt848Source()
{
	close(fBt848);
}

//------------------------------------------------------------------------------

status_t
Bt848Source::InitCheck()
{
	if (fBt848 > 0)
		return B_NO_ERROR;
	else
		return B_ERROR;
}

//------------------------------------------------------------------------------

hw_info
Bt848Source::Probe()
{
	uchar	ee[256];

	ioctl(fBt848,BT848_VERSION,&fConfig);
	PROBE("Device: %d, Driver version: %d\n", fConfig.device_id, fConfig.version);
	
	fHwInfo.decoder = fConfig.device_id;	
	fHwInfo.video_format = B_NTSC_M;
	fHwInfo.tuner_type = 0;
	fHwInfo.tuner_mfg = BT848HW_NONE;
	fHwInfo.serial_number = 0;
	
	// If TDA9850, init it
	if (fI2CBus->I2CDevicePresent(0x5b))
	{
		fI2CBus->I2CWrite2(0x5b,0x04,0x08);
		fI2CBus->I2CWrite2(0x5b,0x05,0x08);
		fI2CBus->I2CWrite2(0x5b,0x06,0x40);
		fI2CBus->I2CWrite2(0x5b,0x07,0x07);
		fI2CBus->I2CWrite2(0x5b,0x08,0x10);
		fI2CBus->I2CWrite2(0x5b,0x09,0x90);
		fI2CBus->I2CWrite2(0x5b,0x0a,0x03);
	}
	
	// If TDA8425, init it
	if (fI2CBus->I2CDevicePresent(0x41))
	{
		fI2CBus->I2CWrite2(0x41,0x00,0xfc);
		fI2CBus->I2CWrite2(0x41,0x01,0xfc);
		fI2CBus->I2CWrite2(0x41,0x02,0xf6);
		fI2CBus->I2CWrite2(0x41,0x03,0xf6);
		fI2CBus->I2CWrite2(0x41,0x08,0xce);
	}	
			
/*	// reset MSP34XX so it will show up during scan
	fConfig.gpio_data = 0x000020;
	ioctl(fBt848, BT848_WRITE_GPIO_DATA, &fConfig);
	fConfig.gpio_data = 0x000000;
	ioctl(fBt848, BT848_WRITE_GPIO_DATA, &fConfig);
	snooze(2500);
	fConfig.gpio_data = 0x000020;
	ioctl(fBt848, BT848_WRITE_GPIO_DATA, &fConfig);
	
	for (uchar addr = 0; addr < 0x80; addr++)
		if (fI2CBus->I2CDevicePresent(addr))
			PROBE("I2C device found at 0x%02x\n",addr);			
*/
	if (fI2CBus->I2CDevicePresent(0x50))
	{
		PROBE("EEPROM Found");
		fI2CBus->I2CWrite1(0x50,0);
		for (int i = 0; i < 48; i++)
		{
			if (i%8 == 0)
			{
				//PROBE("\n%02x:  ",i);
			}
			ee[i] = fI2CBus->I2CRead(0x50);
			//PROBE("%02x ", ee[i]);	
		}
		
		if (ee[0] == 0x84)
		{
			PROBE("\nHauppauge brand card\n");
			PROBE("   Size of vendor info: %d\n", (uint32)ee[1] | ((uint32)ee[2] << 8));
			PROBE("   Version: %02x\n", ee[4]);
			PROBE("   Hardware details %02x\n", ee[5]);

			switch(ee[6])
			{
				case HEE_DECODER1_BT848:
					fHwInfo.decoder = 848;
					PROBE("   Video Decoder:  Bt848\n");
					break;
				case HEE_DECODER1_BT878:
					fHwInfo.decoder = 878;
					PROBE("   Video Decoder:  Bt878\n");
					break;
				case HEE_DECODER1_BT879:
					fHwInfo.decoder = 879;
					PROBE("   Video Decoder:  Bt879\n");
					break;
				default:
					fHwInfo.decoder = 848;
					PROBE("   Video Decoder:  Unknown, but assuming Bt848\n");
					break;
			}

			PROBE("   Video format:  %02x\n", ee[7]);
			switch (ee[7] & 0x3f) // mask off combo bits
			{
				case HEE_VidForm_M:
				case HEE_VidForm_NTSC_M:
				case HEE_VidForm_NTSC_443:
					fHwInfo.video_format = B_NTSC_M;
					PROBE("   Video format:  NTSC\n");
					break;
				case HEE_VidForm_PAL_BGHIDK:
					fHwInfo.video_format = B_PAL_BDGHI;
					PROBE("   Video format:  PAL_BDGHI\n");
					break;
				case HEE_VidForm_PAL_M:
					fHwInfo.video_format = B_PAL_M;
					PROBE("   Video format:  PAL_M\n");
					break;
				case HEE_VidForm_PAL_N:
				case HEE_VidForm_PAL_NCOMBO:
					fHwInfo.video_format = B_PAL_N;
					PROBE("   Video format:  PAL_N\n");
					break;
				case HEE_VidForm_SECAM_L:
					fHwInfo.video_format = B_SECAM;
					PROBE("   Video format:  SECAM\n");
					break;
				default:
					fHwInfo.video_format = B_UNDEFINED_VIDEO_FORMAT;
					PROBE("   Video format:  UNDEFINED_VIDEO_FORMAT\n");
					break;			
			}			
			
			fHwInfo.tuner_type = ee[8];
			switch (fHwInfo.tuner_type)
			{
				default:
					PROBE("   Tuner Type:  %02x\n", fHwInfo.tuner_type);
					break;
			}
			
			fHwInfo.tuner_model = ee[9];
			PROBE("   Tuner Model:  %d\n", fHwInfo.tuner_model);
			switch (fHwInfo.tuner_model)
			{
				case HEE_TUNER_FI1216:
				case HEE_TUNER_FI1216MF:
				case HEE_TUNER_FI1236:
				case HEE_TUNER_FI1246:
				case HEE_TUNER_FI1256:
				case HEE_TUNER_FI1216_MK2:
				case HEE_TUNER_FI1216MF_MK2:
				case HEE_TUNER_FI1236_MK2:
				case HEE_TUNER_FI1246_MK2:
				case HEE_TUNER_FI1256_MK2:
				case HEE_TUNER_FR1216_MK2:
				case HEE_TUNER_FR1216MF_MK2:
				case HEE_TUNER_FR1236_MK2:
				case HEE_TUNER_FR1246_MK2:
				case HEE_TUNER_FR1256_MK2:
				case HEE_TUNER_FM1216:
				case HEE_TUNER_FM1216MF:
				case HEE_TUNER_FM1236:
				case HEE_TUNER_FM1246:
				case HEE_TUNER_FM1256:
					fHwInfo.tuner_mfg = BT848HW_PHILIPS;
					PROBE("   Tuner brand:  PHILIPS\n");
					break;
				case HEE_TUNER_4032FY5:
				case HEE_TUNER_4002FH5:
				case HEE_TUNER_4062FY5:
				case HEE_TUNER_4036FY5:
				case HEE_TUNER_4006FH5:
					fHwInfo.tuner_mfg = BT848HW_TEMIC;
					PROBE("   Tuner brand:  TEMIC\n");
					break;
				case HEE_TUNER_TCPN9082D:
				case HEE_TUNER_TCPM9092P:
				case HEE_TUNER_TCPN9085D:
				case HEE_TUNER_TCPB9085P:
				case HEE_TUNER_TCPL9091P:
					fHwInfo.tuner_mfg = BT848HW_SAMSUNG;
					PROBE("   Tuner brand:  SAMSUNG\n");
					break;
				default:
					fHwInfo.tuner_mfg = BT848HW_NONE;
					PROBE("   Tuner brand:  NONE\n");
					break;
			}
						
			PROBE("   Inputs:  %02x\n", ee[10]);
			
			fHwInfo.model_number  = (uint32)ee[11] | ((uint32)ee[12] << 8);
			PROBE("   Model number: %d\n",	fHwInfo.model_number);
					
			fHwInfo.serial_number = ee[32]<<16 | ee[31]<<8 | ee[30];
			PROBE("   Hauppauge Serial Number: %d\n", fHwInfo.serial_number);
		}
	}
	
	return(fHwInfo);	
}

//------------------------------------------------------------------------------

void	
Bt848Source::SetTunerBrand(bt848_tuner_mfg brand)
{
	fHwInfo.tuner_mfg = brand;
}

//------------------------------------------------------------------------------

bt848_tuner_mfg	
Bt848Source::TunerBrand() const
{
	return(fHwInfo.tuner_mfg);
}

//------------------------------------------------------------------------------

status_t
Bt848Source::AllocateCaptureBuffer(bt848_buffer *buf)
{
	return ((status_t)ioctl(fBt848,BT848_ALLOCATE_BUFFER,buf));
}

//------------------------------------------------------------------------------

void
Bt848Source::FreeCaptureBuffer(bt848_buffer *buf)
{
	ioctl(fBt848,BT848_FREE_BUFFER,buf);
}

//------------------------------------------------------------------------------

BAudioMux *
Bt848Source::AudioMux() const
{
	return fAudioMux;
}

//------------------------------------------------------------------------------

BVideoMux *
Bt848Source::VideoMux() const
{
	return fVideoMux;
}

//------------------------------------------------------------------------------

BTuner *
Bt848Source::Tuner() const
{
	return fTuner;
}


//------------------------------------------------------------------------------

BVideoControls *
Bt848Source::VideoControls() const
{
	return fVideoControls;
}


//------------------------------------------------------------------------------

BI2CBus *
Bt848Source::I2CBus() const
{
	return fI2CBus;
}


//------------------------------------------------------------------------------

status_t	
Bt848Source::SetVideoFormat(video_format format)
{
	switch (format)
	{
		case B_NTSC_M:
			fConfig.video_format = BT848_NTSC_M;
			break;
		case B_NTSC_J:
			fConfig.video_format = BT848_NTSC_JAPAN;
			break;
		case B_PAL_BDGHI:
			fConfig.video_format = BT848_PAL_BDGHI;
			break;
		case B_PAL_M:
			fConfig.video_format = BT848_PAL_M;
			break;
		case B_PAL_N:
			fConfig.video_format = BT848_PAL_N;
			break;
		case B_SECAM:
			fConfig.video_format = BT848_SECAM;
			break;
		default:
			return B_ERROR;
	}
	fVideoFormat = format;
	return B_NO_ERROR;
}

//------------------------------------------------------------------------------

video_format	
Bt848Source::VideoFormat() const
{
	return (fVideoFormat);
}

//------------------------------------------------------------------------------

void	
Bt848Source::SetCaptureMode(uint32 mode)
{
	fConfig.capture_mode = mode;
}

//------------------------------------------------------------------------------

uint32	
Bt848Source::CaptureMode() const
{
	return (fConfig.capture_mode);
}

//------------------------------------------------------------------------------

void	
Bt848Source::SetDecimation(uint32 decimation)
{
	fConfig.decimate = decimation;
}

//------------------------------------------------------------------------------

uint32	
Bt848Source::Decimation() const
{
	return(fConfig.decimate);
}

//------------------------------------------------------------------------------

status_t	
Bt848Source::SetColorSpace(color_space color, video_field_specifier type)
{
	uint32 device_format, endian;
	switch (color)
	{
		case B_RGB32:
		case B_RGBA32:
			device_format = BT848_RGB32;
			endian = BT848_LITTLE;
			break;
		case B_RGB24:
			device_format = BT848_RGB24;
			endian = BT848_LITTLE;
			break;
		case B_RGB16:
			device_format = BT848_RGB16;
			endian = BT848_LITTLE;
			break;
		case B_RGB15:
		case B_RGBA15:
			device_format = BT848_RGB15;
			endian = BT848_LITTLE;
			break;
		case B_CMAP8:
			device_format = BT848_RGB8;
			endian = BT848_LITTLE;
			break;
		case B_YCbCr422:
			device_format = BT848_YUV422;
			endian = BT848_LITTLE;
			break;
		case B_YCbCr411:
			device_format = BT848_YUV411;
			endian = BT848_LITTLE;
			break;
		case B_GRAY8:
			device_format = BT848_Y8;
			endian = BT848_LITTLE;
			break;
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
			device_format = BT848_RGB32;
			endian = BT848_BIG;
			break;
		case B_RGB24_BIG:
			device_format = BT848_RGB24;
			endian = BT848_BIG;
			break;
		case B_RGB16_BIG:
			device_format = BT848_RGB16;
			endian = BT848_BIG;
			break;
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
			device_format = BT848_RGB15;
			endian = BT848_BIG;
			break;
		default:
			return B_ERROR;	
	}
	switch (type)
	{
		case B_VIDEO_F1_ONLY:
			fConfig.o_color_format = device_format;
			fConfig.o_endian = endian;
			break;

		case B_VIDEO_F2_ONLY:
			fConfig.e_color_format = device_format;
			fConfig.e_endian = endian;
			break;
						
		case B_VIDEO_BOTH_FIELDS:
		default:
			fConfig.o_color_format = device_format;
			fConfig.e_color_format = device_format;
			fConfig.o_endian = endian;
			fConfig.e_endian = endian;
			break;
	}
	return B_NO_ERROR;
}

//------------------------------------------------------------------------------

color_space
Bt848Source::ColorSpace(video_field_specifier type) const
{
	uint32 device_format, endian;
	
	switch (type)
	{
		case B_VIDEO_F1_ONLY:
			device_format = fConfig.o_color_format;
			endian = fConfig.o_endian;
			break;
		case B_VIDEO_F2_ONLY:
			device_format = fConfig.e_color_format;
			endian = fConfig.e_endian;
			break;			
		case B_VIDEO_BOTH_FIELDS:
		default:
			device_format = fConfig.o_color_format;
			endian = fConfig.o_endian;
			break;
	}
	
	switch (device_format)
	{
		case BT848_RGB32:
			if (endian == BT848_LITTLE)
				return(B_RGB32);
			else
				return(B_RGB32_BIG);
		case BT848_RGB24:
			if (endian == BT848_LITTLE)
				return(B_RGB24);
			else
				return(B_RGB24_BIG);
		case BT848_RGB16:
			if (endian == BT848_LITTLE)
				return(B_RGB16);
			else
				return(B_RGB16_BIG);
		case BT848_RGB15:
			if (endian == BT848_LITTLE)
				return(B_RGB15);
			else
				return(B_RGB15_BIG);
		case BT848_RGB8:
			return(B_CMAP8);
		case BT848_YUV422:
			return(B_YCbCr422);
		case BT848_YUV411:
			return(B_YCbCr411);
		case BT848_Y8:
			return(B_GRAY8);
		default:
			return (B_NO_COLOR_SPACE);
	}
}

//------------------------------------------------------------------------------

void	
Bt848Source::SetAddressType(uint32 address_type, video_field_specifier type)
{
	switch (type) {
		case B_VIDEO_F1_ONLY:
			fConfig.o_address_type = address_type;
			break;

		case B_VIDEO_F2_ONLY:
			fConfig.e_address_type = address_type;
			break;
						
		case B_VIDEO_BOTH_FIELDS:
		default:
			fConfig.o_address_type = address_type;
			fConfig.e_address_type = address_type;
			break;
	}
}

//------------------------------------------------------------------------------

uint32	
Bt848Source::AddressType(video_field_specifier type) const
{
	switch (type) {
		case B_VIDEO_F1_ONLY:
			return(fConfig.o_address_type);

		case B_VIDEO_F2_ONLY:
			return(fConfig.e_address_type);
						
		case B_VIDEO_BOTH_FIELDS:
		default:
			return(fConfig.o_address_type);
	}
}

//------------------------------------------------------------------------------

status_t
Bt848Source::ConfigureCapture(	BVideoImage **videoImageArrayF1,
								bt848_cliplist clipListF1, 
								uint32 count,
								void **vbiBuffer,
								BVideoImage **videoImageArrayF2,
								bt848_cliplist clipListF2) 
{
	if (videoImageArrayF2 == NULL)
		return ConfigureSingleCapture(videoImageArrayF1, clipListF1, count, vbiBuffer);
	else
		return ConfigureDualCapture(videoImageArrayF1, clipListF1,
									videoImageArrayF2, clipListF2, count, vbiBuffer);

}

//------------------------------------------------------------------------------

status_t
Bt848Source::ConfigureSingleCapture(	BVideoImage **videoImageArray,
										bt848_cliplist clipList, 
										uint32 count,
										void **vbiBuffer) 
{	/* capture_buffer and vbi_buffer *must* be contiguous, locked buffers    */
	/* preferably alocated with Bt848Source::AllocateCaptureBuffer 			*/
	 
	uint32 			i, step, rows, n, vbi_samples;
	void			*even[3][B_PAL_SQUARE_V_MAX/2], *odd[3][B_PAL_SQUARE_V_MAX/2];
	void			*e_line[3], *o_line[3];
	void			*even_vbi[3][32],*odd_vbi[3][32];
	void			*e_vbi[3], *o_vbi[3];
	bt848_cliplist	eclip, oclip;
	uint32		bytes_per_row, lines_per_field;
	uint32 		buffer_base_F1, buffer_base_F2;

	TRACE("ConfigureSingleCapture\n");
		
	if (videoImageArray == NULL)
	{
		TRACE("ConfigureSingleCapture:  NULL BVideoImage pointer\n");	
		return B_ERROR;
	}
	
	fOldVideoImageArrayF1 = fVideoImageArrayF1;
	fOldVideoImageArrayF2 = fVideoImageArrayF2;
	fOlderVideoImage = fOldVideoImage;
	fOldVideoImage = true;

	fVideoImageArrayF1 = videoImageArray;
	fVideoImageArrayF2 = NULL;	
	fConfig.number_buffers = count;
			
	for (i=0; i < fConfig.number_buffers; i++)
	{
		e_line[i] = even[i];
		o_line[i] = odd[i];
		e_vbi[i] = even_vbi[i];
		o_vbi[i] = odd_vbi[i];
	}
			
	for (n = 0; n < count; n++)
	{
		BVideoImage *videoImage = videoImageArray[n];
	
		if (!videoImage)
		{
			TRACE("ConfigureSingleCapture:  no BVideoImage\n");	
			return B_ERROR;
		}

		if (!videoImage->IsValid())
		{
			TRACE("ConfigureSingleCapture:  BVideoImage not valid\n");	
			return B_ERROR;
		}
					
		if (videoImage->IsLogical())
		{
			TRACE("ConfigureSingleCapture:  Logical Address\n");	
			SetAddressType(BT848_LOGICAL);
		}
		else
		{
			TRACE("ConfigureSingleCapture:  Physical Address\n");	
			SetAddressType(BT848_PHYSICAL);
		}
			
		fConfig.o_x_size = (uint32)videoImage->ImageSize().x;
		if ((fVideoFormat == B_NTSC_M) || (fVideoFormat == B_NTSC_J))
			{
				if (fConfig.o_x_size > 720)
					fConfig.o_x_size = 720;
				lines_per_field  = 480/2;
			}
		else
			{
				if (fConfig.o_x_size > 768)
					fConfig.o_x_size = 768;
				lines_per_field  = 576/2;
			}
		TRACE("ConfigureSingleCapture:  image size is %d x %d\n",(uint32)videoImage->ImageSize().x, (uint32)videoImage->ImageSize().y);	
	
		switch (videoImage->Layout())
		{
			case B_INTERLEAVED:
				TRACE("ConfigureSingleCapture:  B_BUFFER_INTERLEAVED\n");	
				fConfig.o_line = (void ***)o_line;
						
				if (videoImage->ImageSize().y <= lines_per_field)  	/* overlay single fields if CIF or lower resolution */
				{
					step = 1;
					fConfig.e_line = NULL;		
					fConfig.o_y_size = (uint32)videoImage->ImageSize().y;
					SetColorSpace(videoImage->ColorSpace(), B_VIDEO_F1_ONLY);
				}
				else
				{
					step = 2;				/* otherwise interlace */
					fConfig.e_line = (void ***)e_line;
					fConfig.e_x_size = fConfig.o_x_size;
					fConfig.o_y_size = ((uint32)videoImage->ImageSize().y + 1)/2;
					fConfig.e_y_size = (uint32)videoImage->ImageSize().y - fConfig.o_y_size;
					if ((fVideoFormat == B_NTSC_M) || (fVideoFormat == B_NTSC_J))
						{
							if (fConfig.e_x_size > 720)
								fConfig.e_x_size = 720;
							if (fConfig.o_y_size > 240)
								fConfig.o_y_size = 240;
							if (fConfig.e_y_size > 240)
								fConfig.e_y_size = 240;
						}
					else
						{
							if (fConfig.e_x_size > 768)
								fConfig.e_x_size = 768;
							if (fConfig.o_y_size > 288)
								fConfig.o_y_size = 288;
							if (fConfig.e_y_size > 288)
								fConfig.e_y_size = 288;
						}
					SetColorSpace(videoImage->ColorSpace());
				}
	
				bytes_per_row	= videoImage->BytesPerRow();
				buffer_base_F1	= (uint32)videoImage->Buffer();
				buffer_base_F2	= buffer_base_F1;
			
				// Generate list of pointers to start of each odd scan line
				if (fConfig.o_line != NULL)
				{
					rows = fConfig.o_y_size;	
					if (videoImage->Orientation() == B_BUFFER_TOP_TO_BOTTOM)
					{
						TRACE("ConfigureSingleCapture:  B_BUFFER_TOP_TO_BOTTOM odd base = %x\n", buffer_base_F1);	
						for (i = 0; i < rows; i++)
						{
							odd[n][i] = (void *)(buffer_base_F1 + ((i*step)*bytes_per_row));
						}
					}
					else /* B_BUFFER_BOTTOM_UP */
					{
						TRACE("ConfigureSingleCapture:  B_BUFFER_BOTTOM_UP odd base = %x\n", buffer_base_F1);	
						for (i = 0; i < rows; i++)
						{
							odd[n][i] = (void *)(buffer_base_F1 + (((rows-1-i)*step)*bytes_per_row));
						}
					}
				}
				
				// Generate list of pointers to start of each even scan line
				if (fConfig.e_line != NULL)
				{
					rows = fConfig.e_y_size;	
					if (videoImage->Orientation() == B_BUFFER_TOP_TO_BOTTOM)
					{
						TRACE("ConfigureSingleCapture:  B_BUFFER_TOP_TO_BOTTOM even base = %x\n", buffer_base_F2);	
						for (i = 0; i < rows; i++)
						{
							even[n][i] = (void *)(buffer_base_F2 + (((i*step)+(step-1))*bytes_per_row));	
						}
					}
					else /* B_BUFFER_BOTTOM_UP */
					{
						TRACE("ConfigureSingleCapture:  B_BUFFER_BOTTOM_UP even base = %x\n", buffer_base_F2);	
						for (i = 0; i < rows; i++)
						{
							even[n][i] = (void *)(buffer_base_F2 + ((((rows-1-i)*step)+(step-1))*bytes_per_row));	
						}
					}
				}
				
				if (clipList != NULL)
				{
					fConfig.o_clip = (bt848_cliplist *)oclip;
						
					for (i = 0; i < fConfig.o_y_size; i++)
					{
						oclip[i] = clipList[i*step];
					}
					
					if (fConfig.e_line != NULL) // no clipping on even field if single field capture
					{
						fConfig.e_clip = (bt848_cliplist *)eclip;
						for (i = 0; i < fConfig.e_y_size; i++)
						{
							eclip[i] = clipList[i*step + 1];
						}
					}
					else fConfig.e_clip = NULL;
				}
				else
				{
					fConfig.o_clip = NULL;
					fConfig.e_clip = NULL;
				}
				break;
			case B_NONINTERLEAVED:
				TRACE("ConfigureSingleCapture:  B_BUFFER_NONINTERLEAVED\n");	
				step = 1;
				fConfig.o_line = (void ***)o_line;
						
				if (videoImage->ImageSize().y <= lines_per_field)  	/* overlay single fields if CIF or lower resolution */
				{
					fConfig.e_line = NULL;		
					fConfig.o_y_size = (uint32)videoImage->ImageSize().y;
					SetColorSpace(videoImage->ColorSpace(), B_VIDEO_F1_ONLY);
				}
				else
				{
					fConfig.e_line = (void ***)e_line;
					fConfig.e_x_size = fConfig.o_x_size;
					fConfig.o_y_size = ((uint32)videoImage->ImageSize().y + 1)/2;
					fConfig.e_y_size = (uint32)videoImage->ImageSize().y - fConfig.o_y_size;
					if ((fVideoFormat == B_NTSC_M) || (fVideoFormat == B_NTSC_J))
						{
							if (fConfig.e_x_size > 720)
								fConfig.e_x_size = 720;
							if (fConfig.o_y_size > 240)
								fConfig.o_y_size = 240;
							if (fConfig.e_y_size > 240)
								fConfig.e_y_size = 240;
						}
					else
						{
							if (fConfig.e_x_size > 768)
								fConfig.e_x_size = 768;
							if (fConfig.o_y_size > 288)
								fConfig.o_y_size = 288;
							if (fConfig.e_y_size > 288)
								fConfig.e_y_size = 288;
						}
					SetColorSpace(videoImage->ColorSpace());
				}
			
				bytes_per_row = videoImage->BytesPerRow();
				buffer_base_F1 = (uint32)videoImage->Buffer();
				buffer_base_F2 = (uint32)videoImage->Buffer() + fConfig.o_y_size * bytes_per_row;
			
				// Generate list of pointers to start of each odd scan line
				if (fConfig.o_line != NULL)
				{
					rows = fConfig.o_y_size;	
					if (videoImage->Orientation() == B_BUFFER_TOP_TO_BOTTOM)
					{
						for (i = 0; i < rows; i++)
						{
							odd[n][i] = (void *)(buffer_base_F1 + i*bytes_per_row);
						}
					}
					else /* B_BUFFER_BOTTOM_UP */
					{
						for (i = 0; i < rows; i++)
						{
							odd[n][i] = (void *)(buffer_base_F1 + (rows-1-i)*bytes_per_row);
						}
					}
				}
			
				// Generate list of pointers to start of each even scan line
				if (fConfig.e_line != NULL)
				{
					rows = fConfig.e_y_size;	
					if (videoImage->Orientation() == B_BUFFER_TOP_TO_BOTTOM)
					{
						for (i = 0; i < rows; i++)
						{
							even[n][i] = (void *)(buffer_base_F2 + i*bytes_per_row);	
						}
					}
					else /* B_BUFFER_BOTTOM_UP */
					{
						for (i = 0; i < rows; i++)
						{
							even[n][i] = (void *)(buffer_base_F2 + (rows-1-i)*bytes_per_row);	
						}
					}
				}
			
				if (clipList != NULL)
				{
					fConfig.o_clip = (bt848_cliplist *)oclip;
						
					for (i = 0; i < fConfig.o_y_size; i++)
					{
						oclip[i] = clipList[i*step];
					}
					
					if (fConfig.e_line != NULL) // no clipping on even field if single field capture
					{
						fConfig.e_clip = (bt848_cliplist *)eclip;
						for (i = 0; i < fConfig.e_y_size; i++)
						{
							eclip[i] = clipList[i*step + 1];
						}
					}
					else fConfig.e_clip = NULL;
				}
				else
				{
					fConfig.o_clip = NULL;
					fConfig.e_clip = NULL;
				}
				break;
			default:
				TRACE("ConfigSingleCapture: Invalid buffer_orientation\n");
				return B_ERROR;
		}
		
		if (vbiBuffer != NULL)
		{
			TRACE("ConfigSingleCapture: VBI capture enabled\n");
			fConfig.o_vbi = (void ***)o_vbi;
			fConfig.e_vbi = (void ***)e_vbi;
			
			if ((fVideoFormat == B_NTSC_M) || (fVideoFormat == B_NTSC_J))
				vbi_samples = B_NTSC_VBI_SAMPLES;
			else
				vbi_samples = B_PAL_VBI_SAMPLES;
	
			for (i = 0; i < 32; i++)
			{
				odd_vbi[n][i] = (void *)((uint32)vbiBuffer[n] + ( i     * vbi_samples));
				even_vbi[n][i] = (void *)((uint32)vbiBuffer[n] + ((i+32) * vbi_samples));
			}
		}
		else
		{
			TRACE("ConfigSingleCapture: VBI capture disabled\n");
			fConfig.o_vbi = NULL;
			fConfig.e_vbi = NULL;
		}
	}	
		
	// build risc program to dma video to specified scan line buffers
	// use specified Risc program memory
	status_t retValue;

	// ping pong between programs 
	fPrimaryRiscProgram = !fPrimaryRiscProgram;

	if (fPrimaryRiscProgram)
	{
		TRACE("Creating Primary Risc program\n");
		retValue = ioctl(fBt848,BT848_BUILD_PRI,&fConfig);
	}
	else
	{
		TRACE("Creating Alternate Risc program\n");
		retValue = ioctl(fBt848,BT848_BUILD_ALT,&fConfig);
	}
	
	return retValue;
}

//------------------------------------------------------------------------------

status_t
Bt848Source::ConfigureDualCapture(	BVideoImage **videoImageArrayF1,
									bt848_cliplist clipListF1, 
									BVideoImage **videoImageArrayF2,
									bt848_cliplist clipListF2, 
									uint32 count,
									void **vbiBuffer) 
{	/* capture_buffer and vbi_buffer *must* be contiguous, locked buffers    */
	/* preferably alocated with Bt848Source::AllocateCaptureBuffer 			*/
	 
	uint32 			i, n, rows, bytes_per_row, buffer_base_F1, buffer_base_F2, vbi_samples;
	void			*even[3][B_PAL_SQUARE_V_MAX/2], *odd[3][B_PAL_SQUARE_V_MAX/2];
	void			*e_line[3], *o_line[3];
	void			*even_vbi[3][32],*odd_vbi[3][32];
	void			*e_vbi[3], *o_vbi[3];

	TRACE("ConfigureDualCapture\n");	
		
	if ((videoImageArrayF1 == NULL) ||
		(videoImageArrayF2 == NULL))	
	{
		TRACE("ConfigureDualCapture:  NULL BVideoImage pointer\n");	
		return B_ERROR;
	}

	fOldVideoImageArrayF1 = fVideoImageArrayF1;
	fOldVideoImageArrayF2 = fVideoImageArrayF2;
	fOlderVideoImage = fOldVideoImage;
	fOldVideoImage = true;

	fVideoImageArrayF1 = videoImageArrayF1;
	fVideoImageArrayF2 = videoImageArrayF2;
	fConfig.number_buffers = count;
			
	for (i=0; i < fConfig.number_buffers; i++)
	{
		o_line[i] = odd[i];
		e_line[i] = even[i];
		e_vbi[i] = even_vbi[i];
		o_vbi[i] = odd_vbi[i];
	}
	
	for (n = 0; n < count; n++)
	{
		BVideoImage *videoImageF1 = videoImageArrayF1[n];
		BVideoImage *videoImageF2 = videoImageArrayF2[n];
	
		if (!(videoImageF1 && videoImageF2))
		{
			TRACE("ConfigureDualCapture:  no BVideoImage\n");	
			return B_ERROR;
		}				
	
		if (!(videoImageF1->IsValid() &&
			  videoImageF2->IsValid()) )
		{
			TRACE("ConfigureDualCapture:  BVideoImage not valid\n");	
			return B_ERROR;
		}				
	
		SetColorSpace(videoImageF1->ColorSpace(),B_VIDEO_F1_ONLY);
		SetColorSpace(videoImageF2->ColorSpace(),B_VIDEO_F2_ONLY);
	
		if (videoImageF1->IsLogical())
			SetAddressType(BT848_LOGICAL, B_VIDEO_F1_ONLY);
		else
			SetAddressType(BT848_PHYSICAL, B_VIDEO_F1_ONLY);
		if (videoImageF2->IsLogical())	
			SetAddressType(BT848_LOGICAL, B_VIDEO_F2_ONLY);
		else	
			SetAddressType(BT848_PHYSICAL, B_VIDEO_F2_ONLY);
	
		fConfig.o_x_size = (uint32) videoImageF1->ImageSize().x;
		fConfig.o_y_size = (uint32) videoImageF1->ImageSize().y;
		if ((fVideoFormat == B_NTSC_M) || (fVideoFormat == B_NTSC_J))
			{
				if (fConfig.o_x_size > 720)
					fConfig.o_x_size = 720;
				if (fConfig.o_y_size > 240)
					fConfig.o_y_size = 240;
			}
		else
			{
				if (fConfig.o_x_size > 768)
					fConfig.o_x_size = 768;
				if (fConfig.o_y_size > 288)
					fConfig.o_y_size = 288;
			}
		
		TRACE("Image size F1: %d x %d\n", fConfig.o_x_size,fConfig.o_y_size);
		 	
		fConfig.e_x_size = (uint32) videoImageF2->ImageSize().x;
		fConfig.e_y_size = (uint32) videoImageF2->ImageSize().y;
		if ((fVideoFormat == B_NTSC_M) || (fVideoFormat == B_NTSC_J))
			{
				if (fConfig.e_x_size > 720)
					fConfig.e_x_size = 720;
				if (fConfig.e_y_size > 240)
					fConfig.e_y_size = 240;
			}
		else
			{
				if (fConfig.e_x_size > 768)
					fConfig.e_x_size = 768;
				if (fConfig.e_y_size > 288)
					fConfig.e_y_size = 288;
			}

		TRACE("Image size F2: %d x %d\n",fConfig.e_x_size,fConfig.e_y_size);
	
		fConfig.o_line = (void ***)o_line;
		fConfig.e_line = (void ***)e_line;			
	
		bytes_per_row = videoImageF1->BytesPerRow();
		buffer_base_F1 = (uint32)videoImageF1->Buffer();
		rows = fConfig.o_y_size;	
		if (videoImageF1->Orientation() == B_BUFFER_TOP_TO_BOTTOM)
		{
			for (i = 0; i < rows; i++)
			{
				odd[n][i] = (void *)(buffer_base_F1 + (i*bytes_per_row));
			}
		}
		else /* B_BUFFER_BOTTOM_TO_TOP */
		{
			for (i = 0; i < rows; i++)
			{
				odd[n][i] = (void *)(buffer_base_F1 + ((rows-1-i)*bytes_per_row));
			}	
		}
	
		bytes_per_row = videoImageF2->BytesPerRow();
		buffer_base_F2 = (uint32)videoImageF2->Buffer();
		rows = fConfig.e_y_size;	
		if (videoImageF2->Orientation() == B_BUFFER_TOP_TO_BOTTOM)
		{
			for (i = 0; i < rows; i++)
			{
				even[n][i] = (void *)(buffer_base_F2 + (i*bytes_per_row));
			}
		}
		else /* B_BUFFER_BOTTOM_TO_TOP */
		{
			for (i = 0; i < rows; i++)
			{
				even[n][i] = (void *)(buffer_base_F2 + ((rows-1-i)*bytes_per_row));	
			}
		}
	
		if (clipListF1 == NULL)
			fConfig.o_clip = NULL;
		else	
			fConfig.o_clip = (bt848_cliplist *)clipListF1;
			
		if (clipListF2 == NULL)
			fConfig.e_clip = NULL;
		else	
			fConfig.e_clip = (bt848_cliplist *)clipListF2;

		if(vbiBuffer != NULL)
		{
			TRACE("ConfigDualCapture: VBI capture enabled\n");
			fConfig.o_vbi = (void ***)o_vbi;
			fConfig.e_vbi = (void ***)e_vbi;
	
			if ((fVideoFormat == B_NTSC_M) || (fVideoFormat == B_NTSC_J))
				vbi_samples = B_NTSC_VBI_SAMPLES;
			else
				vbi_samples = B_PAL_VBI_SAMPLES;
	
			for (i = 0; i < 32; i++)
			{
				odd_vbi[n][i] = (void *)((uint32)vbiBuffer[n] + ( i     * vbi_samples));
				even_vbi[n][i] = (void *)((uint32)vbiBuffer[n] + ((i+32) * vbi_samples));
			}
		}
		else
		{
			TRACE("ConfigDualCapture: VBI capture disabled\n");
			fConfig.o_vbi = NULL;
			fConfig.e_vbi = NULL;
		}
	}
	
	// build risc program to dma video to specified scan line buffers
	// use specified Risc program memory
	status_t retValue;

	/* ping pong between programs */
	fPrimaryRiscProgram = !fPrimaryRiscProgram;

	if (fPrimaryRiscProgram)
	{
		TRACE("Creating Primary Risc program\n");
		retValue = ioctl(fBt848,BT848_BUILD_PRI,&fConfig);
	}
	else
	{
		TRACE("Creating Alternate Risc program\n");
		retValue = ioctl(fBt848,BT848_BUILD_ALT,&fConfig);
	}
		
	return retValue;
}

//------------------------------------------------------------------------------

void
Bt848Source::StartCapture(bool resetFrameCount)
{
 	// init Bt848 registers
	ioctl(fBt848,BT848_INIT_REGS,&fConfig);
	
	if (resetFrameCount) {
		if (fPrimaryRiscProgram)
			ioctl(fBt848,BT848_START_PRI,&fConfig);	
		else
			ioctl(fBt848,BT848_START_ALT,&fConfig);	
	}
	else {
		if (fPrimaryRiscProgram)
			ioctl(fBt848,BT848_RESTART_PRI,&fConfig);	
		else
			ioctl(fBt848,BT848_RESTART_ALT,&fConfig);	
	}
}

//------------------------------------------------------------------------------

void
Bt848Source::StopCapture()
{
	ioctl(fBt848,BT848_STOP,&fConfig);
}

//------------------------------------------------------------------------------

void
Bt848Source::ContinueCapture()
{
	if (fPrimaryRiscProgram)
		ioctl(fBt848,BT848_RESTART_PRI,&fConfig);	
	else
		ioctl(fBt848,BT848_RESTART_ALT,&fConfig);
}

//------------------------------------------------------------------------------

void
Bt848Source::RestartCapture()
{
	if (fPrimaryRiscProgram)
		ioctl(fBt848,BT848_RESTART_PRI,&fConfig);	
	else
		ioctl(fBt848,BT848_RESTART_ALT,&fConfig);	
}

//------------------------------------------------------------------------------

status_t
Bt848Source::SwitchCapture(uint32 *index)
{
	if (fPrimaryRiscProgram)
	{
		TRACE("Switch to primary\n");
		if(ioctl(fBt848,BT848_SWITCH_PRI,&fConfig) < 0)
			return errno;
		TRACE("SUCCESS\n");
	}
	else
	{
		TRACE("Switch to alt\n");
		if(ioctl(fBt848,BT848_SWITCH_ALT,&fConfig) < 0)
			return errno;
		TRACE("SUCCESS\n");
	}
	if(index != NULL)
		*index = fConfig.last_buffer;
	return B_OK;
}

//------------------------------------------------------------------------------

BVideoImage *
Bt848Source::LastFrame(uint32 *index)
{
	bigtime_t offset;
	
	ioctl(fBt848,BT848_GET_LAST,&fConfig);

	if ((fVideoFormat == B_NTSC_M) || (fVideoFormat == B_NTSC_J))
		offset = 33367;
	else
		offset = 40000;
		
	if (fVideoImageArrayF1 != NULL)
	{
		fVideoImageArrayF1[fConfig.last_buffer]->SetStatus(fConfig.capture_status);
		fVideoImageArrayF1[fConfig.last_buffer]->SetTimestamp(fConfig.time_stamp - offset);
		fVideoImageArrayF1[fConfig.last_buffer]->SetFrameNumber(fConfig.frame_number);
		fVideoImageArrayF1[fConfig.last_buffer]->SetTimecode(FrameNumberToTimecode(fConfig.frame_number,
															fVideoImageArrayF1[fConfig.last_buffer]->Timecode().type));
	}
	
	if (fVideoImageArrayF2 != NULL)
	{
		fVideoImageArrayF2[fConfig.last_buffer]->SetStatus(fConfig.capture_status);
		fVideoImageArrayF2[fConfig.last_buffer]->SetTimestamp(fConfig.time_stamp - offset);
		fVideoImageArrayF2[fConfig.last_buffer]->SetFrameNumber(fConfig.frame_number);
		fVideoImageArrayF2[fConfig.last_buffer]->SetTimecode(FrameNumberToTimecode(fConfig.frame_number,
															fVideoImageArrayF2[fConfig.last_buffer]->Timecode().type));
	}
	
	*index = fConfig.last_buffer;
	
	return(fVideoImageArrayF1[fConfig.last_buffer]);
}

//------------------------------------------------------------------------------

status_t 
Bt848Source::WaitForFrame(bigtime_t timeout, uint32 *index)
{
	fConfig.timeout = timeout;
	if(ioctl(fBt848, BT848_SYNC_TIMEOUT, &fConfig) < 0) {
		return errno;
	}
	if (fConfig.capture_status == BT848_CAPTURE_TIMEOUT)
	{
		return B_TIMED_OUT;
	}
	*index = fConfig.last_buffer;
	return B_NO_ERROR;
}

BVideoImage *
Bt848Source::GetFrame(uint32 index)
{
	bigtime_t offset;

	bool oldimage = fOldVideoImage;

	fOlderVideoImage = fOldVideoImage = false;
	
	if(index != fConfig.last_buffer) {
		return NULL;
	}

	if ((fVideoFormat == B_NTSC_M) || (fVideoFormat == B_NTSC_J))
		offset = 33367;
	else
		offset = 40000;
		
	if (fConfig.capture_status  == BT848_CAPTURE_TIMEOUT)
	{
		TRACE("Capture timeout!  Status = %08x\n", fConfig.status);
		if ((fConfig.status & 0x00001000) != 0)
			TRACE("----PIXEL DATA FIFO OVERRUN\n");
		if ((fConfig.status & 0x00002000) != 0)
			TRACE("----TRAANSACTION TERMINATED, EXCESSIVE TARGET LATENCY\n");
		if ((fConfig.status & 0x00004000) != 0)
			TRACE("----FIFO DATA STREAM RESYNCHRONIZATION\n");
		if ((fConfig.status & 0x00008000) != 0)
			TRACE("----PCI PARITY ERROR\n");
		if ((fConfig.status & 0x00010000) != 0)
			TRACE("----FATAL PCI PARITY ERROR\n");
		if ((fConfig.status & 0x00020000) != 0)
			TRACE("----PIXEL PCI MASTER OR TARGET ABORT\n");
		if ((fConfig.status & 0x00040000) != 0)
			TRACE("----ILLEGAL RISC INSTRUCTION\n");
		if ((fConfig.status & 0x00080000) != 0)
			TRACE("----DMA EOL SYNC COUNTER OVERFLOW\n");
		return NULL;
	}

	BVideoImage *videoimageF1 = NULL;
	BVideoImage *videoimageF2 = NULL;
	if(oldimage) {
		if(fOldVideoImageArrayF1)
			videoimageF1 = fOldVideoImageArrayF1[fConfig.last_buffer];
		if(fOldVideoImageArrayF2)
			videoimageF2 = fOldVideoImageArrayF2[fConfig.last_buffer];
	}
	else {
		if(fVideoImageArrayF1)
			videoimageF1 = fVideoImageArrayF1[fConfig.last_buffer];
		if(fVideoImageArrayF2)
			videoimageF2 = fVideoImageArrayF2[fConfig.last_buffer];
	}

	if(videoimageF1) {
		videoimageF1->SetStatus(fConfig.capture_status);
		videoimageF1->SetTimestamp(fConfig.time_stamp - offset);
		videoimageF1->SetFrameNumber(fConfig.frame_number);
		videoimageF1->SetTimecode(FrameNumberToTimecode(fConfig.frame_number,
		                            videoimageF1->Timecode().type));
	}
	if(videoimageF2) {
		videoimageF2->SetStatus(fConfig.capture_status);
		videoimageF2->SetTimestamp(fConfig.time_stamp - offset);
		videoimageF2->SetFrameNumber(fConfig.frame_number);
		videoimageF2->SetTimecode(FrameNumberToTimecode(fConfig.frame_number,
		                            videoimageF2->Timecode().type));
	}
	return videoimageF1;
}


BVideoImage *
Bt848Source::NextFrame(uint32 *index)
{	/* wait till frame captured */
	/* CAUTION: will wait forever if there is a problem with capture hw */

	bigtime_t offset;

	ioctl(fBt848,BT848_SYNC,&fConfig);
	fOlderVideoImage = fOldVideoImage = false;
	
	if ((fVideoFormat == B_NTSC_M) || (fVideoFormat == B_NTSC_J))
		offset = 33367;
	else
		offset = 40000;
		
	if (fVideoImageArrayF1 != NULL)
	{
		fVideoImageArrayF1[fConfig.last_buffer]->SetStatus(fConfig.capture_status);
		fVideoImageArrayF1[fConfig.last_buffer]->SetTimestamp(fConfig.time_stamp - offset);
		fVideoImageArrayF1[fConfig.last_buffer]->SetFrameNumber(fConfig.frame_number);
		fVideoImageArrayF1[fConfig.last_buffer]->SetTimecode(FrameNumberToTimecode(fConfig.frame_number,
															fVideoImageArrayF1[fConfig.last_buffer]->Timecode().type));
	}
	
	if (fVideoImageArrayF2 != NULL)
	{
		fVideoImageArrayF2[fConfig.last_buffer]->SetStatus(fConfig.capture_status);
		fVideoImageArrayF2[fConfig.last_buffer]->SetTimestamp(fConfig.time_stamp - offset);
		fVideoImageArrayF2[fConfig.last_buffer]->SetFrameNumber(fConfig.frame_number);
		fVideoImageArrayF2[fConfig.last_buffer]->SetTimecode(FrameNumberToTimecode(fConfig.frame_number,
															fVideoImageArrayF2[fConfig.last_buffer]->Timecode().type));
	}

	*index = fConfig.last_buffer;	
	
	return(fVideoImageArrayF1[fConfig.last_buffer]);
}

//------------------------------------------------------------------------------

BVideoImage *
Bt848Source::NextFrameWithTimeout(bigtime_t timeout, uint32 *index)
{	/* wait till frame captured or timeout(in microseconds) expired */
	bigtime_t offset;

	fConfig.timeout = timeout;			
	ioctl(fBt848,BT848_SYNC_TIMEOUT,&fConfig);
	
	if ((fVideoFormat == B_NTSC_M) || (fVideoFormat == B_NTSC_J))
		offset = 33367;
	else
		offset = 40000;
		
	if (fConfig.capture_status  == BT848_CAPTURE_TIMEOUT)
	{
		TRACE("Capture timeout!  Status = %08x\n", fConfig.status);
		if ((fConfig.status & 0x00001000) != 0)
			TRACE("----PIXEL DATA FIFO OVERRUN\n");
		if ((fConfig.status & 0x00002000) != 0)
			TRACE("----TRAANSACTION TERMINATED, EXCESSIVE TARGET LATENCY\n");
		if ((fConfig.status & 0x00004000) != 0)
			TRACE("----FIFO DATA STREAM RESYNCHRONIZATION\n");
		if ((fConfig.status & 0x00008000) != 0)
			TRACE("----PCI PARITY ERROR\n");
		if ((fConfig.status & 0x00010000) != 0)
			TRACE("----FATAL PCI PARITY ERROR\n");
		if ((fConfig.status & 0x00020000) != 0)
			TRACE("----PIXEL PCI MASTER OR TARGET ABORT\n");
		if ((fConfig.status & 0x00040000) != 0)
			TRACE("----ILLEGAL RISC INSTRUCTION\n");
		if ((fConfig.status & 0x00080000) != 0)
			TRACE("----DMA EOL SYNC COUNTER OVERFLOW\n");
		return NULL;
	}
	fOlderVideoImage = fOldVideoImage = false;

	if (fVideoImageArrayF1 != NULL)
	{
		fVideoImageArrayF1[fConfig.last_buffer]->SetStatus(fConfig.capture_status);
		fVideoImageArrayF1[fConfig.last_buffer]->SetTimestamp(fConfig.time_stamp - offset);
		fVideoImageArrayF1[fConfig.last_buffer]->SetFrameNumber(fConfig.frame_number);
		fVideoImageArrayF1[fConfig.last_buffer]->SetTimecode(FrameNumberToTimecode(fConfig.frame_number,
															fVideoImageArrayF1[fConfig.last_buffer]->Timecode().type));
	}
	
	if (fVideoImageArrayF2 != NULL)
	{
		fVideoImageArrayF2[fConfig.last_buffer]->SetStatus(fConfig.capture_status);
		fVideoImageArrayF2[fConfig.last_buffer]->SetTimestamp(fConfig.time_stamp - offset);
		fVideoImageArrayF2[fConfig.last_buffer]->SetFrameNumber(fConfig.frame_number);
		fVideoImageArrayF2[fConfig.last_buffer]->SetTimecode(FrameNumberToTimecode(fConfig.frame_number,
															fVideoImageArrayF2[fConfig.last_buffer]->Timecode().type));
	}
	
	*index = fConfig.last_buffer;
	
	if(fVideoImageArrayF1 == NULL)
		return NULL;
	
	return(fVideoImageArrayF1[fConfig.last_buffer]);
}

//------------------------------------------------------------------------------

uint32
Bt848Source::Gpio()
{
	ioctl(fBt848, BT848_READ_GPIO_DATA, &fConfig);
	return fConfig.gpio_data;
}

//------------------------------------------------------------------------------

void
Bt848Source::SetGpio(uint32 data)
{
	fConfig.gpio_data = data;
	ioctl(fBt848, BT848_WRITE_GPIO_DATA, &fConfig);
}

//------------------------------------------------------------------------------

void
Bt848Source::GpioOutEnable(uint32 data)
{
		fConfig.gpio_out_en = data;
		ioctl(fBt848, BT848_WRITE_GPIO_OUT_EN, &fConfig);
}

//------------------------------------------------------------------------------

void
Bt848Source::GpioInEnable(uint32 data)
{
		fConfig.gpio_in_en = data;
		ioctl(fBt848, BT848_WRITE_GPIO_IN_EN, &fConfig);
}

//------------------------------------------------------------------------------

bool	
Bt848Source::Pll() const
{
	return(fConfig.pll);
}

//------------------------------------------------------------------------------

void	
Bt848Source::SetPll(const bool setting)
{
	fConfig.pll = setting;
	PLL("SetPll = %08x\n", fConfig.pll);
	ioctl(fBt848, BT848_PLL, &fConfig);
}

//------------------------------------------------------------------------------

uint32	
Bt848Source::DeviceID() const
{
	return(fConfig.device_id);
}

//------------------------------------------------------------------

bool
Bt848Source::VideoPresent()
{
	ioctl(fBt848,BT848_STATUS,fConfig);
	if((fConfig.status & (BT848_VIDEO_PRESENT | BT848_HLOCK)) == (BT848_VIDEO_PRESENT | BT848_HLOCK))
		return true;
	else
		return false;
}






