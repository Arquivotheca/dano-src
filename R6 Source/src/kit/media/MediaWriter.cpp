#include <stdlib.h>
#include <stdarg.h>

#include "File.h"
#include "MediaWriter.h"


MediaWriter::MediaWriter()
{
}

MediaWriter::~MediaWriter()
{
}

status_t 
MediaWriter::SetWriteBufferSize(size_t /*buffersize*/)
{
	return B_NOT_ALLOWED;
}


status_t
MediaWriter::AddCopyright(const char */*data*/)
{
	return B_OK;
}

status_t
MediaWriter::AddTrackInfo(int32 /*track*/, uint32 /*code*/, const char */*data*/,size_t /*sz*/)
{
	return B_OK;
}

status_t MediaWriter::Perform(int32 /*selector*/, void * /*data*/)
{
	return B_ERROR;
}


status_t MediaWriter::ControlFile(int32 /*selector*/, void * /*data*/, size_t /*size*/)
{
	return EBADF;
}

status_t MediaWriter::_Reserved_MediaWriter_0(int32 arg, ...) {
	void * data;
	size_t size;
	va_list vl;
	va_start(vl, arg);
	data = va_arg(vl, void *);
	size = va_arg(vl, size_t);
	va_end(vl);
	return MediaWriter::ControlFile(arg, data, size);
}

status_t MediaWriter::_Reserved_MediaWriter_1(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_2(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_3(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_4(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_5(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_6(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_7(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_8(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_9(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_10(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_11(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_12(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_13(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_14(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_15(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_16(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_17(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_18(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_19(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_20(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_21(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_22(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_23(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_24(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_25(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_26(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_27(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_28(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_29(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_30(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_31(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_32(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_33(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_34(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_35(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_36(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_37(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_38(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_39(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_40(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_41(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_42(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_43(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_44(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_45(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_46(int32 /*arg*/, ...) { return B_ERROR; }
status_t MediaWriter::_Reserved_MediaWriter_47(int32 /*arg*/, ...) { return B_ERROR; }
