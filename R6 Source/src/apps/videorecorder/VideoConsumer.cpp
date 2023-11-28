//	Copyright (c) 1998-99, Be Incorporated, All Rights Reserved.
//	SMS
//	VideoConsumer.cpp

#include <View.h>
#include <stdio.h>
#include <fcntl.h>
#include <Buffer.h>
#include <unistd.h>
#include <string.h>
#include <NodeInfo.h>
#include <scheduler.h>
#include <TimeSource.h>
#include <StringView.h>
#include <MediaRoster.h>
#include <Application.h>
#include <BufferGroup.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <Alert.h>

#include "VideoConsumer.h"

#define M1 ((double)1000000.0)
#define JITTER		20000

#define	FUNCTION(x)	//printf x
#define ERROR(x)	printf x
#define PROGRESS(x)	//printf x
#define LOOP(x)		//printf x

const media_raw_video_format vid_format = { 29.97,1,0,239,B_VIDEO_TOP_LEFT_RIGHT,1,1,{B_RGB16,320,240,320*4,0,0,0,{0,0,0}}};
//const media_raw_audio_format raw_audio_format = {
//	44100, 2, media_raw_audio_format::B_AUDIO_SHORT, 2, 65536 };
const media_multi_audio_format audio_format = media_multi_audio_format::wildcard;

//---------------------------------------------------------------

static
size_t get_bytes_per_row(color_space cs, uint32 width)
{
	size_t pixelChunk = 0;
	size_t rowAlignment = 0;
	size_t pixelsPerChunk = 0;
	size_t bytes_per_row;
	status_t err = get_pixel_size_for(cs, &pixelChunk,
		&rowAlignment, &pixelsPerChunk);
	if(err != B_NO_ERROR)
		return 0;
		
	bytes_per_row = (width * pixelChunk) / pixelsPerChunk;
	bytes_per_row +=  -bytes_per_row % rowAlignment;
	return bytes_per_row;
}		

//---------------------------------------------------------------

VideoConsumer::VideoConsumer(
	const char * name,
	BView *view,
	BStringView	* statusLine,
	BMediaAddOn *addon,
	const uint32 internal_id) :
	
	BMediaNode(name),
	BMediaEventLooper(),
//	BBufferConsumer(B_MEDIA_RAW_VIDEO),
	BBufferConsumer(B_MEDIA_UNKNOWN_TYPE),	// audio and video

	mStatusLine(statusLine),
	mInternalID(internal_id),
	mAddOn(addon),

	mConnectionActive(false),
	mMyLatency(20000),
	mVidRate(29.97),

	mAudioBuffer(NULL),
//	mAudioBuffers(NULL),	4.5 audio node is broken

	mWindow(NULL),
	mView(view),
	mOurBuffers(false),
	mBuffers(NULL),
	mPreviewBitmap(NULL),

	mReceiveCount(0),

	mMediaFile(NULL),
	mVideoTrack(NULL),
	mAudioTrack(NULL),
	mWriting(false)
	
{
	FUNCTION(("VideoConsumer::VideoConsumer\n"));

	memset(&mCaptureConfig, 0, sizeof(mCaptureConfig));

	AddNodeKind(B_PHYSICAL_OUTPUT);
	SetEventLatency(0);
	mWindow = mView->Window();
	
	for (uint32 j = 0; j < 3; j++)
	{
		mBitmap[j] = NULL;
		mBits[j] = NULL;
		mBufferMap[j] = 0;
	}

	//strcpy(mFileNameText, "");
	//strcpy(mFileFormatName, "");
	//strcpy(mEncoderName, "");
	
	SetPriority(B_DISPLAY_PRIORITY);
}

//---------------------------------------------------------------

VideoConsumer::~VideoConsumer()
{
	FUNCTION(("VideoConsumer::~VideoConsumer\n"));

	UninitMediaFile();
	
	//if(mAudioBuffers) { 4.5 audio node is broken
	//	printf("AudioBuffers not still around\n");
	//	delete mAudioBuffers;
	//	mAudioBuffers = NULL;
	//}
	Quit();

	if (mWindow)
	{
		//printf("Locking the window\n");
		if (mWindow->Lock())
		{
			//printf("Closing the window\n");
			mWindow->Close();
			mWindow = 0;
		}
	}

	DeleteBuffers();
}

