#include <MediaDecoder.h>
#include <addons.h>
#include <Decoder.h>
#include <Extractor.h>

#include <stdio.h>

using namespace BPrivate;

status_t
BMediaDecoder::next_chunk(void *classptr, void **chunkData, size_t *chunkLen, media_header *mh)
{
	BMediaDecoder *decoder = (BMediaDecoder *)classptr;
	if(decoder == NULL)
		return B_ERROR;
	else
		return decoder->GetNextChunk((const void**)chunkData, chunkLen, mh);
}

BMediaDecoder::BMediaDecoder()
	: fDecoder(NULL)
{
	fInitStatus = B_NO_ERROR;
}

BMediaDecoder::BMediaDecoder(const media_format *in_format,
                             const void *info, size_t info_size)
	: fDecoder(NULL)
{
	fInitStatus = SetTo(in_format, info, info_size);
}


BMediaDecoder::BMediaDecoder(const media_codec_info *mci)
	: fDecoder(NULL)
{
	fInitStatus = SetTo(mci);
}

status_t 
BMediaDecoder::InitCheck() const
{
	return fInitStatus;
}


BMediaDecoder::~BMediaDecoder()
{
	ReleaseDecoder();
}

void 
BMediaDecoder::ReleaseDecoder()
{
	if(fDecoder) {
		_AddonManager *mgr = __get_decoder_manager();

		delete fDecoder;
		fDecoder = NULL;
		mgr->ReleaseAddon(fDecoderID);
	}
}


status_t 
BMediaDecoder::SetTo(const media_format *in_format,
                     const void *in_info, size_t in_size)
{
	ReleaseDecoder();

	if ((in_format->Encoding() == 0) && (in_format->type != B_MEDIA_RAW_AUDIO) &&
			(in_format->type != B_MEDIA_RAW_VIDEO)) {
		return B_MEDIA_BAD_FORMAT;
	}

	_AddonManager *mgr = __get_decoder_manager();
	image_id       imgid;
	Decoder     *(*make_decoder)(void);
	int32          cookie=0;

	BMediaFormats fmts;
	BPrivate::addon_list addons;
	fmts.find_addons(in_format, addons);
	addon_list * addp = &addons;
	if (addons.size() == 0) {
		addp = 0;
//		return B_MEDIA_BAD_FORMAT;
	}
//scan_again:

	for (int ix=0; ix<2; ix++)
	{
		bool allowLoad = (ix == 1);
		cookie = 0;
		while ((imgid = mgr->GetNextAddon(&cookie, &fDecoderID, allowLoad, addp)) > 0) {
	
			if (get_image_symbol(imgid, "instantiate_decoder",
								 B_SYMBOL_TYPE_TEXT,
								 (void **)&make_decoder) != B_OK) {
				mgr->ReleaseAddon(fDecoderID);
				continue;
			}
			
			fDecoder = make_decoder();
			if (fDecoder == NULL) {
				mgr->ReleaseAddon(fDecoderID);
				continue;
			}
	
			//printf("trying decoder: %ld\n", fDecoderID);
			status_t err;
			err = SetInputFormat(in_format, in_info, in_size);
			if(err == B_OK) {
				//printf("using decoder: %ld\n", fDecoderID);
				fDecoder->fNextChunk = next_chunk;
				fDecoder->fUserData = this;
				return B_OK;
			}
			//printf("decoder %ld failed, %s\n", fDecoderID, strerror(err));
	
			delete fDecoder;
			mgr->ReleaseAddon(fDecoderID);
			fDecoder = NULL;
		}
	}
//	if (addp != 0) {
//		addp = 0;
//		cookie = 0;
//		goto scan_again;
//	}
	return B_BAD_VALUE;
}

status_t 
BMediaDecoder::SetTo(const media_codec_info *mci)
{
	status_t err;
	_AddonManager *mgr = __get_decoder_manager();
	image_id       imgid;
	Decoder     *(*make_decoder)(void);

	ReleaseDecoder();

	imgid = mgr->GetAddonAt(mci->id);
	if(imgid < B_NO_ERROR)
		return imgid;

	err = get_image_symbol(imgid, "instantiate_decoder", B_SYMBOL_TYPE_TEXT,
	                       (void **)&make_decoder);
	if(err != B_OK) {
		mgr->ReleaseAddon(mci->id);
		return err;
	}

	fDecoder = make_decoder();
	if (fDecoder == NULL) {
		mgr->ReleaseAddon(mci->id);
		return B_ERROR;
	}
	fDecoderID = mci->id;

	fDecoder->fNextChunk = next_chunk;
	fDecoder->fUserData = this;

	return B_NO_ERROR;
}

