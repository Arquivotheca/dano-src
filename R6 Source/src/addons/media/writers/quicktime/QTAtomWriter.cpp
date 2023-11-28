// QTAtomWriter.cpp by Simon Clarke (S.J.Clarke@herts.ac.uk) - Copyright 1996-8

#include "QTAtomWriter.h"

#define DEBUG 1
#include <Debug.h>

#include <memory.h>
#include <Locker.h>
#include <string.h>
#include <List.h>
#include <DataIO.h>
#include <ByteOrder.h>
#include <FileWriter.h>

void
_QTdebugprint(const char *format, ...)
{
	#ifdef DEBUGONE
	char	dst[4096] = "\0";
	va_list	v;

	va_start(v,format);
	vsprintf(dst,format,v);
	printf(dst);
	#endif
}


QTAtomWriter::QTAtomWriter()
{	
	fCopyright = NULL;

	// reset audio stuff
	fAudioTotalChunks = 0;
	fOutStream = 0;

	fVideoIdxNumber = fAudioIdxNumber = 0;

	fTotalMDATSize = 0;
	fMDIAPosition = 0; fSTBLPosition = 0; fTRAKPosition = 0; fMINFPosition = 0;

	fFrameIdxSize = 1000;
	videoitems = (ItemEntry *)malloc(sizeof(ItemEntry) * fFrameIdxSize);
	
	fAFrameIdxSize = fFrameIdxSize;
	
	fVidChunkIdxSize = fFrameIdxSize / 4;
	fAudChunkIdxSize = fAFrameIdxSize / 4;

	// allocate chunk buffers
	cvideoitems = (ChunkEntry *)malloc(sizeof(ChunkEntry) * fVidChunkIdxSize);
	caudioitems = (ChunkEntry *)malloc(sizeof(ChunkEntry) * fAudChunkIdxSize);		

	cvideoitems[0].chunkItems = 0;
	caudioitems[0].chunkItems = 0;
	
	// start off with an empty chunk buffer
	fVidChunkPos = 0;
			
	fTrackList = new BList;
	
	fVideoSTTSentries = NULL;
	fVideoSTTSentries_size = 0;
	fVideoSTTSentry_count = 0;
	fLastVideoTime = 0;

	fRawAudioSwapBuffer = NULL;
	fRawAudioSwapBufferLength = 0;

	fInitStatus = QT_OK;
}

QTAtomWriter::~QTAtomWriter()
{
	if (fVideoSTTSentries)
		free(fVideoSTTSentries);
	fVideoSTTSentries = NULL;
	
	if (fCopyright)
		free(fCopyright);
	fCopyright = NULL;
		
	if (cvideoitems)
		free(cvideoitems);
	cvideoitems = NULL;
	
	if (videoitems)
		free(videoitems);
	videoitems = NULL;
	
	if (caudioitems)
		free(caudioitems);
	caudioitems = NULL;
	
	delete fTrackList;
	fTrackList = NULL;

	if(fRawAudioSwapBuffer)
		free(fRawAudioSwapBuffer);
	fRawAudioSwapBuffer = NULL;
}

void QTAtomWriter::SetTo(FileWriter *stream)
{
	fOutStream = stream;
}

status_t QTAtomWriter::InitCheck()
{
	return fInitStatus;
}

status_t QTAtomWriter::SetCachingScheme(uint32 scheme)
{
	fScheme = scheme;

	return B_OK;
}

status_t QTAtomWriter::BeginTrack(QTTrack *inputTrack)
{
	fTrackList->AddItem(inputTrack);

	return QT_OK;
}

status_t QTAtomWriter::CloseTrack()
{
	return QT_OK;
}

status_t QTAtomWriter::Begin()
{
	SetDefaults();	
	return B_OK;
}


status_t
QTAtomWriter::AddCopyright(const char *data)
{
	if (fCopyright)
		free(fCopyright);
	
	fCopyright = strdup(data);

	return (fCopyright) ? B_OK : B_NO_MEMORY;
}


status_t QTAtomWriter::UpdateSTTS(uint32 last_frame_duration, int final)
{
	if(fVideoSTTSentries_size < (fVideoSTTSentry_count+1) * sizeof(stts_entry)) {
		size_t new_size = (fVideoSTTSentry_count+1) * 2 * sizeof(stts_entry);
		void *tmp = realloc(fVideoSTTSentries, new_size);
		if(tmp == NULL)
			return B_NO_MEMORY;
		fVideoSTTSentries = (stts_entry *)tmp;
		fVideoSTTSentries_size = new_size;
	}

	//printf("add %svideo frame, last_frame_duration %d\n", final?"final ":"", last_frame_duration);

//! combine these;  can probably just get rid of final-specific stuff, since
//  I accidentally ran my test program without passing in 'true' from WriteVideoSTTS
//  without a problem, but I'd rather avoid other bugs this late in the game.
	if(!final) {
		if(fVideoSTTSentry_count == 0) {
			fVideoSTTSentries[0].frame_count = 1;
			fVideoSTTSentries[0].frame_duration = 0;
			fVideoSTTSentry_count++;
		}
		else if(fVideoSTTSentries[fVideoSTTSentry_count-1].frame_count == 1) {
			fVideoSTTSentries[fVideoSTTSentry_count-1].frame_duration =
				last_frame_duration;
			fVideoSTTSentries[fVideoSTTSentry_count-1].frame_count++;
		}
		else if(fVideoSTTSentries[fVideoSTTSentry_count-1].frame_duration ==
				last_frame_duration) {
			fVideoSTTSentries[fVideoSTTSentry_count-1].frame_count++;
		}
		else {
			fVideoSTTSentries[fVideoSTTSentry_count-1].frame_count--;
			fVideoSTTSentries[fVideoSTTSentry_count].frame_count = 2;
			fVideoSTTSentries[fVideoSTTSentry_count].frame_duration =
				last_frame_duration;
			fVideoSTTSentry_count++;
		}
	}
	else {
		if(fVideoSTTSentries[fVideoSTTSentry_count-1].frame_count == 1) {
			fVideoSTTSentries[fVideoSTTSentry_count-1].frame_duration =
				last_frame_duration;
		}
		else if(fVideoSTTSentries[fVideoSTTSentry_count-1].frame_duration !=
				last_frame_duration) {
			fVideoSTTSentries[fVideoSTTSentry_count-1].frame_count--;
			fVideoSTTSentries[fVideoSTTSentry_count].frame_count = 1;
			fVideoSTTSentries[fVideoSTTSentry_count].frame_duration =
				last_frame_duration;
			fVideoSTTSentry_count++;
		}
	}

	return QT_OK;
}


