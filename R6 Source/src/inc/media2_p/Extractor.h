#ifndef _MEDIA2_EXTRACTOR_PRIVATE_
#define _MEDIA2_EXTRACTOR_PRIVATE_

#include <OS.h>
#include <support2/SupportDefs.h>
#include <support2/Locker.h>
#include <support2/IByteStream.h>
#include <media2/MediaDefs.h>
#include <media2/MediaFormats.h>

#include "codec_addons.h"

namespace B {
namespace Private {

using namespace Support2;
using namespace Media2;

class Extractor;

extern "C" {
	_EXPORT void register_extractor(const media_format ** out_list, int32 * out_count);
	// called when the addon is loaded

	_EXPORT status_t get_next_description(int32 *cookie,
                                          media_type *otype,
                                          const media_format_description **odesc,
                                          int32 *ocount);
	// called to update the cached codec format info

	_EXPORT B::Private::Extractor *instantiate_extractor(void);
	// called to instantiate a decoder object
}

#define	B_SEEK_BY_FRAME		1
#define	B_SEEK_BY_TIME		2

enum media_private_seek_flags {
	B_MEDIA_SEEK_PRIVATE_MASK = 0xffff0000,
	B_MEDIA_SEEK_PEEK = 0x00010000
};

// Extractor is the base virtual class that must be subclassed to implement
// specific encapsulation format. Each flavour lives in its own add-on.

struct ExtractorInfo {
	Extractor *	extractor;
	int32		extractor_id;
	int32		stream_count;
};

class MediaExtractor {
	public:
						MediaExtractor(int32 flags);
		virtual			~MediaExtractor();

		// Interface to the Track class
		//   SetRef() is called by the MediaFile constructor after it
		//                   instanciates the Extractor. This in turn calls the
		//                   Setup() hook, which returns an error if the file is
		//                   of the wrong type.
		//   GetNextChunk() is called by BMediaTrack::GetNextChunk(). it in turn
		//                   calls the extractor hook SplitNext() as many times
		//                   as necessary (to handle implicit seeking and
		//                   partial chunks).
		//   SeekTo() is called by BMediaTrack::SeekToFrame/Time(). it in turn
		//                   calls the extractor hook Seek() as many times as
		//                   necessary.
	

		size_t			GetChunkSize(void) { return fChunkSize; }
		status_t		SetSource(IByteInput::arg stream, IByteSeekable::arg seek,
								  int32 *out_streamNum);
		status_t		SetSource(const media_format *input_format,
		                          int32 *out_streamNum);
		status_t		AllocateCookie(int32 in_stream, void **cookieptr);
		status_t		FreeCookie(int32 in_stream, void *cookie);
		status_t		GetNextChunk(int32 in_stream, void *cookie,
		                             char **out_buffer,
									 int32 *out_size, media_header *mh);
		status_t		SeekTo(int32 in_stream, void *cookie,
		                       int32 in_towhat,
							   bigtime_t *inout_time, int64 *inout_frame,
							   int32 flags);
		status_t		FindKeyFrame(int32 in_stream, void *cookie,
		                             int32 in_towhat,
		        		             bigtime_t *inout_time, int64 *inout_frame,
		        		             int32 flags);

		status_t		GetFileFormatInfo(media_file_format *mfi);

		const char *	Copyright(void);
		status_t		TrackInfo(int32 in_stream, media_format *out_format,
		                          void **out_info, int32 *out_infoSize);
		status_t		CountFrames(int32 in_stream, int64 *out_frames);
		status_t		GetDuration(int32 in_stream, bigtime_t *out_duration);

virtual	status_t Perform(int32 selector, void * data);

		status_t		ControlFile(int32 selector, void * data, size_t size);

	private:
		ExtractorInfo	fFileExtractorInfo;
		
		ExtractorInfo *	fStreamExtractorInfo;

		Extractor **	fExtractors;
		int32 *			fStreamNums;

private:

	friend class ExtractorNode;
	friend class ExtractorAddon;
	friend class BMediaFile;

	struct stream_info {
		off_t			pos;
		off_t			pos_chunk;				// pos - pos % fChunkSize, cached for efficiency
		off_t			pos_offset;				// pos % fChunkSize, cached for efficiency
		off_t			source_pos;				// position in source, cached
		int32			chunk[2];
		int32			index;					// fChunks[chunk[index % 2]] points to current chunk
		int32			partial_chunk;			// chunk used for partial frames
		bool			wait[2];
	};

