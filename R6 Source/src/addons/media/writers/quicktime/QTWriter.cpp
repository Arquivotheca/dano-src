#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <File.h>
#include <FileWriter.h>
#include <Errors.h>
#include <GraphicsDefs.h>
#include <MediaDefs.h>
#include <Screen.h>
#include <MediaTrack.h>
#include <MediaFormats.h>
#include "MediaWriter.h"
#include "QTAtomWriter.h"
#include "QTWriter.h"


MediaWriter *
instantiate_mediawriter(void)
{
	return new QTWriter();
}

status_t
get_mediawriter_info(media_file_format *mfi)
{
	strcpy(mfi->mime_type,       "video/quicktime");
	strcpy(mfi->pretty_name,    "QuickTime File Format");
	strcpy(mfi->short_name,     "quicktime");
	strcpy(mfi->file_extension, "mov");

	mfi->capabilities = media_file_format::B_KNOWS_RAW_AUDIO |
		                media_file_format::B_KNOWS_RAW_VIDEO |
						media_file_format::B_KNOWS_ENCODED_AUDIO |
		                media_file_format::B_KNOWS_ENCODED_VIDEO |
						media_file_format::B_WRITABLE;
	mfi->family       = B_QUICKTIME_FORMAT_FAMILY;

	return B_OK;
}


status_t accepts_format(media_format *fmt, uint32 flags)
{
	BMediaFormats formatObject;
	media_format_description fd;
	int reject_wildcards = flags & B_MEDIA_REJECT_WILDCARDS;
	status_t err;

	switch(fmt->type) {
	case B_MEDIA_RAW_VIDEO:
		switch(fmt->u.raw_video.display.format) {
			case 0:	/* wildcard */
				if(reject_wildcards)
					return B_BAD_TYPE;
			case B_RGB32_BIG:
			case B_RGBA32_BIG:
			case B_RGB24_BIG:
			case B_RGB15_BIG:
			case B_RGBA15_BIG:
			case B_CMAP8:
			case B_YCbCr422:
				return B_OK;

			default:
				return B_BAD_TYPE;
		}

		if(fmt->u.raw_video.display.line_width < 1)
			if(reject_wildcards ||
				fmt->u.raw_video.display.line_width != media_video_display_info::wildcard.line_width)
			{
				return B_BAD_TYPE;
			}

		if(fmt->u.raw_video.display.line_count < 1)
			if(reject_wildcards ||
				fmt->u.raw_video.display.line_count != media_video_display_info::wildcard.line_count)
			{
				return B_BAD_TYPE;
			}

		if(fmt->u.raw_video.field_rate < 0.01)
			if(reject_wildcards ||
				fmt->u.raw_video.field_rate != media_raw_video_format::wildcard.field_rate)
			{
				return B_BAD_TYPE;
			}
		
		return B_OK;

	case B_MEDIA_RAW_AUDIO:
		switch(fmt->u.raw_audio.format) {
			case 0:	/* wildcard */
				if(reject_wildcards)
					return B_BAD_TYPE;
			case media_raw_audio_format::B_AUDIO_UCHAR:
			case media_raw_audio_format::B_AUDIO_CHAR:
			case media_raw_audio_format::B_AUDIO_SHORT:
			case media_raw_audio_format::B_AUDIO_INT:
				break;

			default:
				return B_BAD_TYPE;
		}

		if(fmt->u.raw_audio.channel_count < 1)
			if(reject_wildcards ||
				fmt->u.raw_audio.channel_count != media_raw_audio_format::wildcard.channel_count)
			{
				return B_BAD_TYPE;
			}

		if(fmt->u.raw_audio.frame_rate < 0.01)
			if(reject_wildcards ||
				fmt->u.raw_audio.frame_rate != media_raw_audio_format::wildcard.frame_rate)
			{
				return B_BAD_TYPE;
			}
		
		return B_OK;

	case B_MEDIA_ENCODED_VIDEO:
		err = formatObject.GetCodeFor(*fmt, B_QUICKTIME_FORMAT_FAMILY, &fd);
		if(err != B_OK) {
			return err;
		}
		if(fd.family != B_QUICKTIME_FORMAT_FAMILY) {
			return B_BAD_TYPE;
		}

		if(fmt->u.encoded_video.output.display.line_width < 1)
			if(reject_wildcards ||
				fmt->u.encoded_video.output.display.line_width != media_video_display_info::wildcard.line_width)
			{
				return B_BAD_TYPE;
			}

		if(fmt->u.encoded_video.output.display.line_count < 1)
			if(reject_wildcards ||
				fmt->u.encoded_video.output.display.line_count != media_video_display_info::wildcard.line_count)
			{
				return B_BAD_TYPE;
			}

		if(fmt->u.encoded_video.output.field_rate < 0.01)
			if(reject_wildcards ||
				fmt->u.encoded_video.output.field_rate != media_raw_video_format::wildcard.field_rate)
			{
				return B_BAD_TYPE;
			}

		return B_OK;

	case B_MEDIA_ENCODED_AUDIO:
		err = formatObject.GetCodeFor(*fmt, B_QUICKTIME_FORMAT_FAMILY, &fd);
		if(err != B_OK) {
			return err;
		}
		if(fd.family != B_QUICKTIME_FORMAT_FAMILY) {
			return B_BAD_TYPE;
		}

		if(fmt->u.encoded_audio.output.channel_count < 1)
			if(reject_wildcards ||
				fmt->u.encoded_audio.output.channel_count != media_raw_audio_format::wildcard.channel_count)
			{
				return B_BAD_TYPE;
			}

		if(fmt->u.encoded_audio.output.frame_rate < 0.01)
			if(reject_wildcards ||
				fmt->u.encoded_audio.output.frame_rate != media_raw_audio_format::wildcard.frame_rate)
			{
				return B_BAD_TYPE;
			}
		
		return B_OK;

	default:
		return B_BAD_TYPE;
	}
}

