#include <stdio.h>

#include <media2/MediaFile.h>
#include <media2/MediaTrack.h>

#include <support2/StdIO.h>
#define checkpoint \
berr << "thid " << find_thread(0) << " -- " << __FILE__ << ":" << __LINE__ << " -- " << __FUNCTION__ << endl;

#include "Decoder.h"
#include "Extractor.h"
#include "Detractor.h"
#include "Encoder.h"
#include "MediaWriter.h"

#include "codec_addons.h"

#define FPrintf fprintf

namespace B {
namespace Media2 {

using namespace Private;
using namespace Media2Private;

//#define FUNCTION(x) printf x
#define FUNCTION(X)


Decoder *
BMediaTrack::find_decoder(BMediaTrack *track, int32 *id)
{
	_AddonManager *mgr = __get_decoder_manager();
	image_id       imgid;
	Decoder     *(*make_decoder)(void);
	Decoder		  *decoder = NULL;
	int32          cookie=0;

	media_format trackFormat;
	void * ptr;
	int32 size;
	track->TrackInfo(&trackFormat, &ptr, &size);
	addon_list addons;
	BMediaFormats::find_addons(&trackFormat, addons);
	addon_list * addp = &addons;

#if DEBUG
	if (addons.size() == 0) {
		FPrintf(stderr, "no add-ons found for format\n");
	}
//	for (addon_list::iterator ptr(addons.begin()); ptr != addons.end(); ptr++) {
//		FPrintf(stderr, "format add-on %s\n", (*ptr).c_str());
//	}
#endif

	if (addons.size() == 0) {
//		addp = 0;
		return 0;
	}
//scan_again:
	while ((imgid = mgr->GetNextAddon(&cookie, id, addp)) > 0) {

		if (get_image_symbol(imgid, "instantiate_decoder",
							 B_SYMBOL_TYPE_TEXT,
							 (void **)&make_decoder) != B_OK) {
			mgr->ReleaseAddon(*id);
			continue;
		}
		
		decoder = make_decoder();
		if (decoder == NULL) {
			mgr->ReleaseAddon(*id);
			continue;
		}

		if (decoder->SetTrack(track) == B_OK) {  // got it!
			break;
		}

		delete decoder;
		mgr->ReleaseAddon(*id);
		decoder = NULL;
	}
//	if (addp != 0 && decoder == 0) {
//		addp = 0;
//		cookie = 0;
//		goto scan_again;
//	}	
	return decoder;
}

extern "C" {
status_t ReadFrames__11BMediaTrackPcPxP12media_header(BMediaTrack * mt, void * buf, int64 * cnt, media_header * hdr)
{
	return mt->ReadFrames(buf, cnt, hdr);
}
status_t ReplaceFrames__11BMediaTrackPcPxPC12media_header(BMediaTrack * mt, const void * buf, int64 * cnt, media_header * hdr)
{
	return mt->ReplaceFrames(buf, cnt, hdr);
}
}

BMediaTrack::BMediaTrack(MediaExtractor *in_extractor, int32 in_stream)
{
	fWriter    = NULL;
	fEncoder   = NULL;
	fDecoder   = NULL;
	fDecoderID = -1;
	fEncoderID = -1;
	fEncoderStarted = false;
	fWriterFormat.type = B_MEDIA_UNKNOWN_TYPE;

	fExtractor = in_extractor;
	fDetractor = NULL;
	fStream    = in_stream;
	fCurFrame  = 0;
	fCurTime   = 0;
	fErr       = B_OK;
	fExtractorCookie = NULL;
	
	if(fExtractor == NULL) {
		fErr = B_NO_INIT;
		return;
	}
		
	fErr = fExtractor->AllocateCookie(in_stream, &fExtractorCookie);
	if(fErr != B_NO_ERROR)
		return;

	fDecoder = find_decoder(this, &fDecoderID);
}

BMediaTrack::BMediaTrack(Detractor *in_detractor, int32 in_stream)
{
	FUNCTION(("BMediaTrack::BMediaTrack(Detractor *in_detractor, int32 in_stream)\n"));
	fWriter    = NULL;
	fEncoder   = NULL;
	fDecoder   = NULL;
	fDecoderID = -1;
	fEncoderID = -1;
	fEncoderStarted = false;
	fWriterFormat.type = B_MEDIA_UNKNOWN_TYPE;

	fExtractor = NULL;
	fDetractor = in_detractor;
	fStream    = in_stream;
	fCurFrame  = 0;
	fCurTime   = 0;
	fErr       = B_OK;
	fExtractorCookie = NULL;
	
	if(fDetractor == NULL) {
		fErr = B_NO_INIT;
		return;
	}
}

BMediaTrack::BMediaTrack(B::Private::MediaWriter *writer,
						 int32 stream_num,
						 media_format *fmt,
						 Encoder *encoder,
						 media_codec_info *mci)
{
	fStream    = 0;
	fCurFrame  = 0;
	fCurTime   = 0;
	fExtractor = NULL;
	fDetractor = NULL;
	fDecoder   = NULL;

	if (writer == NULL || fmt == NULL || stream_num < 0) {
		fErr = B_BAD_VALUE;
		return;
	}

	fWriterFormat  = *fmt;
	fWriter        = writer;
	fStream        = stream_num;
	fEncoder       = encoder;
	if (mci) {
		fEncoderID = mci->id;
		fMCI 	   = *mci;
	} else {
		fEncoderID = -1;
		memset(&fMCI, 0, sizeof(fMCI));
	}
	fEncoderStarted = false;
	fErr           = B_OK;

	if (fEncoder)
		fEncoder->SetTrack(this);
}


BMediaTrack::~BMediaTrack()
{
	if(fExtractor)
		fExtractor->FreeCookie(fStream, fExtractorCookie);

	if (fDecoder) {
		_AddonManager *mgr = __get_decoder_manager();

		delete fDecoder;
		fDecoder = NULL;
		mgr->ReleaseAddon(fDecoderID);
		fDecoderID = -1;
	}
	
	if (fEncoder) {
		_AddonManager *mgr = __get_encoder_manager();

		delete fEncoder;
		fEncoder = NULL;
		mgr->ReleaseAddon(fEncoderID);
		fEncoderID = -1;
	}
}

status_t
BMediaTrack::InitCheck() const
{
	FUNCTION(("BMediaTrack::InitCheck() const\n"));
	return fErr;
}


status_t
BMediaTrack::GetCodecInfo(media_codec_info *mci) const
{
	FUNCTION(("BMediaTrack::GetCodecInfo(media_codec_info *mci) const\n"));
	if (fDecoder)
		fDecoder->GetCodecInfo(mci);
	else if (fDetractor)
		fDetractor->GetCodecInfo(fStream, mci);
	else if (fEncoder)
		*mci = fMCI;
	else
		return B_BAD_TYPE;

	return B_OK;
}


status_t
BMediaTrack::ReadFrames(void *out_buffer, int64 *out_frameCount,
						media_header *mh)
{
//	FUNCTION(("BMediaTrack::ReadFrames(void *out_buffer, int64 *out_frameCount, media_header *mh)\n"));
	return ReadFrames(out_buffer, out_frameCount, mh, NULL);
}

status_t
BMediaTrack::ReadFrames(void *out_buffer, int64 *out_frameCount,
						media_header *mh, media_decode_info *info)
{
//	FUNCTION(("BMediaTrack::ReadFrames(void *out_buffer, int64 *out_frameCount, media_header *mh, media_decode_info *info)\n"));
	status_t	err;
	int64		framePos, frameCount;
	media_header mymh;

	if (!fDecoder && !fDetractor)
		return EPERM;

	if (mh == NULL)
		mh = &mymh;

	mh->type = B_MEDIA_UNKNOWN_TYPE;

	framePos = fCurFrame;
	if (fDecoder)
		err = fDecoder->Decode(out_buffer, out_frameCount, mh, info);
	else
		err = fDetractor->ReadFrames(fStream,out_buffer, out_frameCount, mh);
	if (err)
		return err;

	frameCount = *out_frameCount;
	fCurFrame += frameCount;
	fCurTime   = mh->start_time;

	return B_OK;
}

status_t
BMediaTrack::ReplaceFrames(const void *, int64 *, const media_header *)
{
	return B_UNSUPPORTED;
}

int64
BMediaTrack::CurrentFrame() const
{
	return fCurFrame;
}

bigtime_t
BMediaTrack::CurrentTime() const
{
	return fCurTime;
}

status_t
BMediaTrack::SeekToFrame(int64 *inout_frame, int32 flags)
{
	FUNCTION(("BMediaTrack::SeekToFrame(int64 *inout_frame, int32 flags)\n"));
	status_t	err;
	bigtime_t	time;
	int64		old_frame;

	if (!fExtractor && !fDetractor) return EPERM;

	old_frame = *inout_frame;
	if(fExtractor)
		err = fExtractor->SeekTo(fStream, fExtractorCookie, B_SEEK_BY_FRAME, &time, inout_frame, flags);
	else
		err = fDetractor->SeekTo(fStream, B_SEEK_BY_FRAME, &time, inout_frame, flags);

	if (err)
		return err;
	if (fDecoder) {
		err = fDecoder->Reset(B_SEEK_BY_FRAME, old_frame, inout_frame, time, &time);
		if (err)
			return err;
	}
	fCurFrame = *inout_frame;
	fCurTime = time;
	return B_OK;
}

status_t
BMediaTrack::SeekToTime(bigtime_t *inout_time, int32 flags)
{
	FUNCTION(("BMediaTrack::SeekToTime(bigtime_t *inout_time, int32 flags)\n"));
	status_t	err;
	int64		frame;
	bigtime_t	old_time;

	if (!fExtractor && !fDetractor) return EPERM;

	old_time = *inout_time;
	if(fExtractor)
		err = fExtractor->SeekTo(fStream, fExtractorCookie, B_SEEK_BY_TIME, inout_time, &frame, flags);
	else
		err = fDetractor->SeekTo(fStream, B_SEEK_BY_TIME, inout_time, &frame, flags);

	if (err)
		return err;
	if (fDecoder) {
		err = fDecoder->Reset(B_SEEK_BY_TIME, frame, &frame, old_time,
							  inout_time);
		if (err)
			return err;
	}
	fCurTime = *inout_time;
	fCurFrame = frame;
	return B_OK;
}

status_t 
BMediaTrack::FindKeyFrameForTime(bigtime_t *inout_time, int32 flags) const
{
	int64 frame;
	
	if (!fExtractor) return EPERM;

	return fExtractor->FindKeyFrame(fStream, fExtractorCookie, B_SEEK_BY_TIME,
	                                inout_time, &frame, flags);
}

status_t 
BMediaTrack::FindKeyFrameForFrame(int64 *inout_frame, int32 flags) const
{
	bigtime_t time;
	
	if (!fExtractor) return EPERM;

	return fExtractor->FindKeyFrame(fStream, fExtractorCookie, B_SEEK_BY_FRAME,
	                                &time, inout_frame, flags);
}

status_t
BMediaTrack::ReadChunk(char **out_buffer, int32 *out_size, media_header *mh)
{
//	FUNCTION(("BMediaTrack::ReadChunk(char **out_buffer, int32 *out_size, media_header *mh)\n"));
	if (!fExtractor) return EPERM;
	return fExtractor->GetNextChunk(fStream, fExtractorCookie,
	                                out_buffer, out_size, mh);
}

status_t
BMediaTrack::TrackInfo(media_format *out_format, void **out_info, int32 *out_infoSize)
{
	FUNCTION(("BMediaTrack::TrackInfo(media_format *out_format, void **out_info, int32 *out_infoSize)\n"));
	if (!fExtractor) return EPERM;
	return fExtractor->TrackInfo(fStream, out_format, out_info, out_infoSize);
}

status_t
BMediaTrack::EncodedFormat(media_format *out_format) const
{
	FUNCTION(("BMediaTrack::EncodedFormat(media_format *out_format) const\n"));
	void		*info;
	int32		size;

	if (fExtractor) {
		return fExtractor->TrackInfo(fStream, out_format, &info, &size);
	} else if (fDetractor) {
		return fDetractor->EncodedFormat(fStream, out_format);
	} else if (fWriterFormat.type != B_MEDIA_UNKNOWN_TYPE) {
		*out_format = fWriterFormat;
		return B_OK;
	}

	return B_BAD_TYPE;
}

status_t
BMediaTrack::DecodedFormat(media_format *inout_format)
{
	FUNCTION(("BMediaTrack::DecodedFormat(media_format *inout_format)\n"));
	media_format mf;

	if (fDetractor)
		return fDetractor->DecodedFormat(fStream,inout_format);

	if (!fDecoder)
		return EPERM;

	return fDecoder->Format(inout_format);
}

int64
BMediaTrack::CountFrames() const
{
	FUNCTION(("BMediaTrack::CountFrames() const\n"));
	status_t	err;
	int64		frames;

	if (fDetractor)
		return fDetractor->CountFrames(fStream);

	if (!fExtractor) return EPERM;

	err = fExtractor->CountFrames(fStream, &frames);
	if (err)
		return err;
	return frames;
}

bigtime_t
BMediaTrack::Duration() const
{
	FUNCTION(("BMediaTrack::Duration() const\n"));
	status_t	err;
	bigtime_t	duration;

	if (fDetractor)
		return fDetractor->Duration(fStream);

	if (!fExtractor) return EPERM;

	err = fExtractor->GetDuration(fStream, &duration);
	if (err)
		return err;
	return duration;
}


status_t
BMediaTrack::AddCopyright(const char *data)
{
	if (!fWriter)
		return EPERM;

	return fWriter->AddCopyright(data);
}


status_t
BMediaTrack::AddTrackInfo(uint32 code, const void *data, size_t size, uint32)
{
	if (!fWriter)
		return EPERM;

	return fWriter->AddTrackInfo(fStream, code, (const char *)data, size);
}

status_t
BMediaTrack::WriteFrames(const void *data, int32 num_frames, int32 flags)
{
	status_t err;
	media_encode_info bufferinfo;
	bufferinfo.flags = flags;
	float rate = 0;
	switch(fWriterFormat.type) {
		case B_MEDIA_RAW_AUDIO:
			rate = fWriterFormat.u.raw_audio.frame_rate;
			break;
		case B_MEDIA_ENCODED_AUDIO:
			rate = fWriterFormat.u.encoded_audio.output.frame_rate;
			break;
		case B_MEDIA_RAW_VIDEO:
			rate = fWriterFormat.u.raw_video.field_rate;
			break;
		case B_MEDIA_ENCODED_VIDEO:
			rate = fWriterFormat.u.encoded_video.output.field_rate;
			break;
		default:
			break;
	}
	if(rate > 0) {
		bufferinfo.start_time = (bigtime_t)(fCurFrame * 1000000 / rate);
	}
		
	err = WriteFrames(data, num_frames, &bufferinfo);
	
	return err;
}

status_t
BMediaTrack::WriteFrames(const void *data, int64 num_frames,
                         media_encode_info *info)
{
	status_t res;
	
	if (!fWriter)
		return EPERM;

	if (fEncoder) {
		if(!fEncoderStarted) {
			res = fEncoder->StartEncoder();
			if(res < B_OK)
				return res;
			fEncoderStarted = true;
		}

		res = fEncoder->Encode(data, num_frames, info);
	} else {
		res = fWriter->WriteData(fStream, fWriterFormat.type,
								 data, num_frames, info);
		fCurFrame += num_frames;
	}

	fCurTime = info->start_time;

	return res;
}


status_t
BMediaTrack::Flush()
{
	if (fEncoder) {
		// maintain old behavior (always call StartEncoder() before Flush())
		if (!fEncoderStarted)
			fEncoder->StartEncoder();
		return fEncoder->Flush();
	}
	return B_OK;
}

status_t
BMediaTrack::WriteChunk(const void *data, size_t size, uint32 flags)
{
	media_encode_info bufferinfo;
	float rate = 0;

	switch(fWriterFormat.type) {
		case B_MEDIA_RAW_AUDIO:
			rate = fWriterFormat.u.raw_audio.frame_rate;
			break;
		case B_MEDIA_ENCODED_AUDIO:
			rate = fWriterFormat.u.encoded_audio.output.frame_rate;
			break;
		case B_MEDIA_RAW_VIDEO:
			rate = fWriterFormat.u.raw_video.field_rate;
			break;
		case B_MEDIA_ENCODED_VIDEO:
			rate = fWriterFormat.u.encoded_video.output.field_rate;
			break;
		default:
			break;
	}
	if(rate > 0) {
		bufferinfo.start_time = (bigtime_t)(fCurFrame * 1000000 / rate);
	}
	bufferinfo.flags = flags;
	return WriteChunk(data, size, &bufferinfo);
}

status_t
BMediaTrack::WriteChunk(const void *data, size_t size, media_encode_info *info)
{
	status_t err;
	
	if (fWriter == 0) return EPERM;

	err = fWriter->WriteData(fStream, fWriterFormat.type, data, size, info);
	if (err < B_OK)
		return err;
		
	fCurFrame += 1;
	fCurTime = info->start_time;

	return B_OK;
}

#if 0
status_t 
BMediaTrack::GetParameterWeb(BParameterWeb **outWeb)
{
	if (fEncoder)
	{
		*outWeb = fEncoder->Web();
		return B_OK;
	}
	else
		return B_BAD_VALUE;	
}

BParameterWeb *
BMediaTrack::Web()
{
	BParameterWeb* w;
	return (GetParameterWeb(&w) == B_OK) ? w : 0;
}


status_t
BMediaTrack::GetParameterValue(int32 id, void *value, size_t *size)
{
	if (fEncoder)
		return fEncoder->GetParameterValue(id, value, size);

	return EPERM;
}


status_t
BMediaTrack::SetParameterValue(int32 id, const void *value, size_t size)
{
	if (fEncoder)
		return fEncoder->SetParameterValue(id, value, size);
	return EPERM;
}


BView *
BMediaTrack::GetParameterView()
{
	if (fEncoder)
		return fEncoder->GetParameterView();

	return NULL;
}
#endif

status_t
BMediaTrack::GetQuality(float *quality)
{
	if (fEncoder) {
		status_t err;
		encode_parameters parm;
		err = fEncoder->GetEncodeParameters(&parm);
		if(err != B_NO_ERROR)
			return err;
		*quality = parm.quality;
		return B_NO_ERROR;
	}
	return B_ERROR;
}


status_t
BMediaTrack::SetQuality(float quality)
{
	if (fEncoder) {
		status_t err;
		encode_parameters parm;
		err = fEncoder->GetEncodeParameters(&parm);
		if(err != B_NO_ERROR)
			return err;
		parm.quality = quality;
		return fEncoder->SetEncodeParameters(&parm);
	}
	return B_ERROR;
}


status_t
BMediaTrack::GetEncodeParameters(encode_parameters *parameters) const
{
	if (fEncoder) {
		return fEncoder->GetEncodeParameters(parameters);
	}
	return B_ERROR;
}


status_t
BMediaTrack::SetEncodeParameters(encode_parameters *parameters)
{
	if (fEncoder) {
		return fEncoder->SetEncodeParameters(parameters);
	}
	return B_ERROR;
}



status_t BMediaTrack::Perform(int32, void *)
{
	return B_ERROR;
}

status_t BMediaTrack::CommitHeader(void)
{
	if (fEncoder) {
		return fEncoder->CommitHeader();
	}
	return B_ERROR;
}


status_t BMediaTrack::ControlCodec(int32 selector, void * io_data, size_t size)
{
	if (fDecoder)
	{
		return fDecoder->ControlCodec(selector, io_data, size);
	}
	else if (fDetractor)
	{
		return fDetractor->ControlCodec(fStream, selector, io_data, size);
	}
	else if (fEncoder)
	{
		return fEncoder->ControlCodec(selector, io_data, size);
	}
	return EBADF;
}


status_t BMediaTrack::_Reserved_BMediaTrack_0(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_1(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_2(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_3(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_4(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_5(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_6(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_7(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_8(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_9(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_10(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_11(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_12(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_13(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_14(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_15(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_16(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_17(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_18(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_19(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_20(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_21(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_22(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_23(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_24(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_25(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_26(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_27(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_28(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_29(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_30(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_31(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_32(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_33(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_34(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_35(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_36(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_37(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_38(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_39(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_40(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_41(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_42(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_43(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_44(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_45(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_46(int32, ...) { return B_ERROR; }
status_t BMediaTrack::_Reserved_BMediaTrack_47(int32, ...) { return B_ERROR; }

} } // B::Media2
