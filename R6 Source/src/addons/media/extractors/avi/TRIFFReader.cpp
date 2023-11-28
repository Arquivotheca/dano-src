//---------------------------------------------------------------------
//
//	File:	TRIFFReader.cpp
//
//	Author:	Gene Z. Ragan
//
//	Date:	08.25.98
//
//	Desc:	RIFF File Format
//
//	Copyright 1998 mediapede Software
//
//---------------------------------------------------------------------

//	Includes
#include "BuildApp.h"

#include <Debug.h>
#include <string.h>

#include "AppUtils.h"
#include "DebugUtils.h"
#include "AVIUtils.h"

#include "AVINodeConstants.h"
#include "AVINodeTypes.h"

#include "TRIFFReader.h"




//	Macros
#define ChunkName(a,b,c,d) (                 	\
    ((static_cast<unsigned long>(a)&255)<<24)	\
  + ((static_cast<unsigned long>(b)&255)<<16)  	\
  + ((static_cast<unsigned long>(c)&255)<<8)  	\
  + ((static_cast<unsigned long>(d)&255)))
  
  
//-------------------------------------------------------------------
//	IsRIFFFile
//-------------------------------------------------------------------
//
//	Determine file type
//

bool IsRIFFFile(BPositionIO *file) 
{
	// Seek to beginning of file
	file->Seek(0, SEEK_SET); 
	
	uint32 form;
	
	//	Get first chunk
	form = ReadIntMsb(file, 4);
	
	//	Form should be 'RIFF'
	if (form != ChunkName('R','I','F','F'))
		return false;
	
	//	Skip size indicator
	file->Seek(sizeof(uint32), SEEK_CUR);	
	
	//	Get next chunk
	form = ReadIntMsb(file, 4);
	
	//	Type should be 'AVI '
	if (form == ChunkName('A','V','I',' '))
		return true;
		
	// 	RIFF file, but not AVI file
	ERROR("Unsupported RIFF file.\n");
	return false; 	
}


//-------------------------------------------------------------------
//	Constructor
//-------------------------------------------------------------------
//
//

TRIFFReader::TRIFFReader(BPositionIO *file)
{	
	//	Save pointer to file	
	m_VideoFile = file;

	//	Set up defaults
	m_VideoChunkList = NULL;
	m_AudioChunkList = NULL;
	m_AudioFile		 = NULL;
	
	m_VideoFrameIndex = 0;
	m_AudioFrameIndex = 0;
			
	m_Copyright = NULL;
	
	m_VIDSHeaderPtr = NULL;

	//	Default initialization
	Init();	
}


//-------------------------------------------------------------------
//	Destructor
//-------------------------------------------------------------------
//
//

TRIFFReader::~TRIFFReader() 
{
	//	Clean up
	if (m_Copyright)
		free(m_Copyright);
	m_Copyright = NULL;
	
	if(m_VIDSHeaderPtr)
		free(m_VIDSHeaderPtr);

	// Free video frame lists (the list contains pointers to the headerindex
	// so we don't have to free the individual pointers)
	if (m_VideoChunkList)
	{
		delete m_VideoChunkList;
		m_VideoChunkList = NULL;
	}
	
	// Free audio frame lists.  Here we have to check each individual
	// index structure to see if it should be free'd since AddAudioChunk()
	// may have synthesized entries if the audio chunks in the file were
	// too big.
	if (m_AudioChunkList)
	{
		// we have to go through each item and check if it needs to be free'd
		for( uint32 index = 0; index < m_AudioChunkList->CountItems(); index++)
		{
			AVIIndex *theFrame = (AVIIndex *)m_AudioChunkList->ItemAt(index);
			
			if (theFrame->Flags & AVIIF_MUST_FREE)
				free(theFrame);
		}
		
		delete m_AudioChunkList;
		m_AudioChunkList = NULL;
	}
	
	if (m_HeaderIndex)
		free(m_HeaderIndex);
	m_HeaderIndex = NULL;
		

	if (m_STRDSize != 0) {
		free(m_STRDBuffer);
		m_STRDBuffer = NULL;
	}
}

//-------------------------------------------------------------------
//	Init
//-------------------------------------------------------------------
//
//	Perform default initialization
//

void TRIFFReader::Init() 
{
	//	Set stream count to 0
	m_StreamCount 		= 0;
	m_InitCheck			= false;
	m_HasVideo 			= false;
	m_HasAudio			= false;
	m_IndexCount 		= 0;
	m_HeaderIndex		= NULL;
	m_AudioFrameCount 	= 0;
	
	m_STRDSize = 0;
	
	for(int32 index = 0; index < kRiffMaxStreams; index++)
	{ 
		m_StreamType[index] = 0; 
		m_StreamOK[index] 	= false; 
	}
		
	//	Rewind file		
	if(m_VideoFile)
		m_VideoFile->Seek(0, SEEK_SET);
		
	//	Init frame lists
	m_VideoChunkList = new BList();
	m_AudioChunkList = new BList();

	//	Process
	bool retVal = ParseAVIFile();
	
	//	Abort if failure
	if (retVal == false)
	{
		ERROR("TRIFFReader::Init() - ParseAVIFile() failure-\n");
		m_InitCheck = false;
		return;
	}
		
	if (m_HasAudio)
	{		
		//	Create copy of file for audio access
		// m_AudioFile = new BFile(*m_VideoFile);	
		m_AudioFile = m_VideoFile;                     // XXXdbg 
		m_AudioFile->Seek(0, SEEK_SET);
	}
		
#if 0
	//	Init Codecs
	retVal = InitCodecs();
	
	//	Set internal InitCheck value
	if (retVal == false)
	{
		ERROR("TRIFFReader::Init() - InitCodecs() failure-\n");
		m_InitCheck = false;
	}
	else
#endif
		m_InitCheck = true;
}


