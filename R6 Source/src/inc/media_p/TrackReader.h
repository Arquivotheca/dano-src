
#if !defined(TrackReader_h)
#define TrackReader_h

#include <MediaTrack.h>

class BFile;

namespace BPrivate {
class BTrackReader
{
public:
		BTrackReader(BMediaTrack * track, const media_raw_audio_format & fmt);
		BTrackReader(BFile * file, const media_raw_audio_format & fmt);
		~BTrackReader();
		ssize_t ReadFrames(void * dest, int32 frameCount);
		ssize_t FrameSize();
		ssize_t SampleSize();
		BMediaTrack * Track();
		ssize_t CountFrames();
		ssize_t read_glob(void * dest, int32 frameCount);
		const media_raw_audio_format & Format() const;
		status_t SeekToFrame(int64 *ioFrame);
		BFile * File();

private:
		char * m_buffer;
		ssize_t m_inBuffer;
		ssize_t m_bufUnused;
		ssize_t m_frameSize;
		BMediaTrack * m_track;
		media_raw_audio_format m_fmt;
		size_t m_bufSize;
		BFile * m_file;
};

}
using namespace BPrivate;

#endif	//	TrackReader_h
