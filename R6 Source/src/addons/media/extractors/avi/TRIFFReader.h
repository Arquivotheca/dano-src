//---------------------------------------------------------------------
//
//	File:	TRIFFReader.h
//
//	Author:	Gene Z. Ragan
//
//	Date:	08.25.98
//
//	Desc:	RIFF File Format
//
//	Copyright (C) 1998 mediapede Software
//
//---------------------------------------------------------------------

#ifndef __TRIFFREADER_H__
#define __TRIFFREADER_H__

//	Includes
#include "RIFFConstants.h"
#include "RIFFTypes.h"

//	Constants
const int32 kRiffMaxStreams = 2;

// Prototypes
bool IsRIFFFile(BPositionIO *file);


//	Class Definition
class TRIFFReader
{
	public:
		TRIFFReader(BPositionIO *file);
		~TRIFFReader();
		
		bool	InitCheck();
		
		//	Header Queries
		uint16	VideoDepth();
		uint32 	Width();
		uint32 	Height();
		bool 	IsBottomUp();
		uint32 	UsPerFrame();
		
		//	Inlines Accessors
		inline const char 		*Copyright(){ return m_Copyright; }
		inline AVIIndex 		*GetIndex(){ return m_HeaderIndex; }
		inline uint32			IndexCount(){ return m_IndexCount; }
		inline AVIHeader		*GetAVIHeader(){ return &m_AVIHeader; };
		inline int32			GetStreamCount(){ return m_StreamCount; }
		inline AVIStreamHeader	*GetStreamHeaderOne(){ return &m_StreamHeaderOne; }
		inline AVIStreamHeader 	*GetStreamHeaderTwo(){ return &m_StreamHeaderTwo; }
		
		//	Video				
		inline bool				HasVideo(){ return m_HasVideo; }
		inline AVIVIDSHeader	*GetVIDSHeader(size_t *size){ *size = m_VIDSHeaderSize; return m_VIDSHeaderPtr; }
		inline BPositionIO		*GetVideoFile(){ return m_VideoFile; }
		inline BList			*VideoChunkList(){ return m_VideoChunkList; }
		inline uint32			CountVideoFrames(){ return m_VideoChunkList->CountItems(); }
		inline size_t 			VideoFrameIndex()  { return m_VideoFrameIndex; }
		
		//	Audio
		inline bool				HasAudio(){ return m_HasAudio; }
		inline AVIAUDSHeader	*GetAUDSHeader(){ return &m_AUDSHeader; }
		inline BPositionIO		*GetAudioFile(){ return m_AudioFile; }
		inline BList			*AudioChunkList(){ return m_AudioChunkList; }				
		inline int32			SamplingRate() { return m_AUDSHeader.SamplesPerSec ; }
		inline int32			CountChannels() { return m_AUDSHeader.Channels; }
		inline int32			SampleSize() { return m_SampleSize; };
		inline int32			ByteOrder() { return B_MEDIA_LITTLE_ENDIAN; }
		inline int32			AudioFrameSize() { return m_AudioFrameSize; }
		inline size_t			CountAudioFrames() { return m_AudioFrameCount; }
		inline size_t			CountAudioChunks() { return m_AudioChunkList->CountItems(); }
		inline size_t 			AudioFrameIndex()  { return m_AudioFrameIndex; }
		inline size_t			AudioFramesRemaining()  { return m_AudioFrameCount - m_AudioFrameIndex; }

	private:    

		//	Member Functions
		void	Init();
		bool 	ParseAVIFile();
		
		//	Chunk handling
		bool 	ReadRIFFChunk(uint32 size);
		bool	ReadLISTChunk(uint32 size);
		bool	ReadavihChunk(uint32 size);
		bool	ReadStreamHeader(uint32 size);
		bool	ReadstrdChunk(uint32 size);		
		bool	ReadStreamFormat(uint32 size);
		bool 	ReadvidsChunk(uint32 size);
		bool 	ReadaudsChunk(uint32 size);
		bool 	Readidx1Chunk(uint32 size);
		bool 	ReadpcChunk(uint32 size);
		bool	ReadINFOChunk(uint32 size);
		bool 	AddVideoChunk(AVIIndex *ptr, uint32 streamNum);
		bool 	AddAudioChunk(AVIIndex *ptr, uint32 streamNum);
		
		//	Header Reading
		bool	ReadAVIHeader();
		
		//	Member Variables
		char		*m_Copyright;
		BPositionIO	*m_AudioFile;
		BPositionIO	*m_VideoFile;
		bool		m_InitCheck;
		
		int64		m_AVISize;
		int			m_BeOS_largefileblocks;
		uint32		m_StreamCount;
		uint32		m_MoviChunkOffset;
		
		uint32		m_StreamType[kRiffMaxStreams];
		bool		m_StreamOK[kRiffMaxStreams];

		bool		m_UseIndexFlag;
		
		//	Video Variables
		bool			m_HasVideo;
		AVIVIDSHeader	*m_VIDSHeaderPtr;
		size_t			m_VIDSHeaderSize;
		uint32			m_IndexCount;
		size_t			m_VideoFrameIndex;
		
		//	Audio Variables		
		bool			m_HasAudio;				
		AVIAUDSHeader	m_AUDSHeader;
		uint32			m_AudioType;
		uint32			m_SampleSize;
		uint32			m_AudioFrameSize;
		uint32			m_AudioEnd;		
		uint32			m_TotalAudioTime;
		size_t			m_AudioFrameCount;
		uint32			m_AudioFPC;
		size_t			m_AudioFrameIndex;
		
		uchar 		*m_STRDBuffer;
		uint32 		m_STRDSize;
		uint32		m_STRDCurSize;
		
		//	Chunk headers
		AVIHeader		m_AVIHeader;
		AVIIndex 		*m_HeaderIndex;
		AVIStreamHeader	m_StreamHeaderOne;
		AVIStreamHeader	m_StreamHeaderTwo;		
						
		//	Data Lists
		BList *m_VideoChunkList;
		BList *m_AudioChunkList;
};

#endif
