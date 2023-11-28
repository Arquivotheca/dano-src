#ifndef _AVIWRITER_H
#define _AVIWRITER_H	1

#include <OS.h>
#include <SupportDefs.h>
#include <Locker.h>
#include <MediaDefs.h>

#if defined (__POWERPC__)
#define _EXPORT __declspec(dllexport)
#else
#define _EXPORT
#endif

class BFile;
class FileWriter;

namespace BPrivate {
	class _EXPORT AviWriter;
}


class AviWriterNode;
class AviWriterAddon;

namespace BPrivate {

// AviWriter is the base virtual class that must be subclassed to implement
// specific encapsulation format. Each flavour lives in its own add-on.

class AviWriter : public MediaWriter {

public:

				AviWriter();
	virtual		~AviWriter();

	status_t	SetSource(BDataIO *source);
	status_t	SetWriteBufferSize(size_t buffersize);
	
	status_t	AddCopyright(const char *data);
	status_t	AddTrackInfo(int32 trk, uint32 code,const char *dta,size_t sz);

	status_t	AddTrack(BMediaTrack *track);
	status_t	AddChunk(int32 type, const char *data, size_t size);
	status_t	CommitHeader(void);

	status_t	WriteData(int32 			tracknum,
						  media_type 		type,
						  const void 		*data,
						  size_t 			size,
						  media_encode_info	*info);

	status_t	CloseFile(void);


private:
	TRIFFWriter *f_riff;
	FileWriter	*f_out;
	bool		header_committed;
	char 		*meta_data;
	int32		meta_data_length;

	BMediaTrack *f_vid;   // we only support 1 audio and 1 video track
	BMediaTrack *f_aud;

};

}	//	namespace


#endif
