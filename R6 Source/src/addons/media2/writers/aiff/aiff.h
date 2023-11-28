#ifndef _AIFF_WRITER_H
#define _AIFF_WRITER_H	1

#include <OS.h>
#include <support2/SupportDefs.h>
#include <support2/Locker.h>
#include <media2/MediaDefs.h>

#include "MediaWriter.h"

namespace B {
namespace Media2 {

class AIFFWriter : public B::Private::MediaWriter {
public:
	struct AudioHeader {
		IByteOutput::ptr		out;
		IByteSeekable::ptr	seek;
		float		rate;
		uint16		channel_count;
		uint16		bit_per_sample;
		uint32		frame_count;
		int32		offset_total_length;
		int32		offset_data_length;
		int32		offset_frame_count;
		uint32		data_length;
		uint32		total_length;
		char		*copyright;
		uint8		mode;
		char		_reserved_[3];
	};

				AIFFWriter();
	virtual		~AIFFWriter();

	status_t	SetDestination(IByteInput::arg in,
							   IByteOutput::arg out,
							   IByteSeekable::arg seek);
	
	status_t	AddTrack(BMediaTrack *track);
	status_t	AddCopyright(const char *data);
	status_t	AddTrackInfo(int32 track, uint32 code, const char *data,size_t sz);
	status_t	AddChunk(int32 type, const char *data, size_t size);
	status_t	CommitHeader(void);

	status_t	WriteData(int32 			tracknum,
						  media_type 		type,
						  const void 		*data,
						  size_t 			size,
						  media_encode_info	*info);

	status_t	CloseFile(void);


private:
	void		ConvertFloatTo80Bits(float f, int16 *exp, uint32 *m0, uint32 *m1);

	BMediaTrack 	*fTrack;		  // we only support 1 audio track
	AudioHeader		fHeader;
};

} } // B::Private
#endif