//-------------------------------------------------------------------
//	InitCheck
//-------------------------------------------------------------------
//
//	Return state of initialization
//

bool TRIFFReader::InitCheck() 
{
	return m_InitCheck;
}

#pragma mark -
#pragma mark === File Parsing ===

//-------------------------------------------------------------------
//	ParseAVIFile
//-------------------------------------------------------------------
//
//	Traverse thorugh AVI file.
//

bool TRIFFReader::ParseAVIFile()
{	
	
	uint32 			chunkID;
	uint32 			chunkSize;
	
	m_MoviChunkOffset	= 0;
	m_AVISize 			= 1;
	m_BeOS_largefileblocks = 0;
	m_UseIndexFlag 		= false;
	bool retVal			= true;
	
		
	//	Get file size
	off_t fileSize;
	fileSize = m_VideoFile->Seek(0, SEEK_END);
	m_VideoFile->Seek(0, SEEK_SET);

	if(fileSize > 0xffffffffLL) {
		m_BeOS_largefileblocks = fileSize / 0x100000000LL;
	}
	
	//	Traverse through entire file
	while( (m_VideoFile->Position() <= fileSize) && (m_AVISize > 0) )
	{
		//	Read in chunk and size identifiers.  We are proceeding through switch
		//	in the most likely order of chunks appearence in the file	
		chunkID 	= ReadIntMsb(m_VideoFile, sizeof(uint32));
		chunkSize 	= ReadIntLsb(m_VideoFile, sizeof(uint32));
		PROGRESS("offset %Ld: chunk %.4s (0x%x) sz %d\n", m_VideoFile->Position(),
				 &chunkID, chunkID, chunkSize);
		
		DumpRIFFID(chunkID);
		
		if (chunkSize > fileSize) {
			ERROR("bogus chunk size %d (file is probably bad)\n", chunkSize);
			retVal = false;
			break;
		}

		//	Subtract from total RIFF size
		m_AVISize -= sizeof(uint32) * 2;

		//	Handle various chunk formats
		switch(chunkID)
		{
			//	'RIFF'
			case kRiff_RIFF_Chunk:
				retVal = ReadRIFFChunk(chunkSize);
				break;
				
			//	'LIST'
			case kRiff_LIST_Chunk:				
				retVal = ReadLISTChunk(chunkSize);
				break;
				
			//	'avih'
			case kRiff_avih_Chunk:				
				retVal = ReadavihChunk(chunkSize);
				break;
				
			//	'strh'
			case kRiff_strh_Chunk:				
				retVal = ReadStreamHeader(chunkSize);				
				break;

			//	'strd'
			case kRiff_strd_Chunk:
				retVal = ReadstrdChunk(chunkSize);
				break;
			
			//	'strf'
			case kRiff_strf_Chunk:				
				retVal = ReadStreamFormat(chunkSize);				
				break;
							
			//	'00pc'
			//	'01pc'
			case kRiff_00pc_Chunk:
			case kRiff_01pc_Chunk:				
				break;
				
			//	'idx1'
			case kRiff_idx1_Chunk:
				//PROGRESS("Reading idx1...\n");
				retVal = Readidx1Chunk(chunkSize);
				break;
			
			//	Skip over these
			case kRiff_hdrl_Chunk:
			case kRiff_strl_Chunk:
			case kRiff_vedt_Chunk:
			case kRiff_vids_Chunk:
			case kRiff_00AM_Chunk:
			case kRiff_DISP_Chunk:
			case kRiff_ISBJ_Chunk:
			case kRiff_JUNK_Chunk:
				{
					//	Pad
					if (chunkSize & 0x01) 
						chunkSize++;
			
					//	Skip these chunks
					m_VideoFile->Seek(chunkSize, SEEK_CUR);
					
					PROGRESS("Skipping ");
					DumpRIFFID(chunkID);
				}
				break;

			case kRiff_INFO_Chunk:
				ReadINFOChunk(chunkSize);
				break;
			
			//	Handle all others and end of file
			default:
				{
					//	Are we past the end of the file?
					if ( m_AVISize <= 0 )
					{
						//	Seek to end of file
					}
					
					//	Handle stream chunks?
					
					//
					ERROR("ParseAVIFIle::Unknown RIFF Chunk ");

					//	Pad
					if (chunkSize & 0x01) 
						chunkSize++;
					m_VideoFile->Seek(chunkSize, SEEK_CUR);
					
					PROGRESS("Skipping ");
					DumpRIFFID(chunkID);
					
				}
				break;	
		}
				
		//	Check for errors
		if (retVal == false)
		{
			ERROR("TRIFFReader::ParseAVIFile() - Parse Error -\n");
			return retVal;
		}
			
		//	Deduct size
		m_AVISize -= chunkSize;
		if (chunkSize & 0x01)
			m_AVISize--;		
	}
	
	PROGRESS("TRIFFReader::ParseAVIFile() - Completed parsing loop - \n");
	PROGRESS("TRIFFReader::ParseAVIFile() - m_AVISize == %Ld - \n", m_AVISize);
	PROGRESS("TRIFFReader::ParseAVIFile() - fileSize == %Ld - \n", fileSize);
	//PROGRESS("TRIFFReader::ParseAVIFile() - Completed parsing loop - \n");
	//while( (m_VideoFile->Position() <= fileSize) && (m_AVISize > 0) )
	
	//	Complete initialization
	if (m_HasVideo)
	{
		if (m_VideoChunkList->CountItems() < 1) {
			m_HasVideo = false;
			delete m_VideoChunkList;
			m_VideoChunkList = NULL;
			ERROR("*** no video chunk list at end of parsing avi file.\n");
		}
	}

	if (m_HasAudio)
	{
		if (m_AudioChunkList->CountItems() < 1) {
			// then hmmm, it probably doesn't really have audio...
			m_HasAudio = false;
			delete m_AudioChunkList;
			m_AudioChunkList = NULL;
			ERROR("*** No audio chunk list at end of parsing avi file.\n");
		} else {
			if (m_AUDSHeader.BlockAlign == 0) 
				m_AUDSHeader.BlockAlign = 1;
		
			//	Use float to avoid uint32 overflow
			if (m_AUDSHeader.SamplesPerSec)
			{
				m_TotalAudioTime = (uint32) ((((float) m_AUDSHeader.ByteCount * (float)m_AUDSHeader.SamplesPerBlock * 1000.0)
											  / (float)m_AUDSHeader.BlockAlign) / (float)m_AUDSHeader.SamplesPerSec);
			}
			else  
				m_TotalAudioTime = 0;   

			//	Calculate audio frame size in bytes
			m_AudioFrameSize = m_SampleSize * m_AUDSHeader.Channels;
			
			//	Calculate number of audio frames.  Use info from stream header
			m_AudioFrameCount = m_AUDSHeader.ByteCount / m_AudioFrameSize;
		
			//	Calculate total audio frames per chunk
			m_AudioFPC = m_AudioFrameCount / m_AudioChunkList->CountItems();
		}
	}
		
	if (m_HasVideo == false && m_HasAudio == false) {
		ERROR("AVI file has no audio or video chunks...?\n");
		return false;
	}

	//	Return success or failure
	return retVal;
}


