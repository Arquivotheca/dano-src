//---------------------------------------------------------------------
//
//	File:	TRIFFWriter.cpp
//
//	Author:	Gene Z. Ragan
//
//	Date:	08.25.98
//
//	Desc:	RIFF File Format Writer
//
//
//---------------------------------------------------------------------


//	Includes
#include <Debug.h>

#include "BuildApp.h"

#include "AVIUtils.h"
#include "DebugUtils.h"

#include "AVINodeConstants.h"

#include "TRIFFWriter.h"

#include <errno.h>
    
//-------------------------------------------------------------------
//	WriteInt16Lsb
//-------------------------------------------------------------------
//
//

status_t
WriteInt16Lsb(FileWriter *theFile, uint16 data) 
{
	uint16 outValue = B_HOST_TO_LENDIAN_INT16(data);
	return theFile->Write(&outValue, sizeof(uint16));
}

//-------------------------------------------------------------------
//	WriteInt32Lsb
//-------------------------------------------------------------------
//
//

status_t
WriteInt32Lsb(FileWriter *theFile, uint32 data) 
{
	uint32 outValue = B_HOST_TO_LENDIAN_INT32(data);
	return theFile->Write(&outValue, sizeof(uint32));
}


//-------------------------------------------------------------------
//	WriteInt32Msb
//-------------------------------------------------------------------
//
//

status_t
WriteInt32Msb(FileWriter *theFile, uint32 data) 
{
	uint32 outValue = B_HOST_TO_BENDIAN_INT32(data);
	return theFile->Write(&outValue, sizeof(uint32));
}

//-------------------------------------------------------------------
//	Constructor
//-------------------------------------------------------------------
//
//

TRIFFWriter::TRIFFWriter(FileWriter *file)
{
	
	//	Save pointer to file
	m_File = file;
			
	//	Default initialization
	Init();	
}


//-------------------------------------------------------------------
//	Destructor
//-------------------------------------------------------------------
//
//

TRIFFWriter::~TRIFFWriter() 
{
	//	Clean up
	if (m_Copyright)
		free(m_Copyright);
	m_Copyright = NULL;
	
	if (m_Idx1Info)
		free(m_Idx1Info);
	m_Idx1Info = NULL;
	

}

//-------------------------------------------------------------------
//	Init
//-------------------------------------------------------------------
//
//	Perform defualt initialization
//

void TRIFFWriter::Init() 
{
	m_InitComplete		= false;
	m_Copyright 		= NULL;
	m_StreamCount 		= 0;
	m_IndexCount 		= 0;
	m_TotalFrames		= 0;
	m_MoviChunkOffset 	= -1;
	m_Idx1Info			= NULL;
			
	//	Rewind file
	m_File->Seek(0);
	
	//	Clear out headers
	m_AUDSHeader.ByteCount = 0;
			
}

#pragma mark -
#pragma mark === File Handling ===

//-------------------------------------------------------------------
//	InitAVIFile
//-------------------------------------------------------------------
//
//	Write AVI file headers and other info that can be 
//	determined at start of write activity.
//

