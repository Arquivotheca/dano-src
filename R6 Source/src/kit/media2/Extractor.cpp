#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <storage2/File.h>
#include <support2/Debug.h>
#include <media2/MediaFile.h>

#include <support2/StdIO.h>
#define checkpoint \
//berr << "thid " << find_thread(0) << " -- " << __FILE__ << ":" << __LINE__ << " -- " << __FUNCTION__ << endl;

#include "Extractor.h"
#include "codec_addons.h"

namespace B {
namespace Private {

using namespace Media2::Media2Private;

Extractor *
Extractor::find_stream_extractor(int32 *id, media_format *format,
                      Extractor *source_extractor, int32 source_streamnum,
                      int32 *numtracks, int32 *chunksize)
{
//puts("find_stream_extractor()");
	if ((format->Encoding() == 0) && (format->type != B_MEDIA_RAW_AUDIO) &&
			(format->type != B_MEDIA_RAW_VIDEO)) {
		return 0;
	}

	_AddonManager *mgr = __get_extractor_manager();
	image_id       imgid;
	Extractor   *(*make_extractor)(void);
	Extractor	  *extractor = NULL;
	int32          cookie=0;

	addon_list addons;
	BMediaFormats::find_addons(format, addons);

	if (addons.size() == 0) {
		return 0;
	}

	for (int ix=0; ix<2; ix++)
	{
		bool allowLoad = (ix == 1);
		cookie = 0;
		while ((imgid = mgr->GetNextAddon(&cookie, id, allowLoad)) > 0) {
			if (get_image_symbol(imgid, "instantiate_extractor",
								 B_SYMBOL_TYPE_TEXT,
								 (void **)&make_extractor) != B_OK) {
				mgr->ReleaseAddon(*id);
				continue;
			}
			
			extractor = make_extractor();
			if (extractor == NULL) {
				mgr->ReleaseAddon(*id);
				continue;
			}
			extractor->SetSourceExtractor(source_extractor, source_streamnum);
//puts("  calling SniffFormat()");
			if (extractor->SniffFormat(format, numtracks, chunksize) == B_OK) { // got it!
				goto allDone;
			}
	
			delete extractor;
			mgr->ReleaseAddon(*id);
			extractor = NULL;
		}
	}
allDone:
//printf("  allDone! (0x%08lx)\n", (uint32)extractor);
	return extractor;
}


Extractor *
Extractor::find_extractor(
	int32 *id, IByteInput::arg stream, IByteSeekable::arg seek,
	int32 *numtracks, int32 *chunksize)
{
	_AddonManager *mgr = __get_extractor_manager();
	image_id       imgid;
	Extractor   *(*make_extractor)(void);
	Extractor	  *extractor = NULL;
	int32          cookie=0;

//puts("find_extractor()");
	for (int ix=0; ix<2; ix++)
	{
		bool allowLoad = (ix == 1);
		cookie = 0;
		while ((imgid = mgr->GetNextAddon(&cookie, id, allowLoad)) > 0) {
			if (get_image_symbol(imgid, "instantiate_extractor",
								 B_SYMBOL_TYPE_TEXT,
								 (void **)&make_extractor) != B_OK) {
				mgr->ReleaseAddon(*id);
				continue;
			}
			
			extractor = make_extractor();
			if (extractor == NULL) {
				mgr->ReleaseAddon(*id);
				continue;
			}
			
			if (extractor->SetSource(stream, seek) < B_OK) {
				delete extractor;
				continue;
			}
	
//puts("  calling Sniff()");
			if (extractor->Sniff(numtracks, chunksize) == B_OK) {  // got it!
				goto allDone;
			}

			delete extractor;
			mgr->ReleaseAddon(*id);
			extractor = NULL;
		}
	}
allDone:	
//printf("  extractor done (0x%08lx)\n", (uint32)extractor);
	return extractor;
	
}


MediaExtractor::MediaExtractor(int32 flags) : fJobLock("extractor_job")
{
	fFlags = flags;
	fSourceStream = NULL;
	fSourceSeekable = NULL;
	fReadArea = NULL;
	fChunks = NULL;
	fStreams = NULL;
	fJobs = NULL;
	fStreamNum = 0;


	fFileExtractorInfo.extractor = NULL;
	fFileExtractorInfo.extractor_id = -1;
	fFileExtractorInfo.stream_count = 0;

	fStreamExtractorInfo = NULL;
	
	fExtractors = NULL;
	fStreamNums = NULL;
}

MediaExtractor::~MediaExtractor()
{
	status_t	status;
	chunk_info	*chunk;

	if (fFileExtractorInfo.extractor) {
		_AddonManager *mgr = __get_extractor_manager();
		if(fStreamExtractorInfo) {
			for(int i = 0; i < fFileExtractorInfo.stream_count; i++) {
				if(fStreamExtractorInfo[i].extractor == NULL)
					continue;

				delete fStreamExtractorInfo[i].extractor;
				fStreamExtractorInfo[i].extractor = NULL;
				mgr->ReleaseAddon(fStreamExtractorInfo[i].extractor_id);
				fStreamExtractorInfo[i].extractor_id = -1;
			}
		}

		delete fFileExtractorInfo.extractor;
		fFileExtractorInfo.extractor = NULL;
		mgr->ReleaseAddon(fFileExtractorInfo.extractor_id);
		fFileExtractorInfo.extractor_id = -1;
	}

	free(fStreamExtractorInfo);
	free(fExtractors);
	free(fStreamNums);

	if (!fSourceStream.ptr())
		return;

	if (fSubmitJobSem > -1) delete_sem(fSubmitJobSem);
	if (fReaderThread > -1) wait_for_thread(fReaderThread, &status);

	if (fChunks) {
		for(int i=0; i<fStreamNum * 2; i++) {
			chunk = &fChunks[i];
			delete_sem(chunk->sem);
		}

		free(fChunks);
	}
	
	free(fStreams);
	free(fJobs);
	free(fReadArea);
	fSourceStream = NULL;
	fSourceSeekable = NULL;
}

status_t 
MediaExtractor::SetSource(IByteInput::arg stream, IByteSeekable::arg seek, int32 *out_streamNum)
{
	int					i;
	chunk_info			*chunk;
	status_t			err;
	int32 				streamnum = 0;

	fSourceStream = stream;
	fSourceSeekable = seek;
	
	fFileExtractorInfo.extractor =
		Extractor::find_extractor(&fFileExtractorInfo.extractor_id, stream, seek,
		               &fFileExtractorInfo.stream_count, &fChunkSize);
	if(fFileExtractorInfo.extractor == NULL) {
		err = B_MEDIA_NO_HANDLER;
		goto error1;
	}
	
	fStreamNum = fFileExtractorInfo.stream_count;


	fStreamExtractorInfo = (ExtractorInfo *)
		malloc(sizeof(ExtractorInfo) * fFileExtractorInfo.stream_count);
	if(fStreamExtractorInfo == NULL) {
		err = B_NO_MEMORY;
		goto error1;
	}
	for(int i = 0; i < fFileExtractorInfo.stream_count; i++) {
		fStreamExtractorInfo[i].extractor = NULL;
		fStreamExtractorInfo[i].extractor_id = -1;
		fStreamExtractorInfo[i].stream_count = 0;

		// look for multistream data
		media_format format;
		void *info;
		int32 info_size;
		if(fFileExtractorInfo.extractor->TrackInfo(i, &format, &info, &info_size) != B_NO_ERROR)
			continue;

		int32 chunk_size;
		fStreamExtractorInfo[i].extractor =
			Extractor::find_stream_extractor(&fStreamExtractorInfo[i].extractor_id, &format,
			                      fFileExtractorInfo.extractor, i,
		                          &fStreamExtractorInfo[i].stream_count,
		                          &chunk_size);
		if(fStreamExtractorInfo[i].extractor == NULL)
			continue;
		if(fStreamExtractorInfo[i].stream_count == 0)
			continue;
		
		if(chunk_size > fChunkSize)
			fChunkSize = chunk_size;

		//printf("found stream extractor, stream count %d\n", fStreamExtractorInfo[i].stream_count);

		fStreamNum += fStreamExtractorInfo[i].stream_count-1;
	}
	
	fStreamNums = (int32*)malloc(sizeof(int32) * fStreamNum);
	if(fStreamNums == NULL) {
		err = B_NO_MEMORY;
		goto error1;
	}
	fExtractors = (Extractor **)malloc(sizeof(Extractor *) * fStreamNum);
	if(fExtractors == NULL) {
		err = B_NO_MEMORY;
		goto error1;
	}
	for(int i = 0; i < fFileExtractorInfo.stream_count; i++) {
		if(fStreamExtractorInfo[i].stream_count > 0) {
			for(int j = 0; j < fStreamExtractorInfo[i].stream_count; j++) {
				fExtractors[streamnum] = fStreamExtractorInfo[i].extractor;
				fStreamNums[streamnum] = j;
				streamnum++;
			}
		}
		else {
			fExtractors[streamnum] = fFileExtractorInfo.extractor;
			fStreamNums[streamnum] = i;
			streamnum++;
		}
	}
	if(streamnum != fStreamNum) {
		printf("Extractor error: stream (%ld) != fStreamNum (%ld)\n",
		       streamnum, fStreamNum);
		err = B_ERROR;
		goto error1;
	}

	fFileSize = (fSourceSeekable != 0) ? fSourceSeekable->Seek(0, SEEK_END) : 0;

	// Work without knowing the size of the file.  This allows streaming.
	if (fFileSize < 0)
		fFileSize = 0;
	
	size_t readAreaSize;
	if ((fFlags & B_MEDIA_FILE_UNBUFFERED) == B_MEDIA_FILE_UNBUFFERED)
		readAreaSize = fStreamNum * fChunkSize;
	else
		readAreaSize = fStreamNum * 3 * fChunkSize;
	
	fReadArea = (char *) malloc(readAreaSize);
	if (fReadArea == NULL) {
		err = B_NO_MEMORY;
		goto error1;
	}

	fChunks = NULL;
	if ((fFlags & B_MEDIA_FILE_UNBUFFERED) != B_MEDIA_FILE_UNBUFFERED) {
		// Create chunks if this is a buffered file
		fChunks = (chunk_info *) malloc(sizeof(chunk_info) * (fStreamNum * 2 + fStreamNum));
		if (fChunks == NULL) {
			err = B_NO_MEMORY;
			goto error2;
		}
	
		for(i=0; i<fStreamNum * 2; i++) {
			chunk = &fChunks[i];
			chunk->start = fReadArea + i * fChunkSize;
			chunk->count = 1;
			chunk->offset = (off_t)-1;
			chunk->sem = create_sem(0, "extractor_chunk");
			if (chunk->sem <= 0) {
				err = chunk->sem;
				while (--i>=0)
					delete_sem(fChunks[i].sem);
				goto error2;
			}
		}
	
		for(i=0; i<fStreamNum; i++) {
			chunk = &fChunks[2*fStreamNum + i];
			chunk->start = fReadArea + (2*fStreamNum + i)*fChunkSize;
	
			// unused
			chunk->sem = (sem_id) 0;
			chunk->count = 0;
			chunk->offset = 0;
		}
	}

	fStreams = (stream_info *) malloc(sizeof(stream_info) * fStreamNum);
	if (fStreams == NULL) {
		err = B_NO_MEMORY;
		goto error3;
	}

	for(i=0; i<fStreamNum; i++) {
		stream_info * info = &fStreams[i];
		info->pos = (off_t)-1;
		info->pos_chunk = (off_t)-1;
		info->pos_offset = (off_t)-1;
		info->source_pos = (off_t)-1;
		info->chunk[0] = i*2 + 0;
		info->chunk[1] = i*2 + 1;
		info->index = 0;
		info->partial_chunk = 2*fStreamNum + i;
		info->wait[0] = false;
		info->wait[1] = false;
	}

	fJobs = NULL;
	if ((fFlags & B_MEDIA_FILE_UNBUFFERED) != B_MEDIA_FILE_UNBUFFERED) {
		// The Job structures are needed when using a buffered file,
		// even if the read ahead thread isn't running.
		fJobsNum = 2 * fStreamNum;
		fCurJob = 0;
		fJobs = (int32 *) malloc(sizeof(int32) * fJobsNum);
		if (fJobs == NULL) {
			err = B_NO_MEMORY;
			goto error4;
		}
	}

	if (!(fFlags & B_MEDIA_FILE_NO_READ_AHEAD)) {
		// Set up structure for the read ahead thread
		fSubmitJobSem = create_sem(0, "extractor_submit");
		if (fSubmitJobSem <= 0) {
			err = fSubmitJobSem;
			goto error5;
		}

		fReaderThread = spawn_thread(start_reader_thread, "extractor_reader", 20, this);
		if (fReaderThread < 0) {
			err = fReaderThread;
			goto error6;
		}
		resume_thread(fReaderThread);
	} else {
		fSubmitJobSem = -1;
		fReaderThread = -1;
	}

	for(i = 0; i < fStreamNum; i++)
		SeekToPosition(i, 0);

	*out_streamNum = fStreamNum;
	return B_OK;

error6:
	delete_sem(fSubmitJobSem);
error5:
	free(fJobs);
error4:
	free(fStreams);
error3:
	if (fChunks) {
		for(i=0; i<fStreamNum * 2; i++) {
			chunk = &fChunks[i];
			delete_sem(chunk->sem);
		}

		free(fChunks);
	}
	
error2:
	free(fReadArea);
	fReadArea = NULL;
error1:
	fSourceStream = NULL;
	fSourceSeekable = NULL;
	return err;
}

status_t 
MediaExtractor::SetSource(const media_format *, int32 *)
{
	return B_ERROR;
}

status_t 
MediaExtractor::AllocateCookie(int32 stream, void **cookieptr)
{
	if (stream < 0 || stream >= fStreamNum)
		return B_BAD_INDEX;
	Extractor *extractor = fExtractors[stream];
	int32	in_stream = fStreamNums[stream];

	return extractor->AllocateCookie(in_stream, cookieptr);
}

status_t 
MediaExtractor::FreeCookie(int32 stream, void *cookie)
{
	if (stream < 0 || stream >= fStreamNum)
		return B_BAD_INDEX;
	Extractor *extractor = fExtractors[stream];
	int32	in_stream = fStreamNums[stream];

	return extractor->FreeCookie(in_stream, cookie);
}

status_t 
MediaExtractor::GetNextChunk(int32 in_stream, void *cookie, char **out_buffer,
                             int32 *out_size, media_header *mh)
{
	stream_info			*stream;
	chunk_info			*chunk;
	chunk_info			*ochunk;
	status_t			err;
	char				*packetPointer = NULL;
	char				*bufferStart;
	int32				packetLength;
	int32				oldPacketLength;
	int32				bufferLength;
	int32				partialOffset;
	off_t				filePos;
	off_t				oldFilePos;
	media_header		bogus_mh;
	
	if (!mh)
		mh = &bogus_mh;
		
	if (in_stream >= fStreamNum)
		return B_BAD_INDEX;

	stream = &fStreams[in_stream];

	filePos = stream->pos;
	packetLength = 0;

	//printf("GetNextChunk(pos %Ld)\n", stream->pos);

	// check whether we've reached the end of the file yet
	if (fFileSize != 0 && stream->pos == fFileSize) {
		//printf("returning B_LAST_BUFFER_ERROR\n");
		return B_LAST_BUFFER_ERROR;
	}

	do {
		// make sure the right chunk is read
		if ((fFlags & B_MEDIA_FILE_UNBUFFERED) == B_MEDIA_FILE_UNBUFFERED) {
			bufferStart = NULL;
			packetLength = 0;
			err = fExtractors[in_stream]->SplitNext(
				fStreamNums[in_stream], cookie, &filePos, packetPointer, &packetLength,
				&bufferStart, &bufferLength, mh);

			if (err) {
				//	printf("Extractor::GetNextChunk(): got error from Split (%s)\n", strerror(err));
				return err;
			}
			
			if(packetLength > fChunkSize)
				return B_ERROR;

			if (packetLength <= 0)
				packetLength = fChunkSize;

			if (fSourceSeekable.ptr() && stream->source_pos != filePos)
			{
				off_t off = fSourceSeekable->Seek(filePos, SEEK_SET);
				if (off < 0) return (status_t)off;
			}
			ssize_t sz = fSourceStream->Read(fReadArea + in_stream * fChunkSize,
				packetLength);
			if (sz < 0)
				return sz;

			stream->source_pos = filePos + (off_t)sz;
			packetPointer = fReadArea;
		}
		else {
			err = ReadAhead(in_stream);
			if (err) {
				//printf("Extractor::GetNextChunk(): ReadAhead failed! (case 2)\n");
				return err;
			}

			if (fChunkSize - stream->pos_offset < packetLength) {
	
				// partial buffer...
	
				chunk = &fChunks[stream->partial_chunk];
	
				// save the partial packet
				ochunk = &fChunks[stream->chunk[stream->index % 2]];
				partialOffset = fChunkSize - stream->pos_offset;
				memcpy(chunk->start, ochunk->start + (fChunkSize - partialOffset), partialOffset);
	
				// seek to the beginning of the next chunk
				stream->pos += partialOffset;
				stream->pos_offset = 0;
				stream->pos_chunk += fChunkSize;
	
				// make sure the next chunk is read
				err = ReadAhead(in_stream);
				if (err) {
					printf("Extractor::GetNextChunk(): ReadAhead failed!\n");
					return err;
				}
	
				// determine of much of the new chunk has to be copied to complete
				// the partial packet.
	
				ochunk = &fChunks[stream->chunk[stream->index % 2]];
				memcpy(chunk->start + partialOffset, ochunk->start, packetLength - partialOffset);
				partialOffset = packetLength - partialOffset;
	
				packetPointer = chunk->start;
	
			} else {
				// complete buffer
				chunk = &fChunks[stream->chunk[stream->index % 2]];
				packetLength = fChunkSize - stream->pos_offset;
				if (fFileSize != 0 && fFileSize - filePos < packetLength)
					packetLength = fFileSize - filePos;
	
				packetPointer = chunk->start + stream->pos_offset;
			}
		}

		// call SplitNext

		oldPacketLength = packetLength;
		oldFilePos = filePos;
		bufferStart = NULL;

		err = fExtractors[in_stream]->SplitNext(
			fStreamNums[in_stream], cookie, &filePos, packetPointer, &packetLength,
			&bufferStart, &bufferLength, mh);
		
		if (err) {
			//printf("Extractor::GetNextChunk(): got error from Split (%s)\n", strerror(err));
			return err;
		}

		// a couple of sanity checks

		if (bufferStart) {
			if (bufferStart - packetPointer >= oldPacketLength) {
				printf("Extractor::GetNextChunk(): Split returns bufferStart out of range (%p not in %p-%p)\n", bufferStart, packetPointer, packetPointer+packetLength);
				return B_ERROR;
			}
			if ((bufferLength < 0) || (bufferStart + bufferLength - packetPointer > oldPacketLength)) {
				printf("Extractor::GetNextChunk(): Split returns bufferLength out of range (%.8lx not in %.8x-%.8lx)\n", bufferLength, 1, packetLength - (bufferStart - packetPointer));
				return B_ERROR;
			}
		}
		if (fFileSize != 0 && filePos > fFileSize) {
			printf("Extractor::GetNextChunk(): Split returns filePos beyond EOF! (%Lx > %Lx)\n", filePos, fFileSize);
			return B_ERROR;
		}
		if (!bufferStart) {
			if (fFileSize != 0 && filePos + packetLength > fFileSize) {
				printf("Extractor::GetNextChunk(): Split returns packetLength beyond EOF! (%Lx + %lx > %Lx)\n", filePos, packetLength, fFileSize);
				return B_ERROR;
			}
		}

		// update the file position
		// optimization: don't do very slow 64 bit division when avoidable

		if (!((filePos >= stream->pos_chunk) && (filePos - stream->pos_chunk < fChunkSize)))
			if ((filePos >= stream->pos_chunk + fChunkSize) && (filePos - stream->pos_chunk < 2*fChunkSize))
				stream->pos_chunk += fChunkSize;
			else 
				if ((filePos >= stream->pos_chunk - fChunkSize) && (filePos < stream->pos_chunk))
					stream->pos_chunk -= fChunkSize;
				else
					stream->pos_chunk = (filePos / fChunkSize) * fChunkSize;
		stream->pos_offset = filePos - stream->pos_chunk;
		stream->pos = filePos;

	} while (!bufferStart);

	// return buffer

	*out_buffer = bufferStart;
	*out_size = bufferLength;
	return B_OK;
}

status_t 
MediaExtractor::SeekTo(int32 global_stream, void *cookie,
                       int32 in_towhat, bigtime_t *inout_time,
                       int64 *inout_frame, int32 flags)
{
	stream_info			*stream;
//	chunk_info			*chunk;
//	chunk_info			*ochunk;
	status_t			err;
	char				*packetPointer = 0;
	int32				packetLength;
	int32				oldPacketLength;
//	int32				partialOffset;
	off_t				filePos;
	off_t				oldFilePos;
	bool				done;

	if (global_stream < 0 || global_stream >= fStreamNum)
		return B_BAD_INDEX;

	Extractor *extractor = fExtractors[global_stream];
	int32	in_stream = fStreamNums[global_stream];

	flags &= ~B_MEDIA_SEEK_PRIVATE_MASK;

	stream = &fStreams[global_stream];

	filePos = stream->pos;
	packetLength = 0;

	//printf("SeekTo(%Ld, %Ld)\n", inout_time ? *inout_time : 0LL, inout_frame ? *inout_frame : 0LL);

int failsafe = 0;

	do {

		if (failsafe++ > 100) {
			printf("************ Failsafe triggered in MediaExtractor::SeekTo()...\n");
			printf("\tIt is likely that the extractor is never setting done to true.\n");
			return B_ERROR;
		}

#if 0
		// make sure the right chunk is read
		err = ReadAhead(in_stream);
		if (err) {
			printf("Extractor::SeekTo(): ReadAhead failed! (case 3)\n");
			return err;
		}

		if (fChunkSize - stream->pos_offset < packetLength) {

			// partial buffer...

			chunk = &fChunks[stream->partial_chunk];

			// save the partial packet
			ochunk = &fChunks[stream->chunk[stream->index % 2]];
			partialOffset = fChunkSize - stream->pos_offset;
			memcpy(chunk->start, ochunk->start + (fChunkSize - partialOffset), partialOffset);

			// seek to the beginning of the next chunk
			stream->pos += partialOffset;
			stream->pos_offset = 0;
			stream->pos_chunk += fChunkSize;

			// make sure the next chunk is read
			err = ReadAhead(in_stream);
			if (err) {
				printf("Extractor::SeekTo(): ReadAhead failed! (case 4)\n");
				return err;
			}

			// determine of much of the new chunk has to be copied to complete
			// the partial packet.

			if (err == B_OK) {
				ochunk = &fChunks[stream->chunk[stream->index % 2]];
				memcpy(chunk->start + partialOffset, ochunk->start, packetLength - partialOffset);
				partialOffset = packetLength - partialOffset;

				packetPointer = chunk->start;
			}	
		} else {

			// complete buffer

			chunk = &fChunks[stream->chunk[stream->index % 2]];
			packetLength = fChunkSize - stream->pos_offset;
			if (fFileSize != 0 && fFileSize - filePos < packetLength)
				packetLength = fFileSize - filePos;
			packetPointer = chunk->start + stream->pos_offset;
		}
#endif

		// call Seek

		oldPacketLength = packetLength;
		oldFilePos = filePos;

		err = extractor->Seek(in_stream, cookie, in_towhat, flags, inout_time,
		                      inout_frame, &filePos, packetPointer,
		                      &packetLength, &done);
		if (err) {
			//printf("Extractor::SeekTo(): got error from Seek (%s)\n", strerror(err));
			return err;
		}

		// a couple of sanity checks

		if (fFileSize != 0 && filePos > fFileSize) {
			printf("Extractor::SeekTo(): Seek returns filePos beyond EOF! (%Lx > %Lx)\n", filePos, fFileSize);
			return B_ERROR;
		}
		if (fFileSize != 0 && filePos + packetLength > fFileSize) {
			printf("Extractor::SeekTo(): Seek returns packetLength beyond EOF! (%Lx + %lx > %Lx)\n", filePos, packetLength, fFileSize);
			return B_ERROR;
		}

		// update the file position
		// optimization: don't do very slow 64 bit division when avoidable

		if (!((filePos >= stream->pos_chunk) && (filePos - stream->pos_chunk < fChunkSize)))
			if ((filePos >= stream->pos_chunk + fChunkSize) && (filePos - stream->pos_chunk < 2*fChunkSize))
				stream->pos_chunk += fChunkSize;
			else 
				if ((filePos >= stream->pos_chunk - fChunkSize) && (filePos < stream->pos_chunk))
					stream->pos_chunk -= fChunkSize;
				else
					stream->pos_chunk = (filePos / fChunkSize) * fChunkSize;
		stream->pos_offset = filePos - stream->pos_chunk;
		stream->pos = filePos;

	} while (!done);

	return B_OK;
}

status_t 
MediaExtractor::FindKeyFrame(int32 stream, void *cookie, int32 in_towhat,
                             bigtime_t *inout_time, int64 *inout_frame,
                             int32 flags)
{
	status_t err;
	bool done;
	int32	packetLength = 0;
	off_t	filePos = 0;

	if (stream < 0 || stream >= fStreamNum)
		return B_BAD_INDEX;
	Extractor *extractor = fExtractors[stream];
	int32	in_stream = fStreamNums[stream];
	
	flags &= ~B_MEDIA_SEEK_PRIVATE_MASK;
	flags |= B_MEDIA_SEEK_PEEK;
	err = extractor->Seek(in_stream, cookie, in_towhat, flags, inout_time, inout_frame,
	                      &filePos, NULL, &packetLength, &done);
	if (err) {
		return err;
	}
	if(!done)
		return B_NOT_ALLOWED;

	return B_OK;
}

status_t 
MediaExtractor::GetFileFormatInfo(media_file_format *mfi)
{
	return fFileExtractorInfo.extractor->GetFileFormatInfo(mfi);
}

const char *
MediaExtractor::Copyright(void)
{
	return fFileExtractorInfo.extractor->Copyright();
}

status_t 
MediaExtractor::TrackInfo(int32 stream, media_format *out_format,
                          void **out_info, int32 *out_infoSize)
{
	if (stream < 0 || stream >= fStreamNum)
		return B_BAD_INDEX;
	Extractor *extractor = fExtractors[stream];
	int32	in_stream = fStreamNums[stream];

	return extractor->TrackInfo(in_stream, out_format, out_info, out_infoSize);
}

status_t 
MediaExtractor::CountFrames(int32 stream, int64 *out_frames)
{
	if (stream < 0 || stream >= fStreamNum)
		return B_BAD_INDEX;
	Extractor *extractor = fExtractors[stream];
	int32	in_stream = fStreamNums[stream];

	return extractor->CountFrames(in_stream, out_frames);
}

status_t 
MediaExtractor::GetDuration(int32 stream, bigtime_t *out_duration)
{
	if (stream < 0 || stream >= fStreamNum)
		return B_BAD_INDEX;
	Extractor *extractor = fExtractors[stream];
	int32	in_stream = fStreamNums[stream];

	return extractor->GetDuration(in_stream, out_duration);
}


int32
MediaExtractor::ProcessJob(
	int32 n)
{
	ASSERT(fJobs != 0);

	int32		count;
	chunk_info	*chunk;
	ssize_t		sz;
	bool		bad;
	chunk = &fChunks[fJobs[n]];

	bad = false;
	// +++ optimize: only seek if necessary +++
	if (fSourceSeekable.ptr()) fSourceSeekable->Seek(chunk->offset, SEEK_SET);
	sz = fSourceStream->Read(chunk->start, fChunkSize);
	if ((sz != fChunkSize) && (chunk->offset + sz !=  fFileSize)) {
		printf("Extractor::ReaderLoop: read error (offset: %Lx, sz: %lx, sum: %Lx, fileSz: %Lx!\n", chunk->offset, sz, chunk->offset+sz, fFileSize);
		bad = true;
	}

	fJobLock.Lock();
	count = chunk->count;
	chunk->busy = false;
	chunk->bad = bad;
	fJobLock.Unlock();

	release_sem_etc(chunk->sem, count, 0);

	n++;
	if (n >= fJobsNum)
		n = 0;
	return n;
}

void
MediaExtractor::ReaderLoop()
{
	int32		n;
	status_t	err;

	n = 0;
	while (true) {
		while (true) {
			err = acquire_sem(fSubmitJobSem);
			if (err == B_BAD_SEM_ID)
				return;
			if (err == B_OK)
				break;
		}
		n = ProcessJob(n);
	}
}

status_t
MediaExtractor::LookupChunk(int32 in_stream, int32 in_index, off_t in_offset)
{
	stream_info			*stream;
	chunk_info			*chunk;
	int32				i;
	int32				submitted = -1;

	stream = &fStreams[in_stream];

	if (fFileSize != 0 && in_offset >= fFileSize)
		return B_LAST_BUFFER_ERROR;

	chunk = &fChunks[stream->chunk[in_index % 2]];
	if (stream->wait[in_index % 2]) {
		while (acquire_sem(chunk->sem) == B_INTERRUPTED)
			;
		stream->wait[in_index % 2] = false;
	}

	fJobLock.Lock();
	chunk->count--;

	for(i=0; i<2*fStreamNum; i++) {
		chunk = &fChunks[i];
		if (chunk->offset == in_offset) {
			chunk->count++;
			stream->chunk[in_index % 2] = i;
			if (chunk->busy)
				stream->wait[in_index % 2] = true;
			break;
		}
	}

	if (i == 2*fStreamNum) {
		for(i=0; i<2*fStreamNum; i++) {
			chunk = &fChunks[i];
			if (chunk->count == 0) {
				chunk->count = 1;
				chunk->offset = in_offset;
				chunk->busy = true;
				stream->chunk[in_index % 2] = i;
				stream->wait[in_index % 2] = true;
				submitted = fCurJob;
				fJobs[fCurJob] = i;
				fCurJob++;
				if (fCurJob >= fJobsNum)
					fCurJob = 0;
				break;
			}
		}
	}
	fJobLock.Unlock();

	if (submitted != -1) {
		if (fSubmitJobSem < 0) {
			(void)ProcessJob(submitted);
		}
		else {
			release_sem(fSubmitJobSem);
		}
	}

	return B_OK;
}


status_t
MediaExtractor::ReadAhead(int32 in_stream)
{
	stream_info			*stream;
	chunk_info			*chunk;
	status_t			err;

	stream = &fStreams[in_stream];
	chunk = &fChunks[stream->chunk[stream->index % 2]];

	if (chunk->offset != stream->pos_chunk) {
		stream->index++;
		chunk = &fChunks[stream->chunk[stream->index % 2]];

		if (chunk->offset == stream->pos_chunk) {
			if (stream->wait[stream->index % 2]) {
				while (acquire_sem(chunk->sem) == B_INTERRUPTED)
					;
				stream->wait[stream->index % 2] = false;
				if (chunk->bad) {
					//printf("Extractor::ReadAhead(): read error!\n");
					return B_IO_ERROR;
				}
			}
		} else {
			stream->index++;
			err = LookupChunk(in_stream, stream->index, stream->pos_chunk);
			if (err) {
				//printf("Extractor::ReadAhead(): LookupChunk(%Ld) failed with error %s\n", stream->pos_chunk, strerror(err));
				return err;
			}
			
			chunk = &fChunks[stream->chunk[stream->index % 2]];
			if (stream->wait[stream->index % 2]) {
				while (acquire_sem(chunk->sem) == B_INTERRUPTED)
					;
				stream->wait[stream->index % 2] = false;
				if (chunk->bad) {
					//printf("Extractor::ReadAhead(): read error!\n");
					return B_IO_ERROR;
				}
			}
		}

		err = LookupChunk(in_stream, stream->index+1, stream->pos_chunk + fChunkSize);
		//if (err != B_OK) {
			//printf("ReadAhead() untested LookupChunk() failed: %s\n", strerror(err));
		//}
	}
	return B_OK;
}



status_t
MediaExtractor::SeekToPosition(int32 in_stream, bigtime_t in_filePos)
{
	stream_info		*stream;

	if (in_stream >= fStreamNum)
		return B_BAD_INDEX;

	stream = &fStreams[in_stream];
	stream->pos_chunk = (in_filePos / fChunkSize) * fChunkSize;
	stream->pos_offset = in_filePos - stream->pos_chunk;
	stream->pos = in_filePos;

	if ((fFlags & B_MEDIA_FILE_UNBUFFERED) == B_MEDIA_FILE_UNBUFFERED)
		return B_OK;
		
	return ReadAhead(in_stream);
}


int32
MediaExtractor::start_reader_thread(void *p)
{
	((MediaExtractor *)p)->ReaderLoop();
	return 0;
}


status_t MediaExtractor::Perform(int32, void *)
{
	return B_ERROR;
}

status_t MediaExtractor::ControlFile(int32 selector, void * data, size_t size)
{
	status_t okErr = EBADF;
	status_t otherErr = EBADF;
	for (int ix=0; ix<fStreamNum; ix++)
	{
		status_t err = fExtractors[ix]->ControlFile(selector, data, size);
		if (err < B_OK)
		{
			if (otherErr == EBADF)
			{
				otherErr = err;
			}
		}
		else if (okErr == EBADF) {
			okErr = err;
		}
	}
	if (okErr != EBADF)
	{
		return okErr;
	}
	return otherErr;
}

status_t MediaExtractor::_Reserved_MediaExtractor_0(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_1(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_2(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_3(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_4(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_5(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_6(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_7(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_8(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_9(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_10(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_11(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_12(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_13(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_14(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_15(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_16(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_17(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_18(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_19(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_20(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_21(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_22(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_23(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_24(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_25(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_26(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_27(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_28(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_29(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_30(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_31(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_32(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_33(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_34(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_35(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_36(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_37(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_38(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_39(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_40(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_41(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_42(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_43(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_44(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_45(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_46(int32, ...) { return B_ERROR; }
status_t MediaExtractor::_Reserved_MediaExtractor_47(int32, ...) { return B_ERROR; }


Extractor::Extractor()
{
	mSourceStream = NULL;
	mSourceSeekable = NULL;
	mSourceExtractor = NULL;
	mSourceStreamIndex = -1;
}

Extractor::~Extractor()
{
	mSourceStream = NULL;
	mSourceSeekable = NULL;
	mSourceExtractor = NULL;
}

status_t 
Extractor::Sniff(int32 *, int32 *)
{
	return B_NOT_ALLOWED;
}

status_t 
Extractor::SniffFormat(const media_format *, int32 *, int32 *)
{
	return B_NOT_ALLOWED;
}

const char *
Extractor::Copyright()
{
	return NULL;
}

IByteInput::ptr 
Extractor::SourceStream() const
{
	return mSourceStream;
}

IByteSeekable::ptr 
Extractor::SourceSeekable() const
{
	return mSourceSeekable;
}

Extractor *
Extractor::SourceExtractor() const
{
	return mSourceExtractor;
}

int32 
Extractor::SourceStreamIndex() const
{
	return mSourceStreamIndex;
}

status_t
Extractor::SetSource(IByteInput::arg stream, IByteSeekable::arg seek)
{
	if (!stream.ptr()) return B_BAD_VALUE;
	mSourceStream = stream;
	mSourceSeekable = seek;
	return B_OK;
}

status_t
Extractor::SetSourceExtractor(Extractor * extractor, int32 index)
{
	if (!extractor) return B_BAD_VALUE;
	if (index < 0) return B_BAD_VALUE;
	mSourceExtractor = extractor;
	mSourceStreamIndex = index;
	return B_OK;
}

status_t Extractor::Perform(int32, void *)
{
	return B_ERROR;
}

status_t Extractor::ControlFile(int32, void *, size_t)
{
	return EBADF;
}

status_t Extractor::_Reserved_Extractor_0(int32 arg, ...) {
	void * data;
	size_t size;
	va_list vl;
	va_start(vl, arg);
	data = va_arg(vl, void *);
	size = va_arg(vl, size_t);
	va_end(vl);
	return Extractor::ControlFile(arg, data, size);
}

status_t Extractor::_Reserved_Extractor_1(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_2(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_3(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_4(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_5(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_6(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_7(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_8(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_9(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_10(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_11(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_12(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_13(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_14(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_15(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_16(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_17(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_18(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_19(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_20(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_21(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_22(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_23(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_24(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_25(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_26(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_27(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_28(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_29(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_30(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_31(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_32(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_33(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_34(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_35(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_36(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_37(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_38(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_39(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_40(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_41(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_42(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_43(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_44(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_45(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_46(int32, ...) { return B_ERROR; }
status_t Extractor::_Reserved_Extractor_47(int32, ...) {  return B_ERROR; }

} } // B::Private
