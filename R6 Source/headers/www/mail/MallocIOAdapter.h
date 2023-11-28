/*
	MallocIOAdapter.h
*/
#ifndef _MALLOC_IO_ADAPTER_H
#define _MALLOC_IO_ADAPTER_H
#include <DataIO.h>

class MallocIOAdapter : public BMallocIO {
	public:
								MallocIOAdapter();
								
	
		virtual ssize_t			Read(void *buffer, size_t size);
		virtual	ssize_t			ReadAt(off_t pos, void *buffer, size_t size);
	
	private:
		bool					fNeedToReset;
};

#endif
