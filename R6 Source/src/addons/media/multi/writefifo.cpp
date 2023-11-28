#include "writefifo.h"
#include <stdlib.h>
#include <string.h>

WriteFifo::WriteFifo(BDataIO *out) : 
	io(out)
{
	buf = (char *)malloc(128*1024);
	s = create_sem(0, "write FIFO");
	resume_thread(t = spawn_thread(ThreadEntry, "write FIFO", 20, this));
	count = 0;
}

WriteFifo::~WriteFifo()
{
	delete_sem(s);
	status_t status;
	wait_for_thread(t, &status);
	free(buf);
	delete io;
}

status_t
WriteFifo::PutData(void * data, size_t size)
{
	int tw = size;
	while (size > 0){
		if (size+count > 128*1024)
			tw = 128*1024-count;
		memcpy(buf+count, data, tw);
		size -= tw;
		count += tw;
		if (count >= 128*1024)
			count = 0;
		release_sem_etc(s, tw, 0);
		data = ((char *)data+tw);
	}
	return B_OK;
}

status_t
WriteFifo::ThreadEntry(void * w)
{
	return ((WriteFifo *)w)->Thread();
}

status_t
WriteFifo::Thread()
{
	int pos = 0;
	while (acquire_sem_etc(s, 64*1024, 0, 0) >= 0) {
		io->Write(buf+pos, 64*1024);
		pos += 64*1024;
		if (pos >= 128*1024) pos = 0;
	}
	return B_OK;
}