bool TRIFFWriter::InitAVIFile(bool use_raw_data)
{	
	
	//	We cannot write out if all elements are not initialized...	
	off_t	savePos;	
	bool 	retVal 	= true;
	ssize_t size 	= 0x4949;
	uint32 outValue;
	
	//	Rewind the file
	//rewind(m_File);
	m_File->Seek(0);
	
	//	Write RIFF header
	WriteRIFFChunk(size);
	
	//	Write AVIHeader LIST chunk
	retVal = WriteLISTChunk(size);
	if (retVal == false)
		return retVal;
	
	//	Save position for later calculation.  Back over size...
	uint32 listHeaderPos = m_File->Position();
	listHeaderPos -= sizeof(uint32);
	WriteInt32Msb(m_File, kRiff_hdrl_Chunk);

	//	Write AVIHeader chunk
	retVal = WriteavihChunk();
	if (retVal == false)
		return retVal;
	
	//
	//	Write stream data List Chunk
	//
	
	//	Make sure we have streams to write
	if ( (m_StreamCount == 0) || (m_StreamCount > kRiffWriteMaxStreams) )
		return false;
	
	//
	//	Write out first stream
	//
		
	retVal = WriteLISTChunk(size);
	if (retVal == false)
		return retVal;
			
	//	Save position for later size calculation
	uint32 streamOnePos = m_File->Position();
	m_streamOnePos = streamOnePos;

	//	Write out 'strl' chunk
	WriteInt32Msb(m_File, kRiff_strl_Chunk);
		
	//	First stream header
	retVal = WriteStreamHeader(&m_StreamHeaderOne);
	
	//	First stream format
	retVal = WriteStreamFormat(&m_StreamHeaderOne);

	//	Update streamOne chunk size
	savePos = m_File->Position();
	uint32 streamOneSize = (uint32)(savePos - streamOnePos);
	
	//	Update list header chunk size
	outValue = B_HOST_TO_LENDIAN_INT32(streamOneSize);
	streamOnePos -= sizeof(uint32);
	m_File->Seek(streamOnePos);
	m_File->Write(&outValue, sizeof(uint32));

	//	Return to saved postion
	m_File->Seek(savePos);
	
	//
	//	Write out second stream
	//
	
	if (m_StreamCount == 2)
	{
		retVal = WriteLISTChunk(size);
		if (retVal == false)
			return retVal;
	
       m_AudioDivisor = m_AUDSHeader.Channels * (m_AUDSHeader.BitsPerSample / 8);
	   if (m_AudioDivisor <= 0)
		   m_AudioDivisor = 1;

		//	Save position for later size calculation
		uint32 streamTwoPos = m_File->Position();
		m_streamTwoPos = streamTwoPos;

		//	Write out 'strl' chunk
		WriteInt32Msb(m_File, kRiff_strl_Chunk);
		
		//	Second stream header
		retVal = WriteStreamHeader(&m_StreamHeaderTwo);
		
		//	Second stream format
		retVal = WriteStreamFormat(&m_StreamHeaderTwo);
		
		//	Update streamTwo chunk size
		savePos = m_File->Position();
		uint32 streamTwoSize = savePos - streamTwoPos;
		
		//	Update list header chunk size
		outValue = B_HOST_TO_LENDIAN_INT32(streamTwoSize);
		streamTwoPos -= sizeof(uint32);
		m_File->Seek(streamTwoPos);
		m_File->Write(&outValue, sizeof(uint32));
		
		//	Restore position
		m_File->Seek(savePos);
	}
			
	if (m_Copyright) {    // write out an info chunk with this in it
		int len = strlen(m_Copyright);

		WriteInt32Msb(m_File, kRiff_INFO_Chunk);
		WriteInt32Lsb(m_File, len + 4);
		WriteInt32Lsb(m_File, 0);

		m_File->Write(m_Copyright, len);
		m_File->Skip(len & 1);
	}

	//	Save position and go back and update unitialized chunk sizes
	savePos = m_File->Position();
	
	uint32 listHeaderSize = savePos - listHeaderPos - sizeof(uint32);
	
	//	Update list header chunk size
	outValue = B_HOST_TO_LENDIAN_INT32(listHeaderSize);
	m_File->Seek(listHeaderPos);
	m_File->Write(&outValue, sizeof(uint32));
	
	//	Restore position
	m_File->Seek(savePos);

	raw_data = use_raw_data;

	off_t nextpos;
	if(raw_data)
		nextpos = savePos + ((-savePos-12-8) & 0x7ff);
	else
		nextpos = savePos + ((-savePos-12) & 0x7ff);

	WriteInt32Msb(m_File, kRiff_JUNK_Chunk);
	WriteInt32Lsb(m_File, nextpos-savePos-8);
	// Write data instead of seeking. This makes the file "cleaner" and
	// easier to parse manually.
	{
		uchar c = 0xbe;
		for(int i=0;i<nextpos-savePos-8;i++)
			m_File->Write(&c,1);
	}

	//
	//	Write movi Chunk.  We don't know the true size yet...
	//
	
	retVal = WriteLISTChunk(size);
	if (retVal == false)
		return retVal;
		
	WriteInt32Msb(m_File, kRiff_movi_Chunk);
	
	//	Save offset for later index chunk calculation
	m_MoviChunkOffset = m_File->Position();
	//m_MoviChunkOffset -= sizeof(uint32);
	
	m_Idx1Info     = NULL;
	m_Idx1Count    = 0;
	m_MaxIdx1Count = 0;

	//fflush(m_File);
	//fsync(fileno(m_File));
	
	if(raw_data) {
		m_File->Skip(8);
	}

	//	All done
	m_InitComplete = retVal;
	return retVal;
	
}



