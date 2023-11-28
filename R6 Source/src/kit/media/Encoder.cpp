#include <stdio.h>
#include <stdarg.h>

#include "Encoder.h"
#include "MediaWriter.h"
#include "addons.h"
#include "MediaTrack.h"
#include "MediaFile.h"



Encoder::Encoder()
{
	fTrack = NULL;
	fWriteChunk = NULL;
	fUserData = NULL;
}

Encoder::~Encoder()
{
}


void
Encoder::SetTrack(BMediaTrack *track)
{
	fTrack = track;
	AttachedToTrack();
}

void
Encoder::AttachedToTrack()
{
}

BParameterWeb *
Encoder::Web()
{
printf("Encoder::Web\n");
	return NULL;
}

status_t
Encoder::AddCopyright(const char *data)
{
	if (fTrack == NULL)
		return B_BAD_VALUE;
	
	return fTrack->AddCopyright(data);
}


status_t
Encoder::AddTrackInfo(uint32 code, const char *data, size_t size)
{
	if (fTrack == NULL)
		return B_BAD_VALUE;
	
	return fTrack->AddTrackInfo(code, data, size);
}

status_t 
Encoder::WriteChunk(const void *data, size_t size, media_encode_info *info)
{
	if(fTrack)
		return fTrack->WriteChunk(data, size, info);
	if(fWriteChunk)
		return fWriteChunk(fUserData, data, size, info);
	return B_NO_INIT;
}

status_t
Encoder::GetParameterValue(int32 /*id*/, void */*valu*/, size_t */*size*/)
{
	return B_ERROR;
}


status_t
Encoder::SetParameterValue(int32 /*id*/, const void */*valu*/, size_t /*size*/)
{
	return B_ERROR;
}


BView *
Encoder::GetParameterView()
{
	return NULL;
}

status_t
Encoder::GetEncodeParameters(encode_parameters */*parameters*/) const
{
	return B_ERROR;
}

status_t
Encoder::SetEncodeParameters(encode_parameters */*parameters*/)
{
	return B_ERROR;
}

status_t 
Encoder::StartEncoder()
{
	return B_OK;
}

status_t
Encoder::Flush()
{
	return B_OK;
}


status_t Encoder::Perform(int32 /*selector*/, void * /*data*/)
{
	return B_ERROR;
}

status_t Encoder::CommitHeader()
{
	return B_ERROR;
}

status_t Encoder::ControlCodec(int32 /*selector*/, void * /*data*/, size_t /*size*/)
{
	return EBADF;
}

#if _R4_5_COMPATIBLE_
extern "C" {

	//	This is a bad example of how to do compatibility.
	//	It may actually cause infinite recursion!
	//	Instead, use the mechanism below for _Encoder_1.
	_EXPORT status_t
	#if __GNUC__
	_Reserved_Encoder_0__Q28BPrivate7Encoderle
	#elif __MWERKS__
	_Reserved_Encoder_0__Q28BPrivate7EncoderFle
	#endif
	(Encoder* This)
	{
		return This->Encoder::CommitHeader();
	}

}
#endif


status_t Encoder::_Reserved_Encoder_1(int32 arg, ...) {
	void * data;
	size_t size;
	va_list vl;
	va_start(vl, arg);
	data = va_arg(vl, void *);
	size = va_arg(vl, size_t);
	va_end(vl);
	return Encoder::ControlCodec(arg, data, size);
}
status_t Encoder::_Reserved_Encoder_2(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_3(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_4(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_5(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_6(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_7(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_8(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_9(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_10(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_11(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_12(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_13(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_14(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_15(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_16(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_17(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_18(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_19(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_20(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_21(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_22(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_23(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_24(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_25(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_26(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_27(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_28(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_29(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_30(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_31(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_32(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_33(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_34(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_35(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_36(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_37(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_38(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_39(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_40(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_41(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_42(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_43(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_44(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_45(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_46(int32 /*arg*/, ...) { return B_ERROR; }
status_t Encoder::_Reserved_Encoder_47(int32 /*arg*/, ...) { return B_ERROR; }
