/*
	
	VideoSource.h
	
	Copyright 1998 Be Incorporated, All Rights Reserved.
	
*/

#ifndef _VIDEO_SOURCE_H
#define _VIDEO_SOURCE_H

#include <bt848_driver.h>

#include "Tuner.h"
#include "I2CBus.h"
#include "AudioMux.h"
#include "VideoMux.h"
#include "ClipList.h"
#include "VideoImage.h"
#include "VideoControls.h"

//-----------------------------------------------------------------------

class BVideoSource
{

public:
							BVideoSource(const char *name);
virtual						~BVideoSource();
virtual	status_t			InitCheck();
virtual	char *				Name();

virtual	BAudioMux *			AudioMux() const;
virtual	BVideoMux *			VideoMux() const;
virtual	BTuner *			Tuner() const;
virtual	BVideoControls *	VideoControls() const;
virtual BI2CBus *			I2CBus() const;		

virtual	status_t			SetVideoFormat(video_format format);
virtual	video_format		VideoFormat() const;
		
virtual	void				SetCaptureMode(uint32 mode);
virtual	uint32				CaptureMode() const;
		
virtual	status_t			ConfigureCapture(BVideoImage **videoImageArrayF1,
											bt848_cliplist clipListF1 = NULL,
											uint32 count = 1,
											void **vbiBuffer = NULL,
											BVideoImage **videoImageArrayF2 = NULL,
											bt848_cliplist clipListF2 = NULL);
											 
virtual	void				StartCapture(bool resetFrameCount = true);
virtual	void				StopCapture();		
virtual	void				RestartCapture();
virtual	void				ContinueCapture();
virtual	status_t			SwitchCapture();
		
virtual	BVideoImage *		LastFrame(uint32 *index);
virtual	BVideoImage *		NextFrame(uint32 *index);
virtual	BVideoImage *		NextFrameWithTimeout(bigtime_t timeout, uint32 *index);
		
virtual	uint32				FramesDropped() const;


//-----------------------------------------------------------------------

private:

virtual	void				_ReservedVideoSource1();
virtual	void				_ReservedVideoSource2();
virtual	void				_ReservedVideoSource3();

							BVideoSource(const BVideoSource &);
		BVideoSource			&operator=(const BVideoSource &);
		
		char				fName[32];
		BVideoImage **		fVideoRingF1;
		BVideoImage **		fVideoRingF2;
		video_format		fVideoFormat;
		uint32				fCaptureMode;
		BAudioMux *			fAudioMux;
		BVideoMux *			fVideoMux;
		BTuner *			fTuner;
		BVideoControls *	fVideoControls;
		BI2CBus *			fI2CBus;		

		uint32				_reserved[3];
};

#endif