//-------------------------------------------------------------------
//	CompleteAVIFile
//-------------------------------------------------------------------
//
//	Complete update of unfilled header fields
//

bool TRIFFWriter::CompleteAVIFile()
{
	FUNCTION("TRIFFWriter::CompleteAVIFile()\n");
	
	bool 	retVal = true;
	uint32 	streamSize;
	uint32  outValue;	
	off_t   fileSize;
	
	// make sure we've been initialized
	if (!m_InitComplete) {
		return m_InitComplete;
	}

	fileSize = m_File->Position();
	
	//	Rewind, Skip over RIFF form, AVI form, LIST chunk and hdrl chunk
	m_File->Seek(2*sizeof(uint32) + sizeof(uint32) * 6);
	
	//
	//	Update unitialized AVIHeader fields
	//
			
	//	Skip filled fields and update fields needing info
	m_File->Skip(sizeof(uint32) * 4);
	outValue = B_HOST_TO_LENDIAN_INT32(m_TotalFrames);
	m_File->Write(&outValue, sizeof(uint32));	

	//	Skip	
	m_File->Skip(sizeof(uint32) * 8);

	//	Frames.  Same value as above
	m_File->Write(&outValue, sizeof(uint32));
	
	//
	//	Update unitialized stream header and format fields
	//	
	
	//	The first should always be the video stream
	
	//	Skip LIST chunk
	m_File->Skip(sizeof(uint32) * 3);
	
	//	Skip stream header chunkID and size
	m_File->Skip(sizeof(uint32) * 2);
	
	//	Skip filled fields and update DataLength.  Skip over rest of chunk...
	m_File->Skip(sizeof(uint32) * 8);
	
	//	Frames.  Same value as above
	m_File->Write(&outValue, sizeof(uint32));
	m_File->Skip(sizeof(uint32) * 3);

	// skip bogus rect thingie
	m_File->Skip(sizeof(uint16) * 4);
	
	//	Skip vids StreamFormat chunk
	m_File->Skip(sizeof(uint32));
	m_File->Read(&streamSize, sizeof(uint32));
	streamSize = B_LENDIAN_TO_HOST_INT32(streamSize);
	m_File->Skip(streamSize);
	
	//	Is there an audio chunk to update?  If so, it will always be the second stream
	if ( m_StreamCount > 1)
	{
        //  Skip LIST chunk, stream header and intervening data
		m_File->Seek(m_streamTwoPos + (4*sizeof(uint32)) + 28);

		//  update the DataLength field
		outValue = B_HOST_TO_LENDIAN_INT32(m_AUDSHeader.ByteCount);
		m_File->Write(&outValue, sizeof(uint32));
	}
	
	//
	//	Write out size of 'movi' chunk
	//
		
	//	Seek to start of 'movi' index
	off_t seekPos = m_MoviChunkOffset - ( sizeof(uint32) * 2);
	m_File->Seek(seekPos);
	
	//	write out chunkSize
	uint32 moviChunkSize = fileSize - m_MoviChunkOffset;
	moviChunkSize += sizeof(uint32);
	outValue = B_HOST_TO_LENDIAN_INT32(moviChunkSize);
	m_File->Write(&outValue, sizeof(uint32));
	
	//	Go to end of file and write out index chunk
	m_File->Seek(fileSize);
	retVal = Writeidx1Chunk();
		
	fileSize = m_File->Position();
	//	Rewind and Skip over RIFF form
	m_File->Seek(sizeof(uint32));
	//	Write out file size
	outValue = B_HOST_TO_LENDIAN_INT32((fileSize - 8));
	m_File->Write(&outValue, sizeof(uint32));

	PROGRESS("Total Frames: %ld\n", m_TotalFrames);
	
	return retVal;

}
				
