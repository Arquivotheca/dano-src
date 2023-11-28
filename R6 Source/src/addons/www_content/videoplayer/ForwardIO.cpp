#include <stdio.h>
#include <stdlib.h>
#include "ForwardIO.h"


ForwardIO::ForwardIO(ssize_t size)
	:	BPositionIO(),
		m_SeekPosition(0),
		m_list()
{
	Buffer * buf = new Buffer;
	if(size>0)
	{
		ssize_t ssize =  size;
		ssize += B_PAGE_SIZE;
		ssize = ssize / B_PAGE_SIZE;
		ssize = ssize * B_PAGE_SIZE;
		buf->size = ssize;
		buf->global_size = ssize;
		
		
		buf->id = create_area("IO alloc",
							  &buf->pt,
							  B_ANY_ADDRESS,
							  ssize,
							  B_NO_LOCK,
							  B_READ_AREA|B_WRITE_AREA);
							  
		if(buf->id < 0)
		{
			printf("Error alloc area\n");
		}
	}
	else
	{
		
		ssize_t ssize = 50 * B_PAGE_SIZE;
		buf->size = ssize;
		buf->global_size = ssize;
		
		
		buf->id = create_area("IO alloc",
							  &buf->pt,
							  B_ANY_ADDRESS,
							  ssize,
							  B_NO_LOCK,
							  B_READ_AREA|B_WRITE_AREA);
							  
		if(buf->id < 0)
		{
			printf("Error alloc area\n");
		}
	}
	buf->size_written = 0;
	m_list.AddItem(buf);
	
}

ForwardIO::~ForwardIO()
{
	long int j = 0;
	for(int32 i=0;i<m_list.CountItems();i++)
	{
		Buffer * buf = (Buffer*)m_list.RemoveItem(j);
		if(buf != NULL)
		{
			delete_area(buf->id);
		}
		delete buf;
	}	
	
}




ssize_t ForwardIO::Write(const void *buffer, size_t Size)
{
	//printf(" ForwardIO::Write %ld\n",Size);

	Buffer * buf = (Buffer*)m_list.LastItem();

	char * pt = (char*) buf->pt;
	pt += buf->size_written;
	
	char * src = (char*)buffer;
	
	
	size_t size = Size;
	size_t size_left = buf->size-buf->size_written;
	//printf("size_left %ld sizetocopy %ld \n",size_left,size);
	
	if((size_left)>= size)
	{
		memcpy(pt,src,size);
		buf->size_written += size;
		size -= size;
		//printf("size_written %ld \n",(int32)buf->size_written);
	}
	else
	{
		memcpy(pt,src,buf->size-buf->size_written);
		//printf("size_written %ld \n",(int32)buf->size_written);
		src += buf->size-buf->size_written;
		size -= buf->size-buf->size_written;
		buf->size_written += buf->size-buf->size_written;
		while(size >0)
		{
			Buffer * buff = new Buffer;	
			ssize_t ssize = 50 * B_PAGE_SIZE;
			buff->size = ssize;
			buff->global_size = buf->global_size + ssize;
		
		
			buff->id = create_area("IO alloc",
							  	   &buff->pt,
							  	   B_ANY_ADDRESS,
							  	   ssize,
							  	   B_NO_LOCK,
							 	   B_READ_AREA|B_WRITE_AREA);
							  
			if(buff->id < 0)
			{
				printf("Error alloc area\n");
			}
	
			buff->size_written = 0;
			m_list.AddItem(buff);
			
			pt = (char*) buff->pt;
			
			if((buff->size)>=size)
			{
				memcpy(pt,src,size);
				buff->size_written += size;
				size -= size;
			}
			else
			{
				memcpy(pt,src,buff->size);
				size -= buff->size;
				buff->size_written = buff->size;
				src += buff->size;
			}
			buf = buff;
		}
	
	}
}



ssize_t ForwardIO::Read(void *buffer, size_t Size)
{
	//printf("ForwardIO::Read\n");
	ssize_t position,taille;
	Buffer * buf = (Buffer*)m_list.LastItem();
	int32 size = Size;

	if((buf->global_size-(buf->size-buf->size_written)) <= m_SeekPosition)
	{
		return B_ERROR;
	}
	
	
	if((buf->global_size-(buf->size-buf->size_written)) <= (m_SeekPosition+size))
	{
		size = (buf->global_size-(buf->size-buf->size_written)) - m_SeekPosition;
	}
	
	taille = size;
	
	buf = (Buffer*)m_list.FirstItem();
	int32 i = 0;
	while(buf->global_size<=m_SeekPosition)
	{
		i ++;
		buf = (Buffer*)m_list.ItemAt(i);	
	}
	
	position = buf->size - (buf->global_size - m_SeekPosition);
	
	char * pt = (char*)buf->pt;
	char * src = (char*)buffer;
	pt += position; 
	  
	if((buf->global_size - m_SeekPosition) >= size)
	{
		memcpy(src,pt,size);
		size -= size;
	}
	else
	{
		memcpy(src,pt,buf->global_size - m_SeekPosition);
		size -= buf->global_size - m_SeekPosition;
		src += buf->global_size - m_SeekPosition;
		while(size > 0)
		{
			i++;
			buf = (Buffer*)m_list.ItemAt(i);	
			
			pt = (char*)buf->pt;
			if((buf->global_size ) >= size)
			{
				memcpy(src,pt,size);
				size -= size;
			}
			else
			{
				memcpy(src,pt,buf->global_size);
				size -= buf->global_size;
				src += buf->global_size;
			}		
		}
		
		
	}

	m_SeekPosition += taille;

	return taille;
}

ssize_t ForwardIO::ReadAt(off_t pos, void *buffer, size_t size)
{
	//printf("ForwardIO::ReadAt\n");
	ssize_t err = Seek(pos,SEEK_SET);
	if(err >= 0)
	{
		err = Read(buffer,size);
	}
	return err;
}



ssize_t ForwardIO::WriteAt(off_t pos, const void *buffer, size_t size)
{
	return B_ERROR;
}

off_t ForwardIO::Seek(off_t offset, uint32 mode)
{
	//printf("ForwardIO::Seek %ld ",(int32)offset);
	switch (mode) {
		case SEEK_SET:
			//printf(" SEEK_SET");
			m_SeekPosition = offset;
			break;
			
		case SEEK_CUR:
			//printf(" SEEK_CUR");
			m_SeekPosition += offset;
			break;
			
		case SEEK_END:
		{
			//printf(" SEEK_END");
			Buffer  * buf = (Buffer*)m_list.LastItem();
			m_SeekPosition = buf->global_size-(buf->size-buf->size_written)-1 + offset;
			break;
		}	
		default:
			return B_ERROR;
	}
	//printf("%ld \n",(int32)m_SeekPosition);
	return m_SeekPosition;
	
}

off_t ForwardIO::Position() const
{
	//printf("ForwardIO::Position()\n");
	return m_SeekPosition;
}

void ForwardIO::Info()
{
	printf("CountItems %ld\n",m_list.CountItems());
	Buffer  * buf = (Buffer*)m_list.LastItem();
	printf("global_size %ld \n",(int32)buf->global_size);
	printf("size %ld \n",(int32)buf->size);
	printf("size_written %ld \n",(int32)buf->size_written);
}

