#ifndef _MEDIA2_FILEWRITER_PRIVATE_
#define _MEDIA2_FILEWRITER_PRIVATE_

#include <support2/SupportDefs.h>
#include <support2/IByteStream.h>
#include <OS.h>
#include <unistd.h>

namespace B {
namespace Media2 {

using namespace Support2;

class BDataIO;
class BPositionIO;

class FileWriter {
	public:
		FileWriter(IByteInput::arg in, IByteOutput::arg out, IByteSeekable::arg seek);
		~FileWriter();
		status_t	InitCheck();
		status_t	SetBufferSize(size_t buffersize);		/* default 512 KB */
		status_t	SetBlockAligmentMask(uint32 alignmask);	/* default 0x3ff */
		
		status_t Read(void *buffer, size_t size);
		status_t Write(const void *buffer, size_t size);
		status_t Skip(ssize_t size);
		off_t	Seek(off_t position, uint32 seek_mode = SEEK_SET);
		off_t	Position();
		status_t Flush();

	private:
static	int32	WriterThread(FileWriter *fw);
		void	WriterLoop();
	
		void SeekBuffer(off_t position);
		void InsertToBuffer(off_t position, const void *srcptr, ssize_t size);
		
		ssize_t		ReadAt(off_t position, void * buffer, size_t size);
		status_t	WriteAt(off_t position, const void * buffer, size_t size);
	
		status_t	init_status;
		IByteInput::ptr		in_stream;
		IByteOutput::ptr	out_stream;
		IByteSeekable::ptr	seekable;
		off_t pos;
		off_t filesize;
		off_t stream_pos;
		int current_buffer;
		uint8 *buffer[2];
		off_t bufferpos[2];
		size_t buffersize[2];
		size_t maxbuffersize;
		uint32 alignmask;
		bool	allocate_buffer_on_write;
		
		thread_id	writer_thread;
		sem_id		free_buffer;
		sem_id		full_buffer;
};

} } // B::Media2
#endif //_MEDIA2_FILEWRITER_PRIVATE_