status_t QTAtomWriter::AddVideoFrame(const void			*inData,
									QTTrack				*inputTrack,
									size_t 				fixedSize,
									media_encode_info	*info)
{
	status_t err;
	
	if (!fOutStream)
		return B_ERROR;	

	CheckVideoIndex(inputTrack);
	
	uint32 last_frame_duration = info->start_time - fLastVideoTime;
	/* todo: round to specified frame time */
	if(last_frame_duration < 0) {
		printf("time runs backwards\n");
		return B_ERROR;
	}

	err = UpdateSTTS(last_frame_duration);
	if(err != B_OK) {
		return err;
	}

	fLastVideoTime = info->start_time;

	if (fixedSize > inputTrack->encodeBufferSize) {
		// fatal problem, we can't buffer a frame of this size, write straight to disk
		cvideoitems[fVidChunkPos].chunkOffset = fOutStream->Position();
		fOutStream->Write(inData, fixedSize);
		videoitems[inputTrack->videoFrameCount++].itemsize = fixedSize;
		cvideoitems[fVidChunkPos++].chunkItems++;
		cvideoitems[fVidChunkPos].chunkItems = 0;
		fTotalMDATSize += fixedSize;
		inputTrack->dataSize += fixedSize;		
		if ((info->flags & B_MEDIA_KEY_FRAME) && inputTrack->stssTable) {
			// add entry to keyframe table as well
			inputTrack->stssTable[inputTrack->stssEntryCount++] = inputTrack->videoFrameCount + 1;
		}
		return B_OK;		
	}
		
	if ((fixedSize + inputTrack->encodeBufferPos) > inputTrack->encodeBufferSize) {
		// flush incoming buffer
		cvideoitems[fVidChunkPos].chunkOffset = fOutStream->Position(); // position of chunk in datastream
		
		err = fOutStream->Write(inputTrack->encodeBuffer, inputTrack->encodeBufferPos);

		if (err != B_NO_ERROR) {
			printf("QTAtomWriter::AddFrame() - Flushed output size was not %ld\n", inputTrack->encodeBufferPos);
			inputTrack->encodeBufferPos = 0;
			return B_ERROR;
		}

		inputTrack->dataSize += inputTrack->encodeBufferPos;		
		fVidChunkPos++;		
		cvideoitems[fVidChunkPos].chunkItems = 0;
		fTotalMDATSize += inputTrack->encodeBufferPos;


		inputTrack->encodeBufferPos = 0;
	}

	if ((info->flags & B_MEDIA_KEY_FRAME) && inputTrack->stssTable) {
		// add entry to keyframe table as well
		inputTrack->stssTable[inputTrack->stssEntryCount++] = inputTrack->videoFrameCount;
	}

	videoitems[inputTrack->videoFrameCount++].itemsize = fixedSize;
	memcpy(	(uint8 *)inputTrack->encodeBuffer + inputTrack->encodeBufferPos,
			inData,
			fixedSize);
	
	inputTrack->encodeBufferPos += fixedSize;
	cvideoitems[fVidChunkPos].chunkItems++;
	
	return B_OK;
}

void QTAtomWriter::CheckAudioIndex()
{
	// resize index, one chunk per item entry
	if (fAudioTotalChunks >= fAudChunkIdxSize) {
		ChunkEntry *newitems = (ChunkEntry *)malloc(sizeof(ChunkEntry) * (fAudChunkIdxSize + 50));
		memcpy(newitems,caudioitems,sizeof(ChunkEntry) * fAudioTotalChunks);
		free(caudioitems);
		caudioitems = newitems;
		fAudChunkIdxSize += 50;
	}
}

void QTAtomWriter::CheckVideoIndex(QTTrack *track)
{
	if (track->videoFrameCount >= fFrameIdxSize) {
		ItemEntry *newviditems = (ItemEntry *)malloc(sizeof(ItemEntry)*(track->videoFrameCount+200));
		memcpy(newviditems,videoitems,sizeof(ItemEntry) * track->videoFrameCount);
		free(videoitems);
		videoitems = newviditems;
		fFrameIdxSize = track->videoFrameCount + 200;
	}

	if (fVidChunkPos+1 >= fVidChunkIdxSize) {
		ChunkEntry *newitems = (ChunkEntry *)malloc(sizeof(ChunkEntry) * (fVidChunkIdxSize + 50));
		memcpy(newitems, cvideoitems,sizeof(ChunkEntry) * fVidChunkPos);
		free(cvideoitems);
		cvideoitems = newitems;
		fVidChunkIdxSize += 50;
	}

	if (track->stssTable 
		&& (track->stssEntryCount >= track->stssMaxCount)) {
		uint32			*items;
		
		track->stssMaxCount += 50;
		items = (uint32 *)malloc(sizeof(uint32) * track->stssMaxCount);		
		memcpy(items, track->stssTable, sizeof(uint32) * track->stssEntryCount);
		free(track->stssTable);
		track->stssTable = items;
	}
}

inline static void
swap_data(void *out_data, const void *in_data, size_t size, size_t sample_bits)
{
	size_t nelems = size / (sample_bits/8);
	int i;

	switch(sample_bits) {
		case 8*4: {
			const uint32 *in = (const uint32 *)in_data;
			uint32 *out = (uint32 *)out_data;
			for(i = 0; i < nelems; i++) {
				out[i] = B_SWAP_INT32(in[i]);
			}
			break;
		}
		case 8*2: {
			const uint16 *in = (const uint16 *)in_data;
			uint16 *out = (uint16 *)out_data;
			for(i = 0; i < nelems; i++) {
				out[i] = B_SWAP_INT16(in[i]);
			}
			break;
		}
	}
}

