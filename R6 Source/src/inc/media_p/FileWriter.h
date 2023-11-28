#ifndef FILEWRITER_H
#define FILEWRITER_H

#include <SupportDefs.h>
#include <OS.h>
#include <unistd.h>

class BDataIO;
class BPositionIO;

class FileWriter {
	public:
		//FileWriter(BDataIO *f);
		FileWriter(BPositionIO *f, bool buffered=true);
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
		
		status_t InitWriterThread();
	
		status_t	init_status;
		//BDataIO *outstream;
		BPositionIO *outposio;
		off_t pos;
		off_t filesize;
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
#endif