QTWriter::QTWriter()
{
	int i;

	f_numtracks = 0;
	f_qt = new QTAtomWriter();

	for(i=0; i < sizeof(f_qttracks)/sizeof(QTTrack *); i++)
		f_qttracks[i] = NULL;

	f_source = NULL;
}

QTWriter::~QTWriter()
{
	int i;
	
	for(i=0; i < sizeof(f_qttracks)/sizeof(QTTrack *); i++) {
		if (f_qttracks[i])
			delete f_qttracks[i];
	}

	if (f_qt)
		delete f_qt;
	f_qt = NULL;

	f_source = NULL;    // just clear this out.  it's not ours to delete 
}


status_t
QTWriter::AddCopyright(const char *data)
{
	if(f_qt == NULL || header_committed)
		return B_NOT_ALLOWED;

	return f_qt->AddCopyright(data);
}

status_t
QTWriter::AddTrackInfo(int32 trk, uint32 code,const char *dta,size_t sz)
{
	return B_ERROR;
}


status_t
QTWriter::SetSource(BDataIO *source)
{
	status_t err;
	
	BPositionIO *file = dynamic_cast<BPositionIO *>(source);
	if (file == NULL)
		return B_BAD_TYPE;

	f_source = new FileWriter(file);
	err = f_source->InitCheck();
	if(err != B_NO_ERROR) {
		delete f_source;
		f_source = NULL;
		return err;
	}

	f_qt->SetTo(f_source);
	f_qt->Begin();
	header_committed = false;

	return B_OK;
}
	

static int32
BytesPerPixel(color_space space)
{
	size_t		alignment, pixelsPerChunk;
	size_t		pixelChunk = 0;

	get_pixel_size_for(space, &pixelChunk, &alignment, &pixelsPerChunk);

	return pixelChunk/pixelsPerChunk;
}

