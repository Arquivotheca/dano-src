#ifndef _QTWRITER_H
#define _QTWRITER_H	1

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

namespace BPrivate {
	class _EXPORT QTWriter;
}


class QTWriterNode;
class QTWriterAddon;
class FileWriter;

namespace BPrivate {

// QTWriter is the base virtual class that must be subclassed to implement
// specific encapsulation format. Each flavour lives in its own add-on.

class QTWriter : public MediaWriter {

public:

				QTWriter();
	virtual		~QTWriter();

	status_t	SetSource(BDataIO *source);

	status_t	AddCopyright(const char *data);
	status_t	AddTrackInfo(int32 trk, uint32 code,const char *dta,size_t sz);
	
	status_t	AddTrack(BMediaTrack *track);
	status_t	AddChunk(int32 type, const char *data, size_t size);
	status_t	CommitHeader(void);

	status_t	WriteData(int32 			tracknum,
						  media_type 		type,
						  const void 		*data,
						  size_t 			size,
						  media_encode_info *info);

	status_t	CloseFile(void);


private:
	QTAtomWriter *f_qt;
	QTTrack      *f_qttracks[64];   // we only support 2 for now...
	int32         f_numtracks;
	FileWriter   *f_source;
	bool          header_committed;
};

}	//	namespace


#endif
