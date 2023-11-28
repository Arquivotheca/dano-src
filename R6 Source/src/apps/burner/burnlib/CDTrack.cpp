
#include <string.h>
#include "CDTrack.h"
#include "CDDataSource.h"

CDTrack::CDTrack(CDDataSource *data)
{
	this->data = NULL;
	next = NULL;
	lba = 0;
	index = 0;
	blockcount = 0;
	blocksize = 0;
	deadblocks = 0;
	datalength = 0;
	SetDataSource(data);
}

CDTrack::~CDTrack()
{
	delete data;
}

CDTrack *CDTrack::FindPrevious(CDTrack *head, CDTrack *track, bool audio_only)
{
	CDTrack *prev = head;

	if (head == track) {
		// if we want the track previous to the head, we want the
		// last track, or in other words, the track where track->Next() == NULL
		track = NULL;
	}
	
	while (prev != NULL) {
		if (prev->Next() == track) {
			if (audio_only && prev->Next() != NULL
				&& prev->Next()->IsData() && prev->IsData())
			{
				prev = FindPrevious(head, prev, audio_only);
			}
			// prev is what we're looking for
			break;
		}
		prev = prev->Next();
	}
	return prev;
}


bool CDTrack::IsData(void)
{
	return type == cd_data ? true : false;
}

void 
CDTrack::SetDataSource(CDDataSource *data)
{
	if (this->data != NULL){
		delete this->data; // free old data source
	}
	
	this->data = data;
	if(data->IsAudio()){
		type = cd_audio;
		blocksize = 2352;
	} else {
		type = cd_data;
		blocksize = 2048;
	}
	blockcount = data->Length() / blocksize;
	if(blockcount * blocksize < data->Length()) blockcount++;

	datalength = data->Length();
		
	// default 2sec pregap
	deadblocks = pre_gap = 150;	
}

CDDataSource *
CDTrack::DataSource()
{
	return data;
}

void 
CDTrack::SyncWithDataSource()
{
	size_t currLen = data->Length();
	if (currLen != datalength) {
		blockcount = currLen / blocksize;
		if(blockcount * blocksize < currLen) blockcount++;
		datalength = currLen;
	}
}



void 
CDTrack::SetPreGap(uint32 frames, uint32 _deadblocks)
{
	pre_gap = frames;
	deadblocks = _deadblocks;
}

void 
CDTrack::SetPreGap(uint32 frames)
{
	pre_gap = frames;
	deadblocks = frames;
}

uint32 
CDTrack::PreGap(void)
{
	return pre_gap;
}

void 
CDTrack::PreGap(uint32 *dataFrames, uint32 *emptyFrames)
{
	*dataFrames = pre_gap - deadblocks;
	*emptyFrames = deadblocks;
}


uint32 
CDTrack::Length(void)
{
	return blockcount + deadblocks;
}

uint32 
CDTrack::FrameSize(void)
{
	return blocksize;
}

void 
CDTrack::FrameData(off_t frame, size_t count, void *_buf)
{
	uchar *buf = (uchar *) _buf;
	
	// copy deadblocks if we're at the start of the track
	while(count && (frame < deadblocks)){
		memset(buf,0,blocksize);
		frame++;
		buf += blocksize;
		count--;
	}
	if(!count) return;
	
	// adjust to start of data source
	frame -= deadblocks;
	
	// figure byte-relative addressing for the datasource
	off_t start = frame * blocksize;
	size_t len = count * blocksize;
	
	if(start >= datalength){
		memset(buf,0,count*blocksize);
	} else {
		if(start + len > datalength){
			data->Read(buf, datalength - start, start);
			memset(buf + (datalength - start), 0, len - (datalength - start));  
		} else {
			data->Read(buf, len, start);
		}
	}
}

int32 
CDTrack::LBA(void)
{
	return lba;
}

void 
CDTrack::MSF(uint32 &min, uint32 &sec, uint32 &frame)
{
	frame = lba + 150;
	
	min = sec = 0;
	
	if(frame >= 75){
		sec = (frame / 75);
		frame = (frame % 75);
		if(sec >= 60){
			min = (sec / 60);
			sec = (sec % 60);
		}
	} 
}

