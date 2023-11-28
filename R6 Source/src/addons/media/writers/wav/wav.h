#ifndef _WAV_WRITER_H
#define _WAV_WRITER_H	1

#include <OS.h>
#include <SupportDefs.h>
#include <Locker.h>
#include <MediaDefs.h>

#if defined (__POWERPC__)
#define _EXPORT __declspec(dllexport)
#else
#define _EXPORT
#endif

class FileWriter;

namespace BPrivate {
	class _EXPORT WAVWriter;
}


namespace BPrivate {

// WAVWriter is the base virtual class that must be subclassed to implement
// specific encapsulation format. Each flavour lives in its own add-on.

class WAVWriter : public MediaWriter {
public:
	struct AudioHeader {
		FileWriter*		writer;
		float		rate;
		uint16		channel_count;
		uint16		bit_per_sample;
		uint32		codec;
		uint32		frame_size;
		uint32		raw_format;
		int32		offset_total_length;
		int32		offset_data_length;
		uint32		data_length;
		uint32		total_length;
		char		*meta_data;
		uint32		meta_data_length;
		char		*copyright;
		uint8		mode;
		char		_reserved_[3];
		uint32		channel_mask;
		int16		valid_bits;
		uint16		matrix_mask;
	};

				WAVWriter();
	virtual		~WAVWriter();

	status_t	SetSource(BDataIO *source);
	
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
	BMediaTrack 	*fTrack;		  // we only support 1 audio track
	AudioHeader		fHeader;
	bool 			fHeaderCommitted;
};

}	//	namespace


#endif
