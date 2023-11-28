#ifndef _MEDIA2_FILE_H_
#define	_MEDIA2_FILE_H_

#include <kernel/image.h>
#include <support2/SupportDefs.h>
#include <support2/PositionIO.h>
#include <storage2/StorageDefs.h>
#include <storage2/File.h>
#include <media2/MediaDefs.h>
#include <media2/MediaFormats.h>

namespace B {
namespace Private {
	class MediaExtractor;
	class MediaWriter;
	class Detractor;
}
namespace Media2 {
namespace Media2Private {
	class _AddonManager;
}

using namespace Support2;
using namespace Storage2;

// forward declarations 
class BMediaTrack;
#if 0
class BParameterWeb;
class BView;
#endif


// flags for the BMediaFile constructor
enum {
	B_MEDIA_FILE_REPLACE_MODE    = 0x00000001,
	B_MEDIA_FILE_NO_READ_AHEAD   = 0x00000002,
	B_MEDIA_FILE_UNBUFFERED      = 0x00000006,
	B_MEDIA_FILE_BIG_BUFFERS     = 0x00000008
};

// flags for BMediaFile::CreateTrack()
enum {
	B_CODEC_INHIBIT_RAW_ENCODER  = 0x00000001
};

// BMediaFile represents a media file (AVI, Quicktime, MPEG, AIFF, WAV, etc)
//
// To read a file you construct a BMediaFile with an entry_ref, get the
// BMediaTracks out of it and use those to read the data.
//
// To write a file you construct a BMediaFile with an entry ref and an id as
// returned by get_next_file_format().   You then CreateTrack() to create
// various audio & video tracks.  Once you're done creating tracks, call
// CommitHeader(), then write data to each track and call CloseFile() when
// you're done.
//

class BMediaFile {

public:
					//	these four constructors are used for read-only access
					BMediaFile(	const entry_ref *ref); 
					BMediaFile(	IStorage::arg source);
					BMediaFile(	const entry_ref * ref,
								int32 flags);
					BMediaFile(	IStorage::arg source,
								int32 flags);

					//	these three constructors are for write access
					BMediaFile(const entry_ref *ref,
							   const media_file_format * mfi,
							   int32 flags=0);
					BMediaFile(IStorage::arg destination,
							   const media_file_format * mfi,
							   int32 flags=0);
					BMediaFile(const media_file_format * mfi, // set file later using SetTo()
							   int32 flags=0);

					status_t SetTo(const entry_ref *ref);
					status_t SetTo(IStorage::arg destination);

	virtual			~BMediaFile();

	status_t		InitCheck() const;

	// Get info about the underlying file format.
	status_t		GetFileFormatInfo(media_file_format *mfi) const;

	//
	// These functions are for read-only access to a media file.  
	// The data is read using the BMediaTrack object.
	//
	const char 		*Copyright(void) const;
	int32			CountTracks() const;

	// Can be called multiple times with the same index.  You must call
	// ReleaseTrack() when you're done with a track.
	BMediaTrack 	*TrackAt(int32 index);

	// Release the resource used by a given BMediaTrack object, to reduce
	// the memory usage of your application. The specific 'track' object
	// can no longer be used, but you can create another one by calling
	// TrackAt() with the same track index.
	status_t		ReleaseTrack(BMediaTrack *track);

	// A convenience.
	status_t		ReleaseAllTracks(void);


	// Create and add a track to the media file
	BMediaTrack 	*CreateTrack(media_format *mf, const media_codec_info *mci, uint32 flags=0);
	// Create and add a raw track to the media file (it has no encoder)
	BMediaTrack 	*CreateTrack(media_format *mf, uint32 flags=0);

	// Lets you set the copyright info for the entire file
	status_t		AddCopyright(const char *data);

	// Call this to add user-defined chunks to a file (if they're supported)
	status_t		AddChunk(int32 type, const void *data, size_t size);

	// After you have added all the tracks you want, call this
	status_t		CommitHeader(void);

	// After you have written all the data to the track objects, call this
	status_t        CloseFile(void);

#if 0
	// +++ to be replaced by an IMediaControllable accessor! +++
	// This is for controlling file format parameters
	
	// returns a copy of the parameter web
	status_t		GetParameterWeb(BParameterWeb** outWeb);
	status_t 		GetParameterValue(int32 id,	void *valu, size_t *size);
	status_t		SetParameterValue(int32 id,	const void *valu, size_t size);
	BView			*GetParameterView();
#endif

	// For the future...
	virtual	status_t Perform(int32 selector, void * data);

	//	Please make really sure you know which extractor/writer you're talking
	//	to before using this function.
	status_t		ControlFile(int32 selector, void * io_data, size_t size);

private:
	B::Private::MediaExtractor *fExtractor;
	B::Private::Detractor		*fDetractor;
	image_id				fDetractorImage;
	int32					fTrackNum;
	status_t				fErr;

	Media2Private::_AddonManager *fEncoderMgr;
	Media2Private::_AddonManager *fWriterMgr;
	B::Private::MediaWriter	*fWriter;
	int32					fWriterID;
	media_file_format		fMFI;

	bool					fFileClosed;
	bool					_reserved_was_fUnused[3];
	BVector<BMediaTrack *>	*fTrackList;

	void					Init();
	void					InitReader(IStorage::arg source, int32 flags = 0);
	void					InitWriter(IStorage::arg destination, const media_file_format * mfi,
									   int32 flags);

	BMediaFile();
	BMediaFile(const BMediaFile&);
	BMediaFile& operator=(const BMediaFile&);

#if 0
	// deprecated
	BParameterWeb	*Web();
#endif
	BFile::ptr				fFile;
	BPositionIO::ptr		fPositionIO;

	/* fbc data and virtuals */

	uint32 _reserved_BMediaFile_[31];

virtual	status_t _Reserved_BMediaFile_0(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_1(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_2(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_3(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_4(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_5(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_6(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_7(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_8(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_9(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_10(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_11(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_12(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_13(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_14(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_15(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_16(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_17(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_18(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_19(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_20(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_21(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_22(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_23(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_24(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_25(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_26(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_27(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_28(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_29(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_30(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_31(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_32(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_33(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_34(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_35(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_36(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_37(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_38(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_39(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_40(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_41(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_42(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_43(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_44(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_45(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_46(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_47(int32 arg, ...);
};

} } // B::Media2
#endif //_MEDIA2_FILE_H_
