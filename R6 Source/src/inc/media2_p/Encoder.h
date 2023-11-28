#ifndef _MEDIA2_ENCODER_PRIVATE_
#define	_MEDIA2_ENCODER_PRIVATE_

#include <media2/MediaDefs.h>
#include <media2/MediaFormats.h>
#include <media2/MediaFile.h>

#include "codec_addons.h"

namespace B {

namespace Media2 {
	class BMediaTrack;
}

namespace Private {

using namespace Support2;
using namespace Media2;

#if defined (__POWERPC__)
#define _EXPORT __declspec(dllexport)
#else
#define _EXPORT
#endif

class _EXPORT Encoder;

extern "C" {
	_EXPORT extern void               register_encoder(void);
	// called when the addon is loaded

	// only implement one of the instantiate functions
	_EXPORT extern B::Private::Encoder *instantiate_encoder(void);
	// called to instantiate an encoder object

	_EXPORT extern B::Private::Encoder *instantiate_nth_encoder(int32 index);
	// called to instantiate an encoder object. Index specifies which
	// of the possible output encodings to use. Return NULL if index is out
	// of range.
}

// Encoder is the base virtual class that should be subclassed to
// implement specific codecs. Each flavour lives in its own add-on.

class Encoder {

public:

						Encoder();
	virtual				~Encoder();

	void				SetTrack(BMediaTrack *track);

	// To be called in AttachedToTrack() to add special info to a track
	status_t			AddCopyright(const char *data);
	status_t			AddTrackInfo(uint32 code,const char *data,size_t size);

	// Called from Encode()
	status_t			WriteChunk(const void *data, size_t size,
								   media_encode_info *info);

	virtual status_t	GetCodecInfo(media_codec_info *mci) const = 0;

	/* return B_OK if format is acceptable, otherwise change format to
	   be acceptable, and return an error, will be called until B_OK
	   is returned before StartEncoder. Can also be called while encoding
	   to request changes. If the format cannot be changed, return an error
	   and update the format to the current format */
	virtual status_t	SetFormat(media_file_format *mfi,
								  media_format *input_format,
								  media_format *output_format) = 0;
	virtual void		AttachedToTrack();

	virtual status_t	StartEncoder();
	virtual status_t	Encode(const void *in_buffer, int64 num_frames,
						       media_encode_info *info) = 0;
	virtual status_t	Flush();

#if 0
	// +++ to be replaced by an IMediaControllable accessor! +++
	// These are for controlling the underlying encoder and track parameters

	// return a copy of your parameter web or NULL
	virtual BParameterWeb	*Web();
	virtual status_t 		GetParameterValue(int32 id, void *valu, size_t *size);
	virtual status_t		SetParameterValue(int32 id, const void *valu, size_t size);
	virtual BView			*GetParameterView();
#endif
	
	// This is the simplified control API.
	// SetEncodeParameters should return an error and modify the input
	// if a parameter could not be changed. These functioins can be called
	// before or after StartEncoder is called and should affect all
	// frames encoded thereon.
	virtual status_t		GetEncodeParameters(encode_parameters *parameters) const;
	virtual status_t		SetEncodeParameters(encode_parameters *parameters);

virtual	status_t Perform(int32 selector, void * data);
	virtual status_t		CommitHeader();
	virtual status_t		ControlCodec(int32 selector, void * data, size_t size);

private:

	Encoder(const Encoder&);
	Encoder& operator=(const Encoder&);

	// friend class _addon_sniffer;
	friend class BMediaEncoder;
	friend class BMediaFile;
	friend class BMediaTrack;

	typedef status_t (write_chunk)(void *, const void *, size_t, media_encode_info *);

	BMediaTrack *		fTrack;
	write_chunk *		fWriteChunk;
	void *				fUserData;

	uint32 _reserved_Encoder_[16];

/*virtual	status_t _Reserved_Encoder_0(int32 arg, ...)*/;
		status_t _Reserved_Encoder_1(int32 arg, ...);
virtual	status_t _Reserved_Encoder_2(int32 arg, ...);
virtual	status_t _Reserved_Encoder_3(int32 arg, ...);
virtual	status_t _Reserved_Encoder_4(int32 arg, ...);
virtual	status_t _Reserved_Encoder_5(int32 arg, ...);
virtual	status_t _Reserved_Encoder_6(int32 arg, ...);
virtual	status_t _Reserved_Encoder_7(int32 arg, ...);
virtual	status_t _Reserved_Encoder_8(int32 arg, ...);
virtual	status_t _Reserved_Encoder_9(int32 arg, ...);
virtual	status_t _Reserved_Encoder_10(int32 arg, ...);
virtual	status_t _Reserved_Encoder_11(int32 arg, ...);
virtual	status_t _Reserved_Encoder_12(int32 arg, ...);
virtual	status_t _Reserved_Encoder_13(int32 arg, ...);
virtual	status_t _Reserved_Encoder_14(int32 arg, ...);
virtual	status_t _Reserved_Encoder_15(int32 arg, ...);
virtual	status_t _Reserved_Encoder_16(int32 arg, ...);
virtual	status_t _Reserved_Encoder_17(int32 arg, ...);
virtual	status_t _Reserved_Encoder_18(int32 arg, ...);
virtual	status_t _Reserved_Encoder_19(int32 arg, ...);
virtual	status_t _Reserved_Encoder_20(int32 arg, ...);
virtual	status_t _Reserved_Encoder_21(int32 arg, ...);
virtual	status_t _Reserved_Encoder_22(int32 arg, ...);
virtual	status_t _Reserved_Encoder_23(int32 arg, ...);
virtual	status_t _Reserved_Encoder_24(int32 arg, ...);
virtual	status_t _Reserved_Encoder_25(int32 arg, ...);
virtual	status_t _Reserved_Encoder_26(int32 arg, ...);
virtual	status_t _Reserved_Encoder_27(int32 arg, ...);
virtual	status_t _Reserved_Encoder_28(int32 arg, ...);
virtual	status_t _Reserved_Encoder_29(int32 arg, ...);
virtual	status_t _Reserved_Encoder_30(int32 arg, ...);
virtual	status_t _Reserved_Encoder_31(int32 arg, ...);
virtual	status_t _Reserved_Encoder_32(int32 arg, ...);
virtual	status_t _Reserved_Encoder_33(int32 arg, ...);
virtual	status_t _Reserved_Encoder_34(int32 arg, ...);
virtual	status_t _Reserved_Encoder_35(int32 arg, ...);
virtual	status_t _Reserved_Encoder_36(int32 arg, ...);
virtual	status_t _Reserved_Encoder_37(int32 arg, ...);
virtual	status_t _Reserved_Encoder_38(int32 arg, ...);
virtual	status_t _Reserved_Encoder_39(int32 arg, ...);
virtual	status_t _Reserved_Encoder_40(int32 arg, ...);
virtual	status_t _Reserved_Encoder_41(int32 arg, ...);
virtual	status_t _Reserved_Encoder_42(int32 arg, ...);
virtual	status_t _Reserved_Encoder_43(int32 arg, ...);
virtual	status_t _Reserved_Encoder_44(int32 arg, ...);
virtual	status_t _Reserved_Encoder_45(int32 arg, ...);
virtual	status_t _Reserved_Encoder_46(int32 arg, ...);
virtual	status_t _Reserved_Encoder_47(int32 arg, ...);
};

} } // B::Private
#endif //_MEDIA2_ENCODER_PRIVATE_
