/*
	
	Bt848Source.h
	
	Copyright 1997 Be Incorporated, All Rights Reserved.
	
*/

#ifndef BT848_SOURCE_H
#define BT848_SOURCE_H

#include <GraphicsDefs.h>
#include <bt848_driver.h>

#include "VideoSource.h"
#include "VideoConversions.h"

typedef enum
{
	BT848HW_NONE = 0,
	BT848HW_ALPS,
	BT848HW_PANASONIC,
	BT848HW_PHILIPS,
	BT848HW_TEMIC,
	BT848HW_SAMSUNG
} bt848_tuner_mfg;

typedef struct
{
	uint32			model;
	uint32			version;
	uint32			ee_size;
	uint32			teletext;
	uint32			decoder;
	uint32			video_format;
	uint32			tuner_type;
	bt848_tuner_mfg	tuner_mfg;
	uint32			tuner_model;
	uint32			model_number;
	uint32			balun;
	uint32			fm_radio;
	uint32			ir;
	uint32			audio_decoder;
	uint32			serial_number;
} hw_info;

class Bt848Source : public BVideoSource
{
public:

/* the basics */
							Bt848Source(const char *name);
							~Bt848Source();
		status_t			InitCheck();

/* tuner brand */
		hw_info				Probe();
		void				SetTunerBrand(bt848_tuner_mfg brand);
		bt848_tuner_mfg		TunerBrand() const;

/* buffer management */
		status_t			AllocateCaptureBuffer(bt848_buffer *buf);
		void				FreeCaptureBuffer(bt848_buffer *buf);

/* audio/video muxes */
		BAudioMux *			AudioMux() const;
		BVideoMux *			VideoMux() const;
		BTuner *			Tuner() const;
		BVideoControls *	VideoControls() const;
		BI2CBus *			I2CBus() const;		

/* capture control & status */
		status_t			SetVideoFormat(video_format format);
		video_format		VideoFormat() const;
		void				SetCaptureMode(uint32 mode);
		uint32				CaptureMode() const;
		void				SetDecimation(uint32 decimation);
		uint32				Decimation() const;
		status_t			ConfigureCapture(BVideoImage **videoImageArrayF1,
											bt848_cliplist clipListF1 = NULL,
											uint32 count = 1,
											void **vbiBuffer = NULL,
											BVideoImage **videoImageArrayF2 = NULL,
											bt848_cliplist clipListF2 = NULL); 

		void				StartCapture(bool resetFrameCount = true);
		void				StopCapture();
		void				ContinueCapture();
		void				RestartCapture();
		status_t			SwitchCapture(uint32 *index = NULL);
		
		status_t			WaitForFrame(bigtime_t timeout, uint32 *index);
		BVideoImage *		GetFrame(uint32 index);
		BVideoImage *		LastFrame(uint32 *index = NULL);
		BVideoImage *		NextFrame(uint32 *index = NULL);
		BVideoImage *		NextFrameWithTimeout(bigtime_t timeout,uint32 *index = NULL);

		uint32				Gpio();
		void				SetGpio(uint32 data);
		void				GpioOutEnable(uint32 data);
		void				GpioInEnable(uint32 data);
		bool				Pll() const;
		void				SetPll(const bool setting);
		uint32				DeviceID() const;
		bool				VideoPresent();


private:
							Bt848Source(const Bt848Source &);
		Bt848Source			&operator=(const Bt848Source &);

		status_t			ConfigureSingleCapture(BVideoImage **videoImageArray,
									 		bt848_cliplist clipList = NULL,
											uint32 count = 1,
									 		void **vbiBuffer = NULL);
		status_t			ConfigureDualCapture(BVideoImage **videoImageArrayF1,
											bt848_cliplist clipListF1 = NULL,
											BVideoImage **videoImageArrayF2 = NULL,
											bt848_cliplist clipListF2 = NULL, 
											uint32 countF1 = 1,
											void **vbiBuffer = NULL);
		status_t			SetColorSpace(color_space space, video_field_specifier type = B_VIDEO_BOTH_FIELDS);
		color_space			ColorSpace(video_field_specifier type = B_VIDEO_F1_ONLY) const;
		void				SetAddressType(uint32 address_type, video_field_specifier type = B_VIDEO_BOTH_FIELDS);
		uint32				AddressType(video_field_specifier type = B_VIDEO_F1_ONLY) const;
		
		int32 				fBt848;
		bt848_config 		fConfig;
		hw_info				fHwInfo;	
		BAudioMux			*fAudioMux;
		BVideoMux			*fVideoMux;
		BTuner				*fTuner;
		BI2CBus				*fI2CBus;
		BVideoControls		*fVideoControls;
		video_format		fVideoFormat;
		BVideoImage			**fVideoImageArrayF1;
		BVideoImage			**fVideoImageArrayF2;
		BVideoImage			**fOldVideoImageArrayF1;
		BVideoImage			**fOldVideoImageArrayF2;
		bool				fOldVideoImage;
		bool				fOlderVideoImage;
		bool				fPrimaryRiscProgram;

};

#endif


