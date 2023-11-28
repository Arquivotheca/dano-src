#include "FileWriter.h"
#include <DataIO.h>
#include <malloc.h>
#include <OS.h>

#include <stdio.h>
#include <string.h>

FileWriter::FileWriter(BPositionIO *f)
{
	outposio = f;
	maxbuffersize = 0;
	allocate_buffer_on_write = true;
	alignmask = 0x3ff;
	current_buffer = 0;
	pos = 0;
	init_status = B_NO_ERROR;
	filesize = f->Seek(0, SEEK_END);
	for(int i = 0; i < 2; i++) {
		buffer[i] = NULL;
		buffersize[i] = 0;
		bufferpos[i] = 0;
	}
	full_buffer = create_sem(0, "full filewriter buffer");
	if(full_buffer < 0) {
		init_status = full_buffer;
		goto err2;
	}
	free_buffer = create_sem(1, "free filewriter buffer");
	if(free_buffer < 0) {
		init_status = free_buffer;
		goto err3;
	}
	writer_thread = spawn_thread((thread_func)WriterThread, "filewriter thread",
	                             B_NORMAL_PRIORITY, this);
	if(writer_thread < 0) {
		init_status = writer_thread;
		goto err4;
	}
	
	resume_thread(writer_thread);

	return;

err4:
	delete_sem(free_buffer);
err3:
	delete_sem(full_buffer);
err2:
	if(init_status == B_NO_ERROR)
		init_status = B_ERROR;
}


FileWriter::~FileWriter()
{
	if(writer_thread >= 0) {
		Flush();
		delete_sem(full_buffer);
		while(acquire_sem(free_buffer) == B_NO_ERROR)
			;
		status_t dummy;
		wait_for_thread(writer_thread, &dummy);
		for(int i = 0; i < 2; i++) {
			if(buffer[i])
				free(buffer[i]);
		}
	}

}

status_t 
FileWriter::InitCheck()
{
	return init_status;
}

status_t 
FileWriter::SetBufferSize(size_t newbuffersize)
{
	status_t err;
	uint8 *new_buffers[2];
	
	if(newbuffersize & alignmask)
		return B_BAD_VALUE;
	
	err = Flush();
	if(err != B_NO_ERROR)
		return err;
	for(int i = 0; i < 2; i++) {
		if(newbuffersize == 0) {
			new_buffers[i] = NULL;
		}
		else {
			new_buffers[i] = (uint8*)malloc(newbuffersize);
			if(new_buffers[i] == NULL)
				err = B_NO_MEMORY;
		}
	}
	if(err != B_NO_ERROR) {
		for(int i = 0; i < 2; i++) {
			if(new_buffers[i])
				free(new_buffers[i]);
		}
		return err;
	}
	maxbuffersize = newbuffersize;
	for(int i = 0; i < 2; i++) {
		if(buffer[i])
			free(buffer[i]);
		buffer[i] = new_buffers[i];
		buffersize[i] = 0;
		bufferpos[i] = 0;
	}
	
	allocate_buffer_on_write = false;

	return B_NO_ERROR;
}

status_t 
FileWriter::SetBlockAligmentMask(uint32 newalignmask)
{
	if(maxbuffersize & newalignmask)
		return B_BAD_VALUE;
	alignmask = newalignmask;
	return B_NO_ERROR;
}


status_t
FileWriter::Flush()
{
	status_t err;
	err = acquire_sem(free_buffer);
	if(err != B_NO_ERROR) {
		printf("FileWriter::Flush: acquire_sem falied\n");
		return B_ERROR;
	}

	for(int i = 0; i < 2; i++) {
		if(buffersize[i] > 0) {
			ssize_t written;
			written = outposio->WriteAt(bufferpos[i], buffer[i], buffersize[i]);
			if(written != (ssize_t)buffersize[i]) {
				printf("FileWriter::Flush - write failed\n");
				err = B_ERROR;
			}
			buffersize[i] = 0;
		}
	}
	release_sem_etc(free_buffer, 1, B_DO_NOT_RESCHEDULE);
	return err;
}

int32
FileWriter::WriterThread(FileWriter *fw)
{
	fw->WriterLoop();
	return 0;
}

void 
FileWriter::WriterLoop()
{
	int write_buffer = 0;
	while(acquire_sem(full_buffer) == B_NO_ERROR) {
		ssize_t written;
		written = outposio->WriteAt(bufferpos[write_buffer],
		                            buffer[write_buffer],
		                            buffersize[write_buffer]);
		if(written != (ssize_t)buffersize[write_buffer]) {
			printf("FileWriter::WriterLoop - write failed\n");
		}
		buffersize[write_buffer] = 0;
		write_buffer = (write_buffer + 1) % 2;
		release_sem_etc(free_buffer, 1, B_DO_NOT_RESCHEDULE);
	}
	delete_sem(free_buffer);
}