status_t QTAtomWriter::AddAudioFrames(const void *inData,
									  QTTrack	 *inputTrack,
									  size_t	 size)
{
	status_t err;

	// AddAudioFrames() must be accessed sync!

	//if (inputTrack->audioCodecList != NULL)
	//	printf("bpf:%d\n", inputTrack->audioCodecList[0].bytesPerFrame);

	if (size > inputTrack->encodeBufferSize) {
		// we can't buffer it if it's bigger than our buffer...

		caudioitems[fAudioTotalChunks].chunkItems =
			size / inputTrack->audioCodecList[0].bytesPerFrame;

		if (inputTrack->audioCodecList[0].codecID == 'ima4') { 		// ick
			caudioitems[fAudioTotalChunks].chunkItems =
				(caudioitems[fAudioTotalChunks].chunkItems * 128) / 34;
		}
		
		caudioitems[fAudioTotalChunks].chunkOffset = fOutStream->Position();

		if(inputTrack->swapRawAudio) {
			if(fRawAudioSwapBufferLength < size) {
				void *p = realloc(fRawAudioSwapBuffer, size);
				if(p == NULL) {
					return B_NO_MEMORY;
				}
				fRawAudioSwapBuffer = p;
				fRawAudioSwapBufferLength = size;
			}
			swap_data(fRawAudioSwapBuffer, inData, size, inputTrack->audioCodecList[0].bitsPerSample);
			fOutStream->Write(fRawAudioSwapBuffer, size);
		}
		else {
			fOutStream->Write(inData, size);
		}

		fTotalMDATSize += size;
		fAudioTotalChunks++;
		inputTrack->dataSize += size;

		return B_OK;		
	}

	if ((inputTrack->encodeBufferPos + size) > inputTrack->encodeBufferSize) {
		// flush first
		CheckAudioIndex();

		caudioitems[fAudioTotalChunks].chunkItems =
			inputTrack->encodeBufferPos / inputTrack->audioCodecList[0].bytesPerFrame;

		if (inputTrack->audioCodecList[0].codecID == 'ima4') { 		// ick
			caudioitems[fAudioTotalChunks].chunkItems =
				(caudioitems[fAudioTotalChunks].chunkItems * 128) / 34;
		}

		caudioitems[fAudioTotalChunks].chunkOffset = fOutStream->Position();

		ASSERT(fOutStream != NULL);

		err = fOutStream->Write(inputTrack->encodeBuffer, inputTrack->encodeBufferPos);

		if (err != B_NO_ERROR) {
			// couldn't write all of buffer
			printf("QTAtomWriter::AddAudioFrames() - could not write %ld\n", inputTrack->encodeBufferPos);
			return QT_ERROR;
		}

		fTotalMDATSize += (size_t)inputTrack->encodeBufferPos;

		// reset
		inputTrack->dataSize += inputTrack->encodeBufferPos;
		inputTrack->encodeBufferPos = 0;			
		fAudioTotalChunks++;
	}

	if(inputTrack->swapRawAudio) {
		swap_data((char*)inputTrack->encodeBuffer + inputTrack->encodeBufferPos, inData, size,
			inputTrack->audioCodecList[0].bitsPerSample);
	}
	else {
		memcpy((char*)inputTrack->encodeBuffer + inputTrack->encodeBufferPos, inData, size);
	}
	inputTrack->encodeBufferPos += size;
	
	return B_OK;
}

status_t QTAtomWriter::CloseMovie()
{
	status_t		myErr;
	int32			i;
	QTTrack			*track;

	// flush any remaining chunk buffers out
	for (i = 0;i < fTrackList->CountItems(); i++) {
		track = (QTTrack *)fTrackList->ItemAt(i);

		if (track && (track->encodeBufferPos > 0)) {
			switch (track->Type()) {
				case QT_VIDEO:
					cvideoitems[fVidChunkPos].chunkOffset = fOutStream->Position();
					myErr = fOutStream->Write(track->encodeBuffer, track->encodeBufferPos);
					if (myErr != B_NO_ERROR) {
						printf("QTAtomWriter::CloseMovie() - Final video flush couldn't write enough\n");
					}
					fVidChunkPos++;
					fTotalMDATSize += track->encodeBufferPos;
					track->dataSize += track->encodeBufferPos;
					break;
				
				case QT_AUDIO:
					caudioitems[fAudioTotalChunks].chunkItems =
						track->encodeBufferPos / track->audioCodecList[0].bytesPerFrame;

					if (track->audioCodecList[0].codecID == 'ima4') { 		// ick
						caudioitems[fAudioTotalChunks].chunkItems =
							(caudioitems[fAudioTotalChunks].chunkItems * 128) / 34;
					}

					caudioitems[fAudioTotalChunks].chunkOffset = fOutStream->Position();
					
					myErr = fOutStream->Write(track->encodeBuffer, track->encodeBufferPos);
					if (myErr != B_NO_ERROR) {
						printf("QTAtomWriter::CloseMovie() - Final audio flush couldn't write enough\n");
					}
					fTotalMDATSize += track->encodeBufferPos;
					fAudioTotalChunks++;
					track->dataSize += track->encodeBufferPos;
					break;	
			}
		}
	}
	
	ReplaceLength(0, fTotalMDATSize + 8);
	myErr = WriteFullHeader();
	
	DEBUGTEXT("QTAtomWriter: Finished CloseMovie()\n");
	return myErr;
}

status_t QTAtomWriter::WriteMVHD(qt_mvhd_details *header)
{
	WriteMSB32(header->version);
//	WriteMSB32(header->creation);
	WriteMSB32(0xAE4B2DFF); // precalc

//	WriteMSB32(header->modtime);
	WriteMSB32(0xAE4B2E6B); // example time
	
	WriteMSB32(header->timescale);
	WriteMSB32(header->duration);
	WriteMSB32(header->rate);
	WriteMSB16(header->volume);
	WriteMSB32(header->r1);
	WriteMSB32(header->r2);

	memset(header->matrix, 0, sizeof(header->matrix));
	header->matrix[0][0] = 1;
	header->matrix[1][1] = 1;
	header->matrix[1][2] = 0;
	header->matrix[2][2] = 16384;

  	for (int32 i=0;i<3;i++) for(int32 j=0;j<3;j++) 
		WriteMSB32(header->matrix[i][j]);

	WriteMSB16(header->r3);
	WriteMSB32(header->r4);
	WriteMSB32(header->pv_time);
	WriteMSB32(header->post_time);
	WriteMSB32(header->sel_time);
	WriteMSB32(header->sel_durat);
	WriteMSB32(header->cur_time);
	WriteMSB32(header->nxt_tk_id);
	
	return QT_OK;
}