#pragma mark -
#pragma mark === Chunk Routines ===

//-------------------------------------------------------------------
//	ReadRIFFChunk
//-------------------------------------------------------------------
//
//	Return true if this is a 'RIFF/AVI ' file

bool TRIFFReader::ReadRIFFChunk(uint32 size) 
{	

	PROGRESS("Reading RIFF chunk...\n");
	
	//	Save file size
	m_AVISize = (2 * (uint64)size) - sizeof(uint32);
	m_AVISize += m_BeOS_largefileblocks * 0x100000000LL;

	//	Get form
	uint32 form = ReadIntMsb(m_VideoFile, 4);
	
	//	Type should be 'AVI '
	if (form == ChunkName('A','V','I',' '))
		return true;
		
	// 	RIFF file, but not AVI file
	return false; 	
}

//-------------------------------------------------------------------
//	ReadListHeaderChunk
//-------------------------------------------------------------------
//
//	Read in ListHeader chunk.  This chunk contains the AVIHeader
//	and the StreamListHeader chunk defining the streams.
//

bool TRIFFReader::ReadLISTChunk(uint32 sizein) 
{	
	PROGRESS("Reading LIST...\n");
	
	uint32 chunkID = ReadIntMsb(m_VideoFile, sizeof(uint32));
	uint64 size = sizein;
	
	//	Is there an Index chunk?  If so, ship over
	//	movie list data.
	if ( (chunkID == kRiff_movi_Chunk) && m_UseIndexFlag )
	{
		m_MoviChunkOffset = m_VideoFile->Position();
		
		size += m_BeOS_largefileblocks * 0x100000000LL;
		m_AVISize -= m_BeOS_largefileblocks * 0x100000000LL;
		
		//	Back up over type code
		m_MoviChunkOffset -= 4;
		size -= 4;
								
		//	Pad
		if (size & 0x01)
			size++;
			
		//	Skip over the list	
		m_VideoFile->Seek( size,  SEEK_CUR);
	}
	else
	{
		// Re-add list size minus size of type
		m_AVISize += (size - 4);
	}
	
	return true; 	
}


//-------------------------------------------------------------------
//	ReadavihChunk
//-------------------------------------------------------------------
//
//	Read in AVIHeader chunk
//

bool TRIFFReader::ReadavihChunk(uint32 size) 
{			
	PROGRESS("Reading 'avif' chunk...\n");
	
	//	Verify size of ChunkSize.	It should be 56 bytes
	if (size != 0x38)
		return false;
		
	//	Load in the header
	bool retVal = ReadAVIHeader();
	
	if (retVal == true)
	{	
		//	Convert file time to milliseconds
		double fileTime = (double)((m_AVIHeader.TimeBetweenFrames)/1000.0); 
		
		//	Verify 2 streams or less
//
// XXXdbg -- this should be changed... we need to keep an array of
//		     streams.
//		
		if (m_AVIHeader.NumberOfStreams > 2)
		{
			ERROR("Unable to handle %d streams.", m_AVIHeader.NumberOfStreams);
			return false;
		}
		
		//	Check header flags
		if ( (m_AVIHeader.Flags & kAVIMustUseIndexFlag) || (m_AVIHeader.Flags & kAVIHasIndexFlag) )	
			m_UseIndexFlag = true;
		else 
			m_UseIndexFlag = false;	
			
		DumpAVIHeaderFlags(&m_AVIHeader);
	}
	
	return retVal;
}


//-------------------------------------------------------------------
//	ReadStreamHeaderChunk
//-------------------------------------------------------------------
//
//

