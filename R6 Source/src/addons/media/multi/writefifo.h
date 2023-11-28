#ifndef WRITEFIFO_H
#define WRITEFIFO_H

#include <OS.h>
#include <DataIO.h>
#include <stdlib.h>

class WriteFifo {
public:
		WriteFifo(BDataIO * out);
		~WriteFifo();
		status_t PutData(void * data, size_t size);
static		status_t ThreadEntry(void *);
		status_t Thread();
private:
		BDataIO * io;
		char * buf;
		sem_id s;
		thread_id t;
		int count;
};

#endif