status_t QTAtomWriter::WriteMDHD(qt_mdhd_details *header)
{
	WriteMSB32(header->version);

//	WriteMSB32(header->creation);
	WriteMSB32(0xAE4B2DFF); // precalc

//	WriteMSB32(header->modtime);
	WriteMSB32(0xAE4B2E6B); // precalc
	
	WriteMSB32(header->timescale);
	WriteMSB32(header->duration);
	WriteMSB16(header->language);
	WriteMSB16(header->quality);
	
	return QT_OK;
}

status_t QTAtomWriter::WriteHDLR(	const char 		*headerName,
									int32 			subType,
									int32 			type,
									int32 			headerNameLength)
{
	// media header component data
	WriteMSB32(0); // version and flags
	WriteMSB32(type); // its a media handler
	WriteMSB32(subType);
	WriteMSB32('simo');
	WriteMSB32(0);

	// fake apple flag masks
	if (type == 'dhlr') WriteMSB32(0x00010016);
	else WriteMSB32(0x00010011); // component flag mask

	// write about us
	WriteName(headerName,headerNameLength);

	return QT_OK;
}

status_t QTAtomWriter::WriteAppleHDLR(	const char 		*headerName,
										int32 			subType,
										int32 			type,
										int32 			headerNameLength)
{
	off_t		tempPosition;

	tempPosition = fOutStream->Position();
	WriteAtomHeader(QT_hdlr,0); //sizeof(qt_hdlr_details)+QTAH_SIZE+strlen(avmedianame)+QT_FLAGSSIZE+1);

	// media header component data
	WriteMSB32(0); // version and flags
	WriteMSB32(type); // its a media handler
	WriteMSB32(subType);
	WriteMSB32('appl'); // manufac

	// this has a reserved area with 64 in it!
	WriteMSB32(0x40000000); // flags

	if (type == 'dhlr') WriteMSB32(0x00010016);
	else WriteMSB32(0x00010011); // component flag mask

	// write about us
	WriteName(headerName,headerNameLength);

	ReplaceLength(tempPosition,fOutStream->Position()-tempPosition);
	
	//printf("len = %ld\n",(int32)fOutStream->Position()-(int32)temppos);

	return QT_OK;
}

status_t QTAtomWriter::WriteVMHD()
{
	// cheat with copying VMHD header, standard headre

	int32 		i;
	
	WriteAtomHeader(QT_vmhd,QTAH_SIZE + 12); // vmhd header size is 12
	
	WriteMSB32(1); 	// version and flags
					// shows we are not a QuickTime 1.0 file

	// dither copy
	WriteMSB16(64); // QuickDraw transfer mode?

	for (i=0; i < 3;i++) // opcolors
		WriteMSB16(32768); 
	
	return QT_OK;
}

status_t QTAtomWriter::WriteSMHD()
{
	WriteAtomHeader(QT_smhd,QTAH_SIZE+8); // vmhd header size is 12
	WriteMSB32(0); // version and flags
	WriteMSB32(0); // balance and reserved
	
	return QT_OK;
}

status_t QTAtomWriter::WriteTKHD(qt_tkhd_details *header)
{
	// write the track header details onto disk
	off_t		savePosition;

	savePosition = fOutStream->Position();

	// removed +8 11/01/98
//	WriteAtomHeader(QT_tkhd,sizeof(qt_tkhd_details)); //+8);
	WriteAtomHeader(QT_tkhd, 92);
	
	//int32 flags = (TRAK_ENABLED | TRAK_INMOVIE);
	//fOutStream->Write(&flags+1,3);

	WriteMSB8(0); 	 // version
	WriteMSB8(0x00); // flags!
	WriteMSB8(0x00); // flags!
	WriteMSB8(0x0F); // flags
					
	WriteMSB32(0xAE4B2DFF);       // WriteMSB32(header->creation);
	WriteMSB32(0xAE4B2E6B);       // WriteMSB32(header->modtime);
	WriteMSB32(fTrackCount);      // WriteMSB32(header->trackid);
	WriteMSB32(0); 				  // not timescale, reserved by apple

	WriteMSB32(header->duration);		
	WriteMSB32(header->time_off);
	WriteMSB32(header->priority);
	WriteMSB16(header->layer);
	WriteMSB16(header->alt_group);
	WriteMSB16(header->volume);

	memset(header->matrix, 0, sizeof(header->matrix));
	header->matrix[0][0] = 1;
	header->matrix[1][1] = 1;
	header->matrix[1][2] = 0;
	header->matrix[2][2] = 16384;

  	for(int32 i=0;i<3;i++) for(int32 j=0;j<3;j++) 
		WriteMSB32(header->matrix[i][j]);
	
	WriteMSB32(header->tk_width);
	WriteMSB32(header->tk_height);
	WriteMSB16(header->pad);

	return QT_OK;
}

void QTAtomWriter::WriteMSB32(int32 value)
{
	int32		outValue;

	// need to do byte swapping
	outValue = B_HOST_TO_BENDIAN_INT32(value);
	fOutStream->Write(&outValue,sizeof(int32));
}

void QTAtomWriter::WriteMSB16(int16 value)
{
	uint16		outValue;

	// need to do byte swapping
	outValue= B_HOST_TO_BENDIAN_INT16(value);
	fOutStream->Write(&outValue,sizeof(uint16));
}

void QTAtomWriter::WriteMSB8(uchar value)
{
	fOutStream->Write(&value,sizeof(uchar));
}

void QTAtomWriter::WriteAtomHeader(int32 id, int32 length)
{
	_QTdebugprint("QTAtomWriter: Writing Atom ID %.4s (len %d)\n",
			   &id, length);
	
	WriteMSB32(length);
	WriteMSB32(id);
}