static QTTrack *
create_compressed_qt_video_track(BMediaTrack *track, media_format *mf)
{
	QTTrack *myTrack = new QTTrack();


	myTrack->SetType(QT_VIDEO);			

	myTrack->imageWidth = mf->u.encoded_video.output.display.line_width;
	myTrack->imageHeight = mf->u.encoded_video.output.display.line_count;

	/* work out what to set codecID and codecVendor based on media_file_format */

	if (mf->u.encoded_video.encoding == media_encoded_video_format::B_ANY) {
		/* we have been given no information about what encoding to use, B_ANY is no use */
		return NULL;
	}

	/* setup codec details as Apple raw */
	myTrack->videoCodecList = new video_smp_details;
	memset(myTrack->videoCodecList, 0, sizeof(video_smp_details));
	myTrack->videoCodecCount = 1;
	myTrack->videoCodecList[0].width = myTrack->imageWidth;
	myTrack->videoCodecList[0].height = myTrack->imageHeight;
	
	BMediaFormats				formatObject;
	media_format_description 	mfd;

	memset(&mfd, 0, sizeof(mfd));
	
	formatObject.GetCodeFor(*mf, B_QUICKTIME_FORMAT_FAMILY, &mfd);
	

	myTrack->videoCodecList[0].codecVendor = 'aapl';
	myTrack->videoCodecList[0].codecID     = mfd.u.quicktime.codec;
	myTrack->videoCodecList[0].usecs_per_frame = 1000000.0 / mf->u.encoded_video.output.field_rate; 
	
	myTrack->videoCodecList[0].depth = BytesPerPixel(mf->u.encoded_video.output.display.format) * 8;
		
	if ((myTrack->videoCodecList[0].depth == 0) ||
		(myTrack->videoCodecList[0].depth > 24)) {
		myTrack->videoCodecList[0].depth = 24;
	}
		
	myTrack->encodeBufferSize = 128*1024;   /* flush size is 128k */
	myTrack->encodeBuffer = malloc(myTrack->encodeBufferSize);
		
	myTrack->stssMaxCount = 50;
	myTrack->stssTable = (uint32 *)malloc(myTrack->stssMaxCount * sizeof(uint32));

	return myTrack;
}


static QTTrack *
create_raw_qt_video_track(BMediaTrack *track, media_format *mf)
{
	QTTrack *myTrack = new QTTrack();

	myTrack->SetType(QT_VIDEO);			

	myTrack->imageWidth  = mf->u.raw_video.display.line_width;
	myTrack->imageHeight = mf->u.raw_video.display.line_count;
		
	/* setup codec details as Apple raw */
	myTrack->videoCodecList = new video_smp_details;
	memset(myTrack->videoCodecList, 0, sizeof(video_smp_details));
	myTrack->videoCodecCount = 1;
	myTrack->videoCodecList[0].width       = mf->u.raw_video.display.line_width;
	myTrack->videoCodecList[0].height      = mf->u.raw_video.display.line_count;
	myTrack->videoCodecList[0].codecVendor = 'appl';
	if(mf->u.raw_video.display.format == B_YCbCr422) {
		myTrack->videoCodecList[0].codecID     = 'YUV2';
	}
	else {
		myTrack->videoCodecList[0].codecID     = 'raw ';
	}
	myTrack->videoCodecList[0].usecs_per_frame = 1000000.0 / mf->u.raw_video.field_rate; 

	myTrack->videoCodecList[0].depth = BytesPerPixel(mf->u.raw_video.display.format) * 8;

	if (mf->u.raw_video.display.format == B_CMAP8) {
		/* fill colour map with standard BeOS */
		BScreen			screen;			

		myTrack->videoCodecList[0].colourMap = new rgb_color[256];
		myTrack->videoCodecList[0].depth = 8; 
		memcpy(myTrack->videoCodecList[0].colourMap,
				screen.ColorMap()->color_list,
				sizeof(rgb_color) * 256);
		myTrack->videoCodecList[0].colourCount = 256;
	}

	myTrack->encodeBufferSize  = (myTrack->imageWidth * myTrack->imageHeight);
	myTrack->encodeBufferSize *= BytesPerPixel(mf->u.raw_video.display.format);
	myTrack->encodeBuffer      = malloc(myTrack->encodeBufferSize);

	return myTrack;
}



