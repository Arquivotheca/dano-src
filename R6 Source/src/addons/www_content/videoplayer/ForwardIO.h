
#ifndef _IO_FORWARD_H
#define _IO_FORWARD_H

#include <DataIO.h>
#include <OS.h>
#include <List.h>


struct Buffer
{
	ssize_t		size;
	ssize_t		global_size;
	size_t		size_written;
	void *		pt;
	area_id		id;	
};

class ForwardIO : public BPositionIO
{
	
public:
	ForwardIO(ssize_t size); 
	~ForwardIO();
	
	

virtual	ssize_t		Read(void *buffer, size_t size) ;
virtual	ssize_t		Write(const void *buffer, size_t size);

virtual	ssize_t		ReadAt(off_t pos, void *buffer, size_t size) ;
virtual	ssize_t		WriteAt(off_t pos, const void *buffer, size_t size);

virtual off_t		Seek(off_t position, uint32 seek_mode);
virtual	off_t		Position() const; 

virtual status_t	SetSize(off_t size) { size = 0; return B_ERROR; }

void				Info();

private:
	ssize_t					m_BufferSize;
	ssize_t					m_SeekPosition;
	int32					m_WaitFlags;
	BList 					m_list;
};


#endif	//	ForwardIO_h