void QTAtomWriter::ReplaceLength(off_t offset, int32 value)
{
	off_t 		replacePosition;
	
	replacePosition = fOutStream->Position();
	
	fOutStream->Seek(offset,SEEK_SET);
	WriteMSB32(value);
	
	// go back to last position we were at
	fOutStream->Seek(replacePosition,SEEK_SET);
}

status_t QTAtomWriter::WriteAudioSTSD(	int32 		sampleRate, 
										int32 		channelCount, 
										int32 		bitsPerSample, 
										int32 		codecType)
{
	WriteAtomHeader(QT_stsd,QTAH_SIZE+44); // need to find real len of this
	
	WriteMSB32(0); // version and flags
	WriteMSB32(1); // number of codecs
	
	// variable from here
	WriteMSB32(36); // length
	
	WriteMSB32(codecType);  // compression revision

	WriteMSB32(0); 			// 6 bytes of reserved 0's
	WriteMSB16(0);

	WriteMSB16(1); 			// data reference index
	
	WriteMSB32(0); 			// version
	WriteMSB32(gVendor); 	// vendor, use global by default

	WriteMSB16(channelCount);
	WriteMSB16(bitsPerSample);
	
	WriteMSB16(0); 			// comp_id
	WriteMSB16(0); 			// pack size
	
	WriteMSB16(sampleRate);
	WriteMSB16(0);
	
	return QT_OK;
}

status_t QTAtomWriter::WriteVideoSTSD(	video_smp_details 	*header,
										const char 			*codecName)
{
	off_t		stsdPosition, stsdSize, posReplace;

	stsdPosition = fOutStream->Position();

	WriteAtomHeader(QT_stsd,0); // need to find real len of this

	WriteMSB32(0); 			// version and flags
	WriteMSB32(1); 			// number of entries!
	
	// table is VARIABLE length from this point, based on the number of entries
	
	stsdSize = fOutStream->Position();
	WriteMSB32(0); 			// size
		
	WriteMSB32(header->codecID); // "compression format or media type" - id?
	
	// six reserved bytes
	WriteMSB32(0); 			// reserved
	WriteMSB16(0); 			// reserved
	
	WriteMSB16(1); 			// sample description (see STSC) 28
	
	// after this we have video specific data
	WriteMSB32(0); 			// reserved 
	WriteMSB32(gVendor); 	// vendor
	WriteMSB32(0); 			// temp_qual 36 to here
	WriteMSB32(100); 		// spat_qual 40
	
	WriteMSB16(header->width); // width of image 
	WriteMSB16(header->height); // height of image 44
	
	WriteMSB16(72.0); 		// h_res - DPI
	WriteMSB16(0);			// unk_4
	WriteMSB16(72.0); 		// v_res
	WriteMSB16(0);			// unk_5 52
	
	WriteMSB32(0); 			// datasize
	WriteMSB16(1); 			// framecount should be 1

	// write codec name here
	WriteName(codecName,strlen(codecName),true);

	WriteMSB16(header->depth); // depth
	WriteMSB16(-1); // flag 62 colour table flag

	ReplaceLength(stsdSize,fOutStream->Position()-stsdSize);

	posReplace = fOutStream->Position()-stsdPosition;
	ReplaceLength(stsdPosition,posReplace);
	
	return QT_OK;
}

status_t QTAtomWriter::WriteSTSS(	QTTrack		*track)
{
	uint32			i;

	// sync sample table - maps keyframes
	WriteAtomHeader(QT_stss,8 + QTAH_SIZE + (track->stssEntryCount * 4));
	
	WriteMSB32(0); // version and flags
	WriteMSB32(track->stssEntryCount); // number of entries
	
	for (i = 0; i < track->stssEntryCount; i++) {
		WriteMSB32(track->stssTable[i] + 1); // sample position		
	}
	
	return QT_OK;
}

status_t 
QTAtomWriter::WriteVideoSTTS(int32 duration)
{

	if(fVideoSTTSentry_count == 0) {
		return QT_OK;
	}

	int32 last_frame_duration = duration*100 - fLastVideoTime;
	if(last_frame_duration > 0) {
		status_t err = UpdateSTTS(last_frame_duration, true);
		if(err != QT_OK) {
			return err;
		}
	}

	WriteAtomHeader(QT_stts, QTAH_SIZE + 8 + 8*fVideoSTTSentry_count);
	WriteMSB32(0); // version and flags
	WriteMSB32(fVideoSTTSentry_count); // number of entries

	int32 tmp_duration = 0;

	for(int i=0; i<fVideoSTTSentry_count; i++) {
		WriteMSB32(fVideoSTTSentries[i].frame_count);
		WriteMSB32(fVideoSTTSentries[i].frame_duration/100);
		tmp_duration += fVideoSTTSentries[i].frame_duration/100 *
			fVideoSTTSentries[i].frame_count;
	}

	if(tmp_duration != duration) {
		//printf("Video STTS: file duration %d != sum of entries %d\n",
		//    duration, tmp_duration);
	}

	return QT_OK;
}

status_t QTAtomWriter::WriteSTTS(	int32 		sampleCount, 
									int32 		duration,
									int32		frameCount)
{
	int32 frame_time = 0;
	if(frameCount > 0)
		frame_time = duration / frameCount;
	int32 last_frame_time = duration - (frame_time* (sampleCount-1));

	// time to sample table - duration of sample information (Page 48)
	// individual length of samples is set in packets of consecutive
	// samples that have the same result
	if (frame_time == last_frame_time) {
		WriteAtomHeader(QT_stts, QTAH_SIZE + 8 + 8);

		WriteMSB32(0); // version and flags
		WriteMSB32(1); // number of entries

		// we assume all samples are the same length
		WriteMSB32(sampleCount);
		WriteMSB32(frame_time);
	} else {
		WriteAtomHeader(QT_stts, QTAH_SIZE + 8 + 8 + 8);

		WriteMSB32(0); // version and flags
		WriteMSB32(2); // number of entries

		// we assume all samples are the same length
		WriteMSB32(sampleCount - 1);
		WriteMSB32(frame_time);

		WriteMSB32(1);
		WriteMSB32(duration - last_frame_time);
	}
	
	return QT_OK;
}