bool TRIFFReader::ReadStreamHeader(uint32 size)
{		
	PROGRESS("Reading strh chunk...\n");
	
	//	Determine StreamHeader to use
	AVIStreamHeader *streamHeader;
	
	if (m_StreamCount == 0)
		streamHeader = &m_StreamHeaderOne;
	else
		streamHeader = &m_StreamHeaderTwo;
	
	//	Fill header
	streamHeader->DataType    			= ReadIntMsb(m_VideoFile, sizeof(uint32));
	streamHeader->DataHandler 			= ReadIntMsb(m_VideoFile, sizeof(uint32));
	streamHeader->Flags       			= ReadIntLsb(m_VideoFile, sizeof(uint32));
	streamHeader->Priority    			= ReadIntLsb(m_VideoFile, sizeof(uint32));
	streamHeader->InitialFrames	 		= ReadIntLsb(m_VideoFile, sizeof(uint32));
	streamHeader->TimeScale       		= ReadIntLsb(m_VideoFile, sizeof(uint32));
	streamHeader->DataRate        		= ReadIntLsb(m_VideoFile, sizeof(uint32));

	streamHeader->StartTime       		= ReadIntLsb(m_VideoFile, sizeof(uint32));
	streamHeader->DataLength     	 	= ReadIntLsb(m_VideoFile, sizeof(uint32));
	streamHeader->SuggestedBufferSize	= ReadIntLsb(m_VideoFile, sizeof(uint32));
	streamHeader->Quality     			= ReadIntLsb(m_VideoFile, sizeof(uint32));
	streamHeader->SampleSize  	 		= ReadIntLsb(m_VideoFile, sizeof(uint32));

	size -= 48;

	if (size == 8) {
		streamHeader->rect[0]			= ReadIntLsb(m_VideoFile, sizeof(uint16));
		streamHeader->rect[1]			= ReadIntLsb(m_VideoFile, sizeof(uint16));
		streamHeader->rect[2]			= ReadIntLsb(m_VideoFile, sizeof(uint16));
		streamHeader->rect[3]			= ReadIntLsb(m_VideoFile, sizeof(uint16));
		size -= 8;
	} else if (size >= sizeof(AVIStreamHeader)) {
		streamHeader->rect[0]			= ReadIntLsb(m_VideoFile, sizeof(uint32));
		streamHeader->rect[1]			= ReadIntLsb(m_VideoFile, sizeof(uint32));
		streamHeader->rect[2]			= ReadIntLsb(m_VideoFile, sizeof(uint32));
		streamHeader->rect[3]			= ReadIntLsb(m_VideoFile, sizeof(uint32));
		size -= 16;
	} else {
		streamHeader->rect[0]			= 0;
		streamHeader->rect[1]			= 0;
		streamHeader->rect[2]			= 0;
		streamHeader->rect[3]			= 0;
	}

	if ((int)size > 0)
		m_VideoFile->Seek(size, SEEK_CUR);
		
	DumpAVIStreamHeader(streamHeader);

	return true;
}


//-------------------------------------------------------------------
//	ReadstrdChunk
//-------------------------------------------------------------------
//
//	Read in 'strd' chunk.
//		

bool TRIFFReader::ReadstrdChunk(uint32 size) 
{	
	PROGRESS("Reading strd chunk...\n");

	bool retVal = false;
	
	//	Save current strd size
	m_STRDCurSize = size;
	
	//	Pad
	if (size & 0x01)
		size++;
		
	//	Allocate STRD buffer
	if (m_STRDSize == 0)
	{
		m_STRDSize = size;
		m_STRDBuffer = (uchar *)malloc(size);
		ASSERT(m_STRDBuffer);
	}
	//	Reallocate...
	else if (size > m_STRDSize)
	{
		uchar *tmpBuf;
		
		m_STRDSize = size;
		tmpBuf = (uchar *)realloc(m_STRDBuffer, size);
		ASSERT(tmpBuf);
		m_STRDBuffer = tmpBuf;	
	}
	
	//	Read in strd data 
	retVal = m_VideoFile->Read(m_STRDBuffer, size) == size;
	
	return retVal;
}

//-------------------------------------------------------------------
//	ReadStreamFormat
//-------------------------------------------------------------------
//
//	Read in StreamFormat chunk.  We handle the following types:
//		
//		--	'auds'	(Audio)
//		--	'vids'	(Video) 
//		--	'pads'	(Padding)
//		--	'txts'	(Text)
//

bool TRIFFReader::ReadStreamFormat(uint32 size) 
{	
	PROGRESS("Reading strf...\n");
	
	bool retVal = true;
	
	//	Pad
	if (size & 0x01)
		size++;

	//	Determine stream header to use	
	uint32 dataType;
	
	if (m_StreamCount == 0)
		dataType = m_StreamHeaderOne.DataType;		
	else
		dataType = m_StreamHeaderTwo.DataType;
	
	m_StreamType[m_StreamCount] = dataType;
				
	//	Read in stream based on previouslly determined format
	switch(dataType)
	{
		case kRiff_vids_Chunk:			
			retVal = ReadvidsChunk(size);
			m_StreamOK[m_StreamCount] = retVal;
			break;

		case kRiff_auds_Chunk:			
			retVal = ReadaudsChunk(size);
			m_StreamOK[m_StreamCount] = retVal;
			break;
			
		//	Skip
		case kRiff_pads_Chunk:			
			PROGRESS("strf/pads ignored.\n");
			m_StreamOK[m_StreamCount] = false;
			m_VideoFile->Seek(size, SEEK_CUR);
			break;

		//	Skip
		case kRiff_txts_Chunk:
			PROGRESS("strf/txts ignored.\n");
			m_StreamOK[m_StreamCount] = false;
			m_VideoFile->Seek(size, SEEK_CUR);
			break;
			
		//	Unknown.  Skip over
		default:							
			PROGRESS("Unknown strf/type ignored.\n");
			m_StreamOK[m_StreamCount] = false;
			m_VideoFile->Seek(size, SEEK_CUR);
			break;
	}
	
	//	Increment stream count
	m_StreamCount++;

	return retVal;
}

