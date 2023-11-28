#ifndef _MEDIAWRITER_H
#define _MEDIAWRITER_H	1

#include <OS.h>
#include <SupportDefs.h>
#include <Locker.h>
#include <MediaDefs.h>
#include <MediaFormats.h>

class BFile;

#if defined (__POWERPC__)
#define _EXPORT __declspec(dllexport)
#else
#define _EXPORT
#endif

namespace BPrivate {
	class _EXPORT MediaWriter;
}

using namespace BPrivate;

extern "C" {
	_EXPORT BPrivate::MediaWriter *instantiate_mediawriter(void);
	_EXPORT status_t    get_mediawriter_info(media_file_format *mfi);
	_EXPORT status_t    accepts_format(media_format *mf, uint32 flags);
}

namespace BPrivate {


// MediaWriter is the base virtual class that must be subclassed to implement
// specific encapsulation format. Each flavour lives in its own add-on.

class MediaWriter {

public:
						MediaWriter();
	virtual				~MediaWriter();

	virtual status_t	SetSource(BDataIO *source) = 0;
	
	virtual status_t	AddCopyright(const char *data);
	virtual status_t	AddTrackInfo(int32 track, uint32 code,const char *data,
									 size_t size);

	virtual status_t	AddTrack(BMediaTrack *track) = 0;
	// this function is so that users can add their own chunks to the
	// meta-data about a file (such as UDTA or CPY (copyright) chunks
	// in QuickTime.
	virtual status_t	AddChunk(int32 type, const char *data, size_t size) = 0;
	virtual status_t	CommitHeader(void) = 0;

	virtual status_t	WriteData(int32 			tracknum,
								  media_type	 	type,
								  const void	 	*data,
								  size_t 			size,
								  media_encode_info	*info) = 0;

	virtual status_t	CloseFile(void) = 0;

	virtual status_t	SetWriteBufferSize(size_t buffersize);

virtual	status_t Perform(int32 selector, void * data);

	//	return positive or 0 for success, negative for error
	virtual status_t	ControlFile(int32 selector, void * io_data, size_t size);

	// implementation
private:

	MediaWriter(const MediaWriter&);
	MediaWriter& operator=(const MediaWriter&);

	int pad;  // we've gotta have something....

	uint32 _reserved_MediaWriter_[16];

	status_t _Reserved_MediaWriter_0(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_1(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_2(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_3(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_4(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_5(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_6(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_7(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_8(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_9(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_10(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_11(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_12(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_13(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_14(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_15(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_16(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_17(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_18(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_19(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_20(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_21(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_22(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_23(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_24(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_25(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_26(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_27(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_28(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_29(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_30(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_31(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_32(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_33(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_34(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_35(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_36(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_37(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_38(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_39(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_40(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_41(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_42(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_43(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_44(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_45(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_46(int32 arg, ...);
virtual	status_t _Reserved_MediaWriter_47(int32 arg, ...);
};

}	//	namespace


#endif