status_t QTAtomWriter::WriteSTSC(	int32 			numChunks, 
									ChunkEntry 		*entryList,
									int32			data_reference_index)
{
	int32 		firstChunk = 1, last_size = 0, num_entries = 0, i;

	// sample to chunk table - maps samples to correct chunks (Page 51)
	// this table describes how we can find a specific sample within a
	// chunk

	/* first count how many entries we'll have */
	for (i=0,num_entries=0; i < numChunks; i++) {	
		if (last_size == entryList[i].chunkItems)
			continue;

		last_size = entryList[i].chunkItems;
		num_entries++;
	}

	WriteAtomHeader(QT_stsc,8+QTAH_SIZE+(num_entries*12));
	WriteMSB32(0); 			 // version
	WriteMSB32(num_entries); // number
	
	last_size = 0;
	for (i=0; i < numChunks; i++,firstChunk++) {	
		if (last_size == entryList[i].chunkItems)
			continue;

		WriteMSB32(firstChunk); // first chk
		WriteMSB32(entryList[i].chunkItems); // samples per chk
		WriteMSB32(data_reference_index); // chunk_tag - codec

		last_size = entryList[i].chunkItems;
	}

	return QT_OK;
}

status_t QTAtomWriter::WriteSTSZ(	int32 		sampleCount, 
									ItemEntry 	*itemList)
{
	int32		i;

	_QTdebugprint("QTAtomWriter::WriteSTSZ() - sampleCount = %ld\n", sampleCount);

	// sample size table - locate individual sample sizes (Page 53)
	WriteAtomHeader(QT_stsz,12+QTAH_SIZE+(4*sampleCount));

	WriteMSB32(0); // version and flags
	// if the sample sizes are all the same, this should be set to the
	// value of the samples, else it should be 0, we will assume that
	// all the sizes are going to be different
	WriteMSB32(0); // sample size
	WriteMSB32(sampleCount); // number of entries
	
	ASSERT(itemList != 0);
		
	for (i = 0; i < sampleCount;i++) {
//		ASSERT(itemList[i].itemsize > 0);
		WriteMSB32(itemList[i].itemsize); // size of sample
	}

	return QT_OK;
}

status_t QTAtomWriter::WriteAudioSTSZ(int32 bytesPerFrame, int32 numFrames)
{
	// sample size table - locate individual sample sizes (Page 53)
	WriteAtomHeader(QT_stsz,12+QTAH_SIZE);

	WriteMSB32(0); // version and flags
	
	// if the sample sizes are all the same, this should be set to the
	// value of the samples, else it should be 0, we will assume that
	// all the sizes are going to be different
	WriteMSB32(bytesPerFrame); // sample size
	WriteMSB32(numFrames); // number of entries
		
	return QT_OK;
}

status_t QTAtomWriter::WriteSTCO(	int32 		chunkCount,
									ChunkEntry 	*itemList)
{
	int32 		i;
	// chunk offsets table - locate chunk for sample (Page 55)

	// total size of table is 8 for atom header, 8 for version and
	// num and then the number of entries
	WriteAtomHeader(QT_stco,QTAH_SIZE+8+(chunkCount*4));

	WriteMSB32(0); // version and flags
	WriteMSB32(chunkCount); // number of chunks we want

	// num is the number of samples to put into a chunk
	for (i = 0;i < chunkCount;i++) {		
		// fill in num of chunks per numbersamps		
		WriteMSB32((int32)itemList[i].chunkOffset);
	}
	
	return QT_OK;
}

status_t QTAtomWriter::WriteELST(int32 duration)
{
	WriteAtomHeader(QT_elst,QTAH_SIZE+8+12);
	
	WriteMSB32(0); // version and flags
	WriteMSB32(1); // entries
	
	WriteMSB32(duration); // track duration
	WriteMSB32(0); // media start time 
	WriteMSB16(1.0); // rate
	WriteMSB16(0); // pad

	return QT_OK;
}

status_t QTAtomWriter::WriteDINFnDREF()
{
	// leaf atom
	WriteAtomHeader(QT_dinf,12+8+QTAH_SIZE+QTAH_SIZE);
	WriteAtomHeader('dref',12+8+QTAH_SIZE);
	
	// one fake data reference
	WriteMSB32(0); // version and flags
	WriteMSB32(1); // entries
	
	WriteMSB32(12); // size, 12 bytes for one entry
	WriteMSB32('alis');	
	WriteMSB32(1);	

	return QT_OK;
}

void QTAtomWriter::WriteName(	const char 	*name, 
								int32 		stringlength, 
								bool 		padString)
{
	int32 		i, end = 0;
	
	if (padString) 
		end = 31-stringlength;
		
	WriteMSB8(stringlength); // pascal bit
	
	for (i = 0;i<stringlength;i++)
		WriteMSB8(name[i]);

	if (padString)
		for (i = 0;i<end;i++) WriteMSB8(0);
}

void QTAtomWriter::SetDefaults()
{
	WriteAtomHeader(QT_mdat,0);

	//
	// this will pad out the header to a 2k boundary so that
	// we begin writing data on a nice boundary (good for
	// raw video).
	//
	char buff[2048];
	memset(buff, 0xbe, sizeof(buff));
	fOutStream->Write(buff, sizeof(buff) - 8);
	fTotalMDATSize = sizeof(buff) - 8;
		
	// trak is incremented by SetupVideoValues()
	fTrackCount = 1;
}

void QTAtomWriter::StartTrak(qt_tkhd_details *trakHeader)
{
	// this function is used to initalize some variables for the start of a quicktime
	// track

	fTRAKPosition = fOutStream->Position();

	WriteAtomHeader(QT_trak,0);
	WriteTKHD(trakHeader);

	fTrackCount++;
}


