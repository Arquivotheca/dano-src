#include <stdarg.h>
#include <MediaFormats.h>
#include <MediaDefs.h>
#include "addons.h"
#include "Decoder.h"
#include "Extractor.h"
#include "MediaTrack.h"

using namespace BPrivate;

Decoder::Decoder()
{
	fTrack = NULL;
	fNextChunk = NULL;
}

Decoder::~Decoder()
{
}


status_t
Decoder::Reset(int32 in_towhat,
			   int64 in_requiredFrame, int64 *inout_frame,
			   bigtime_t in_requiredTime, bigtime_t *inout_time)
{
	return B_OK;
}

status_t
Decoder::SetTrack(BMediaTrack *track)
{
	media_format	format;
	status_t		err;
	int64			frame;
	bigtime_t		time;
	void			*info = NULL;
	int32			size = 0;

	fTrack = track;
	err = track->TrackInfo(&format, &info, &size);
	if (err) {
		return err;
	}
	err = Sniff(&format, info, size);
	if (err) {
		return err;
	}
	time = 0;
	return Reset(B_SEEK_BY_TIME, frame, &frame, time, &time);
}

//status_t 
//Decoder::GetNextChunk(char **chunkData, int32 *chunkLen, media_header *mh)
//{
//	return GetNextChunk((void**)chunkData, chunkLen, mh, NULL);
//}

status_t
Decoder::GetNextChunk(const void **chunkData, size_t *chunkLen, media_header *mf,
                      media_decode_info *info)
{
	if (fTrack)
		return fTrack->ReadChunk((char**)chunkData,(int32*)chunkLen, mf);
	else if (fNextChunk)
		return fNextChunk(fUserData,(void **)chunkData,chunkLen, mf);
	return B_ERROR;
};

status_t Decoder::Perform(int32 selector, void * data)
{
	return B_ERROR;
}

status_t Decoder::ControlCodec(int32 selector, void * data, size_t size)
{
	return EBADF;
}

status_t Decoder::_Reserved_Decoder_0(int32 arg, ...) {
	void * data;
	size_t size;
	va_list vl;
	va_start(vl, arg);
	data = va_arg(vl, void *);
	size = va_arg(vl, size_t);
	va_end(vl);
	return Decoder::ControlCodec(arg, data, size);
}
status_t Decoder::_Reserved_Decoder_1(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_2(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_3(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_4(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_5(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_6(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_7(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_8(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_9(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_10(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_11(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_12(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_13(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_14(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_15(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_16(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_17(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_18(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_19(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_20(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_21(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_22(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_23(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_24(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_25(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_26(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_27(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_28(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_29(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_30(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_31(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_32(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_33(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_34(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_35(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_36(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_37(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_38(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_39(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_40(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_41(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_42(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_43(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_44(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_45(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_46(int32 arg, ...) { return B_ERROR; }
status_t Decoder::_Reserved_Decoder_47(int32 arg, ...) { return B_ERROR; }

