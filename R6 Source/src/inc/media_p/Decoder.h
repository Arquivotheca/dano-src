#ifndef _DECODER_H
#define	_DECODER_H	1

#include <MediaDefs.h>
#include <MediaFormats.h>

#include "addons.h"

class BMediaTrack;


namespace BPrivate {
	class _EXPORT Decoder;
}


extern "C" {
_EXPORT extern void               register_decoder(const media_format ** out_ptr, int32 * out_count);
	// called when the addon is loaded

_EXPORT extern status_t           get_next_description(int32 *cookie,
                                                       media_type *otype,
                                                       const media_format_description **odesc,
                                                       int32 *ocount);
	// called to update the cached codec format info

_EXPORT extern BPrivate::Decoder *instantiate_decoder(void);
	// called to instantiate a decoder object
}


// Decoder is the base virtual class that should be subclassed to
// implement specific codecs. Each flavour lives in its own add-on.

class DecoderNode;
class BMediaDecoder;

namespace BPrivate {

class Decoder {

public:

					Decoder();
	virtual			~Decoder();

	status_t		SetTrack(BMediaTrack *track);

	// Hooks
	//  GetCodecInfo() is called to fill in a media_codec_info struct
	//                 with info about this decoder.
	//	Sniff() is called when looking for a Decoder for a track.
	//			it should return an error if the Decoder cannot handle
	//			this track. Otherwise, it should initialize the object.
	//	Format() gets upon entry the format that the application wants and
	//			 returns that format untouched if it can output it, or the
	//			 closest match if it cannot output that format.
	//	Decode() outputs a frame. It gets the chunks from the file
	//			 by invoking GetTrack()->GetNextChunk(). If Decode() is invoked
	//			 on a key frame, then it should reset its state before it
	//			 decodes the frame. it is guaranteed that the output buffer 
	//			 will not be touched by the application, which implies this 
	//			 buffer may be used to cache its state from frame to frame. 
	//  Reset() is called immediately after the decoder is constructed or 
	//			when the track is seeked.

	virtual status_t	GetCodecInfo(media_codec_info *mci) const = 0;
	virtual status_t	Sniff(const media_format *input_format,
	                          const void *in_info, size_t in_size) = 0;
	virtual status_t	Format(media_format *output_format) = 0;
	virtual status_t	Decode(void *out_buffer, int64 *out_frameCount,
							   media_header *mh, media_decode_info *info) = 0;
	virtual status_t	Reset(int32 in_towhat,
							  int64 in_requiredFrame, int64 *inout_frame,
							  bigtime_t in_requiredTime,bigtime_t *inout_time);

	// Call backs
	
	status_t			GetNextChunk(const void **chunkData, size_t *chunkLen,
									 media_header *mh, media_decode_info *info);

virtual	status_t Perform(int32 selector, void * data);

	//	return 0 or greater for success, < 0 for failure
virtual	status_t ControlCodec(int32 selector, void * data, size_t size);

private:
	//status_t			GetNextChunk(char **chunkData, int32 *chunkLen,
	//								 media_header *mh); // obsolete

	Decoder(const Decoder&);
	Decoder& operator=(const Decoder&);

	friend class BMediaFile;
	friend class BMediaTrack;
	friend class DecoderNode;
	friend class BMediaDecoder;
	typedef status_t (next_chunk)(void *, void **, size_t *, media_header *mh);

	BMediaTrack *		fTrack;
	next_chunk *		fNextChunk;
	void *				fUserData;

	uint32 _reserved_Decoder_[16];

	status_t _Reserved_Decoder_0(int32 arg, ...);
virtual	status_t _Reserved_Decoder_1(int32 arg, ...);
virtual	status_t _Reserved_Decoder_2(int32 arg, ...);
virtual	status_t _Reserved_Decoder_3(int32 arg, ...);
virtual	status_t _Reserved_Decoder_4(int32 arg, ...);
virtual	status_t _Reserved_Decoder_5(int32 arg, ...);
virtual	status_t _Reserved_Decoder_6(int32 arg, ...);
virtual	status_t _Reserved_Decoder_7(int32 arg, ...);
virtual	status_t _Reserved_Decoder_8(int32 arg, ...);
virtual	status_t _Reserved_Decoder_9(int32 arg, ...);
virtual	status_t _Reserved_Decoder_10(int32 arg, ...);
virtual	status_t _Reserved_Decoder_11(int32 arg, ...);
virtual	status_t _Reserved_Decoder_12(int32 arg, ...);
virtual	status_t _Reserved_Decoder_13(int32 arg, ...);
virtual	status_t _Reserved_Decoder_14(int32 arg, ...);
virtual	status_t _Reserved_Decoder_15(int32 arg, ...);
virtual	status_t _Reserved_Decoder_16(int32 arg, ...);
virtual	status_t _Reserved_Decoder_17(int32 arg, ...);
virtual	status_t _Reserved_Decoder_18(int32 arg, ...);
virtual	status_t _Reserved_Decoder_19(int32 arg, ...);
virtual	status_t _Reserved_Decoder_20(int32 arg, ...);
virtual	status_t _Reserved_Decoder_21(int32 arg, ...);
virtual	status_t _Reserved_Decoder_22(int32 arg, ...);
virtual	status_t _Reserved_Decoder_23(int32 arg, ...);
virtual	status_t _Reserved_Decoder_24(int32 arg, ...);
virtual	status_t _Reserved_Decoder_25(int32 arg, ...);
virtual	status_t _Reserved_Decoder_26(int32 arg, ...);
virtual	status_t _Reserved_Decoder_27(int32 arg, ...);
virtual	status_t _Reserved_Decoder_28(int32 arg, ...);
virtual	status_t _Reserved_Decoder_29(int32 arg, ...);
virtual	status_t _Reserved_Decoder_30(int32 arg, ...);
virtual	status_t _Reserved_Decoder_31(int32 arg, ...);
virtual	status_t _Reserved_Decoder_32(int32 arg, ...);
virtual	status_t _Reserved_Decoder_33(int32 arg, ...);
virtual	status_t _Reserved_Decoder_34(int32 arg, ...);
virtual	status_t _Reserved_Decoder_35(int32 arg, ...);
virtual	status_t _Reserved_Decoder_36(int32 arg, ...);
virtual	status_t _Reserved_Decoder_37(int32 arg, ...);
virtual	status_t _Reserved_Decoder_38(int32 arg, ...);
virtual	status_t _Reserved_Decoder_39(int32 arg, ...);
virtual	status_t _Reserved_Decoder_40(int32 arg, ...);
virtual	status_t _Reserved_Decoder_41(int32 arg, ...);
virtual	status_t _Reserved_Decoder_42(int32 arg, ...);
virtual	status_t _Reserved_Decoder_43(int32 arg, ...);
virtual	status_t _Reserved_Decoder_44(int32 arg, ...);
virtual	status_t _Reserved_Decoder_45(int32 arg, ...);
virtual	status_t _Reserved_Decoder_46(int32 arg, ...);
virtual	status_t _Reserved_Decoder_47(int32 arg, ...);

};

}	//	namespace

#endif