#pragma mark -
#pragma mark === Chunk Routines ===

//-------------------------------------------------------------------
//	WriteRIFFChunk
//-------------------------------------------------------------------
//
//

bool TRIFFWriter::WriteRIFFChunk(uint32 size) 
{	
	FUNCTION("Writing RIFF chunk...\n");
	
	// Seek to beginning of file
	m_File->Seek(0);
		
	//	Write RIFF form
	WriteInt32Msb(m_File, kRiff_RIFF_Chunk);
		
	//	Write file size
	WriteInt32Lsb(m_File, size);
		
	//	Write AVI form
	WriteInt32Msb(m_File, kRiff_AVI);
		
	return true;		
}

//-------------------------------------------------------------------
//	WriteLISTChunk
//-------------------------------------------------------------------
//
//

bool TRIFFWriter::WriteLISTChunk(uint32 size) 
{	
	FUNCTION("Writing LIST...\n");
	
	//	Write LIST chunk
	WriteInt32Msb(m_File, kRiff_LIST_Chunk);
		
	//	Write chunk size
	WriteInt32Lsb(m_File, size + 8);
		
	return true; 	
}


//-------------------------------------------------------------------
//	WriteavihChunk
//-------------------------------------------------------------------
//
//	Write AVIHeader chunk
//

bool TRIFFWriter::WriteavihChunk() 
{			
	FUNCTION("Writing 'avif' chunk...\n");
			
	//	Write out 'avih' chunkID
	WriteInt32Msb(m_File, kRiff_avih_Chunk);
	
	//	Write out size of avih data
	WriteInt32Lsb(m_File, sizeof(AVIHeader));
		
	//	Write out the header
	bool retVal = WriteAVIHeader();
		
	return retVal;
}


//-------------------------------------------------------------------
//	WriteStreamHeader
//-------------------------------------------------------------------
//
//

bool TRIFFWriter::WriteStreamHeader(AVIStreamHeader *streamHeader)
{		
	FUNCTION("Writing strh chunk...\n");
	
	//	Write chunkID
	WriteInt32Msb(m_File, kRiff_strh_Chunk);
	
	//	Write chunkSize
	WriteInt32Lsb(m_File, sizeof(AVIStreamHeader));	
	
	//	Write header
	WriteInt32Msb(m_File, streamHeader->DataType);		
	WriteInt32Msb(m_File, streamHeader->DataHandler);			
	WriteInt32Lsb(m_File, streamHeader->Flags);				
	WriteInt32Lsb(m_File, streamHeader->Priority);
	WriteInt32Lsb(m_File, streamHeader->InitialFrames);					
	WriteInt32Lsb(m_File, streamHeader->TimeScale);					
	WriteInt32Lsb(m_File, streamHeader->DataRate);					
	WriteInt32Lsb(m_File, streamHeader->StartTime);						
	WriteInt32Lsb(m_File, streamHeader->DataLength);					
	WriteInt32Lsb(m_File, streamHeader->SuggestedBufferSize);					
	WriteInt32Lsb(m_File, streamHeader->Quality);					
	WriteInt32Lsb(m_File, streamHeader->SampleSize);

//	This is a pain, so we want to not write it!
	WriteInt32Lsb(m_File, streamHeader->rect[0]);
	WriteInt32Lsb(m_File, streamHeader->rect[1]);
	WriteInt32Lsb(m_File, streamHeader->rect[2]);
	WriteInt32Lsb(m_File, streamHeader->rect[3]);
					
  
	//DumpAVIStreamHeader(streamHeader);
	
	return true;
}


//-------------------------------------------------------------------
//	WriteStreamFormat
//-------------------------------------------------------------------
//
//	Write out StreamFormat chunk.  We handle the following types:
//		
//		--	'auds'	(Audio)
//		--	'vids'	(Video) 
//

bool TRIFFWriter::WriteStreamFormat(AVIStreamHeader *streamHeader) 
{	
	FUNCTION("Writing strf...\n");
	
	bool retVal = false;
	

	//	Write out stream based on previouslly determined format
	switch(streamHeader->DataType)
	{
		case kRiff_vids_Chunk:			
			retVal = WritevidsChunk();
			break;

		case kRiff_auds_Chunk:			
			retVal = WriteaudsChunk();
			break;
									
		//	Unknown
		default:							
			break;
	}

	return retVal;
}