//-------------------------------------------------------------------
//	ReadVIDSChunk
//-------------------------------------------------------------------
//
//	Read 'vids' Chunk
//

bool TRIFFReader::ReadvidsChunk(uint32 size) 
{		
	PROGRESS("Reading vids...\n");
	
	bool retVal = false;
	
	//	Set our video flag to true
	m_HasVideo = true;
	
	
	//	Adjust size for padding
	if (size & 0x01) 
		size++;

	m_VIDSHeaderPtr = (AVIVIDSHeader*)calloc(1,size);
	if(m_VIDSHeaderPtr == NULL)
		return false;

	m_VIDSHeaderSize = size;
	//memset(&m_VIDSHeader, 0, sizeof(AVIVIDSHeader));
			
	//	Read in header
	m_VIDSHeaderPtr->Size        	= ReadIntLsb(m_VideoFile, sizeof(uint32));
	m_VIDSHeaderPtr->Width       	= ReadIntLsb(m_VideoFile, sizeof(uint32));
	m_VIDSHeaderPtr->Height      	= ReadIntLsb(m_VideoFile, sizeof(uint32));
	
	m_VIDSHeaderPtr->Planes      	= ReadIntLsb(m_VideoFile, sizeof(uint16));
	m_VIDSHeaderPtr->BitCount    	= ReadIntLsb(m_VideoFile, sizeof(uint16));
	m_VIDSHeaderPtr->Compression 	= ReadIntMsb(m_VideoFile, sizeof(uint32));
	m_VIDSHeaderPtr->ImageSize  	= ReadIntLsb(m_VideoFile, sizeof(uint32));
	m_VIDSHeaderPtr->XPelsPerMeter 	= ReadIntLsb(m_VideoFile, sizeof(uint32));
	m_VIDSHeaderPtr->YPelsPerMeter 	= ReadIntLsb(m_VideoFile, sizeof(uint32));
	m_VIDSHeaderPtr->NumColors  	= ReadIntLsb(m_VideoFile, sizeof(uint32));
	m_VIDSHeaderPtr->ImpColors  	= ReadIntLsb(m_VideoFile, sizeof(uint32));
	
	//	Adjust size
	size -= sizeof(AVIVIDSHeader);
		
#if 0  /* CODECS */
	//	Check for supported video codec
	switch(m_VIDSHeader.Compression)
	{
		
		default:
			retVal = true;
			break;
	}
#else
	retVal = true;
#endif
	
	//	Calculate amount of data left and read it
	if ((int)size > 0) {
		m_VideoFile->Read(m_VIDSHeaderPtr+1, size);
	}

	DumpVIDSHeader(m_VIDSHeaderPtr);
	
	return retVal;
}


//-------------------------------------------------------------------
//	ReadAUDSChunk
//-------------------------------------------------------------------
//
//	Read 'auds' Chunk
//