status_t 
VideoConsumer::InitMediaFile()
{
	media_format			format, outfmt;
	//media_codec_info    	ei;
	//media_file_format		mfi;
	int32					cookie;
	//entry_ref       		ref;
	status_t err;

	if(mWriting) {
		UpdateStatus("Already recording");
		return B_ERROR;
	}

	mWriteStartTime = 0;

	mFile = new BFile(mCaptureConfig.filename, B_FAIL_IF_EXISTS | B_CREATE_FILE | B_READ_WRITE);

	err = mFile->InitCheck();

	if(err == B_FILE_EXISTS) {
		char alertstring[128];
		sprintf(alertstring, "The mFile \"%s\" already exists. Do you want to replace it?\n",
		        mCaptureConfig.filename);
		if((new BAlert("Warning", alertstring, "Cancel", "Replace", NULL,
			B_WIDTH_AS_USUAL,B_WARNING_ALERT))->Go() != 1) {
			return B_ERROR;
		}
		err = mFile->SetTo(mCaptureConfig.filename, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	}

	if(err != B_NO_ERROR) {
		UpdateStatus("Failed create output mFile");
		return err;
	}

	mMediaFile = new BMediaFile(mFile, &mCaptureConfig.fileformat);
	err = mMediaFile->InitCheck();
	if (err != B_OK) {
		printf("failed to properly init the output mFile... (%s)\n", strerror(err));
		UpdateStatus("Failed to init output mFile");
		delete mMediaFile;
		mMediaFile = NULL;
		return err;
	}

	format = mVideoIn.format;
	media_raw_video_format *rvf = &format.u.raw_video;

	printf("record at %ld*%ld, space %x\n", rvf->display.line_width,
	       rvf->display.line_count, rvf->display.format);
	//printf("record at %d*%d, space %x\n", mWidth, mHeight, mColorSpace);

	if(mVideoIn.format.type == B_MEDIA_ENCODED_VIDEO) {
		format.u.encoded_video.output.field_rate /= mCaptureConfig.interval;
		mVideoTrack = mMediaFile->CreateTrack(&format);
	}
	else {
		format.u.raw_video.field_rate /= mCaptureConfig.interval;
		mVideoTrack = mMediaFile->CreateTrack(&format, &mCaptureConfig.videoencoder);
	}
	
	if(!mVideoTrack){
		UpdateStatus("Failed to create video track");
		delete mMediaFile;
		mMediaFile = NULL;
		return B_ERROR;
	}
	
	if(mAudioBuffer != NULL) {
		printf("Recording audio\n");
		media_format			audiooutfmt;
		media_codec_info    	aei;

		cookie = 0;
		while (get_next_encoder(&cookie, &mCaptureConfig.fileformat, &mAudioIn.format, &audiooutfmt, &aei) == B_OK) {
			printf("found audio encoder %s (%ld)\n", aei.pretty_name, aei.id);
			//const char *encodername = "raw-video";
			//const char *encodername = "iv50";
			//const char *encodername = "iv50rt";
			if (strcmp(aei.short_name, mCaptureConfig.audioencoder.short_name) == 0) {
				printf("using %s\n", aei.short_name);
				break;
			}
		}

		mAudioTrack = mMediaFile->CreateTrack(&mAudioIn.format, &aei);
		
		if(!mAudioTrack){
			UpdateStatus("Failed to create audio track");
			delete mMediaFile;
			mMediaFile = NULL;
			return B_ERROR;
		}
	}
	
	if(mCaptureConfig.quality >= 0) {
		float quality;
		if(mVideoTrack->GetQuality(&quality) == B_NO_ERROR) {
			printf("Quality was %.2f\n", quality);
		}
		err = mVideoTrack->SetQuality(mCaptureConfig.quality);
		if(err != B_NO_ERROR) {
			printf("SetQuality failed\n");
		}
		if(mVideoTrack->GetQuality(&quality) == B_NO_ERROR) {
			printf("Quality is %.2f\n", quality);
		}
	}

	// Add the copyright and commit the header
	//mMediaFile->AddCopyright("Copyright 1999 Be Incorporated");
	err = mMediaFile->CommitHeader();
	if(err != B_NO_ERROR) {
		delete mMediaFile;
		mMediaFile = NULL;
		UpdateStatus("Could not write mFile header");
		return err;
	}
	mAudioFramesWritten = mFramesWritten = mFramesDropped = mReceiveCount = 0;

	BNodeInfo ninfo(mFile); 
	ninfo.SetType(mCaptureConfig.fileformat.mime_type); 

	UpdateStatus("Capture started");
	mWriting = true;
	return B_NO_ERROR;
}

status_t 
VideoConsumer::UninitMediaFile()
{
	mWriting = false;
	if(mMediaFile == NULL)
		return B_ERROR;
	if(mAudioTrack) {
		printf("Audio frames dropped %d, repeated %d\n", mAudioFramesDropped,
		       mAudioFramesRepeated);
		mAudioTrack = NULL;
	}
	mMediaFile->CloseFile();
	delete mMediaFile;
	mMediaFile = NULL;
	delete mFile;
	printf("mMediaFile done\n");

	char status[256];
	sprintf(status, "Capture completed: frames written %d, dropped %d",
	        mFramesWritten, mFramesDropped);
	UpdateStatus(status);

	return B_NO_ERROR;
}

void 
VideoConsumer::Preroll()
{
	printf("VideoConsumer::Preroll()\n");
	PrerollStatus = InitMediaFile();
}


/********************************
	From BMediaNode
********************************/

//---------------------------------------------------------------

BMediaAddOn *
VideoConsumer::AddOn(long *cookie) const
{
	FUNCTION(("VideoConsumer::AddOn\n"));
	// do the right thing if we're ever used with an add-on
	*cookie = mInternalID;
	return mAddOn;
}

//---------------------------------------------------------------

void
VideoConsumer::NodeRegistered()
{
	FUNCTION(("VideoConsumer::NodeRegistered\n"));
	mVideoIn.destination.port = ControlPort();
	mVideoIn.destination.id = 0;
	mVideoIn.source = media_source::null;
	mVideoIn.format.type = B_MEDIA_RAW_VIDEO;
	mVideoIn.format.u.raw_video = vid_format;

	mAudioIn.destination.port = ControlPort();
	mAudioIn.destination.id = 1;
	mAudioIn.source = media_source::null;
	mAudioIn.format.type = B_MEDIA_RAW_AUDIO;
	mAudioIn.format.u.raw_audio = audio_format;

	Run();
}

//---------------------------------------------------------------

status_t
VideoConsumer::RequestCompleted(const media_request_info & info)
{
	FUNCTION(("VideoConsumer::RequestCompleted\n"));
	switch(info.what)
	{
		case media_request_info::B_SET_OUTPUT_BUFFERS_FOR:
			{
				if (info.status != B_OK)
					ERROR(("VideoConsumer::RequestCompleted: Not using our buffers!\n"));
			}
			break;
		default:
printf("info.what is %u, error is %lx\n", info.what, info.status);
			;
	}
	return B_OK;
}

//---------------------------------------------------------------

status_t
VideoConsumer::HandleMessage(int32 message, const void * data, size_t /*size*/)
{
	//FUNCTION(("VideoConsumer::HandleMessage\n"));
	writer_msg_info *info = (writer_msg_info *)data;
	status_t status = B_OK;
		
	switch (message)
	{
		case WRITER_INFO: {
			float old_quality = mCaptureConfig.quality;
			PROGRESS(("VideoConsumer::HandleMessage - WRITER_INFO message\n"));
			mCaptureConfig = info->config;
			if(mWriting && old_quality != mCaptureConfig.quality)
				mVideoTrack->SetQuality(mCaptureConfig.quality);
			}
			break;

		case START_WRITE:
			printf("START_WRITE obsolete\n");
			break;

		case STOP_WRITE:
			printf("STOP_WRITE obsolete\n");
			break;
	}
			
	return status;
}

//---------------------------------------------------------------
status_t 
VideoConsumer::ConsumeAudioBuffer(BBuffer *buffer)
{
	if(!mAudioTrack)
		return B_NO_ERROR;

	int framesize = mAudioIn.format.u.raw_audio.format & 0xf;
	framesize *= mAudioIn.format.u.raw_audio.channel_count;

	float video_frame_time = (float)mCaptureConfig.interval / mVidRate;
	int audio_frames_per_video_frame = (int)(
		mAudioIn.format.u.raw_audio.frame_rate * video_frame_time);
		
	if(audio_frames_per_video_frame*framesize > mAudioBufferSize) {
		printf("mAudioBuffer too small %d < %d\n",
		mAudioBufferSize, audio_frames_per_video_frame*framesize);
		return B_ERROR;
	}
	
	if(mWriteStartTime == 0) {
		int audioprerollsize = audio_frames_per_video_frame*framesize;
		uint8 *srcbuf = (uint8*)buffer->Data();
		int srcsize = (int)buffer->SizeUsed();

		int audio_jitter_bytes =
			(int)(mCaptureConfig.audio_jitter *
			      mAudioIn.format.u.raw_audio.frame_rate) *
			framesize;
		
		printf("audio_jitter_bytes = %d\n", audio_jitter_bytes);
		audioprerollsize -= audio_jitter_bytes / 2;
		if(audioprerollsize < 0)
			audioprerollsize = 0;
		
		if(srcsize > audioprerollsize) {
			//printf("fill audio preroll buffer\n");
			memcpy(mAudioBuffer, srcbuf+srcsize-audioprerollsize, audioprerollsize);
			mAudioBufferUsed = audioprerollsize;
		}
		else if(mAudioBufferUsed + srcsize > audioprerollsize) {
			int skip = mAudioBufferUsed + srcsize - audioprerollsize;
			for(int i=0; i<mAudioBufferUsed-skip; i++) {
				mAudioBuffer[i] = mAudioBuffer[i+skip];
			}
			memcpy(mAudioBuffer+mAudioBufferUsed-skip, srcbuf, srcsize);
			mAudioBufferUsed += srcsize-skip;
			//printf("add %d bytes to audio preroll buffer (%d bytes removed)\n", srcsize-skip, skip);
		}
		else {
			//printf("add %d bytes to audio preroll buffer\n", srcsize);
			memcpy(mAudioBuffer+mAudioBufferUsed, srcbuf, srcsize);
			mAudioBufferUsed += srcsize;
		}
		//printf("audio preroll buffer is %d bytes of %d\n", mAudioBufferUsed, audioprerollsize);
		//printf("skip early audio buffer, %Ld\n", buffer->Header()->start_time);
		if((int)buffer->SizeUsed() > mAudioBufferSize) {
			printf("audio buffer received is too big (%d>%d)\n",(int)buffer->SizeUsed(), mAudioBufferSize);
			return B_ERROR;
		}
		mAudioFramesWritten = mAudioBufferUsed/framesize;
		return B_NO_ERROR;
	}

	//printf("got audio buffer, id=%d\n", buffer->ID());
	//printf("audiotime %Ld\n", buffer->Header()->start_time);
	
	float audio_time = (float)mAudioFramesWritten / mAudioIn.format.u.raw_audio.frame_rate;

	//float audio_time = (float)mAudioFramesWritten / 44800.0; // test

	float video_time = (float)mFramesWritten * video_frame_time;
	
	int srcsize = (int)buffer->SizeUsed();
	
	if(audio_time > video_time + video_frame_time) {
		printf("audio ahead\n");
		mAudioExpandCount = 0;
		int dropcount = (int)((audio_time-video_time-video_frame_time)*mAudioIn.format.u.raw_audio.frame_rate);
		if(dropcount * framesize > srcsize) {
			printf("drop buffer\n");
			mAudioFramesDropped += srcsize / framesize;
			return B_NO_ERROR;
		}
		printf("drop %d samples\n", dropcount);
		mAudioFramesDropped += dropcount;
		srcsize -= dropcount * framesize;
	}

	if(audio_time < video_time - mCaptureConfig.audio_jitter /*- video_frame_time*/ || mAudioExpandCount > 0) {
		int expandcount = (int)((video_time-mCaptureConfig.audio_jitter-audio_time)*mAudioIn.format.u.raw_audio.frame_rate);
		if(expandcount < 0)
			expandcount = 0;
		//printf("audio %.6f s %d+%d frames late\n",
		//	video_time-audio_time, expandcount, mAudioExpandCount);
		if(audio_time < video_time - 2 * video_frame_time) {
			printf("audio 2 frames late\n");
			//expandcount += mAudioExpandCount;
			mAudioExpandCount = 0;
		}
		else if(audio_time < video_time - video_frame_time) {
			//printf("audio late\n");
			printf("audio %.6f s %d+%d frames late\n",
				video_time-audio_time, expandcount, mAudioExpandCount);
			//expandcount += mAudioExpandCount;
			mAudioExpandCount = 0;
		}
		else {
			if(expandcount > mAudioExpandCount || expandcount > audio_frames_per_video_frame)
				mAudioExpandCount = expandcount;
			if(mAudioExpandCount > 0) {
				expandcount = 1 + mAudioExpandCount * 2 / audio_frames_per_video_frame;
			}
			mAudioExpandCount -= expandcount;
			//printf("expandcount = %d\n", expandcount);
		}

		mAudioFramesRepeated += expandcount;
		
		if(mAudioBufferUsed + framesize*expandcount < mAudioBufferSize) {
			if(framesize*expandcount <= srcsize) {
				memcpy(mAudioBuffer+mAudioBufferUsed, buffer->Data(), framesize*expandcount);
			}
			else {
				printf("audio to late to repeat\n");
				memset(mAudioBuffer+mAudioBufferUsed, 0, framesize*expandcount);
			}
			mAudioBufferUsed += framesize*expandcount;
			mAudioFramesWritten += expandcount;
		}
		else {
			while(mAudioBufferUsed + framesize*expandcount >= mAudioBufferSize) {
				int audiobufferfree = mAudioBufferSize-mAudioBufferUsed;
				memset(mAudioBuffer+mAudioBufferUsed, 0, audiobufferfree);
				if(mAudioTrack->WriteFrames(mAudioBuffer, mAudioBufferSize/framesize, (int32)0) < B_NO_ERROR) {
					UpdateStatus("Audio Write failed");
					return B_ERROR;
				}
				mAudioFramesWritten += audiobufferfree / framesize;
				expandcount -= audiobufferfree / framesize;
				mAudioBufferUsed = 0;
			}
			memset(mAudioBuffer, 0, framesize*expandcount);
			mAudioBufferUsed = framesize*expandcount;
			mAudioFramesWritten += expandcount;
		}
	}
	else {
		//printf("audio %.6f s ahead\n", audio_time-video_time);
	}

	int audiobufferfree = mAudioBufferSize-mAudioBufferUsed;
	if(srcsize < audiobufferfree) {
		memcpy(mAudioBuffer+mAudioBufferUsed, buffer->Data(), srcsize);
		mAudioBufferUsed += srcsize;
		mAudioFramesWritten += srcsize/framesize;
	}
	else {
		uint8 *newdata = (uint8*)buffer->Data();
		memcpy(mAudioBuffer+mAudioBufferUsed, newdata, audiobufferfree);
		mAudioFramesWritten += audiobufferfree/framesize;

		newdata += audiobufferfree;
		//printf("write audio\n");
		if(mAudioTrack->WriteFrames(mAudioBuffer, mAudioBufferSize/framesize, (int32)0) < B_NO_ERROR) {
			UpdateStatus("Audio Write failed");
		}
		mAudioBufferUsed = 0;
		if(srcsize > audiobufferfree) {
			memcpy(mAudioBuffer, newdata, srcsize-audiobufferfree);
			mAudioBufferUsed = srcsize-audiobufferfree;
			mAudioFramesWritten += mAudioBufferUsed/framesize;
		}
	}
	return B_NO_ERROR;
}


void
VideoConsumer::BufferReceived(BBuffer * buffer)
{
	LOOP(("VideoConsumer::Buffer #%d received\n", buffer->ID()));
	bigtime_t receivetime = system_time();
	if(buffer->Type() == B_MEDIA_RAW_AUDIO) {
		static bigtime_t last_audio_ptime = 0;
		static bigtime_t last_audio_rtime = 0;
		if(0 && receivetime-last_audio_rtime > 20000) {
		printf("audio buffer: perfromance time %Ld +%Ld, system time %Ld +%Ld\n",
		       buffer->Header()->start_time,
		       buffer->Header()->start_time - last_audio_ptime,
		       receivetime, receivetime-last_audio_rtime);
		}
		last_audio_rtime = receivetime;
		last_audio_ptime = buffer->Header()->start_time;

		//printf("got audio buffer\n");
		ConsumeAudioBuffer(buffer);
		buffer->Recycle();
		return;
	}
	static bigtime_t last_video_rtime = 0;
	static bigtime_t last_video_ptime = 0;
	if(0 && receivetime-last_video_rtime > 40000) {
	printf("video buffer: perfromance time %Ld +%Ld, system time %Ld +%Ld\n",
	       buffer->Header()->start_time,
	       buffer->Header()->start_time-last_video_ptime,
	       receivetime, receivetime-last_video_rtime);
	}
	last_video_rtime = receivetime;
	last_video_ptime = buffer->Header()->start_time;

	//if (RunState() == B_STOPPED)
	//{
	//	buffer->Recycle();
	//	return;
	//}
	if(mReceiveCount++ % mCaptureConfig.interval != 0) {
		buffer->Recycle();
		return;
	}

	//printf("videotime %Ld\n", buffer->Header()->start_time);

	if(RunState() == B_STARTED) {
		if(!mWriting) {
			printf("node started without writer\n");
		}
		else if(TimeSource()->Now() > (buffer->Header()->start_time + JITTER*2)) {
			//printf("dropped write frame %Ld >> buffer->Header()->start_time %Ld\n",
			//TimeSource()->Now(), buffer->Header()->start_time);
			if(mFramesWritten)
				mFramesDropped++;
		}
		else {
			if(mFramesWritten == 0) {
				mWriteStartTime = buffer->Header()->start_time;
			}
			if(buffer->Header()->u.encoded_video.field_flags & B_MEDIA_KEY_FRAME)
				mVideoEncodeInfo.flags = B_MEDIA_KEY_FRAME;
			else
				mVideoEncodeInfo.flags = 0;
			mVideoEncodeInfo.start_time = (bigtime_t)((mFramesWritten+mFramesDropped) *
				1000000 * mCaptureConfig.interval / mVidRate);
			mVideoEncodeInfo.start_time = buffer->Header()->start_time - mWriteStartTime;
			mVideoEncodeInfo.time_to_encode =
				buffer->Header()->start_time - TimeSource()->Now();
			if(mVideoEncodeInfo.time_to_encode < 0) {
				mVideoEncodeInfo.time_to_encode = 0;
			}
			status_t write_status;
			if(mVideoIn.format.type == B_MEDIA_ENCODED_VIDEO)
				write_status = mVideoTrack->WriteChunk(buffer->Data(),
				                                       buffer->SizeUsed(),
				                                       &mVideoEncodeInfo);
			else
				write_status = mVideoTrack->WriteFrames(buffer->Data(), 1,
				                                        &mVideoEncodeInfo);
			if(write_status < B_NO_ERROR) {
				mFramesDropped++;
				UpdateStatus("Write failed");
			}
			else {
				mFramesWritten++;
			}
		}
		if((mFramesWritten+mFramesDropped) % (30/mCaptureConfig.interval) == 0) {
			char status[256];
			off_t bytes;
			double filesize;
			char *filesizescale = "";
			
			if(mFile->GetSize(&bytes) != B_NO_ERROR)
				bytes = 0;
			filesize = bytes;
			
			if(filesize > 0) {
				filesizescale = "Bytes";
			}
			if(filesize > 1024) {
				filesize /= 1024;
				filesizescale = "KB";
			}
			if(filesize > 1024) {
				filesize /= 1024;
				filesizescale = "MB";
			}
			if(filesize > 1024) {
				filesize /= 1024;
				filesizescale = "GB";
			}
			sprintf(status,
			        "Capturing: frames written %d, dropped %d, filesize %.2f %s",
			        mFramesWritten, mFramesDropped, filesize, filesizescale);
			UpdateStatus(status);
		}
	}

	media_timed_event event(buffer->Header()->start_time, BTimedEventQueue::B_HANDLE_BUFFER,
						buffer, BTimedEventQueue::B_RECYCLE_BUFFER);
	EventQueue()->AddEvent(event);
}


//---------------------------------------------------------------

void
VideoConsumer::ProducerDataStatus(
	const media_destination &for_whom,
	int32 /*status*/,
	bigtime_t /*at_media_time*/)
{
	FUNCTION(("VideoConsumer::ProducerDataStatus\n"));

	if (for_whom == mVideoIn.destination) {
	}
	else if(for_whom == mVideoIn.destination) {
	}
}

//---------------------------------------------------------------

status_t
VideoConsumer::CreateBuffers(
	const media_format & with_format)
{
	FUNCTION(("VideoConsumer::CreateBuffers\n"));
	
	BBuffer ** buffList = NULL;

	// delete any old buffers
	DeleteBuffers();	

	status_t status = B_OK;

	if(with_format.type != B_MEDIA_RAW_VIDEO)
		return B_MEDIA_BAD_FORMAT;

	// create a buffer group
	uint32 mXSize = with_format.u.raw_video.display.line_width;
	uint32 mYSize = with_format.u.raw_video.display.line_count;	
	uint32 mRowBytes = with_format.u.raw_video.display.bytes_per_row;
	color_space mColorspace = with_format.u.raw_video.display.format;
	PROGRESS(("VideoConsumer::CreateBuffers - Colorspace = %d\n", mColorspace));

	mBuffers = new BBufferGroup();
	status = mBuffers->InitCheck();
	if (B_OK != status)
	{
		ERROR(("VideoConsumer::CreateBuffers - ERROR CREATING BUFFER GROUP\n"));
		goto err;
	}
	buffList = new BBuffer * [3];
	for (int j = 0; j < 3; j++) buffList[j] = 0;
	
	// and attach the  bitmaps to the buffer group
	for (uint32 j=0; j < 3; j++)
	{
		mBitmap[j] = new BBitmap(BRect(0, 0, (mXSize-1), (mYSize - 1)),
		                         B_BITMAP_IS_LOCKED, mColorspace);
		//mBitmap[j] = new BBitmap(BRect(0, 0, (mXSize-1), (mYSize - 1)),
		//                         B_BITMAP_IS_CONTIGUOUS, mColorspace);
		if(mBitmap[j] == NULL) {
			status = B_NO_MEMORY;
			goto err;
		}
		if (mBitmap[j]->IsValid())
		{
			if((uint32)mBitmap[j]->BitsLength() != mYSize * mRowBytes) {
				ERROR(("VideoConsumer::CreateBuffers - bad bytes_per_row\n"));
				status = B_ERROR;
				goto err;
			}
			mBits[j] = mBitmap[j]->Bits();
			//printf("bits: %p\n", mBits[j]);
		}
		else {
			delete mBitmap[j];
			mBitmap[j] = NULL;
			ERROR(("VideoConsumer::CreateBuffers - bad bitmap\n"));
			status = B_ERROR;
			goto err;
		}
		if(mBits[j]) {
			buffer_clone_info info;
			area_info ainfo;
			if ((info.area = area_for(mBits[j])) < B_NO_ERROR)
				ERROR(("VideoConsumer::CreateBuffers - ERROR IN AREA_FOR\n"));;
			if(get_area_info(info.area, &ainfo) != B_NO_ERROR) {
				ERROR(("VideoConsumer::CreateBuffers - ERROR IN GET_AREA_INFO\n"));;
			}
			//printf("area address %p, bitmap address %p\n", ainfo.address, mBits[j]);
			info.offset = (uint32)mBits[j] - (uint32)ainfo.address;
			//info.offset = (uint32)mBits[j] & (B_PAGE_SIZE-1);
			info.size = (size_t)(mYSize * mRowBytes);
			info.flags = j;
			info.buffer = 0;

			if ((status = mBuffers->AddBuffer(info)) != B_OK)
			{
				ERROR(("VideoConsumer::CreateBuffers - ERROR ADDING BUFFER TO GROUP\n"));
				goto err;
			} else PROGRESS(("VideoConsumer::CreateBuffers - SUCCESSFUL ADD BUFFER TO GROUP\n"));
		}
		else 
		{
			ERROR(("VideoConsumer::CreateBuffers - ERROR CREATING VIDEO RING BUFFER: %08lx\n", status));
			status = B_ERROR;
			goto err;
		}	
	}
	
	if ((status = mBuffers->GetBufferList(3, buffList)) == B_OK)					
		for (int j = 0; j < 3; j++)
			if (buffList[j] != NULL)
			{
				mBufferMap[j] = (uint32) buffList[j];
				PROGRESS((" j = %d buffer = %08x\n", j, mBufferMap[j]));
			}
			else
			{
				ERROR(("VideoConsumer::CreateBuffers ERROR MAPPING RING BUFFER\n"));
				status = B_ERROR;
				goto err;
			}
	else
		ERROR(("VideoConsumer::CreateBuffers ERROR IN GET BUFFER LIST\n"));
		
	FUNCTION(("VideoConsumer::CreateBuffers - EXIT\n"));
	return status;

err:
	if(buffList == NULL) {
		for(int j = 0; j < 3; j++)
			mBufferMap[j] = 0;
		delete [] buffList;
	}
	delete mBuffers;
	mBuffers = NULL;
	
	for (uint32 j = 0; j < 3; j++) {
		if(mBits[j] && !mBitmap[j]) {
			free(mBits);
		}
		else {
			delete mBitmap[j];
			mBitmap[j] = NULL;
		}
		mBits[j] = NULL;
	}
	printf("CreateBuffers return error %lx\n", status);
	return status;
}

//---------------------------------------------------------------

void
VideoConsumer::DeleteBuffers()
{
	FUNCTION(("VideoConsumer::DeleteBuffers\n"));
	
	if (mBuffers)
	{
		delete mBuffers;
		mBuffers = NULL;
		
		for (uint32 j = 0; j < 3; j++) {
			if(mBits[j] && !mBitmap[j]) {
				free(mBits);
			}
			else {
				delete mBitmap[j];
				mBitmap[j] = NULL;
			}
			mBits[j] = NULL;
		}
	}
	FUNCTION(("VideoConsumer::DeleteBuffers - EXIT\n"));
}

//---------------------------------------------------------------

status_t
VideoConsumer::Connected(
	const media_source & producer,
	const media_destination & where,
	const media_format & with_format,
	media_input * out_input)
{
	FUNCTION(("VideoConsumer::Connected\n"));
	
	if(where == mAudioIn.destination) {
		FUNCTION(("VideoConsumer::Connected Audio\n"));

		mAudioIn.source = producer;
		mAudioIn.format = with_format;
		mAudioIn.node = Node();
		sprintf(mAudioIn.name, "Audio Consumer");
		*out_input = mAudioIn;

#if 0
// 4.5 audio node is broken
		status_t err;
		
		mAudioBuffers = new BBufferGroup(4096, 16);
		err = mAudioBuffers->InitCheck();
		if(err != B_NO_ERROR) {
			delete mAudioBuffers;
			mAudioBuffers = NULL;
			return err;
		}

		int32 dummy_change_tag;
		err = BBufferConsumer::SetOutputBuffersFor(producer, where,
		                                           mAudioBuffers, (void*)NULL,
		                                           &dummy_change_tag,
		                                           true /* 4.5 media kit does not work with false */ );
		if(err != B_NO_ERROR) {
			delete mAudioBuffers;
			mAudioBuffers = NULL;
			return err;
		}
#endif
#if 0
BBuffer * bufs[16];
int32 cnt = 0;
mAudioBuffers->CountBuffers(&cnt);
if (cnt > 16) cnt = 16;
mAudioBuffers->GetBufferList(16, bufs);
for (int ix=0; ix<cnt; ix++) {
	printf("buffer at %d: %p id %d\n", ix, bufs[ix], bufs[ix]->ID());
}
#endif
		if(strcmp(mCaptureConfig.videoencoder.short_name, "raw-video") == 0) {
			mAudioBufferSize = 65536;
		}
		else {
			int audioframesize = mAudioIn.format.u.raw_audio.format & 0xf;
			audioframesize *= mAudioIn.format.u.raw_audio.channel_count;
		
			float video_frame_time = (float)mCaptureConfig.interval / 29.97;
			int audio_frames_per_video_frame = (int)(
				mAudioIn.format.u.raw_audio.frame_rate * video_frame_time);
				
			mAudioBufferSize = audio_frames_per_video_frame*audioframesize;
			if (mAudioIn.format.u.raw_audio.buffer_size > mAudioBufferSize)
				mAudioBufferSize = mAudioIn.format.u.raw_audio.buffer_size;
		}
//		printf("using %d byte audio buffers (%d)\n", mAudioBufferSize,mAudioIn.format.u.raw_audio.buffer_size);
//		mAudioBufferSize = 16*1024;
		mAudioBuffer = (uint8*)malloc(mAudioBufferSize);
		mAudioBufferUsed = 0;
		mAudioExpandCount = 0;
		mAudioFramesRepeated = 0;
		mAudioFramesDropped = 0;

		return B_NO_ERROR;
	}
	
	if(where != mVideoIn.destination) {
		return B_MEDIA_BAD_DESTINATION;
	}
	mVideoIn.source = producer;
	mVideoIn.format = with_format;
	mVidRate = mVideoIn.format.u.raw_video.field_rate;
	mVideoIn.node = Node();
	sprintf(mVideoIn.name, "Video Consumer");
	*out_input = mVideoIn;

	uint32 user_data = 0;
	int32 change_tag = 1;	

	if(mVideoIn.format.type == B_MEDIA_ENCODED_VIDEO) {
		DeleteBuffers();
		ConfigureDecoder();
		mConnectionActive = true;
		return B_NO_ERROR;
	}

	uint32 supportFlags = 0;
	bool colorSpaceSupported = bitmaps_support_space(with_format.u.raw_video.display.format, &supportFlags);
	if (!colorSpaceSupported || !(supportFlags & B_VIEWS_SUPPORT_DRAW_BITMAP) )
	{
		colorSpaceSupported = false;
	}

	if (colorSpaceSupported && CreateBuffers(with_format) == B_OK)
		BBufferConsumer::SetOutputBuffersFor(producer, mDestination, 
											mBuffers, (void *)&user_data, &change_tag, true);
	else
	{
		DeleteBuffers();
		//mPreviewBitmap = new BBitmap(
		//	BRect(0, 0, (mWidth-1), (mHeight - 1)), B_RGB32, false, true);
		media_video_display_info *display;
		display = &mVideoIn.format.u.raw_video.display;
		BRect bitmapframe(0, 0, display->line_width-1, display->line_count-1);
		mPreviewBitmap = new BBitmap(bitmapframe, B_RGB32, false, false);

		//printf("PreviewBitmap: %f %f\n", bitmapframe.Width(), bitmapframe.Height());

		if(!mPreviewBitmap->IsValid()) {
			ERROR(("VideoConsumer::Connected - COULDN'T create preview bitmap\n"));
			delete mPreviewBitmap;
			mPreviewBitmap = NULL;
		}
		//return B_ERROR;
	}

	mConnectionActive = true;
		
	FUNCTION(("VideoConsumer::Connected - EXIT\n"));
	return B_OK;
}

//---------------------------------------------------------------

void
VideoConsumer::Disconnected(
	const media_source & producer,
	const media_destination & where)
{
	FUNCTION(("VideoConsumer::Disconnected\n"));

	if (where == mVideoIn.destination && producer == mVideoIn.source)
	{
		delete(mPreviewBitmap);
		mPreviewBitmap = NULL;
		// disconnect the connection
		mVideoIn.source = media_source::null;
		mVidRate = 29.97;
		mConnectionActive = false;
	}
	else if (where == mAudioIn.destination && producer == mAudioIn.source)
	{
		// disconnect the connection
		mAudioIn.source = media_source::null;
		//mAudioConnectionActive = false;
		//delete mAudioBuffers;	// 4.5 audio node is broken
		//mAudioBuffers = NULL;
		free(mAudioBuffer);
		mAudioBuffer = NULL;
	}

	FUNCTION(("VideoConsumer::Disconnected done\n"));

}

//---------------------------------------------------------------

status_t 
VideoConsumer::AcceptFormat(const media_destination  &dest, media_format *format)
{
	char format_string[256];		
	string_for_format(*format, format_string, 256);

	FUNCTION(("VideoConsumer::AcceptFormat: %s\n", format_string));

	if(dest == mAudioIn.destination) {
		if (format->type == B_MEDIA_NO_TYPE)
			format->type = B_MEDIA_RAW_AUDIO;
		
		if (format->type != B_MEDIA_RAW_AUDIO)
		{
			format->type = B_MEDIA_RAW_AUDIO;
			format->u.raw_audio = audio_format;
			return B_MEDIA_BAD_FORMAT;
		}
		return B_NO_ERROR;
	}

	if (format->type == B_MEDIA_NO_TYPE)
		format->type = B_MEDIA_RAW_VIDEO;
	
	if ((format->type != B_MEDIA_RAW_VIDEO) &&
		(format->type != B_MEDIA_ENCODED_VIDEO))
	{	
		format->u.raw_video = vid_format;
		return B_MEDIA_BAD_FORMAT;
	}
	
	if(mWriting) {
		return B_NOT_ALLOWED;
	}

	if (format->type == B_MEDIA_ENCODED_VIDEO)
		return B_OK;

	bool goodFormat = true;

	//field_rate
	if (format->u.raw_video.field_rate == media_raw_video_format::wildcard.field_rate)
		format->u.raw_video.field_rate = 29.97;
	if (format->u.raw_video.field_rate < 1 || format->u.raw_video.field_rate > 60)
	{
		format->u.raw_video.field_rate = 29.97;
		printf("field_rate bad\n");
		goodFormat = false;
	}
	
	if (format->u.raw_video.interlace == media_raw_video_format::wildcard.interlace)
		format->u.raw_video.interlace = 1;
		
	if (format->u.raw_video.interlace > 2)
	{
		format->u.raw_video.interlace = 1;
		printf("interlace bad\n");
		goodFormat = false;
	}

	if (format->u.raw_video.first_active == media_raw_video_format::wildcard.first_active)
		format->u.raw_video.first_active = 0;
	if (format->u.raw_video.first_active != 0)
	{
		format->u.raw_video.first_active = 0;
		printf("first_active bad\n");
		goodFormat = false;
	}

	if (format->u.raw_video.orientation == media_raw_video_format::wildcard.orientation)
		format->u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
	if (format->u.raw_video.orientation != B_VIDEO_TOP_LEFT_RIGHT)
	{
		format->u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
		printf("orientation bad\n");
		goodFormat = false;
	}

	if (format->u.raw_video.pixel_width_aspect == media_raw_video_format::wildcard.pixel_width_aspect)
		format->u.raw_video.pixel_width_aspect = 1;
	
	if (format->u.raw_video.pixel_height_aspect == media_raw_video_format::wildcard.pixel_height_aspect)
		format->u.raw_video.pixel_height_aspect = format->u.raw_video.pixel_width_aspect;
	
	if (format->u.raw_video.display.format == media_video_display_info::wildcard.format)
		format->u.raw_video.display.format = B_RGB32;
	
	// if we can't draw bitmaps in this color space don't accept it
//	uint32 supportFlags = 0;
//	bool colorSpaceSupported = bitmaps_support_space(format->u.raw_video.display.format, &supportFlags);
//	if (!colorSpaceSupported || !(supportFlags & B_VIEWS_SUPPORT_DRAW_BITMAP) )
//	{
//		format->u.raw_video.display.format = B_RGB32;
//		goodFormat = false;
//	}

	if (format->u.raw_video.display.pixel_offset == media_video_display_info::wildcard.pixel_offset)
		format->u.raw_video.display.pixel_offset = 0;
	if (format->u.raw_video.display.pixel_offset != 0)
	{
		format->u.raw_video.display.pixel_offset = 0;
		printf("pixel_offset bad\n");
		goodFormat = false;
	}

	if (format->u.raw_video.display.line_offset == media_video_display_info::wildcard.line_offset)
		format->u.raw_video.display.line_offset = 0;
		
	if (format->u.raw_video.display.line_offset != 0)
	{
		format->u.raw_video.display.line_offset = 0;
		printf("line_offset bad\n");
		goodFormat = false;
	}
	
	// last_active <= line_count -1
	if (format->u.raw_video.last_active == media_raw_video_format::wildcard.last_active)
	{
		if (format->u.raw_video.display.line_count == media_video_display_info::wildcard.line_count)
		{
			// both are wildcards so lets pick a line count
			format->u.raw_video.display.line_count = 240;
		}
		// set the last_active based on the line count
		format->u.raw_video.last_active = format->u.raw_video.display.line_count -1;
	
	}
	else {
		if (format->u.raw_video.display.line_count == media_video_display_info::wildcard.line_count)
			format->u.raw_video.display.line_count = format->u.raw_video.last_active + 1;
		
		if (format->u.raw_video.last_active > format->u.raw_video.display.line_count -1)
		{
			ERROR(("raw_video.last_active beyond the size of the display!\n"));
			printf("last_active (%ld) bad\n", format->u.raw_video.last_active);
			format->u.raw_video.last_active = format->u.raw_video.display.line_count -1;
			goodFormat = false;
		}
		else {
//			fClipBitmap = true;
//			INFO("we need to clip the bitmap\n");
		}
	}
	
	// bytes_per_row = line_width * bytes_per_colorspace
	
	//determine size of a pixel in bytes
	size_t pixelChunk = 0;
	size_t rowAlignment = 0;
	size_t pixelsPerChunk = 0;
	get_pixel_size_for(format->u.raw_video.display.format, &pixelChunk, &rowAlignment, &pixelsPerChunk);
		
	if (format->u.raw_video.display.line_width == media_video_display_info::wildcard.line_width)
	{
		format->u.raw_video.display.line_width = 320;
	}
	size_t our_bytes_per_row =
		get_bytes_per_row(format->u.raw_video.display.format,
		                  format->u.raw_video.display.line_width);
	if (format->u.raw_video.display.bytes_per_row == media_video_display_info::wildcard.bytes_per_row)
	{
		format->u.raw_video.display.bytes_per_row = our_bytes_per_row;
	}
	
	if(format->u.raw_video.display.bytes_per_row != our_bytes_per_row) {
		format->u.raw_video.display.bytes_per_row = our_bytes_per_row;
		goodFormat = false;
	}
		
	string_for_format(*format, format_string, 256);

	FUNCTION(("AcceptFormat %s: %s\n", (goodFormat)?"OK":"BadFormat", format_string));
	
	if (goodFormat)
		return B_OK;
	else 
		return B_MEDIA_BAD_FORMAT;
}

//---------------------------------------------------------------

status_t
VideoConsumer::GetNextInput(
	int32 * cookie,
	media_input * out_input)
{
	FUNCTION(("VideoConsumer::GetNextInput\n"));

	// custom build a destination for this connection
	// put connection number in id

	if (*cookie < 1) {
		mVideoIn.node = Node();
		mVideoIn.destination.id = *cookie;
		sprintf(mVideoIn.name, "Video Consumer");
		*out_input = mVideoIn;
		(*cookie)++;
		return B_OK;
	}
	else if (*cookie < 2) {
		mAudioIn.node = Node();
		mAudioIn.destination.id = *cookie;
		sprintf(mAudioIn.name, "Audio Consumer");
		*out_input = mAudioIn;
		(*cookie)++;
		return B_OK;
	}
	else {
		//ERROR(("VideoConsumer::GetNextInput - - BAD INDEX\n"));
		return B_MEDIA_BAD_DESTINATION;
	}
}

//---------------------------------------------------------------

void
VideoConsumer::DisposeInputCookie(int32 /*cookie*/)
{
}

//---------------------------------------------------------------

status_t
VideoConsumer::GetLatencyFor(
	const media_destination &for_whom,
	bigtime_t * out_latency,
	media_node_id * out_timesource)
{
	FUNCTION(("VideoConsumer::GetLatencyFor\n"));
	
	if(for_whom == mVideoIn.destination) {
		*out_latency = mMyLatency;
		*out_timesource = TimeSource()->ID();
	}
	else if (for_whom != mAudioIn.destination) {
		*out_latency = mMyLatency;
		*out_timesource = TimeSource()->ID();
	}
	else {
		return B_MEDIA_BAD_DESTINATION;
	}
	
	return B_OK;
}


//---------------------------------------------------------------

status_t
VideoConsumer::FormatChanged(
				const media_source & producer,
				const media_destination & consumer, 
				int32 /*from_change_count*/,
				const media_format &format)
{
	FUNCTION(("VideoConsumer::FormatChanged\n"));
	
	if (consumer != mVideoIn.destination)
		return B_MEDIA_BAD_DESTINATION;

	if (producer != mVideoIn.source)
		return B_MEDIA_BAD_SOURCE;

	mVideoIn.format = format;
	
	return CreateBuffers(format);
}

//---------------------------------------------------------------

status_t 
VideoConsumer::ConfigureDecoder()
{
	status_t err;

	if(mVideoIn.format.type != B_MEDIA_ENCODED_VIDEO) {
		return B_MEDIA_BAD_FORMAT;
	}

	err = mDecoder.SetTo(&mVideoIn.format);
	if(err != B_NO_ERROR) {
		// Special case DV as it contains audio and video
		media_format	dvstream, dvvideo;

		err = BMediaFormats::GetBeOSFormatFor('dv_v', &dvvideo, B_MEDIA_ENCODED_VIDEO);
		if(err != B_NO_ERROR)
			return err;
		err = BMediaFormats::GetAVIFormatFor('cdvc', &dvstream, B_MEDIA_ENCODED_VIDEO);
		if(err != B_NO_ERROR)
			return err;
		if(mVideoIn.format.u.encoded_video.encoding != dvstream.u.encoded_video.encoding)
			return B_MEDIA_BAD_FORMAT;
	
		printf("Found encoded video format is DV\n"); 

		media_format dv_video_format = mVideoIn.format;
		dv_video_format.u.encoded_video.encoding =
			dvvideo.u.encoded_video.encoding;
		err = mDecoder.SetTo(&dv_video_format);
		if(err != B_NO_ERROR)
			return err;
	}
	printf("Found decoder for encoded video format\n"); 

	media_format decoded_format;
	decoded_format.type = B_MEDIA_RAW_VIDEO;
	decoded_format.u.raw_video = media_raw_video_format::wildcard;
	decoded_format.u.raw_video.display.format = B_RGB32;
	decoded_format.u.raw_video.interlace = 2;
	media_format last_requested_format;
	while(1) {
		bool format_good = true;
		last_requested_format = decoded_format;
		err = mDecoder.SetOutputFormat(&decoded_format);
		if(err != B_NO_ERROR) {
			printf("SetOutputFormat failed\n");
			return err;
		}

		if(decoded_format.u.raw_video.display.format != B_RGB32) {
			format_good = false;
			decoded_format.u.raw_video.display.format = B_RGB32;
		}

		if(decoded_format.u.raw_video.interlace == 2) {
			if(decoded_format.u.raw_video.display.bytes_per_row !=
			   decoded_format.u.raw_video.display.line_width * 8) {
				decoded_format.u.raw_video.display.bytes_per_row =
					decoded_format.u.raw_video.display.line_width * 8;
				format_good = false;
			}
		}
		else {
			if(decoded_format.u.raw_video.display.bytes_per_row !=
			   decoded_format.u.raw_video.display.line_width * 4) {
				decoded_format.u.raw_video.display.bytes_per_row =
					decoded_format.u.raw_video.display.line_width * 4;
				format_good = false;
			}
		}
		if(format_good)
			break;

		if(memcmp(&last_requested_format, &decoded_format, sizeof(decoded_format)) == 0) {
			return B_MEDIA_BAD_FORMAT;
		}
	}
	mDecodeFields = decoded_format.u.raw_video.interlace == 2;
	if(decoded_format.u.raw_video.display.flags & B_TOP_SCANLINE_F2)
		mBottomField = 0;
	else
		mBottomField = 1;

	media_video_display_info *display;
	display = &decoded_format.u.raw_video.display;
	BRect bitmapframe(0, 0, display->line_width-1,
	                  display->line_count-1);
	mPreviewBitmap = new BBitmap(bitmapframe, B_RGB32, false, false);

	if(!mPreviewBitmap->IsValid()) {
		ERROR(("VideoConsumer::Connected - COULDN'T create preview bitmap\n"));
		delete mPreviewBitmap;
		mPreviewBitmap = NULL;
		return B_ERROR;
	}
	printf("PreviewBitmap: %f %f\n", bitmapframe.Width(), bitmapframe.Height());
	return B_NO_ERROR;
}

#define SHIFT_BITS 8

extern int32 LUT_1_164[0x100];
extern int32 LUT_1_596[0x100];
extern int32 LUT_0_813[0x100];
extern int32 LUT_0_392[0x100];
extern int32 LUT_2_017[0x100];

uchar fixed32toclipped8(int32 fixed)
{
	if (fixed <= 0)
		return 0;
	else if (fixed >= (255 << SHIFT_BITS))
		return 255;
	else
		return (fixed + (1 << (SHIFT_BITS - 1))) >> SHIFT_BITS;
}

inline uint32 ycbcr_to_rgb(uchar y, uchar cb, uchar cr)
{
	int32 Y;
	uchar red, green, blue;

/*
	red = clip8(1.164 * (y - 16) + 1.596 * (cr - 128));
	green = clip8(1.164 * (y - 16) - 0.813 * (cr - 128) - 0.392 * (cb - 128));
	blue = clip8(1.164 * (y - 16) + 2.017 * (cb - 128));
*/

	Y = LUT_1_164[y];

	red =	fixed32toclipped8(Y + LUT_1_596[cr]);
	green =	fixed32toclipped8(Y - LUT_0_813[cr] - LUT_0_392[cb]);
	blue =	fixed32toclipped8(Y + LUT_2_017[cb]);

#if B_HOST_IS_LENDIAN
	return (blue | (green << 8) | (red << 16));
#else
	return ((red << 8) | (green << 16) | (blue << 24));
#endif
}

#if 0
inline uint32 clip8(double d)
{
	if (d > 255.0)
		return 255;
	else if (d < 0)
		return 0;
	else
		return (uint32) d;
}

inline uint32 ycbcr_to_rgb(double a, double b, double c)
{
	double y = a;
	double cb = b;
	double cr = c;
	double rscale = 1;
	double bscale = 1;
#if 0
	uint8 red = clip8(y + 1.371 * (cr - 128));
	uint8 green = clip8(y - 0.698 * (cr - 128) - 0.336 * (cb - 128));
	uint8 blue = clip8(y + 1.732 * (cb - 128));
#elif 1
	uint8 red = clip8(1.164 * (y - 16) + rscale * 1.596 * (cr - 128));
	uint8 green = clip8(1.164 * (y - 16) - rscale * 0.813 * (cr - 128) - bscale * 0.392 * (cb - 128));
	uint8 blue = clip8(1.164 * (y - 16) + bscale * 2.017 * (cb - 128));
#else
	uint8 red = clip8(y + rscale * (cr - 128));
	uint8 green = clip8(y - rscale * (cr - 128) - bscale * (cb - 128));
	uint8 blue = clip8(y + bscale * (cb - 128));
#endif

//	TRACE(("YCbCr %.0f %.0f %.0f > RGB %d %d %d\n", y, cb, cr, red, green, blue));

#if B_HOST_IS_LENDIAN
	return (blue | (green << 8) | (red << 16));
#else
	return ((red << 8) | (green << 16) | (blue << 24));
#endif
}
#endif


status_t 
VideoConsumer::DrawPreview(BBuffer *buffer)
{
	if(mVideoIn.format.type == B_MEDIA_ENCODED_VIDEO) {
		int64 frames = 1;
		media_header mh;
		status_t err;
		uint8	*bitmap_bits = (uint8 *)mPreviewBitmap->Bits();
		if(mDecodeFields && buffer->VideoHeader()->field_number == mBottomField) {
			bitmap_bits += mPreviewBitmap->BytesPerRow();
		}
		err = mDecoder.DecodeBuffer(buffer->Data(), buffer->SizeUsed(),
		                            bitmap_bits, &frames, &mh);
		if(err != B_NO_ERROR)
			return err;
		if(frames != 1)
			return B_ERROR;
	}
	else {
		size_t width = mVideoIn.format.u.raw_video.display.line_width;
		size_t height = mVideoIn.format.u.raw_video.display.line_count;
		size_t rowbytes = mVideoIn.format.u.raw_video.display.bytes_per_row;
		
		//printf("DrawPreview: %d %d\n", width, height);
		switch(mVideoIn.format.u.raw_video.display.format) {
			case B_GRAY8: {
				uint8 *dest = (uint8*)mPreviewBitmap->Bits();
				uint8 *src = (uint8*)buffer->Data();
				uint8 *end = src+width*height;
				if(width*height > buffer->SizeUsed()) {
					return B_ERROR;
				}
				while(src < end) {
					uint8 Y0 = *src++;
					*dest++ = Y0;
					*dest++ = Y0;
					*dest++ = Y0;
					dest++;
				}
			} break;
	
			case B_RGB24: {
				uint8 *dest = (uint8*)mPreviewBitmap->Bits();
				uint8 *src = (uint8*)buffer->Data();
				uint8 *end = src+width*height*3;
				if(width*height*3 > buffer->SizeUsed()) {
					return B_ERROR;
				}
				while(src < end) {
					*dest++ = *src++;
					*dest++ = *src++;
					*dest++ = *src++;
					dest++;
				}
			} break;
	
			case B_YCbCr422: {
				uint32 *dest = (uint32*)mPreviewBitmap->Bits();
				uint8 *src = (uint8*)buffer->Data();
				uint8 *end = src+rowbytes*height;
				int skip = rowbytes-width*2;
				int skipin = width;
				if(skip < 0 || rowbytes*height > buffer->SizeUsed()) {
					return B_ERROR;
				}
				while(src < end) {
					uint8 Y0, Y1, Cb, Cr;
					Y0 = *src++;
					Cb = *src++;
					if(--skipin == 0) {
						Cr = *(src-3);
						src += skip;
						skipin = width;
						*dest++ = ycbcr_to_rgb(Y0, Cb, Cr);
					}
					else {
						Y1 = *src++;
						Cr = *src++;
						*dest++ = ycbcr_to_rgb(Y0, Cb, Cr);
						*dest++ = ycbcr_to_rgb(Y1, Cb, Cr);
						if(--skipin == 0) {
							src += skip;
							skipin = width;
						}
					}
				}
			} break;
	
			case B_YCbCr411: {
				uint32 *dest = (uint32*)mPreviewBitmap->Bits();
				uint8 *src = (uint8*)buffer->Data();
				uint8 *end = src+width*height+width*height/2;
				if(width*height+width*height/2 > buffer->SizeUsed()) {
					return B_ERROR;
				}
				while(src < end) {
					uint8 Y0, Y1, Y2, Y3, Y4, Y5, Y6, Y7;
					uint8 Cb0, Cr0;
					uint8 Cb4, Cr4;
					Cb0 = *src++;
					Y0 = *src++;
					Cr0 = *src++;
					Y1 = *src++;
					Cb4 = *src++;
					Y2 = *src++;
					Cr4 = *src++;
					Y3 = *src++;
					Y4 = *src++;
					Y5 = *src++;
					Y6 = *src++;
					Y7 = *src++;
					*dest++ = ycbcr_to_rgb(Y0, Cb0, Cr0);
					*dest++ = ycbcr_to_rgb(Y1, Cb0, Cr0);
					*dest++ = ycbcr_to_rgb(Y2, Cb0, Cr0);
					*dest++ = ycbcr_to_rgb(Y3, Cb0, Cr0);
					*dest++ = ycbcr_to_rgb(Y4, Cb4, Cr4);
					*dest++ = ycbcr_to_rgb(Y5, Cb4, Cr4);
					*dest++ = ycbcr_to_rgb(Y6, Cb4, Cr4);
					*dest++ = ycbcr_to_rgb(Y7, Cb4, Cr4);
				}
			} break;
	
			default:
				return B_ERROR;
		}
	}
	mView->DrawBitmap(mPreviewBitmap, mView->Bounds());
	//BRect(0,0,319,239));
	return B_NO_ERROR;
}


void
VideoConsumer::HandleEvent(
	const media_timed_event *event,
	bigtime_t /*lateness*/,
	bool /*realTimeEvent*/)

{
	LOOP(("VideoConsumer::HandleEvent\n"));
	
	BBuffer *buffer;
	
	switch (event->type)
	{
		case BTimedEventQueue::B_START:
			PROGRESS(("VideoConsumer::HandleEvent - START\n"));
			break;
		case BTimedEventQueue::B_STOP:
			PROGRESS(("VideoConsumer::HandleEvent - STOP\n"));
			EventQueue()->FlushEvents(event->event_time, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HANDLE_BUFFER);
			UninitMediaFile();
			break;
		case BTimedEventQueue::B_USER_EVENT:
			PROGRESS(("VideoConsumer::HandleEvent - USER EVENT\n"));
			if (RunState() == B_STARTED)
			{
				//PROGRESS(("Pushing user event for %.4f, time now %.4f\n", (event->event_time + mRate)/M1, event->event_time/M1));
				//media_timed_event newEvent(event->event_time + mRate, BTimedEventQueue::B_USER_EVENT);
				//EventQueue()->AddEvent(newEvent);
			}
				break;
		case BTimedEventQueue::B_HANDLE_BUFFER:
			LOOP(("VideoConsumer::HandleEvent - HANDLE BUFFER\n"));
			buffer = (BBuffer *) event->pointer;
			if(!mWriting || mCaptureConfig.preview) {
				if (/*RunState() == B_STARTED && */ mConnectionActive)
				{
					// see if this is one of our buffers
					uint32 index = 0;
					mOurBuffers = true;
					while(index < 3)
						if ((uint32)buffer == mBufferMap[index])
							break;
						else
							index++;
							
					if (index == 3)
					{
						// no, buffers belong to consumer
						mOurBuffers = false;
						index = 0;
					}
					
					//printf("HandleBuffer: Now: %Ld buffer->Header()->start_time: %Ld\n", TimeSource()->Now(),  buffer->Header()->start_time);
					if ( (RunMode() == B_OFFLINE) ||
						 ((TimeSource()->Now() > (buffer->Header()->start_time - JITTER)) &&
						  (TimeSource()->Now() < (buffer->Header()->start_time + JITTER))) )
					{
						if (!mOurBuffers && mBitmap[index])
							// not our buffers, so we need to copy
							memcpy(mBitmap[index]->Bits(), buffer->Data(),mBitmap[index]->BitsLength());
							
	#if 0
						if(track && mBitmap[index]) {
							//printf("Writing bitmap %p\n", mBitmap[index]);
							track->WriteFrames(mBitmap[index]->Bits(), 1, 0 /*B_MEDIA_KEY_FRAME*/);
						}
						else {
							printf("could not write\n");
						}
	#endif
	#if 1
						if (mWindow->Lock())
						{
#if 0
							uint32 flags;
							if ((mColorSpace == B_GRAY8) &&
								!bitmaps_support_space(mColorSpace, &flags))
							{
								// handle mapping of GRAY8 until app server knows how
								uint32 *start = (uint32 *)mBits[index];
								int32 size = mWidth*mHeight;
								uint32 *end = start + size/4;
								for (uint32 *p = start; p < end; p++)
									*p = (*p >> 3) & 0x1f1f1f1f;									
							}
							else
#endif
							if(mPreviewBitmap) {
								DrawPreview(buffer);
							}
							if(mBitmap[index])
								mView->DrawBitmap(mBitmap[index], mView->Bounds());
//								mView->DrawBitmap(mBitmap[index], BRect(0,0,319,239));
							mWindow->Unlock();
						}
	#endif
					}
					else {
						//PROGRESS(("VidConsumer::HandleEvent - DROPPED FRAME\n"));
					}
					buffer->Recycle();
				}
				else
					buffer->Recycle();
			}
			else
				buffer->Recycle();
			break;
		default:
			ERROR(("VideoConsumer::HandleEvent - BAD EVENT\n"));
			break;
	}			
}

//---------------------------------------------------------------

void
VideoConsumer::UpdateStatus(char *status)
{
	//printf("STATUS: %s\n",status);
	if (mView->Window()->Lock())
	{
		mStatusLine->SetText(status);
		mView->Window()->Unlock();
	}
}