//-------------------------------------------------------------------
//	WritevidsChunk
//-------------------------------------------------------------------
//
//	Write 'vids' Chunk
//

bool TRIFFWriter::WritevidsChunk() 
{		
	FUNCTION("Writing vids...\n");
		
	bool retVal = true;
	
	//	Write chunkID
	WriteInt32Msb(m_File, kRiff_strf_Chunk);		
	
	//	Write chunkSize
	WriteInt32Lsb(m_File, sizeof(AVIVIDSHeader));
	
	//	Write out header
	WriteInt32Lsb(m_File, m_VIDSHeader.Size);
	WriteInt32Lsb(m_File, m_VIDSHeader.Width);
	WriteInt32Lsb(m_File, m_VIDSHeader.Height);
	WriteInt16Lsb(m_File, m_VIDSHeader.Planes);
	WriteInt16Lsb(m_File, m_VIDSHeader.BitCount);
	WriteInt32Msb(m_File, m_VIDSHeader.Compression);
	WriteInt32Lsb(m_File, m_VIDSHeader.ImageSize);
	WriteInt32Lsb(m_File, m_VIDSHeader.XPelsPerMeter);
	WriteInt32Lsb(m_File, m_VIDSHeader.YPelsPerMeter);
	WriteInt32Lsb(m_File, m_VIDSHeader.NumColors);
	WriteInt32Lsb(m_File, m_VIDSHeader.ImpColors);
	
	//DumpVIDSHeader(&m_VIDSHeader);
	
	return retVal;
}


//-------------------------------------------------------------------
//	WriteaudsChunk
//-------------------------------------------------------------------
//
//	Write 'auds' Chunk
//

bool TRIFFWriter::WriteaudsChunk() 
{		
	FUNCTION("Writing auds...\n");
	
	bool retVal = true;
	
	//	Write chunkID
	WriteInt32Msb(m_File, kRiff_strf_Chunk);
	
	//	Write chunkSize
	int32 base_size = 0x12;
	int32 extension_size = 6 + m_AUDSHeader.NumCoefficients * 4;

	int32 total_size = base_size;
	if(m_AUDSHeader.Format == WAVE_FORMAT_ADPCM)
		total_size += extension_size;

	WriteInt32Lsb(m_File, total_size);
	
	//	Write out header info
	WriteInt16Lsb(m_File, m_AUDSHeader.Format);
	WriteInt16Lsb(m_File, m_AUDSHeader.Channels);       			
	WriteInt32Lsb(m_File, m_AUDSHeader.SamplesPerSec);          		
	WriteInt32Lsb(m_File, m_AUDSHeader.AvgBytesPerSec);
	WriteInt16Lsb(m_File, m_AUDSHeader.BlockAlign);	
	WriteInt16Lsb(m_File, m_AUDSHeader.BitsPerSample);

	if(m_AUDSHeader.Format == WAVE_FORMAT_ADPCM)
	{
		WriteInt16Lsb(m_File, extension_size);	
		WriteInt16Lsb(m_File, m_AUDSHeader.SamplesPerBlock);
		WriteInt16Lsb(m_File, m_AUDSHeader.NumCoefficients);
		for(int i=0;i<m_AUDSHeader.NumCoefficients;i++)
		{
			WriteInt16Lsb(m_File, m_AUDSHeader.Coefficients[i].Coef1);	
			WriteInt16Lsb(m_File, m_AUDSHeader.Coefficients[i].Coef2);	
		}
		WriteInt16Lsb(m_File, 0);
		//WriteInt16Lsb(m_File, m_AUDSHeader.Style);	
		//WriteInt32Lsb(m_File, m_AUDSHeader.ByteCount);
	}
	else
		WriteInt16Lsb(m_File, 0);	// make it a complete 'WaveFormatEx' structure with zero extension size
		
	//DumpAUDSHeader(&m_AUDSHeader);
	
	return retVal;	
}

//-------------------------------------------------------------------
//	Writeidx1Chunk
//-------------------------------------------------------------------
//
//	Write 'idx1' Chunk
//