bool TRIFFReader::ReadaudsChunk(uint32 size) 
{		
	PROGRESS("Reading auds...\n");

	bool retVal = true;
	
	//	Set our audio flag to true
	m_HasAudio = true;

	memset(&m_AUDSHeader, 0, sizeof(AVIAUDSHeader));
	
	//	Read in header info
	m_AUDSHeader.Format				= ReadIntLsb(m_VideoFile, sizeof(uint16));
	m_AUDSHeader.Channels			= ReadIntLsb(m_VideoFile, sizeof(uint16));
	m_AUDSHeader.SamplesPerSec		= ReadIntLsb(m_VideoFile, sizeof(uint32));
	m_AUDSHeader.AvgBytesPerSec 	= ReadIntLsb(m_VideoFile, sizeof(uint32));
	m_AUDSHeader.BlockAlign 		= ReadIntLsb(m_VideoFile, sizeof(uint16));
	m_AUDSHeader.SamplesPerBlock	= 1;
	m_AUDSHeader.ByteCount			= 0;
	
	//	Adjust size for padding
	if (size & 0x01) 
		size++;

	//	Get sample size 
	if (size >= 0x10) 
	{ 
		m_AUDSHeader.BitsPerSample = ReadIntLsb(m_VideoFile, sizeof(uint16));
		size -= 0x10; 
	}
	//	Old style.  Set to 8-bits per sample
  	else  
  	{ 
  		m_AUDSHeader.BitsPerSample = 8; 
  		size -= 0x0e; 
  	}

	//	Figure out compression	
	switch(m_AUDSHeader.Format)
	{
		case WAVE_FORMAT_MPEG:
			{
				m_AudioType = kAudioLinear;
				m_AUDSHeader.BitsPerSample = 16;
				// correct bitrate for mpeg, since some mpeg-in-AVI (DivX) get it slightly wrong
				m_AUDSHeader.AvgBytesPerSec = (((m_AUDSHeader.AvgBytesPerSec * 8) + 500) / 1000) * 1000 / 8;
			}
			break;

		case WAVE_FORMAT_PCM:
			{
				switch(m_AUDSHeader.BitsPerSample)
				{
					case 8:
						m_AudioType = kAudioLinear;
						break;
						
					case 16:
						m_AudioType = kAudioSigned;
						break;
						
					default:
						m_AudioType = kAudioInvalid;
						break;									
				}			
			}
			break;
			
	
	
		case WAVE_FORMAT_ADPCM:
			{
				switch(m_AUDSHeader.BitsPerSample)
				{
					case 4:
						m_AudioType = kAudioADPCM;
						break;
						
					default:
						m_AudioType = kAudioInvalid;
						break;									
				}

				if(size<=0)
					break; // no extension

				m_AUDSHeader.ExtensionSize   = ReadIntLsb(m_VideoFile, sizeof(uint16));
				size -= sizeof(uint16);

				if(m_AUDSHeader.ExtensionSize<4)
					break; // extension size is insufficient

				m_AUDSHeader.SamplesPerBlock = ReadIntLsb(m_VideoFile, sizeof(uint16));
				m_AUDSHeader.NumCoefficients = ReadIntLsb(m_VideoFile, sizeof(uint16));
				size -= sizeof(uint16) * 2;
				
				PROGRESS("MSADPM_EXT: SamplesPerBlock %d NumCoefficients %d\n", m_AUDSHeader.SamplesPerBlock, m_AUDSHeader.NumCoefficients);
				for( uint32 index = 0; index < m_AUDSHeader.NumCoefficients; index++)
				{ 
					m_AUDSHeader.Coefficients[index].Coef1 = ReadIntLsb(m_VideoFile, sizeof(uint16));
					m_AUDSHeader.Coefficients[index].Coef2 = ReadIntLsb(m_VideoFile, sizeof(uint16));	
					size -= sizeof(uint16) * 2;
				}
			}
			break;
			
		case WAVE_FORMAT_DVI_ADPCM:
			{
				m_AudioType = kAudioDVI;
				m_AUDSHeader.ExtensionSize	 = ReadIntLsb(m_VideoFile, sizeof(uint16));
				m_AUDSHeader.SamplesPerBlock = ReadIntLsb(m_VideoFile, sizeof(uint16));
				size -= sizeof(uint16) * 2;
		
				PROGRESS("DVI: samples per block %d\n", m_AUDSHeader.SamplesPerBlock);
			}
			break;
	
		case WAVE_FORMAT_MULAW:
			{
				m_AudioType = kAudioULaw;
			}
			break;
			
		case WAVE_FORMAT_GSM610:
			{
				m_AudioType = kAudioMSGSM;
				m_AUDSHeader.ExtensionSize	 = ReadIntLsb(m_VideoFile, sizeof(uint16));
				m_AUDSHeader.SamplesPerBlock = ReadIntLsb(m_VideoFile, sizeof(uint16)); 
				size -= sizeof(uint16) * 2;
		
				PROGRESS("GSM: Samples per block %d\n", m_AUDSHeader.SamplesPerBlock);
			
				//	Init GSM tables here...			
			}
			break;
			
		default:
			m_AudioType = kAudioInvalid;
			//retVal = false;
			break;
	}
				
	//	Detemrine BPS
	switch(m_AUDSHeader.BitsPerSample)
	{
		case 4:
		case 8:
			m_SampleSize = 1;	
			break;

		case 16:
			m_SampleSize = 2;	
			break;

		case 32:
			m_SampleSize = 4;	
			break;

		default:
			m_SampleSize = 1000 + m_AUDSHeader.BitsPerSample;	
			break;		
	}
		
	m_AudioEnd   = 0;
	
	if (m_AUDSHeader.Channels > 2) 
		retVal = false;
	
	/*
	#ifdef DEBUG
	{
		ERROR("Audio Codec: "); 
		DumpAudioType(m_AUDSHeader.Format);
		ERROR(" Rate=%d Chans = %d bps = %d\n", m_AudioFrequency, m_AUDSHeader.Channels, m_AUDSHeader.Size);
		ERROR("block_align %d\n", m_AUDSHeader.BlockAlign);
	}
	#endif
	*/
	
	//	Modify type
	if (m_AUDSHeader.Channels == 2)	
		m_AudioType |= kAudioStereoMask;
		
	if (m_AUDSHeader.Channels == 2)		
		m_AudioType |= kAudioBPS2Mask;
		
	PROGRESS("size = %d retVal = %x\n", size, retVal);
	
	//	Seek through rest of chunk
	m_VideoFile->Seek(size, SEEK_CUR);
	
	DumpAUDSHeader(&m_AUDSHeader);
	
	return retVal;	
}

//-------------------------------------------------------------------
//	Readidx1Chunk
//-------------------------------------------------------------------
//
//	Read 'idx1' Chunk
//

