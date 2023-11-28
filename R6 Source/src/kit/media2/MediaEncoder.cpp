#include <media2/MediaEncoder.h>
#include "codec_addons.h"
#include "Encoder.h"

namespace B {
namespace Media2 {

using namespace Private;
using namespace Media2Private;

BMediaEncoder::BMediaEncoder()
{
	Init();
}


BMediaEncoder::BMediaEncoder(const media_format *output_format)
{
	Init();
	if(fInitStatus == B_NO_ERROR)
		fInitStatus = SetTo(output_format);
}


BMediaEncoder::BMediaEncoder(const media_codec_info *mci)
{
	Init();
	if(fInitStatus == B_NO_ERROR)
		fInitStatus = SetTo(mci);
}

void 
BMediaEncoder::Init()
{
	fEncoder = NULL;
	fFormatValid = false;
	fEncoderStarted = false;
	fEncoderMgr = __get_encoder_manager();
	if(fEncoderMgr == NULL)
		fInitStatus = B_ERROR;
	else
		fInitStatus = B_NO_ERROR;
}

status_t 
BMediaEncoder::InitCheck() const
{
	return fInitStatus;
}


BMediaEncoder::~BMediaEncoder()
{
	ReleaseEncoder();
}

void 
BMediaEncoder::ReleaseEncoder()
{
	if(fEncoder) {
		delete fEncoder;
		fEncoder = NULL;
		fEncoderMgr->ReleaseAddon(fEncoderID);
	}
	fFormatValid = false;
	fEncoderStarted = false;
}

status_t 
BMediaEncoder::SetTo(const media_format *output_format)
{
	status_t err;
	media_codec_info ei;
	int32 cookie = 0;

	err = get_next_encoder(&cookie, NULL, NULL, output_format, &ei, NULL, NULL);
	if(err != B_NO_ERROR)
		return err;
	return SetTo(&ei);
}

status_t 
BMediaEncoder::SetTo(const media_codec_info *mci)
{
	status_t err;
	image_id imgid;
	ReleaseEncoder();
	
	if(fEncoderMgr == NULL)
		return B_NO_INIT;

	fEncoderID = mci->id;
	imgid = fEncoderMgr->GetAddonAt(fEncoderID);
	if(imgid < 0)
		return imgid;

	if(mci->sub_id == 0) {
		Encoder *(*func)(void);

		err = get_image_symbol(imgid, "instantiate_encoder",
		                       B_SYMBOL_TYPE_TEXT, (void **)&func);
		if(err != B_OK) {
			fEncoderMgr->ReleaseAddon(fEncoderID);
			return err;
		}
		
		fEncoder = func();
	}
	else {
		Encoder     *(*nth_func)(int32);
		
		err = get_image_symbol(imgid, "instantiate_nth_encoder",
		                       B_SYMBOL_TYPE_TEXT, (void **)&nth_func);

		if (err != B_OK) {
			fEncoderMgr->ReleaseAddon(fEncoderID);
			return err;
		}
		fEncoder = nth_func(mci->sub_id - 1); 
	}

	if(fEncoder == NULL) {
		fEncoderMgr->ReleaseAddon(fEncoderID);
		return err;
	}
	fEncoder->fWriteChunk = write_chunk;
	fEncoder->fUserData = this;
	return B_NO_ERROR;
}

status_t 
BMediaEncoder::SetFormat(media_format *in_format, media_format *out_format,
                         media_file_format *mfi)
{
	status_t err;
	media_file_format tmp_mfi;
	if(fEncoder == NULL)
		return B_NO_INIT;
	if(mfi == NULL)
		mfi = &tmp_mfi;
	err = fEncoder->SetFormat(mfi, in_format, out_format);
	fFormatValid = (err == B_NO_ERROR);
	return err;
}

status_t 
BMediaEncoder::Encode(const void *buffer, int64 frameCount,
                      media_encode_info *info)
{
	status_t err;
	if(!fFormatValid)
		return B_NO_INIT;
	if(!fEncoderStarted) {
		err = fEncoder->StartEncoder();
		if(err != B_NO_ERROR)
			return err;
		fEncoderStarted = true;
	}
	return fEncoder->Encode(buffer, frameCount, info);
}

status_t 
BMediaEncoder::GetEncodeParameters(encode_parameters *parameters) const
{
	return fEncoder->GetEncodeParameters(parameters);
}

status_t 
BMediaEncoder::SetEncodeParameters(encode_parameters *parameters)
{
	return fEncoder->SetEncodeParameters(parameters);
}

status_t 
BMediaEncoder::write_chunk(void *classptr, const void *chunk_data,
                           size_t chunk_len, media_encode_info *info)
{
	BMediaEncoder *encoder = (BMediaEncoder *)classptr;
	if(encoder == NULL)
		return B_ERROR;
	return encoder->WriteChunk(chunk_data, chunk_len, info);
}

status_t 
BMediaEncoder::AddTrackInfo(uint32, const char *, size_t)
{
	return B_ERROR;
}


BMediaBufferEncoder::BMediaBufferEncoder()
{
	fBuffer = NULL;
}


BMediaBufferEncoder::BMediaBufferEncoder(const media_format *output_format)
	: BMediaEncoder(output_format)
{
	fBuffer = NULL;
}


BMediaBufferEncoder::BMediaBufferEncoder(const media_codec_info *mci)
	: BMediaEncoder(mci)
{
	fBuffer = NULL;
}

status_t 
BMediaBufferEncoder::EncodeToBuffer(void *output_buffer,
                                    size_t *output_size, const void *in_buffer,
                                    int64 in_frameCount,
                                    media_encode_info *info)
{
	status_t err;
	fBuffer = output_buffer;
	fBufferSize = *output_size;
	err = Encode(in_buffer, in_frameCount, info);
	if(fBuffer) {
		fBuffer = NULL;
		*output_size = 0;
	}
	else {
		*output_size = fBufferSize;
	}
	return err;
}

status_t 
BMediaBufferEncoder::WriteChunk(const void *chunkData, size_t chunkLen,
                                media_encode_info *)
{
	if(fBuffer == NULL)
		return B_ENTRY_NOT_FOUND;
	if(chunkLen > (size_t)fBufferSize) {
		memcpy(fBuffer, chunkData, fBufferSize);
		fBuffer = NULL;
		return B_DEVICE_FULL;
	}
	memcpy(fBuffer, chunkData, chunkLen);
	fBufferSize = chunkLen;
	fBuffer = NULL;
	return B_NO_ERROR;
}


status_t BMediaEncoder::_Reserved_BMediaEncoder_0(int32, ...) {return B_ERROR;}
status_t BMediaEncoder::_Reserved_BMediaEncoder_1(int32, ...) {return B_ERROR;}
status_t BMediaEncoder::_Reserved_BMediaEncoder_2(int32, ...) {return B_ERROR;}
status_t BMediaEncoder::_Reserved_BMediaEncoder_3(int32, ...) {return B_ERROR;}
status_t BMediaEncoder::_Reserved_BMediaEncoder_4(int32, ...) {return B_ERROR;}
status_t BMediaEncoder::_Reserved_BMediaEncoder_5(int32, ...) {return B_ERROR;}
status_t BMediaEncoder::_Reserved_BMediaEncoder_6(int32, ...) {return B_ERROR;}
status_t BMediaEncoder::_Reserved_BMediaEncoder_7(int32, ...) {return B_ERROR;}
status_t BMediaEncoder::_Reserved_BMediaEncoder_8(int32, ...) {return B_ERROR;}
status_t BMediaEncoder::_Reserved_BMediaEncoder_9(int32, ...) {return B_ERROR;}
status_t BMediaEncoder::_Reserved_BMediaEncoder_10(int32, ...) {return B_ERROR;}
status_t BMediaEncoder::_Reserved_BMediaEncoder_11(int32, ...) {return B_ERROR;}
status_t BMediaEncoder::_Reserved_BMediaEncoder_12(int32, ...) {return B_ERROR;}
status_t BMediaEncoder::_Reserved_BMediaEncoder_13(int32, ...) {return B_ERROR;}
status_t BMediaEncoder::_Reserved_BMediaEncoder_14(int32, ...) {return B_ERROR;}
status_t BMediaEncoder::_Reserved_BMediaEncoder_15(int32, ...) {return B_ERROR;}

} } // B::Media2