status_t QTAtomWriter::WriteAudioTrack(QTTrack *track)
{
	audio_smp_details			*audioDetails;
	
	audioDetails = track->audioCodecList;	

	_QTdebugprint("QTAtomWriter::WriteAudioTrack()\n");
	
	if (!track->dataSize) {
		return QT_ERROR;
	}
	
	StartTrak(&track->qt_tkhdr);
	
	WriteAtomHeader(QT_edts,QTAH_SIZE + QTAH_SIZE + 8 + 12); 
	WriteELST(track->qt_tkhdr.duration);

	fMDIAPosition = fOutStream->Position();	
	WriteAtomHeader(QT_mdia,0);
	
	WriteAtomHeader(QT_mdhd,sizeof(qt_mdhd_details)+4);
	WriteMDHD(&track->qt_mdhdr);

	// push QT_hdlr, header information for this media
	if (FAKE_APPLE) {
		WriteAppleHDLR(gSMediaName,'soun','mhlr',strlen(gSMediaName));
	} else {
		WriteAtomHeader(QT_hdlr,sizeof(qt_hdlr_details)+QTAH_SIZE+strlen(gMediaName)+QT_FLAGSSIZE+1);
		WriteHDLR(gMediaName,'soun','mhlr',strlen(gMediaName));
	}
	// push QT_minf, sub atom of QT_trak
	fMINFPosition = fOutStream->Position();	
	WriteAtomHeader(QT_minf,QTAH_SIZE);

	WriteSMHD();

	if (FAKE_APPLE) {
		WriteAppleHDLR(gAliasName,'alis','dhlr',strlen(gAliasName));		
	} else {
		WriteAtomHeader(QT_hdlr,sizeof(qt_hdlr_details)+QTAH_SIZE+strlen(gDataName)+QT_FLAGSSIZE+1);	
		WriteHDLR(gDataName, 'beos', 'dhlr', strlen(gDataName));
	}

	WriteDINFnDREF();

	fSTBLPosition = fOutStream->Position();
	WriteAtomHeader(QT_stbl,0); // leaf atom
	
	WriteAudioSTSD(audioDetails->audioSampleRate,audioDetails->audioChannels,
				   audioDetails->bitsPerSample,audioDetails->codecID);
		
	if (audioDetails->codecID == 'ima4') {		// ick
		WriteSTTS((track->dataSize * 128) / 34, track->qt_mdhdr.duration,
				  track->qt_mdhdr.duration);
	} else {
		WriteSTTS(track->dataSize, track->qt_mdhdr.duration,
				  track->qt_mdhdr.duration);
	}
	WriteSTSC(fAudioTotalChunks, caudioitems, 1);
	WriteAudioSTSZ(track->audioCodecList[0].bytesPerFrame, track->qt_mdhdr.duration);
	WriteSTCO(fAudioTotalChunks, caudioitems);
	
	ReplaceEndTrak();	

	return QT_OK;
}

status_t QTAtomWriter::WriteVideoTrack(QTTrack *track)
{
	StartTrak(&track->qt_tkhdr);

	// edit list
	WriteAtomHeader(QT_edts,QTAH_SIZE + QTAH_SIZE + 8 + 12); // leaf atom for edit list stuff, includes size of ELST
	WriteELST(track->qt_tkhdr.duration);

	// push QT_mdia, sub atom of QT_trak
	fMDIAPosition = fOutStream->Position();	

	WriteAtomHeader(QT_mdia,0);
	
	// push mediaheader QT_mdhr
	// 11/01/98, changed +8 to +4 to bring inline with other quicktime
	// movies sizes

	WriteAtomHeader(QT_mdhd,sizeof(qt_mdhd_details)+4);
	WriteMDHD(&track->qt_mdhdr);

	// push QT_hdlr, header information for this media
	if (FAKE_APPLE) {
		WriteAppleHDLR(gAVMediaName, 'vide', 'mhlr', strlen(gAVMediaName));	
	} else {
		WriteAtomHeader(QT_hdlr,sizeof(qt_hdlr_details)+QTAH_SIZE+strlen(gMediaName)+QT_FLAGSSIZE+1);
		WriteHDLR(gMediaName,'vide','mhlr',strlen(gMediaName));
	}

	fMINFPosition = fOutStream->Position();
	WriteAtomHeader(QT_minf,0);

	// we are a video trak
	WriteVMHD(); // video!

	if (FAKE_APPLE) {
		WriteAppleHDLR(gAliasName,'alis','dhlr',strlen(gAliasName));		
	} else {
		WriteAtomHeader(QT_hdlr,sizeof(qt_hdlr_details)+QTAH_SIZE+strlen(gDataName)+QT_FLAGSSIZE+1);	
		WriteHDLR(gDataName,'beos','dhlr',strlen(gDataName));
	}

	WriteDINFnDREF();

	// now we get to sample table stuff, first we need to 
	// tell it about what codecs we are using with a STSD table
	
	fSTBLPosition = fOutStream->Position();
	WriteAtomHeader(QT_stbl,0); // leaf atom
	
	WriteVideoSTSD(track->videoCodecList, "BeOS Codec");

	// next is a STTS table, which gives lengths	
	WriteVideoSTTS(track->qt_mdhdr.duration);

	// sync sample table needed?
	if ((track->stssEntryCount > 0) && track->stssTable)
		WriteSTSS(track);

	// next is STSC, which is sample to chunk stuff
	WriteSTSC(fVidChunkPos, cvideoitems, 1);

	// next is sample size stuff
	WriteSTSZ(track->videoFrameCount, videoitems);

	// finally we have the important one, the chunk offsets
	// for all the data, this is allocated and filled in as
	// we get the video information	
	WriteSTCO(fVidChunkPos, cvideoitems); // same as number of frames fTotalVideoFrames/5

	// close off video trak
	ReplaceEndTrak();

	return QT_OK;
}

status_t
QTAtomWriter::WriteCopyright(const char *copyright)
{
	int i, len = strlen(copyright);
	char copyright_string[] = { 0xa9, 'c', 'p', 'y' };

	WriteAtomHeader(QT_udta, QTAH_SIZE + 8 + 4 + len + 4);
	WriteAtomHeader(*(int *)copyright_string, QTAH_SIZE + len + 4);
	WriteMSB16(len);
	WriteMSB16(0x0000);

	for (i=0; i < len; i++)
		WriteMSB8(copyright[i]);

	WriteMSB32(0);
	return B_OK;
}