bool TRIFFReader::Readidx1Chunk(uint32 size) 
{		
	bool retVal = true;

	//	Pad
	if (size & 0x01)
		size++;
	
	//	Set up count
	m_IndexCount = size >> 4;
	
	// Allocate index entries
	m_HeaderIndex = (AVIIndex *)malloc(m_IndexCount * sizeof(AVIIndex));
	
	//	Verify allocation
	if (m_HeaderIndex == 0)
	{
		ERROR("Error allocating AVIIndex. size %d\n", size);
		retVal = false;
		return retVal;
	}

	AVIRawIndex *indexbuffer;
	const int rawindexmask = 0x3fff;
	if(m_IndexCount > rawindexmask)
		indexbuffer = (AVIRawIndex *)malloc(sizeof(AVIRawIndex) * (rawindexmask+1));
	else
		indexbuffer = (AVIRawIndex *)malloc(sizeof(AVIRawIndex) * m_IndexCount);
	if (indexbuffer == 0)
	{
		ERROR("Error allocating AVIIndex buffer. size %d\n", size);
		retVal = false;
		return retVal;
	}
	
	//	Initialize minumum offset
	uint32 minOffset = 0xFFFFFFFF;
	
	uint32 lastoffset = 0;
	uint64 offsetdelta = 0;
	
	//	Iterate through and read index information
	for (uint32 index = 0; index < m_IndexCount; index++)
	{
		uint32 rawindex = index & rawindexmask;
		if(rawindex == 0) {
			if((m_IndexCount & ~rawindexmask) == (index & ~rawindexmask))
				m_VideoFile->Read(indexbuffer, (m_IndexCount & rawindexmask) * sizeof(AVIRawIndex));
			else
				m_VideoFile->Read(indexbuffer, (rawindexmask+1) * sizeof(AVIRawIndex));
		}
	
		m_HeaderIndex[index].ChunkID = B_BENDIAN_TO_HOST_INT32(indexbuffer[rawindex].ChunkID);
		m_HeaderIndex[index].Flags = B_LENDIAN_TO_HOST_INT32(indexbuffer[rawindex].Flags);
		m_HeaderIndex[index].Offset = B_LENDIAN_TO_HOST_INT32(indexbuffer[rawindex].Offset);
		m_HeaderIndex[index].Length = B_LENDIAN_TO_HOST_INT32(indexbuffer[rawindex].Length);
		
		//printf("INDEX %4d: id %.4s flgs 0x%x offset 0x%x len %d\n", index,
		//	   &m_HeaderIndex[index].ChunkID, m_HeaderIndex[index].Flags,
		//	   m_HeaderIndex[index].Offset, m_HeaderIndex[index].Length);

		//	Check mimumum offset and adjust if neccessary
		if(m_HeaderIndex[index].Length > 0) {
			if ( m_HeaderIndex[index].Offset < minOffset) 
				minOffset = m_HeaderIndex[index].Offset;
		}
		
		if(m_BeOS_largefileblocks) {
			if(m_HeaderIndex[index].Offset < lastoffset) {
				offsetdelta += 0x100000000LL;
			}
			lastoffset = m_HeaderIndex[index].Offset;
			
			m_HeaderIndex[index].Offset += offsetdelta;
		}
					
		//DumpRIFFID(m_HeaderIndex[index].ChunkID);
	}
	free(indexbuffer);
	//	Calculate offset from either start of file or from
	//	start of the LIST movi.  Use best guess to determine
	//	as there is no set rule to do so.
	if (minOffset >= m_MoviChunkOffset) 
		m_MoviChunkOffset = 0;
		
  	//	Skip of ID and Size
  	m_MoviChunkOffset += 8;
  	
  	//	Jump over any extra bytes
	size -= (m_IndexCount << 4);	
	if ((int)size > 0)
		m_VideoFile->Seek(size, SEEK_CUR);
	
	//	Save position for later restore
	uint32 tmpPos = m_VideoFile->Position();

	//	Adjust offsets and get stream types
	int32 streamNum = -1;
	for (uint32 index = 0; index < m_IndexCount; index++)
	{
		m_HeaderIndex[index].Offset += m_MoviChunkOffset;
		
		if(m_HeaderIndex[index].ChunkID == kRiff_rec_Chunk) {
			// skip 'rec ' headers
			continue;
		}
		else {
			streamNum =
				((m_HeaderIndex[index].ChunkID & kRiff_FF00_Chunk) >> 16) - '00';
			if(streamNum < 0 || streamNum > 7) {
				ERROR("unknown chunk '%s' (0x%x) @ %d:: ", &m_HeaderIndex[index].ChunkID,
					  m_HeaderIndex[index].ChunkID, index);
				streamNum = -1; 
			}
		}
		
		uint32 	streamType;
		bool	streamOK;
		
		if (streamNum >= 0)
		{ 
			streamType = m_StreamType[streamNum];
			streamOK   = m_StreamOK[streamNum];
   		}
		else 
		{ 
			streamType = 0; 
			streamOK   = false; 
		}
		
		if (streamOK == false) 
		{
			ERROR("Readidx1Chunk::streamOK == false\n");
			continue;
		}
			
		//	Handle streamType
		switch(streamType)
		{
			case kRiff_vids_Chunk:
			{ 
				//PROGRESS("idx1 vids stream...\n");					
				retVal = AddVideoChunk(&m_HeaderIndex[index], streamNum);
			}
			break;
			
			case kRiff_auds_Chunk:
			{ 				
				//PROGRESS("idx1 auds stream...\n");
				// make sure we never see the high bit which is special for us
				m_HeaderIndex[index].Flags &= ~AVIIF_MUST_FREE;
				retVal = AddAudioChunk(&m_HeaderIndex[index], streamNum);
			}
			break;
			
			default:
				{
					DumpRIFFID(streamType);
					PROGRESS ("Readidx1Chunk::Unknown Stream Type\n");
				}
				break;
		}
	}
	
	//	Restore file position
	m_VideoFile->Seek(tmpPos, SEEK_SET);
	
	return retVal;	
}


//-------------------------------------------------------------------
//	ReadpcChunk
//-------------------------------------------------------------------
//
//	Read 'pc' Chunk
//

bool TRIFFReader::ReadpcChunk(uint32 size) 
{		
	size = 0;
	
	return false;
}

bool TRIFFReader::ReadINFOChunk(uint32 size)
{
	int pad = (size & 0x01);
					
	ReadIntLsb(m_VideoFile, sizeof(uint32));   // unknown data

	m_Copyright = (char *)malloc(size - 4 + 1);
	if (m_Copyright) {
		m_VideoFile->Read(m_Copyright, size - 4);
		if (pad)
			ReadIntLsb(m_VideoFile, sizeof(uint8));
	} else {
		m_VideoFile->Seek(size - 4 + pad, SEEK_CUR);
	}

	return true;
}

#pragma mark -
#pragma mark === AVI Header Routines ===

//-------------------------------------------------------------------
//	ReadAVIHeader
//-------------------------------------------------------------------
//
//

