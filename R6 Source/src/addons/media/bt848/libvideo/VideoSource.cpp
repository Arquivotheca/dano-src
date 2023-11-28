/*
	
	VideoSource.cpp
	
	Copyright 1997-8 Be Incorporated, All Rights Reserved.
	
*/

#include "VideoSource.h"

//-----------------------------------------------------------------------

BVideoSource::BVideoSource(const char *name)
{
	strncpy(fName, name, 32);
	fAudioMux = new BAudioMux("Dummy Audio Mux");
	fVideoMux = new BVideoMux("Dummy Video Mux");
	fTuner = new BTuner("Dummy Tuner");
	fVideoControls = new BVideoControls("Dummy Video COntrols");
	fI2CBus = new BI2CBus("Dummy I2CBus");
}


//-----------------------------------------------------------------------

BVideoSource::~BVideoSource()
{

}

//-----------------------------------------------------------------------

status_t
BVideoSource::InitCheck()
{
	return B_ERROR;
}

//-----------------------------------------------------------------------

char *
BVideoSource::Name()
{
	return fName;
}

//-----------------------------------------------------------------------

BAudioMux *
BVideoSource::AudioMux() const
{
	return fAudioMux;
}

//-----------------------------------------------------------------------

BVideoMux *
BVideoSource::VideoMux() const
{
	return fVideoMux;
}

//-----------------------------------------------------------------------

BTuner *
BVideoSource::Tuner() const
{
	return fTuner;
}

//-----------------------------------------------------------------------

BVideoControls *
BVideoSource::VideoControls() const		
{
	return fVideoControls;

}

//-----------------------------------------------------------------------

BI2CBus *
BVideoSource::I2CBus() const		
{
	return fI2CBus;

}

//-----------------------------------------------------------------------

status_t
BVideoSource::SetVideoFormat(video_format format)
{
	fVideoFormat = format;
	return B_NO_ERROR;
}

//-----------------------------------------------------------------------

video_format
BVideoSource::VideoFormat() const
{
	return fVideoFormat;
}

//-----------------------------------------------------------------------

void
BVideoSource::SetCaptureMode(uint32 mode)
{
	fCaptureMode = mode;
}

//-----------------------------------------------------------------------

uint32
BVideoSource::CaptureMode() const
{
	return fCaptureMode;
}

//-----------------------------------------------------------------------

status_t
BVideoSource::ConfigureCapture(BVideoImage **videoImageArrayF1,
				bt848_cliplist ,
				uint32 ,
				void **,
				BVideoImage **videoImageArrayF2,
				bt848_cliplist )
{
	fVideoRingF1 = videoImageArrayF1;
	fVideoRingF2 = videoImageArrayF2;	
	return B_NO_ERROR;
}

//-----------------------------------------------------------------------

void
BVideoSource::StartCapture(bool /*resetFrameCount*/)
{

}

//-----------------------------------------------------------------------

void
BVideoSource::StopCapture()
{

}

//-----------------------------------------------------------------------

void
BVideoSource::RestartCapture()
{

}

//-----------------------------------------------------------------------

void
BVideoSource::ContinueCapture()
{

}

//-----------------------------------------------------------------------

status_t
BVideoSource::SwitchCapture()
{
	return B_ERROR;
}

//-----------------------------------------------------------------------

BVideoImage *
BVideoSource::LastFrame(uint32 *index)
{
	*index = 0;
	return fVideoRingF1[0];
}

//-----------------------------------------------------------------------

BVideoImage *
BVideoSource::NextFrame(uint32 *index)
{
	*index = 0;
	return fVideoRingF1[0];
}

//-----------------------------------------------------------------------

BVideoImage *
BVideoSource::NextFrameWithTimeout(bigtime_t , uint32 *index)
{
	*index = 0;
	return fVideoRingF1[0];
}

//-----------------------------------------------------------------------

uint32
BVideoSource::FramesDropped() const
{
	return 0;
}

//-----------------------------------------------------------------------

void
BVideoSource::_ReservedVideoSource1()
{

}

//-----------------------------------------------------------------------

void
BVideoSource::_ReservedVideoSource2()
{

}


//-----------------------------------------------------------------------

void
BVideoSource::_ReservedVideoSource3()
{

}

