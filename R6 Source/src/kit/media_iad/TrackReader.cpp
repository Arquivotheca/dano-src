
#include "TrackReader.h"
#include <RealtimeAlloc.h>
#include <File.h>
#include <assert.h>


BTrackReader::BTrackReader(BMediaTrack * track, const media_raw_audio_format & fmt) :
	m_track(track), m_fmt(fmt), m_file(NULL)
{
	m_bufSize = m_fmt.buffer_size;
	if (m_bufSize < 1) {
		m_bufSize = B_PAGE_SIZE;
	}
	m_bufUnused = m_inBuffer = 0;
	m_buffer = 0;
	m_frameSize = (m_fmt.format & 0xf) * m_fmt.channel_count;
	if (m_frameSize == 0) m_frameSize = 4;
}

BTrackReader::BTrackReader(BFile * file, const media_raw_audio_format & fmt) :
	m_track(NULL), m_fmt(fmt), m_file(file)
{
	m_bufSize = m_fmt.buffer_size;
	if (m_bufSize < 1) {
		m_bufSize = B_PAGE_SIZE;
	}
	m_bufUnused = m_inBuffer = 0;
	m_buffer = 0;
	m_frameSize = (m_fmt.format & 0xf) * m_fmt.channel_count;
	if (m_frameSize == 0) m_frameSize = 4;
}

BTrackReader::~BTrackReader()
{
	rtm_free(m_buffer);
	delete m_file;
}

ssize_t
BTrackReader::ReadFrames(void * dest, int32 frameCount)
{
	ssize_t total = 0;
	ssize_t temp = 0;
	while ((temp = read_glob(dest, frameCount-total)) > 0) {
		total += temp;
		dest = ((char *)dest)+temp*FrameSize();
	}
	if (temp == B_LAST_BUFFER_ERROR) {
		temp = B_OK;
	}
	return (total > 0) ? total : (temp < 0) ? temp : 0;
}

ssize_t
BTrackReader::FrameSize()
{
	return m_frameSize;
}

ssize_t
BTrackReader::SampleSize()
{
	return m_fmt.format & 0xf;
}

BMediaTrack *
BTrackReader::Track()
{
	return m_track;
}

ssize_t
BTrackReader::CountFrames()
{
	int64 frameCount;
	if (m_track) {
		frameCount = m_track->CountFrames();
	}
	else {
		assert(m_file != 0);
		off_t pos = m_file->Position();
		frameCount = m_file->Seek(0, 2);
		m_file->Seek(pos, 0);
		frameCount /= m_frameSize;
	}
		//	clamp to something that will not wrap when calculating size
	if (frameCount > LONG_MAX/FrameSize()) return LONG_MAX/FrameSize();
	return frameCount;
}

ssize_t
BTrackReader::read_glob(void * dest, int32 frameCount)
{
	ssize_t ret = 0;
	if (m_file != 0) {
		ret = m_file->Read(dest, frameCount*m_frameSize);
		if (ret > 0) {
			ret /= m_frameSize;
		}
		return ret;
	}
	//	empty buffer, if any there
	if (m_bufUnused > 0) {
		ssize_t toCopy = frameCount;
		if (toCopy > m_bufUnused) toCopy = m_bufUnused;
		memcpy(dest, m_buffer+(m_inBuffer-m_bufUnused)*m_frameSize,
				toCopy*m_frameSize);
		m_bufUnused -= toCopy;
		ret += toCopy;
		frameCount -= toCopy;
		dest = ((char *)dest)+toCopy*m_frameSize;
	}
	if ((frameCount <= 0) || (ret < 0)) {
		return ret;
	}
	//	read as much as we can directly from file
	bool fileDone = false;
	while (frameCount*FrameSize() >= m_bufSize) {
		int64 nRead = 0;
		status_t err = m_track->ReadFrames((char *)dest, &nRead);
		if (err < B_OK) {
			ret = (ret > 0) ? ret : err;
			fileDone = true;
			break;
		}
		if (nRead == 0) {
			fileDone = true;
		}
		dest = ((char *)dest) + nRead*m_frameSize;
		frameCount -= nRead;
		ret += nRead;
	}
	if ((frameCount <= 0) || (ret < 0) || (fileDone)) {
		return ret;
	}
	//	read new block into buffer
	if (!m_buffer) {
		m_buffer = (char *)rtm_alloc(NULL, m_bufSize);
		if (m_buffer == 0) {
			return (ret > 0) ? ret : B_NO_MEMORY;
		}
	}
	{
		//	... read ...
		int64 nRead = 0;
		status_t err = m_track->ReadFrames(m_buffer, &nRead);
		if (err < B_OK) {
			ret = (ret > 0) ? ret : err;
		}
		else {
			//	... copy ...
			m_inBuffer = nRead;
			m_bufUnused = m_inBuffer;
			ssize_t toCopy = frameCount;
			if (toCopy > m_bufUnused) {
				toCopy = m_bufUnused;
			}
			memcpy(dest, m_buffer, toCopy*m_frameSize);
			ret += toCopy;
			frameCount -= toCopy;
			m_bufUnused -= toCopy;
		}
	}
	return ret;
}

const media_raw_audio_format &
BTrackReader::Format() const
{
	return m_fmt;
}

status_t
BTrackReader::SeekToFrame(
	int64 * ioFrame)
{
	m_inBuffer = m_bufUnused = 0;
	if (m_file != 0) {
		off_t where = m_file->Seek(*ioFrame * m_frameSize, 0);
		if (where < 0) return where;
		*ioFrame = where / m_frameSize;
		return B_OK;
	}
	return m_track->SeekToFrame(ioFrame, B_MEDIA_SEEK_CLOSEST_BACKWARD);
}

BFile *
BTrackReader::File()
{
	return m_file;
}