bool TRIFFWriter::Writeidx1Chunk() 
{
	if(m_Idx1Info == NULL)
		return B_ERROR;
	//	Write header info
	WriteInt32Msb(m_File, kRiff_idx1_Chunk);
	WriteInt32Lsb(m_File, m_Idx1Count * sizeof(AVIRawIndex));
	
	m_File->Write(m_Idx1Info, sizeof(AVIRawIndex)*m_Idx1Count);

	return true;
}

#pragma mark -
#pragma mark === AVI Header Routines ===

//-------------------------------------------------------------------
//	WriteAVIHeader
//-------------------------------------------------------------------
//
//	Write out AVIHeader
//

bool TRIFFWriter::WriteAVIHeader()
{		
	WriteInt32Lsb(m_File, m_AVIHeader.TimeBetweenFrames);
	WriteInt32Lsb(m_File, m_AVIHeader.MaximumDataRate);
	WriteInt32Lsb(m_File, m_AVIHeader.PaddingGranularity);
	WriteInt32Lsb(m_File, m_AVIHeader.Flags);
	WriteInt32Lsb(m_File, m_AVIHeader.TotalNumberOfFrames);
	WriteInt32Lsb(m_File, m_AVIHeader.NumberOfInitialFrames);
	WriteInt32Lsb(m_File, m_AVIHeader.NumberOfStreams);
	WriteInt32Lsb(m_File, m_AVIHeader.SuggestedBufferSize);
	WriteInt32Lsb(m_File, m_AVIHeader.Width);
	WriteInt32Lsb(m_File, m_AVIHeader.Height);
	WriteInt32Lsb(m_File, m_AVIHeader.TimeScale);
	WriteInt32Lsb(m_File, m_AVIHeader.DataRate);
	WriteInt32Lsb(m_File, m_AVIHeader.StartTime);
	WriteInt32Lsb(m_File, m_AVIHeader.DataLength);

	//DumpAVIHeader(&m_AVIHeader);
		
	//	Success
	return true;
}

#pragma mark -
#pragma mark === AVI Stream Routines ===

void
TRIFFWriter::AddIndex(uint32 chunkID, off_t pos, size_t length,int32 flags)
{
	if (m_Idx1Count >= m_MaxIdx1Count) {
		AVIRawIndex *ptr;

		m_MaxIdx1Count += 32;
		ptr = (AVIRawIndex *)realloc(m_Idx1Info, m_MaxIdx1Count*sizeof(AVIRawIndex));
		if (ptr == NULL) {
			printf("AVI writer out of memory\n");
			return;
		}

		m_Idx1Info = ptr;
	}

	m_Idx1Info[m_Idx1Count].ChunkID = B_HOST_TO_BENDIAN_INT32(chunkID);
	m_Idx1Info[m_Idx1Count].Flags   = B_HOST_TO_LENDIAN_INT32(flags);
	m_Idx1Info[m_Idx1Count].Length  = B_HOST_TO_LENDIAN_INT32(length);
	m_Idx1Info[m_Idx1Count].Offset  = B_HOST_TO_LENDIAN_INT32(pos-m_MoviChunkOffset+4);
	m_Idx1Count++;
}

//-------------------------------------------------------------------
//	Write00DCChunk
//-------------------------------------------------------------------
//
//