bool TRIFFReader::ReadAVIHeader()
{		
	m_AVIHeader.TimeBetweenFrames     	= ReadIntLsb(m_VideoFile, sizeof(uint32));
	m_AVIHeader.MaximumDataRate      	= ReadIntLsb(m_VideoFile, sizeof(uint32));
	m_AVIHeader.PaddingGranularity     	= ReadIntLsb(m_VideoFile, sizeof(uint32));
	m_AVIHeader.Flags        			= ReadIntLsb(m_VideoFile, sizeof(uint32));
	m_AVIHeader.TotalNumberOfFrames   	= ReadIntLsb(m_VideoFile, sizeof(uint32));
	m_AVIHeader.NumberOfInitialFrames	= ReadIntLsb(m_VideoFile, sizeof(uint32));
	m_AVIHeader.NumberOfStreams      	= ReadIntLsb(m_VideoFile, sizeof(uint32));
	m_AVIHeader.SuggestedBufferSize    	= ReadIntLsb(m_VideoFile, sizeof(uint32));
	m_AVIHeader.Width        			= ReadIntLsb(m_VideoFile, sizeof(uint32));
	m_AVIHeader.Height       			= ReadIntLsb(m_VideoFile, sizeof(uint32));
	if (m_AVIHeader.Height < 0)
		m_AVIHeader.Height = -m_AVIHeader.Height;
	
	m_AVIHeader.TimeScale        		= ReadIntLsb(m_VideoFile, sizeof(uint32));
	m_AVIHeader.DataRate         		= ReadIntLsb(m_VideoFile, sizeof(uint32));
	m_AVIHeader.StartTime        		= ReadIntLsb(m_VideoFile, sizeof(uint32));
	m_AVIHeader.DataLength       		= ReadIntLsb(m_VideoFile, sizeof(uint32));

	DumpAVIHeader(&m_AVIHeader);
		
	//	Success
	return true;
}

#pragma mark -
#pragma mark === AVI Stream Routines ===

//-------------------------------------------------------------------
//	AddVideoChunk
//-------------------------------------------------------------------
//
//

bool TRIFFReader::AddVideoChunk(AVIIndex *ptr, uint32 streamNum)
{
	bool retVal = true;
	
	//	Add to frame list
	m_VideoChunkList->AddItem(ptr);
	
	return(retVal);
}


//-------------------------------------------------------------------
//	Read00WCChunk
//-------------------------------------------------------------------
//
//	Compressed waveform data
//

bool TRIFFReader::AddAudioChunk(AVIIndex *ptr, uint32 streamNum)
{
	bool retVal = true;
	size_t length = ptr->Length;
	off_t offset = 0;
	
	//	Increment audio byte count
	m_AUDSHeader.ByteCount += length;
	
	if((int)length < 64*1024) {
		//	just add the pointer directly to the audio chunk list
		m_AudioChunkList->AddItem(ptr);
		return true;
	}

	// if we get here we have to break up the audio frame into
	// reasonable sized pieces...

	while(length > 64*1024) {
		AVIIndex *audioFrame = (AVIIndex *)malloc(sizeof(AVIIndex));
		ASSERT(audioFrame);

		//	Initialize
		audioFrame->ChunkID = ptr->ChunkID; 
		audioFrame->Length	= 64*1024;
		audioFrame->Offset 	= ptr->Offset + offset;
		audioFrame->Flags 	= AVIIF_MUST_FREE;  // mark it so we'll free it 

		//	Add to frame list
		m_AudioChunkList->AddItem(audioFrame);

		offset += 64*1024;
		length -= 64*1024;
	}

	if((int)length > 0) {
		//	Alocate frame
		AVIIndex *audioFrame = (AVIIndex *)malloc(sizeof(AVIIndex));
		ASSERT(audioFrame);

		//	Initialize
		audioFrame->ChunkID = ptr->ChunkID; 
		audioFrame->Length	= length;
		audioFrame->Offset 	= ptr->Offset + offset;
		audioFrame->Flags 	= AVIIF_MUST_FREE;
		
		//	Add to frame list
		m_AudioChunkList->AddItem(audioFrame);
	}

	return(retVal);
}

#pragma mark -
#pragma mark === Header Queries ===

//-------------------------------------------------------------------
//	VideoDepth
//-------------------------------------------------------------------
//
//

uint16 TRIFFReader::VideoDepth()
{
	if (m_HasVideo)
	{
		if(m_VIDSHeaderPtr)
			return m_VIDSHeaderPtr->BitCount;
	}
	//	No video stream
	return 0;
}


//-------------------------------------------------------------------
//	Width
//-------------------------------------------------------------------
//
//

uint32 TRIFFReader::Width()
{
	if (m_HasVideo)
	{
		if(m_VIDSHeaderPtr)
			return m_VIDSHeaderPtr->Width;
	}
	//	No video stream
	return 0;
}


//-------------------------------------------------------------------
//	Height
//-------------------------------------------------------------------
//
//

uint32 TRIFFReader::Height()
{
	if (m_HasVideo && m_VIDSHeaderPtr)
	{
		if (m_VIDSHeaderPtr->Height < 0)
			return -m_VIDSHeaderPtr->Height;
		else
			return m_VIDSHeaderPtr->Height;
	}
	//	No video stream
	else
	{
		return 0;
	}
}


//-------------------------------------------------------------------
//	IsBottomUp
//-------------------------------------------------------------------
//
//

bool TRIFFReader::IsBottomUp()
{
	if ( m_VIDSHeaderPtr && m_VIDSHeaderPtr->Height > 0)
		return true;
	else
		return false;
}

//-------------------------------------------------------------------
//	UsPerFrame
//-------------------------------------------------------------------
//
//	Milliseconds per frame
//

uint32 TRIFFReader::UsPerFrame()
{
	if (m_HasVideo)
	{
		return m_AVIHeader.TimeBetweenFrames;
	}
	//	No video stream
	else
	{
		return 0;
	}
}
