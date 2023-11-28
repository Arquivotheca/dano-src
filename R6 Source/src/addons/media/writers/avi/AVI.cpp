#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <File.h>
#include <malloc.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <MediaFormats.h>
#include "MediaWriter.h"
#include "TRIFFWriter.h"
#include "AVI.h"

#include <FileWriter.h>

MediaWriter *
instantiate_mediawriter(void)
{
	return new AviWriter();
}

status_t
get_mediawriter_info(media_file_format *mfi)
{
	strcpy(mfi->mime_type,      "video/x-msvideo");
	strcpy(mfi->pretty_name,    "AVI File Format");
	strcpy(mfi->short_name,     "avi");
	strcpy(mfi->file_extension, "avi");

	mfi->capabilities = media_file_format::B_KNOWS_RAW_AUDIO |
		                media_file_format::B_KNOWS_RAW_VIDEO |
						media_file_format::B_KNOWS_ENCODED_AUDIO |
		                media_file_format::B_KNOWS_ENCODED_VIDEO |
						media_file_format::B_WRITABLE;
	mfi->family       = B_AVI_FORMAT_FAMILY;

	return B_OK;
}


status_t
accepts_format(media_format *fmt, uint32 flags)
{
	int reject_wildcards = flags & B_MEDIA_REJECT_WILDCARDS;

	if (fmt->IsVideo()) {
		if(fmt->type == B_MEDIA_RAW_VIDEO) {
			switch(fmt->u.raw_video.display.format) {
				case 0:	/* wildcard */
					if(reject_wildcards)
						return B_BAD_TYPE;
				case B_RGB32:
				case B_RGBA32:
				case B_RGB24:
				case B_RGB15:
				case B_RGBA15:
				case B_YCbCr422:
					break;
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

			if(fmt->u.raw_video.display.bytes_per_row < 1)
				if(reject_wildcards ||
					fmt->u.raw_video.display.bytes_per_row != media_video_display_info::wildcard.bytes_per_row)
				{
					return B_BAD_TYPE;
				}

			if(fmt->u.raw_video.field_rate < 0.01)
				if(reject_wildcards ||
					fmt->u.raw_video.field_rate != media_raw_video_format::wildcard.field_rate)
				{
					return B_BAD_TYPE;
				}
		}
		else if(fmt->type == B_MEDIA_ENCODED_VIDEO) {
			BMediaFormats formatObject;
			media_format_description fd;
			status_t err;

			err = formatObject.GetCodeFor(*fmt, B_AVI_FORMAT_FAMILY, &fd);
			if(err != B_OK) {
				return err;
			}
			if(fd.family != B_AVI_FORMAT_FAMILY) {
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
		}
	} else if (fmt->IsAudio()) {
		if(fmt->type == B_MEDIA_RAW_AUDIO) {
			// AVI PCM (raw) audio is treated thusly:
			// bits_per_sample <= 8, samples are unsigned
			// bits_per_sample > 8, samples are signed two's-complement

			switch(fmt->u.raw_audio.format) {
				case 0:	/* wildcard */
					if(reject_wildcards)
						return B_BAD_TYPE;
				case media_raw_audio_format::B_AUDIO_UCHAR:
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
		}
		else {
			// we don't support encoded audio yet
	//		return B_BAD_TYPE;
		}
	} else {
		return B_BAD_TYPE;
	}

	return B_OK;
}


AviWriter::AviWriter()
{
	//	Create RIFFWriter
	f_riff = NULL;
	f_vid  = NULL;
	f_aud  = NULL;
	f_out  = NULL;
	meta_data = NULL;
}

AviWriter::~AviWriter()
{
	if (f_riff)
		delete f_riff;
	f_riff = NULL;
	if (f_out)
		delete f_out;
	f_out = NULL;
	if (meta_data)
		free(meta_data);
	meta_data=NULL;
}


status_t
AviWriter::SetSource(BDataIO *source)
{
	//int fd;
	status_t err;

	BPositionIO *file = dynamic_cast<BPositionIO *>(source);
	if (file == NULL)
		return B_BAD_TYPE;

	//fd = file->Dup();
	//f_out = fdopen(fd, "w+");
	//

	f_out = new FileWriter(file);
	err = f_out->InitCheck();
	if(err != B_NO_ERROR) {
		delete f_out;
		f_out = NULL;
		return err;
	}
	header_committed = false;

	//f_out->SetBufferSize(128*1024);

	f_riff = new TRIFFWriter(f_out);

	return B_OK;
}

status_t 
AviWriter::SetWriteBufferSize(size_t buffersize)
{
	if(f_out == NULL)
		return B_ERROR;
	if(header_committed)
		return B_NOT_ALLOWED;

	return f_out->SetBufferSize(buffersize);
}

status_t
AviWriter::AddCopyright(const char *data)
{
	if(f_riff == NULL || header_committed)
		return B_NOT_ALLOWED;

	return f_riff->SetCopyright(data);
}


status_t
AviWriter::AddTrackInfo(int32 stream, uint32 code, const char* data, size_t size)
{
    if(header_committed)
        return B_NOT_ALLOWED;
	/* accept only stream-format data for stream 1 */
	if (code != 'strf' || stream != 1)
		return B_ERROR;
    /* Replace previous meta_data */
    if (meta_data != NULL)
        free(meta_data);
    /* Allocate a buffer and copy the block */
    meta_data = (char*)malloc(size);
    if (meta_data == NULL)
        return B_ERROR;
    meta_data_length = size;
    memcpy(meta_data, data, size);
    return B_OK;
}


status_t
AviWriter::AddTrack(BMediaTrack *track)
{
	media_format mf;
	status_t err;

	if(header_committed)
		return B_NOT_ALLOWED;

	track->EncodedFormat(&mf);

	err = accepts_format(&mf, B_MEDIA_REJECT_WILDCARDS);
	if(err != B_OK) {
		return err;
	}

	if (mf.IsVideo()) {
		if (f_vid != NULL)
			return B_BAD_INDEX;
		f_vid = track;
	} else if (mf.IsAudio()) {
		if (f_aud != NULL)
			return B_BAD_INDEX;
		f_aud = track;
	} else {
		return B_BAD_TYPE;
	}
	return B_OK;
}

status_t
AviWriter::AddChunk(int32 /*type*/, const char* /*data*/, size_t /*size*/)
{
	return B_ERROR;
}

status_t
AviWriter::CommitHeader(void)
{
	status_t err;
	int m_XSize=0, m_YSize=0, m_RowBytes=0;
	color_space m_Colorspace = B_NO_COLOR_SPACE;
	media_format vid_fmt;
	int32 encoding_fmt = 0;
	
	if (f_vid == NULL)
		return B_ERROR;
		
	if(header_committed)
		return B_NOT_ALLOWED;

	//	Set up initial AVI headers
	AVIHeader *aviHeader = f_riff->GetAVIHeader();
	
	aviHeader->MaximumDataRate		= 0;
	aviHeader->PaddingGranularity	= 0;
	aviHeader->Flags				= kAVIHasIndexFlag;
	aviHeader->TotalNumberOfFrames	= 0;	// gets updated later
	aviHeader->NumberOfInitialFrames= 0;
	
	int32 count = 0;
	
	if (f_vid)
		count++;
	if (f_aud)
		count++;
	
	if (f_vid) {
		f_vid->EncodedFormat(&vid_fmt);

		if (vid_fmt.type == B_MEDIA_ENCODED_VIDEO) {
			m_XSize 	 = vid_fmt.u.encoded_video.output.display.line_width;
			m_YSize 	 = vid_fmt.u.encoded_video.output.display.line_count;
			m_RowBytes 	 = m_XSize*3;
			m_Colorspace = B_RGB24;

			BMediaFormats				formatObject;
			media_format_description 	mfd;
			
			memset(&mfd, 0, sizeof(mfd));
		
			err = formatObject.GetCodeFor(vid_fmt, B_AVI_FORMAT_FAMILY, &mfd);
			if(err != B_NO_ERROR)
				return err;

			encoding_fmt	= mfd.u.avi.codec;

			aviHeader->TimeBetweenFrames = (uint32)(1000000.0 / (float)vid_fmt.u.encoded_video.output.field_rate);
			aviHeader->Height = m_YSize;
		} else if (vid_fmt.type == B_MEDIA_RAW_VIDEO) {
			m_XSize 	 = vid_fmt.u.raw_video.display.line_width;
			m_YSize 	 = vid_fmt.u.raw_video.display.line_count;
			m_RowBytes 	 = vid_fmt.u.raw_video.display.bytes_per_row;
			m_Colorspace = vid_fmt.u.raw_video.display.format;
			if(m_Colorspace == B_YCbCr422) {
				encoding_fmt = 'YUY2';
				aviHeader->Height = m_YSize;
			}
			else {
				encoding_fmt = 0;
				aviHeader->Height = -m_YSize;
			}
			aviHeader->TimeBetweenFrames = (uint32)(1000000.0 / (float)vid_fmt.u.raw_video.field_rate);
		}
	}

	aviHeader->NumberOfStreams		= count;	//	One video, one audio
	aviHeader->SuggestedBufferSize	= m_YSize * m_RowBytes;
	aviHeader->Width				= m_XSize;
	aviHeader->TimeScale			= 1000;
	aviHeader->DataRate				= (1000000 * 1000) / aviHeader->TimeBetweenFrames;
	aviHeader->StartTime			= 0;
	aviHeader->DataLength			= 0;	//	Number of video frames...	
	
	//	Set up number of streams
	bool retVal = f_riff->SetStreamCount(aviHeader->NumberOfStreams);
	if (!retVal) {
		delete f_riff;
		f_riff = NULL;
		return B_ERROR;
	}
	
	if (f_vid) {	
		//	Set up first stream header, in this case video
		AVIStreamHeader	*streamOneHeader 	= f_riff->GetStreamOneHeader();
		streamOneHeader->DataType			= kRiff_vids_Chunk;
		streamOneHeader->DataHandler		= encoding_fmt;
		streamOneHeader->Flags				= 0;
		streamOneHeader->Priority			= 0; //1;
		streamOneHeader->InitialFrames		= 0;
		streamOneHeader->TimeScale			= 1000;
		streamOneHeader->DataRate			= (1000000 * 1000) / aviHeader->TimeBetweenFrames;
		streamOneHeader->StartTime			= 0;
		streamOneHeader->DataLength			= 0;	// Come back and update this! # of frames for video
		streamOneHeader->SuggestedBufferSize= aviHeader->SuggestedBufferSize;
		streamOneHeader->Quality			= 0;
		streamOneHeader->SampleSize			= 0;
		streamOneHeader->rect[0]			= 0;		//left
		streamOneHeader->rect[1]			= -m_YSize;	//top
		streamOneHeader->rect[2]			= m_XSize;	//right
		streamOneHeader->rect[3]			= 0;		//bottom
			
		//	Setup VIDSHeader
		AVIVIDSHeader *vidsHeader 	= f_riff->GetVIDSHeader();

		memset(vidsHeader, 0, sizeof(AVIVIDSHeader));

		vidsHeader->Size			= sizeof(AVIVIDSHeader);
		vidsHeader->Width			= aviHeader->Width;
		vidsHeader->Height			= aviHeader->Height;
		vidsHeader->Planes			= 1;
		uint32 bpp;
		switch (m_Colorspace) {
			case B_RGB32:
			case B_RGBA32:
			case B_RGB32_BIG:
			case B_RGBA32_BIG:
				bpp = 4;
				break;

			case B_RGB16:
			case B_RGB15:
			case B_RGB16_BIG:
			case B_RGB15_BIG:
				bpp = 2;
				break;

			case B_CMAP8:
				bpp = 1;
				break;
				
			case B_YCbCr422:
				bpp = 2;
				break;

			case B_RGB24:
			case B_RGB24_BIG:
			case B_NO_COLOR_SPACE:  // if no colorspace was specified...
				bpp = 3;
				break;
				
			default:
				return B_ERROR;
		}

		vidsHeader->BitCount		= bpp * 8;
		vidsHeader->Compression		= encoding_fmt;
		vidsHeader->ImageSize		= m_XSize * m_YSize * bpp;
		vidsHeader->XPelsPerMeter	= 0;
		vidsHeader->YPelsPerMeter	= 0;
		vidsHeader->NumColors		= 0;
		vidsHeader->ImpColors		= 0;
	}
	
	if(f_aud) {
		media_format aud_fmt;

		f_aud->EncodedFormat(&aud_fmt);

		//	Set up second stream header, in this case audio
		AVIStreamHeader	*streamTwoHeader 	= f_riff->GetStreamTwoHeader();
		streamTwoHeader->rect[0]			= 0;
		streamTwoHeader->rect[1]			= 0;
		streamTwoHeader->rect[2]			= 0;
		streamTwoHeader->rect[3]			= 0;
		streamTwoHeader->DataType			= kRiff_auds_Chunk;
		streamTwoHeader->DataHandler		= 0; // 0 == Uncompressed audio, XXXdbg ADPCM 
		streamTwoHeader->Flags				= 0;
		streamTwoHeader->Priority			= 0;//2;
		streamTwoHeader->InitialFrames		= 0;
		streamTwoHeader->TimeScale			= 1;
		streamTwoHeader->DataRate			= (uint32)aud_fmt.u.raw_audio.frame_rate;
		streamTwoHeader->StartTime			= 0;
		streamTwoHeader->DataLength			= 0;	// Come back and update this!
		streamTwoHeader->SuggestedBufferSize= 44100;
		streamTwoHeader->Quality			= 0;
		
		//	Setup AUDSHeader
		AVIAUDSHeader *audsHeader 	= f_riff->GetAUDSHeader();						

		if(meta_data && meta_data_length <= sizeof(AVIAUDSHeader))
		{
			memcpy(audsHeader, meta_data, sizeof(AVIAUDSHeader));
		}
		else 
		{
			memset(audsHeader, 0, sizeof(AVIAUDSHeader));

			if(aud_fmt.type == B_MEDIA_RAW_AUDIO)
			{
				audsHeader->BitsPerSample	= (aud_fmt.u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK) * 8;
				audsHeader->Format			= WAVE_FORMAT_PCM;
				audsHeader->Channels		= aud_fmt.u.raw_audio.channel_count;
				audsHeader->SamplesPerSec	= (uint32)aud_fmt.u.raw_audio.frame_rate;
				audsHeader->BlockAlign      = audsHeader->Channels * (audsHeader->BitsPerSample / 8);
				audsHeader->AvgBytesPerSec	= audsHeader->BlockAlign * audsHeader->SamplesPerSec;
				streamTwoHeader->SampleSize = audsHeader->BlockAlign;
				streamTwoHeader->Quality	= 10000;
			}
			else if(aud_fmt.type == B_MEDIA_ENCODED_AUDIO)
			{
				BMediaFormats				formatObject;
				media_format_description	fd;
	
				memset(&fd, 0, sizeof(fd));
				if(B_OK!=formatObject.GetCodeFor(aud_fmt, B_AVI_FORMAT_FAMILY, &fd))
				{
					printf("AAARGH!! no code for format\n");
					return B_ERROR;
				}
	
				audsHeader->BitsPerSample	= 0;
				audsHeader->Format			= fd.u.avi.codec & 0xffff;
				audsHeader->Channels		= aud_fmt.u.encoded_audio.output.channel_count;
				audsHeader->SamplesPerSec	= (uint32)aud_fmt.u.encoded_audio.output.frame_rate;
				audsHeader->AvgBytesPerSec	= int32(aud_fmt.u.encoded_audio.bit_rate / 8);
	
				audsHeader->BlockAlign      = 1;
				streamTwoHeader->SampleSize = audsHeader->BlockAlign;
	
				if(audsHeader->Format == WAVE_FORMAT_ADPCM)
				{
					audsHeader->BitsPerSample	= 4;
					audsHeader->AvgBytesPerSec	= audsHeader->Channels*audsHeader->SamplesPerSec*audsHeader->BitsPerSample/8;
					audsHeader->ExtensionSize	= 12;	//	Size of extended data
					audsHeader->SamplesPerBlock	= 0;	// 	S Used by MSADPCM and Intel DVI
					audsHeader->NumCoefficients	= 0;	// 	S Used by MSADPCM  num of follow sets
					memset(audsHeader->Coefficients,0,28);
					audsHeader->Style			= 0;  	// 	- SIGN2 or unsigned
					audsHeader->ByteCount		= 0;	// 	used to keep track of length
				}
			}
		}
	}	
	
	//	Now write header info out
	if (f_riff->InitAVIFile(vid_fmt.type == B_MEDIA_RAW_VIDEO) == false) {
		delete f_riff;
		f_riff = NULL;
		return B_ERROR;
	}
	header_committed = true;

	return B_OK;
}
	
status_t
AviWriter::WriteData(int32 				tracknum,
					 media_type 		/*type*/,
					 const void 		*data,
					 size_t 			size,
					 media_encode_info	*info)
{
	status_t retval;
	if(!header_committed)
		return B_NOT_ALLOWED;
		
	if (tracknum == 0) {
		retval = f_riff->Write00DCChunk(kRiff_00dc_Chunk, size,
		                                (void *)data, info);
	} else if (tracknum == 1) {
		retval = f_riff->Write00WBChunk('01wb', size, (void *)data, info);
	} else {
		return B_BAD_INDEX;
	}

	return retval;
}


status_t
AviWriter::CloseFile(void)
{
	status_t retval = B_NO_ERROR;

	if (!f_riff || !f_out || !header_committed || f_riff->CompleteAVIFile() != true)
		return B_ERROR;

	delete f_riff;
	f_riff = NULL;
	retval = f_out->Flush();
	delete f_out;
	f_out = NULL;
	
	return retval;
}