status_t
TRIFFWriter::Write00DCChunk(uint32 chunkID, int length, void *data,
                            media_encode_info *info)
{
	status_t err;
	off_t chunkpos = m_File->Position();
	if(raw_data)
		chunkpos -= 8;
//m_AVIHeader.TimeBetweenFrames = 40000;
	if((bigtime_t)m_AVIHeader.TimeBetweenFrames * m_TotalFrames > info->start_time + m_AVIHeader.TimeBetweenFrames) {
		printf("frame rate wrong,\n");
		printf("frame %d, %Ld > %Ld\n", m_TotalFrames, (bigtime_t)m_AVIHeader.TimeBetweenFrames * m_TotalFrames, info->start_time);
		return B_ERROR;
	}
	while((bigtime_t)m_AVIHeader.TimeBetweenFrames * m_TotalFrames + m_AVIHeader.TimeBetweenFrames/2 <= info->start_time) {
		if(m_TotalFrames == 0) {
			printf("AVI track need to start at time 0, got %Ld\n", info->start_time);
			return B_ERROR;
		}
		printf("repeat frame %d, %Ld < %Ld\n", m_TotalFrames, (bigtime_t)m_AVIHeader.TimeBetweenFrames * m_TotalFrames, info->start_time);
		AddIndex(chunkID, m_MoviChunkOffset-4, 0, 0);
	
		//	Increment our internal frame counter
		m_TotalFrames++;
		
		//	Increment our internal index counter
		m_IndexCount++;
	}

	bool is_key_frame = (info->flags & B_MEDIA_KEY_FRAME) != 0;

	//FUNCTION("Write00DCChunk ENTER & %d\n", ftell(m_File));

	if(!raw_data) { // skip headers for raw video
		struct {
			uint32	id;
			uint32	length;
		} header;

		header.id = B_HOST_TO_BENDIAN_INT32(chunkID);
		header.length = B_HOST_TO_LENDIAN_INT32(length);
		err = m_File->Write(&header, 8);
		if(err != B_NO_ERROR) {
			//printf("AVI: write frame header failed\n");
			return err;
		}
	}

	//	Write video data
	err = m_File->Write(data, length);
	if(err != B_NO_ERROR) {
		if(raw_data) {
			chunkpos += 8;
		}
		m_File->Seek(chunkpos);
		//printf("AVI: write frame failed\n");
		return err;
	}
	m_File->Skip(length & 0x1);

	AddIndex(chunkID, chunkpos, length, (is_key_frame) ? AVIIF_KEYFRAME : 0);

	//	Increment our internal frame counter
	m_TotalFrames++;
	
	//	Increment our internal index counter
	m_IndexCount++;
	
	//FUNCTION("Write00DCChunk EXIT\n");

	return B_NO_ERROR;
}




//-------------------------------------------------------------------
//	Write00WBChunk
//-------------------------------------------------------------------
//
//	Compressed waveform data
//

status_t
TRIFFWriter::Write00WBChunk(uint32 chunkID, int length, void *data,
                            media_encode_info *info)
{
	status_t err;
	off_t chunkpos = m_File->Position();
	if(raw_data)
		chunkpos -= 8;

	bool is_key_frame = (info->flags & B_MEDIA_KEY_FRAME) != 0;

	if(!raw_data) { // skip headers for raw video
		struct {
			uint32	id;
			uint32	length;
		} header;

		header.id = B_HOST_TO_BENDIAN_INT32(chunkID);
		header.length = B_HOST_TO_LENDIAN_INT32(length);
		err = m_File->Write(&header, 8);
		if(err != B_NO_ERROR) {
			//printf("AVI: write frame header failed\n");
			return err;
		}
	}

	err = m_File->Write(data, length);
	if(err != B_NO_ERROR) {
		if(raw_data) {
			chunkpos += 8;
		}
		m_File->Seek(chunkpos);
		//printf("AVI: write frame failed\n");
		return err;
	}
	m_File->Skip(length & 0x1);

	//	Increment audio byte count
	m_AUDSHeader.ByteCount += length / m_AudioDivisor;
	
	//	Increment our internal index counter
	m_IndexCount++;

	AddIndex(chunkID, chunkpos, length, (is_key_frame) ? AVIIF_KEYFRAME : 0);

	return B_NO_ERROR;
}


#pragma mark -
#pragma mark === Utility Routines ===

//-------------------------------------------------------------------
//	SetCopyright
//-------------------------------------------------------------------
//
//

bool TRIFFWriter::SetCopyright(const char *data)
{
	if (m_Copyright)
		free(m_Copyright);
	
	m_Copyright = strdup(data);
	
	return (m_Copyright) ? true : false;

}

//-------------------------------------------------------------------
//	SetStreamCount
//-------------------------------------------------------------------
//
//

bool TRIFFWriter::SetStreamCount(int32 streamCount)
{
	//	Verify we have at least one stream
	if (streamCount <= 0)
		return false;
		
	//	Verify we have two streams or less
	if (streamCount > 2)
		return false;
	
	//	Set streams		
	m_StreamCount = streamCount;
	
	return true;

}
