#ifndef _MP3_WRITER_H
#define _MP3_WRITER_H	1

#include <OS.h>
#include <SupportDefs.h>
#include <Locker.h>
#include <MediaDefs.h>

#if defined (__POWERPC__)
#define _EXPORT __declspec(dllexport)
#else
#define _EXPORT
#endif

namespace BPrivate {
	class _EXPORT MP3Writer;
}

namespace BPrivate {

// MP3Writer is the base virtual class that must be subclassed to implement
// specific encapsulation format. Each flavour lives in its own add-on.

class MP3Writer : public MediaWriter {
public:
				MP3Writer();
	virtual		~MP3Writer();

	status_t	SetSource(BDataIO *source);
	
	status_t	AddTrack(BMediaTrack *track);
	status_t	AddCopyright(const char *data);
	status_t	AddTrackInfo(int32 track, uint32 code, const char *data,
							 size_t sz);
	status_t	AddChunk(int32 type, const char *data, size_t size);
	status_t	CommitHeader(void);

	status_t	WriteData(int32 		tracknum,
						  media_type 	type,
						  const void 	*data,
						  size_t 		size,
						  media_encode_info *mei);

	status_t	CloseFile(void);


private:
	BMediaTrack 	*fTrack;		  // we only support 1 audio track
	BDataIO       *fDestination;
};

}	//	namespace


#endif