void 
FileWriter::SeekBuffer(off_t position)
{
	if(buffersize[current_buffer] > 0) {
		release_sem_etc(full_buffer, 1, B_DO_NOT_RESCHEDULE);
		if(acquire_sem(free_buffer) != B_NO_ERROR) {
			printf("FileWriter::SeekBuffer: acquire_sem falied\n");
			return;
		}
		current_buffer = (current_buffer + 1) % 2;
	}
	//printf("seek buffer to %Ld of %Ld\n", position, filesize);
	position -= position & alignmask;	// align
	bufferpos[current_buffer] = position;
	buffersize[current_buffer] = 0;
#if 0  // moved to InsertToBuffer
	if(position < filesize) {
		ssize_t bytesread;
		bytesread = outposio->ReadAt(position, buffer[current_buffer], maxbuffersize);
		if(bytesread < 0)
			buffersize[current_buffer] = 0;
		else
			buffersize[current_buffer] = bytesread;
	}
#endif
	//printf("got buffer to %Ld size %ld\n", position, buffersize[current_buffer]);
}

void 
FileWriter::InsertToBuffer(off_t position, const void *srcptr, ssize_t size)
{
	off_t offset = position - bufferpos[current_buffer];
	if(offset < 0 || offset + size > maxbuffersize) {
		printf("FileWriter::InsertToBuffer - internal error\n");
		return;
	}
	
	off_t buffer_end_pos =
		bufferpos[current_buffer] +  buffersize[current_buffer];
	size_t buffer_free = maxbuffersize - buffersize[current_buffer];
	if(buffer_end_pos < filesize && buffer_free > 0) {
		ssize_t bytesread;
		bytesread = outposio->ReadAt(buffer_end_pos, buffer[current_buffer] +
		                             buffersize[current_buffer], buffer_free);
		if(bytesread > 0)
			buffersize[current_buffer] += bytesread;
	}
	
	memcpy(buffer[current_buffer]+offset, srcptr, size);
	if(offset + size > buffersize[current_buffer])
		buffersize[current_buffer] = offset + size;
}

status_t 
FileWriter::Read(void *destptr, size_t size)
{
	off_t offset = pos - bufferpos[current_buffer];
	if(offset >= 0 && offset+size <= buffersize[current_buffer]) {
		memcpy(destptr, buffer[current_buffer]+offset, size);
	}
	else {
		Flush();
		if(outposio->ReadAt(pos, destptr, size) != (ssize_t)size) {
			return B_ERROR;
		}
	}
	pos += size;
	return B_NO_ERROR;
}

status_t 
FileWriter::Write(const void *srcptr, size_t size)
{
	status_t err;
	bigtime_t t1 = system_time();

	if(allocate_buffer_on_write) {
		SetBufferSize(512*1024);
		allocate_buffer_on_write = false;
	}

	off_t offset = pos - bufferpos[current_buffer];
	//printf("FileWriter::Write: pos %Ld, size %ld\n", pos, size); 
	//printf("FileWriter::Write: offset %Ld, buffersize %ld of %ld\n",
	//       offset, buffersize[current_buffer], maxbuffersize); 
	if(size >= maxbuffersize) {
		//printf("FileWriter::Write: unbuffered\n"); 
		err = Flush();
		if(err != B_NO_ERROR)
			return err;
		if(outposio->WriteAt(pos, srcptr, size) != (ssize_t)size) {
			return B_ERROR;
		}
	}
	else if(offset >= 0 && offset < maxbuffersize && buffersize[current_buffer] > 0) {
	 	if(offset+size <= maxbuffersize) {
			InsertToBuffer(pos, srcptr, size);
		}
		else {
			ssize_t remsize = maxbuffersize - offset;
			InsertToBuffer(pos, srcptr, remsize);
			//err = Flush();
			//if(err != B_NO_ERROR)
			//	return err;
			SeekBuffer(pos+remsize);
			InsertToBuffer(pos+remsize, (uint8*)srcptr+remsize, size-remsize);
		}
	}
	else {
		//err = Flush();
		//if(err != B_NO_ERROR)
		//	return err;
		SeekBuffer(pos);
		InsertToBuffer(pos, srcptr, size);
	}
	pos += size;
	if(pos > filesize)
		filesize = pos;
	bigtime_t curr_time = system_time()-t1;
	
	static bigtime_t max_time = 0;
	static bigtime_t min_time = LONGLONG_MAX;
	static bigtime_t avg_time = 0;
	static bigtime_t time_count = 0;
	
	if(curr_time > max_time)
		max_time = curr_time;
	if(curr_time < min_time)
		min_time = curr_time;
	avg_time = (avg_time * time_count + curr_time) / (time_count+1);
	time_count++;
	
	//printf("write time curr %6Ld min %6Ld max %6Ld avg %6Ld\n",
	//       curr_time, min_time, max_time, avg_time);
	return B_NO_ERROR;
}

status_t 
FileWriter::Skip(ssize_t size)
{
	pos += size;
	return B_NO_ERROR;
}

off_t 
FileWriter::Seek(off_t position, uint32 seek_mode)
{
	//printf("FileWriter::Seek %Ld, mode %d\n", position, seek_mode);
	switch(seek_mode) {
		case SEEK_SET:
			return pos = position;
		case SEEK_CUR:
			return pos = pos + position;
		case SEEK_END:
			return pos = filesize + position;
		default:
			printf("FileWriter::Seek: Unknown seek mode %ld\n", seek_mode);
			return -1;
	}
}

off_t 
FileWriter::Position()
{
	return pos;
}