status_t 
BMediaDecoder::GetDecoderInfo(media_codec_info *out_info) const
{
	if(fDecoder == NULL)
		return B_NO_INIT;
	out_info->id = fDecoderID;
	return fDecoder->GetCodecInfo(out_info);
}

status_t 
BMediaDecoder::SetInputFormat(const media_format *in_format,
                              const void *in_info, size_t in_size)
{
	status_t	err;
	bigtime_t	time;
	int64		frame;

	if(fDecoder == NULL)
		return B_NO_INIT;

	err = fDecoder->Sniff(in_format, in_info, in_size);
	if(err != B_NO_ERROR)
		return err;
	time = 0;
	return fDecoder->Reset(B_SEEK_BY_TIME, frame, &frame, time, &time);
}

status_t 
BMediaDecoder::SetOutputFormat(media_format *inout_format)
{
	if(fDecoder == NULL)
		return B_NO_INIT;
	return fDecoder->Format(inout_format);
}

status_t 
BMediaDecoder::Decode(void *out_buffer, int64 *out_frameCount, media_header *mh,
                      media_decode_info *info)
{
	if(fDecoder == NULL)
		return B_NO_INIT;
	return fDecoder->Decode((char*)out_buffer, out_frameCount, mh, info);
}

BMediaBufferDecoder::BMediaBufferDecoder()
{
	buffer = NULL;
	buffer_size = 0;
}

BMediaBufferDecoder::BMediaBufferDecoder(const media_format *in_format,
                                         const void *info, size_t info_size)
	: BMediaDecoder(in_format, info, info_size)
{
	buffer = NULL;
	buffer_size = 0;
}


BMediaBufferDecoder::BMediaBufferDecoder(const media_codec_info *mci)
	: BMediaDecoder(mci)
{
	buffer = NULL;
	buffer_size = 0;
}


status_t
BMediaBufferDecoder::DecodeBuffer(const void *input_buffer, size_t input_size,
                                  void *out_buffer, int64 *out_frameCount,
                                  media_header *out_mh, media_decode_info *info)
{
	buffer = input_buffer;
	buffer_size = input_size;
	return Decode(out_buffer, out_frameCount, out_mh, info);
}

status_t 
BMediaBufferDecoder::GetNextChunk(const void **chunkData, size_t *chunkLen, media_header */*mh*/)
{
	if(buffer == NULL)
		return B_ERROR;
	*chunkData = buffer;
	*chunkLen = buffer_size;
	buffer = NULL;
	buffer_size = 0;
	return B_NO_ERROR;
}


status_t BMediaDecoder::_Reserved_BMediaDecoder_0(int32 /*arg*/, ...) {return B_ERROR;}
status_t BMediaDecoder::_Reserved_BMediaDecoder_1(int32 /*arg*/, ...) {return B_ERROR;}
status_t BMediaDecoder::_Reserved_BMediaDecoder_2(int32 /*arg*/, ...) {return B_ERROR;}
status_t BMediaDecoder::_Reserved_BMediaDecoder_3(int32 /*arg*/, ...) {return B_ERROR;}
status_t BMediaDecoder::_Reserved_BMediaDecoder_4(int32 /*arg*/, ...) {return B_ERROR;}
status_t BMediaDecoder::_Reserved_BMediaDecoder_5(int32 /*arg*/, ...) {return B_ERROR;}
status_t BMediaDecoder::_Reserved_BMediaDecoder_6(int32 /*arg*/, ...) {return B_ERROR;}
status_t BMediaDecoder::_Reserved_BMediaDecoder_7(int32 /*arg*/, ...) {return B_ERROR;}
status_t BMediaDecoder::_Reserved_BMediaDecoder_8(int32 /*arg*/, ...) {return B_ERROR;}
status_t BMediaDecoder::_Reserved_BMediaDecoder_9(int32 /*arg*/, ...) {return B_ERROR;}
status_t BMediaDecoder::_Reserved_BMediaDecoder_10(int32 /*arg*/, ...) {return B_ERROR;}
status_t BMediaDecoder::_Reserved_BMediaDecoder_11(int32 /*arg*/, ...) {return B_ERROR;}
status_t BMediaDecoder::_Reserved_BMediaDecoder_12(int32 /*arg*/, ...) {return B_ERROR;}
status_t BMediaDecoder::_Reserved_BMediaDecoder_13(int32 /*arg*/, ...) {return B_ERROR;}
status_t BMediaDecoder::_Reserved_BMediaDecoder_14(int32 /*arg*/, ...) {return B_ERROR;}
status_t BMediaDecoder::_Reserved_BMediaDecoder_15(int32 /*arg*/, ...) {return B_ERROR;}