	struct chunk_info {
		char			*start;
		int32			count;
		off_t			offset;
		bool			busy;
		bool			bad;
		sem_id			sem;					// released when the job completes
	};

	void				ReaderLoop();
	status_t			LookupChunk(int32 in_stream, int32 in_index, off_t in_offset);
	status_t			ReadAhead(int32 in_stream);
	status_t			SeekToPosition(int32 in_stream, off_t in_filePos);
	int32				ProcessJob(int32 job);

	int32				fChunkSize;
	int32				fStreamNum;
	stream_info *		fStreams;
	sem_id				fSubmitJobSem;			// released every time a job is submitted
	int32				fJobsNum;				// # of entries in fJobs
	int32				fCurJob;				// current entry in fJobs, as enqueue by the splitters
	int32				*fJobs;
	BLocker				fJobLock;
	chunk_info			*fChunks;	
	char *				fReadArea;
	thread_id			fReaderThread;
	off_t				fFileSize;
	IByteInput::ptr		fSourceStream;
	IByteSeekable::ptr	fSourceSeekable;
	int32				fFlags;

static	int32	start_reader_thread(void *);

	MediaExtractor();
	MediaExtractor(const MediaExtractor&);
	MediaExtractor& operator=(const MediaExtractor&);

	/* fbc data and virtuals */