status_t QTAtomWriter::WriteFullHeader()
{	
	off_t				moovPos;
	QTTrack				*track;
	int32 				i;
	double				audio_length;
	qt_mvhd_details		mvhdHeader;

	memset(&mvhdHeader, 0, sizeof(qt_mvhd_details));

	mvhdHeader.nxt_tk_id = fTrackList->CountItems() + 1;
	mvhdHeader.rate = 1 << 16; // normal rate
	mvhdHeader.pv_time = 0; // preview time
	mvhdHeader.pv_durat = 0; // preview duration
	mvhdHeader.post_time = 0; // poster time
	mvhdHeader.sel_time = 0; // selection time
	mvhdHeader.sel_durat = 0; // selection duration
	mvhdHeader.cur_time = 0; // current time in this trak
	mvhdHeader.creation = 0; // creation date
	mvhdHeader.modtime = 0; // modified date
	mvhdHeader.volume = 255; // volume

	// push QT_moov chunk, but save its position for later
	DEBUGSAVE("QTAtomWriter: Writing header\n");
	
	moovPos = fOutStream->Position();
	WriteAtomHeader(QT_moov,0);

	for (i = 0;i < fTrackList->CountItems(); i++) {
		track = (QTTrack *)fTrackList->ItemAt(i);

		// setup duration / timescale in qt_tkhd_details and qt_mdhd_details
		track->qt_mdhdr.quality = 50;
		
		switch (track->Type()) {
			case QT_VIDEO:			
				// work this out from the values given by startTime and endTime				
				if (!track->videoCodecList[0].usecs_per_frame) {				
					track->qt_mdhdr.duration = track->forceDuration / 1000;
					track->qt_mdhdr.timescale = 1000000 / 1000;
				} else {
					track->qt_mdhdr.duration = (int32)((fLastVideoTime +
						track->videoCodecList[0].usecs_per_frame)/100.0);
						
					//track->qt_mdhdr.duration = (int32)((track->videoCodecList[0].usecs_per_frame *
					//	                                (double)track->videoFrameCount) / 100.0);
					track->qt_mdhdr.timescale = 10000;
				}
				
				mvhdHeader.duration       = track->qt_mdhdr.duration;
				mvhdHeader.timescale      = track->qt_mdhdr.timescale;

				track->qt_tkhdr.duration  = track->qt_mdhdr.duration;
				track->qt_tkhdr.timescale = track->qt_mdhdr.timescale;
		
				track->qt_tkhdr.tk_width  = track->videoCodecList[0].width;
				track->qt_tkhdr.tk_height = track->videoCodecList[0].height;
				break;

			case QT_AUDIO:
				track->qt_mdhdr.duration = (int32)((double)track->dataSize /
						(double)track->audioCodecList[0].bytesPerFrame);
				track->qt_mdhdr.timescale = track->audioCodecList[0].audioSampleRate;

				if (track->audioCodecList[0].codecID == 'ima4')	{		// ick
					// IMA is a 4:1 compression (we say 128:34 [3.8:1]),
					// so scale up the duration we determined from the data length
					track->qt_mdhdr.duration = (track->qt_mdhdr.duration * 128) / 34;
				}

				track->qt_tkhdr.duration  = track->qt_mdhdr.duration;
				track->qt_tkhdr.timescale = track->qt_mdhdr.timescale;
				track->qt_tkhdr.volume = 255;

				// set the duration/timescale if a video track hasn't already done so
				if(mvhdHeader.duration == 0) {
					mvhdHeader.duration       = track->qt_mdhdr.duration;
					mvhdHeader.timescale      = track->qt_mdhdr.timescale;
				}

				break;
		
			default:
				printf("QTAtomWriter::WriteFullHeader() - unknown track type!! (%x)\n", track->Type());
				break;		
		}
		
		//if (track->Type() == QT_VIDEO)
		//		printf("%ld V dur=%ld ts=%ld tD=%.4f\n", i, track->qt_mdhdr.duration, track->qt_mdhdr.timescale, (double)track->qt_mdhdr.duration / (double)track->qt_mdhdr.timescale);
		//else
		//	printf("%ld A dur=%ld ts=%ld tD=%.4f\n", i, track->qt_mdhdr.duration, track->qt_mdhdr.timescale, (double)track->qt_mdhdr.duration / (double)track->qt_mdhdr.timescale);
		//
		//printf("%ld: RT dur=%.4f\n", i, ((double)track->forceDuration) / (double)1000000);
	}

	
	// push QT_mvhd
	WriteAtomHeader(QT_mvhd, sizeof(qt_mvhd_details)); //+8);
	WriteMVHD(&mvhdHeader);
	
	for (i = 0;i < fTrackList->CountItems(); i++) {
		track = (QTTrack *)fTrackList->ItemAt(i);
		
		// calculate duration based upon mvhdHeader.timescale
		switch (track->Type()) {
			case QT_VIDEO:
				WriteVideoTrack(track);
				break;
				
			case QT_AUDIO:
				WriteAudioTrack(track);
				break;
		}
	}
		
	if (fCopyright) {
		WriteCopyright(fCopyright);
	}


	// all of header data
	fOutStream->Seek(0,SEEK_END);
	ReplaceLength(moovPos,fOutStream->Position()-moovPos);	
	
	return QT_OK;
}

void QTAtomWriter::ReplaceEndTrak()
{
	// this function replaces all the values that are needed to correctly terminate
	// the current track

	off_t		endPosition;
	
	// save position we are in
	endPosition = fOutStream->Position();

	// everything up to the sample tables is part of one track
	if (fTRAKPosition > 0) ReplaceLength(fTRAKPosition,endPosition-fTRAKPosition);
	// everything up to hdlr and mdhd
	if (fMDIAPosition > 0) ReplaceLength(fMDIAPosition,endPosition-fMDIAPosition);
	// sample tables
	if (fSTBLPosition > 0) ReplaceLength(fSTBLPosition,endPosition-fSTBLPosition);
	// media information
	if (fMINFPosition > 0) ReplaceLength(fMINFPosition,endPosition-fMINFPosition);
	
	fOutStream->Seek(endPosition,SEEK_SET);
	
	_QTdebugprint("QTAtomWriter: ReplaceEndTrak() done\n");
}

void QTAtomWriter::PrintTrackCount()
{
	printf("fTrackCount=%ld\n",fTrackCount);
}