QTTrack *
create_raw_qt_audio_track(BMediaTrack *track, media_format *mf)
{
	QTTrack *myTrack = new QTTrack();

	myTrack->SetType(QT_AUDIO);			

	/* setup codec details as Apple raw */
	myTrack->audioCodecList = new audio_smp_details;
	memset(myTrack->audioCodecList, 0, sizeof(audio_smp_details));
	myTrack->audioCodecCount = 1;
	myTrack->audioCodecList[0].codecVendor = 'appl';

	myTrack->audioCodecList[0].audioChannels   = mf->u.raw_audio.channel_count;
	myTrack->audioCodecList[0].audioSampleRate = (int32)mf->u.raw_audio.frame_rate;
		
	switch (mf->u.raw_audio.format) {
		case media_raw_audio_format::B_AUDIO_UCHAR:
			myTrack->audioCodecList[0].codecID     = 'raw ';
			myTrack->audioCodecList[0].bitsPerSample = sizeof(uchar) * 8;
			break;
		case media_raw_audio_format::B_AUDIO_CHAR:
			myTrack->audioCodecList[0].codecID     = 'twos';
			myTrack->audioCodecList[0].bitsPerSample = sizeof(char) * 8;
			break;
		case media_raw_audio_format::B_AUDIO_SHORT:
			myTrack->audioCodecList[0].codecID     = 'twos';
			myTrack->audioCodecList[0].bitsPerSample = sizeof(short) * 8;
			break;
		case media_raw_audio_format::B_AUDIO_INT:
			myTrack->audioCodecList[0].codecID     = 'twos';
			myTrack->audioCodecList[0].bitsPerSample = sizeof(int) * 8;
			break;
		default:
			// the above are the only raw formats supported by QuickTime
			return NULL;
	}

	myTrack->audioCodecList[0].bytesPerFrame =
		myTrack->audioCodecList[0].audioChannels *
		myTrack->audioCodecList[0].bitsPerSample / 8;

	//
	// raw audio needs to be written as big endian, see if we need to swap it.
	// if byte_order is a wildcard (0), assume the data is in host order.
	//
	if(mf->u.raw_audio.byte_order > 0 && mf->u.raw_audio.byte_order != B_MEDIA_BIG_ENDIAN ||
		mf->u.raw_audio.byte_order == 0 && B_MEDIA_HOST_ENDIAN != B_MEDIA_BIG_ENDIAN)
	{
		myTrack->swapRawAudio = myTrack->audioCodecList[0].bitsPerSample > 8;
	}

	myTrack->audioCodecList[0].bytesPerFrame =
		myTrack->audioCodecList[0].audioChannels *
		myTrack->audioCodecList[0].bitsPerSample / 8;

	//
	// raw audio needs to be written as big endian, see if we need to swap it.
	// if byte_order is a wildcard (0), assume the data is in host order.
	//
	if(mf->u.raw_audio.byte_order > 0 && mf->u.raw_audio.byte_order != B_MEDIA_BIG_ENDIAN ||
		mf->u.raw_audio.byte_order == 0 && B_MEDIA_HOST_ENDIAN != B_MEDIA_BIG_ENDIAN)
	{
		myTrack->swapRawAudio = myTrack->audioCodecList[0].bitsPerSample > 8;
	}

	myTrack->encodeBufferSize = myTrack->audioCodecList[0].bytesPerFrame *
		                        myTrack->audioCodecList[0].audioSampleRate;
	myTrack->encodeBuffer = malloc(myTrack->encodeBufferSize);			

	return myTrack;
}