	uint32 _reserved_MediaExtractor_[15];

virtual	status_t _Reserved_MediaExtractor_0(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_1(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_2(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_3(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_4(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_5(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_6(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_7(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_8(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_9(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_10(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_11(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_12(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_13(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_14(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_15(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_16(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_17(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_18(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_19(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_20(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_21(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_22(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_23(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_24(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_25(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_26(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_27(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_28(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_29(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_30(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_31(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_32(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_33(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_34(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_35(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_36(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_37(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_38(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_39(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_40(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_41(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_42(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_43(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_44(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_45(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_46(int32 arg, ...);
virtual	status_t _Reserved_MediaExtractor_47(int32 arg, ...);

};

class Extractor {

public:

						Extractor();
	virtual				~Extractor();

	// Hooks

	virtual status_t	GetFileFormatInfo(media_file_format *mfi) = 0;
	/* for file extractors */
	virtual status_t	Sniff(int32 *out_streamNum, int32 *out_chunkSize);
	/* for stream extractors */
	virtual status_t	SniffFormat(const media_format *input_format,
	                	            int32 *out_streamNum, int32 *out_chunkSize);
	virtual const char  *Copyright(void);
	virtual status_t	TrackInfo(int32 in_stream, media_format *out_format, void **out_info, int32 *out_infoSize) = 0;
	virtual status_t	CountFrames(int32 in_stream, int64 *out_frames) = 0;
	virtual status_t	GetDuration(int32 in_stream, bigtime_t *out_duration) = 0;

	// allocate and free cookie used in split and seek.
	virtual status_t	AllocateCookie(int32 in_stream, void **cookieptr) = 0;
	virtual	status_t	FreeCookie(int32 in_stream, void *cookie) = 0;

	virtual status_t	SplitNext(int32 in_stream, void *cookie,
								  off_t *inout_filepos, char *in_packetPointer,
								  int32 *inout_packetLength,
								  char **out_bufferStart,
								  int32 *out_bufferLength,
								  media_header *out_mh) = 0;
								
	// flags: B_MEDIA_SEEK_CLOSEST_FORWARD, B_MEDIA_SEEK_CLOSEST_BACKWARD,
	//        B_MEDIA_SEEK_PEEK
	virtual status_t	Seek(int32 in_stream, void *cookie,
	                         int32 in_towhat, int32 flags,
							 bigtime_t *inout_time, int64 *inout_frame,
							 off_t *inout_filePos, char *in_packetPointer,
							 int32 *inout_packetLength, bool *out_done) = 0;

#if 0
crackmonkey alert!
	// Call backs
	//	File() may be called by Setup() to get direct access to the file.

	IStorage::ptr		Source() const;

	IStorage::ptr		fSource;
	Extractor *			fSourceExtractor;
	int32				fSourceStream;
#endif

	IByteInput::ptr		SourceStream() const;
	IByteSeekable::ptr	SourceSeekable() const;
	
	Extractor *			SourceExtractor() const;
	int32				SourceStreamIndex() const;

virtual	status_t Perform(int32 selector, void * data);

	//	return positive or 0 for success, negative for error
virtual	status_t ControlFile(int32 selector, void * io_data, size_t size);

private:

	friend class MediaExtractor;

static	Extractor *		find_stream_extractor(int32 *id, media_format *format,
		                      Extractor *source_extractor, int32 source_streamnum,
    		                  int32 *numtracks, int32 *chunksize);

static Extractor *		find_extractor(int32 *id,
							IByteInput::arg stream, IByteSeekable::arg seek,
							int32 *numtracks, int32 *chunksize);

	status_t			SetSource(IByteInput::arg stream, IByteSeekable::arg seek);
	status_t			SetSourceExtractor(Extractor * extractor, int32 index);

	Extractor(const Extractor&);
	Extractor& operator=(const Extractor&);

	IByteInput::ptr		mSourceStream;
	IByteSeekable::ptr	mSourceSeekable;
	Extractor *			mSourceExtractor;
	int32				mSourceStreamIndex;

	uint32 _reserved_Extractor_[16];

		status_t _Reserved_Extractor_0(int32 arg, ...);
virtual	status_t _Reserved_Extractor_1(int32 arg, ...);
virtual	status_t _Reserved_Extractor_2(int32 arg, ...);
virtual	status_t _Reserved_Extractor_3(int32 arg, ...);
virtual	status_t _Reserved_Extractor_4(int32 arg, ...);
virtual	status_t _Reserved_Extractor_5(int32 arg, ...);
virtual	status_t _Reserved_Extractor_6(int32 arg, ...);
virtual	status_t _Reserved_Extractor_7(int32 arg, ...);
virtual	status_t _Reserved_Extractor_8(int32 arg, ...);
virtual	status_t _Reserved_Extractor_9(int32 arg, ...);
virtual	status_t _Reserved_Extractor_10(int32 arg, ...);
virtual	status_t _Reserved_Extractor_11(int32 arg, ...);
virtual	status_t _Reserved_Extractor_12(int32 arg, ...);
virtual	status_t _Reserved_Extractor_13(int32 arg, ...);
virtual	status_t _Reserved_Extractor_14(int32 arg, ...);
virtual	status_t _Reserved_Extractor_15(int32 arg, ...);
virtual	status_t _Reserved_Extractor_16(int32 arg, ...);
virtual	status_t _Reserved_Extractor_17(int32 arg, ...);
virtual	status_t _Reserved_Extractor_18(int32 arg, ...);
virtual	status_t _Reserved_Extractor_19(int32 arg, ...);
virtual	status_t _Reserved_Extractor_20(int32 arg, ...);
virtual	status_t _Reserved_Extractor_21(int32 arg, ...);
virtual	status_t _Reserved_Extractor_22(int32 arg, ...);
virtual	status_t _Reserved_Extractor_23(int32 arg, ...);
virtual	status_t _Reserved_Extractor_24(int32 arg, ...);
virtual	status_t _Reserved_Extractor_25(int32 arg, ...);
virtual	status_t _Reserved_Extractor_26(int32 arg, ...);
virtual	status_t _Reserved_Extractor_27(int32 arg, ...);
virtual	status_t _Reserved_Extractor_28(int32 arg, ...);
virtual	status_t _Reserved_Extractor_29(int32 arg, ...);
virtual	status_t _Reserved_Extractor_30(int32 arg, ...);
virtual	status_t _Reserved_Extractor_31(int32 arg, ...);
virtual	status_t _Reserved_Extractor_32(int32 arg, ...);
virtual	status_t _Reserved_Extractor_33(int32 arg, ...);
virtual	status_t _Reserved_Extractor_34(int32 arg, ...);
virtual	status_t _Reserved_Extractor_35(int32 arg, ...);
virtual	status_t _Reserved_Extractor_36(int32 arg, ...);
virtual	status_t _Reserved_Extractor_37(int32 arg, ...);
virtual	status_t _Reserved_Extractor_38(int32 arg, ...);
virtual	status_t _Reserved_Extractor_39(int32 arg, ...);
virtual	status_t _Reserved_Extractor_40(int32 arg, ...);
virtual	status_t _Reserved_Extractor_41(int32 arg, ...);
virtual	status_t _Reserved_Extractor_42(int32 arg, ...);
virtual	status_t _Reserved_Extractor_43(int32 arg, ...);
virtual	status_t _Reserved_Extractor_44(int32 arg, ...);
virtual	status_t _Reserved_Extractor_45(int32 arg, ...);
virtual	status_t _Reserved_Extractor_46(int32 arg, ...);
virtual	status_t _Reserved_Extractor_47(int32 arg, ...);

};

} } // B::Private
#endif //_MEDIA2_EXTRACTOR_PRIVATE_