QTTrack *
create_compressed_qt_audio_track(BMediaTrack *track, media_format *mf)
{
	QTTrack *myTrack = new QTTrack();

	myTrack->SetType(QT_AUDIO);			

	myTrack->audioCodecList = new audio_smp_details;
	memset(myTrack->audioCodecList, 0, sizeof(audio_smp_details));
	myTrack->audioCodecCount = 1;

	BMediaFormats				formatObject;
	media_format_description 	mfd;

	memset(&mfd, 0, sizeof(mfd));
	
	formatObject.GetCodeFor(*mf, B_QUICKTIME_FORMAT_FAMILY, &mfd);

	myTrack->audioCodecList[0].codecVendor = 'aapl';
	myTrack->audioCodecList[0].codecID = mfd.u.quicktime.codec;

	myTrack->audioCodecList[0].audioChannels   = mf->u.raw_audio.channel_count;
	myTrack->audioCodecList[0].audioSampleRate = (int32)mf->u.raw_audio.frame_rate;

	switch (mf->u.raw_audio.format) {
		case media_raw_audio_format::B_AUDIO_UCHAR:
			myTrack->audioCodecList[0].bitsPerSample = sizeof(uchar) * 8;
			break;
		case media_raw_audio_format::B_AUDIO_CHAR:
			myTrack->audioCodecList[0].bitsPerSample = sizeof(char) * 8;
			break;
		case media_raw_audio_format::B_AUDIO_SHORT:
			myTrack->audioCodecList[0].bitsPerSample = sizeof(short) * 8;
			break;
		default:
			// the data is compressed so the type doesn't really matter
			// default to INT so there's enough room in the buffer
			//FALLTHROUGH//
		case media_raw_audio_format::B_AUDIO_INT:
			myTrack->audioCodecList[0].bitsPerSample = sizeof(int) * 8;
			break;
	}

	myTrack->audioCodecList[0].bytesPerFrame =
		myTrack->audioCodecList[0].audioChannels *
		myTrack->audioCodecList[0].bitsPerSample / 8;

	myTrack->encodeBufferSize = myTrack->audioCodecList[0].bytesPerFrame *
		                        myTrack->audioCodecList[0].audioSampleRate;
	myTrack->encodeBuffer = malloc(myTrack->encodeBufferSize);			


	return myTrack;
}


status_t
QTWriter::AddTrack(BMediaTrack *track)
{
	media_format mf;
	QTTrack *qttrack;
	status_t err;

	if (f_qt == NULL)
		return B_ERROR;

	if(header_committed)
		return B_NOT_ALLOWED;

	if (f_numtracks > 2)    // XXXdbg we only support 1 audio and 1 video track
		return B_BAD_INDEX;

	track->EncodedFormat(&mf);

	err = accepts_format(&mf, B_MEDIA_REJECT_WILDCARDS);
	if(err != B_OK) {
		return err;
	}

	if (mf.type == B_MEDIA_RAW_VIDEO)
		qttrack = create_raw_qt_video_track(track, &mf);
	else if (mf.type == B_MEDIA_ENCODED_VIDEO)
		qttrack = create_compressed_qt_video_track(track, &mf);
	else if (mf.type == B_MEDIA_RAW_AUDIO)
		qttrack = create_raw_qt_audio_track(track, &mf);
	else if (mf.type == B_MEDIA_ENCODED_AUDIO)
		qttrack = create_compressed_qt_audio_track(track, &mf);
	else
		return B_BAD_TYPE;

	if (qttrack == NULL) {
		printf("QTWriter failed to create qttrack!\n");
		return B_BAD_VALUE;
	}
	
	f_qttracks[f_numtracks++] = qttrack;
	f_qt->BeginTrack(qttrack);

	return B_OK;
}

status_t
QTWriter::AddChunk(int32 type, const char *data, size_t size)
{
	return B_ERROR;
}

status_t
QTWriter::CommitHeader(void)
{
	if (f_qt == NULL)
		return B_ERROR;
	if(header_committed)
		return B_NOT_ALLOWED;

	/* nothing to do for quicktime */
	header_committed = true;

	return B_OK;
}
	
status_t
QTWriter::WriteData(int32 				tracknum,
					media_type 			type,
					const void 			*data,
					size_t 				size,
					media_encode_info	*info)
{
	status_t res = B_BAD_TYPE;
	QTTrack  *qttrack;
	
	if (f_qt == NULL)
		return B_ERROR;
	
	if (tracknum < 0 || tracknum >= f_numtracks)
		return B_BAD_INDEX;

	if(!header_committed)
		return B_NOT_ALLOWED;

	qttrack = f_qttracks[tracknum];

	if (qttrack->Type() == QT_VIDEO)
		res = f_qt->AddVideoFrame(data, qttrack, size, info);
	else if (qttrack->Type() == QT_AUDIO)
		res = f_qt->AddAudioFrames(data, qttrack, size);

	return res;
}


status_t
QTWriter::CloseFile(void)
{
	status_t err;
	if (f_qt == NULL)
		return B_ERROR;

	err = f_qt->CloseMovie();
	if (err == B_OK) err = f_source->Flush();
	delete f_source;
	f_source = NULL;

	if(!header_committed)
		err = B_ERROR;

	return err;
}

